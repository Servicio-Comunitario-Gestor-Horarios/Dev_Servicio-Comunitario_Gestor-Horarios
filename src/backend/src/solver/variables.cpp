#include "backend/solver/variables.hpp"
#include "ortools/sat/cp_model.h"

VariablesSolver crearVariables(CpModelBuilder& model, const SolverConfig& config) {
    VariablesSolver vars;
    const auto& dim = config.dimensiones;

    QMap<QString, QVector<int>> slotsPorTurno;
    for (const auto& t : config.turnos.turnos) {
        slotsPorTurno[t.nombre] = t.slot;
    }

    // assignment[p][m][c][d][s]
    for (int c = 0; c < dim.num_cursos; c++) {
        const auto& curso = config.cursos[c];
        QVector<int> slotsTurno = slotsPorTurno.value(curso.turno);

        for (const auto& mc : curso.materias) {
            int m = mc.materiaIDx;

            for (int p = 0; p < dim.num_profesores; p++) {
                const auto& prof = config.profesores[p];

                bool esTitular = prof.materias_asignadas.contains(m);
                bool esSuplente = false;

                for (const auto& ms : prof.materias_suplente) {
                    if (ms.materiaIDx == m) { esSuplente = true; break; }
                }

                if (!esTitular && !esSuplente) continue;

                for (int d = 0; d < dim.num_dias; d++) {
                    QVector<int> slotsDisponibles;
                    for (const auto& disp : prof.disponibilidad) {
                        if (disp.dia == d) { slotsDisponibles = disp.slot; break;}
                    }

                    for (int s : slotsTurno) {
                        if (!slotsDisponibles.contains(s)) continue;
                        std::tuple<int, int, int, int, int> key{p, m, c, d, s};
                        vars.assignment[key] = model.NewBoolVar();
                    }
                }
            }
        }
    }

    for ( int c = 0; c < dim.num_cursos; c++) {
        if ( config.cursos[c].aula_fija != -1 ) continue;

        QVector<int> slotsTurno = slotsPorTurno.value(config.cursos[c].turno);

        for (int d = 0; d < dim.num_dias; d++) {
            for (int s : slotsTurno) {
                for (int a = 0; a < dim.num_aulas; a++) {
                    std::tuple<int, int, int, int> key{c, d, s, a};
                    vars.aulaAssignment[key] = model.NewBoolVar();
                }
            }
        }
    }

    return vars;
}
