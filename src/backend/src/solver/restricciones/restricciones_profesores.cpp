#include "backend/solver/restricciones/restricciones_profesores.hpp"
#include "backend/solver/config/contrato_solver.hpp"

#include <vector>

// D5 — un profesor no da dos clases a la vez.
void restriccionDocenteNoSolapado(
    [[maybe_unused]] CpModelBuilder& model,
    [[maybe_unused]] const SolverConfig& config,
    [[maybe_unused]] const VariablesSolver& vars
) {
    for (int p = 0; p < config.dimensiones.num_profesores; p++) {
        for (int d = 0; d < config.dimensiones.num_dias; d++) {
            for (int s = 0; s < config.dimensiones.num_slots_dia; s++) {
                std::vector<BoolVar> varsProfesor;
                for (auto it = vars.assignment.constBegin(); it != vars.assignment.constEnd(); ++it) {
                    const auto [profesor, m, curso, dia, slot] = it.key();
                    (void)m; (void)curso;
                    if (profesor == p && dia == d && slot == s) varsProfesor.push_back(it.value());
                }
                if (varsProfesor.size() > 1) model.AddAtMostOne(varsProfesor);
            }
        }
    }
}

// D1 + D2 + D3 — un docente por materia-curso; solo él la dicta; con sus horas exactas.
void restriccionUnDocentePorMateria(
    [[maybe_unused]] CpModelBuilder& model,
    [[maybe_unused]] const SolverConfig& config,
    [[maybe_unused]] const VariablesSolver& vars
) {
    const int dur = config.franja_horaria.duracion_minutos;

    for (int c = 0; c < config.cursos.size(); ++c) {
        for (const PlanMateriaConfig& mc : materiasDelCurso(config, config.cursos[c])) {
            const int m = mc.materiaIDx;
            const int bloquesMateria = bloques(mc.horasSemanales, dur);

            std::vector<BoolVar> zDe;   // z de los docentes candidatos de (c,m)
            for (auto it = vars.asignacionDocente.constBegin();
                 it != vars.asignacionDocente.constEnd(); ++it) {
                const auto [p, zc, zm] = it.key();
                if (zc != c || zm != m) continue;
                zDe.push_back(it.value());

                // D3: sum_{d,s} x[p,m,c,d,s] == bloques(c,m) * z[p,c,m].
                // La igualdad SIEMPRE se publica: si el docente no tiene ninguna x (turno o
                // disponibilidad que no se solapan), fuerza z=0 en lugar de omitirse.
                std::vector<BoolVar> xDe;
                for (auto xi = vars.assignment.constBegin(); xi != vars.assignment.constEnd(); ++xi) {
                    const auto [xp, xm, xc, xd, xs] = xi.key();
                    (void)xd; (void)xs;
                    if (xp == p && xm == m && xc == c) xDe.push_back(xi.value());
                }
                const LinearExpr sumaX = xDe.empty() ? LinearExpr(0) : LinearExpr::Sum(xDe);
                model.AddEquality(sumaX, LinearExpr(it.value()) * bloquesMateria);
            }

            // D1: exactamente un docente por materia-curso. Se publica para TODA materia del plan;
            // si no hay ningún candidato (zDe vacío) la igualdad 0 == 1 hace INFEASIBLE.
            model.AddEquality(zDe.empty() ? LinearExpr(0) : LinearExpr::Sum(zDe), 1);
        }
    }
}

void agregarRestriccionProfesores(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
) {
    restriccionDocenteNoSolapado(model, config, vars);
    restriccionUnDocentePorMateria(model, config, vars);
}
