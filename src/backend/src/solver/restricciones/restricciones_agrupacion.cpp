#include "backend/solver/restricciones/restricciones_agrupacion.hpp"
#include "backend/solver/config/contrato_solver.hpp"
#include "backend/solver/variables.hpp"
#include "backend/solver/config/solver_config.hpp"

#include <vector>

using namespace operations_research::sat;

/**
 * @brief Restricciones de agrupación S3.
 *
 * Formulación eficiente y exacta de la penalización por fragmentación:
 *   - isolated[s] = 1  ⇔  hay clase en s y NO en s-1 ni en s+1 (grupo de 1).
 *   - longGroup[s] = 1 ⇔  hay clase en s, s-1, s-2 y s-3 (4.ª franja o posterior).
 *
 * El objetivo suma `isolated + longGroup`, que equivale a f(len) de RF-2:
 *   len == 1 → 1; len == 2,3 → 0; len >= 4 → len − 3.
 *
 * Los vecinos que no existen (sin ninguna x) se tratan como ausencia de clase.
 */
void agregarRestriccionesAgrupacion(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars) {
    const auto& dim = config.dimensiones;

    QMap<QString, QVector<QVector<int>>> segmentosPorTurno;
    for (const TurnoConfig& t : config.turnos.turnos)
        segmentosPorTurno[t.nombre] = segmentosTurno(t, config.recesos);

    for (int c = 0; c < dim.num_cursos; ++c) {
        const CursoSolverConfig& curso = config.cursos[c];
        const QVector<QVector<int>>& segs = segmentosPorTurno.value(curso.turno);
        if (segs.isEmpty()) continue;

        for (const PlanMateriaConfig& mc : materiasDelCurso(config, curso)) {
            const int m = mc.materiaIDx;

            for (int d = 0; d < dim.num_dias; ++d) {
                for (int seg = 0; seg < segs.size(); ++seg) {
                    const QVector<int>& franjas = segs[seg];
                    if (franjas.isEmpty()) continue;

                    // ── hay[s] = OR de las x de la materia en esa franja ──
                    QVector<BoolVar> hays(franjas.size());
                    QVector<bool> existe(franjas.size(), false);

                    for (int i = 0; i < franjas.size(); ++i) {
                        const int s = franjas[i];
                        const auto key = std::make_tuple(m, c, d, seg, s);
                        auto it = vars.hay.constFind(key);
                        if (it == vars.hay.constEnd()) continue;

                        std::vector<LinearExpr> xVars;
                        for (auto itX = vars.assignment.constBegin();
                             itX != vars.assignment.constEnd(); ++itX) {
                            const auto [p, mX, cX, dX, sX] = itX.key();
                            if (mX == m && cX == c && dX == d && sX == s)
                                xVars.push_back(LinearExpr(itX.value()));
                        }
                        BoolVar hay = it.value();
                        if (xVars.empty()) {
                            model.AddEquality(hay, false);
                        } else {
                            model.AddMaxEquality(hay, xVars);
                        }
                        hays[i] = hay;
                        existe[i] = true;
                    }

                    // ── isolated y longGroup por franja ──
                    for (int i = 0; i < franjas.size(); ++i) {
                        if (!existe[i]) continue;
                        const auto key = std::make_tuple(m, c, d, seg, franjas[i]);
                        BoolVar hay = hays[i];
                        BoolVar iso = vars.isolated[key];

                        // isolated ⇒ hay
                        model.AddImplication(iso, hay);
                        // isolated ⇒ ¬prev (si existe)
                        if (i > 0 && existe[i - 1]) model.AddImplication(iso, ~hays[i - 1]);
                        // isolated ⇒ ¬next (si existe)
                        if (i + 1 < franjas.size() && existe[i + 1]) model.AddImplication(iso, ~hays[i + 1]);
                        // (hay ∧ ¬prev ∧ ¬next) ⇒ isolated
                        {
                            std::vector<BoolVar> rev = {~hay, iso};
                            if (i > 0 && existe[i - 1]) rev.push_back(hays[i - 1]);
                            if (i + 1 < franjas.size() && existe[i + 1]) rev.push_back(hays[i + 1]);
                            model.AddBoolOr(rev);
                        }

                    }
                }
            }
        }
    }
}
