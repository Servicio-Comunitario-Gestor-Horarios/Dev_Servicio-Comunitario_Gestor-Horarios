#pragma once

#include <QString>
#include <QVector>

/* Auxiciliares */
struct MateriaCurso { int materiaIDx; int horasSemanales; };
struct MateriaSuplente { int materiaIDx; int peso; };
struct Disponibilidad { int dia; QVector<int> slot; };

/* Entidades */
struct CursoSolverConfig {
    QString nombre;
    QString turno;
    int aula_fija = -1;
    int num_estudiantes;
    QString plan;
    QVector<MateriaCurso> materias;
};

struct ProfesorSolverConfig {
    QString nombre;
    int horas_requeridas;
    int horas_planificacion = 4;
    QVector<int> materias_asignadas;
    QVector<MateriaSuplente> materias_suplente;
    QVector<Disponibilidad> disponibilidad;
};

struct MateriaSolverConfig {
    QString nombre;
};

struct AulaSolverConfig {
    QString nombre;
    QString tipo;
    int capacidad;
};

/* Configuracion */
struct DimensionesConfig {
    int num_profesores = 0;
    int num_materias = 0;
    int num_aulas = 0;
    int num_cursos = 0;
    int num_dias = 0;
    int num_slots_dia = 0;
};

struct FranjaHorariaConfig {
    int duracion_minutos = 0;
    int slots_por_turno = 0;
};

struct TurnoConfig {
    QString nombre;
    QVector<int> slot;
};

struct TurnosConfig {
    QVector<TurnoConfig> turnos;
};

struct RecesoConfig {
    QString turno;
    int despues_de_slot;
    int duracion;
    QString inicio;
    QString fin;
};

struct PlanificacionConfig {
    bool activa;
};

struct GeneracionConfig {
    QVector<int> cursos_a_generar;
    // Falta crear la struct de resultados
};

struct PenalizacionConfig {
    int capacidad_aula = 100;
    int emergencia_profesor = 50;
};

/* Raiz */
struct SolverConfig {
    QString version;
    DimensionesConfig dimensiones;
    FranjaHorariaConfig franja_horaria;
    TurnosConfig turnos;
    QVector<RecesoConfig> recesos;
    QVector<CursoSolverConfig> cursos;
    QVector<ProfesorSolverConfig> profesores;
    QVector<MateriaSolverConfig> materias;
    QVector<AulaSolverConfig> aulas;
    PlanificacionConfig planificacion;
    GeneracionConfig generacion;
    PenalizacionConfig penalizaciones;
};