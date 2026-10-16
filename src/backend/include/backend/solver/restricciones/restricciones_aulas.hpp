#pragma once

#include "backend/solver/variables.hpp"
#include "backend/solver/config/solver_config.hpp"
#include "ortools/sat/cp_model.h"

using namespace operations_research::sat;

/// D8: un aula no aloja dos clases a la vez (moviles por `y`, fijos por `x`).
void restriccionAulaNoSolapada(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);

/// D9: un curso movil ocupa exactamente un aula en cada franja con clase.
void restriccionCursoOcupaUnAula(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);

void agregarRestriccionesAulas(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);
