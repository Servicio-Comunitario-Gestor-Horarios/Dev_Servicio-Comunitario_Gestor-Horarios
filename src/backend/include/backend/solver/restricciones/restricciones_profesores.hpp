#pragma once

#include "backend/solver/variables.hpp"
#include "backend/solver/config/solver_config.hpp"
#include "ortools/sat/cp_model.h"

using namespace operations_research::sat;

void restriccionDocenteNoSolapado (
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);

void restriccionProfesorPorTurno (
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);

void restriccionProfesorPorPlan (
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);

void agregarRestriccionProfesores (
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);
