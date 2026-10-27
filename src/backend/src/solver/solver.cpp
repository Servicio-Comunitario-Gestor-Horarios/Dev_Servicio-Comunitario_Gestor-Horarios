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

/// Máximo de clases que un docente puede cubrir de una materia-curso (una por franja disponible).
int bloquesDisponiblesDocente(const SolverConfig& config, const CursoSolverConfig& curso,
                              const ProfesorSolverConfig& prof) {
    int total = 0;
    for (int d = 0; d < config.dimensiones.num_dias; ++d)
        total += slotsDisponibles(config, curso, prof, d).size();
    return total;
}

QString nombreMateria(const SolverConfig& config, int idx) {
    return (idx >= 0 && idx < config.materias.size()) ? config.materias[idx].nombre : QString::number(idx);
}

/// Clave normalizada del tipo requerido por la materia `m` ("" si el índice no existe).
QString claveMateria(const SolverConfig& config, int m) {
    if (m < 0 || m >= config.materias.size()) return QString();
    return claveTipo(config.materias[m].tipo);
}

/// Categoría (1) entrada inválida: la configuración no es consistente como entrada del solver.
/// En el flujo real, V1–V18 la rechaza antes de invocar al solver; aquí cubre el modelo inválido
/// y las llamadas directas al solver con una configuración inconsistente.
QString causaEntradaInvalida(const SolverConfig& config) {
    const DimensionesConfig& d = config.dimensiones;
    if (d.num_profesores != config.profesores.size() ||
        d.num_materias   != config.materias.size() ||
        d.num_aulas      != config.aulas.size() ||
        d.num_cursos     != config.cursos.size())
        return QStringLiteral("Causa (1) entrada inválida: las dimensiones declaradas no coinciden con los tamaños de las listas.");

    QSet<QString> tiposDeAula;
    for (const AulaSolverConfig& a : config.aulas) {
        const QString clave = claveTipo(a.tipo);
        if (!clave.isEmpty()) tiposDeAula.insert(clave);
    }

    for (const CursoSolverConfig& c : config.cursos) {
        if (c.turno.isEmpty())
            return QStringLiteral("Causa (1) entrada inválida: el curso %1 no declara turno.")
                .arg(claveCurso(c.anio, c.seccion));
        if (materiasDelCurso(config, c).isEmpty())
            return QStringLiteral("Causa (1) entrada inválida: el curso %1 no tiene materias en su plan '%2'.")
                .arg(claveCurso(c.anio, c.seccion)).arg(c.plan);

        // V17 (RF-5): el tipo requerido por una materia tiene que tener aulas. Es entrada
        // inválida, no conflicto de aula: el modelo no puede cumplir la regla de tipo.
        for (const PlanMateriaConfig& mc : materiasDelCurso(config, c)) {
            const QString clave = claveMateria(config, mc.materiaIDx);
            if (clave.isEmpty() || tiposDeAula.contains(clave)) continue;
            return QStringLiteral("Causa (1) entrada inválida: la materia «%1» del curso %2 requiere "
                                  "el tipo de aula «%3» y no hay ninguna aula de ese tipo.")
                .arg(nombreMateria(config, mc.materiaIDx))
                .arg(claveCurso(c.anio, c.seccion))
                .arg(config.materias[mc.materiaIDx].tipo.trimmed());
        }
    }
    return {};
}

/// Categoría (2) conflicto de aula: la demanda de un tipo supera la capacidad de sus aulas
/// (RF-6). Demanda = bloques de las materias que requieren el tipo en todos los cursos;
/// capacidad = nº de aulas del tipo × franjas de la semana que aportan los turnos de los cursos
/// que lo requieren (`capacidadDelTipo`). No se descuenta el bloqueo de las aulas de referencia:
/// es una regla del modelo, no una cifra de demanda. Un tipo sin aulas es entrada inválida (V17),
/// así que aquí solo se miden tipos con aulas.
QString causaSaturacionPorTipo(const SolverConfig& config) {
    // Excepción de precedencia de RF-6 (aprobada el 2026-10-26): si un curso no cabe en su turno,
    // la causa es (3) conflicto de persona y una (2) mecánica taparía el motivo concreto.
    for (const CursoSolverConfig& c : config.cursos)
        if (bloquesCurso(config, c) > franjasSemanales(config, c)) return {};

    // Nombre visible del tipo (el de la primera materia que lo requiere), por clave. El QMap
    // recorre en orden de clave: el diagnóstico es determinista (RNF-1).
    QMap<QString, QString> nombreVisible;
    for (const CursoSolverConfig& c : config.cursos) {
        for (const PlanMateriaConfig& mc : materiasDelCurso(config, c)) {
            const QString clave = claveMateria(config, mc.materiaIDx);
            if (clave.isEmpty()) continue;   // V16 lo rechaza como entrada inválida
            if (!nombreVisible.contains(clave))
                nombreVisible.insert(clave, config.materias[mc.materiaIDx].tipo.trimmed());
        }
    }

    for (auto it = nombreVisible.constBegin(); it != nombreVisible.constEnd(); ++it) {
        // Un tipo sin aulas es entrada inválida (V17), no conflicto de aula. Un tipo sin franjas
        // significa que ningún curso que lo requiere tiene turno válido: lo juzga la categoría (3).
        const int aulas = aulasDelTipo(config, it.key());
        if (aulas == 0) continue;
        if (franjasDelTipo(config, it.key()).isEmpty()) continue;

        const int demanda = demandaDelTipo(config, it.key());
        const int capacidad = capacidadDelTipo(config, it.key());
        if (demanda > capacidad) {
            return QStringLiteral("Causa (2) conflicto de aula: el tipo «%1» necesita %2 bloques y "
                                  "sus %3 aulas ofrecen %4 franjas.")
                .arg(it.value())
                .arg(demanda)
                .arg(aulas)
                .arg(capacidad);
        }
    }
    return {};
}

/// Categoría (3) conflicto de persona (D4/D11: el curso no cabe en su turno declarado).
QString causaConflictoDePersona(const SolverConfig& config) {
    for (const CursoSolverConfig& c : config.cursos) {
        const int bloques = bloquesCurso(config, c);
        const int franjas = franjasSemanales(config, c);
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
    causa = causaSaturacionPorTipo(config);
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

        // Aula real de la clase: la única `y[c][d][s][a]` activa, también en modo de aula
        // fija (RF-4). Una clase desplazada informa el aula donde se imparte, no la referencia.
        int aula = -1;
        for (auto itA = vars.aulaAssignment.constBegin(); itA != vars.aulaAssignment.constEnd(); ++itA) {
            const auto [ac, ad, as, aa] = itA.key();
            if (ac == c && ad == d && as == s && SolutionBooleanValue(response, itA.value())) {
                aula = aa;
                break;
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
                errores.append(QStringLiteral("Causa: no atribuible. Ninguna categoría de (1) a (4) "
                                              "explica el fallo; revise la capacidad de los tipos de "
                                              "aula, las franjas de los turnos y la disponibilidad "
                                              "de los docentes."));

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
