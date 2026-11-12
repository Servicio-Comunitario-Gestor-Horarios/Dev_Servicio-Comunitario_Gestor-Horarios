#include <QTest>
#include <QElapsedTimer>
#include <backend/solver/config/solver_config.hpp>
#include <backend/solver/solver.hpp>

class TestSolverBenchmark : public QObject {
    Q_OBJECT

private:
    SolverConfig crearConfigGrande() {
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = true;

        // 10 profesores, 15 materias, 6 aulas, 8 cursos, 5 dias, 6 slots/dia
        config.dimensiones = {10, 15, 6, 8, 5, 6};
        config.franja_horaria = {60, 6};
        config.turnos.turnos = {
            {"manana", {0, 1, 2}},
            {"tarde", {3, 4, 5}}
        };

        // 15 materias: solo Tecnologia (indice 14) exige laboratorio (plan 010, §1.3)
        config.materias = {
            {"Matematica", "Aula común"}, {"Fisica", "Aula común"},
            {"Quimica", "Aula común"}, {"Biologia", "Aula común"},
            {"Ciencias General", "Aula común"}, {"Historia", "Aula común"},
            {"Geografia", "Aula común"}, {"Literatura", "Aula común"},
            {"Filosofia", "Aula común"}, {"Arte", "Aula común"},
            {"Ingles", "Aula común"}, {"Frances", "Aula común"},
            {"Portugues", "Aula común"}, {"Educacion Fisica", "Aula común"},
            {"Tecnologia", "Laboratorio"}
        };

        // 6 aulas: 0-4 comunes, 5 laboratorio (plan 010, §1.3)
        config.aulas = {
            {"Aula 101", "Aula común", 30},
            {"Aula 102", "Aula común", 25},
            {"Aula 202", "Aula común", 35},
            {"Aula 201", "Aula común", 30},
            {"Aula 203", "Aula común", 100},
            {"Laboratorio", "Laboratorio", 20}
        };

        // 8 cursos: 4 manana, 4 tarde
        // Cursos por identidad (anio+seccion); aula = referencia (modo fijo).
        // Referencias por turno sin repetir aula: manana 1A->0, 1B->1, 2A->2, 3A->3, 4A->4;
        // tarde 2B->0, 3B->1, 4B->2. Solo 4B (Ciencias) desplaza Tecnologia al laboratorio.
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
        // Ajuste T3a: cada curso suma 10 h (par) y sus turnos tienen 3 franjas/día, así que
        // cada día lleva 2 clases (tramo par). Antes había totales impares (9 y 11) y 12 h en
        // un turno de 3 franjas, que el emparejamiento duro ya no admite.
        // Ajuste T3b (R7): Tecnología es la única materia de laboratorio; sus horas deben ser
        // pares para poder formar pares del mismo tipo (3 h no tiene con quién compartir).
        config.planes = {
            {"General", {{0,1,4},{5,1,3},{10,1,3},
                         {5,2,4},{7,2,3},{10,2,3},
                         {6,3,4},{8,3,3},{12,3,3},
                         {5,4,4},{7,4,3},{13,4,3}}},
            {"Ciencias", {{0,2,4},{1,2,2},{2,2,2},{3,2,2},
                          {0,3,3},{1,3,3},{2,3,2},{4,3,2},
                          {0,4,4},{3,4,4},{14,4,2}}}
        };

        // 10 profesores con disponibilidad completa
        QVector<Disponibilidad> dispCompleta;
        for (int d = 0; d < 5; d++) {
            dispCompleta.append({d, {0, 1, 2, 3, 4, 5}});
        }

        config.profesores = {
            {"Prof 01", 16, 16, "",       "General", {},   {0, 5},    {}, dispCompleta},
            {"Prof 02", 14, 14, "",       "Ciencias", {},  {0, 1},    {}, dispCompleta},
            {"Prof 03", 14, 14, "",       "Ciencias", {},  {2, 3, 4}, {}, dispCompleta},
            {"Prof 04", 12, 12, "",       "", {},          {5, 6},    {}, dispCompleta},
            {"Prof 05", 14, 14, "",       "", {},          {7, 8},    {}, dispCompleta},
            {"Prof 06", 12, 12, "",       "General", {},   {10, 11},  {}, dispCompleta},
            {"Prof 07", 12, 12, "",       "", {},          {10, 12},  {}, dispCompleta},
            {"Prof 08", 10, 10, "",       "", {},          {13, 14},  {}, dispCompleta},
            {"Prof 09", 14, 14, "",       "General", {},   {0, 5, 6}, {}, dispCompleta},
            {"Prof 10", 14, 14, "",       "Ciencias", {},  {0, 1, 3, 4}, {}, dispCompleta}
        };

        return config;
    }

    /// La misma configuración grande con preferencias proyectadas: cada docente evita la mañana
    /// del primer día y prefiere la tarde del último (013, escenario de RNF-3).
    SolverConfig crearConfigGrandeConPreferencias() {
        SolverConfig config = crearConfigGrande();
        for (int p = 0; p < config.profesores.size(); ++p) {
            PreferenciaSolverConfig evitada;
            evitada.docente = p;
            evitada.dia = 0;
            evitada.slot = {0, 1, 2};
            evitada.sentido = QStringLiteral("evitada");
            config.preferencias.append(evitada);

            PreferenciaSolverConfig preferida;
            preferida.docente = p;
            preferida.dia = 4;
            preferida.slot = {3, 4, 5};
            preferida.sentido = QStringLiteral("preferida");
            config.preferencias.append(preferida);
        }
        return config;
    }

    /// Resumen determinista de una salida: curso|día|slot|materia|profesor|aula, ordenado.
    static QString resumenDeSalida(const Solver::ResultadoSolver& resultado) {
        QStringList clases;
        for (auto it = resultado.resultado.horarios.constBegin();
             it != resultado.resultado.horarios.constEnd(); ++it)
            for (const DiaOutput& dia : it.value().dias)
                for (const AsignacionOutput& asignacion : dia.asignaciones)
                    clases.append(QStringLiteral("%1|%2|%3|%4|%5|%6")
                                      .arg(it.key())
                                      .arg(dia.dia)
                                      .arg(asignacion.slot)
                                      .arg(asignacion.materia)
                                      .arg(asignacion.profesor)
                                      .arg(asignacion.aula));
        clases.sort();
        return clases.join(QLatin1Char('\n'));
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
        qDebug() << "Dimensiones: 10 prof, 15 materias, 6 aulas, 8 cursos, 5 dias, 6 slots";
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
        QVERIFY(elapsed < 60000); // techo histórico del benchmark (no es el plazo del servicio: 300 s)
    }

    void benchmark_SolverGrandeConPreferencias() {
        Solver solver;

        // Referencia: la misma entrada sin preferencias (RNF-3).
        const SolverConfig referenciaConfig = crearConfigGrande();
        QElapsedTimer timer;
        timer.start();
        const auto referencia = solver.resolver(referenciaConfig);
        const qint64 msReferencia = timer.elapsed();
        QVERIFY2(referencia.exito, qPrintable(referencia.errores.join("\n")));

        // Con preferencias (fase 1 + 2a + 2b).
        const SolverConfig config = crearConfigGrandeConPreferencias();
        timer.restart();
        const auto primera = solver.resolver(config);
        const qint64 msPrimera = timer.elapsed();
        timer.restart();
        const auto segunda = solver.resolver(config);
        const qint64 msSegunda = timer.elapsed();

        QVERIFY2(primera.exito, qPrintable(primera.errores.join("\n")));
        QVERIFY2(segunda.exito, qPrintable(segunda.errores.join("\n")));

        // 014b: con 8 workers no se promete el mismo horario; ambas ejecuciones deben ser
        // válidas y cubrir el mismo número de clases.
        QCOMPARE(segunda.resultado.metadata.total_asignaciones,
                 primera.resultado.metadata.total_asignaciones);

        // Techo histórico del benchmark: 60 s (no es el plazo del servicio, 300 s). El +50 %
        // se documenta, no se aserta: depende de la máquina.
        QVERIFY2(msPrimera < 60000,
                 qPrintable(QStringLiteral("la primera ejecución tardó %1 ms").arg(msPrimera)));
        QVERIFY2(msSegunda < 60000,
                 qPrintable(QStringLiteral("la segunda ejecución tardó %1 ms").arg(msSegunda)));

        const double sobrecoste = msReferencia > 0
                                      ? 100.0 * (msPrimera - msReferencia) / msReferencia
                                      : 0.0;
        qDebug() << "=== BENCHMARK CON PREFERENCIAS ===";
        qDebug() << "Referencia (sin preferencias):" << msReferencia << "ms";
        qDebug() << "Con preferencias (fase 1 + 2a + 2b):" << msPrimera << "ms /"
                 << msSegunda << "ms";
        qDebug() << "Sobrecoste frente a la referencia:" << sobrecoste
                 << "% (umbral +50 %, se documenta en journal.md)";
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
