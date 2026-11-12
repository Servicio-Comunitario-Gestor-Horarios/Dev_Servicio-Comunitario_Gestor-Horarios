#include "backend/solver/restricciones/restricciones_bloques.hpp"

#include "backend/solver/config/contrato_solver.hpp"

#include <map>
#include <tuple>
#include <vector>

namespace {

/// Índice de las `x` por (curso, día, franja).
using ClasesPorFranja = QMap<std::tuple<int, int, int>, std::vector<BoolVar>>;

/// Índice de las `x` por (materia, curso, día, franja).
using ClasesPorMateriaFranja = QMap<std::tuple<int, int, int, int>, std::vector<BoolVar>>;

ClasesPorFranja indexarPorFranja(const VariablesSolver& vars) {
    ClasesPorFranja indice;
    for (auto it = vars.assignment.constBegin(); it != vars.assignment.constEnd(); ++it) {
        const auto [p, m, c, d, s] = it.key();
        (void)p; (void)m;
        indice[{c, d, s}].push_back(it.value());
    }
    return indice;
}

ClasesPorMateriaFranja indexarPorMateriaFranja(const VariablesSolver& vars) {
    ClasesPorMateriaFranja indice;
    for (auto it = vars.assignment.constBegin(); it != vars.assignment.constEnd(); ++it) {
        const auto [p, m, c, d, s] = it.key();
        (void)p;
        indice[{m, c, d, s}].push_back(it.value());
    }
    return indice;
}

} // namespace

// R1 + R3 — `ocupada` y `materiaEnFranja` como suma de las clases (0/1 por D1–D4).
void restriccionOcupacion(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
) {
    (void)config;
    const ClasesPorFranja clases = indexarPorFranja(vars);
    const ClasesPorMateriaFranja porMateria = indexarPorMateriaFranja(vars);

    // R1: una franja del turno está ocupada si el curso tiene clase en ella.
    for (auto it = vars.ocupada.constBegin(); it != vars.ocupada.constEnd(); ++it) {
        const auto [c, d, s] = it.key();
        const auto clasesIt = clases.constFind({c, d, s});
        const LinearExpr suma =
            (clasesIt == clases.constEnd() || clasesIt.value().empty())
                ? LinearExpr(0)
                : LinearExpr::Sum(clasesIt.value());
        model.AddEquality(LinearExpr(it.value()), suma);
    }

    // R3: la materia está en la franja si alguno de sus docentes da clase en ella.
    for (auto it = vars.materiaEnFranja.constBegin(); it != vars.materiaEnFranja.constEnd(); ++it) {
        const auto [m, c, d, s] = it.key();
        const auto clasesIt = porMateria.constFind({m, c, d, s});
        const LinearExpr suma =
            (clasesIt == porMateria.constEnd() || clasesIt.value().empty())
                ? LinearExpr(0)
                : LinearExpr::Sum(clasesIt.value());
        model.AddEquality(LinearExpr(it.value()), suma);
    }
}

// R2 — emparejamiento exacto por segmento.
//
// Para cada franja `s` del segmento: `ocupada[s] = par[s] + par[s−1]`, donde `par[s]` solo
// existe si `s+1` está en el segmento y `par[s−1]` solo si `s−1` está en el segmento (en los
// bordes el término ausente es 0). La igualdad en `s` y en `s+1` hace que `par[s] = 1` obligue
// a `ocupada[s] = ocupada[s+1] = 1`; se publica además la implicación explícita (OnlyEnforceIf).
//
// Demostración de la paridad: sumando las igualdades de un segmento,
// `Σ ocupada = 2·Σ par`, luego el número de franjas ocupadas del segmento es PAR. Un tramo
// impar no se puede cubrir: además, la igualdad en la primera franja libre tras el tramo
// fuerza `par[última] = 0`, así que la última franja del tramo quedaría sin par.
void restriccionEmparejamiento(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
) {
    QMap<QString, QVector<QVector<int>>> segmentosPorTurno;
    for (const TurnoConfig& turno : config.turnos.turnos)
        segmentosPorTurno[turno.nombre] = segmentosTurno(turno, config.eventos);

    for (int c = 0; c < config.cursos.size(); ++c) {
        const QVector<QVector<int>>& segs =
            segmentosPorTurno.value(config.cursos[c].turno);

        for (int d = 0; d < config.dimensiones.num_dias; ++d) {
            for (const QVector<int>& franjas : segs) {
                for (int k = 0; k < franjas.size(); ++k) {
                    const int s = franjas[k];
                    const auto ocupadaIt = vars.ocupada.constFind({c, d, s});
                    if (ocupadaIt == vars.ocupada.constEnd()) continue;

                    LinearExpr rhs(0);
                    if (k + 1 < franjas.size()) {
                        const auto parIt = vars.par.constFind({c, d, s});
                        if (parIt != vars.par.constEnd())
                            rhs = rhs + LinearExpr(*parIt);
                    }
                    if (k > 0) {
                        const auto parIt = vars.par.constFind({c, d, franjas[k - 1]});
                        if (parIt != vars.par.constEnd())
                            rhs = rhs + LinearExpr(*parIt);
                    }
                    model.AddEquality(LinearExpr(*ocupadaIt), rhs);

                    // par[s] ⇒ ocupada[s] = 1 ∧ ocupada[s+1] = 1.
                    if (k + 1 < franjas.size()) {
                        const auto parIt = vars.par.constFind({c, d, s});
                        if (parIt == vars.par.constEnd()) continue;

                        model.AddEquality(LinearExpr(*ocupadaIt), 1).OnlyEnforceIf(*parIt);
                        const auto siguienteIt =
                            vars.ocupada.constFind({c, d, franjas[k + 1]});
                        if (siguienteIt != vars.ocupada.constEnd())
                            model.AddEquality(LinearExpr(*siguienteIt), 1)
                                .OnlyEnforceIf(*parIt);
                    }
                }
            }
        }
    }
}

// R7 — las dos franjas de un par activo comparten aula.
//
// `par[c][d][s] ⇒ y[c][d][s][a] = y[c][d][s+1][a]` para cada aula `a` con `y` en las dos
// franjas. La implicación solo se impone con el par activo: en un tramo de 4 los dos pares
// son independientes y pueden usar aulas distintas. Junto con D9 (una `y` por franja) y el
// tipo exacto de la 010, esto obliga a que los dos restos de un bloque compartido requieran
// el mismo tipo de aula.
void restriccionMismaAulaDelPar(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
) {
    (void)config;

    // (c,d,s) → aula → y
    QMap<std::tuple<int, int, int>, QMap<int, BoolVar>> aulaPorFranja;
    for (auto it = vars.aulaAssignment.constBegin(); it != vars.aulaAssignment.constEnd(); ++it) {
        const auto [c, d, s, a] = it.key();
        aulaPorFranja[{c, d, s}].insert(a, it.value());
    }

    for (auto it = vars.par.constBegin(); it != vars.par.constEnd(); ++it) {
        const auto [c, d, s] = it.key();
        const auto actualIt = aulaPorFranja.constFind({c, d, s});
        const auto siguienteIt = aulaPorFranja.constFind({c, d, s + 1});
        if (actualIt == aulaPorFranja.constEnd() || siguienteIt == aulaPorFranja.constEnd())
            continue;

        for (auto itAula = actualIt.value().constBegin();
             itAula != actualIt.value().constEnd(); ++itAula) {
            const auto siguienteAula = siguienteIt.value().constFind(itAula.key());
            if (siguienteAula == siguienteIt.value().constEnd()) continue;

            model.AddEquality(LinearExpr(itAula.value()), LinearExpr(*siguienteAula))
                .OnlyEnforceIf(it.value());
        }
    }
}

// R4 + R5 + R6 — alineación de cada materia-curso en bloques enteros y un bloque por día.
void restriccionAlineacion(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
) {
    // R4: bloqueSimple = min(par, u[s], u[s+1]).
    for (auto it = vars.bloqueSimple.constBegin(); it != vars.bloqueSimple.constEnd(); ++it) {
        const auto [m, c, d, s] = it.key();
        const auto parIt = vars.par.constFind({c, d, s});
        const auto uActual = vars.materiaEnFranja.constFind({m, c, d, s});
        const auto uSiguiente = vars.materiaEnFranja.constFind({m, c, d, s + 1});
        if (parIt == vars.par.constEnd() || uActual == vars.materiaEnFranja.constEnd() ||
            uSiguiente == vars.materiaEnFranja.constEnd()) {
            model.AddEquality(LinearExpr(it.value()), 0);
            continue;
        }
        model.AddMinEquality(it.value(), {LinearExpr(*parIt), LinearExpr(*uActual),
                                          LinearExpr(*uSiguiente)});
    }

    for (int c = 0; c < config.cursos.size(); ++c) {
        for (const PlanMateriaConfig& mc : materiasDelCurso(config, config.cursos[c])) {
            const int m = mc.materiaIDx;
            const int horas = bloques(mc.horasSemanales, config.franja_horaria.duracion_minutos);

            // R5: 2·Σ simple ≥ h − (h mod 2); equivale a Σ simple ≥ ⌊h/2⌋.
            std::vector<BoolVar> simples;
            for (auto it = vars.bloqueSimple.constBegin(); it != vars.bloqueSimple.constEnd();
                 ++it) {
                const auto [sm, sc, sd, ss] = it.key();
                (void)sd; (void)ss;
                if (sm == m && sc == c) simples.push_back(it.value());
            }
            const int requeridos = (horas - (horas % 2)) / 2;
            model.AddGreaterOrEqual(
                simples.empty() ? LinearExpr(0) : LinearExpr::Sum(simples), requeridos);

            // R6: una sola aparición (bloque simple o participación en uno compartido) al día.
            //
            // Apariciones del día = franjas de la materia − bloques simples del día: un bloque
            // simple usa 2 franjas y cuenta 1; una franja en un par compartido cuenta 1; dos
            // bloques (o un bloque más un resto) suman 2. La fórmula literal del plan con
            // `u[s] + u[s+1] − simple` por par cuenta dos veces la franja de un par inactivo.
            for (int d = 0; d < config.dimensiones.num_dias; ++d) {
                LinearExpr apariciones(0);
                for (auto itU = vars.materiaEnFranja.constBegin();
                     itU != vars.materiaEnFranja.constEnd(); ++itU) {
                    const auto [um, uc, ud, us] = itU.key();
                    (void)us;
                    if (um == m && uc == c && ud == d)
                        apariciones = apariciones + LinearExpr(itU.value());
                }
                for (auto itS = vars.bloqueSimple.constBegin();
                     itS != vars.bloqueSimple.constEnd(); ++itS) {
                    const auto [sm, sc, sd, ss] = itS.key();
                    (void)ss;
                    if (sm == m && sc == c && sd == d)
                        apariciones = apariciones - LinearExpr(itS.value());
                }
                model.AddLessOrEqual(apariciones, 1);
            }
        }
    }
}

// R8 — definiciones de la separación blanda.
void restriccionSeparacion(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
) {
    (void)config;

    // apareceDia[m,c,d] = franjas de la materia en el día − bloques simples del día (0/1 por R6).
    for (auto it = vars.apareceDia.constBegin(); it != vars.apareceDia.constEnd(); ++it) {
        const auto [m, c, d] = it.key();
        LinearExpr apariciones(0);
        for (auto itU = vars.materiaEnFranja.constBegin();
             itU != vars.materiaEnFranja.constEnd(); ++itU) {
            const auto [um, uc, ud, us] = itU.key();
            (void)us;
            if (um == m && uc == c && ud == d)
                apariciones = apariciones + LinearExpr(itU.value());
        }
        for (auto itS = vars.bloqueSimple.constBegin();
             itS != vars.bloqueSimple.constEnd(); ++itS) {
            const auto [sm, sc, sd, ss] = itS.key();
            (void)ss;
            if (sm == m && sc == c && sd == d)
                apariciones = apariciones - LinearExpr(itS.value());
        }
        model.AddEquality(LinearExpr(it.value()), apariciones);
    }

    // R8 blanda: `incumpleSeparacion = min(apareceDia[d], apareceDia[d+1])`.
    for (auto it = vars.incumpleSeparacion.constBegin();
         it != vars.incumpleSeparacion.constEnd(); ++it) {
        const auto [m, c, d] = it.key();
        const auto actual = vars.apareceDia.constFind({m, c, d});
        const auto siguiente = vars.apareceDia.constFind({m, c, d + 1});
        if (actual == vars.apareceDia.constEnd() || siguiente == vars.apareceDia.constEnd()) {
            model.AddEquality(LinearExpr(it.value()), 0);
            continue;
        }
        model.AddMinEquality(it.value(), {LinearExpr(*actual), LinearExpr(*siguiente)});
    }
}

// R9 — los huecos son un sufijo del día: ocupada[s] ≥ ocupada[s+1].
void restriccionHuecosAlFinal(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
) {
    for (int c = 0; c < config.cursos.size(); ++c) {
        QVector<int> franjasTurno = slotsTurno(config, config.cursos[c].turno);
        std::sort(franjasTurno.begin(), franjasTurno.end());
        for (int d = 0; d < config.dimensiones.num_dias; ++d) {
            for (int i = 0; i + 1 < franjasTurno.size(); ++i) {
                const auto actual = vars.ocupada.constFind({c, d, franjasTurno[i]});
                const auto siguiente = vars.ocupada.constFind({c, d, franjasTurno[i + 1]});
                if (actual == vars.ocupada.constEnd() || siguiente == vars.ocupada.constEnd())
                    continue;
                model.AddGreaterOrEqual(LinearExpr(*actual), LinearExpr(*siguiente));
            }
        }
    }
}

void agregarRestriccionesBloques(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
) {
    restriccionOcupacion(model, config, vars);
    restriccionEmparejamiento(model, config, vars);
    restriccionMismaAulaDelPar(model, config, vars);
    restriccionAlineacion(model, config, vars);
    restriccionSeparacion(model, config, vars);
    restriccionHuecosAlFinal(model, config, vars);
}
