#pragma once

/**
 * @file ServicioComposicionPlan.hpp
 * @brief Servicio para ensamblar planes de estudio para el solver usando tablas existentes.
 */

#include <QSqlDatabase>
#include <backend/resultado.hpp>
#include <backend/data/plan_estudio.hpp>
#include <backend/data/materia.hpp>

/**
 * @brief Servicio que compone un plan de estudio completo a partir de las tablas existentes:
 *        - PlanEstudio (tabla existente)
 *        - Materias (tabla existente)
 *        - PlanEstudio_Materia (tabla existente)
 */
class ServicioComposicionPlan {
public:
    explicit ServicioComposicionPlan(QSqlDatabase& db);

    /**
     * @brief Composición de plan de estudio completo.
     *
     * Lee las tablas PlanEstudio, Materias y PlanEstudio_Materia
     * para construir un PlanEstudio listo para el solver.
     *
     * @param codigoPlan Código del plan a componer.
     * @return Resultado con PlanEstudio completo o error.
     */
    Resultado<PlanEstudio> componerPlanCompleto(const QString& codigoPlan) const;

    /**
     * @brief Obtiene todas las materias de un plan con su carga horaria.
     *
     * @param codigoPlan Código del plan.
     * @return Vector de MateriaEnPlan con datos completos.
     */
    QVector<MateriaEnPlan> obtenerMateriasConCarga(const QString& codigoPlan) const;

    /**
     * @brief Lista todos los planes de estudio disponibles.
     *
     * @return Vector de nombres/códigos de planes.
     */
    QVector<QPair<QString, QString>> listarPlanesDisponibles() const;

private:
    QSqlDatabase& m_db;

    /**
     * @brief Obtiene los requisitos de una materia desde la tabla Materias.
     *
     * La tabla Materias tiene campo "requisitos" como TEXT.
     * Se parsea para obtener vector de QString.
     */
    QVector<QString> obtenerRequisitos(int materiaId) const;

    /**
     * @brief Verifica si un plan existe en la tabla PlanEstudio.
     */
    bool planExiste(const QString& codigoPlan) const;
};