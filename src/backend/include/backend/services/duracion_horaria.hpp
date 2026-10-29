#pragma once

/**
 * @file duracion_horaria.hpp
 * @brief Punto único de la duración de la hora académica (RF-1, RF-3, RNF-4).
 *
 * La duración D es global por base, 35 minutos por defecto y rango 30–90. Este
 * módulo concentra la validación del rango, la única conversión de horas
 * académicas a minutos y la aritmética entera del encaje del turno
 * (`nº franjas × D + tiempo de eventos ≤ ventana`).
 *
 * Sin E/S, sin reloj y determinista: no incluye `QSql*` ni consulta la
 * configuración; la D efectiva la recibe siempre el llamador. Ninguna otra
 * pieza del backend repite estas cuentas.
 */

#include <QString>

namespace duracion_horaria
{
    /// Valor por defecto de la hora académica (Educación Básica, Gaceta 42.505).
    constexpr int POR_DEFECTO = 35;

    /// Duración mínima aceptada, en minutos (extremo inclusivo).
    constexpr int MINIMA = 30;

    /// Duración máxima aceptada, en minutos (extremo inclusivo).
    constexpr int MAXIMA = 90;

    /// Mensaje de rechazo del rango 30–90, o cadena vacía si la duración es válida.
    ///
    /// Es **el** punto único de validación del rango: lo usan la configuración
    /// global y la validación del contrato (RF-1, RNF-4). El mensaje identifica
    /// el campo, el valor recibido, el rango y la acción disponible (RNF-1).
    QString errorDeRango(int duracionMinutos);

    /// Minutos de `horasAcademicas` horas académicas: `horasAcademicas × D`.
    ///
    /// Es **el** punto único de conversión horas académicas → minutos (RF-3).
    /// Devuelve 0 si las horas son negativas o la duración no es válida.
    int minutosDeHorasAcademicas(int horasAcademicas, int duracionMinutos);

    /// Franjas que caben en un turno: `floor((ventana − eventos) / D)`.
    ///
    /// Devuelve 0 si la duración o la ventana no son positivas o si no cabe
    /// ninguna franja completa (RF-4).
    int franjasUtilizables(int ventanaMinutos, int tiempoEventos, int duracionMinutos);

    /// Minutos que faltan para que el turno encaje, o 0 si ya cabe.
    ///
    /// `max(0, numSlots × D + tiempoEventos − ventana)`; el sobrante de la
    /// ventana no se reparte entre las franjas (RF-4). Devuelve 0 si la duración
    /// o la ventana no son positivas.
    int deficitDeEncaje(int numSlots, int ventanaMinutos, int tiempoEventos, int duracionMinutos);
} // namespace duracion_horaria
