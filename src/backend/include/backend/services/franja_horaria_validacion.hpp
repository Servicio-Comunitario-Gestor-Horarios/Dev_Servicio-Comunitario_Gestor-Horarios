#pragma once

/**
 * @file franja_horaria_validacion.hpp
 * @brief Validación compartida de día y rango de una franja horaria (RF-1, RF-2, RNF-1).
 *
 * Sin E/S y determinista. Es la **única** definición de las reglas del día (0–6) y del
 * rango (inicio estrictamente anterior al fin): la usan la disponibilidad y las
 * preferencias del docente, de modo que no haya dos mensajes ni dos fuentes de verdad.
 */

#include <QString>

#include "backend/data/franja_horaria.hpp"

namespace franja_horaria
{
    /// Día mínimo admitido (0 = domingo).
    constexpr int DIA_MIN = 0;

    /// Día máximo admitido (6 = sábado).
    constexpr int DIA_MAX = 6;

    /// Mensaje de rechazo de la franja, o cadena vacía si es válida.
    ///
    /// Comprueba el día 0–6 citando el valor recibido, que las horas sean válidas y que
    /// el inicio sea anterior al fin; cada mensaje dice qué corregir (RNF-1).
    QString errorDeFranja(const FranjaHoraria& franja);
} // namespace franja_horaria
