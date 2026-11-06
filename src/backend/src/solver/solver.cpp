#include "backend/solver/solver.hpp"
#include "backend/solver/config/solver_config.hpp"
#include "backend/solver/variables.hpp"
#include "backend/solver/restricciones/restricciones_turnos.hpp"
#include "backend/solver/restricciones/restricciones_aulas.hpp"
#include "backend/solver/restricciones/restricciones_profesores.hpp"
#include "backend/solver/restricciones/restricciones_agrupacion.hpp"
#include "backend/solver/config/contrato_solver.hpp"
#include "ortools/sat/sat_parameters.pb.h"
#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model.pb.h"
#include "ortools/sat/cp_model_solver.h"
#include "ortools/sat/model.h"
#include "qvariant.h"
#include <cmath>
#include <cstdio>
#include <QElapsedTimer>
#include <QMap>
#include <QSet>
#include <QStringList>

#include <tuple>

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
/// En el flujo real, V1–V19 la rechaza antes de invocar al solver; aquí cubre el modelo inválido
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

/// Primera categoría aplicable, en el orden de la definición «Causa de un resultado no factible» (RF-6).
QString primeraCausaNoFactible(const SolverConfig& config) {
    QString causa = causaEntradaInvalida(config);
    if (!causa.isEmpty()) return causa;
    causa = causaSaturacionPorTipo(config);
    if (!causa.isEmpty()) return causa;
    causa = causaConflictoDePersona(config);
    if (!causa.isEmpty()) return causa;
    return causaConflictoDeAsignacion(config);
}

/// ¿La respuesta contiene una solución utilizable? (OPTIMAL o FEASIBLE)
bool esFactible(CpSolverStatus status) {
    return status == CpSolverStatus::OPTIMAL || status == CpSolverStatus::FEASIBLE;
}

/// Clave de deduplicación del objetivo: (docente, día de la generación, slot).
///
/// Es la misma unidad que `FranjaSemana` de la 005, pero con el docente: Qt no ofrece `qHash`
/// para `std::tuple`, así que el proyecto usa una clave con `qHash` propia (como `FranjaSemana`).
struct FranjaPreferencia {
    int docente = 0;
    int dia = 0;
    int slot = 0;
};

inline bool operator==(const FranjaPreferencia& a, const FranjaPreferencia& b) {
    return a.docente == b.docente && a.dia == b.dia && a.slot == b.slot;
}

inline size_t qHash(const FranjaPreferencia& franja, size_t seed = 0) noexcept {
    return qHashMulti(seed, franja.docente, franja.dia, franja.slot);
}

} // namespace


Solver::ResultadoSolver Solver::resolver(const SolverConfig& config) {
    return resolver(config, PresupuestoFases{});
}

Solver::ResultadoSolver Solver::resolver(const SolverConfig& config,
                                         const PresupuestoFases& presupuesto) {
    QElapsedTimer timer;
    timer.start();

    ResultadoSolver resultado;
    resultado.exito = false;

    // FASE 1 — referencia (013, RF-1): el modelo de HOY, S1 + S2 + S3. Es la generación de
    // referencia contra la que se exige la igualdad de reparto y de exceso.
    prepararModelo(config);
    construirObjetivo(config);
    const CpSolverResponse referencia = ejecutar(presupuesto.referencia);

    if (!esFactible(referencia.status())) {
        resultado.tiempo_ms = timer.elapsed();
        resultado.errores = analizarFallo(referencia, config);
        return resultado;
    }

    // Se extrae AHORA: `vars` corresponde todavía a esta respuesta (con otro modelo, los
    // índices de variable no valdrían para leerla).
    HorarioSalida horario = extraerSolucion(referencia, config);

    // Sin preferencias no se ejecuta la fase 2: se devuelve exactamente la referencia (RF-4).
    if (config.preferencias.isEmpty()) {
        resultado.exito = true;
        resultado.tiempo_ms = timer.elapsed();
        horario.metadata.tiempo_ejecucion_ms = static_cast<int>(resultado.tiempo_ms);
        resultado.resultado = horario;
        return resultado;
    }

    // FASE 2 — cumplimiento de preferencias con z* y e* FIJADOS por igualdad (RF-1). La
    // garantía estructural es por construcción, no por magnitudes: la preferencia solo elige
    // dónde caen las clases.
    const AnclajeEstructural anclaje = extraerAnclaje(referencia);
    prepararModelo(config);
    fijarAnclaje(anclaje);
    model.Minimize(expresionPreferencias(config));
    const CpSolverResponse fase2 = ejecutar(presupuesto.preferencias);

    if (esFactible(fase2.status())) {
        // Se extrae AHORA, antes de reconstruir el modelo para la 2b.
        horario = extraerSolucion(fase2, config);

        // FASE 2b — S3 como desempate con la preferencia FIJADA a su valor de la 2a (RF-5).
        // Así la preferencia gana a la agrupación en agregado: no depende de 25 frente a 10.
        // `objective_value()` es la suma entera de la expresión de preferencias; se redondea
        // para pasarla como término independiente de la igualdad.
        const int64_t valorPreferencia =
            static_cast<int64_t>(std::llround(fase2.objective_value()));

        prepararModelo(config);
        fijarAnclaje(anclaje);
        model.AddEquality(expresionPreferencias(config), valorPreferencia);
        construirObjetivoSoloS3();
        const CpSolverResponse fase2b = ejecutar(presupuesto.agrupacion);

        if (esFactible(fase2b.status())) {
            horario = extraerSolucion(fase2b, config);
        } else if (fase2b.status() == CpSolverStatus::INFEASIBLE ||
                   fase2b.status() == CpSolverStatus::UNKNOWN) {
            // 2b→2a: el cumplimiento ya estaba garantizado; S3 solo desempataba. Se queda la
            // solución de la 2a (P7).
        } else {
            // MODEL_INVALID y otros errores internos se propagan como fallo.
            resultado.tiempo_ms = timer.elapsed();
            resultado.errores = analizarFallo(fase2b, config);
            return resultado;
        }
    } else if (fase2.status() == CpSolverStatus::INFEASIBLE ||
               fase2.status() == CpSolverStatus::UNKNOWN) {
        // P6/P7: 2a→1, se devuelve la referencia. La preferencia se incumple antes que cambiar
        // el reparto de docentes o el exceso de horas; nunca se declara infactible por preferir.
    } else {
        // MODEL_INVALID y otros errores internos sí se propagan como fallo.
        resultado.tiempo_ms = timer.elapsed();
        resultado.errores = analizarFallo(fase2, config);
        return resultado;
    }

    resultado.exito = true;
    resultado.tiempo_ms = timer.elapsed();
    horario.metadata.tiempo_ejecucion_ms = static_cast<int>(resultado.tiempo_ms);
    resultado.resultado = horario;
    return resultado;
}

void Solver::prepararModelo(const SolverConfig& config) {
    // No arrastrar estado entre resoluciones: `model` y `vars` son miembros y una segunda
    // llamada acumularía variables y restricciones. Se reinician al inicio (F3), de modo que
    // dos llamadas a la misma instancia equivalen a dos instancias nuevas.
    model = CpModelBuilder();
    vars = VariablesSolver();

    vars = crearVariables(model, config);
    agregarRestricciones(config);
}

CpSolverResponse Solver::ejecutar(double presupuesto) {
    // Parámetros deterministas: un worker, seed 0 y presupuesto determinista (RNF-1).
    SatParameters params;
    params.set_num_search_workers(1);
    params.set_random_seed(0);
    params.set_max_deterministic_time(presupuesto);

    Model sat_model;
    sat_model.Add(NewSatParameters(params));
    return SolveCpModel(model.Build(), &sat_model);
}

Solver::AnclajeEstructural Solver::extraerAnclaje(const CpSolverResponse& response) const {
    AnclajeEstructural anclaje;
    for (auto it = vars.asignacionDocente.constBegin(); it != vars.asignacionDocente.constEnd();
         ++it) {
        anclaje.docentePorMateria.insert(it.key(),
                                         SolutionBooleanValue(response, it.value()) ? 1 : 0);
    }
    for (auto it = vars.exceso.constBegin(); it != vars.exceso.constEnd(); ++it) {
        anclaje.excesoPorDocente.insert(
            it.key(), static_cast<int>(SolutionIntegerValue(response, it.value())));
    }
    return anclaje;
}

void Solver::fijarAnclaje(const AnclajeEstructural& anclaje) {
    for (auto it = vars.asignacionDocente.constBegin(); it != vars.asignacionDocente.constEnd();
         ++it) {
        model.AddEquality(it.value(), anclaje.docentePorMateria.value(it.key(), 0));
    }
    for (auto it = vars.exceso.constBegin(); it != vars.exceso.constEnd(); ++it) {
        model.AddEquality(it.value(), anclaje.excesoPorDocente.value(it.key(), 0));
    }
}

LinearExpr Solver::expresionPreferencias(const SolverConfig& config) const {
    // Deduplicación por (docente, día, slot) y sentido: una franja alcanzada por dos
    // preferencias del MISMO sentido se cuenta una sola vez (P1). Dos sentidos opuestos sobre
    // la misma franja se aplican los dos y se compensan (V19 no comprueba solapes).
    QSet<FranjaPreferencia> evitadas;
    QSet<FranjaPreferencia> preferidas;
    for (const PreferenciaSolverConfig& preferencia : config.preferencias) {
        QSet<FranjaPreferencia>* destino = nullptr;
        if (preferencia.sentido == QLatin1String("evitada"))
            destino = &evitadas;
        else if (preferencia.sentido == QLatin1String("preferida"))
            destino = &preferidas;
        if (!destino)
            continue;   // sentido desconocido: V19 lo rechaza; aquí no se inventa un término

        for (int slot : preferencia.slot)
            destino->insert(FranjaPreferencia{preferencia.docente, preferencia.dia, slot});
    }

    // Penaliza lo evitado y premia lo preferido con el mismo peso y la misma unidad (por franja
    // de clase). El premio es un coste negativo equivalente (RF-4).
    LinearExpr expresion = LinearExpr(0);
    for (auto it = vars.assignment.constBegin(); it != vars.assignment.constEnd(); ++it) {
        const auto [p, m, c, d, s] = it.key();
        const FranjaPreferencia franja{p, d, s};
        if (evitadas.contains(franja)) {
            expresion =
                expresion + LinearExpr::Term(it.value(), ContratoSolver::PESO_PREFERENCIA);
        }
        if (preferidas.contains(franja)) {
            expresion =
                expresion + LinearExpr::Term(it.value(), -ContratoSolver::PESO_PREFERENCIA);
        }
    }
    return expresion;
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

    // S3: penalización por fragmentación (agrupación de bloques) - RF-2
    // Penalización = PESO_AGRUPACION · Σ (isolated + longGroup)
    //   isolated  → grupo de 1          (1 unidad)
    //   longGroup → 4.ª franja o más    (1 unidad por franja extra)
    //   grupos de 2–3 → 0
    // Esto equivale a f(len): 1 → 1, 2–3 → 0, ≥4 → len − 3.
    for (auto it = vars.isolated.constBegin(); it != vars.isolated.constEnd(); ++it)
        objetivo = objetivo + LinearExpr::Term(it.value(), ContratoSolver::PESO_AGRUPACION);

    model.Minimize(objetivo);
}

void Solver::construirObjetivoSoloS3() {
    // S3 tal cual lo define hoy el motor: PESO_AGRUPACION por cada clase AISLADA (grupo de 1).
    // Es el mismo término que la fase 1, sin S1 ni S2 (ya fijados por igualdad).
    LinearExpr objetivo = LinearExpr(0);
    for (auto it = vars.isolated.constBegin(); it != vars.isolated.constEnd(); ++it)
        objetivo = objetivo + LinearExpr::Term(it.value(), ContratoSolver::PESO_AGRUPACION);
    model.Minimize(objetivo);
}

void Solver::agregarRestricciones(const SolverConfig& config) {
    agregarRestriccionesTurnos(model, config, vars);
    agregarRestriccionesAulas(model, config, vars);
    agregarRestriccionProfesores(model, config, vars);
    agregarRestriccionesAgrupacion(model, config, vars);
}

HorarioSalida Solver::extraerSolucion(const CpSolverResponse& response, const SolverConfig& config) {
    HorarioSalida horario;

    QMap<int, QMap<int, QVector<AsignacionOutput>>> porCursoDia;

    // Metadata real de la salida: clases emitidas (no variables booleanas del modelo)
    // y docentes distintos con al menos una clase.
    int totalAsignaciones = 0;
    QSet<int> docentesAsignados;

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
        ++totalAsignaciones;
        if (p >= 0)
            docentesAsignados.insert(p);
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
    // Clases (asignaciones) realmente extraídas de la solución; no el número de
    // variables booleanas del modelo.
    metadata.total_asignaciones = totalAsignaciones;
    metadata.profesores_asignados = docentesAsignados.size();
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
