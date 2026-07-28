#include <QTest>
#include <QJsonObject>
#include <QJsonDocument>
#include <backend/solver/config/solver_config.hpp>
#include <backend/solver/solver.hpp>

class TestSolverHorarios : public QObject {
    Q_OBJECT

private:
    SolverConfig crearConfigMinima() {
        SolverConfig config;
        config.version = "1.0";
        config.dimensiones = {2, 2, 2, 1, 3, 4};
        config.franja_horaria = {60, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};

        config.materias = {{"Matematica"}, {"Historia"}};
        config.aulas = {{"Aula 1", "regular", 30}, {"Aula 2", "regular", 25}};

        config.cursos = {{
            "1ro A", "manana", -1, 20, "Plan General",
            {{0, 4}, {1, 4}}
        }};

        config.profesores = {{
            "Prof A", 20, 4, "manana", "Plan General",
            {0, 1}, {}, {{0, {0,1,2,3}}, {1, {0,1,2,3}}, {2, {0,1,2,3}}}
        }, {
            "Prof B", 20, 4, "manana", "Plan General",
            {0, 1}, {}, {{0, {0,1,2,3}}, {1, {0,1,2,3}}, {2, {0,1,2,3}}}
        }};

        config.planificacion = {false};
        config.generacion = {{0}};
        config.penalizaciones = {100, 50};
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

        for (const auto& curso : resultado.resultado.horarios) {
            for (const auto& dia : curso.dias) {
                for (const auto& asig : dia.asignaciones) {
                    QVERIFY(asig.slot >= 0 && asig.slot <= 3);
                }
            }
        }
    }

    void solver_docenteNoSolapado() {
        SolverConfig config = crearConfigMinima();
        Solver solver;
        auto resultado = solver.resolver(config);
        QVERIFY(resultado.exito);

        QMap<QPair<int, int>, QVector<int>> profSlotDia;
        for (const auto& curso : resultado.resultado.horarios) {
            for (const auto& dia : curso.dias) {
                for (const auto& asig : dia.asignaciones) {
                    auto key = qMakePair(asig.profesor, dia.dia);
                    QVERIFY(!profSlotDia[key].contains(asig.slot));
                    profSlotDia[key].append(asig.slot);
                }
            }
        }
    }

    void solver_aulasNoSolapadas() {
        SolverConfig config = crearConfigMinima();
        Solver solver;
        auto resultado = solver.resolver(config);
        QVERIFY(resultado.exito);

        QMap<QPair<int, int>, QVector<int>> aulaSlotDia;
        for (const auto& curso : resultado.resultado.horarios) {
            for (const auto& dia : curso.dias) {
                for (const auto& asig : dia.asignaciones) {
                    auto key = qMakePair(asig.aula, dia.dia);
                    QVERIFY(!aulaSlotDia[key].contains(asig.slot));
                    aulaSlotDia[key].append(asig.slot);
                }
            }
        }
    }

    void solver_infeasible_retornaError() {
        SolverConfig config = crearConfigMinima();
        config.cursos = {
            {"1ro A", "manana", -1, 20, "Plan General", {{0, 4}, {1, 4}}},
            {"2do A", "manana", -1, 20, "Plan General", {{0, 4}, {1, 4}}}
        };
        config.profesores = {{
            "Prof A", 16, 16, "manana", "Plan General",
            {0, 1}, {}, {{0, {0, 1}}}
        }};
        config.dimensiones = {1, 2, 2, 2, 3, 4};

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY(!resultado.exito);
        QVERIFY(!resultado.errores.isEmpty());
        QVERIFY(resultado.errores[0].contains("INFEASIBLE"));
    }

    // ── Milestone 7: XOR — un docente por materia-curso ──
    void solver_xorUnDocentePorMateria() {
        // 2 professores, ambos titulares de Matematica, 1 curso que necesita 4h.
        // Solo UNO de ellos debe ensenar Matematica en ese curso.
        SolverConfig config;
        config.version = "1.0";
        config.dimensiones = {2, 1, 2, 1, 3, 4};
        config.franja_horaria = {60, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};
        config.materias = {{"Matematica"}};
        config.aulas = {{"Aula 1", "regular", 30}, {"Aula 2", "regular", 25}};
        config.cursos = {{
            "1ro A", "manana", -1, 20, "Plan General",
            {{0, 4}}
        }};
        config.profesores = {{
            "Prof A", 4, 4, "manana", "Plan General",
            {0}, {}, {{0, {0,1,2,3}}, {1, {0,1,2,3}}, {2, {0,1,2,3}}}
        }, {
            "Prof B", 4, 4, "manana", "Plan General",
            {0}, {}, {{0, {0,1,2,3}}, {1, {0,1,2,3}}, {2, {0,1,2,3}}}
        }};
        config.planificacion = {false};
        config.generacion = {{0}};
        config.penalizaciones = {100, 50};

        Solver solver;
        auto resultado = solver.resolver(config);
        QVERIFY(resultado.exito);

        // Contar cuantos slots asigna cada profesor a Matematica en 1ro A
        int slotsA = 0, slotsB = 0;
        const auto& curso = resultado.resultado.horarios["1ro A"];
        for (const auto& dia : curso.dias) {
            for (const auto& asig : dia.asignaciones) {
                if (asig.materia == 0) {
                    if (asig.profesor == 0) slotsA++;
                    else if (asig.profesor == 1) slotsB++;
                }
            }
        }
        // XOR: exactamente uno ensena, el otro no
        QVERIFY(slotsA == 4 || slotsB == 4);
        QVERIFY((slotsA == 4) != (slotsB == 4));
    }

    // ── Milestone 8: Distribucion de horas en >=2 dias ──
    void solver_distribucionHorasMultiplesDias() {
        // 1 prof, 1 materia, 1 curso, 4h requeridas.
        // Prof solo disponible 2 slots/dia -> obligado a usar >=2 dias.
        SolverConfig config;
        config.version = "1.0";
        config.dimensiones = {1, 1, 1, 1, 5, 4};
        config.franja_horaria = {60, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};
        config.materias = {{"Matematica"}};
        config.aulas = {{"Aula 1", "regular", 30}};
        config.cursos = {{
            "1ro A", "manana", 0, 20, "Plan General",
            {{0, 4}}
        }};
        // Prof disponible solo slots 0,1 cada dia (max 2 por dia)
        config.profesores = {{
            "Prof A", 4, 4, "manana", "Plan General",
            {0}, {}, {{0, {0,1}}, {1, {0,1}}, {2, {0,1}}, {3, {0,1}}, {4, {0,1}}}
        }};
        config.planificacion = {false};
        config.generacion = {{0}};
        config.penalizaciones = {100, 50};

        Solver solver;
        auto resultado = solver.resolver(config);
        QVERIFY(resultado.exito);

        // Verificar que las asignaciones usan al menos 2 dias distintos
        QSet<int> diasUsados;
        const auto& curso = resultado.resultado.horarios["1ro A"];
        for (const auto& dia : curso.dias) {
            for (const auto& asig : dia.asignaciones) {
                diasUsados.insert(dia.dia);
            }
        }
        QVERIFY(diasUsados.size() >= 2);

        // Verificar total = 4 slots
        int total = 0;
        for (const auto& dia : curso.dias) {
            total += dia.asignaciones.size();
        }
        QCOMPARE(total, 4);
    }

    // ── Milestone 9: Suplente reemplaza completo ──
    void solver_suplenteReemplaza() {
        // Prof A: titular de Matematica, pero plan "Ciencias" (mismatch con curso "General")
        // Prof B: suplente de Matematica, plan "General"
        // Resultado: Prof A forzado a 0 por plan, Prof B cubre todo
        SolverConfig config;
        config.version = "1.0";
        config.dimensiones = {2, 1, 1, 1, 3, 4};
        config.franja_horaria = {60, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};
        config.materias = {{"Matematica"}};
        config.aulas = {{"Aula 1", "regular", 30}};
        config.cursos = {{
            "1ro A", "manana", 0, 20, "Plan General",
            {{0, 4}}
        }};
        config.profesores = {{
            // Titular pero plan incorrecto
            "Prof A", 4, 4, "manana", "Ciencias",
            {0}, {}, {{0, {0,1,2,3}}, {1, {0,1,2,3}}, {2, {0,1,2,3}}}
        }, {
            // Suplente con plan correcto
            "Prof B", 4, 4, "manana", "Plan General",
            {}, {{0, 3}}, {{0, {0,1,2,3}}, {1, {0,1,2,3}}, {2, {0,1,2,3}}}
        }};
        config.planificacion = {false};
        config.generacion = {{0}};
        config.penalizaciones = {100, 50};

        Solver solver;
        auto resultado = solver.resolver(config);
        QVERIFY(resultado.exito);

        // Prof A no debe tener ninguna asignacion (plan mismatch)
        int slotsA = 0, slotsB = 0;
        const auto& curso = resultado.resultado.horarios["1ro A"];
        for (const auto& dia : curso.dias) {
            for (const auto& asig : dia.asignaciones) {
                if (asig.profesor == 0) slotsA++;
                else if (asig.profesor == 1) slotsB++;
            }
        }
        QCOMPARE(slotsA, 0);
        QCOMPARE(slotsB, 4);
    }
};

#include "test_solver_horarios.moc"
QTEST_MAIN(TestSolverHorarios)
