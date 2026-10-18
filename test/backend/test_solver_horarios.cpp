#include <QTest>
#include <QSet>
#include <QPair>
#include <QMap>
#include <backend/solver/config/contrato_solver.hpp>
#include <backend/data/horario_salida.hpp>
#include <backend/solver/config/solver_config.hpp>
#include <backend/solver/solver.hpp>

/// Helper: franjas en las que se imparte una materia en un curso.
static QVector<int> franjasDeMateria(const CursoOutput& curso, int materia) {
    QVector<int> franjas;
    for (const auto& dia : curso.dias)
        for (const auto& asig : dia.asignaciones)
            if (asig.materia == materia) franjas.append(asig.slot);
    std::sort(franjas.begin(), franjas.end());
    return franjas;
}

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
        config.dimensiones = {1, 2, 2, 2, 3, 4};   // 1 docente, 2 materias, 2 aulas, 2 cursos

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY(!resultado.exito);
        QVERIFY(!resultado.errores.isEmpty());
        QVERIFY(resultado.errores[0].contains("INFEASIBLE"));
        // RF-6: la causa es un conflicto de asignación de docente (D3): el único docente
        // elegible solo dispone de 2 franjas para una materia de 4 bloques.
        const QString texto = resultado.errores.join("\n");
        QVERIFY2(texto.contains("Causa (4)"), qPrintable(texto));
    }

    // ── RF-6/RNF-3: al no ser factible, se informa la PRIMERA categoría de causa ──

    void solver_falloCausaEntradaInvalida() {
        // Curso sin turno declarado: configuración inválida → categoría (1) entrada inválida.
        SolverConfig config;
        config.version = "1.0";
        config.aula_fija = true;
        config.dimensiones = {1, 1, 1, 1, 1, 2};
        config.franja_horaria = {60, 2};
        config.turnos.turnos = {{"manana", {0, 1}}};
        config.materias = {{"Matematica"}};
        config.aulas = {{"Aula 1", "regular", 30}};
        config.planes = {{"Plan General", {{0, 1, 2}}}};
        config.cursos = {{1, "A", "", 20, "Plan General", 0}};   // sin turno
        config.profesores = {
            {"Prof A", 2, 2, "", "Plan General", {}, {0}, {}, {{0, {0, 1}}}}};

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY(!resultado.exito);
        const QString texto = resultado.errores.join("\n");
        QVERIFY2(texto.contains("Causa (1)"), qPrintable(texto));
        QVERIFY2(texto.contains("turno"), qPrintable(texto));
    }

    void solver_falloCausaConflictoDeAula() {
        // Dos cursos que comparten el aula fija y, juntos, piden más bloques que franjas tiene
        // el aula (cada uno cabe por separado): la causa es el aula → categoría (2).
        SolverConfig config;
        config.version = "1.0";
        config.aula_fija = true;
        config.dimensiones = {2, 1, 1, 2, 1, 4};
        config.franja_horaria = {60, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};
        config.materias = {{"Matematica"}};
        config.aulas = {{"Aula compartida", "regular", 30}};
        config.planes = {{"Plan General", {{0, 1, 4}, {0, 2, 4}}}};   // 4 bloques por curso
        config.cursos = {
            {1, "A", "manana", 20, "Plan General", 0},
            {2, "A", "manana", 20, "Plan General", 0}};                // MISMA aula
        config.profesores = {
            {"Prof A", 4, 4, "manana", "Plan General", {}, {0}, {}, {{0, {0, 1, 2, 3}}}},
            {"Prof B", 4, 4, "manana", "Plan General", {}, {0}, {}, {{0, {0, 1, 2, 3}}}}};

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY(!resultado.exito);
        const QString texto = resultado.errores.join("\n");
        QVERIFY2(texto.contains("Causa (2)"), qPrintable(texto));
        QVERIFY2(texto.contains("Aula compartida"), qPrintable(texto));
    }

    void solver_falloCausaConflictoDePersona() {
        // Un curso cuyo plan pide más bloques que franjas ofrece su turno → categoría (3).
        SolverConfig config;
        config.version = "1.0";
        config.aula_fija = true;
        config.dimensiones = {1, 1, 1, 1, 1, 2};
        config.franja_horaria = {60, 2};
        config.turnos.turnos = {{"manana", {0, 1}}};
        config.materias = {{"Matematica"}};
        config.aulas = {{"Aula 1", "regular", 30}};
        config.planes = {{"Plan General", {{0, 1, 4}}}};   // 4 bloques en un turno de 2 franjas
        config.cursos = {{1, "A", "manana", 20, "Plan General", 0}};
        config.profesores = {
            {"Prof A", 4, 4, "manana", "Plan General", {}, {0}, {}, {{0, {0, 1}}}}};

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY(!resultado.exito);
        const QString texto = resultado.errores.join("\n");
        QVERIFY2(texto.contains("Causa (3)"), qPrintable(texto));
        QVERIFY2(texto.contains("1|A"), qPrintable(texto));
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

    // ── Rectificación: D3 asigna exactamente horas_m bloques y S1 compara con horas_aula ──
    // Con duracion_minutos = 45, la conversión antigua pediría ceil(2×60/45) = 3 bloques y
    // ceil(2×60/45) = 3 de tope, volviendo el problema infeasible (solo hay 2 franjas).
    // Con la identidad (1 hora académica = 1 bloque), una materia de 2 horas ocupa exactamente
    // 2 bloques y el tope del docente es 2 (sin ×60), luego hay solución con 2 bloques.
    void solver_d3_bloquesSonHoras_s1_topeSinConversion() {
        SolverConfig config;
        config.version = "1.0";
        config.aula_fija = true;
        config.dimensiones = {1, 1, 1, 1, 1, 2};   // 1 docente, 1 materia, 1 aula, 1 curso, 1 día, 2 franjas
        config.franja_horaria = {45, 2};
        config.turnos.turnos = {{"manana", {0, 1}}};
        config.materias = {{"Matematica"}};
        config.aulas = {{"Aula 1", "regular", 30}};
        config.planes = {{"Plan General", {{0, 1, 2}}}};   // materia 0, 2 horas académicas
        config.cursos = {{1, "A", "manana", 20, "Plan General", 0}};
        config.profesores = {
            {"Prof A", 2, 2, "manana", "Plan General", {}, {0}, {}, {{0, {0, 1}}}}};

        Solver solver;
        auto resultado = solver.resolver(config);
        QVERIFY2(resultado.exito, qPrintable(resultado.errores.join("\n")));

        const auto& curso = resultado.resultado.horarios[claveCurso(1, "A")];
        int total = 0;
        for (const auto& dia : curso.dias)
            total += dia.asignaciones.size();
        QCOMPARE(total, 2);   // exactamente 2 bloques (no 3)
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

    // ── T5: Agrupación de bloques (S3) ──

    // AC-3: una materia de 2 h en un solo día debe ir en 2 franjas CONTIGUAS,
    // no en 2 grupos de 1 (la penalización de S3 desincentiva el segundo arranque).
    void solver_agrupacion_dosHorasContiguas() {
        SolverConfig config;
        config.version = "1.0";
        config.aula_fija = true;
        config.dimensiones = {1, 1, 1, 1, 1, 4};   // 1 día, 4 franjas
        config.franja_horaria = {40, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};
        config.materias = {{"Matematica"}};
        config.aulas = {{"Aula 1", "regular", 30}};
        config.planes = {{"Plan General", {{0, 1, 2}}}};   // 2 horas
        config.cursos = {{1, "A", "manana", 20, "Plan General", 0}};
        config.profesores = {
            {"Prof A", 2, 2, "manana", "Plan General", {}, {0}, {}, {{0, {0, 1, 2, 3}}}}};

        Solver solver;
        auto resultado = solver.resolver(config);
        QVERIFY2(resultado.exito, qPrintable(resultado.errores.join("\n")));

        const auto& curso = resultado.resultado.horarios[claveCurso(1, "A")];
        const QVector<int> franjas = franjasDeMateria(curso, 0);
        QCOMPARE(franjas.size(), 2);
        // Contiguas: la diferencia entre los dos slots debe ser 1.
        QCOMPARE(franjas[1] - franjas[0], 1);
    }

    // AC-9: el receso rompe la contigüidad. Con un receso entre las franjas 1 y 2,
    // una materia de 2 h debe ir en {0,1} o {2,3}, nunca en {1,2}.
    void solver_agrupacion_recesoRompeContiguedad() {
        SolverConfig config;
        config.version = "1.0";
        config.aula_fija = true;
        config.dimensiones = {1, 1, 1, 1, 1, 4};
        config.franja_horaria = {40, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};
        config.recesos = {{"manana", 1, 10, "08:20", "08:30"}};  // rompe entre 1 y 2
        config.materias = {{"Matematica"}};
        config.aulas = {{"Aula 1", "regular", 30}};
        config.planes = {{"Plan General", {{0, 1, 2}}}};
        config.cursos = {{1, "A", "manana", 20, "Plan General", 0}};
        config.profesores = {
            {"Prof A", 2, 2, "manana", "Plan General", {}, {0}, {}, {{0, {0, 1, 2, 3}}}}};

        Solver solver;
        auto resultado = solver.resolver(config);
        QVERIFY2(resultado.exito, qPrintable(resultado.errores.join("\n")));

        const auto& curso = resultado.resultado.horarios[claveCurso(1, "A")];
        const QVector<int> franjas = franjasDeMateria(curso, 0);
        QCOMPARE(franjas.size(), 2);
        // No deben ser {1,2} (cruzando el receso). Válidas: {0,1} o {2,3}.
        const bool mismoSegmento = (franjas[0] == 0 && franjas[1] == 1) ||
                                   (franjas[0] == 2 && franjas[1] == 3);
        QVERIFY2(mismoSegmento,
                 qPrintable(QString("Slots %1,%2 cruzan el receso").arg(franjas[0]).arg(franjas[1])));
    }

    // AC-4: una materia de 4 h en un día de 4 franjas ocupa 4 contiguas (un solo grupo).
    void solver_agrupacion_cuatroHorasUnGrupo() {
        SolverConfig config;
        config.version = "1.0";
        config.aula_fija = true;
        config.dimensiones = {1, 1, 1, 1, 1, 4};
        config.franja_horaria = {40, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};
        config.materias = {{"Matematica"}};
        config.aulas = {{"Aula 1", "regular", 30}};
        config.planes = {{"Plan General", {{0, 1, 4}}}};
        config.cursos = {{1, "A", "manana", 20, "Plan General", 0}};
        config.profesores = {
            {"Prof A", 4, 4, "manana", "Plan General", {}, {0}, {}, {{0, {0, 1, 2, 3}}}}};

        Solver solver;
        auto resultado = solver.resolver(config);
        QVERIFY2(resultado.exito, qPrintable(resultado.errores.join("\n")));

        const auto& curso = resultado.resultado.horarios[claveCurso(1, "A")];
        const QVector<int> franjas = franjasDeMateria(curso, 0);
        QCOMPARE(franjas.size(), 4);
        QCOMPARE(franjas, (QVector<int>{0, 1, 2, 3}));
    }
};

QTEST_MAIN(TestSolverHorarios)
#include "test_solver_horarios.moc"
