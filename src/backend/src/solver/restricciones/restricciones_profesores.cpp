#include "backend/solver/restricciones/restricciones_profesores.hpp"

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
                    auto [profesor, m, curso, dia, slot] = it.key();
                    if (profesor == p && dia == d && slot == s) {
                        varsProfesor.push_back(it.value());
                    }
                }

                if (varsProfesor.size() > 1) {
                    model.AddAtMostOne(varsProfesor);
                }
            }
        }
    }
}

void restriccionProfesorPorTurno(
    [[maybe_unused]] CpModelBuilder& model,
    [[maybe_unused]] const SolverConfig& config,
    [[maybe_unused]] const VariablesSolver& vars
) {
    for (int p = 0; p < config.dimensiones.num_profesores; p++) {
        const auto& prof = config.profesores[p];
        if (prof.turno.isEmpty()) continue;

        QVector<int> slotsTurno;
        for (const auto& t : config.turnos.turnos) {
            if (t.nombre == prof.turno) {
                slotsTurno = t.slot;
                break;
            }
        }
        if (slotsTurno.isEmpty()) continue;

        QSet<int> slotsValidos(slotsTurno.begin(), slotsTurno.end());

        for (auto it = vars.assignment.constBegin(); it != vars.assignment.constEnd(); ++it) {
            auto [profesor, m, curso, dia, slot] = it.key();
            if (profesor == p && !slotsValidos.contains(slot)) {
                model.AddEquality(it.value(), 0);
            }
        }
    }
}

void restriccionProfesorPorPlan(
    [[maybe_unused]] CpModelBuilder& model,
    [[maybe_unused]] const SolverConfig& config,
    [[maybe_unused]] const VariablesSolver& vars
) {
    for (int p = 0; p < config.dimensiones.num_profesores; p++) {
        const auto& prof = config.profesores[p];
        if (prof.plan.isEmpty()) continue;

        for (auto it = vars.assignment.constBegin(); it != vars.assignment.constEnd(); ++it) {
            auto [profesor, m, curso, dia, slot] = it.key();
            if (profesor == p && config.cursos[curso].plan != prof.plan) {
                model.AddEquality(it.value(), 0);
            }
        }
    }
}

void agregarRestriccionProfesores(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
) {
    restriccionDocenteNoSolapado(model, config, vars);
    restriccionProfesorPorTurno(model, config, vars);
    restriccionProfesorPorPlan(model, config, vars);
}
