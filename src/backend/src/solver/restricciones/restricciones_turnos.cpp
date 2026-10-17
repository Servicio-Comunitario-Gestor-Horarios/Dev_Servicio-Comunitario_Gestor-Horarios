#include "backend/solver/restricciones/restricciones_turnos.hpp"
#include "backend/solver/config/contrato_solver.hpp"

#include <vector>

// D4 — un curso no tiene dos clases a la vez.
void restriccionUnoPorTurno(
    [[maybe_unused]] CpModelBuilder& model,
    [[maybe_unused]] const SolverConfig& config,
    [[maybe_unused]] const VariablesSolver& vars
) {
    for (int c = 0; c < config.dimensiones.num_cursos; c++) {
        for (int d = 0; d < config.dimensiones.num_dias; d++) {
            for (int s = 0; s < config.dimensiones.num_slots_dia; s++) {
                std::vector<BoolVar> varsEnSlot;
                for (auto it = vars.assignment.constBegin(); it != vars.assignment.constEnd(); ++it) {
                    const auto [p, m, curso, dia, slot] = it.key();
                    (void)p; (void)m;
                    if (curso == c && dia == d && slot == s) varsEnSlot.push_back(it.value());
                }
                if (varsEnSlot.size() > 1) model.AddAtMostOne(varsEnSlot);
            }
        }
    }
}

// S1 — tope de horas de aula del profesor + exceso penalizado (sustituye la igualdad dura).
void restriccionTopeHorasProfesor(
    [[maybe_unused]] CpModelBuilder& model,
    [[maybe_unused]] const SolverConfig& config,
    [[maybe_unused]] const VariablesSolver& vars
) {
    const int dur = config.franja_horaria.duracion_minutos;

    for (int p = 0; p < config.dimensiones.num_profesores; ++p) {
        std::vector<BoolVar> xDe;
        for (auto it = vars.assignment.constBegin(); it != vars.assignment.constEnd(); ++it)
            if (std::get<0>(it.key()) == p) xDe.push_back(it.value());
        if (xDe.empty()) continue;

        const auto excesoIt = vars.exceso.constFind(p);
        if (excesoIt == vars.exceso.constEnd()) continue;

        const int tope = bloquesAula(config.profesores[p].horas_aula, dur);
        model.AddLessOrEqual(LinearExpr::Sum(xDe),
                             LinearExpr(*excesoIt) + tope);
    }
}

void agregarRestriccionesTurnos(
    CpModelBuilder& model,
    const SolverConfig& config,
    const VariablesSolver& vars
) {
    restriccionUnoPorTurno(model, config, vars);
    restriccionTopeHorasProfesor(model, config, vars);
}
