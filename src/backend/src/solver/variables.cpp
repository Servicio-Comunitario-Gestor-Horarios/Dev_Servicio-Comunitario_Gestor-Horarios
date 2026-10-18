#include "backend/solver/variables.hpp"
#include "backend/solver/config/contrato_solver.hpp"
#include "ortools/sat/cp_model.h"

VariablesSolver crearVariables(CpModelBuilder& model, const SolverConfig& config) {
    VariablesSolver vars;
    const auto& dim = config.dimensiones;

    QMap<QString, QVector<int>> slotsPorTurno;
    for (const auto& t : config.turnos.turnos)
        slotsPorTurno[t.nombre] = t.slot;

    // Pre-computar segmentos por turno (para S3)
    QMap<QString, QVector<QVector<int>>> segmentosPorTurno;
    for (const TurnoConfig& t : config.turnos.turnos) {
        segmentosPorTurno[t.nombre] = segmentosTurno(t, config.recesos);
    }

    // ── x[p][m][c][d][s]  +  z[p][c][m] ──
    for (int c = 0; c < dim.num_cursos; ++c) {
        const CursoSolverConfig& curso = config.cursos[c];

        for (const PlanMateriaConfig& mc : materiasDelCurso(config, curso)) {
            const int m = mc.materiaIDx;

            for (int p = 0; p < dim.num_profesores; ++p) {
                const ProfesorSolverConfig& prof = config.profesores[p];

                // Poda de elegibilidad: opta por la materia (titular/suplente), plan (D7) y año (D13).
                if (!docenteOptaPorMateria(prof, m)) continue;
                if (!prof.plan.isEmpty()  && curso.plan != prof.plan) continue;
                if (!prof.anios.isEmpty() && !prof.anios.contains(curso.anio)) continue;

                // z SIEMPRE para todo candidato elegible por plan/año: los recortes por turno y
                // disponibilidad viven en las `x` y, vía D3 (`Σx = bloques·z`) fuerzan z=0. De este
                // modo un (curso,materia) sin candidato con disponibilidad hace INFEASIBLE a D1,
                // en lugar de omitirse en silencio.
                vars.asignacionDocente[{p, c, m}] = model.NewBoolVar();

                for (int d = 0; d < dim.num_dias; ++d)
                    for (int s : slotsDisponibles(config, curso, prof, d))
                        vars.assignment[{p, m, c, d, s}] = model.NewBoolVar();
            }
        }
    }

    // ── S3: Variables de agrupación (start y len) ──
    // Solo para las materias que EXISTEN en el plan del curso Y para las franjas
    // donde hay al menos una x (un docente candidato disponible). Esto evita
    // crear decenas de miles de variables innecesarias.
    for (int c = 0; c < dim.num_cursos; ++c) {
        const CursoSolverConfig& curso = config.cursos[c];
        const QVector<QVector<int>>& segs = segmentosPorTurno.value(curso.turno);
        if (segs.isEmpty()) continue;

        for (const PlanMateriaConfig& mc : materiasDelCurso(config, curso)) {
            const int m = mc.materiaIDx;

            for (int d = 0; d < dim.num_dias; ++d) {
                for (int seg = 0; seg < segs.size(); ++seg) {
                    const QVector<int>& slotsSeg = segs[seg];
                    if (slotsSeg.isEmpty()) continue;

                    for (int s : slotsSeg) {
                        // Solo crear start/len si existe al menos una x para (m,c,d,s)
                        bool hayX = false;
                        for (auto it = vars.assignment.constBegin(); it != vars.assignment.constEnd(); ++it) {
                            const auto [p, mX, cX, dX, sX] = it.key();
                            if (mX == m && cX == c && dX == d && sX == s) { hayX = true; break; }
                        }
                        if (!hayX) continue;

                        vars.start[{m, c, d, seg, s}] = model.NewBoolVar();
                    }
                }
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
