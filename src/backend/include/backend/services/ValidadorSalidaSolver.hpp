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
 *  - S3: análisis de agrupación de bloques (grupos de 2–3 franjas contiguas).
 *
 * P1–P3 generan avisos; P4 hace que el resultado no sea presentable. Es una
 * función pura (sin E/S) que el frontend consume para pintar el horario y sus
 * avisos.
 */

#include <QString>
#include <QStringList>
#include <QVector>

#include "backend/data/horario_salida.hpp"
#include "backend/solver/config/solver_config.hpp"

/// Un grupo contiguo de una materia-curso en un día y segmento.
struct GrupoAgrupacion
{
    QString curso;        ///< Clave del curso ("año|sección").
    int materia = -1;     ///< Índice de materia.
    int dia = -1;         ///< Día (0 = lunes).
    int segmento = -1;    ///< Índice del segmento dentro del turno.
    int inicio = -1;      ///< Primera franja del grupo.
    int tamano = 0;       ///< Número de franjas contiguas.
    int penalizacion = 0; ///< Penalización S3 (0 si 2–3; 1 si 1 injustificado; n-3 si ≥4).
};

/// Resultado de analizar la salida del solver.
struct AnalisisSalida
{
    QStringList conflictos;  ///< P4: solapamientos encontrados (no vacío ⇒ no presentable).
    QStringList avisos;      ///< P1–P3: horas no cubiertas, exceso de horas/capacidad.

    /// S3: grupos contiguos detectados y penalización total.
    QVector<GrupoAgrupacion> grupos;
    int penalizacionAgrupacion = 0;

    /// Un resultado con conflictos de solapamiento nunca es presentable.
    bool presentable() const { return conflictos.isEmpty(); }

    /// Indica si el resultado tiene conflictos de solapamiento (P4).
    bool hayConflictoSolapamiento() const { return !conflictos.isEmpty(); }
};

/// Analiza `horario` contra `config` (pura). No modifica sus argumentos.
AnalisisSalida analizarSalidaSolver(const HorarioSalida& horario, const SolverConfig& config);
