#pragma once

/**
 * @file desenlace_solver_backend.hpp
 * @brief Mapeo del estado de CP-SAT al desenlace del cálculo (spec 020, RF-2/RF-3).
 *
 * Header SOLO backend: incluye OR-Tools. El enum vive en `desenlace_solver.hpp`, que no
 * arrastra el motor (decisión D5), de modo que el servicio y el frontend pueden incluir
 * el enum sin OR-Tools.
 *
 * `inline` evita un símbolo sin definir y un target nuevo.
 */

#include "backend/solver/desenlace_solver.hpp"
#include "ortools/sat/cp_model.pb.h"

/// Traduce el estado del motor al desenlace observable:
/// `OPTIMAL`/`FEASIBLE` → `Exito`, `INFEASIBLE` → `Infactible`,
/// `UNKNOWN` → `NoConcluyente`, `MODEL_INVALID`/otros → `ErrorContrato`.
///
/// Un `UNKNOWN` **nunca** es `Infactible` (no hay prueba de infactibilidad) y un
/// `MODEL_INVALID` tampoco: es un error de entrada o de contrato.
inline DesenlaceSolver desenlaceDesdeEstado(operations_research::sat::CpSolverStatus estado) {
    using operations_research::sat::CpSolverStatus;

    switch (estado) {
        case CpSolverStatus::OPTIMAL:
        case CpSolverStatus::FEASIBLE:
            return DesenlaceSolver::Exito;
        case CpSolverStatus::INFEASIBLE:
            return DesenlaceSolver::Infactible;
        case CpSolverStatus::UNKNOWN:
            return DesenlaceSolver::NoConcluyente;
        case CpSolverStatus::MODEL_INVALID:
        default:
            return DesenlaceSolver::ErrorContrato;
    }
}

/// ¿El estado prueba optimalidad? Solo `OPTIMAL`.
///
/// Un `FEASIBLE` es un horario **presentable** cuya fase no quedó probada como óptima: la
/// falta de prueba de optimalidad no descarta el horario (RF-3), solo marca la fase.
inline bool pruebaOptimalidad(operations_research::sat::CpSolverStatus estado) {
    return estado == operations_research::sat::CpSolverStatus::OPTIMAL;
}
