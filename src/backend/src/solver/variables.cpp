#include "backend/solver/variables.hpp"
#include "backend/solver/config/contrato_solver.hpp"
#include "ortools/sat/cp_model.h"

#include <QSet>

VariablesSolver crearVariables(CpModelBuilder& model, const SolverConfig& config) {
    VariablesSolver vars;
    const auto& dim = config.dimensiones;

    QMap<QString, QVector<int>> slotsPorTurno;
    for (const auto& t : config.turnos.turnos)
        slotsPorTurno[t.nombre] = t.slot;

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
                // disponibilidad viven en las `x` y, vía D3 (`Σx = bloques·z`), fuerzan z=0. De este
                // modo un (curso,materia) sin candidato con disponibilidad hace INFEASIBLE a D1,
                // en lugar de omitirse en silencio.
                vars.asignacionDocente[{p, c, m}] = model.NewBoolVar();

                for (int d = 0; d < dim.num_dias; ++d)
                    for (int s : slotsDisponibles(config, curso, prof, d))
                        vars.assignment[{p, m, c, d, s}] = model.NewBoolVar();
            }
        }
    }

    // ── y[c][d][s][a]  (aula REAL de la clase: fijos y móviles, D9 ampliada) ──
    // Solo se crea la `y` de las aulas cuyo tipo requiere alguna materia del curso: una `y`
    // de otro tipo sería siempre 0 por la regla de tipo exacto (RF-3, spec 010).
    const int numAulas = qMin(dim.num_aulas, config.aulas.size());
    for (int c = 0; c < dim.num_cursos; ++c) {
        const CursoSolverConfig& curso = config.cursos[c];

        QSet<QString> tiposRequeridos;
        for (const PlanMateriaConfig& mc : materiasDelCurso(config, curso)) {
            const int m = mc.materiaIDx;
            if (m >= 0 && m < config.materias.size())
                tiposRequeridos.insert(claveTipo(config.materias[m].tipo));
        }

        const QVector<int> slotsCurso = slotsPorTurno.value(curso.turno);
        for (int d = 0; d < dim.num_dias; ++d)
            for (int s : slotsCurso)
                for (int a = 0; a < numAulas; ++a) {
                    if (!tiposRequeridos.contains(claveTipo(config.aulas[a].tipo))) continue;
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
