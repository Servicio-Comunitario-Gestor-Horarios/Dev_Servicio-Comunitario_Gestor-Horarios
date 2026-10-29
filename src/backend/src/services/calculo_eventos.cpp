#include "backend/services/calculo_eventos.hpp"

#include <algorithm>

#include "backend/services/duracion_horaria.hpp"

namespace
{
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

int minutosDeVentana(const TurnoDTO& turno)
{
    if (!turno.inicio.isValid() || !turno.fin.isValid())
        return 0;

    return turno.inicio.secsTo(turno.fin) / 60;
}

int tiempoDeEventos(const TurnoDTO& turno)
{
    int total = 0;
    for (const EventoDTO& evento : turno.eventos)
    {
        if (evento.duracion > 0)
            total += evento.duracion;
    }
    return total;
}

int franjasUtilizables(const TurnoDTO& turno, int duracionMinutos)
{
    return duracion_horaria::franjasUtilizables(minutosDeVentana(turno),
                                                tiempoDeEventos(turno), duracionMinutos);
}

int deficitDeEncaje(const TurnoDTO& turno, int duracionMinutos)
{
    return duracion_horaria::deficitDeEncaje(turno.numSlots, minutosDeVentana(turno),
                                             tiempoDeEventos(turno), duracionMinutos);
}

QString errorDeEncaje(const TurnoDTO& turno, int duracionMinutos)
{
    // El rango 30–90 es el del punto único (RNF-4); no se repite aquí.
    const QString errorRango = duracion_horaria::errorDeRango(duracionMinutos);
    if (!errorRango.isEmpty())
        return errorRango;

    const int ventana = minutosDeVentana(turno);
    if (ventana <= 0)
    {
        return QStringLiteral(
                   "El turno «%1» no declara una ventana válida de inicio y fin. Declare sus "
                   "horas (HH:mm) para poder comprobar el encaje con la hora académica.")
            .arg(turno.nombre);
    }

    if (turno.numSlots <= franjasUtilizables(turno, duracionMinutos))
        return QString();

    const int eventos = tiempoDeEventos(turno);
    const int total = turno.numSlots * duracionMinutos + eventos;

    return QStringLiteral(
               "El turno «%1» no encaja con una hora académica de %2 min: %3 franjas × %2 min + "
               "%4 min de eventos = %5 min y su ventana es de %6 min (déficit %7 min). Ajuste el "
               "número de franjas, la ventana o la duración de la hora académica.")
        .arg(turno.nombre)
        .arg(duracionMinutos)
        .arg(turno.numSlots)
        .arg(eventos)
        .arg(total)
        .arg(ventana)
        .arg(total - ventana);
}

QPair<QTime, QTime> ventanaDePosicion(const TurnoDTO& turno, int posicion,
                                      int duracionMinutos)
{
    const QPair<QTime, QTime> sinVentana{QTime(), QTime()};

    if (posicion < 1 || posicion > turno.numSlots)
        return sinVentana;

    if (duracionMinutos <= 0 || minutosDeVentana(turno) <= 0)
        return sinVentana;

    const int previas = duracionesAntesDe(turno.eventos, posicion);
    const int propias = duracionesEn(turno.eventos, posicion);

    const QTime inicio = turno.inicio.addSecs((posicion * duracionMinutos + previas) * 60);
    QTime fin = inicio.addSecs(propias * 60);

    // La última posición termina con el turno (el sobrante no se reparte).
    if (posicion == turno.numSlots)
        fin = turno.fin;

    return QPair<QTime, QTime>(inicio, fin);
}

bool eventoDentroDeVentana(const EventoDTO& evento, const TurnoDTO& turno, int posicion,
                           int duracionMinutos)
{
    if (!evento.inicio.isValid() || !evento.fin.isValid())
        return true;

    const QPair<QTime, QTime> ventana = ventanaDePosicion(turno, posicion, duracionMinutos);
    if (!ventana.first.isValid() || !ventana.second.isValid())
        return false;

    return ventana.first <= evento.inicio && evento.fin <= ventana.second;
}

// ─── Legado temporal (T4–T8): se retira en T9 ───────────────────────────────
// `duracionFranjaEstimada` conserva su fórmula de reparto y las firmas antiguas
// son envoltorios finos de las nuevas con D = franja estimada, para no duplicar
// la aritmética mientras T5, T6 y T8 migran a las firmas con D.

int duracionFranjaEstimada(const TurnoDTO& turno)
{
    if (turno.numSlots <= 0)
        return 0;

    const int ventana = minutosDeVentana(turno);
    if (ventana <= 0)
        return 0;

    const int restante = ventana - tiempoDeEventos(turno);
    if (restante <= 0)
        return 0;

    const int franja = restante / turno.numSlots;
    return franja > 0 ? franja : 0;
}

QPair<QTime, QTime> ventanaDePosicion(const TurnoDTO& turno, int posicion)
{
    return ventanaDePosicion(turno, posicion, duracionFranjaEstimada(turno));
}

bool eventoDentroDeVentana(const EventoDTO& evento, const TurnoDTO& turno, int posicion)
{
    return eventoDentroDeVentana(evento, turno, posicion, duracionFranjaEstimada(turno));
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
