#pragma once

/**
 * @file restricciones_agrupacion.hpp
 * @brief Restricciones para la agrupación de bloques (S3).
 *
 * Implementa la penalización por fragmentación de materias en el horario:
 * favorece grupos de 2–3 franjas contiguas y penaliza grupos de 1 (salvo
 * 1h/día inevitable) y ≥4.
 */

#include "backend/solver/config/solver_config.hpp"
#include "backend/solver/variables.hpp"

namespace operations_research::sat {
class CpModelBuilder;
}

void agregarRestriccionesAgrupacion(
    operations_research::sat::CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars);
