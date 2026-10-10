#pragma once

/**
 * @file CargadorConfiguracionSolver.hpp
 * @brief Carga y guarda la configuración del solver y los presets como JSON (RF-1, RF-4).
 *
 * Un «preset» es un archivo JSON con el mismo formato que la configuración (las
 * 13 secciones del contrato del solver). No toca la base de datos ni depende de
 * OR-Tools: es lógica del lado cliente.
 */

#include <QString>
#include <QStringList>

#include "backend/resultado.hpp"
#include "backend/solver/config/solver_config.hpp"

class CargadorConfiguracionSolver
{
public:
    /// Lee un archivo JSON y lo valida con `SolverConfig::fromJson`.
    /// Archivo inexistente, ilegible o JSON inválido → `Resultado` con error.
    static Resultado<SolverConfig> cargar(const QString& ruta);

    /// Escribe la configuración como JSON legible en `ruta`.
    /// Ruta o carpeta inválida, o sin permisos → `Resultado` con error.
    static Resultado<bool> guardar(const QString& ruta, const SolverConfig& config);

    /// Lista los presets (`*.json`) de un directorio, ordenados por nombre.
    /// Directorio inexistente → `Resultado` con error.
    static Resultado<QStringList> listarPresets(const QString& directorio);
};
