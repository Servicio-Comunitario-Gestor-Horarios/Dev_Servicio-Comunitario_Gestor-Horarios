#pragma once

#include <QtGlobal>

#include "backend/data/horario_salida.hpp"
#include "backend/solver/config/solver_config.hpp"
#include "backend/solver/desenlace_solver.hpp"
#include "backend/solver/variables.hpp"
#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model.pb.h"
#include "ortools/sat/cp_model_solver.h"


using namespace operations_research::sat;

/// Milisegundos de reloj que quedan hasta `deadlineMs` (nunca negativo). Puro (020, D15).
///
/// El presupuesto del solver se mide en **tiempo de reloj** desde el inicio de la resolución:
/// la fase de referencia puede consumir los 55 min y cada fase blanda recibe lo que quede.
inline qint64 restanteMs(qint64 deadlineMs, qint64 ahoraMs) {
    return deadlineMs > ahoraMs ? deadlineMs - ahoraMs : 0;
}

/// Segundos de reloj que quedan hasta `deadlineMs` (nunca negativo). Puro.
inline double restanteSegundos(qint64 deadlineMs, qint64 ahoraMs) {
    return static_cast<double>(restanteMs(deadlineMs, ahoraMs)) / 1000.0;
}

class Solver {
    public:
        struct ResultadoSolver {
            bool exito;
            /// Cómo terminó el cálculo (020, RF-2/RF-3). `exito` se conserva para los
            /// consumidores vigentes; `desenlace` distingue NoFactible de no concluyente y de
            /// error de contrato.
            DesenlaceSolver desenlace = DesenlaceSolver::SinRespuesta;
            /// Fases que devolvieron `FEASIBLE` sin probarse óptimas: "referencia",
            /// "separacion" o "preferencias". Vacía = todas las fases ejecutadas son óptimas.
            QStringList fasesNoOptimas;
            HorarioSalida resultado;
            QStringList errores;
            double tiempo_ms;
        };

        /// Presupuesto de **reloj** (020, D2/D3): un único deadline para todas las fases.
        ///
        /// `totalSegundos` es el presupuesto del solver **en total** (55 min por defecto = 3300 s).
        /// La fase de referencia puede consumirlo entero; las fases blandas usan solo lo que quede
        /// (`restante = max(0, deadline − ahora)`) y se saltan si no hay tiempo. Los 5 min hasta
        /// los 60 son reserva fija de extracción y comunicación del servicio, **fuera** de este
        /// presupuesto; no se usan para fases blandas.
        ///
        /// `ejecutarSeparacionHuecos`/`ejecutarPreferencias` permiten probar la escalera sin
        /// depender del reloj. `workers` vale 8 por defecto (014b); seed 0 se mantiene.
        struct PresupuestoFases {
            double totalSegundos = 3300.0;   ///< 55 min de reloj para todo el solver
            int workers = 8;
            bool ejecutarSeparacionHuecos = true;
            bool ejecutarPreferencias = true;
        };

        ResultadoSolver resolver(const SolverConfig& config);
        ResultadoSolver resolver(const SolverConfig& config,
                                 const PresupuestoFases& presupuesto);
    
    private:
        /// Anclaje estructural de la referencia (013, RF-1; 020): asignación del docente por
        /// materia-curso (z*). La fase 2 lo fija por igualdad, de modo que la preferencia solo
        /// decida DÓNDE caen las clases. El exceso por docente (e*) ya no existe: el tope de
        /// horas es una invariante dura y no hay nada que anclar.
        struct AnclajeEstructural {
            QMap<std::tuple<int, int, int>, int> docentePorMateria;   ///< z*[p][c][m]
        };

        VariablesSolver vars;
        CpModelBuilder model;

        /// Reinicia `model`/`vars` y construye el modelo de una fase (F3): dos resoluciones
        /// seguidas no arrastran variables ni restricciones.
        void prepararModelo(const SolverConfig& config);
        /// Ejecuta el modelo actual con un límite de **tiempo de reloj** (020, D2). Nunca se
        /// llama con 0 s: en CP-SAT un límite 0 significa «sin límite», no «sin tiempo».
        CpSolverResponse ejecutar(double segundos, int workers);
        AnclajeEstructural extraerAnclaje(const CpSolverResponse& response) const;
        void fijarAnclaje(const AnclajeEstructural& anclaje);
        LinearExpr expresionSeparacionViernes(const SolverConfig& config) const;
        LinearExpr expresionPreferencias(const SolverConfig& config) const;

        void agregarRestricciones(const SolverConfig& config);
        void construirObjetivo(const SolverConfig& config);
        HorarioSalida extraerSolucion(const CpSolverResponse& response, const SolverConfig& config);
        QStringList analizarFallo(const CpSolverResponse& response, const SolverConfig& config);
};