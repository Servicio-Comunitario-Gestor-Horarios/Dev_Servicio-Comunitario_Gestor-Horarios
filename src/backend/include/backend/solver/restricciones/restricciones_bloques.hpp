#pragma once

/**
 * @file restricciones_bloques.hpp
 * @brief Reglas duras del emparejamiento en bloques de dos horas (spec 014, R1–R3).
 *
 * R1: `ocupada[c][d][s] = Σ_{p,m} x[p][m][c][d][s]`.
 * R2: en cada segmento del turno, `ocupada[s] = par[s] + par[s−1]` (los términos que no
 *     existen en el borde valen 0) y `par[s] ⇒ ocupada[s] = ocupada[s+1] = 1`.
 * R3: `materiaEnFranja[m][c][d][s] = Σ_p x[p][m][c][d][s]`.
 */

#include "backend/solver/config/solver_config.hpp"
#include "backend/solver/variables.hpp"
#include "ortools/sat/cp_model.h"

using namespace operations_research::sat;

/// R1 + R3: ocupación del curso en cada franja y presencia de cada materia en la franja.
void restriccionOcupacion(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);

/// R2: emparejamiento exacto de cada segmento con pares adyacentes disjuntos.
///
/// La igualdad `ocupada = par[s] + par[s−1]` cubre todas las franjas ocupadas con pares;
/// un tramo de longitud impar no admite solución (la última franja necesitaría un par con
/// una franja libre) y un tramo de 4 admite sus dos pares adyacentes.
void restriccionEmparejamiento(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);

/// R7: las dos franjas de un par activo usan la MISMA aula.
///
/// `par[c][d][s] ⇒ y[c][d][s][a] = y[c][d][s+1][a]` para cada aula `a`. Si el par no está
/// activo no se impone nada: dos pares distintos de un tramo de 4 pueden usar aulas
/// distintas. Con las reglas de tipo exacto y exclusividad de la 010, esto exige que los
/// dos restos de un bloque compartido requieran el mismo tipo de aula.
void restriccionMismaAulaDelPar(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);

/// R4 + R5 + R6: bloque simple, alineación de cada materia y un bloque por materia y día.
///
/// - R4: `bloqueSimple[m,par] = min(par, u[s], u[s+1])`.
/// - R5: `2·Σ simple ≥ h − (h mod 2)` por materia (`franjas descubiertas ≤ h mod 2`).
/// - R6: `Σ_{pares del día} participa ≤ 1`, con `participa = u[s] + u[s+1] − simple`.
///
/// Con R2 y D3, una materia par no puede quedar en un par compartido (no tiene resto) y una
/// impar deja exactamente una franja, que solo puede ser el otro resto de un par compartido.
void restriccionAlineacion(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);

/// R8: `apareceDia[m,c,d] = franjas del día − bloques simples del día` (0/1 por R6) y
/// `incumpleSeparacion[m,c,d] = min(apareceDia[d], apareceDia[d+1])` (separación blanda con
/// prioridad alta; el peso vive en `Solver`).
void restriccionSeparacion(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);

/// R9: ocupación monótona dentro del día (`ocupada[s] ≥ ocupada[s+1]`): las franjas libres
/// son un sufijo y el curso sale temprano. Es una regla dura.
void restriccionHuecosAlFinal(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);

void agregarRestriccionesBloques(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
);
