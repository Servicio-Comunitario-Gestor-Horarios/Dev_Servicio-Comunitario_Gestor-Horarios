#pragma once

#include "backend/data/horario_salida.hpp"
#include "backend/solver/config/solver_config.hpp"
#include "backend/solver/variables.hpp"
#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model.pb.h"
#include "ortools/sat/cp_model_solver.h"


using namespace operations_research::sat;

class Solver {
    public:
        struct ResultadoSolver {
            bool exito;
            HorarioSalida resultado;
            QStringList errores;
            double tiempo_ms;
        };

        ResultadoSolver resolver(const SolverConfig& config);
    
    private:

        VariablesSolver vars;
        CpModelBuilder model;
        
        void agregarRestricciones(const SolverConfig& config);
        HorarioSalida extraerSolucion(const SolverConfig& config);
};