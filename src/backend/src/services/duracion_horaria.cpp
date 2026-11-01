#include "backend/services/duracion_horaria.hpp"

#include <algorithm>

namespace duracion_horaria
{
    QString errorDeRango(int duracionMinutos)
    {
        if (duracionMinutos >= MINIMA && duracionMinutos <= MAXIMA)
            return QString();

        return QStringLiteral(
                   "La duración de la hora académica debe ser un entero entre 30 y 90 minutos; "
                   "se recibió %1. Ajuste la duración a un valor del rango (30–90 minutos).")
            .arg(duracionMinutos);
    }

    int minutosDeHorasAcademicas(int horasAcademicas, int duracionMinutos)
    {
        if (horasAcademicas < 0)
            return 0;

        if (!errorDeRango(duracionMinutos).isEmpty())
            return 0;

        return horasAcademicas * duracionMinutos;
    }

    int franjasUtilizables(int ventanaMinutos, int tiempoEventos, int duracionMinutos)
    {
        if (duracionMinutos <= 0 || ventanaMinutos <= 0)
            return 0;

        const int disponible = ventanaMinutos - std::max(0, tiempoEventos);
        if (disponible <= 0)
            return 0;

        const int franjas = disponible / duracionMinutos;
        return franjas > 0 ? franjas : 0;
    }

    int deficitDeEncaje(int numSlots, int ventanaMinutos, int tiempoEventos, int duracionMinutos)
    {
        if (duracionMinutos <= 0 || ventanaMinutos <= 0)
            return 0;

        const int ocupado = numSlots * duracionMinutos + std::max(0, tiempoEventos);
        return std::max(0, ocupado - ventanaMinutos);
    }
} // namespace duracion_horaria
