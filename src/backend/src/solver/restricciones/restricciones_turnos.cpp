#include "backend/solver/restricciones/restricciones_turnos.hpp"

void restriccionUnoPorTurno (
    [[maybe_unused]] CpModelBuilder& model,
    [[maybe_unused]] const SolverConfig& config,
    [[maybe_unused]] const VariablesSolver& vars
) {
    for (int c = 0; c < config.dimensiones.num_cursos; c++) {
        for (int d = 0; d < config.dimensiones.num_dias; d++) {
            for (int s = 0; s < config.dimensiones.num_slots_dia; s++) {
                
                std::vector<BoolVar> varsEnSlots;

                for (auto it = vars.assignment.constBegin(); it != vars.assignment.constEnd(); ++it) {
                    auto [p, m, curso, dia, slot] = it.key();
                    if (curso == c && dia == d && slot == s) {
                        varsEnSlots.push_back(it.value());
                    }
                }

                if (varsEnSlots.size() > 1) {
                    model.AddAtMostOne(varsEnSlots);
                }
            }
        }
    }
}

void restriccionBloquesRequeridos(
    [[maybe_unused]] CpModelBuilder& model,
    [[maybe_unused]] const SolverConfig& config,
    [[maybe_unused]] const VariablesSolver& vars
) {
    for (int c = 0; c < config.dimensiones.num_cursos; c++) {
        for (auto& mc : config.cursos[c].materias) {
            int m = mc.materiaIDx;
            int totalMinutos = mc.horasSemanales * 60;
            int duracionSlot = config.franja_horaria.duracion_minutos;
            int bloques = (totalMinutos + duracionSlot - 1) / duracionSlot;

            std::vector<BoolVar> varsMaterias;

            for (auto it = vars.assignment.constBegin(); it != vars.assignment.constEnd(); it++){
                auto [p, materia, curso, d, s] = it.key();
                if (curso == c && materia == m) {
                    varsMaterias.push_back(it.value());
                }
            }

            if (varsMaterias.size () > 1) {
                model.AddEquality(LinearExpr::Sum(varsMaterias), bloques);
            }
        }
    }
}

void restriccionHorasSemanalesProfesor(
    [[maybe_unused]] CpModelBuilder& model,
    [[maybe_unused]] const SolverConfig& config,
    [[maybe_unused]] const VariablesSolver& vars
) {
    int duracionSlot = config.franja_horaria.duracion_minutos;

    for (int p = 0; p < config.dimensiones.num_profesores; p++) {
        const auto& prof = config.profesores[p];
        int totalMinutos = prof.horas_aula * 60;
        int bloques = (totalMinutos + duracionSlot - 1) / duracionSlot;

        std::vector<BoolVar> varsProfesor;

        for (auto it = vars.assignment.constBegin(); it != vars.assignment.constEnd(); ++it) {
            auto [profesor, m, curso, dia, slot] = it.key();
            if (profesor == p) {
                varsProfesor.push_back(it.value());
            }
        }

        if (!varsProfesor.empty()) {
            model.AddEquality(LinearExpr::Sum(varsProfesor), bloques);
        }
    }
}

void agregarRestriccionesTurnos(
    [[maybe_unused]] CpModelBuilder& model,
    [[maybe_unused]] const SolverConfig& config,
    [[maybe_unused]] const VariablesSolver& vars
) {
    restriccionUnoPorTurno(model, config, vars);
    restriccionBloquesRequeridos(model, config, vars);
    restriccionHorasSemanalesProfesor(model, config, vars);
}
