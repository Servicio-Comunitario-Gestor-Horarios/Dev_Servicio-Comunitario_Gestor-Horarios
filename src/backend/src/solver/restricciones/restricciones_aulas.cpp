#include "backend/solver/restricciones/restricciones_aulas.hpp"

#include "backend/solver/config/contrato_solver.hpp"

#include <map>
#include <tuple>
#include <utility>
#include <vector>

namespace {

/// Índice de las `x` por (curso, día, franja) → materia → variables.
using ClasesPorFranja = QMap<std::tuple<int, int, int>, QMap<int, std::vector<BoolVar>>>;

ClasesPorFranja indexarClasesPorFranja(const VariablesSolver& vars) {
    ClasesPorFranja indice;
    for (auto it = vars.assignment.constBegin(); it != vars.assignment.constEnd(); ++it) {
        const auto [p, m, c, d, s] = it.key();
        (void)p;
        indice[{c, d, s}][m].push_back(it.value());
    }
    return indice;
}

/// Clave normalizada del tipo del aula `a` ("" si el índice no existe).
QString claveAula(const SolverConfig& config, int a) {
    if (a < 0 || a >= config.aulas.size()) return QString();
    return claveTipo(config.aulas[a].tipo);
}

/// Clave normalizada del tipo requerido por la materia `m` ("" si el índice no existe).
QString claveMateria(const SolverConfig& config, int m) {
    if (m < 0 || m >= config.materias.size()) return QString();
    return claveTipo(config.materias[m].tipo);
}

}  // namespace

// D8 — un aula no aloja dos clases a la vez. Con la `y` de TODOS los cursos, la exclusividad
// cubre también las clases desplazadas a otra aula (RF-3, spec 010).
void restriccionAulaNoSolapada(
    CpModelBuilder& model,
    [[maybe_unused]] const SolverConfig& config,
    const VariablesSolver& vars
) {
    QMap<std::tuple<int, int, int>, std::vector<BoolVar>> yPorFranjaAula;   // (d, s, a)

    for (auto it = vars.aulaAssignment.constBegin(); it != vars.aulaAssignment.constEnd(); ++it) {
        const auto [c, d, s, a] = it.key();
        (void)c;
        yPorFranjaAula[{d, s, a}].push_back(it.value());
    }

    for (auto it = yPorFranjaAula.begin(); it != yPorFranjaAula.end(); ++it)
        if (it.value().size() > 1) model.AddAtMostOne(it.value());
}

// D9 ampliada — una clase ocupa exactamente un aula, en los dos modos (fijos y móviles):
// Σ_a y[c][d][s][a] == Σ_{p,m} x[p][m][c][d][s].
void restriccionCursoOcupaUnAula(
    CpModelBuilder& model,
    [[maybe_unused]] const SolverConfig& config,
    const VariablesSolver& vars
) {
    std::map<std::tuple<int, int, int>, std::pair<std::vector<BoolVar>, std::vector<BoolVar>>> franjas;

    for (auto it = vars.aulaAssignment.constBegin(); it != vars.aulaAssignment.constEnd(); ++it) {
        const auto [c, d, s, a] = it.key();
        (void)a;
        franjas[{c, d, s}].first.push_back(it.value());
    }
    for (auto it = vars.assignment.constBegin(); it != vars.assignment.constEnd(); ++it) {
        const auto [p, m, c, d, s] = it.key();
        (void)p; (void)m;
        franjas[{c, d, s}].second.push_back(it.value());
    }

    for (auto it = franjas.begin(); it != franjas.end(); ++it) {
        const auto& [yDe, xDe] = it->second;
        if (yDe.empty() && xDe.empty()) continue;
        // Si no hay `y` en la franja (el curso no tiene aulas de los tipos que requiere), la
        // igualdad deja las clases a 0: la regla de tipo es dura y el modelo es infactible.
        model.AddEquality(LinearExpr::Sum(yDe), LinearExpr::Sum(xDe));
    }
}

// Tipo exacto (RF-3) — la clase de la materia m solo puede ocupar aulas de su tipo:
// y[c][d][s][a] + Σ_p x[p][m][c][d][s] <= 1 para todo aula y materia con clave distinta.
void restriccionTipoExacto(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
) {
    const ClasesPorFranja clases = indexarClasesPorFranja(vars);

    for (auto itY = vars.aulaAssignment.constBegin(); itY != vars.aulaAssignment.constEnd(); ++itY) {
        const auto [c, d, s, a] = itY.key();
        const QString clave = claveAula(config, a);

        const auto clasesIt = clases.constFind({c, d, s});
        if (clasesIt == clases.constEnd()) continue;

        for (auto itM = clasesIt.value().constBegin(); itM != clasesIt.value().constEnd(); ++itM) {
            if (claveMateria(config, itM.key()) == clave) continue;   // tipo exacto: permitido
            if (itM.value().empty()) continue;

            std::vector<BoolVar> terminos = itM.value();
            terminos.push_back(itY.value());
            model.AddAtMostOne(terminos);
        }
    }
}

// Aula de referencia (RF-3, solo con el modo de aula fija activo):
//  - si el tipo de la materia coincide con el del aula de referencia, la clase se imparte en ella;
//  - si no, la clase se desplaza a un aula del tipo requerido (lo fija la regla de tipo exacto);
//  - mientras hay una clase desplazada en una franja, la referencia queda vacía y bloqueada.
void restriccionAulaDeReferencia(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
) {
    if (!config.aula_fija) return;

    const ClasesPorFranja clases = indexarClasesPorFranja(vars);

    for (int c = 0; c < config.cursos.size(); ++c) {
        const int referencia = config.cursos[c].aula;
        if (referencia < 0 || referencia >= config.aulas.size()) continue;   // V8 lo exige
        const QString claveReferencia = claveAula(config, referencia);

        for (auto itX = clases.constBegin(); itX != clases.constEnd(); ++itX) {
            const auto [curso, d, s] = itX.key();
            if (curso != c) continue;

            const auto refIt = vars.aulaAssignment.constFind({c, d, s, referencia});
            std::vector<BoolVar> desplazadas;

            for (auto itM = itX.value().constBegin(); itM != itX.value().constEnd(); ++itM) {
                if (claveMateria(config, itM.key()) == claveReferencia) {
                    // Coincidencia: se usa la referencia y no otra aula de su mismo tipo.
                    if (refIt != vars.aulaAssignment.constEnd())
                        model.AddLessOrEqual(LinearExpr::Sum(itM.value()), LinearExpr(*refIt));
                    else
                        model.AddEquality(LinearExpr::Sum(itM.value()), 0);
                } else {
                    desplazadas.insert(desplazadas.end(), itM.value().begin(), itM.value().end());
                }
            }

            if (desplazadas.empty()) continue;

            // Bloqueo: y[c'][d][s][referencia] + desplazado <= 1. Solo se recorren las `y` ya
            // creadas: con turnos distintos, (c', d, s, referencia) puede no existir.
            for (auto itY = vars.aulaAssignment.constBegin(); itY != vars.aulaAssignment.constEnd(); ++itY) {
                const auto [otroCurso, otroDia, otroSlot, otraAula] = itY.key();
                if (otroCurso == c || otroDia != d || otroSlot != s || otraAula != referencia) continue;

                std::vector<BoolVar> terminos = desplazadas;
                terminos.push_back(itY.value());
                model.AddAtMostOne(terminos);
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
    restriccionTipoExacto(model, config, vars);
    restriccionAulaDeReferencia(model, config, vars);
}
