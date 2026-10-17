#include <QTest>
#include <QElapsedTimer>
#include <backend/solver/config/solver_config.hpp>
#include <backend/solver/solver.hpp>

class TestSolverBenchmark : public QObject {
    Q_OBJECT

private:
    SolverConfig crearConfigGrande() {
        SolverConfig config;
        config.version = "1.0";
        config.aula_fija = true;

        // 10 profesores, 15 materias, 5 aulas, 8 cursos, 5 dias, 6 slots/dia
        config.dimensiones = {10, 15, 5, 8, 5, 6};
        config.franja_horaria = {60, 6};
        config.turnos.turnos = {
            {"manana", {0, 1, 2}},
            {"tarde", {3, 4, 5}}
        };

        // 15 materias
        config.materias = {
            {"Matematica"}, {"Fisica"}, {"Quimica"}, {"Biologia"}, {"Ciencias General"},
            {"Historia"}, {"Geografia"}, {"Literatura"}, {"Filosofia"}, {"Arte"},
            {"Ingles"}, {"Frances"}, {"Portugues"}, {"Educacion Fisica"}, {"Tecnologia"}
        };

        // 5 aulas
        config.aulas = {
            {"Aula 101", "regular", 30},
            {"Aula 102", "regular", 25},
            {"Lab Ciencias", "laboratorio", 20},
            {"Aula 201", "regular", 35},
            {"Auditorio", "auditorio", 100}
        };

        // 8 cursos: 4 manana, 4 tarde
        // Cursos por identidad (anio+seccion); aula = indice (modo fijo).
        config.cursos = {
            {1, "A", "manana", 25, "General",  0},
            {1, "B", "manana", 20, "General",  1},
            {2, "A", "manana", 22, "Ciencias", 2},
            {2, "B", "tarde",  18, "General",  0},
            {3, "A", "manana", 20, "Ciencias", 3},
            {3, "B", "tarde",  15, "General",  1},
            {4, "A", "manana", 20, "General",  4},
            {4, "B", "tarde",  18, "Ciencias", 2}
        };

        // Planes embebidos: materias {materia_idx, curso(anio), horas}.
        config.planes = {
            {"General", {{0,1,4},{5,1,3},{10,1,3},
                         {5,2,4},{7,2,3},{10,2,3},
                         {6,3,3},{8,3,3},{12,3,3},
                         {5,4,3},{7,4,3},{13,4,3}}},
            {"Ciencias", {{0,2,4},{1,2,3},{2,2,3},{3,2,2},
                          {0,3,3},{1,3,3},{2,3,3},{4,3,2},
                          {0,4,3},{3,4,3},{14,4,3}}}
        };

        // 10 profesores con disponibilidad completa
        QVector<Disponibilidad> dispCompleta;
        for (int d = 0; d < 5; d++) {
            dispCompleta.append({d, {0, 1, 2, 3, 4, 5}});
        }

        config.profesores = {
            {"Prof 01", 16, 16, "manana", "General", {},   {0, 5},    {}, dispCompleta},
            {"Prof 02", 14, 14, "manana", "Ciencias", {},  {0, 1},    {}, dispCompleta},
            {"Prof 03", 14, 14, "manana", "Ciencias", {},  {2, 3, 4}, {}, dispCompleta},
            {"Prof 04", 12, 12, "",       "", {},          {5, 6},    {}, dispCompleta},
            {"Prof 05", 14, 14, "",       "", {},          {7, 8},    {}, dispCompleta},
            {"Prof 06", 12, 12, "tarde",  "General", {},   {10, 11},  {}, dispCompleta},
            {"Prof 07", 12, 12, "",       "", {},          {10, 12},  {}, dispCompleta},
            {"Prof 08", 10, 10, "",       "", {},          {13, 14},  {}, dispCompleta},
            {"Prof 09", 14, 14, "manana", "General", {},   {0, 5, 6}, {}, dispCompleta},
            {"Prof 10", 14, 14, "tarde",  "Ciencias", {},  {0, 1, 3, 4}, {}, dispCompleta}
        };

        return config;
    }

private slots:
    void benchmark_solverGrandeResuelve() {
        SolverConfig config = crearConfigGrande();

        QElapsedTimer timer;
        timer.start();

        Solver solver;
        auto resultado = solver.resolver(config);

        qint64 elapsed = timer.elapsed();

        qDebug() << "=== BENCHMARK ===";
        qDebug() << "Dimensiones: 10 prof, 15 materias, 5 aulas, 8 cursos, 5 dias, 6 slots";
        qDebug() << "Tiempo total:" << elapsed << "ms";
        qDebug() << "Status:" << (resultado.exito ? "EXITO" : "FALLO");
        if (resultado.exito) {
            int totalAsig = 0;
            for (const auto& curso : resultado.resultado.horarios) {
                for (const auto& dia : curso.dias) {
                    totalAsig += dia.asignaciones.size();
                }
            }
            qDebug() << "Total asignaciones:" << totalAsig;
        } else {
            qDebug() << "Errores:" << resultado.errores;
        }

        QVERIFY(resultado.exito);
        QVERIFY(elapsed < 60000); // < 60 segundos
    }

    void benchmark_verificaConsistencia() {
        SolverConfig config = crearConfigGrande();
        Solver solver;
        auto resultado = solver.resolver(config);
        QVERIFY(resultado.exito);

        // Verificar que ningun profesor tiene 2 clases solapadas
        QMap<QPair<int, int>, QVector<int>> profDiaSlots;
        for (const auto& curso : resultado.resultado.horarios) {
            for (const auto& dia : curso.dias) {
                for (const auto& asig : dia.asignaciones) {
                    auto key = qMakePair(asig.profesor, dia.dia);
                    QVERIFY(!profDiaSlots[key].contains(asig.slot));
                    profDiaSlots[key].append(asig.slot);
                }
            }
        }

        // Verificar que ningun aula tiene 2 clases solapadas
        QMap<QPair<int, int>, QVector<int>> aulaDiaSlots;
        for (const auto& curso : resultado.resultado.horarios) {
            for (const auto& dia : curso.dias) {
                for (const auto& asig : dia.asignaciones) {
                    auto key = qMakePair(asig.aula, dia.dia);
                    QVERIFY(!aulaDiaSlots[key].contains(asig.slot));
                    aulaDiaSlots[key].append(asig.slot);
                }
            }
        }
    }
};

#include "test_solver_benchmark.moc"
QTEST_MAIN(TestSolverBenchmark)
