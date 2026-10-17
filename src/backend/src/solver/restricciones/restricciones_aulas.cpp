#include "backend/solver/restricciones/restricciones_aulas.hpp"

#include <vector>

// D8 — un aula no aloja dos clases a la vez (moviles por `y`; cursos con aula FIJA por `x`).
void restriccionAulaNoSolapada(
    [[maybe_unused]] CpModelBuilder& model,
    [[maybe_unused]] const SolverConfig& config,
    [[maybe_unused]] const VariablesSolver& vars
) {
    for (int a = 0; a < config.dimensiones.num_aulas; a++) {
        for (int d = 0; d < config.dimensiones.num_dias; d++) {
            for (int s = 0; s < config.dimensiones.num_slots_dia; s++) {
                std::vector<BoolVar> varsAula;

                // Cursos moviles que ocupan esta aula (variable y).
                for (auto it = vars.aulaAssignment.constBegin(); it != vars.aulaAssignment.constEnd(); ++it) {
                    const auto [curso, dia, slot, aula] = it.key();
                    (void)curso;
                    if (aula == a && dia == d && slot == s) varsAula.push_back(it.value());
                }

                // Cursos con aula fija que usan esta aula (clases x).
                if (config.aula_fija) {
                    for (int c = 0; c < config.cursos.size(); ++c) {
                        if (config.cursos[c].aula != a) continue;
                        for (auto it = vars.assignment.constBegin(); it != vars.assignment.constEnd(); ++it) {
                            const auto [p, m, curso, dia, slot] = it.key();
                            (void)p; (void)m;
                            if (curso == c && dia == d && slot == s) varsAula.push_back(it.value());
                        }
                    }
                }

                if (varsAula.size() > 1) model.AddAtMostOne(varsAula);
            }
        }
    }
}

// D9 — un curso movil ocupa exactamente un aula en cada franja en que tiene clase.
void restriccionCursoOcupaUnAula(
    [[maybe_unused]] CpModelBuilder& model,
    [[maybe_unused]] const SolverConfig& config,
    [[maybe_unused]] const VariablesSolver& vars
) {
    if (config.aula_fija) return;   // sin aulas moviles

    for (int c = 0; c < config.cursos.size(); ++c) {
        for (int d = 0; d < config.dimensiones.num_dias; ++d) {
            for (int s = 0; s < config.dimensiones.num_slots_dia; ++s) {
                std::vector<BoolVar> yDe;   // aulas del curso (c,d,s)
                for (auto it = vars.aulaAssignment.constBegin(); it != vars.aulaAssignment.constEnd(); ++it) {
                    const auto [yc, yd, ys, ya] = it.key();
                    (void)ya;
                    if (yc == c && yd == d && ys == s) yDe.push_back(it.value());
                }

                std::vector<BoolVar> xDe;   // clases del curso (c,d,s)
                for (auto it = vars.assignment.constBegin(); it != vars.assignment.constEnd(); ++it) {
                    const auto [xp, xm, xc, xd, xs] = it.key();
                    (void)xp; (void)xm;
                    if (xc == c && xd == d && xs == s) xDe.push_back(it.value());
                }

                if (yDe.empty() && xDe.empty()) continue;
                model.AddEquality(LinearExpr::Sum(yDe), LinearExpr::Sum(xDe));
            }
        }
    }
}

void agregarRestriccionesAulas(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
) {
    restriccionAulaNoSolapada(model, config, vars);
    restriccionCursoOcupaUnAula(model, config, vars);
}
