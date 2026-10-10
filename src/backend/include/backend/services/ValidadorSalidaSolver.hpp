#pragma once

/**
 * @file ValidadorSalidaSolver.hpp
 * @brief Validaciones posteriores de la salida del solver (RF-3).
 *
 * Aplica las validaciones del contrato sobre el horario devuelto por el proceso
 * de cálculo:
 *  - P1: todas las horas semanales de cada curso están cubiertas.
 *  - P2: ningún profesor excede sus horas requeridas.
 *  - P3: ninguna aula se usa por encima de su capacidad.
 *  - P4: sin conflictos de solapamiento (profesor, aula o curso).
 *
 * P1–P3 generan avisos; P4 hace que el resultado no sea presentable. Es una
 * función pura (sin E/S) que el frontend consume para pintar el horario y sus
 * avisos.
 */

#include <QStringList>

#include "backend/data/horario_salida.hpp"
#include "backend/solver/config/solver_config.hpp"

/// Resultado de analizar la salida del solver.
struct AnalisisSalida
{
    QStringList conflictos;  ///< P4: solapamientos encontrados (no vacío ⇒ no presentable).
    QStringList avisos;      ///< P1–P3: horas no cubiertas, exceso de horas/capacidad.

    /// Un resultado con conflictos de solapamiento nunca es presentable.
    bool presentable() const { return conflictos.isEmpty(); }

    /// Indica si el resultado tiene conflictos de solapamiento (P4).
    bool hayConflictoSolapamiento() const { return !conflictos.isEmpty(); }
};

/// Analiza `horario` contra `config` (pura). No modifica sus argumentos.
AnalisisSalida analizarSalidaSolver(const HorarioSalida& horario, const SolverConfig& config);
