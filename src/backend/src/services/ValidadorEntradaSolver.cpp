#include "backend/services/ValidadorEntradaSolver.hpp"

Resultado<SolverConfig> validarEntradaSolver(const QJsonObject& json)
{
    // V1–V13 viven en el parser del contrato; aquí solo se expone el punto de
    // entrada del flujo de generación.
    return SolverConfig::fromJson(json);
}
