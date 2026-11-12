/**
 * @file test_solver_aforo.cpp
 * @brief Comprobación de modelo del aforo simultáneo de la cancha (018 T2, RF-1…RF-5).
 *
 * `Solver::model` y `Solver::vars` son privados, así que el modelo se construye aquí con las
 * restricciones reales (turnos, aulas, profesores y bloques) y se resuelve con CP-SAT, igual
 * que `test_solver_bloques.cpp`. Sobre ese modelo se **fuerza** la clase con `AddEquality`:
 *
 *  (a) dos cursos en la cancha en la **misma posición de par** → factible (aforo 2);
 *  (b) tres cursos en la misma franja → infactible (aforo 2);
 *  (c) un bloque forzado en las franjas 3–4 de un segmento que empieza en 0 → infactible (R9:
 *      las franjas ocupadas son un prefijo, así que el par no puede empezar en 3);
 *  (d) control positivo de (c) en 0–1 → factible;
 *  (e) un solo docente candidato para dos cursos: no comparten franja y forzarlos en la misma
 *      franja es infactible (un docente no está en dos clases a la vez).
 *
 * (c) demuestra que el **par cruzado no es una solución** del modelo: la alineación la imponen
 * R2 (emparejamiento), R7 (misma aula del par) y R9 (huecos al final).
 */

#include <gtest/gtest.h>

#include <tuple>

#include <QMap>
#include <QSet>
#include <QString>
#include <QVector>

#include "backend/solver/config/solver_config.hpp"
#include "backend/solver/restricciones/restricciones_aulas.hpp"
#include "backend/solver/restricciones/restricciones_bloques.hpp"
#include "backend/solver/restricciones/restricciones_profesores.hpp"
#include "backend/solver/restricciones/restricciones_turnos.hpp"
#include "backend/solver/variables.hpp"
#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model.pb.h"
#include "ortools/sat/cp_model_solver.h"
#include "ortools/sat/model.h"
#include "ortools/sat/sat_parameters.pb.h"

using namespace operations_research::sat;

namespace {

/// `dias` días de 8 franjas, `cursos` cursos con una materia de Educación Física de 2 h en la
/// cancha y `docentes` docentes candidatos (todos disponibles en todas las franjas).
SolverConfig configCancha(int cursos, int docentes, int dias = 1) {
    SolverConfig config;
    config.version = QStringLiteral("2.0");
    config.aula_fija = false;
    config.dimensiones = {docentes, 1, 1, cursos, dias, 8};
    config.franja_horaria = {40, 8};

    QVector<int> franjasDelDia;
    for (int s = 0; s < 8; ++s) franjasDelDia.append(s);
    config.turnos.turnos = {
        TurnoConfig{QStringLiteral("Mañana"), franjasDelDia, QString(), QString()}};

    config.materias = {{QStringLiteral("Educación Física 1º"), QStringLiteral("Cancha")}};
    config.aulas = {{QStringLiteral("Cancha"), QStringLiteral("Cancha"), 88}};
    config.planes = {{QStringLiteral("31059"), {{0, 1, 2}}}};   // 2 h de EF en 1.º

    for (int c = 0; c < cursos; ++c)
        config.cursos.append(CursoSolverConfig{1, QString(QChar('A' + c)), QStringLiteral("Mañana"),
                                               40, QStringLiteral("31059"), -1});

    QVector<Disponibilidad> disponibles;
    for (int d = 0; d < dias; ++d) disponibles.append(Disponibilidad{d, franjasDelDia});
    for (int p = 0; p < docentes; ++p)
        config.profesores.append(ProfesorSolverConfig{
            QStringLiteral("Prof %1").arg(p), -1, 8 * dias, QStringLiteral("Mañana"),
            QStringLiteral("31059"), {}, {0}, {}, disponibles});
    return config;
}

/// Modelo completo del motor, como `test_solver_bloques.cpp`.
VariablesSolver construirModelo(CpModelBuilder& model, const SolverConfig& config) {
    VariablesSolver vars = crearVariables(model, config);
    agregarRestriccionesTurnos(model, config, vars);
    agregarRestriccionesAulas(model, config, vars);
    agregarRestriccionProfesores(model, config, vars);
    agregarRestriccionesBloques(model, config, vars);
    return vars;
}

CpSolverResponse resolverModelo(CpModelBuilder& model) {
    SatParameters params;
    params.set_num_search_workers(1);
    params.set_random_seed(0);
    params.set_max_deterministic_time(10.0);

    Model sat_model;
    sat_model.Add(NewSatParameters(params));
    return SolveCpModel(model.Build(), &sat_model);
}

bool esFactible(CpSolverStatus status) {
    return status == CpSolverStatus::OPTIMAL || status == CpSolverStatus::FEASIBLE;
}

/// Fuerza la clase (docente, materia 0, curso, día 0, franja) a 1. La variable debe existir:
/// el docente tiene que ser candidato de la materia del curso y estar disponible.
void forzar(CpModelBuilder& model, const VariablesSolver& vars, int docente, int curso, int slot) {
    const auto it = vars.assignment.constFind({docente, 0, curso, 0, slot});
    ASSERT_NE(it, vars.assignment.constEnd()) << "falta la variable x[" << docente << ",0,"
                                              << curso << ",0," << slot << "]";
    model.AddEquality(*it, 1);
}

/// Aula activa (índice) de una franja en la solución, o -1 si no hay ninguna.
int aulaActiva(const CpSolverResponse& respuesta, const VariablesSolver& vars, int curso,
               int day, int slot) {
    for (auto it = vars.aulaAssignment.constBegin(); it != vars.aulaAssignment.constEnd(); ++it) {
        const auto [c, d, s, a] = it.key();
        if (c == curso && d == day && s == slot && SolutionBooleanValue(respuesta, it.value()))
            return a;
    }
    return -1;
}

/// Franjas (día, slot) ocupadas por un curso en la solución.
QSet<QPair<int, int>> franjasDe(const CpSolverResponse& respuesta, const VariablesSolver& vars,
                                int curso) {
    QSet<QPair<int, int>> franjas;
    for (auto it = vars.assignment.constBegin(); it != vars.assignment.constEnd(); ++it) {
        const auto [p, m, c, d, s] = it.key();
        (void)p; (void)m;
        if (c == curso && SolutionBooleanValue(respuesta, it.value()))
            franjas.insert(qMakePair(d, s));
    }
    return franjas;
}

}  // namespace

// ── (a) Dos cursos en la cancha en la misma posición de par → factible ──────────────────────

TEST(TestSolverAforo, dosCursosEnLaMismaPosicionDePar_esFactible) {
    const SolverConfig config = configCancha(2, 2);

    CpModelBuilder modelo;
    const VariablesSolver vars = construirModelo(modelo, config);
    forzar(modelo, vars, 0, 0, 0);
    forzar(modelo, vars, 0, 0, 1);
    forzar(modelo, vars, 1, 1, 0);
    forzar(modelo, vars, 1, 1, 1);

    const CpSolverResponse respuesta = resolverModelo(modelo);
    ASSERT_TRUE(esFactible(respuesta.status()))
        << CpSolverStatus_Name(respuesta.status());

    // Las dos clases están en la cancha (única aula del modelo), las dos franjas del par.
    EXPECT_EQ(aulaActiva(respuesta, vars, 0, 0, 0), 0);
    EXPECT_EQ(aulaActiva(respuesta, vars, 1, 0, 0), 0);

    int enCancha = 0;
    for (int curso = 0; curso < 2; ++curso)
        if (aulaActiva(respuesta, vars, curso, 0, 0) == 0) ++enCancha;
    EXPECT_EQ(enCancha, 2) << "la cancha debe alojar dos cursos a la vez (aforo 2)";
}

// ── (b) Tres cursos en la misma franja → infactible ─────────────────────────────────────────

TEST(TestSolverAforo, tresCursosEnLaMismaFranja_esInfactible) {
    const SolverConfig config = configCancha(3, 3);

    CpModelBuilder modelo;
    const VariablesSolver vars = construirModelo(modelo, config);
    for (int curso = 0; curso < 3; ++curso) {
        forzar(modelo, vars, curso, curso, 0);
        forzar(modelo, vars, curso, curso, 1);
    }

    const CpSolverResponse respuesta = resolverModelo(modelo);
    EXPECT_EQ(respuesta.status(), CpSolverStatus::INFEASIBLE)
        << "el aforo 2 no admite tres cursos en la misma franja";
}

// ── (c) Bloque forzado en 3–4 (segmento que empieza en 0) → infactible (R9) ────────────────

TEST(TestSolverAforo, bloqueForzadoFueraDelPrefijo_esInfactible) {
    const SolverConfig config = configCancha(1, 1);

    CpModelBuilder modelo;
    const VariablesSolver vars = construirModelo(modelo, config);
    forzar(modelo, vars, 0, 0, 3);
    forzar(modelo, vars, 0, 0, 4);

    const CpSolverResponse respuesta = resolverModelo(modelo);
    EXPECT_EQ(respuesta.status(), CpSolverStatus::INFEASIBLE)
        << "R9 exige que las franjas ocupadas sean un prefijo: el par no puede empezar en 3";
}

// ── (d) Control positivo de (c): el mismo bloque en 0–1 → factible ─────────────────────────

TEST(TestSolverAforo, bloqueForzadoEnElPrefijo_esFactible) {
    const SolverConfig config = configCancha(1, 1);

    CpModelBuilder modelo;
    const VariablesSolver vars = construirModelo(modelo, config);
    forzar(modelo, vars, 0, 0, 0);
    forzar(modelo, vars, 0, 0, 1);

    const CpSolverResponse respuesta = resolverModelo(modelo);
    ASSERT_TRUE(esFactible(respuesta.status()))
        << CpSolverStatus_Name(respuesta.status());
    EXPECT_EQ(aulaActiva(respuesta, vars, 0, 0, 0), 0);
    EXPECT_EQ(aulaActiva(respuesta, vars, 0, 0, 1), 0);
}

// ── (e) Un solo docente candidato para dos cursos → no coinciden en la misma franja ────────

TEST(TestSolverAforo, unSoloDocenteParaDosCursos_noCoincidenEnLaMismaFranja) {
    // Dos días: hay dos posiciones de par (una por día) para el mismo docente.
    const SolverConfig config = configCancha(2, 1, 2);

    // Sin forzar: hay solución y las dos clases no comparten (día, franja): un docente no está
    // en dos clases a la vez. Con el aforo 2 la cancha admitiría dos cursos, no este par.
    CpModelBuilder modelo;
    const VariablesSolver vars = construirModelo(modelo, config);
    const CpSolverResponse respuesta = resolverModelo(modelo);
    ASSERT_TRUE(esFactible(respuesta.status()))
        << CpSolverStatus_Name(respuesta.status());

    const QSet<QPair<int, int>> franjasA = franjasDe(respuesta, vars, 0);
    const QSet<QPair<int, int>> franjasB = franjasDe(respuesta, vars, 1);
    EXPECT_FALSE(franjasA.isEmpty());
    EXPECT_FALSE(franjasB.isEmpty());
    for (const QPair<int, int>& franja : franjasA)
        EXPECT_FALSE(franjasB.contains(franja))
            << "el único docente no puede dar dos clases el día " << franja.first
            << " en la franja " << franja.second;

    // Forzarlas en la misma posición de par (día 0, franjas 0–1) no tiene solución.
    CpModelBuilder forzado;
    const VariablesSolver varsForzado = construirModelo(forzado, config);
    forzar(forzado, varsForzado, 0, 0, 0);
    forzar(forzado, varsForzado, 0, 1, 0);
    const CpSolverResponse coincidentes = resolverModelo(forzado);
    EXPECT_EQ(coincidentes.status(), CpSolverStatus::INFEASIBLE)
        << "dos clases del mismo docente en la misma franja no son una solución";
}
