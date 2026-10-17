#pragma once

/**
 * @file generacion_tipos.hpp
 * @brief Tipos compartidos por la vista de generación de horarios y su
 *        composición de la entrada del solver (header-only, sin dependencias
 *        del backend para poder incluirse siempre).
 */

#include <QMap>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QVector>

namespace GeneracionSolver {

/// Parámetros por curso que el usuario configura en la vista de generación.
struct ParametrosCurso {
    QString plan;             ///< Código del plan de estudio ("" = sin plan).
    int numEstudiantes = 0;   ///< Número de estudiantes del curso.
};

/// Parámetros por docente que el usuario configura en la vista de generación.
struct ParametrosDocente {
    int horasRequeridas = 0;      ///< Horas semanales requeridas (> 0).
    int horasAula = 0;            ///< Horas reales en aula (> 0 y <= requeridas).
    QString turno;                ///< Turno preferido ("" = indiferente).
    QString plan;                 ///< Plan preferido ("" = indiferente).
    QVector<int> anios;           ///< Años en que puede dictar (vacío = cualquiera).
    QStringList materiasSuplentes;///< Nombres de materias suplentes (opcional).
};

/**
 * @brief Estado completo de configuración que expone la vista de generación.
 *
 * Reúne todo lo que el motor necesita y que no vive en la base de datos:
 * franja horaria, dimensiones, la opción global de aulas fijas y los parámetros
 * por curso/docente. Las materias y sus horas ya no viajan aquí: el solver las
 * deriva del plan embebido de cada curso, y los slots de cada turno se calculan
 * a partir de los turnos definidos en la base (no se configuran aparte).
 */
struct OpcionesUI {
    QString version = QStringLiteral("1.0");

    // Franja horaria.
    int duracionMinutos = 45;   ///< Duración de cada slot (min).

    // Dimensiones.
    int numDias = 5;            ///< Días de la semana a planificar.
    int numSlotsDia = 12;       ///< Slots por día (0 = se deriva de los turnos).

    // Opción global: ¿usar el aula fija asignada a cada curso en el dominio?
    bool usarAulasFijas = true;

    /// Parámetros por nombre de curso (p. ej. "1ro A").
    QMap<QString, ParametrosCurso> cursos;
    /// Parámetros por nombre de docente.
    QMap<QString, ParametrosDocente> docentes;
};

/**
 * @brief Contexto de nombres/índices de la última entrada construida.
 *
 * El HorarioSalida del solver referencia materias, profesores y aulas por
 * índice; este contexto permite traducirlos a texto al visualizar.
 */
struct ContextoHorario {
    QStringList materias;    ///< Índice de materia -> nombre.
    QStringList profesores;  ///< Índice de profesor -> nombre.
    QStringList aulas;       ///< Índice de aula -> nombre.

    /// Clave de curso del solver (`claveCurso`, "año|sección") -> (año, sección).
    QMap<QString, QPair<int, QString>> cursos;
};

} // namespace GeneracionSolver
