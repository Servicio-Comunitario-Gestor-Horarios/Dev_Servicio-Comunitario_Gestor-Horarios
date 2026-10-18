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
    QString turno;            ///< Turno del curso ("" = sin turno). Obligatorio para el solver (V7);
                              ///< se aporta por el preset (la base es solo el respaldo).
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
 * @brief Franjas que ocupa un turno en la rejilla temporal.
 *
 * Los turnos se numeran sobre una misma recta de slots por día (p. ej. mañana
 * 0..7 y tarde 8..15), así que cada turno declara los suyos.
 */
struct TurnoVisual {
    QString nombre;        ///< Nombre del turno ("Mañana", "Tarde").
    QVector<int> franjas;  ///< Índices de slot del turno, en orden.
    QString inicio;        ///< Hora de inicio del turno ("HH:mm"), si se conoce.
};

/**
 * @brief Receso declarado para un turno.
 *
 * La duración no es fija (los datos de prueba usan 10 min, pero puede ser otra):
 * la visualización lee de aquí cuándo va y cuánto dura.
 */
struct RecesoVisual {
    QString turno;             ///< Turno al que pertenece.
    int despuesDeSlot = 0;     ///< Se inserta tras esa cantidad de franjas del turno.
    int duracion = 0;          ///< Minutos.
    QString inicio;            ///< "HH:mm" según el dominio, si se conoce.
    QString fin;               ///< "HH:mm" según el dominio, si se conoce.
};

/**
 * @brief Rejilla temporal con la que se generó el horario (RF-1).
 *
 * La sección de visualización dibuja la cuadrícula con estos datos en lugar de
 * una plantilla fija: duración real del bloque académico, franjas de cada turno
 * y recesos (posición y duración), tal como viajaron al solver.
 */
struct CuadriculaVisual {
    int duracionMinutos = 0;   ///< Duración de cada franja académica (min).
    int numDias = 5;           ///< Días planificados.
    int numSlotsDia = 0;       ///< Franjas por día.
    QVector<TurnoVisual> turnos;
    QVector<RecesoVisual> recesos;

    /// ¿Trae suficiente información para dibujar la rejilla?
    bool valida() const { return duracionMinutos > 0 && !turnos.isEmpty(); }
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

    /// Rejilla real (franjas y recesos) para la visualización.
    CuadriculaVisual cuadricula;
};

} // namespace GeneracionSolver
