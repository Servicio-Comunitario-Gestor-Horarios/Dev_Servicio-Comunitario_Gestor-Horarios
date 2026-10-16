#include "backend/solver/variables.hpp"
#include "backend/solver/config/contrato_solver.hpp"
#include "ortools/sat/cp_model.h"

VariablesSolver crearVariables(CpModelBuilder& model, const SolverConfig& config) {
    VariablesSolver vars;
    const auto& dim = config.dimensiones;

    QMap<QString, QVector<int>> slotsPorTurno;
    for (const auto& t : config.turnos.turnos)
        slotsPorTurno[t.nombre] = t.slot;

    // ── x[p][m][c][d][s]  +  z[p][c][m] ──
    for (int c = 0; c < dim.num_cursos; ++c) {
        const CursoSolverConfig& curso = config.cursos[c];
        const QVector<int> slotsCurso = slotsPorTurno.value(curso.turno);   // D11

        for (const PlanMateriaConfig& mc : materiasDelCurso(config, curso)) {
            const int m = mc.materiaIDx;

            for (int p = 0; p < dim.num_profesores; ++p) {
                const ProfesorSolverConfig& prof = config.profesores[p];

                // Elegibilidad (poda): D7 (plan) y D13 (año).
                if (!prof.plan.isEmpty()  && curso.plan != prof.plan) continue;
                if (!prof.anios.isEmpty() && !prof.anios.contains(curso.anio)) continue;

                bool esTitular = prof.materias_asignadas.contains(m);
                bool esSuplente = false;
                for (const MateriaSuplente& ms : prof.materias_suplente)
                    if (ms.materiaIDx == m) { esSuplente = true; break; }
                if (!esTitular && !esSuplente) continue;

                const QVector<int> slotsProfesor =
                    prof.turno.isEmpty() ? QVector<int>() : slotsPorTurno.value(prof.turno);

                bool creoAlguno = false;
                for (int d = 0; d < dim.num_dias; ++d) {
                    QVector<int> dispDia;
                    for (const Disponibilidad& disp : prof.disponibilidad)
                        if (disp.dia == d) { dispDia = disp.slot; break; }

                    for (int s : slotsCurso) {
                        if (!dispDia.contains(s)) continue;
                        // D6: turno del docente (si definido).
                        if (!prof.turno.isEmpty() && !slotsProfesor.contains(s)) continue;
                        vars.assignment[{p, m, c, d, s}] = model.NewBoolVar();
                        creoAlguno = true;
                    }
                }
                if (creoAlguno)
                    vars.asignacionDocente[{p, c, m}] = model.NewBoolVar();
            }
        }
    }

    // ── y[c][d][s][a]  (solo cursos MÓVILES: D9/D10) ──
    if (!config.aula_fija) {
        for (int c = 0; c < dim.num_cursos; ++c) {
            const QVector<int> slotsCurso = slotsPorTurno.value(config.cursos[c].turno);
            for (int d = 0; d < dim.num_dias; ++d)
                for (int s : slotsCurso)
                    for (int a = 0; a < dim.num_aulas; ++a)
                        vars.aulaAssignment[{c, d, s, a}] = model.NewBoolVar();
        }
    }

    // ── e[p]  (exceso sobre el tope, S1) ──
    const int cotaExceso = dim.num_dias * dim.num_slots_dia;
    for (int p = 0; p < dim.num_profesores; ++p)
        vars.exceso[p] = model.NewIntVar(operations_research::Domain(0, cotaExceso))
                             .WithName("exceso_" + std::to_string(p));

    return vars;
}
