#pragma once

#include "backend/data/horario_salida.hpp"
#include "backend/solver/config/solver_config.hpp"

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
        void agregarRestricciones(const SolverConfig& config);
        void extraerSolucion(const SolverConfig& config);
};