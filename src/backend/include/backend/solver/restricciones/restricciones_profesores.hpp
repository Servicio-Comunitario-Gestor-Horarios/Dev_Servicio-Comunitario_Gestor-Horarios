#pragma once

#include "backend/solver/variables.hpp"
#include "backend/solver/config/solver_config.hpp"
#include "ortools/sat/cp_model.h"

using namespace operations_research::sat;

/// D5: un profesor no da dos clases a la vez.
void restriccionDocenteNoSolapado(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);

/// D1+D2+D3: un docente por materia-curso; solo el la dicta; con sus horas exactas.
void restriccionUnDocentePorMateria(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);

void agregarRestriccionProfesores(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);
