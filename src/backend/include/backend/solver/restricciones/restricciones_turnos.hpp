#pragma once

#include "backend/solver/variables.hpp"
#include "backend/solver/config/solver_config.hpp"
#include "ortools/sat/cp_model.h"

using namespace operations_research::sat;

void restriccionUnoPorTurno (
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);

void restriccionBloquesRequeridos (
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);

void restriccionHorasSemanalesProfesor (
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);

void agregarRestriccionesTurnos(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);
