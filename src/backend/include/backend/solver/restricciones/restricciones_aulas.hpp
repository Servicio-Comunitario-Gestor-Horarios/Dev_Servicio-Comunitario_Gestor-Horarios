#pragma once

#include "backend/solver/variables.hpp"
#include "backend/solver/config/solver_config.hpp"
#include "ortools/sat/cp_model.h"

using namespace operations_research::sat;

/// D8: un aula no aloja dos clases a la vez (por la `y` de todos los cursos, desplazadas incluidas).
void restriccionAulaNoSolapada(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);

/// D9 ampliada: toda clase (fija o móvil) ocupa exactamente un aula.
void restriccionCursoOcupaUnAula(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);

/// RF-3: la clase se imparte en un aula cuyo tipo coincide con el requerido por su materia.
void restriccionTipoExacto(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);

/// RF-3: aula de referencia (coincidencia), desplazamiento y referencia vacía y bloqueada.
/// Solo actúa con el modo de aula fija activo.
void restriccionAulaDeReferencia(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);

void agregarRestriccionesAulas(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);
