#pragma once

/**
 * @file ServicioCargaHoraria.hpp
 * @brief Servicio para gestión de carga horaria de planes (tabla PlanEstudio_Materia existente).
 */

#include <QSqlDatabase>
#include <QVector>
#include <backend/resultado.hpp>

/** @brief Máximo de horas semanales permitidas por año. */
inline constexpr int MAX_HORAS_SEMANALES_POR_ANIO = 40;

/**
 * @brief DTO para carga horaria de un plan (tabla PlanEstudio_Materia existente).
 */
struct CargaHorariaDTO {
    int id = -1;                     // id_Materia
    QString codigoPlanEstudio;
    int idMateria = -1;
    int curso = 0;
    int horas = 0;

    /** @brief Nombre de la materia (llenado por join). */
    QString nombreMateria;
};

/**
 * @brief Servicio para operaciones sobre tabla PlanEstudio_Materia existente.
 */
class ServicioCargaHoraria {
public:
    explicit ServicioCargaHoraria(QSqlDatabase& db);

    // ─── CRUD ─────────────────────────────────────────────────────────────

    /**
     * @brief Asigna una materia a un plan con carga horaria específica.
     * @note Usa la tabla PlanEstudio_Materia existente.
     */
    Resultado<CargaHorariaDTO> asignarCarga(const QString& codigoPlan, int idMateria,
                                            int curso, int horas);

    /** @brief Obtiene un registro de carga por ID de materia. */
    Resultado<CargaHorariaDTO> obtenerCarga(int idMateria, const QString& codigoPlan) const;

    /** @brief Lista toda la carga horaria de un plan. */
    QVector<CargaHorariaDTO> listarCargaPorPlan(const QString& codigoPlan) const;

    /** @brief Actualiza las horas de un registro de carga. */
    Resultado<CargaHorariaDTO> actualizarCarga(int idMateria, const QString& codigoPlan, int horas);

    /** @brief Elimina un registro de carga. */
    bool eliminarCarga(int idMateria, const QString& codigoPlan);

    // ─── Consultas para el solver ────────────────────────────────────────

    /** @brief Calcula el total de horas semanales por año en un plan. */
    int totalHorasPorAnio(const QString& codigoPlan, int curso) const;

    /** @brief Verifica si la carga horaria de un año excede el máximo. */
    bool excedeLimiteHoras(const QString& codigoPlan, int curso, int horasAdicionales = 0) const;

private:
    QSqlDatabase& m_db;

    bool validarCarga(const QString& codigoPlan, int idMateria, int curso,
                      int horas, QString& error) const;

    CargaHorariaDTO mapearARecord(const QSqlRecord& record) const;
};