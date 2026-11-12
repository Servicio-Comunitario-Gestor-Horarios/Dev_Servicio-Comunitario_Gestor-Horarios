#pragma once

/**
 * @file desenlace_solver.hpp
 * @brief Desenlace del cálculo del solver (spec 020, RF-2/RF-3).
 *
 * Header LIMPIO: solo el enum. **No** incluye OR-Tools ni Qt, porque lo incluyen
 * `ServicioGeneracion.hpp` y el frontend, que no compilan OR-Tools (decisión D5).
 * El mapeo desde el estado de CP-SAT vive en `desenlace_solver_backend.hpp`, que sí
 * incluye el motor y solo compilan el backend y sus tests.
 */

/// Cómo terminó el cálculo, con los desenlaces que la spec 020 distingue.
enum class DesenlaceSolver {
    /// Hay horario (óptimo o factible al límite del presupuesto). La optimalidad se declara
    /// por fase en `ResultadoSolver::fasesNoOptimas`.
    Exito,
    /// El motor DEMOSTRÓ que no hay solución que respete las invariantes duras (INFEASIBLE).
    Infactible,
    /// El presupuesto se agotó sin prueba de factibilidad ni de infactibilidad (UNKNOWN).
    /// **No** es NoFactible.
    NoConcluyente,
    /// Modelo inválido, error interno o respuesta fuera de contrato. **No** es NoFactible ni
    /// no concluyente.
    ErrorContrato,
    /// No hubo respuesta del proceso (pérdida de comunicación). Lo marca el puerto/servicio.
    SinRespuesta,
};
