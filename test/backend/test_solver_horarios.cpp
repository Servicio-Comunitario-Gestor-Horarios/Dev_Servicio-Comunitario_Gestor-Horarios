#include <QTest>
#include <QSet>
#include <QPair>
#include <QMap>
#include <backend/solver/config/contrato_solver.hpp>
#include <backend/solver/config/solver_config.hpp>
#include <backend/solver/solver.hpp>

/// Tests del solver CP-SAT con el contrato nuevo (spec 005).
/// El curso se identifica por anio+seccion; sus materias salen del plan.
class TestSolverHorarios : public QObject {
    Q_OBJECT

private:
    // Disponibilidad completa (3 dias x 4 franjas).
    static QVector<Disponibilidad> dispCompleta() {
        return {{0, {0,1,2,3}}, {1, {0,1,2,3}}, {2, {0,1,2,3}}};
    }

    SolverConfig crearConfigMinima() {
        SolverConfig config;
        config.version = "1.0";
        config.aula_fija = true;
        config.dimensiones = {2, 2, 2, 1, 3, 4};
        config.franja_horaria = {60, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};

        config.materias = {{"Matematica"}, {"Historia"}};
        config.aulas = {{"Aula 1", "regular", 30}, {"Aula 2", "regular", 25}};

        // Plan: materias {materia_idx, curso(anio), horas} para el anio 1.
        config.planes = {{"Plan General", {{0, 1, 4}, {1, 1, 4}}}};

        // Curso por identidad; aula fija = indice 0.
        config.cursos = {{1, "A", "manana", 20, "Plan General", 0}};

        config.profesores = {
            {"Prof A", 20, 4, "manana", "Plan General", {}, {0, 1}, {}, dispCompleta()},
            {"Prof B", 20, 4, "manana", "Plan General", {}, {0, 1}, {}, dispCompleta()}};
        return config;
    }

private slots:

    void configValida_noTieneErrores() {
        SolverConfig config = crearConfigMinima();
        QCOMPARE(config.profesores.size(), 2);
        QCOMPARE(config.cursos.size(), 1);
        QCOMPARE(config.materias.size(), 2);
        QCOMPARE(config.aulas.size(), 2);
    }

    void solver_encuentraSolucion() {
        SolverConfig config = crearConfigMinima();
        Solver solver;
        auto resultado = solver.resolver(config);
        QVERIFY(resultado.exito);
        QVERIFY(resultado.errores.isEmpty());
        QVERIFY(resultado.tiempo_ms >= 0);
    }

    void solver_respetaTurnoManana() {
        SolverConfig config = crearConfigMinima();
        Solver solver;
        auto resultado = solver.resolver(config);
        QVERIFY(resultado.exito);
        for (const auto& curso : resultado.resultado.horarios)
            for (const auto& dia : curso.dias)
                for (const auto& asig : dia.asignaciones)
                    QVERIFY(asig.slot >= 0 && asig.slot <= 3);
    }

    void solver_docenteNoSolapado() {
        SolverConfig config = crearConfigMinima();
        Solver solver;
        auto resultado = solver.resolver(config);
        QVERIFY(resultado.exito);

        QMap<QPair<int, int>, QVector<int>> profSlotDia;
        for (const auto& curso : resultado.resultado.horarios)
            for (const auto& dia : curso.dias)
                for (const auto& asig : dia.asignaciones) {
                    auto key = qMakePair(asig.profesor, dia.dia);
                    QVERIFY(!profSlotDia[key].contains(asig.slot));
                    profSlotDia[key].append(asig.slot);
                }
    }

    void solver_aulasNoSolapadas() {
        SolverConfig config = crearConfigMinima();
        Solver solver;
        auto resultado = solver.resolver(config);
        QVERIFY(resultado.exito);

        QMap<QPair<int, int>, QVector<int>> aulaSlotDia;
        for (const auto& curso : resultado.resultado.horarios)
            for (const auto& dia : curso.dias)
                for (const auto& asig : dia.asignaciones) {
                    auto key = qMakePair(asig.aula, dia.dia);
                    QVERIFY(!aulaSlotDia[key].contains(asig.slot));
                    aulaSlotDia[key].append(asig.slot);
                }
    }

    void solver_infeasible_retornaError() {
        SolverConfig config = crearConfigMinima();
        // 2 cursos (uno por anio), misma materia; un solo docente con 2 franjas.
        config.cursos = {
            {1, "A", "manana", 20, "Plan General", 0},
            {2, "A", "manana", 20, "Plan General", 1}};
        config.planes = {{"Plan General", {{0, 1, 4}, {1, 1, 4}}}}; // solo cubre anio 1
        config.planes = {{"Plan General", {{0, 1, 4}, {0, 2, 4}}}}; // materia 0 en anios 1 y 2
        config.profesores = {
            {"Prof A", 16, 16, "manana", "Plan General", {}, {0}, {}, {{0, {0, 1}}}}};
        config.dimensiones = {1, 1, 2, 2, 3, 4};

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY(!resultado.exito);
        QVERIFY(!resultado.errores.isEmpty());
        QVERIFY(resultado.errores[0].contains("INFEASIBLE"));
    }

    // ── D1/D3 SIEMPRE publicadas: sin candidato disponible => INFEASIBLE, nunca éxito con 0 bloques ──
    // Curso cuyo turno (tarde) no se solapa con la disposición del único titular (mañana):
    // debe fallar la factibilidad, no devolver éxito con la materia-curso sin cubrir.
    void solver_sinCandidatoDisponible_esInfeasible() {
        SolverConfig config;
        config.version = "1.0";
        config.aula_fija = true;
        config.dimensiones = {1, 1, 1, 1, 3, 8};
        config.franja_horaria = {60, 8};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}, {"tarde", {4, 5, 6, 7}}};
        config.materias = {{"Matematica"}};
        config.aulas = {{"Aula 1", "regular", 30}};
        config.planes = {{"Plan General", {{0, 1, 2}}}};                  // materia 0, 2 bloques
        config.cursos = {{1, "A", "tarde", 20, "Plan General", 0}};        // curso de turno TARDE
        config.profesores = {
            // Único titular: solo disponible por la mañana, fuera del turno del curso.
            {"Prof A", 2, 2, "manana", "Plan General", {}, {0}, {}, dispCompleta()}};

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY2(!resultado.exito,
                 "un curso cuyo único candidato no se solapa con su turno debe ser INFEASIBLE");
        QVERIFY(!resultado.errores.isEmpty());
        QVERIFY(resultado.errores[0].contains("INFEASIBLE"));
    }

    // ── XOR: un docente por materia-curso ──
    void solver_xorUnDocentePorMateria() {
        SolverConfig config;
        config.version = "1.0";
        config.aula_fija = true;
        config.dimensiones = {2, 1, 2, 1, 3, 4};
        config.franja_horaria = {60, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};
        config.materias = {{"Matematica"}};
        config.aulas = {{"Aula 1", "regular", 30}, {"Aula 2", "regular", 25}};
        config.planes = {{"Plan General", {{0, 1, 4}}}};
        config.cursos = {{1, "A", "manana", 20, "Plan General", 0}};
        config.profesores = {
            {"Prof A", 4, 4, "manana", "Plan General", {}, {0}, {}, dispCompleta()},
            {"Prof B", 4, 4, "manana", "Plan General", {}, {0}, {}, dispCompleta()}};

        Solver solver;
        auto resultado = solver.resolver(config);
        QVERIFY(resultado.exito);

        int slotsA = 0, slotsB = 0;
        const auto& curso = resultado.resultado.horarios[claveCurso(1, "A")];
        for (const auto& dia : curso.dias)
            for (const auto& asig : dia.asignaciones)
                if (asig.materia == 0) {
                    if (asig.profesor == 0) slotsA++;
                    else if (asig.profesor == 1) slotsB++;
                }
        QVERIFY(slotsA == 4 || slotsB == 4);
        QVERIFY((slotsA == 4) != (slotsB == 4));
    }

    // ── Distribucion en >=2 dias ──
    void solver_distribucionHorasMultiplesDias() {
        SolverConfig config;
        config.version = "1.0";
        config.aula_fija = true;
        config.dimensiones = {1, 1, 1, 1, 5, 4};
        config.franja_horaria = {60, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};
        config.materias = {{"Matematica"}};
        config.aulas = {{"Aula 1", "regular", 30}};
        config.planes = {{"Plan General", {{0, 1, 4}}}};
        config.cursos = {{1, "A", "manana", 20, "Plan General", 0}};
        config.profesores = {
            {"Prof A", 4, 4, "manana", "Plan General", {}, {0}, {},
             {{0, {0,1}}, {1, {0,1}}, {2, {0,1}}, {3, {0,1}}, {4, {0,1}}}}};

        Solver solver;
        auto resultado = solver.resolver(config);
        QVERIFY(resultado.exito);

        QSet<int> diasUsados;
        const auto& curso = resultado.resultado.horarios[claveCurso(1, "A")];
        int total = 0;
        for (const auto& dia : curso.dias) {
            for (const auto& asig : dia.asignaciones) { diasUsados.insert(dia.dia); (void)asig; }
            total += dia.asignaciones.size();
        }
        QVERIFY(diasUsados.size() >= 2);
        QCOMPARE(total, 4);
    }

    // ── Suplente reemplaza completo ──
    void solver_suplenteReemplaza() {
        SolverConfig config;
        config.version = "1.0";
        config.aula_fija = true;
        config.dimensiones = {2, 1, 1, 1, 3, 4};
        config.franja_horaria = {60, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};
        config.materias = {{"Matematica"}};
        config.aulas = {{"Aula 1", "regular", 30}};
        config.planes = {{"Plan General", {{0, 1, 4}}}};
        config.cursos = {{1, "A", "manana", 20, "Plan General", 0}};
        config.profesores = {
            // Titular pero plan incompatible con el curso.
            {"Prof A", 4, 4, "manana", "Ciencias", {}, {0}, {}, dispCompleta()},
            // Suplente con plan correcto.
            {"Prof B", 4, 4, "manana", "Plan General", {}, {}, {{0, 3}}, dispCompleta()}};

        Solver solver;
        auto resultado = solver.resolver(config);
        QVERIFY(resultado.exito);

        int slotsA = 0, slotsB = 0;
        const auto& curso = resultado.resultado.horarios[claveCurso(1, "A")];
        for (const auto& dia : curso.dias)
            for (const auto& asig : dia.asignaciones) {
                if (asig.profesor == 0) slotsA++;
                else if (asig.profesor == 1) slotsB++;
            }
        QCOMPARE(slotsA, 0);
        QCOMPARE(slotsB, 4);
    }
};

QTEST_MAIN(TestSolverHorarios)
#include "test_solver_horarios.moc"
