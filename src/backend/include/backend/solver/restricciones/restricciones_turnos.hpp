#pragma once

#include "backend/solver/variables.hpp"
#include "backend/solver/config/solver_config.hpp"
#include "ortools/sat/cp_model.h"

using namespace operations_research::sat;

/// D4: un curso no tiene dos clases a la vez.
void restriccionUnoPorTurno(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);

/// S1 (020, RF-1): tope DURO de horas de aula del profesor: Σ clases ≤ horas_aula, sin exceso.
void restriccionTopeHorasProfesor(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);

void agregarRestriccionesTurnos(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);
