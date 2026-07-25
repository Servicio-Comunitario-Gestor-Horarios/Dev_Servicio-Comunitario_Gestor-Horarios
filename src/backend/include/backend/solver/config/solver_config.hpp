#pragma once

/**
 * @file solver_config.hpp
 * @brief Estructuras de configuración para el solver CP-SAT.
 *
 * Define las estructuras C++ que mapean el JSON de configuración de 13 secciones
 * utilizado por el motor de optimización de horarios.
 */

#include <QString>
#include <QVector>
#include <QJsonObject>
#include "backend/resultado.hpp"

/* Auxiliares */

/**
 * @brief Materia asignada a un curso con sus horas semanales.
 */
struct MateriaCurso {
    int materiaIDx;       ///< Índice de la materia en el array de materias
    int horasSemanales;   ///< Horas semanales requeridas para esta materia en el curso
};

/**
 * @brief Materia suplente con peso de preferencia.
 */
struct MateriaSuplente {
    int materiaIDx;  ///< Índice de la materia suplente
    int peso;        ///< Peso de preferencia (mayor = más preferido)
};

/**
 * @brief Disponibilidad de un profesor por día y slots.
 */
struct Disponibilidad {
    int dia;             ///< Índice del día (0-based)
    QVector<int> slot;   ///< Slots disponibles en ese día
};

/* Entidades */

/**
 * @brief Configuración de un curso para el solver.
 */
struct CursoSolverConfig {
    QString nombre;                ///< Nombre del curso (ej: "1ro A")
    QString turno;                 ///< Turno: "manana" o "tarde"
    int aula_fija = -1;            ///< Índice de aula fija, -1 si es móvil
    int num_estudiantes;           ///< Número de estudiantes
    QString plan;                  ///< Plan de estudios asociado
    QVector<MateriaCurso> materias; ///< Materias del curso con horas semanales
};

/**
 * @brief Configuración de un profesor para el solver.
 */
struct ProfesorSolverConfig {
    QString nombre;                        ///< Nombre del profesor
    int horas_requeridas;                  ///< Horas semanales requeridas
    int horas_planificacion = 4;           ///< Horas de planificación (default: 4)
    QVector<int> materias_asignadas;       ///< Índices de materias asignadas
    QVector<MateriaSuplente> materias_suplente; ///< Materias suplentes con peso
    QVector<Disponibilidad> disponibilidad;     ///< Disponibilidad por día/slot
};

/**
 * @brief Configuración de una materia para el solver.
 */
struct MateriaSolverConfig {
    QString nombre;  ///< Nombre de la materia
};

/**
 * @brief Configuración de un aula para el solver.
 */
struct AulaSolverConfig {
    QString nombre;   ///< Nombre del aula
    QString tipo;     ///< Tipo de aula (ej: "regular", "laboratorio")
    int capacidad;    ///< Capacidad máxima de estudiantes
};

/* Configuracion */

/**
 * @brief Dimensiones del problema de optimización.
 */
struct DimensionesConfig {
    int num_profesores = 0;  ///< Cantidad total de profesores
    int num_materias = 0;    ///< Cantidad total de materias
    int num_aulas = 0;       ///< Cantidad total de aulas
    int num_cursos = 0;      ///< Cantidad total de cursos
    int num_dias = 0;        ///< Cantidad de días de la semana
    int num_slots_dia = 0;   ///< Cantidad de slots por día
};

/**
 * @brief Configuración de la franja horaria.
 */
struct FranjaHorariaConfig {
    int duracion_minutos = 0;  ///< Duración de cada slot en minutos
    int slots_por_turno = 0;   ///< Cantidad de slots por turno
};

/**
 * @brief Configuración de un turno individual.
 */
struct TurnoConfig {
    QString nombre;           ///< Nombre del turno (ej: "manana", "tarde")
    QVector<int> slot;        ///< Rango de slots que cubre el turno
};

/**
 * @brief Configuración de todos los turnos.
 */
struct TurnosConfig {
    QVector<TurnoConfig> turnos;  ///< Lista de turnos configurados
};

/**
 * @brief Configuración de un período de receso.
 */
struct RecesoConfig {
    QString turno;           ///< Turno al que pertenece el receso
    int despues_de_slot;     ///< Slot después del cual se toma el receso
    int duracion;            ///< Duración del receso en minutos
    QString inicio;          ///< Hora de inicio (HH:MM)
    QString fin;             ///< Hora de fin (HH:MM)
};

/**
 * @brief Configuración de planificación.
 */
struct PlanificacionConfig {
    bool activa;  ///< Indica si la planificación está activa
};

/**
 * @brief Configuración de generación de horarios.
 */
struct GeneracionConfig {
    QVector<int> cursos_a_generar;  ///< Índices de cursos a procesar
    // TODO: Agregar campo para horario previo cuando se tengan los structs de salida del solver
};

/**
 * @brief Configuración de penalizaciones para soft constraints.
 */
struct PenalizacionConfig {
    int capacidad_aula = 100;        ///< Penalización por exceder capacidad de aula
    int emergencia_profesor = 50;    ///< Penalización por asignación de emergencia
};

/**
 * @brief Configuración completa del solver CP-SAT.
 *
 * Estructura raíz que contiene todas las 13 secciones del JSON de configuración.
 */
struct SolverConfig {
    QString version;                           ///< Versión del formato de configuración
    DimensionesConfig dimensiones;             ///< Dimensiones del problema
    FranjaHorariaConfig franja_horaria;        ///< Configuración de franja horaria
    TurnosConfig turnos;                       ///< Configuración de turnos
    QVector<RecesoConfig> recesos;             ///< Períodos de receso
    QVector<CursoSolverConfig> cursos;         ///< Cursos a procesar
    QVector<ProfesorSolverConfig> profesores;  ///< Profesores disponibles
    QVector<MateriaSolverConfig> materias;     ///< Materias del instituto
    QVector<AulaSolverConfig> aulas;           ///< Aulas disponibles
    PlanificacionConfig planificacion;         ///< Configuración de planificación
    GeneracionConfig generacion;               ///< Configuración de generación
    PenalizacionConfig penalizaciones;         ///< Configuración de penalizaciones

    /**
     * @brief Parsea un QJsonObject en un SolverConfig completo.
     * @param obj Objeto JSON con las 13 secciones de configuración.
     * @return Resultado con el SolverConfig en caso de éxito, o error con mensaje descriptivo.
     *
     * Aplica validaciones V1-V12:
     * - V1: JSON parseable y con secciones requeridas
     * - V2: Tipos correctos en cada sección
     * - V3: dimensiones coincide con longitudes de arrays
     * - V4: Índices de materias en rango
     * - V5: Índices de profesores en rango
     * - V6: Índices de aulas en rango
     * - V7: turno en {"manana", "tarde"} o ausente
     * - V8: aula_fija = -1, o índice válido, o ausente
     * - V9: num_estudiantes > 0
     * - V10: materias y profesores no vacíos
     * - V11: Un solo plan por curso
     * - V12: Disponibilidad en rango de slots
     */
    static Resultado<SolverConfig> fromJson(const QJsonObject& obj);
};
