#pragma once

/**
 * @file ValidadorEntradaSolver.hpp
 * @brief Validación previa del JSON de entrada del solver (RF-1).
 *
 * El lado cliente prepara el JSON de entrada y lo valida antes de enviarlo. Las
 * reglas V1–V13 viven en `SolverConfig::fromJson`; este punto de entrada las
 * expone como una operación explícita del flujo de generación y devuelve el
 * `SolverConfig` ya tipado cuando el JSON es válido.
 */

#include <QJsonObject>

#include "backend/resultado.hpp"
#include "backend/solver/config/solver_config.hpp"

/// Valida el JSON de entrada contra las reglas del contrato (V1–V13).
///
/// Devuelve `Resultado<SolverConfig>`: `ok` con la configuración tipada si es
/// válido; si no, `error` con el motivo (qué regla falló y por qué).
Resultado<SolverConfig> validarEntradaSolver(const QJsonObject& json);
