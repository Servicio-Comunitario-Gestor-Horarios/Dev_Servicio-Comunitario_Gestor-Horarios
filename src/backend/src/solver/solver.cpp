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

/// Bloques desplazados de un grupo de cursos que comparten aula de referencia: cuántos bloques
/// de cada tipo de aula requieren y en qué franjas pueden caer (unión de los turnos de los cursos
/// que aportan esos bloques).
struct DesplazamientosDeGrupo {
    QMap<QString, int> bloques;                   ///< tipo requerido → bloques desplazados
    QMap<QString, QSet<int>> franjasAlcanzables;  ///< tipo requerido → slots donde pueden caer
};

/// Nº de aulas del CENTRO de un tipo de aula (0 si no hay ninguna: V17 ya lo rechaza como entrada
/// inválida, así que quien divide lo comprueba antes).
int aulasDelTipo(const SolverConfig& config, const QString& clave) {
    int aulas = 0;
    for (const AulaSolverConfig& a : config.aulas)
        if (claveTipo(a.tipo) == clave) ++aulas;
    return aulas;
}

/// Cota inferior, en franjas distintas, de las que necesitan las clases desplazadas de un grupo
/// (palomar: una clase por franja y aula):
///
///   max_t ⌈bloques_t / nº de aulas del centro de tipo t⌉
///
/// Con A aulas del tipo t, k clases desplazadas que lo requieren no caben en menos de ⌈k/A⌉
/// franjas distintas. Es una cota inferior por capacidad del tipo y puede quedarse corta: la
/// colocación real puede necesitar más franjas (competencia con otros grupos, con las clases que
/// se quedan, turnos que no se solapan...), de modo que una entrada infactible puede quedarse sin
/// causa (2). El nº de aulas es el del CENTRO (no el del grupo): es la capacidad a la que pueden
/// optar esos bloques.
int palomarPorTipo(const SolverConfig& config, const QMap<QString, int>& bloques) {
    int total = 0;
    for (auto it = bloques.constBegin(); it != bloques.constEnd(); ++it) {
        const int aulas = aulasDelTipo(config, it.key());
        if (aulas <= 0) continue;   // V17: entrada inválida, no conflicto de aula
        total = qMax(total, (it.value() + aulas - 1) / aulas);   // ⌈k / A⌉
    }
    return total;
}

/// Franjas que las clases desplazadas de un grupo dejan bloqueadas en su aula de referencia,
/// limitadas a las `franjasContadas` con las que se midió la capacidad del tipo. Para cada tipo t
/// con k bloques desplazados y A aulas en el centro:
///
///   cota_t = max(0, ⌈k / A⌉ − |alcanzables_t \ franjasContadas|)
///
/// ⌈k/A⌉ clases no caben en menos franjas distintas, y de ellas como mucho
/// |alcanzables_t \ franjasContadas| pueden caer FUERA de las franjas contadas, así que al menos
/// esa diferencia cae dentro (nunca más que |alcanzables_t ∩ franjasContadas|, de ahí el tope).
/// Invariante: el descuento se limita a las franjas contadas en la capacidad y el palomar es cota
/// inferior; nunca resta franjas que no se hayan sumado. Puede quedarse corta por lo dicho en
/// `palomarPorTipo`.
int franjasBloqueadasEnFranjasContadas(const SolverConfig& config,
                                       const QMap<QString, int>& bloques,
                                       const QMap<QString, QSet<int>>& franjasAlcanzables,
                                       const QSet<int>& franjasContadas) {
    int total = 0;
    for (auto it = bloques.constBegin(); it != bloques.constEnd(); ++it) {
        const int aulas = aulasDelTipo(config, it.key());
        if (aulas <= 0) continue;   // V17: entrada inválida, no conflicto de aula
        const QSet<int> alcanzables = franjasAlcanzables.value(it.key());
        int fuera = 0;
        for (int s : alcanzables)
            if (!franjasContadas.contains(s)) ++fuera;
        const int dentro = alcanzables.size() - fuera;
        const int palomar = (it.value() + aulas - 1) / aulas;              // ⌈k / A⌉
        total = qMax(total, qMin(qMax(palomar - fuera, 0), dentro));
    }
    return total;
}

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

/// Clave normalizada del tipo requerido por la materia `m` ("" si el índice no existe).
QString claveMateria(const SolverConfig& config, int m) {
    if (m < 0 || m >= config.materias.size()) return QString();
    return claveTipo(config.materias[m].tipo);
}

/// Clave normalizada del tipo del aula `a` ("" si el índice no existe).
QString claveAula(const SolverConfig& config, int a) {
    if (a < 0 || a >= config.aulas.size()) return QString();
    return claveTipo(config.aulas[a].tipo);
}

/// Categoría (1) entrada inválida: la configuración no es consistente como entrada del solver.
/// En el flujo real, V1–V17 la rechaza antes de invocar al solver; aquí cubre el modelo inválido
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

/// Categoría (2) conflicto de aula: la demanda de un tipo supera la capacidad de sus aulas.
/// Demanda = bloques de las materias que requieren el tipo (en todos los cursos); capacidad =
/// nº de aulas de ese tipo × días × franjas que aportan los turnos de los cursos que lo piden.
/// Un tipo sin aulas es entrada inválida (V17), así que aquí solo se miden tipos con aulas.
QString causaSaturacionPorTipo(const SolverConfig& config) {
    // Si un curso no cabe en su propio turno, la causa es de persona (categoría 3): la
    // saturación de un tipo no debe ocultar esa categoría.
    for (const CursoSolverConfig& c : config.cursos)
        if (bloquesCurso(config, c) > franjasCurso(config, c)) return {};

    struct DemandaTipo {
        int bloques = 0;
        QString nombre;          ///< nombre visible del tipo, tal como lo declara la materia
        QSet<int> franjas;       ///< slots que aportan los turnos de los cursos que lo requieren
    };
    QMap<QString, DemandaTipo> demanda;

    for (const CursoSolverConfig& c : config.cursos) {
        const QVector<int> franjasTurno = slotsTurno(config, c.turno);
        for (const PlanMateriaConfig& mc : materiasDelCurso(config, c)) {
            const QString clave = claveMateria(config, mc.materiaIDx);
            if (clave.isEmpty()) continue;   // V16 lo rechaza como entrada inválida
            DemandaTipo& d = demanda[clave];
            d.bloques += bloquesMateria(config, mc);
            if (d.nombre.isEmpty()) d.nombre = config.materias[mc.materiaIDx].tipo.trimmed();
            for (int s : franjasTurno) d.franjas.insert(s);
        }
    }

    // Bloques desplazados de cada curso: los que no son del tipo de su aula de referencia. Cada
    // uno ocupa una franja propia y, mientras tanto, deja la referencia vacía y bloqueada
    // (RF-3): consume una franja de las aulas de ese tipo aunque no se imparta en ellas.
    QVector<int> desplazadosPorCurso(config.cursos.size(), 0);
    if (config.aula_fija) {
        for (int i = 0; i < config.cursos.size(); ++i) {
            const CursoSolverConfig& c = config.cursos[i];
            const QString claveReferencia = claveAula(config, c.aula);
            if (claveReferencia.isEmpty()) continue;   // sin referencia no hay bloqueo
            for (const PlanMateriaConfig& mc : materiasDelCurso(config, c))
                if (claveMateria(config, mc.materiaIDx) != claveReferencia)
                    desplazadosPorCurso[i] += bloquesMateria(config, mc);
        }
    }

    for (auto it = demanda.constBegin(); it != demanda.constEnd(); ++it) {
        const DemandaTipo& d = it.value();
        if (d.franjas.isEmpty()) continue;   // curso sin turno válido: lo juzga la categoría (3)

        const int aulas = aulasDelTipo(config, it.key());
        if (aulas == 0) continue;   // V17: entrada inválida (categoría 1), no conflicto de aula

        // Franjas que las clases desplazadas dejan bloqueadas en las referencias de este tipo,
        // contando solo las que caen dentro de `d.franjas` (las que se sumaron en la capacidad).
        // Se agrupa por aula de referencia: dentro de una misma referencia el bloqueo es el
        // palomar sobre el conjunto de cursos, mientras que entre referencias distintas del mismo
        // tipo las franjas bloqueadas sí se acumulan.
        QMap<int, DesplazamientosDeGrupo> bloqueosPorReferencia;
        for (int i = 0; i < config.cursos.size(); ++i) {
            if (desplazadosPorCurso[i] == 0) continue;
            const CursoSolverConfig& c = config.cursos[i];
            const QString claveReferencia = claveAula(config, c.aula);
            if (claveReferencia != it.key()) continue;
            DesplazamientosDeGrupo& grupo = bloqueosPorReferencia[c.aula];
            const QVector<int> franjasCurso = slotsTurno(config, c.turno);
            for (const PlanMateriaConfig& mc : materiasDelCurso(config, c)) {
                const QString clave = claveMateria(config, mc.materiaIDx);
                if (clave.isEmpty() || clave == claveReferencia) continue;
                grupo.bloques[clave] += bloquesMateria(config, mc);
                for (int s : franjasCurso) grupo.franjasAlcanzables[clave].insert(s);
            }
        }
        int bloqueadas = 0;
        for (auto itBloqueo = bloqueosPorReferencia.constBegin();
             itBloqueo != bloqueosPorReferencia.constEnd(); ++itBloqueo)
            bloqueadas += franjasBloqueadasEnFranjasContadas(
                config, itBloqueo->bloques, itBloqueo->franjasAlcanzables, d.franjas);

        const int capacidad = config.dimensiones.num_dias * d.franjas.size() * aulas;
        const int disponibles = capacidad - qMin(bloqueadas, capacidad);
        if (d.bloques > disponibles) {
            if (bloqueadas > 0)
                return QStringLiteral("Causa (2) conflicto de aula: el tipo «%1» necesita %2 bloques y sus %3 aulas ofrecen %4 franjas (%5 las bloquean clases desplazadas).")
                    .arg(d.nombre).arg(d.bloques).arg(aulas).arg(capacidad).arg(qMin(bloqueadas, capacidad));
            return QStringLiteral("Causa (2) conflicto de aula: el tipo «%1» necesita %2 bloques y sus %3 aulas ofrecen %4 franjas.")
                .arg(d.nombre).arg(d.bloques).arg(aulas).arg(capacidad);
        }
    }
    return {};
}

/// Categoría (2) conflicto de aula (D8/D9/D10 y saturación por tipo). Devuelve "" si no aplica.
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
        return causaSaturacionPorTipo(config);
    }

    // D8: aulas de referencia compartidas. Solo las clases que se QUEDAN en la referencia (las
    // del tipo del aula) la ocupan; las de otro tipo se desplazan, pero mientras ocupan su franja
    // dejan la referencia vacía y bloqueada para los demás cursos (RF-3). La capacidad disponible
    // es la del aula menos esas franjas bloqueadas, estimadas con una COTA INFERIOR por capacidad
    // del tipo (palomar: con A aulas del tipo, k clases desplazadas no caben en menos de ⌈k/A⌉
    // franjas). La cota puede quedarse corta y el descuento nunca excede la capacidad contada;
    // solo es causa del aula si cada curso cabe por separado en su turno (si no, es categoría 3).
    QMap<int, QVector<int>> cursosPorAula;
    for (int i = 0; i < config.cursos.size(); ++i)
        if (config.cursos[i].aula >= 0)
            cursosPorAula[config.cursos[i].aula].append(i);

    for (auto it = cursosPorAula.constBegin(); it != cursosPorAula.constEnd(); ++it) {
        const QString claveReferencia = claveAula(config, it.key());
        int total = 0;                                  // bloques que se quedan en la referencia
        QMap<QString, int> desplazados;                 // bloqueos de las clases que se van
        QSet<int> franjas;
        bool todosCaben = true;
        for (int idx : it.value()) {
            const CursoSolverConfig& c = config.cursos[idx];
            if (bloquesCurso(config, c) > franjasCurso(config, c)) { todosCaben = false; break; }
            for (const PlanMateriaConfig& mc : materiasDelCurso(config, c)) {
                const int bloques = bloquesMateria(config, mc);
                if (!claveReferencia.isEmpty() && claveMateria(config, mc.materiaIDx) == claveReferencia) {
                    total += bloques;
                } else {
                    // Clase desplazada: bloquea 1 franja de la referencia; cuántas pueden
                    // compartir franja lo decide el nº de aulas del tipo que requiere.
                    const QString clave = claveMateria(config, mc.materiaIDx);
                    if (!clave.isEmpty()) desplazados[clave] += bloques;
                }
            }
            for (int s : slotsTurno(config, c.turno)) franjas.insert(s);
        }
        if (!todosCaben) continue;
        const int capacidad = config.dimensiones.num_dias * franjas.size();
        // Cota inferior de las franjas que el grupo deja bloqueadas: las que exige el palomar por
        // tipo (k clases desplazadas que requieren un tipo con A aulas no caben en menos de ⌈k/A⌉
        // franjas). Aquí no hace falta limitarla a las franjas contadas: la capacidad se mide con
        // las franjas de los turnos de TODOS los cursos del grupo, también los desplazados, así
        // que ninguna franja bloqueada cae fuera; el tope frente a la capacidad en franjas-día lo
        // pone el `qMin` de abajo. Puede quedarse corta, pero nunca descuenta de más.
        const int bloqueadas = palomarPorTipo(config, desplazados);
        const int disponibles = capacidad - qMin(bloqueadas, capacidad);
        // Sin clases que se queden, la referencia no compite: el desplazamiento no causa conflicto.
        if (total <= 0) continue;
        if (total > disponibles) {
            if (bloqueadas > 0)
                return QStringLiteral("Causa (2) conflicto de aula: el aula «%1» no da abasto para las clases que se quedan en ella (%2 bloques en %3 franjas; %4 quedan bloqueadas por clases desplazadas).")
                    .arg(nombreAula(config, it.key())).arg(total).arg(capacidad).arg(qMin(bloqueadas, capacidad));
            return QStringLiteral("Causa (2) conflicto de aula: el aula «%1» no da abasto para las clases que se quedan en ella (%2 bloques en %3 franjas).")
                .arg(nombreAula(config, it.key())).arg(total).arg(capacidad);
        }
    }

    // Saturación de un tipo de aula (RF-6): la competición por el mismo tipo no se reasigna.
    return causaSaturacionPorTipo(config);
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
