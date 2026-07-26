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
};

#include "test_solver_horarios.moc"
QTEST_MAIN(TestSolverHorarios)
