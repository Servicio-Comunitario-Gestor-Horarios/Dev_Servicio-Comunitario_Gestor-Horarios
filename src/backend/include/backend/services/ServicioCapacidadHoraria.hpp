#pragma once

/**
 * @file ServicioCapacidadHoraria.hpp
 * @brief Compara las horas de aula disponibles entre docentes con las que piden los cursos.
 *
 * La base no guarda las horas de aula por docente (el 36 es política del perfil) ni el plan
 * de cada curso: ambos los aporta el frontend. El detalle por plan, año y materia sí sale de
 * `PlanEstudio_Materia`. El servicio no modifica datos ni interviene en la generación.
 */

#include <QMap>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>
#include <QVector>

#include "backend/resultado.hpp"

/// Horas de aula semanales por docente cuando el frontend no envía su mapa.
inline constexpr int HORAS_AULA_POR_DOCENTE_POR_DEFECTO = 36;

/**
 * @brief Horas semanales de una materia en un año de un plan (fila de `PlanEstudio_Materia`).
 */
struct HorasPlanAnioMateriaDTO {
    QString codigoPlan;      ///< código del plan de estudio
    QString nombrePlan;      ///< nombre legible del plan
    int anio = 0;            ///< año/curso del plan (1..5)
    int idMateria = -1;      ///< id de la materia en el catálogo
    QString nombreMateria;   ///< nombre legible de la materia
    int horas = 0;           ///< horas semanales de esa materia en ese año
};

/**
 * @brief Resumen de capacidad: horas disponibles, horas necesarias y detalle por plan.
 *
 * Los datos incompletos (sin mapa de horas, curso sin plan, plan inexistente) no son error:
 * se informan en `avisos` y el cálculo sigue con lo que haya.
 */
struct ResumenCapacidadHorariaDTO {
    int docentes = 0;            ///< docentes registrados en la base
    int horasDisponibles = 0;    ///< Σ de las horas de aula del frontend (o 36 por docente)
    int cursos = 0;              ///< cursos registrados en la base
    int horasNecesarias = 0;     ///< Σ de las horas del plan de cada curso en su año
    QVector<HorasPlanAnioMateriaDTO> detalle;  ///< todas las cargas de todos los planes
    QStringList avisos;          ///< datos incompletos y entradas ignoradas, en español
};

/**
 * @brief Servicio de solo lectura para el resumen de capacidad horaria del centro.
 */
class ServicioCapacidadHoraria {
public:
    explicit ServicioCapacidadHoraria(QSqlDatabase& db);

    /**
     * @brief Todas las cargas de todos los planes, ordenadas por plan, año y materia.
     *
     * Devuelve error solo si falla una lectura de la base; un plan sin materias no es error.
     */
    Resultado<QVector<HorasPlanAnioMateriaDTO>> horasPorPlanAnioMateria() const;

    /**
     * @brief Resume horas disponibles y necesarias del centro.
     *
     * @param horasAulaPorDocente horas de aula por **nombre** de docente; vacío = 36 por docente
     *        (con aviso). Un nombre que no exista en la base se ignora con aviso.
     * @param planPorCurso plan de cada curso, con clave `nombreCurso(anio, seccion)`
     *        («1ro A»); vacío = no se calculan las horas necesarias (con aviso). Un curso sin
     *        plan o con un plan inexistente se omite con aviso.
     */
    Resultado<ResumenCapacidadHorariaDTO> resumen(
        const QMap<QString, int>& horasAulaPorDocente = {},
        const QMap<QString, QString>& planPorCurso = {}) const;

private:
    QSqlDatabase& m_db;
};
