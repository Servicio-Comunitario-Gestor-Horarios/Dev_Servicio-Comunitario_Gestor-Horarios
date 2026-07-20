#pragma once

/**
 * @file ServicioConsultaHorario.hpp
 * @brief Servicio de consulta y análisis de horarios de salida.
 *
 * Operaciones de solo lectura sobre un HorarioSalida en memoria.
 * No realiza E/S de archivos.
 */

#include <QVector>
#include <QMap>
#include <QPair>
#include <QString>
#include <backend/resultado.hpp>
#include <backend/data/output_schedule.hpp>

/**
 * @brief Asignación junto con su contexto de ubicación en el horario.
 */
struct AsignacionConContexto {
    AsignacionOutput asignacion;
    QString curso;
    int dia = -1;
};

/**
 * @brief Par de asignaciones en conflicto.
 */
struct ParConflicto {
    AsignacionConContexto primera;
    AsignacionConContexto segunda;
    QString tipo; // "profesor" o "aula"
};

/**
 * @brief Servicio de consulta de horarios de salida.
 *
 * Proporciona operaciones de filtrado, detección de conflictos y
 * cálculo de estadísticas sobre un HorarioSalida en memoria.
 */
class ServicioConsultaHorario {
public:
    explicit ServicioConsultaHorario(const HorarioSalida& horario);

    // ─── Consultas por filtro ───────────────────────────────────────────

    /** @brief Busca todas las asignaciones de un profesor por índice. */
    Resultado<QVector<AsignacionConContexto>> buscarPorProfesor(int profesorIdx) const;

    /** @brief Busca todas las asignaciones de un aula por índice. */
    Resultado<QVector<AsignacionConContexto>> buscarPorAula(int aulaIdx) const;

    /** @brief Busca todas las asignaciones de un día. */
    Resultado<QVector<AsignacionConContexto>> buscarPorDia(int dia) const;

    /** @brief Busca todas las asignaciones de un curso por nombre. */
    Resultado<QVector<AsignacionConContexto>> buscarPorCurso(const QString& curso) const;

    // ─── Detección de conflictos ───────────────────────────────────────

    /** @brief Verifica si una asignación específica entraria en conflicto. */
    Resultado<bool> hayConflicto(const QString& curso, int dia, int slot) const;

    /** @brief Detecta todos los pares de asignaciones en conflicto. */
    Resultado<QVector<ParConflicto>> detectarConflictos() const;

    // ─── Estadísticas ──────────────────────────────────────────────────

    /** @brief Calcula estadísticas completas del horario. */
    Resultado<EstadisticasHorario> obtenerEstadisticas() const;

private:
    const HorarioSalida& m_horario;

    /** @brief Recopila todas las asignaciones con su contexto. */
    QVector<AsignacionConContexto> recopilarAsignaciones() const;
};
