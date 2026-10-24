#include "backend/services/calculo_eventos.hpp"

#include <algorithm>

namespace
{
    /// Minutos de la ventana del turno; 0 si las horas no forman un intervalo válido.
    int minutosDeVentana(const TurnoDTO& turno)
    {
        if (!turno.inicio.isValid() || !turno.fin.isValid())
            return 0;

        return turno.inicio.secsTo(turno.fin) / 60;
    }

    /// Suma de las duraciones de todos los eventos (las no positivas no suman).
    int sumaDuraciones(const QVector<EventoDTO>& eventos)
    {
        int total = 0;
        for (const EventoDTO& evento : eventos)
        {
            if (evento.duracion > 0)
                total += evento.duracion;
        }
        return total;
    }

    /// Suma de las duraciones de los eventos situados antes de `posicion`.
    int duracionesAntesDe(const QVector<EventoDTO>& eventos, int posicion)
    {
        int total = 0;
        for (const EventoDTO& evento : eventos)
        {
            if (evento.despuesDeSlot < posicion && evento.duracion > 0)
                total += evento.duracion;
        }
        return total;
    }

    /// Suma de las duraciones de los eventos situados en `posicion`.
    int duracionesEn(const QVector<EventoDTO>& eventos, int posicion)
    {
        int total = 0;
        for (const EventoDTO& evento : eventos)
        {
            if (evento.despuesDeSlot == posicion && evento.duracion > 0)
                total += evento.duracion;
        }
        return total;
    }

    /// Orden estable de presentación: posición, orden e identidad.
    bool eventoAnterior(const EventoDTO& a, const EventoDTO& b)
    {
        if (a.despuesDeSlot != b.despuesDeSlot)
            return a.despuesDeSlot < b.despuesDeSlot;
        if (a.orden != b.orden)
            return a.orden < b.orden;
        return a.id < b.id;
    }
} // namespace

int duracionFranjaEstimada(const TurnoDTO& turno)
{
    if (turno.numSlots <= 0)
        return 0;

    const int ventana = minutosDeVentana(turno);
    if (ventana <= 0)
        return 0;

    const int restante = ventana - sumaDuraciones(turno.eventos);
    if (restante <= 0)
        return 0;

    const int franja = restante / turno.numSlots;
    return franja > 0 ? franja : 0;
}

QPair<QTime, QTime> ventanaDePosicion(const TurnoDTO& turno, int posicion)
{
    const QPair<QTime, QTime> sinVentana{QTime(), QTime()};

    if (posicion < 1 || posicion > turno.numSlots)
        return sinVentana;

    const int franja = duracionFranjaEstimada(turno);
    if (franja <= 0)
        return sinVentana;

    const int previas = duracionesAntesDe(turno.eventos, posicion);
    const int propias = duracionesEn(turno.eventos, posicion);

    const QTime inicio = turno.inicio.addSecs((posicion * franja + previas) * 60);
    QTime fin = inicio.addSecs(propias * 60);

    // La última posición termina con el turno (el resto del reparto cae ahí).
    if (posicion == turno.numSlots)
        fin = turno.fin;

    return QPair<QTime, QTime>(inicio, fin);
}

bool eventoDentroDeVentana(const EventoDTO& evento, const TurnoDTO& turno, int posicion)
{
    if (!evento.inicio.isValid() || !evento.fin.isValid())
        return true;

    const QPair<QTime, QTime> ventana = ventanaDePosicion(turno, posicion);
    if (!ventana.first.isValid() || !ventana.second.isValid())
        return false;

    return ventana.first <= evento.inicio && evento.fin <= ventana.second;
}

bool haySolapeCronologico(const QVector<EventoDTO>& eventos)
{
    QVector<EventoDTO> ordenados = eventos;
    std::stable_sort(ordenados.begin(), ordenados.end(), eventoAnterior);

    QTime finAnterior;
    bool hayAnterior = false;

    for (const EventoDTO& evento : ordenados)
    {
        if (!evento.inicio.isValid() || !evento.fin.isValid())
            continue;

        if (hayAnterior && evento.inicio < finAnterior)
            return true;

        if (!hayAnterior || evento.fin > finAnterior)
            finAnterior = evento.fin;
        hayAnterior = true;
    }

    return false;
}

int ordenSiguiente(const QVector<EventoDTO>& eventos, int posicion)
{
    int maximo = 0;
    for (const EventoDTO& evento : eventos)
    {
        if (evento.despuesDeSlot == posicion && evento.orden > maximo)
            maximo = evento.orden;
    }
    return maximo + 1;
}

QVector<EventoDTO> eventosFueraDeRango(const TurnoDTO& turno, int numSlots)
{
    QVector<EventoDTO> afectados;
    for (const EventoDTO& evento : turno.eventos)
    {
        if (evento.despuesDeSlot > numSlots)
            afectados.append(evento);
    }
    return afectados;
}
