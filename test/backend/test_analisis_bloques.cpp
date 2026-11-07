/**
 * @file test_analisis_bloques.cpp
 * @brief Tests del informe de bloques, desvío, huecos y separación (014 T6, RF-12).
 */

#include <gtest/gtest.h>

#include <QMap>
#include <QString>
#include <QVector>

#include "backend/services/analisis_bloques.hpp"
#include "backend/services/ValidadorSalidaSolver.hpp"
#include "backend/solver/config/contrato_solver.hpp"
#include "backend/solver/config/solver_config.hpp"

namespace {

/// Curso 1|A de 2 días × 4 franjas con dos materias de 3 h cada una.
SolverConfig configDosMateriasDeTres() {
    SolverConfig config;
    config.version = QStringLiteral("2.0");
    config.dimensiones = {0, 2, 1, 1, 2, 4};
    config.franja_horaria = {40, 4};
    config.turnos.turnos = {
        TurnoConfig{QStringLiteral("manana"), {0, 1, 2, 3}, QString(), QString()}};
    config.materias = {{QStringLiteral("Matemática"), QStringLiteral("regular")},
                       {QStringLiteral("Historia"), QStringLiteral("regular")}};
    config.aulas = {{QStringLiteral("A1"), QStringLiteral("regular"), 30}};
    config.planes = {{QStringLiteral("Plan"), {{0, 1, 3}, {1, 1, 3}}}};
    config.cursos = {CursoSolverConfig{1, QStringLiteral("A"), QStringLiteral("manana"), 20,
                                       QStringLiteral("Plan"), 0}};
    return config;
}

/// Horario: día 0 con dos bloques simples (m0 en 0-1 y m1 en 2-3) y día 1 con el bloque
/// compartido de los dos restos (m0 en 0 y m1 en 1).
HorarioSalida horarioDosMateriasDeTres() {
    HorarioSalida horario;
    CursoOutput curso;
    curso.turno = QStringLiteral("manana");

    DiaOutput dia0;
    dia0.dia = 0;
    dia0.asignaciones = {AsignacionOutput{0, 0, -1, 0},
                         AsignacionOutput{1, 0, -1, 0},
                         AsignacionOutput{2, 1, -1, 0},
                         AsignacionOutput{3, 1, -1, 0}};

    DiaOutput dia1;
    dia1.dia = 1;
    dia1.asignaciones = {AsignacionOutput{0, 0, -1, 0}, AsignacionOutput{1, 1, -1, 0}};

    curso.dias = {dia0, dia1};
    horario.horarios[claveCurso(1, QStringLiteral("A"))] = curso;
    return horario;
}

} // namespace

TEST(TestAnalisisBloques, bloquesSimplesYCompartido) {
    const SolverConfig config = configDosMateriasDeTres();
    const AnalisisBloques analisis = analizarBloques(horarioDosMateriasDeTres(), config);

    ASSERT_EQ(analisis.bloques.size(), 3);
    EXPECT_EQ(analisis.bloquesSimples, 2);
    EXPECT_EQ(analisis.bloquesCompartidos, 1);
    EXPECT_EQ(analisis.bloquesDeUno, 0);

    // Día 0: dos bloques simples; día 1: el compartido (materias 0 y 1).
    EXPECT_EQ(analisis.bloques[0].diaDominio, 1);   // generación 0 → dominio 1 (lunes)
    EXPECT_EQ(analisis.bloques[0].materiaA, 0);
    EXPECT_EQ(analisis.bloques[0].materiaB, 0);
    EXPECT_FALSE(analisis.bloques[0].compartido);
    EXPECT_EQ(analisis.bloques[2].diaDominio, 2);
    EXPECT_TRUE(analisis.bloques[2].compartido);
    EXPECT_EQ(analisis.bloques[2].materiaA, 0);
    EXPECT_EQ(analisis.bloques[2].materiaB, 1);
    EXPECT_EQ(analisis.bloques[2].aula, 0);

    ASSERT_EQ(analisis.cursos.size(), 1);
    EXPECT_EQ(analisis.cursos[0].curso, QStringLiteral("1|A"));
    EXPECT_EQ(analisis.cursos[0].bloquesSimples, 2);
    EXPECT_EQ(analisis.cursos[0].bloquesCompartidos, 1);
    EXPECT_EQ(analisis.cursos[0].bloquesDeUno, 0);
    EXPECT_EQ(analisis.cursos[0].desvio, 0);
    EXPECT_EQ(analisis.cursos[0].tope, 2);
}

TEST(TestAnalisisBloques, desvioUnicoYTope) {
    const SolverConfig config = configDosMateriasDeTres();
    const AnalisisBloques analisis = analizarBloques(horarioDosMateriasDeTres(), config);

    // Cada materia de 3 h tiene una franja descubierta = su resto → desvío 0; tope 2.
    EXPECT_EQ(analisis.desvio, 0);
    EXPECT_EQ(analisis.tope, 2);

    ASSERT_EQ(analisis.materias.size(), 2);
    for (const MateriaSalida& materia : analisis.materias) {
        EXPECT_EQ(materia.resto, 1);
        EXPECT_EQ(materia.franjasDescubiertas, 1);
        EXPECT_EQ(materia.curso, QStringLiteral("1|A"));
        EXPECT_EQ(materia.apariciones, 2);
        EXPECT_EQ(materia.franjas.size(), 2);
        EXPECT_EQ(materia.franjas[0].diaDominio, 1);
        const int inicioEsperado = (materia.materia == 0) ? 0 : 2;
        EXPECT_EQ(materia.franjas[0].slotInicio, inicioEsperado);
        EXPECT_EQ(materia.franjas[0].slotFin, inicioEsperado + 1);
        EXPECT_EQ(materia.diasDominio, (QVector<int>{1, 2}));
        EXPECT_EQ(materia.diasIncumplidosSeparacion, (QVector<int>{1, 2}));
        EXPECT_EQ(materia.incumplimientosSeparacion, 1);
    }
    EXPECT_EQ(analisis.incumplimientosSeparacion, 2);
    EXPECT_EQ(analisis.cursos[0].incumplimientosSeparacion, 2);
}

TEST(TestAnalisisBloques, huecosAlFinalYNoViernes) {
    const SolverConfig config = configDosMateriasDeTres();
    const AnalisisBloques analisis = analizarBloques(horarioDosMateriasDeTres(), config);

    // Día 0 está lleno; día 1 solo ocupa 0-1 → un tramo de huecos 2-3.
    ASSERT_EQ(analisis.huecos.size(), 1);
    EXPECT_EQ(analisis.huecos[0].diaDominio, 2);
    EXPECT_EQ(analisis.huecos[0].slotInicio, 2);
    EXPECT_EQ(analisis.huecos[0].slotFin, 3);
    EXPECT_EQ(analisis.huecosNoFinal, 0);      // están al final del día
    EXPECT_EQ(analisis.huecosNoViernes, 2);    // día de generación 1 (no viernes)
    EXPECT_FALSE(analisis.huecos[0].esViernes);
    EXPECT_EQ(analisis.cursos[0].huecos, 2);
    EXPECT_EQ(analisis.cursos[0].huecosNoViernes, 2);
}

// ─── P4: bloque de 1, materia mal alineada y dos aulas en un bloque ─────────

TEST(TestAnalisisBloques, p4DetectaBloqueDeUno) {
    // Una materia de 2 h con una sola clase ocupada: el curso tiene una franja sin bloque.
    SolverConfig config;
    config.version = QStringLiteral("2.0");
    config.dimensiones = {0, 1, 1, 1, 1, 2};
    config.franja_horaria = {40, 2};
    config.turnos.turnos = {
        TurnoConfig{QStringLiteral("manana"), {0, 1}, QString(), QString()}};
    config.materias = {{QStringLiteral("Matemática"), QStringLiteral("regular")}};
    config.aulas = {{QStringLiteral("A1"), QStringLiteral("regular"), 30}};
    config.planes = {{QStringLiteral("Plan"), {{0, 1, 2}}}};
    config.cursos = {CursoSolverConfig{1, QStringLiteral("A"), QStringLiteral("manana"), 20,
                                       QStringLiteral("Plan"), 0}};

    HorarioSalida horario;
    CursoOutput curso;
    curso.turno = QStringLiteral("manana");
    DiaOutput dia;
    dia.dia = 0;
    dia.asignaciones = {AsignacionOutput{0, 0, -1, 0}};
    curso.dias = {dia};
    horario.horarios[claveCurso(1, QStringLiteral("A"))] = curso;

    const AnalisisSalida analisis = analizarSalidaSolver(horario, config);
    EXPECT_FALSE(analisis.presentable());
    EXPECT_TRUE(analisis.conflictos.join(QStringLiteral("\n"))
                    .contains(QStringLiteral("sin bloque de dos horas")));
}

TEST(TestAnalisisBloques, p4DetectaMateriaMalAlineada) {
    // La rejilla del curso se empareja, pero cada materia de 2 h queda partida en dos franjas
    // sueltas de bloques compartidos: dos descubiertas con resto 0.
    SolverConfig config;
    config.version = QStringLiteral("2.0");
    config.dimensiones = {0, 2, 1, 1, 1, 4};
    config.franja_horaria = {40, 4};
    config.turnos.turnos = {
        TurnoConfig{QStringLiteral("manana"), {0, 1, 2, 3}, QString(), QString()}};
    config.materias = {{QStringLiteral("Matemática"), QStringLiteral("regular")},
                       {QStringLiteral("Historia"), QStringLiteral("regular")}};
    config.aulas = {{QStringLiteral("A1"), QStringLiteral("regular"), 30}};
    config.planes = {{QStringLiteral("Plan"), {{0, 1, 2}, {1, 1, 2}}}};
    config.cursos = {CursoSolverConfig{1, QStringLiteral("A"), QStringLiteral("manana"), 20,
                                       QStringLiteral("Plan"), 0}};

    HorarioSalida horario;
    CursoOutput curso;
    curso.turno = QStringLiteral("manana");
    DiaOutput dia;
    dia.dia = 0;
    // m0 en 0 y 2; m1 en 1 y 3: pares (0,1) y (2,3) compartidos, sin bloques de 1.
    dia.asignaciones = {AsignacionOutput{0, 0, -1, 0}, AsignacionOutput{1, 1, -1, 0},
                        AsignacionOutput{2, 0, -1, 0}, AsignacionOutput{3, 1, -1, 0}};
    curso.dias = {dia};
    horario.horarios[claveCurso(1, QStringLiteral("A"))] = curso;

    const AnalisisSalida analisis = analizarSalidaSolver(horario, config);
    EXPECT_FALSE(analisis.presentable());
    EXPECT_TRUE(analisis.conflictos.join(QStringLiteral("\n"))
                    .contains(QStringLiteral("franjas descubiertas")));
}

TEST(TestAnalisisBloques, p4DetectaDosAulasEnUnBloque) {
    // Materia de 2 h en el par (0,1), pero cada franja informa un aula distinta.
    SolverConfig config;
    config.version = QStringLiteral("2.0");
    config.dimensiones = {0, 1, 2, 1, 1, 2};
    config.franja_horaria = {40, 2};
    config.turnos.turnos = {
        TurnoConfig{QStringLiteral("manana"), {0, 1}, QString(), QString()}};
    config.materias = {{QStringLiteral("Matemática"), QStringLiteral("regular")}};
    config.aulas = {{QStringLiteral("A1"), QStringLiteral("regular"), 30},
                    {QStringLiteral("A2"), QStringLiteral("regular"), 30}};
    config.planes = {{QStringLiteral("Plan"), {{0, 1, 2}}}};
    config.cursos = {CursoSolverConfig{1, QStringLiteral("A"), QStringLiteral("manana"), 20,
                                       QStringLiteral("Plan"), 0}};

    HorarioSalida horario;
    CursoOutput curso;
    curso.turno = QStringLiteral("manana");
    DiaOutput dia;
    dia.dia = 0;
    dia.asignaciones = {AsignacionOutput{0, 0, -1, 0}, AsignacionOutput{1, 0, -1, 1}};
    curso.dias = {dia};
    horario.horarios[claveCurso(1, QStringLiteral("A"))] = curso;

    const AnalisisSalida analisis = analizarSalidaSolver(horario, config);
    EXPECT_FALSE(analisis.presentable());
    EXPECT_TRUE(analisis.conflictos.join(QStringLiteral("\n"))
                    .contains(QStringLiteral("dos aulas distintas")));
}


TEST(TestAnalisisBloques, p4DetectaHuecoNoSufijo) {
    // Par en las franjas 1-2 de un día de 4: la franja 0 queda libre con clase después.
    SolverConfig config;
    config.version = QStringLiteral("2.0");
    config.dimensiones = {0, 1, 1, 1, 1, 4};
    config.franja_horaria = {40, 4};
    config.turnos.turnos = {
        TurnoConfig{QStringLiteral("manana"), {0, 1, 2, 3}, QString(), QString()}};
    config.materias = {{QStringLiteral("Matemática"), QStringLiteral("regular")}};
    config.aulas = {{QStringLiteral("A1"), QStringLiteral("regular"), 30}};
    config.planes = {{QStringLiteral("Plan"), {{0, 1, 2}}}};
    config.cursos = {CursoSolverConfig{1, QStringLiteral("A"), QStringLiteral("manana"), 20,
                                       QStringLiteral("Plan"), 0}};

    HorarioSalida horario;
    CursoOutput curso;
    curso.turno = QStringLiteral("manana");
    DiaOutput dia;
    dia.dia = 0;
    dia.asignaciones = {AsignacionOutput{1, 0, -1, 0}, AsignacionOutput{2, 0, -1, 0}};
    curso.dias = {dia};
    horario.horarios[claveCurso(1, QStringLiteral("A"))] = curso;

    const AnalisisSalida analisis = analizarSalidaSolver(horario, config);
    EXPECT_FALSE(analisis.presentable());
    EXPECT_TRUE(analisis.conflictos.join(QStringLiteral("\n"))
                    .contains(QStringLiteral("hueco que no es sufijo")));
}

TEST(TestAnalisisBloques, separacionIncumplidaSeInformaNoEsConflictoP4) {
    // La separación es blanda: el informe la cuenta, pero P4 no la convierte en conflicto.
    const SolverConfig config = configDosMateriasDeTres();
    const AnalisisBloques informe = analizarBloques(horarioDosMateriasDeTres(), config);
    EXPECT_EQ(informe.incumplimientosSeparacion, 2);

    const AnalisisSalida analisis = analizarSalidaSolver(horarioDosMateriasDeTres(), config);
    EXPECT_TRUE(analisis.presentable());
}
