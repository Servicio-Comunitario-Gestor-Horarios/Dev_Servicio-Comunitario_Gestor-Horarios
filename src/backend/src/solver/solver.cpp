#include "backend/solver/solver.hpp"
#include "backend/solver/config/solver_config.hpp"
#include "backend/solver/variables.hpp"
#include "backend/solver/restricciones/restricciones_turnos.hpp"
#include "backend/solver/restricciones/restricciones_aulas.hpp"
#include "backend/solver/restricciones/restricciones_profesores.hpp"
#include "backend/solver/config/contrato_solver.hpp"
#include "ortools/sat/sat_parameters.pb.h"
#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model.pb.h"
#include "ortools/sat/cp_model_solver.h"
#include "ortools/sat/model.h"
#include "qvariant.h"
#include <QElapsedTimer>

using namespace operations_research::sat;

Solver::ResultadoSolver Solver::resolver(const SolverConfig& config) {
    QElapsedTimer timer;
    timer.start();

    ResultadoSolver resultado;
    resultado.exito = false;
    
    // Crear variables de decision
    vars = crearVariables(model, config);

    // Agregar restricciones
    agregarRestricciones(config);

    // Objetivo

    construirObjetivo(config);

    // Resolver CP-SAT (parametros deterministas: RNF-1)
    SatParameters params;
    params.set_num_search_workers(1);
    params.set_random_seed(0);

    Model sat_model;
    sat_model.Add(NewSatParameters(params));
    const CpSolverResponse response = SolveCpModel(model.Build(), &sat_model);

    // Verificar status
    if (response.status() == CpSolverStatus::OPTIMAL ||
        response.status() == CpSolverStatus::FEASIBLE) {
            resultado.exito = true;
            resultado.resultado = extraerSolucion(response, config);
        } else {
            resultado.errores = analizarFallo(response, config);
        }
    
    resultado.tiempo_ms = timer.elapsed();
    return resultado;
}

void Solver::construirObjetivo(const SolverConfig& config) {
    LinearExpr objetivo = LinearExpr(0);

    // S1: exceso de horas de aula
    for (int p = 0; p < config.dimensiones.num_profesores; ++p) {
        const auto eIt = vars.exceso.constFind(p);
        if (eIt != vars.exceso.constEnd()) {
            objetivo = objetivo + LinearExpr::Term(*eIt, ContratoSolver::PESO_HORAS);
        }
    }

    // S2: coste fijo por asignar a un SUPLENTE
    for (auto it = vars.asignacionDocente.constBegin(); it != vars.asignacionDocente.constEnd(); ++it) {
        const auto [p, c, m] = it.key();
        const bool titular = config.profesores[p].materias_asignadas.contains(m);
        if (!titular) {
            objetivo = objetivo + LinearExpr::Term(it.value(), ContratoSolver::COSTE_SUPLENTE);
        }
    }

    model.Minimize(objetivo);
}

void Solver::agregarRestricciones(const SolverConfig& config) {
    agregarRestriccionesTurnos(model, config, vars);
    agregarRestriccionesAulas(model, config, vars);
    agregarRestriccionProfesores(model, config, vars);
}

HorarioSalida Solver::extraerSolucion(const CpSolverResponse& response, const SolverConfig& config) {
    HorarioSalida horario;

    QMap<int, QMap<int, QVector<AsignacionOutput>>> porCursoDia;

    for (auto it = vars.assignment.constBegin(); it != vars.assignment.constEnd(); ++it) {
        if (!SolutionBooleanValue(response, it.value())) continue;

        auto [p, m, c, d, s] = it.key();

        // Aula: la fija del curso (indice) en modo fijo; en movil, la asignada.
        int aula = config.aula_fija ? config.cursos[c].aula : -1;
        if (aula < 0) {
            for (auto itA = vars.aulaAssignment.constBegin(); itA != vars.aulaAssignment.constEnd(); ++itA) {
                auto [ac, ad, as, aa] = itA.key();
                if (ac == c && ad == d && as == s && SolutionBooleanValue(response, itA.value())) {
                    aula = aa;
                    break;
                }
            }
        }

        AsignacionOutput asignacion;
        asignacion.slot = s;
        asignacion.materia = m;
        asignacion.profesor = p;
        asignacion.aula = aula;

        porCursoDia[c][d].append(asignacion);
    }

    for (auto itC = porCursoDia.constBegin(); itC != porCursoDia.constEnd(); ++itC) {
        int c = itC.key();
        const auto& diasMap = itC.value();

        CursoOutput curso;
        curso.turno = config.cursos[c].turno;

        for (auto itD = diasMap.constBegin(); itD != diasMap.constEnd(); ++itD) {
            DiaOutput dia;
            dia.dia = itD.key();
            dia.asignaciones = itD.value();
            curso.dias.append(dia);
        }

        horario.horarios[claveCurso(config.cursos[c].anio, config.cursos[c].seccion)] = curso;
    }

    MetadataSalida metadata;
    metadata.total_asignaciones = vars.assignment.size();
    metadata.cursos_generados = config.cursos.size();
    horario.metadata = metadata;

    return horario;
}

QStringList Solver::analizarFallo(const CpSolverResponse& response, const SolverConfig& config) {
    QStringList errores;

    switch (response.status()) {
        case CpSolverStatus::INFEASIBLE:
            errores.append("El modelo no tiene solucion factible (INFEASIBLE)");
            errores.append("Causas posibles:");
            errores.append("- Restricciones de horas semanales imposibles de cumplir");
            errores.append("- Solapamiento de docentes sin suficiente disponibilidad");
            errores.append("- Materias sin cupo en aulas disponibles");
            errores.append(QString("- Profesores: %1, Cursos: %2, Materias: %3")
                .arg(config.profesores.size())
                .arg(config.cursos.size())
                .arg(config.materias.size()));
            errores.append(QString("- Variables: %1 booleans, %2 integers, Conflicto: %3")
                .arg(response.num_booleans())
                .arg(response.num_integers())
                .arg(response.num_conflicts()));
            break;

        case CpSolverStatus::MODEL_INVALID:
            errores.append("Modelo CP-SAT invalido (MODEL_INVALID)");
            errores.append("Revise que las restricciones estan bien formadas");
            break;

        case CpSolverStatus::UNKNOWN:
            errores.append("Solver no determino factibilidad (UNKNOWN)");
            errores.append("Tiempo agotado o limite de exploracion alcanzado");
            errores.append(QString("Wall time: %1s").arg(response.wall_time(), 0, 'f', 2));
            break;

        default:
            errores.append(QString("Solver fallo con status: %1")
                .arg(CpSolverStatus_Name(response.status()).c_str()));
            break;
    }

    return errores;
}
