#pragma once

/**
 * @file solver_config.hpp
 * @brief Estructuras de configuración del solver CP-SAT (contrato nuevo, spec 005).
 *
 * El curso se identifica por `anio`+`seccion`; su `plan` (obligatorio) es la fuente de
 * sus materias y horas; los planes van EMBEBIDOS; el modo `aula_fija` es GLOBAL; el
 * docente puede limitarse a unos `anios`. `horas_requeridas` viaja como dato de dominio
 * (el solver no lo usa).
 */

#include <QJsonObject>
#include <QString>
#include <QVector>

#include "backend/resultado.hpp"

/* Auxiliares */
struct MateriaCurso    { int materiaIDx = 0; int horasSemanales = 0; };
struct MateriaSuplente { int materiaIDx = 0; int peso = 0; };
struct Disponibilidad  { int dia = 0; QVector<int> slot; };

/* Planes embebidos */
struct PlanMateriaConfig { int materiaIDx = 0; int curso = 0; int horasSemanales = 0; };
struct PlanSolverConfig  { QString codigo; QVector<PlanMateriaConfig> materias; };

/* Entidades */
struct CursoSolverConfig {
    int     anio = 0;
    QString seccion;
    QString turno;                 ///< obligatorio para generar (V7)
    int     num_estudiantes = 0;   ///< opcional
    QString plan;                  ///< codigo (obligatorio)
    int     aula = -1;             ///< indice de aula (o -1); se usa con el modo global
};

struct ProfesorSolverConfig {
    QString nombre;
    int horas_requeridas = 0;      ///< opcional: dato de dominio (coordinacion/planificacion);
                                   ///< el solver NO lo usa
    int horas_aula = 0;            ///< tope de horas de aula (obligatorio)
    QString turno;                 ///< "" = cualquiera
    QString plan;                  ///< "" = cualquiera
    QVector<int> anios;            ///< vacio = cualquiera
    QVector<int> materias_asignadas;
    QVector<MateriaSuplente> materias_suplente;
    QVector<Disponibilidad> disponibilidad;
};

struct MateriaSolverConfig { QString nombre; };
struct AulaSolverConfig    { QString nombre; QString tipo; int capacidad = 0; };

/* Configuracion */
struct DimensionesConfig   { int num_profesores = 0, num_materias = 0, num_aulas = 0,
                                 num_cursos = 0, num_dias = 0, num_slots_dia = 0; };
struct FranjaHorariaConfig { int duracion_minutos = 0, slots_por_turno = 0; };
struct TurnoConfig         { QString nombre; QVector<int> slot; };
struct TurnosConfig        { QVector<TurnoConfig> turnos; };
struct RecesoConfig        { QString turno; int despues_de_slot = 0, duracion = 0;
                             QString inicio; QString fin; };

struct SolverConfig {
    QString version;
    bool aula_fija = true;                     ///< modo GLOBAL
    DimensionesConfig dimensiones;
    FranjaHorariaConfig franja_horaria;
    TurnosConfig turnos;
    QVector<RecesoConfig> recesos;
    QVector<PlanSolverConfig> planes;          ///< embebidos (solo los referenciados)
    QVector<CursoSolverConfig> cursos;
    QVector<ProfesorSolverConfig> profesores;
    QVector<MateriaSolverConfig> materias;
    QVector<AulaSolverConfig> aulas;

    static Resultado<SolverConfig> fromJson(const QJsonObject& obj);
    QJsonObject toJson() const;
};
