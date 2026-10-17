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
#include <QMap>
#include <QSet>
#include <QStringList>

using namespace operations_research::sat;

namespace {

/// Slots declarados por el turno `nombre`, o vacío si no existe.
QVector<int> slotsTurno(const SolverConfig& config, const QString& nombre) {
    for (const TurnoConfig& t : config.turnos.turnos)
        if (t.nombre == nombre)
            return t.slot;
    return {};
}

int bloquesMateria(const SolverConfig& config, const PlanMateriaConfig& mc) {
    return bloques(mc.horasSemanales, config.franja_horaria.duracion_minutos);
}

/// Suma de bloques de todas las materias del plan del curso.
int bloquesCurso(const SolverConfig& config, const CursoSolverConfig& curso) {
    int total = 0;
    for (const PlanMateriaConfig& mc : materiasDelCurso(config, curso))
        total += bloquesMateria(config, mc);
    return total;
}

/// Franjas semanales del curso = nº_días × |franjas del turno declarado|.
int franjasCurso(const SolverConfig& config, const CursoSolverConfig& curso) {
    return config.dimensiones.num_dias * slotsTurno(config, curso.turno).size();
}

/// Máximo de clases que un docente puede cubrir de una materia-curso (una por franja disponible).
int bloquesDisponiblesDocente(const SolverConfig& config, const CursoSolverConfig& curso,
                              const ProfesorSolverConfig& prof) {
    int total = 0;
    for (int d = 0; d < config.dimensiones.num_dias; ++d)
        total += slotsDisponibles(config, curso, prof, d).size();
    return total;
}

QString nombreAula(const SolverConfig& config, int idx) {
    return (idx >= 0 && idx < config.aulas.size()) ? config.aulas[idx].nombre : QString::number(idx);
}

QString nombreMateria(const SolverConfig& config, int idx) {
    return (idx >= 0 && idx < config.materias.size()) ? config.materias[idx].nombre : QString::number(idx);
}

/// Categoría (1) entrada inválida: la configuración no es consistente como entrada del solver.
/// En el flujo real, V1–V15 la rechaza antes de invocar al solver; aquí cubre el modelo inválido
/// y las llamadas directas al solver con una configuración inconsistente.
QString causaEntradaInvalida(const SolverConfig& config) {
    const DimensionesConfig& d = config.dimensiones;
    if (d.num_profesores != config.profesores.size() ||
        d.num_materias   != config.materias.size() ||
        d.num_aulas      != config.aulas.size() ||
        d.num_cursos     != config.cursos.size())
        return QStringLiteral("Causa (1) entrada inválida: las dimensiones declaradas no coinciden con los tamaños de las listas.");

    for (const CursoSolverConfig& c : config.cursos) {
        if (c.turno.isEmpty())
            return QStringLiteral("Causa (1) entrada inválida: el curso %1 no declara turno.")
                .arg(claveCurso(c.anio, c.seccion));
        if (materiasDelCurso(config, c).isEmpty())
            return QStringLiteral("Causa (1) entrada inválida: el curso %1 no tiene materias en su plan '%2'.")
                .arg(claveCurso(c.anio, c.seccion)).arg(c.plan);
    }
    return {};
}

/// Categoría (2) conflicto de aula (D8/D9/D10). Devuelve "" si no aplica.
QString causaConflictoDeAula(const SolverConfig& config) {
    if (!config.aula_fija) {
        // D9: cursos móviles en un centro sin aulas.
        if (config.dimensiones.num_aulas <= 0)
            return QStringLiteral("Causa (2) conflicto de aula: el modo de aulas móviles no tiene aulas disponibles.");
        int total = 0;
        QSet<int> franjas;
        for (const CursoSolverConfig& c : config.cursos) {
            total += bloquesCurso(config, c);
            for (int s : slotsTurno(config, c.turno)) franjas.insert(s);
        }
        const int capacidad = config.dimensiones.num_dias * franjas.size() * config.dimensiones.num_aulas;
        if (total > capacidad)
            return QStringLiteral("Causa (2) conflicto de aula: los cursos móviles necesitan %1 bloques y las %2 aulas ofrecen %3 franjas.")
                .arg(total).arg(config.dimensiones.num_aulas).arg(capacidad);
        return {};
    }

    // D8: aulas fijas compartidas. Solo es causa del aula si cada curso cabe por separado
    // (si un curso no cabe en su turno, la causa es de persona: categoría 3).
    QMap<int, QVector<int>> cursosPorAula;
    for (int i = 0; i < config.cursos.size(); ++i)
        if (config.cursos[i].aula >= 0)
            cursosPorAula[config.cursos[i].aula].append(i);

    for (auto it = cursosPorAula.constBegin(); it != cursosPorAula.constEnd(); ++it) {
        int total = 0;
        QSet<int> franjas;
        bool todosCaben = true;
        for (int idx : it.value()) {
            const CursoSolverConfig& c = config.cursos[idx];
            const int bloques = bloquesCurso(config, c);
            if (bloques > franjasCurso(config, c)) { todosCaben = false; break; }
            total += bloques;
            for (int s : slotsTurno(config, c.turno)) franjas.insert(s);
        }
        if (!todosCaben) continue;
        const int capacidad = config.dimensiones.num_dias * franjas.size();
        if (total > capacidad)
            return QStringLiteral("Causa (2) conflicto de aula: el aula «%1» no da abasto para los cursos que la comparten (%2 bloques en %3 franjas).")
                .arg(nombreAula(config, it.key())).arg(total).arg(capacidad);
    }
    return {};
}

/// Categoría (3) conflicto de persona (D4/D11: el curso no cabe en su turno declarado).
QString causaConflictoDePersona(const SolverConfig& config) {
    for (const CursoSolverConfig& c : config.cursos) {
        const int bloques = bloquesCurso(config, c);
        const int franjas = franjasCurso(config, c);
        if (bloques > franjas)
            return QStringLiteral("Causa (3) conflicto de persona: el curso %1 necesita %2 bloques y su turno «%3» solo ofrece %4 franjas semanales.")
                .arg(claveCurso(c.anio, c.seccion)).arg(bloques).arg(c.turno).arg(franjas);
    }
    return {};
}

/// Categoría (4) conflicto de asignación de docente (D1/D2/D3: cobertura de la materia-curso).
QString causaConflictoDeAsignacion(const SolverConfig& config) {
    for (const CursoSolverConfig& c : config.cursos) {
        for (const PlanMateriaConfig& mc : materiasDelCurso(config, c)) {
            const int bloques = bloquesMateria(config, mc);
            bool hayCandidato = false;
            bool hayCandidatoSuficiente = false;
            for (const ProfesorSolverConfig& p : config.profesores) {
                if (!docenteCandidatoElegible(config, c, p, mc.materiaIDx)) continue;
                hayCandidato = true;
                if (bloquesDisponiblesDocente(config, c, p) >= bloques) { hayCandidatoSuficiente = true; break; }
            }
            const QString entidad = QStringLiteral("la materia «%1» del curso %2")
                .arg(nombreMateria(config, mc.materiaIDx)).arg(claveCurso(c.anio, c.seccion));
            if (!hayCandidato)
                return QStringLiteral("Causa (4) conflicto de asignación de docente: %1 no tiene ningún docente elegible.")
                    .arg(entidad);
            if (!hayCandidatoSuficiente)
                return QStringLiteral("Causa (4) conflicto de asignación de docente: %1 necesita %2 bloques y ningún docente elegible dispone de tantas franjas.")
                    .arg(entidad).arg(bloques);
        }
    }
    return {};
}

/// Primera categoría aplicable, en el orden de «Causa de un resultado no factible» (RF-6).
QString primeraCausaNoFactible(const SolverConfig& config) {
    QString causa = causaEntradaInvalida(config);
    if (!causa.isEmpty()) return causa;
    causa = causaConflictoDeAula(config);
    if (!causa.isEmpty()) return causa;
    causa = causaConflictoDePersona(config);
    if (!causa.isEmpty()) return causa;
    return causaConflictoDeAsignacion(config);
}

} // namespace


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
        case CpSolverStatus::INFEASIBLE: {
            errores.append(QStringLiteral("El modelo no tiene solución factible (INFEASIBLE)."));

            // RF-6: se informa la PRIMERA categoría aplicable, en el orden de la definición
            // «Causa de un resultado no factible».
            const QString causa = primeraCausaNoFactible(config);
            if (!causa.isEmpty())
                errores.append(causa);
            else
                errores.append(QStringLiteral("Causa: no se pudo identificar una categoría concreta; "
                                              "revise horas, aulas y disponibilidad de docentes."));

            errores.append(QStringLiteral("Entorno: %1 docentes, %2 cursos, %3 materias, %4 aulas.")
                .arg(config.profesores.size())
                .arg(config.cursos.size())
                .arg(config.materias.size())
                .arg(config.aulas.size()));
            break;
        }

        case CpSolverStatus::MODEL_INVALID:
            errores.append(QStringLiteral("El modelo CP-SAT no es válido (MODEL_INVALID)."));
            errores.append(QStringLiteral("Causa (1) entrada inválida: la configuración no produce un modelo válido para el solver."));
            break;

        case CpSolverStatus::UNKNOWN:
            errores.append(QStringLiteral("El solver no determinó la factibilidad (UNKNOWN)."));
            errores.append(QStringLiteral("Tiempo agotado o límite de exploración alcanzado; no se puede afirmar que no exista solución."));
            errores.append(QStringLiteral("Wall time: %1 s").arg(response.wall_time(), 0, 'f', 2));
            break;

        default:
            errores.append(QStringLiteral("El solver falló con estado: %1")
                .arg(QString::fromUtf8(CpSolverStatus_Name(response.status()).c_str())));
            break;
    }

    return errores;
}
