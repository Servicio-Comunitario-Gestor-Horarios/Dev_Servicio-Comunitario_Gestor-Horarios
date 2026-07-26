#pragma once

#include "backend/solver/variables.hpp"
#include "backend/solver/config/solver_config.hpp"
#include "ortools/sat/cp_model.h"

using namespace operations_research::sat;

void agregarRestriccionProfesores(
    CpModelBuilder& model,
    const VariablesSolver& vars,
    const SolverConfig& config
);