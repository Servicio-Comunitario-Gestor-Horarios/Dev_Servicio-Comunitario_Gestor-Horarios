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

        /// Presupuesto determinista (segundos) de cada fase (013, §3 del plan).
        ///
        /// La fase 2 solo se ejecuta si hay preferencias y la 2b solo si la 2a produjo solución.
        /// Los tests pueden reducir un presupuesto para ejercitar la escalera de plazo
        /// (2b→2a→referencia) sin cambiar los valores de producción.
        struct PresupuestoFases {
            double referencia = 6.0;
            double preferencias = 2.0;
            double agrupacion = 1.0;
        };

        ResultadoSolver resolver(const SolverConfig& config);
        ResultadoSolver resolver(const SolverConfig& config,
                                 const PresupuestoFases& presupuesto);
    
    private:
        /// Anclaje estructural de la referencia (013, RF-1): asignación del docente por
        /// materia-curso (z*) y exceso de horas por docente (e*). La fase 2 los fija por
        /// igualdad, de modo que la preferencia solo decida DÓNDE caen las clases.
        struct AnclajeEstructural {
            QMap<std::tuple<int, int, int>, int> docentePorMateria;   ///< z*[p][c][m]
            QMap<int, int>                       excesoPorDocente;    ///< e*[p]
        };

        VariablesSolver vars;
        CpModelBuilder model;

        /// Reinicia `model`/`vars` y construye el modelo de una fase (F3): dos resoluciones
        /// seguidas no arrastran variables ni restricciones.
        void prepararModelo(const SolverConfig& config);
        CpSolverResponse ejecutar(double presupuesto);
        AnclajeEstructural extraerAnclaje(const CpSolverResponse& response) const;
        void fijarAnclaje(const AnclajeEstructural& anclaje);
        LinearExpr expresionPreferencias(const SolverConfig& config) const;

        void agregarRestricciones(const SolverConfig& config);
        void construirObjetivo(const SolverConfig& config);
        /// Objetivo de la fase 2b: solo S3 (agrupación). S1 y S2 ya están fijados por igualdad.
        void construirObjetivoSoloS3();
        HorarioSalida extraerSolucion(const CpSolverResponse& response, const SolverConfig& config);
        QStringList analizarFallo(const CpSolverResponse& response, const SolverConfig& config);
};