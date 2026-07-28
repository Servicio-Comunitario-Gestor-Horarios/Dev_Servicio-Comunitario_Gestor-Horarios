#include "backend/solver/restricciones/restricciones_aulas.hpp"

void restriccionAulaNoSolapada(
    [[maybe_unused]] CpModelBuilder& model,
    [[maybe_unused]] const SolverConfig& config,
    [[maybe_unused]] const VariablesSolver& vars
) {
    for (int a = 0; a < config.dimensiones.num_aulas; a++) {
        for (int d = 0; d < config.dimensiones.num_dias; d++) {
            for (int s = 0; s < config.dimensiones.num_slots_dia; s++) {

                std::vector<BoolVar> varsAula;

                for (auto it = vars.aulaAssignment.constBegin(); it != vars.aulaAssignment.constEnd(); ++it) {
                    auto [curso, dia, slot, aula] = it.key();
                    if (aula == a && dia == d && slot == s) {
                        varsAula.push_back(it.value());
                    }
                }

                if (varsAula.size() > 1) {
                    model.AddAtMostOne(varsAula);
                }
            }
        }
    }
}

void restriccionMateriaUnicaPorAula(
    [[maybe_unused]] CpModelBuilder& model,
    [[maybe_unused]] const SolverConfig& config,
    [[maybe_unused]] const VariablesSolver& vars
) {
    for (int a = 0; a < config.dimensiones.num_aulas; a++) {
        for (int d = 0; d < config.dimensiones.num_dias; d++) {
            for (int s = 0; s < config.dimensiones.num_slots_dia; s++) {

                QMap<int, std::vector<BoolVar>> materiasPorAula;

                for (auto it = vars.aulaAssignment.constBegin(); it != vars.aulaAssignment.constEnd(); ++it) {
                    auto [curso, dia, slot, aula] = it.key();
                    if (aula == a && dia == d && slot == s) {
                        for (const auto& mc : config.cursos[curso].materias) {
                            materiasPorAula[mc.materiaIDx].push_back(it.value());
                        }
                    }
                }

                for (auto matIt = materiasPorAula.constBegin(); matIt != materiasPorAula.constEnd(); ++matIt) {
                    if (matIt.value().size() > 1) {
                        model.AddAtMostOne(matIt.value());
                    }
                }
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
    restriccionMateriaUnicaPorAula(model, config, vars);
}
