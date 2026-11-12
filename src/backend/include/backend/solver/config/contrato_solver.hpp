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
#include "qcontainerfwd.h"

// Constantes del motor
namespace ContratoSolver {
    /// HISTÓRICO (020, RF-1): peso de S1 (exceso de horas de aula) del comportamiento
    /// anterior. El tope de horas es ahora una invariante DURA y S1 no aporta al objetivo:
    /// la constante y su `static_assert` se conservan solo para no romper referencias y
    /// dejar documentada la jerarquía retirada.
    constexpr int PESO_HORAS = 1000;
    constexpr int COSTE_SUPLENTE = 100;
    constexpr int COSTE_MAX_SUPLENTE = 100;
    /// Preferencia horaria del docente (013, RF-5): por debajo de S2.
    /// No viaja en la configuración: es una constante del motor.
    constexpr int PESO_PREFERENCIA = 25;

    // Jerarquía por unidad: S1 (histórico) > S2 > preferencia (RF-5). La igualdad de S2 entre
    // fases la garantiza la fase 2 por anclaje (el orden por unidad no basta con un objetivo
    // aditivo). Con el tope duro, S2 y la preferencia son los únicos términos activos.
    static_assert(PESO_HORAS > COSTE_SUPLENTE,
                  "S1 (PESO_HORAS, histórico) debe pesar más que S2 (COSTE_SUPLENTE)");
    static_assert(COSTE_SUPLENTE > PESO_PREFERENCIA,
                  "la preferencia (PESO_PREFERENCIA) debe quedar por debajo de S2 (COSTE_SUPLENTE)");
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

/// Aforo simultáneo del tipo `clave` (018, RF-1): cancha → 2; cualquier otro → 1.
/// Envuelve `aforo_aula::aforoDeClave` (una sola verdad).
int aforoDelTipo(const QString& clave);

/// Aforo del aula `indice` (018): puente índice → tipo → clave → aforo.
/// Un índice fuera de rango devuelve 1 (RF-7): nunca abre un hueco en la comprobación.
int aforoDelAula(const SolverConfig& config, int indice);

/// Posiciones de par de un turno (018, RF-5): Σ ⌊|segmento|/2⌋ sobre `segmentosTurno`.
/// En el centro, 4 por día (1 en el primer segmento + 3 en el segundo).
int posicionesDePar(const TurnoConfig& turno, const QVector<EventoConfig>& eventos);

/// Capacidad del tipo `clave`: `aulasDelTipo × |franjasDelTipo| × aforoDelTipo` (018, RF-6).
int capacidadDelTipo(const SolverConfig& config, const QString& clave);

/// Demanda total de horas académicas del centro (020, RF-2): Σ bloques de las materias del plan
/// de todos los cursos. Una hora académica = un bloque; la duración de la franja no interviene.
int demandaTotalHoras(const SolverConfig& config);

/// Suma de los topes duros de horas de aula del centro (020, RF-2): Σ `bloquesAula(horas_aula)`
/// de todos los docentes. Un docente sin capacidad (tope 0, declarado o ausente) suma 0, y un
/// tope negativo no resta.
int sumaTopesHorasAula(const SolverConfig& config);

/// Tipo de aula general/ordinario del centro, por la convención de nombre única:
/// el tipo general. La definición del literal vive en `common/tipo_general.hpp`.
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

/// Devuelve los segmentos contiguos de un turno, cortados por los eventos.
/// Cada segmento es un vector de slots consecutivos.
QVector<QVector<int>> segmentosTurno(const TurnoConfig& turno, const QVector<EventoConfig>& eventos);
