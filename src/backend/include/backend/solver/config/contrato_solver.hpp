#pragma once

/**
* @file contrato_solver.hpp
* @brief Funciones puras y constantes del contrato del solver
*
* Puras y deterministas: sin E/S, sin reloj y sin OR-Tools
*/

#include <QString>
#include <QVector>

#include "backend/solver/config/solver_config.hpp"

// Constantes del motor
namespace ContratoSolver {
    constexpr int PESO_HORAS = 1000;
    constexpr int COSTE_SUPLENTE = 100;
    constexpr int COSTE_MAX_SUPLENTE = 100;
}

int bloques(int horas, int duracion_minutos);

int bloquesAula(int horas_aula, int duracion_minutos);

QString claveCurso(int anio, const QString& seccion);

/// Clave normalizada del nombre de un tipo de aula: `trimmed().toCaseFolded()`.
///
/// Es la misma noción que usa el catálogo (`ServicioTiposAula::claveTipo`), pero pura: el
/// parser/validador del contrato y el modelo del solver comparan tipos por esta clave. Solo
/// sirve para comparar; el nombre visible se conserva tal cual (RF-5).
QString claveTipo(const QString& nombre);

QVector<PlanMateriaConfig> materiasDelCurso(const SolverConfig& config, const CursoSolverConfig& curso);

/// ¿El docente `prof` opta por la materia `materiaIDx` (titular o suplente declarado)?
bool docenteOptaPorMateria(const ProfesorSolverConfig& prof, int materiaIDx);

/// Slots (índices) del día `dia` en que el docente `prof` puede dar clase para el turno declarado
/// del curso `curso`: intersección de las franjas del turno del curso (D11), su disponibilidad y
/// el turno del propio docente (D6, solo si lo declara). Es la poda que decide qué `x` se crean.
QVector<int> slotsDisponibles(const SolverConfig& config, const CursoSolverConfig& curso,
                              const ProfesorSolverConfig& prof, int dia);

/// ¿El docente `prof` es candidato ELEGIBLE (factibilidad previa) de la materia `materiaIDx` del
/// curso `curso`? Exige: optar por la materia, respetar plan y año y tener al menos un bloque de
/// disponibilidad dentro de las franjas del turno declarado del curso (V13). NO exige capacidad de
/// horas: la capacidad es preferencia del objetivo, no una compuerta de elegibilidad.
bool docenteCandidatoElegible(const SolverConfig& config, const CursoSolverConfig& curso,
                              const ProfesorSolverConfig& prof, int materiaIDx);
