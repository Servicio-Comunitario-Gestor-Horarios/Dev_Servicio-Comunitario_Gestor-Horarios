#include "backend/services/franja_horaria_validacion.hpp"

#include <QTime>

namespace
{
    /// Hora legible para el mensaje; «—» si no es válida.
    QString formatearHora(const QTime& hora)
    {
        return hora.isValid() ? hora.toString(QStringLiteral("HH:mm")) : QStringLiteral("—");
    }
} // namespace

QString franja_horaria::errorDeFranja(const FranjaHoraria& franja)
{
    if (franja.dia < DIA_MIN || franja.dia > DIA_MAX)
    {
        return QStringLiteral("El día debe ser un entero entre 0 (domingo) y 6 (sábado); se "
                              "recibió %1. Corrija el día.")
            .arg(franja.dia);
    }

    if (!franja.inicio.isValid() || !franja.fin.isValid())
    {
        return QStringLiteral("Las horas de la franja deben ser válidas (HH:mm); se recibió "
                              "%1–%2. Corrija el rango.")
            .arg(formatearHora(franja.inicio), formatearHora(franja.fin));
    }

    if (franja.inicio >= franja.fin)
    {
        return QStringLiteral("La hora de inicio (%1) debe ser anterior a la de fin (%2). "
                              "Corrija el rango.")
            .arg(formatearHora(franja.inicio), formatearHora(franja.fin));
    }

    return QString();
}
