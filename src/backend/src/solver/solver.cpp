#include "backend/solver/solver.hpp"
#include "backend/solver/variables.hpp"
#include "backend/solver/restricciones/restricciones_turnos.hpp"
#include "backend/solver/restricciones/restricciones_aulas.hpp"
#include "backend/solver/restricciones/restricciones_profesores.hpp"
#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model.pb.h"
#include "ortools/sat/cp_model_solver.h"
#include "ortools/sat/model.h"
#include <QElapsedTimer>

using namespace operations_research::sat;

Solver::ResultadoSolver Solver::resolver(const SolverConfig& config) {
    QElapsedTimer timer;
    timer.start();

    ResultadoSolver resultado;
    resultado.exito = false;

    // Crear variables de decision
    vars = crearVariables(model, config);

    // Agregar restricciones
    agregarRestricciones(config);

    // Resolver CP-SAT
    Model sat_model;
    const CpSolverResponse response = SolveCpModel(model.Build(), &sat_model);

    // Verificar status
    if (response.status() == CpSolverStatus::OPTIMAL ||
        response.status() == CpSolverStatus::FEASIBLE) {
            resultado.exito = true;
            resultado.resultado = extraerSolucion(config);
        } else {
            resultado.errores.append("Solver no encontro solucion");
        }
    
    resultado.tiempo_ms = timer.elapsed();
    return resultado;
}

void Solver::agregarRestricciones(const SolverConfig& config) {
    agregarRestriccionesTurnos(model, config, vars);
    agregarRestriccionesAulas(model, config, vars);
    agregarRestriccionProfesores(model, config, vars);
}

HorarioSalida Solver::extraerSolucion(const SolverConfig& config) {
    HorarioSalida horario;
    // TODO: Implementar extraccion de solucion
    return horario;
}