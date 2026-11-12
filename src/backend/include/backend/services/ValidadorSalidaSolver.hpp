#pragma once

/**
 * @file ValidadorSalidaSolver.hpp
 * @brief Validaciones posteriores de la salida del solver (RF-3).
 *
 * Aplica las validaciones del contrato sobre el horario devuelto por el proceso
 * de cálculo:
 *  - P2: ningún profesor excede sus horas requeridas (aviso).
 *  - P4: sin conflictos de solapamiento (profesor, aula o curso).
 *
 * P2 genera avisos; P4 hace que el resultado no sea presentable. Es una función pura
 * (sin E/S) que el frontend consume para pintar el horario y sus avisos.
 *
 * La 005 (decisión I2) retiró los avisos de horas no cubiertas (P1) y de capacidad
 * de aula (P3): no se generan.
 */

#include <QString>
#include <QStringList>
#include <QVector>

#include "backend/data/horario_salida.hpp"
#include "backend/solver/config/solver_config.hpp"

/// Una línea del reparto de horas (020, RF-5): horas asignadas y tope de un docente.
struct LineaHorasDocente
{
    int     profesor = -1;
    QString nombre;
    int     horasAsignadas = 0;
    int     horasAula = 0;   ///< tope duro declarado (`horas_aula`)
};

/// Resultado de analizar la salida del solver.
struct AnalisisSalida
{
    QStringList conflictos;  ///< P4: solapamientos encontrados (no vacío ⇒ no presentable).
    QStringList avisos;      ///< P2: exceso de horas requeridas por el docente.

    /// 020 (RF-1/RF-5): docentes con tope duro de horas de aula 0 (declarado o ausente: el
    /// parser lee el campo ausente como 0). Es un dato informativo para no deshabilitarlos en
    /// silencio; no altera la presentabilidad del resultado.
    QStringList docentesSinCapacidad;

    /// 020 (RF-5): reparto observable — una línea por docente **con clases**.
    QVector<LineaHorasDocente> reparto;
    int totalHorasAsignadas = 0;   ///< Σ de las horas asignadas del reparto.
    int totalHorasAula = 0;        ///< Σ de los topes de horas de aula del centro.

    /// 020 (RF-5): P5 — docentes cuyas horas asignadas superan su `horas_aula`. No es un aviso:
    /// el resultado no es presentable.
    QStringList excesosTopeHoras;

    /// Un resultado es presentable si no hay solapamientos (P4) ni excesos del tope duro (P5).
    bool presentable() const { return conflictos.isEmpty() && excesosTopeHoras.isEmpty(); }

    /// Indica si el resultado tiene conflictos de solapamiento (P4). Se conserva para distinguir
    /// P4 de P5: un exceso de tope nunca es un solapamiento.
    bool hayConflictoSolapamiento() const { return !conflictos.isEmpty(); }

    /// Reparto en texto: una línea por docente («Nombre: X/Y h»), la línea de totales
    /// («Total del centro: A/S h») y los docentes sin capacidad. Vacío si no hay reparto.
    QString repartoTexto() const;
};

/// Analiza `horario` contra `config` (pura). No modifica sus argumentos.
AnalisisSalida analizarSalidaSolver(const HorarioSalida& horario, const SolverConfig& config);
