/**
 * @file test_solver_bloques.cpp
 * @brief Tests de modelo del emparejamiento en bloques de dos horas (014 T3a).
 *
 * Construye el modelo CP-SAT con las restricciones reales (D-rules + R1/R2/R3 + R7) y comprueba:
 *  (a) un segmento lleno produce sus pares (y un tramo de 4 admite dos pares adyacentes);
 *  (b) un tramo impar no tiene solución;
 *  (c) un par usa una sola aula, un par compartido también y dos pares del mismo tramo
 *      pueden usar aulas distintas;
 *  (d) el tamaño del modelo representativo (24 cursos) se imprime y queda en ~30–40k.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <iostream>
#include <tuple>
#include <vector>

#include <QMap>
#include <QSet>
#include <QString>
#include <QVector>

#include "backend/services/analisis_bloques.hpp"
#include "backend/solver/config/bloques.hpp"
#include "backend/solver/config/solver_config.hpp"
#include "backend/solver/restricciones/restricciones_aulas.hpp"
#include "backend/solver/restricciones/restricciones_bloques.hpp"
#include "backend/solver/restricciones/restricciones_profesores.hpp"
#include "backend/solver/restricciones/restricciones_turnos.hpp"
#include "backend/solver/solver.hpp"
#include "backend/solver/variables.hpp"
#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model.pb.h"
#include "ortools/sat/cp_model_solver.h"
#include "ortools/sat/model.h"
#include "ortools/sat/sat_parameters.pb.h"

using namespace operations_research::sat;

namespace {

/// Construye el modelo completo (D-rules + bloques) y devuelve sus variables.
VariablesSolver construirModelo(CpModelBuilder& model, const SolverConfig& config) {
    VariablesSolver vars = crearVariables(model, config);
    agregarRestriccionesTurnos(model, config, vars);
    agregarRestriccionesAulas(model, config, vars);
    agregarRestriccionProfesores(model, config, vars);
    agregarRestriccionesBloques(model, config, vars);
    return vars;
}

/// Resuelve el modelo con parámetros deterministas (RNF-1), igual que el motor.
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

/// Un curso, una materia de `horas` h y un turno de `franjas` slots (un solo segmento).
SolverConfig configUnaMateria(int horas, int franjas, int numAulas = 1) {
    SolverConfig config;
    config.version = QStringLiteral("2.0");
    config.aula_fija = true;
    config.dimensiones = {1, 1, numAulas, 1, 1, franjas};
    config.franja_horaria = {40, franjas};

    QVector<int> slotsTurno;
    for (int s = 0; s < franjas; ++s) slotsTurno.append(s);
    config.turnos.turnos = {
        TurnoConfig{QStringLiteral("manana"), slotsTurno, QString(), QString()}};

    config.materias = {{QStringLiteral("Matemática"), QStringLiteral("regular")}};
    for (int a = 0; a < numAulas; ++a)
        config.aulas.append(AulaSolverConfig{QStringLiteral("Aula %1").arg(a),
                                             QStringLiteral("regular"), 30});
    config.planes = {{QStringLiteral("Plan"), {{0, 1, horas}}}};
    config.cursos = {CursoSolverConfig{1, QStringLiteral("A"), QStringLiteral("manana"), 20,
                                       QStringLiteral("Plan"), 0}};
    config.profesores = {ProfesorSolverConfig{
        QStringLiteral("Prof A"), -1, horas, QStringLiteral("manana"), QStringLiteral("Plan"),
        {}, {0}, {}, {{0, slotsTurno}}}};
    return config;
}

/// Dos materias de 1 h que comparten el único par del día (2 franjas).
SolverConfig configDosMateriasUnPar(int numAulas = 2) {
    SolverConfig config;
    config.version = QStringLiteral("2.0");
    config.aula_fija = false;   // móvil: la única restricción de aula es D9 + R7
    config.dimensiones = {1, 2, numAulas, 1, 1, 2};
    config.franja_horaria = {40, 2};
    config.turnos.turnos = {
        TurnoConfig{QStringLiteral("manana"), {0, 1}, QString(), QString()}};

    config.materias = {{QStringLiteral("Matemática"), QStringLiteral("regular")},
                       {QStringLiteral("Historia"), QStringLiteral("regular")}};
    for (int a = 0; a < numAulas; ++a)
        config.aulas.append(AulaSolverConfig{QStringLiteral("Aula %1").arg(a),
                                             QStringLiteral("regular"), 30});
    config.planes = {{QStringLiteral("Plan"), {{0, 1, 1}, {1, 1, 1}}}};
    config.cursos = {CursoSolverConfig{1, QStringLiteral("A"), QStringLiteral("manana"), 20,
                                       QStringLiteral("Plan"), 0}};
    config.profesores = {ProfesorSolverConfig{
        QStringLiteral("Prof A"), -1, 2, QStringLiteral("manana"), QStringLiteral("Plan"),
        {}, {0, 1}, {}, {{0, {0, 1}}}}};
    return config;
}

/// Dos materias de 2 h con tipos de aula distintos en un tramo de 4: cada par debe usar
/// una sola aula y los dos pares NO pueden compartir aula (tipos distintos).
SolverConfig configDosParesDistintaAula() {
    SolverConfig config;
    config.version = QStringLiteral("2.0");
    config.aula_fija = false;
    config.dimensiones = {1, 2, 2, 1, 1, 4};
    config.franja_horaria = {40, 4};
    config.turnos.turnos = {
        TurnoConfig{QStringLiteral("manana"), {0, 1, 2, 3}, QString(), QString()}};

    config.materias = {{QStringLiteral("Matemática"), QStringLiteral("regular")},
                       {QStringLiteral("Taller"), QStringLiteral("Laboratorio")}};
    config.aulas = {{QStringLiteral("A1"), QStringLiteral("regular"), 30},
                    {QStringLiteral("Lab"), QStringLiteral("Laboratorio"), 20}};
    config.planes = {{QStringLiteral("Plan"), {{0, 1, 2}, {1, 1, 2}}}};
    config.cursos = {CursoSolverConfig{1, QStringLiteral("A"), QStringLiteral("manana"), 20,
                                       QStringLiteral("Plan"), 0}};
    config.profesores = {ProfesorSolverConfig{
        QStringLiteral("Prof A"), -1, 4, QStringLiteral("manana"), QStringLiteral("Plan"),
        {}, {0, 1}, {}, {{0, {0, 1, 2, 3}}}}};
    return config;
}

/// Preferencia ya proyectada para la configuración del solver (día de la generación).
PreferenciaSolverConfig preferencia(int docente, int dia, const QVector<int>& franjas,
                                     const QString& sentido) {
    PreferenciaSolverConfig p;
    p.docente = docente;
    p.dia = dia;
    p.slot = franjas;
    p.sentido = sentido;
    return p;
}

/// Aula activa (índice) de una franja en la solución, o -1 si no hay ninguna.
int aulaActiva(const CpSolverResponse& respuesta, const VariablesSolver& vars, int c, int d,
               int s) {
    for (auto it = vars.aulaAssignment.constBegin(); it != vars.aulaAssignment.constEnd(); ++it) {
        const auto [ac, ad, as, aa] = it.key();
        if (ac == c && ad == d && as == s && SolutionBooleanValue(respuesta, it.value()))
            return aa;
    }
    return -1;
}

/// Curso 1|A con un plan `{nombre, horas}`, `dias` días de `franjas` slots y una sola aula
/// (modo móvil, un único segmento por día).
SolverConfig configAlineacion(const QVector<QPair<QString, int>>& materias, int dias,
                              int franjas) {
    SolverConfig config;
    config.version = QStringLiteral("2.0");
    config.aula_fija = false;
    config.dimensiones = {1, static_cast<int>(materias.size()), 1, 1, dias, franjas};
    config.franja_horaria = {40, franjas};

    QVector<int> slotsTurno;
    for (int s = 0; s < franjas; ++s) slotsTurno.append(s);
    config.turnos.turnos = {
        TurnoConfig{QStringLiteral("manana"), slotsTurno, QString(), QString()}};

    QVector<PlanMateriaConfig> plan;
    int horasTotales = 0;
    for (int m = 0; m < materias.size(); ++m) {
        config.materias.append(
            MateriaSolverConfig{materias[m].first, QStringLiteral("regular")});
        plan.append(PlanMateriaConfig{m, 1, materias[m].second});
        horasTotales += materias[m].second;
    }
    config.aulas = {{QStringLiteral("A1"), QStringLiteral("regular"), 30}};
    config.planes = {PlanSolverConfig{QStringLiteral("Plan"), plan}};
    config.cursos = {CursoSolverConfig{1, QStringLiteral("A"), QStringLiteral("manana"), 20,
                                       QStringLiteral("Plan"), 0}};

    QVector<int> todasLasMaterias;
    for (int m = 0; m < materias.size(); ++m) todasLasMaterias.append(m);
    QVector<Disponibilidad> disp;
    for (int d = 0; d < dias; ++d) disp.append(Disponibilidad{d, slotsTurno});
    config.profesores = {ProfesorSolverConfig{
        QStringLiteral("Prof A"), -1, horasTotales, QStringLiteral("manana"),
        QStringLiteral("Plan"), {}, todasLasMaterias, {}, disp}};
    return config;
}

/// Franjas ocupadas de un curso en la solución.
QSet<FranjaSemana> franjasOcupadas(const HorarioSalida& horario, const QString& curso) {
    QSet<FranjaSemana> ocupadas;
    for (const DiaOutput& dia : horario.horarios.value(curso).dias)
        for (const AsignacionOutput& asignacion : dia.asignaciones)
            ocupadas.insert(FranjaSemana{dia.dia, asignacion.slot});
    return ocupadas;
}

/// Materia que ocupa (curso, día, slot) en la solución, o -1 si está libre.
int materiaEn(const HorarioSalida& horario, const QString& curso, int dia, int slot) {
    for (const DiaOutput& d : horario.horarios.value(curso).dias)
        if (d.dia == dia)
            for (const AsignacionOutput& asignacion : d.asignaciones)
                if (asignacion.slot == slot) return asignacion.materia;
    return -1;
}

/// Nº de clases de una materia en la solución.
int clasesDe(const HorarioSalida& horario, const QString& curso, int materia) {
    int total = 0;
    for (const DiaOutput& dia : horario.horarios.value(curso).dias)
        for (const AsignacionOutput& asignacion : dia.asignaciones)
            if (asignacion.materia == materia) ++total;
    return total;
}

/// Franjas ocupadas por cada materia del curso en la solución.
QMap<int, QSet<FranjaSemana>> franjasPorMateria(const HorarioSalida& horario,
                                                const QString& curso) {
    QMap<int, QSet<FranjaSemana>> porMateria;
    for (const DiaOutput& dia : horario.horarios.value(curso).dias)
        for (const AsignacionOutput& asignacion : dia.asignaciones)
            porMateria[asignacion.materia].insert(FranjaSemana{dia.dia, asignacion.slot});
    return porMateria;
}

/// Separación recalculada con las funciones puras de `bloques` (la misma que informará T6).
int separacionDe(const SolverConfig& config, const HorarioSalida& horario,
                 const QString& curso) {
    const QMap<int, QSet<FranjaSemana>> porMateria = franjasPorMateria(horario, curso);
    int total = 0;
    for (const PlanMateriaConfig& mc : materiasDelCurso(config, config.cursos[0])) {
        if (!porMateria.contains(mc.materiaIDx)) continue;
        total += rejilla::incumplimientosSeparacion(
            rejilla::diasDeMateria(config, 0, mc.materiaIDx, porMateria.value(mc.materiaIDx)));
    }
    return total;
}

/// Recuento independiente de pares de días consecutivos (para contrastar el recálculo).
int separacionManual(const HorarioSalida& horario, const QString& curso) {
    QMap<int, QSet<int>> diasPorMateria;
    for (const DiaOutput& dia : horario.horarios.value(curso).dias)
        for (const AsignacionOutput& asignacion : dia.asignaciones)
            diasPorMateria[asignacion.materia].insert(dia.dia);

    int total = 0;
    for (auto it = diasPorMateria.constBegin(); it != diasPorMateria.constEnd(); ++it) {
        QVector<int> dias(it.value().begin(), it.value().end());
        std::sort(dias.begin(), dias.end());
        for (int i = 1; i < dias.size(); ++i)
            if (dias[i] - dias[i - 1] == 1) ++total;
    }
    return total;
}

/// Modelo representativo del fixture: 24 cursos, 9 materias, 12 aulas, 30 docentes.
SolverConfig configRepresentativa() {
    SolverConfig config;
    config.version = QStringLiteral("2.0");
    config.aula_fija = true;
    config.dimensiones = {30, 9, 12, 24, 5, 8};
    config.franja_horaria = {40, 8};

    QVector<int> slotsTurno;
    for (int s = 0; s < 8; ++s) slotsTurno.append(s);
    config.turnos.turnos = {
        TurnoConfig{QStringLiteral("manana"), slotsTurno, QString(), QString()}};
    config.eventos = {EventoConfig{QStringLiteral("Receso"), QStringLiteral("manana"), 2, 1, 10,
                                   QString(), QString()}};

    QVector<PlanMateriaConfig> plan;
    for (int m = 0; m < 9; ++m) {
        config.materias.append(
            MateriaSolverConfig{QStringLiteral("Materia %1").arg(m), QStringLiteral("regular")});
        plan.append(PlanMateriaConfig{m, 1, 4});
    }
    for (int a = 0; a < 12; ++a)
        config.aulas.append(
            AulaSolverConfig{QStringLiteral("Aula %1").arg(a), QStringLiteral("regular"), 30});
    config.planes = {PlanSolverConfig{QStringLiteral("Plan"), plan}};

    for (int c = 0; c < 24; ++c)
        config.cursos.append(CursoSolverConfig{1, QString(QChar('A' + c)),
                                               QStringLiteral("manana"), 20,
                                               QStringLiteral("Plan"), c % 12});

    QVector<Disponibilidad> disp;
    for (int d = 0; d < 5; ++d) disp.append(Disponibilidad{d, slotsTurno});
    for (int p = 0; p < 30; ++p) {
        QVector<int> materiasAsignadas;
        if (p < 9) materiasAsignadas.append(p);   // un titular por materia del catálogo
        config.profesores.append(ProfesorSolverConfig{
            QStringLiteral("Prof %1").arg(p), -1, 40, QString(), QString(), {},
            materiasAsignadas, {}, disp});
    }

    return config;
}

} // namespace

// ─── (a) Segmento lleno → pares ─────────────────────────────────────────────

TEST(TestSolverBloques, segmentoLlenoProduceSusPares) {
    // Un segmento de 4 lleno por dos materias de 2 h: cada una ocupa un par entero y el
    // segmento se tapa con dos pares adyacentes (par[0] y par[2]).
    const SolverConfig config =
        configAlineacion({{QStringLiteral("Matemática"), 2}, {QStringLiteral("Historia"), 2}},
                         /*dias=*/1, /*franjas=*/4);
    CpModelBuilder model;
    const VariablesSolver vars = construirModelo(model, config);

    const CpSolverResponse respuesta = resolverModelo(model);
    ASSERT_TRUE(esFactible(respuesta.status()))
        << "estado: " << CpSolverStatus_Name(respuesta.status());

    EXPECT_TRUE(SolutionBooleanValue(respuesta, vars.par.value({0, 0, 0})));
    EXPECT_FALSE(SolutionBooleanValue(respuesta, vars.par.value({0, 0, 1})));
    EXPECT_TRUE(SolutionBooleanValue(respuesta, vars.par.value({0, 0, 2})));
    EXPECT_FALSE(vars.par.contains({0, 0, 3}));

    for (int s = 0; s < 4; ++s)
        EXPECT_TRUE(SolutionBooleanValue(respuesta, vars.ocupada.value({0, 0, s})));

    // Cada materia ocupa exactamente sus dos franjas (un bloque simple).
    for (int m = 0; m < 2; ++m) {
        int franjas = 0;
        for (int s = 0; s < 4; ++s)
            if (SolutionBooleanValue(respuesta,
                                     vars.materiaEnFranja.value({m, 0, 0, s})))
                ++franjas;
        EXPECT_EQ(franjas, 2) << "materia " << m;
    }
}

// ─── (b) Tramo impar → infactible ───────────────────────────────────────────

TEST(TestSolverBloques, tramoImparNoTieneSolucion) {
    // 5 h en un segmento de 5 franjas: la paridad dura no permite cubrirlo con pares.
    const SolverConfig config = configUnaMateria(/*horas=*/5, /*franjas=*/5);
    CpModelBuilder model;
    const VariablesSolver vars = construirModelo(model, config);

    // El modelo sí se construye (con `par` en 0..3); lo que no existe es solución.
    EXPECT_EQ(vars.par.size(), 4);
    const CpSolverResponse respuesta = resolverModelo(model);
    EXPECT_EQ(respuesta.status(), CpSolverStatus::INFEASIBLE);
}

// ─── (a) y (b) R7: un par usa una sola aula ─────────────────────────────────

TEST(TestSolverBloques, parSimpleUsaUnaSolaAula) {
    // Una materia de 2 h en un día de 2 franjas: el par debe usar la misma aula en las dos.
    SolverConfig config = configUnaMateria(/*horas=*/2, /*franjas=*/2, /*numAulas=*/2);
    config.aula_fija = false;   // móvil: sin referencia, la única atadura es R7

    CpModelBuilder model;
    const VariablesSolver vars = construirModelo(model, config);
    const CpSolverResponse respuesta = resolverModelo(model);
    ASSERT_TRUE(esFactible(respuesta.status()))
        << "estado: " << CpSolverStatus_Name(respuesta.status());

    const int aula0 = aulaActiva(respuesta, vars, 0, 0, 0);
    const int aula1 = aulaActiva(respuesta, vars, 0, 0, 1);
    EXPECT_NE(aula0, -1);
    EXPECT_EQ(aula0, aula1) << "las dos franjas del par deben compartir aula";
    EXPECT_TRUE(aula0 == 0 || aula0 == 1);
}

TEST(TestSolverBloques, parDeDosMateriasUsaUnaSolaAula) {
    // Dos materias de 1 h comparten el único par: las dos franjas, una sola aula.
    const SolverConfig config = configDosMateriasUnPar(/*numAulas=*/2);
    CpModelBuilder model;
    const VariablesSolver vars = construirModelo(model, config);
    const CpSolverResponse respuesta = resolverModelo(model);
    ASSERT_TRUE(esFactible(respuesta.status()))
        << "estado: " << CpSolverStatus_Name(respuesta.status());

    const int aula0 = aulaActiva(respuesta, vars, 0, 0, 0);
    const int aula1 = aulaActiva(respuesta, vars, 0, 0, 1);
    EXPECT_NE(aula0, -1);
    EXPECT_EQ(aula0, aula1) << "el bloque compartido debe usar la misma aula";
}

// ─── (c) R7 no fuerza la misma aula entre pares distintos ────────────────────

TEST(TestSolverBloques, dosParesDelMismoTramoPuedenUsarAulasDistintas) {
    // Dos materias de 2 h con tipos de aula distintos en un tramo de 4. Cada materia debe
    // ocupar un par entero (si se mezclan los tipos en un par, R7 + tipo exacto no tienen
    // solución) y los dos pares usan aulas distintas. Si R7 forzara la igualdad entre TODAS
    // las franjas del tramo, este escenario sería infactible.
    const SolverConfig config = configDosParesDistintaAula();
    CpModelBuilder model;
    const VariablesSolver vars = construirModelo(model, config);
    const CpSolverResponse respuesta = resolverModelo(model);
    ASSERT_TRUE(esFactible(respuesta.status()))
        << "estado: " << CpSolverStatus_Name(respuesta.status());

    QVector<int> iniciosDePar;
    for (auto it = vars.par.constBegin(); it != vars.par.constEnd(); ++it) {
        const auto [c, d, s] = it.key();
        if (SolutionBooleanValue(respuesta, it.value())) iniciosDePar.append(s);
    }
    ASSERT_EQ(iniciosDePar.size(), 2);

    QSet<int> aulasDeLosPares;
    for (int inicio : iniciosDePar) {
        const int aula0 = aulaActiva(respuesta, vars, 0, 0, inicio);
        const int aula1 = aulaActiva(respuesta, vars, 0, 0, inicio + 1);
        EXPECT_EQ(aula0, aula1) << "cada par debe usar una sola aula";
        aulasDeLosPares.insert(aula0);
    }
    EXPECT_EQ(aulasDeLosPares, (QSet<int>{0, 1}))
        << "los dos pares deben poder usar aulas distintas";
}

// ─── R4/R5/R6: alineación de materias ───────────────────────────────────────

TEST(TestSolverBloques, tresMasTresCompartenBloque) {
    // Dos materias de 3 h: cada una tiene un bloque simple y su resto; los dos restos forman
    // el único bloque compartido. R6 obliga a que simple y resto caigan en días distintos.
    const SolverConfig config =
        configAlineacion({{QStringLiteral("Matemática"), 3}, {QStringLiteral("Historia"), 3}},
                         /*dias=*/3, /*franjas=*/4);
    Solver solver;
    const auto resultado = solver.resolver(config);
    ASSERT_TRUE(resultado.exito) << resultado.errores.join("\n").toStdString();

    const QString curso = QStringLiteral("1|A");
    const QVector<rejilla::Par> pares =
        rejilla::paresDeCurso(config, 0, franjasOcupadas(resultado.resultado, curso));
    ASSERT_EQ(pares.size(), 3);

    int simples = 0, compartidos = 0;
    for (const rejilla::Par& par : pares) {
        const int a = materiaEn(resultado.resultado, curso, par.dia, par.inicio);
        const int b = materiaEn(resultado.resultado, curso, par.dia, par.inicio + 1);
        ASSERT_NE(a, -1);
        ASSERT_NE(b, -1);
        if (a == b) {
            ++simples;
        } else {
            ++compartidos;
            EXPECT_TRUE((a == 0 && b == 1) || (a == 1 && b == 0))
                << "el bloque compartido debe tener una franja de cada materia";
        }
    }
    EXPECT_EQ(simples, 2);
    EXPECT_EQ(compartidos, 1);
    EXPECT_EQ(clasesDe(resultado.resultado, curso, 0), 3);
    EXPECT_EQ(clasesDe(resultado.resultado, curso, 1), 3);
}

TEST(TestSolverBloques, cuatroHorasNoComparte) {
    // Una materia de 4 h sin resto: dos bloques simples en días distintos, ningún compartido.
    const SolverConfig config =
        configAlineacion({{QStringLiteral("Matemática"), 4}}, /*dias=*/3, /*franjas=*/2);
    Solver solver;
    const auto resultado = solver.resolver(config);
    ASSERT_TRUE(resultado.exito) << resultado.errores.join("\n").toStdString();

    const QString curso = QStringLiteral("1|A");
    const QVector<rejilla::Par> pares =
        rejilla::paresDeCurso(config, 0, franjasOcupadas(resultado.resultado, curso));
    ASSERT_EQ(pares.size(), 2);
    for (const rejilla::Par& par : pares) {
        const int a = materiaEn(resultado.resultado, curso, par.dia, par.inicio);
        const int b = materiaEn(resultado.resultado, curso, par.dia, par.inicio + 1);
        EXPECT_EQ(a, 0);
        EXPECT_EQ(b, 0);   // nunca comparte: no tiene resto
    }
    EXPECT_NE(pares[0].dia, pares[1].dia);
    EXPECT_EQ(clasesDe(resultado.resultado, curso, 0), 4);
}

TEST(TestSolverBloques, cincoHorasFormaDosMasDosMasUno) {
    // 5 h (2+2+1) más 1 h: el resto de la materia de 5 h comparte bloque con la de 1 h.
    const SolverConfig config =
        configAlineacion({{QStringLiteral("Matemática"), 5}, {QStringLiteral("Historia"), 1}},
                         /*dias=*/5, /*franjas=*/2);
    Solver solver;
    const auto resultado = solver.resolver(config);
    ASSERT_TRUE(resultado.exito) << resultado.errores.join("\n").toStdString();

    const QString curso = QStringLiteral("1|A");
    const QVector<rejilla::Par> pares =
        rejilla::paresDeCurso(config, 0, franjasOcupadas(resultado.resultado, curso));
    ASSERT_EQ(pares.size(), 3);

    int simplesDeCinco = 0, compartidos = 0;
    for (const rejilla::Par& par : pares) {
        const int a = materiaEn(resultado.resultado, curso, par.dia, par.inicio);
        const int b = materiaEn(resultado.resultado, curso, par.dia, par.inicio + 1);
        if (a == b) {
            if (a == 0) ++simplesDeCinco;
        } else {
            ++compartidos;
            EXPECT_TRUE((a == 0 && b == 1) || (a == 1 && b == 0))
                << "el resto de 5 h comparte con el de 1 h";
        }
    }
    EXPECT_EQ(simplesDeCinco, 2);
    EXPECT_EQ(compartidos, 1);
    EXPECT_EQ(clasesDe(resultado.resultado, curso, 0), 5);
    EXPECT_EQ(clasesDe(resultado.resultado, curso, 1), 1);
}

TEST(TestSolverBloques, seisHorasFormaDosMasDosMasDosEnTresDias) {
    // 6 h: tres bloques simples, uno por día (R6).
    const SolverConfig config =
        configAlineacion({{QStringLiteral("Matemática"), 6}}, /*dias=*/5, /*franjas=*/2);
    Solver solver;
    const auto resultado = solver.resolver(config);
    ASSERT_TRUE(resultado.exito) << resultado.errores.join("\n").toStdString();

    const QString curso = QStringLiteral("1|A");
    const QVector<rejilla::Par> pares =
        rejilla::paresDeCurso(config, 0, franjasOcupadas(resultado.resultado, curso));
    ASSERT_EQ(pares.size(), 3);

    QSet<int> dias;
    for (const rejilla::Par& par : pares) {
        EXPECT_EQ(materiaEn(resultado.resultado, curso, par.dia, par.inicio), 0);
        EXPECT_EQ(materiaEn(resultado.resultado, curso, par.dia, par.inicio + 1), 0);
        dias.insert(par.dia);
    }
    EXPECT_EQ(dias.size(), 3);
    EXPECT_EQ(clasesDe(resultado.resultado, curso, 0), 6);
}

// ─── Causa (5) en el motor ──────────────────────────────────────────────────

TEST(TestSolverBloques, causaCincoPorTotalImparDeCurso) {
    const SolverConfig config =
        configAlineacion({{QStringLiteral("Matemática"), 11}}, /*dias=*/5, /*franjas=*/6);
    Solver solver;
    const auto resultado = solver.resolver(config);

    ASSERT_FALSE(resultado.exito);
    const QString texto = resultado.errores.join("\n");
    EXPECT_TRUE(texto.contains(QStringLiteral("Causa (5)"))) << texto.toStdString();
    EXPECT_TRUE(texto.contains(QStringLiteral("11 horas"))) << texto.toStdString();
    EXPECT_TRUE(texto.contains(QStringLiteral("impar"))) << texto.toStdString();
}

TEST(TestSolverBloques, causaCincoPorMasAparicionesQueDias) {
    // 11 + 1 = 12 (par): la causa (5) es la segunda condición, no la del total.
    const SolverConfig config =
        configAlineacion({{QStringLiteral("Matemática"), 11}, {QStringLiteral("Historia"), 1}},
                         /*dias=*/5, /*franjas=*/6);
    Solver solver;
    const auto resultado = solver.resolver(config);

    ASSERT_FALSE(resultado.exito);
    const QString texto = resultado.errores.join("\n");
    EXPECT_TRUE(texto.contains(QStringLiteral("Causa (5)"))) << texto.toStdString();
    EXPECT_TRUE(texto.contains(QStringLiteral("Matemática"))) << texto.toStdString();
    EXPECT_TRUE(texto.contains(QStringLiteral("6 apariciones"))) << texto.toStdString();
    EXPECT_TRUE(texto.contains(QStringLiteral("5 días"))) << texto.toStdString();
}

TEST(TestSolverBloques, sieteHorasSeInformaComoIncumplimientoDeSeparacion) {
    // 7–10 h no es causa (5): se resuelve e informa el incumplimiento de separación.
    const SolverConfig config =
        configAlineacion({{QStringLiteral("Matemática"), 7}, {QStringLiteral("Historia"), 1}},
                         /*dias=*/5, /*franjas=*/6);
    Solver solver;
    const auto resultado = solver.resolver(config);
    ASSERT_TRUE(resultado.exito) << resultado.errores.join("\n").toStdString();

    const AnalisisBloques analisis = analizarBloques(resultado.resultado, config);
    EXPECT_GT(analisis.incumplimientosSeparacion, 0);
}


TEST(TestSolverBloques, falloDeAulaNoSeAtribuyeACausaCinco) {
    // Dos cursos de 2 h de laboratorio con un solo laboratorio: la causa es (2), nunca (5).
    SolverConfig config;
    config.version = QStringLiteral("2.0");
    config.aula_fija = true;
    config.dimensiones = {2, 1, 1, 2, 1, 2};
    config.franja_horaria = {40, 2};
    config.turnos.turnos = {
        TurnoConfig{QStringLiteral("manana"), {0, 1}, QString(), QString()}};
    config.materias = {{QStringLiteral("Laboratorio"), QStringLiteral("Laboratorio")}};
    config.aulas = {{QStringLiteral("L1"), QStringLiteral("Laboratorio"), 20}};
    config.planes = {{QStringLiteral("Plan"), {{0, 1, 2}}}};
    config.cursos = {CursoSolverConfig{1, QStringLiteral("A"), QStringLiteral("manana"), 20,
                                       QStringLiteral("Plan"), 0},
                     CursoSolverConfig{1, QStringLiteral("B"), QStringLiteral("manana"), 20,
                                       QStringLiteral("Plan"), 0}};
    config.profesores = {
        ProfesorSolverConfig{QStringLiteral("Prof A"), -1, 2, QStringLiteral("manana"),
                             QStringLiteral("Plan"), {}, {0}, {}, {{0, {0, 1}}}},
        ProfesorSolverConfig{QStringLiteral("Prof B"), -1, 2, QStringLiteral("manana"),
                             QStringLiteral("Plan"), {}, {0}, {}, {{0, {0, 1}}}}};

    Solver solver;
    const auto resultado = solver.resolver(config);
    ASSERT_FALSE(resultado.exito);
    const QString texto = resultado.errores.join("\n");
    EXPECT_TRUE(texto.contains(QStringLiteral("Causa (2)"))) << texto.toStdString();
    EXPECT_FALSE(texto.contains(QStringLiteral("Causa (5)"))) << texto.toStdString();
}

TEST(TestSolverBloques, falloDeAulaConcretaEsNoAtribuibleNuncaCausaCinco) {
    // Dos cursos con referencia compartida y laboratorio en turnos de 3 franjas: el fallo es
    // de disposición de aulas, no de bloques (totales pares y sin condición de causa (5)).
    SolverConfig config;
    config.version = QStringLiteral("2.0");
    config.aula_fija = true;
    config.dimensiones = {2, 2, 3, 2, 2, 3};
    config.franja_horaria = {60, 3};
    config.turnos.turnos = {{"manana", {0, 1, 2}}};
    config.materias = {{"Matematica", "Aula común"}, {"Laboratorio", "Laboratorio"}};
    config.aulas = {{"A1", "Aula común", 30},
                    {"Lab 1", "Laboratorio", 20},
                    {"Lab 2", "Laboratorio", 20}};
    config.planes = {{"Plan A", {{0, 1, 2}, {1, 1, 2}}},
                     {"Plan B", {{0, 2, 2}, {1, 2, 2}}}};
    config.cursos = {{1, "A", "manana", 20, "Plan A", 0},
                     {2, "B", "manana", 20, "Plan B", 0}};
    config.profesores = {
        {"Prof A", 4, 4, "manana", "Plan A", {}, {0, 1}, {}, {{0, {0, 1, 2}}, {1, {0, 1, 2}}}},
        {"Prof B", 4, 4, "manana", "Plan B", {}, {0, 1}, {}, {{0, {0, 1, 2}}, {1, {0, 1, 2}}}}};

    Solver solver;
    const auto resultado = solver.resolver(config);
    ASSERT_FALSE(resultado.exito);
    const QString texto = resultado.errores.join("\n");
    EXPECT_TRUE(texto.contains(QStringLiteral("no atribuible"))) << texto.toStdString();
    EXPECT_FALSE(texto.contains(QStringLiteral("Causa (5)"))) << texto.toStdString();
}

// ─── Separación blanda (T5a) ────────────────────────────────────────────────

TEST(TestSolverBloques, seisHorasQuedaEnLunesMiercolesViernes) {
    // 6 h = 3 bloques simples (uno por día). La separación blanda elige la única terna de
    // días con diferencia ≥ 2 en una semana de cinco: lunes, miércoles y viernes.
    const SolverConfig config =
        configAlineacion({{QStringLiteral("Matemática"), 6}}, /*dias=*/5, /*franjas=*/2);
    Solver solver;
    const auto resultado = solver.resolver(config);
    ASSERT_TRUE(resultado.exito) << resultado.errores.join("\n").toStdString();

    const QString curso = QStringLiteral("1|A");
    const QVector<int> dias = rejilla::diasDeMateria(
        config, 0, 0, franjasPorMateria(resultado.resultado, curso).value(0));
    EXPECT_EQ(dias, (QVector<int>{0, 2, 4}));
    EXPECT_EQ(rejilla::incumplimientosSeparacion(dias), 0);
}

TEST(TestSolverBloques, separacionInformadaIgualAlRecalculoYNoPeorQueLaReferencia) {
    const SolverConfig config =
        configAlineacion({{QStringLiteral("Matemática"), 6}}, /*dias=*/5, /*franjas=*/2);
    Solver solver;

    // Referencia: sin fase 2a (020, D4: la escalera se prueba con los flags, sin reloj).
    Solver::PresupuestoFases sinSeparacion;
    sinSeparacion.ejecutarSeparacionHuecos = false;
    const auto referencia = solver.resolver(config, sinSeparacion);
    const auto optimo = solver.resolver(config);

    ASSERT_TRUE(referencia.exito) << referencia.errores.join("\n").toStdString();
    ASSERT_TRUE(optimo.exito) << optimo.errores.join("\n").toStdString();

    const QString curso = QStringLiteral("1|A");
    // El valor recalculado con las funciones puras coincide con el recuento independiente.
    EXPECT_EQ(separacionDe(config, optimo.resultado, curso),
              separacionManual(optimo.resultado, curso));
    // La separación es blanda: la 2a no empeora la referencia.
    EXPECT_LE(separacionDe(config, optimo.resultado, curso),
              separacionDe(config, referencia.resultado, curso));
}

TEST(TestSolverBloques, escalera2aDevuelveLaReferenciaSinError) {
    const SolverConfig config =
        configAlineacion({{QStringLiteral("Matemática"), 6}}, /*dias=*/5, /*franjas=*/2);
    Solver solver;

    // Sin presupuesto para la fase 2a: no se ejecuta y se devuelve la referencia de la fase 1.
    Solver::PresupuestoFases sin2a;
    sin2a.ejecutarSeparacionHuecos = false;
    const auto conPresupuesto = solver.resolver(config, sin2a);

    ASSERT_TRUE(conPresupuesto.exito) << conPresupuesto.errores.join("\n").toStdString();
    // La escalera devuelve la referencia (puede tener incumplimientos de separación).
    const QString curso = QStringLiteral("1|A");
    EXPECT_EQ(clasesDe(conPresupuesto.resultado, curso, 0), 6);
}

// ─── Huecos blandos y fase 2b (T5b) ─────────────────────────────────────────

TEST(TestSolverBloques, huecosAlFinalYPreferentementeElViernes) {
    // 4 h = dos bloques simples. La 2a elige días separados, con el par al inicio de cada día
    // (huecos al final) y deja el viernes libre (sus 4 huecos no cuestan).
    const SolverConfig config =
        configAlineacion({{QStringLiteral("Matemática"), 4}}, /*dias=*/5, /*franjas=*/4);
    Solver solver;
    const auto resultado = solver.resolver(config);
    ASSERT_TRUE(resultado.exito) << resultado.errores.join("\n").toStdString();

    const QString curso = QStringLiteral("1|A");
    QSet<int> diasConClase;
    for (const DiaOutput& dia : resultado.resultado.horarios.value(curso).dias) {
        for (const AsignacionOutput& asignacion : dia.asignaciones) {
            diasConClase.insert(dia.dia);
            EXPECT_LE(asignacion.slot, 1)
                << "la clase debe quedar al inicio para dejar los huecos al final";
        }
    }
    EXPECT_EQ(diasConClase.size(), 2);
    EXPECT_FALSE(diasConClase.contains(4)) << "el viernes debe quedar libre";
}

TEST(TestSolverBloques, preferenciaNoRompeLaSeparacionNiElValorDeLa2a) {
    // La preferencia intenta concentrar la materia en lunes/martes/miércoles, lo que empeoraría
    // la separación (0 → 2). La 2b fija el valor de la 2a y no lo permite.
    SolverConfig config =
        configAlineacion({{QStringLiteral("Matemática"), 6}}, /*dias=*/5, /*franjas=*/2);
    for (int dia = 0; dia <= 2; ++dia)
        config.preferencias.append(
            preferencia(0, dia, {0, 1}, QStringLiteral("preferida")));

    Solver solver;
    const auto resultado = solver.resolver(config);
    ASSERT_TRUE(resultado.exito) << resultado.errores.join("\n").toStdString();

    const QString curso = QStringLiteral("1|A");
    const QVector<int> dias = rejilla::diasDeMateria(
        config, 0, 0, franjasPorMateria(resultado.resultado, curso).value(0));
    EXPECT_EQ(dias, (QVector<int>{0, 2, 4}));
    EXPECT_EQ(rejilla::incumplimientosSeparacion(dias), 0);
}

TEST(TestSolverBloques, sinPreferenciasSeOmiteLa2b) {
    const SolverConfig config =
        configAlineacion({{QStringLiteral("Matemática"), 6}}, /*dias=*/5, /*franjas=*/2);
    Solver solver;

    const auto normal = solver.resolver(config);
    Solver::PresupuestoFases sin2b;
    sin2b.ejecutarPreferencias = false;   // irrelevante: sin preferencias la 2b no se ejecuta
    const auto conPresupuesto = solver.resolver(config, sin2b);

    ASSERT_TRUE(normal.exito) << normal.errores.join("\n").toStdString();
    ASSERT_TRUE(conPresupuesto.exito) << conPresupuesto.errores.join("\n").toStdString();
    const QString curso = QStringLiteral("1|A");
    EXPECT_EQ(rejilla::diasDeMateria(
                  config, 0, 0, franjasPorMateria(normal.resultado, curso).value(0)),
              rejilla::diasDeMateria(
                  config, 0, 0, franjasPorMateria(conPresupuesto.resultado, curso).value(0)));
}

TEST(TestSolverBloques, escalera2bDevuelveLa2a) {
    // Preferencia que no puede mejorar la 2a; con la 2b desactivada se devuelve la 2a, no un
    // error (020: las fases blandas comparten el deadline y la escalera se prueba por flags).
    SolverConfig config =
        configAlineacion({{QStringLiteral("Matemática"), 6}}, /*dias=*/5, /*franjas=*/2);
    config.preferencias = {preferencia(0, 0, {0, 1}, QStringLiteral("preferida"))};

    Solver solver;
    const auto dosA = solver.resolver(config);

    Solver::PresupuestoFases sin2b;
    sin2b.ejecutarPreferencias = false;
    const auto conPresupuesto = solver.resolver(config, sin2b);

    ASSERT_TRUE(dosA.exito) << dosA.errores.join("\n").toStdString();
    ASSERT_TRUE(conPresupuesto.exito) << conPresupuesto.errores.join("\n").toStdString();
    const QString curso = QStringLiteral("1|A");
    const QVector<int> dias = rejilla::diasDeMateria(
        config, 0, 0, franjasPorMateria(conPresupuesto.resultado, curso).value(0));
    EXPECT_EQ(dias, (QVector<int>{0, 2, 4}));
}

TEST(TestSolverBloques, presupuestoPorDefectoDeLa020) {
    // 020 (RF-3): 55 min de reloj para todo el solver; los 5 hasta la hora son reserva del
    // servicio. 8 workers se mantienen (014b) y seed 0 no cambia.
    const Solver::PresupuestoFases presupuesto;
    EXPECT_DOUBLE_EQ(presupuesto.totalSegundos, 3300.0);
    EXPECT_EQ(presupuesto.workers, 8);
    EXPECT_TRUE(presupuesto.ejecutarSeparacionHuecos);
    EXPECT_TRUE(presupuesto.ejecutarPreferencias);
}

TEST(TestSolverBloques, restanteDeReloj_nuncaEsNegativo) {
    // 020 (D15): el presupuesto de cada fase es `max(0, deadline − ahora)`; con 0 no se llama
    // al motor (en CP-SAT 0 s significa SIN límite).
    EXPECT_EQ(restanteMs(3300000, 0), 3300000);
    EXPECT_EQ(restanteMs(3300000, 3300000), 0);
    EXPECT_EQ(restanteMs(3300000, 4000000), 0);
    EXPECT_DOUBLE_EQ(restanteSegundos(1500, 0), 1.5);
    EXPECT_DOUBLE_EQ(restanteSegundos(1500, 2000), 0.0);
}

// ─── Tamaño del modelo representativo ───────────────────────────────────────

TEST(TestSolverBloques, tamanoDelModeloRepresentativo) {
    const SolverConfig config = configRepresentativa();
    CpModelBuilder model;
    const VariablesSolver vars = crearVariables(model, config);
    agregarRestriccionesBloques(model, config, vars);

    const CpModelProto proto = model.Build();
    std::cout << "[   INFO   ] modelo representativo: " << proto.variables_size()
              << " variables (" << vars.assignment.size() << " x, "
              << vars.aulaAssignment.size() << " y, " << vars.ocupada.size() << " ocupada, "
              << vars.par.size() << " par, " << vars.materiaEnFranja.size()
              << " materiaEnFranja, " << vars.bloqueSimple.size() << " bloqueSimple), "
              << proto.constraints_size() << " restricciones" << std::endl;

    // Presupuesto del plan §12: ~30–40k booleanas. Si se supera, se poda.
    EXPECT_GE(proto.variables_size(), 25000);
    EXPECT_LE(proto.variables_size(), 40000);
}
