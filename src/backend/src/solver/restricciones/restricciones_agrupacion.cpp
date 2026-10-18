#include "backend/solver/restricciones/restricciones_agrupacion.hpp"
#include "backend/solver/config/contrato_solver.hpp"
#include "backend/solver/variables.hpp"
#include "backend/solver/config/solver_config.hpp"

#include <vector>

using namespace operations_research::sat;

/**
 * @brief Agrega las restricciones para la agrupación de bloques (S3).
 *
 * Formulación completa (T3/T4):
 * - start[m,c,d,seg,s] (BoolVar): arranque de grupo en s
 * - len[m,c,d,seg,s] (IntVar): longitud del grupo que arranca en s (0..maxLen)
 *
 * Reglas:
 * - start = 1  ⇔  hay clase en s Y (idx==0 O no hay clase en s-1)
 * - len = 0 si start = 0
 * - len = 1 + lenNext si start=1 y startNext=0
 * - len = 1 si start=1 y startNext=1
 * - Último slot del segmento: si start=1 → len = 1
 * - start=0 → len = 0
 */
void agregarRestriccionesAgrupacion(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars) {
    const auto& dim = config.dimensiones;

    // Pre-computar segmentos por turno
    QMap<QString, QVector<QVector<int>>> segmentosPorTurno;
    for (const TurnoConfig& t : config.turnos.turnos) {
        QVector<RecesoConfig> recesosTurno;
        for (const RecesoConfig& r : config.recesos)
            if (r.turno == t.nombre) recesosTurno.append(r);
        segmentosPorTurno[t.nombre] = segmentosTurno(TurnoConfig{t.nombre, t.slot}, config.recesos);
    }

    // Para cada materia-curso-día-segmento
    for (int m = 0; m < config.dimensiones.num_materias; ++m) {
        for (int c = 0; c < dim.num_cursos; ++c) {
            const CursoSolverConfig& curso = config.cursos[c];
            const QVector<QVector<int>>& segs = segmentosPorTurno.value(curso.turno);
            if (segs.isEmpty()) continue;

            for (int d = 0; d < dim.num_dias; ++d) {
                for (int seg = 0; seg < segs.size(); ++seg) {
                    const QVector<int>& slotsSeg = segs[seg];
                    if (slotsSeg.isEmpty()) continue;

                    for (int idx = 0; idx < slotsSeg.size(); ++idx) {
                        int s = slotsSeg[idx];

                        // ──── start ⇔ x ────
                        std::vector<LinearExpr> xEnS;
                        for (auto it = vars.assignment.constBegin(); it != vars.assignment.constEnd(); ++it) {
                            const auto [p, mX, cX, dX, sX] = it.key();
                            if (mX == m && cX == c && dX == d && sX == s) {
                                xEnS.push_back(LinearExpr(it.value()));
                            }
                        }
                        if (xEnS.empty()) continue;

                        BoolVar hayClaseEnS = model.NewBoolVar();
                        model.AddMaxEquality(hayClaseEnS, xEnS);

                        BoolVar noHayClaseEnAnterior = model.NewBoolVar();
                        if (idx == 0) {
                            model.AddEquality(noHayClaseEnAnterior, true);
                        } else {
                            int sPrev = slotsSeg[idx - 1];
                            std::vector<LinearExpr> xEnPrev;
                            for (auto it = vars.assignment.constBegin(); it != vars.assignment.constEnd(); ++it) {
                                const auto [p, mX, cX, dX, sX] = it.key();
                                if (mX == m && cX == c && dX == d && sX == sPrev) {
                                    xEnPrev.push_back(LinearExpr(it.value()));
                                }
                            }
                            if (xEnPrev.empty()) {
                                model.AddEquality(noHayClaseEnAnterior, true);
                            } else {
                                BoolVar hayClaseEnPrev = model.NewBoolVar();
                                model.AddMaxEquality(hayClaseEnPrev, xEnPrev);
                                BoolVar notHayClaseEnPrev = model.NewBoolVar();
                                model.AddEquality(notHayClaseEnPrev, LinearExpr(1) - hayClaseEnPrev);
                                model.AddEquality(noHayClaseEnAnterior, notHayClaseEnPrev);
                            }
                        }

                        BoolVar startVar = vars.start[{m, c, d, seg, s}];
                        model.AddBoolAnd({hayClaseEnS, noHayClaseEnAnterior}).OnlyEnforceIf(startVar);
                        model.AddBoolOr({~hayClaseEnS, ~noHayClaseEnAnterior}).OnlyEnforceIf(~startVar);


                    }
                }
            }
        }
    }
}
