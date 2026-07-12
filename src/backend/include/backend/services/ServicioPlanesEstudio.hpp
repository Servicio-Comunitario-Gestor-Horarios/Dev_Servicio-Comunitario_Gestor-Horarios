#pragma once

/**
 * @file ServicioPlanesEstudio.hpp
 * @brief Servicio para gestión de planes de estudio (tabla PlanEstudio existente).
 */

#include <QVector>
#include <backend/resultado.hpp>

/**
 * @brief DTO para un plan de estudio (tabla PlanEstudio existente).
 */
struct PlanDTO {
    QString codigo;
    QString nombre;
    QString descripcion;
};

/**
 * @brief Servicio para operaciones CRUD sobre la tabla PlanEstudio existente.
 */
class ServicioPlanesEstudio {
public:
    explicit ServicioPlanesEstudio(QSqlDatabase& db);

    // ─── CRUD ─────────────────────────────────────────────────────────────

    /** @brief Crea un nuevo plan de estudio. */
    Resultado<PlanDTO> crearPlan(const QString& codigo, const QString& nombre,
                                 const QString& descripcion = QString());

    /** @brief Obtiene un plan por su código. */
    Resultado<PlanDTO> obtenerPlan(const QString& codigo) const;

    /** @brief Lista todos los planes de estudio. */
    QVector<PlanDTO> listarPlanes() const;

    /** @brief Actualiza los datos de un plan. */
    Resultado<PlanDTO> actualizarPlan(const QString& codigo, const QString& nombre,
                                      const QString& descripcion = QString());

    /** @brief Elimina un plan por su código. */
    bool eliminarPlan(const QString& codigo);

private:
    QSqlDatabase& m_db;

    bool validarPlan(const QString& codigo, const QString& nombre, QString& error) const;
    PlanDTO mapearARecord(const QSqlRecord& record) const;
};