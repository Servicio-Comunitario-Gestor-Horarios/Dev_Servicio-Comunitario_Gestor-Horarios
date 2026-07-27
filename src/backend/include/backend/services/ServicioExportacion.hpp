/**
 * @file ServicioExportacion.hpp
 * @brief Declaración del servicio de exportación de horarios a formatos CSV y JSON.
 */

#pragma once

#include <QJsonObject>
#include <QString>
#include "backend/data/output_schedule.hpp"
#include "backend/resultado.hpp"
#include "backend/solver/config/solver_config.hpp"

namespace backend::services {

/**
 * @class ServicioExportacion
 * @brief Proporciona métodos estáticos para transformar struct HorarioSalida
 * a representaciones legibles en CSV (por Cursos y Profesores) y JSON.
 */
class ServicioExportacion {
public:
    ServicioExportacion() = delete; // Clase utilitaria con métodos estáticos

    /**
     * @brief Exporta un horario a formato CSV agrupado por Cursos.
     * @param horario Datos del horario generados o cargados.
     * @param config Configuración del solver para la resolución de índices a nombres.
     * @return QString con el contenido formateado del CSV o un mensaje de error.
     */
    static Resultado<QString> exportarCsvCursos(
        const HorarioSalida& horario,
        const SolverConfig& config);

    /**
     * @brief Exporta un horario a formato CSV agrupado por Profesores.
     * @param horario Datos del horario generados o cargados.
     * @param config Configuración del solver para la resolución de índices a nombres.
     * @return QString con el contenido formateado del CSV o un mensaje de error.
     */
    static Resultado<QString> exportarCsvProfesores(
        const HorarioSalida& horario,
        const SolverConfig& config);

    /**
     * @brief Exporta un horario a su representación estructurada en JSON.
     * @param horario Datos del horario generados o cargados.
     * @return Objeto QJsonObject listo para ser serializado.
     */
    static QJsonObject exportarJson(const HorarioSalida& horario);

private:
    /**
     * @brief Escapa valores de texto para cumplir con las reglas del estándar CSV (RFC 4180).
     */
    static QString escaparCsv(const QString& campo);

    /**
     * @brief Retorna la representación en texto del día según el índice (0 = Lunes, ..., 4 = Viernes).
     */
    static QString obtenerNombreDia(int dia);
};

} // namespace backend::services
