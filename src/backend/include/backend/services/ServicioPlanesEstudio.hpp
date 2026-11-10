#pragma once

/**
 * @file ServicioPlanesEstudio.hpp
 * @brief Servicio para gestión de planes de estudio (tabla PlanEstudio existente).
 */

#include <QSqlDatabase>
#include <QVector>
#include <backend/resultado.hpp>
#include <backend/services/ServicioCargaHoraria.hpp>

/**
 * @brief DTO para un plan de estudio (tabla PlanEstudio existente).
 */
struct PlanDTO {
    QString codigo;
    QString nombre;
    QString descripcion;
};

/**
 * @brief Materia asociada a un plan, con su año y horas semanales.
 *
 * Los planes nuevos creados desde la interfaz son «globales»: todas sus
 * materias se guardan con `curso = 1` y la composición para el solver las
 * replica en los años 1–5. Los planes existentes conservan el curso real de
 * cada materia.
 */
struct PlanMateriaDTO {
    int idMateria = -1;
    int curso = 1;
    int horas = 2;
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

    /** @brief Lista todos los planes de estudio. Error y vacío se distinguen. */
    Resultado<QVector<PlanDTO>> listarPlanes() const;

    /** @brief Actualiza los datos de un plan. */
    Resultado<PlanDTO> actualizarPlan(const QString& codigo, const QString& nombre,
                                      const QString& descripcion = QString());

    /**
     * @brief Crea un plan y sus materias dentro de una sola transacción.
     *
     * Valida que el código sea numérico y único, que el nombre no esté repetido,
     * que haya al menos una materia sin duplicados y que ningún año supere el
     * máximo de horas semanales. Si algo falla no deja el plan a medias.
     */
    Resultado<PlanDTO> crearPlanCompleto(const QString& codigo, const QString& nombre,
                                         const QString& descripcion,
                                         const QVector<PlanMateriaDTO>& materias);

    /**
     * @brief Actualiza un plan y reemplaza sus materias de forma atómica.
     *
     * El código es la identidad del plan y no se modifica. Las materias se
     * reemplazan por la lista recibida (misma validación que el alta).
     */
    Resultado<PlanDTO> actualizarPlanCompleto(const QString& codigo, const QString& nombre,
                                              const QString& descripcion,
                                              const QVector<PlanMateriaDTO>& materias);

    /** @brief Elimina un plan por su código. */
    bool eliminarPlan(const QString& codigo);

private:
    QSqlDatabase& m_db;

    bool validarPlan(const QString& codigo, const QString& nombre, QString& error) const;

    /// Valida la lista de materias del plan (no vacía, sin duplicados, horas y tope anual).
    bool validarMaterias(const QVector<PlanMateriaDTO>& materias, QString& error) const;

    /// Comprueba que el nombre no lo use ya otro plan (ignorando `codigoExcluido`).
    bool nombrePlanDisponible(const QString& nombre, const QString& codigoExcluido,
                              QString& error) const;

    /// Comprueba que la materia exista en el catálogo global.
    bool materiaExiste(int idMateria) const;

    /// Indica si `codigo` está formado solo por dígitos (código numérico).
    static bool esCodigoNumerico(const QString& codigo);

    PlanDTO mapearARecord(const QSqlRecord& record) const;
};