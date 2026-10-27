#pragma once

/**
* @file contrato_solver.hpp
* @brief Funciones puras y constantes del contrato del solver
*
* Puras y deterministas: sin E/S, sin reloj y sin OR-Tools
*/

#include <QHashFunctions>
#include <QSet>
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

/// Una franja de la semana: día y slot dentro del día.
///
/// La unidad de las cuentas de capacidad es el par (día, slot), no el slot repetido por día. Con
/// `==`, `<` y `qHash` sirve en `QSet` y `QHash` sin ambigüedad de unidades (RF-6).
struct FranjaSemana {
    int dia = 0;
    int slot = 0;
};

inline bool operator==(const FranjaSemana& a, const FranjaSemana& b) {
    return a.dia == b.dia && a.slot == b.slot;
}

/// Orden lexicográfico: primero el día y después la franja (determinismo de los recorridos).
inline bool operator<(const FranjaSemana& a, const FranjaSemana& b) {
    return a.dia != b.dia ? a.dia < b.dia : a.slot < b.slot;
}

inline size_t qHash(const FranjaSemana& franja, size_t seed = 0) noexcept {
    return qHashMulti(seed, franja.dia, franja.slot);
}

/// Slots declarados por el turno `turno`, o vacío si no existe.
QVector<int> slotsTurno(const SolverConfig& config, const QString& turno);

/// Franjas semanales del curso: `{ (d, s) : d < num_dias ∧ s ∈ slotsTurno(curso.turno) }`.
QSet<FranjaSemana> franjasDelCurso(const SolverConfig& config, const CursoSolverConfig& curso);

/// Nº de franjas semanales del curso: `|franjasDelCurso|` (días × franjas de su turno).
int franjasSemanales(const SolverConfig& config, const CursoSolverConfig& curso);

/// Unión de `franjasDelCurso` de los cursos con al menos una materia del tipo `clave`.
QSet<FranjaSemana> franjasDelTipo(const SolverConfig& config, const QString& clave);

/// Bloques de las materias de todos los cursos cuyo tipo de aula tiene la clave `clave`.
int demandaDelTipo(const SolverConfig& config, const QString& clave);

/// Nº de aulas del centro del tipo `clave`.
int aulasDelTipo(const SolverConfig& config, const QString& clave);

/// Capacidad del tipo `clave`: `aulasDelTipo × |franjasDelTipo|`.
int capacidadDelTipo(const SolverConfig& config, const QString& clave);

/// Tipo de aula general/ordinario del centro, por convención de nombre: «Aula de estudio».
QString tipoGeneralAula();

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
