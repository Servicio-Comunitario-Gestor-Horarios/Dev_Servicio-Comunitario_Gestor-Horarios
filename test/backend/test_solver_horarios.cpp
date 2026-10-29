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

    // Disponibilidad completa sobre `dias` días × `franjasPorDia` franjas.
    static QVector<Disponibilidad> dispCompleta(int dias, int franjasPorDia) {
        QVector<Disponibilidad> disp;
        for (int d = 0; d < dias; ++d) {
            QVector<int> franjas;
            for (int s = 0; s < franjasPorDia; ++s) franjas.append(s);
            disp.append({d, franjas});
        }
        return disp;
    }

    SolverConfig crearConfigMinima() {
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {2, 2, 2, 1, 3, 4};
        config.franja_horaria = {60, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};

        config.materias = {{"Matematica", "regular"}, {"Historia", "regular"}};
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

    // Fixture compartido por dos tests: dos cursos comparten la referencia A1 y cada uno cursa
    // 2 bloques comunes + 2 de laboratorio. Con 2 laboratorios y un docente por curso el horario
    // existe (control positivo); si el docente de 2|B no opta por el laboratorio, la causa es (4).
    // El conteo del tipo no falla: «Aula común» demanda 4 frente a 1 aula × 6 franjas y
    // «Laboratorio» 4 frente a 2 aulas × 6.
    static SolverConfig configDosCursosDesplazadosALaMismaFranja() {
        SolverConfig config;
        config.version = "2.0";
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
                         {2, "B", "manana", 20, "Plan B", 0}};   // MISMA referencia
        config.profesores = {
            {"Prof A", 6, 6, "manana", "Plan A", {}, {0, 1}, {}, dispCompleta(2, 3)},
            {"Prof B", 6, 6, "manana", "Plan B", {}, {0, 1}, {}, dispCompleta(2, 3)}};
        return config;
    }

    // Caso R2 del reviewer: dos cursos comparten la referencia A1 (5 días × 6 franjas = 30) y
    // cada uno cursa 14 bloques comunes + 2 de laboratorio. Con un solo laboratorio los 4
    // bloques de laboratorio necesitan 4 franjas distintas (una clase por franja y aula), así
    // que A1 pierde 4 franjas y solo le quedan 26 para 28 bloques comunes: infactible por
    // aula. Con 2 laboratorios los mismos 4 bloques caben en 2 franjas (una por aula) y el
    // mismo dato es factible. Cada curso tiene su docente propio y con disponibilidad completa
    // (16 bloques ≤ 30 franjas), así que la causa solo puede ser de aula.
    static SolverConfig configDosCursosCasiLlenosConLaboratorios(int laboratorios) {
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {2, 2, laboratorios + 1, 2, 5, 6};
        config.franja_horaria = {60, 6};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3, 4, 5}}};

        config.materias = {{"Matematica", "Aula común"}, {"Laboratorio", "Laboratorio"}};
        config.aulas = {{"A1", "Aula común", 30}};
        for (int i = 1; i <= laboratorios; ++i)
            config.aulas.append({QStringLiteral("Lab %1").arg(i), "Laboratorio", 20});
        config.planes = {{"Plan A", {{0, 1, 14}, {1, 1, 2}}},
                         {"Plan B", {{0, 2, 14}, {1, 2, 2}}}};
        config.cursos = {{1, "A", "manana", 20, "Plan A", 0},
                         {2, "B", "manana", 20, "Plan B", 0}};   // MISMA referencia
        config.profesores = {
            {"Prof A", 16, 16, "manana", "Plan A", {}, {0, 1}, {}, dispCompleta(5, 6)},
            {"Prof B", 16, 16, "manana", "Plan B", {}, {0, 1}, {}, dispCompleta(5, 6)}};
        return config;
    }

    // Fixture compartido por dos tests: «Aula base» se mide con las franjas de los turnos de los
    // cursos que lo REQUIEREN (2|B y 3|C, turno TB = {0..3}), mientras que 1|A solo cursa
    // laboratorio y va en TL = {4,5}. Los 8 bloques de «Aula base» caben en 2 aulas × 4 franjas
    // (8 = 8) y los 2 de laboratorio en 1 aula × 2 franjas de TL, así que el conteo no falla; la
    // causa es (4): el docente de laboratorio solo tiene una franja para 2 bloques. (Fixture
    // heredado de la corrección 5, ya histórica: el bloqueo de la referencia no entra en ninguna
    // cuenta.)
    static SolverConfig configLaboratorioEnTurnoDistinto() {
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {3, 2, 3, 3, 1, 6};
        config.franja_horaria = {60, 6};
        config.turnos.turnos = {{"TB", {0, 1, 2, 3}}, {"TL", {4, 5}}};

        config.materias = {{"Ciencias", "Aula base"}, {"Laboratorio", "Laboratorio"}};
        config.aulas = {{"A1", "Aula base", 30}, {"A2", "Aula base", 30},
                        {"L1", "Laboratorio", 20}};
        config.planes = {{"Plan C1", {{1, 1, 2}}},   // 1|A: SOLO laboratorio
                         {"Plan C2", {{0, 2, 4}}},   // 2|B: 4 bloques de «Aula base»
                         {"Plan C3", {{0, 3, 4}}}};  // 3|C: 4 bloques de «Aula base»
        config.cursos = {{1, "A", "TL", 20, "Plan C1", 0},    // referencia A1
                         {2, "B", "TB", 20, "Plan C2", 1},    // referencia A2
                         {3, "C", "TB", 20, "Plan C3", 0}};   // referencia A1
        config.profesores = {
            // Titular de laboratorio con UNA sola franja disponible (slot 4 de TL).
            {"Prof Lab", 2, 2, "", "", {}, {1}, {}, {{0, {4}}}},
            {"Prof B", 4, 4, "", "", {}, {0}, {}, dispCompleta(1, 6)},
            {"Prof C", 4, 4, "", "", {}, {0}, {}, dispCompleta(1, 6)}};
        return config;
    }

    // Fixture compartido por dos tests: dos cursos comparten la referencia A1 pero sus turnos son
    // DISTINTOS (1|A usa T1 = {0,1,2,3}; 2|B usa T2 = {2,3}). El conteo del tipo no falla: «Aula
    // común» demanda 6 frente a 1 aula × 8 franjas de la unión de T1 y T2, y «Laboratorio» demanda
    // 4 frente a 2 aulas × 8. Con el docente de 2|B cubriendo el laboratorio el horario existe; sin
    // él, la causa es (4).
    static SolverConfig configDosCursosConTurnosDistintos() {
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {2, 2, 3, 2, 2, 4};
        config.franja_horaria = {60, 4};
        config.turnos.turnos = {{"T1", {0, 1, 2, 3}}, {"T2", {2, 3}}};

        config.materias = {{"Matematica", "Aula común"}, {"Laboratorio", "Laboratorio"}};
        config.aulas = {{"A1", "Aula común", 30},
                        {"Lab1", "Laboratorio", 20},
                        {"Lab2", "Laboratorio", 20}};
        config.planes = {{"Plan A", {{0, 1, 4}, {1, 1, 2}}},
                         {"Plan B", {{0, 2, 2}, {1, 2, 2}}}};
        config.cursos = {{1, "A", "T1", 20, "Plan A", 0},
                         {2, "B", "T2", 20, "Plan B", 0}};   // MISMA referencia, turnos distintos
        config.profesores = {
            {"Prof A", 6, 6, "T1", "Plan A", {}, {0, 1}, {}, dispCompleta(2, 4)},
            {"Prof B", 4, 4, "T2", "Plan B", {}, {0, 1}, {}, dispCompleta(2, 4)}};
        return config;
    }

    // Fixture de plan.md §0.3: 1 día, 7 franjas, modo móvil, 1 aula `A1` y tres cursos de 1 bloque.
    // 1|A y 2|B solo pueden ir a la celda (0,4) —D8 impide compartirla— y 3|C tiene (0,5) y (0,6).
    // Demanda 3 = capacidad 3 (1 aula × 3 franjas de la unión TB∪TC), así que el conteo del tipo
    // no falla: la infactibilidad es de ventanas entre cursos (condición de Hall), no de capacidad.
    static SolverConfig configVentanasDeTurnoHall() {
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = false;
        config.dimensiones = {3, 1, 1, 3, 1, 7};
        config.franja_horaria = {60, 7};
        config.turnos.turnos = {{"TB", {4}}, {"TC", {5, 6}}};

        config.materias = {{"Matemática", "Aula de estudio"}};
        config.aulas = {{"A1", "Aula de estudio", 30}};
        config.planes = {{"PA", {{0, 1, 1}}},
                         {"PB", {{0, 2, 1}}},
                         {"PC", {{0, 3, 1}}}};
        config.cursos = {{1, "A", "TB", 20, "PA", -1},
                         {2, "B", "TB", 20, "PB", -1},
                         {3, "C", "TC", 20, "PC", -1}};
        config.profesores = {
            // Ninguno declara turno ni plan (vacío = cualquiera); uno por curso, con su celda.
            {"Prof A", 1, 1, "", "", {1}, {0}, {}, {{0, {4}}}},
            {"Prof B", 1, 1, "", "", {2}, {0}, {}, {{0, {4}}}},
            {"Prof C", 1, 1, "", "", {3}, {0}, {}, {{0, {5, 6}}}}};
        return config;
    }

    /// Resumen determinista de una salida: por curso, sus clases (día, slot, materia, profesor,
    /// aula), ordenadas. Vacío si el resultado no tiene solución.
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

    // ── F2: la metadata publica datos reales de la salida, no el número de variables ──
    void solver_metadataCuadraConLaSalida() {
        SolverConfig config = crearConfigMinima();
        Solver solver;
        const auto resultado = solver.resolver(config);
        QVERIFY2(resultado.exito, qPrintable(resultado.errores.join("\n")));

        int clases = 0;
        QSet<int> docentes;
        for (auto it = resultado.resultado.horarios.constBegin();
             it != resultado.resultado.horarios.constEnd(); ++it)
            for (const DiaOutput& dia : it.value().dias)
                for (const AsignacionOutput& asig : dia.asignaciones) {
                    ++clases;
                    if (asig.profesor >= 0)
                        docentes.insert(asig.profesor);
                }

        const MetadataSalida& m = resultado.resultado.metadata;
        QVERIFY(clases > 0);
        // `total_asignaciones` son clases emitidas (2 materias × 4 h = 8), no las variables
        // del modelo (cursos × materias × días × slots).
        QCOMPARE(m.total_asignaciones, clases);
        QCOMPARE(m.profesores_asignados, docentes.size());
        QCOMPARE(m.cursos_generados, config.cursos.size());
        QCOMPARE(m.tiempo_ejecucion_ms, static_cast<int>(resultado.tiempo_ms));
        QVERIFY(m.tiempo_ejecucion_ms >= 0);
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
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {1, 1, 1, 1, 1, 2};
        config.franja_horaria = {60, 2};
        config.turnos.turnos = {{"manana", {0, 1}}};
        config.materias = {{"Matematica", "regular"}};
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
        // Dos cursos que comparten la referencia y, juntos, piden más bloques que la capacidad de
        // su tipo (8 > 1 aula × 4 franjas; cada uno cabe por separado): categoría (2).
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {2, 1, 1, 2, 1, 4};
        config.franja_horaria = {60, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};
        config.materias = {{"Matematica", "regular"}};
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
        QVERIFY2(texto.contains("«regular»"), qPrintable(texto));
    }

    void solver_falloCausaConflictoDePersona() {
        // Un curso cuyo plan pide más bloques que franjas ofrece su turno → categoría (3).
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {1, 1, 1, 1, 1, 2};
        config.franja_horaria = {60, 2};
        config.turnos.turnos = {{"manana", {0, 1}}};
        config.materias = {{"Matematica", "regular"}};
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
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {1, 1, 1, 1, 3, 8};
        config.franja_horaria = {60, 8};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}, {"tarde", {4, 5, 6, 7}}};
        config.materias = {{"Matematica", "regular"}};
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
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {2, 1, 2, 1, 3, 4};
        config.franja_horaria = {60, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};
        config.materias = {{"Matematica", "regular"}};
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
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {1, 1, 1, 1, 5, 4};
        config.franja_horaria = {60, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};
        config.materias = {{"Matematica", "regular"}};
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
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {1, 1, 1, 1, 1, 2};   // 1 docente, 1 materia, 1 aula, 1 curso, 1 día, 2 franjas
        config.franja_horaria = {45, 2};
        config.turnos.turnos = {{"manana", {0, 1}}};
        config.materias = {{"Matematica", "regular"}};
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
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {2, 1, 1, 1, 3, 4};
        config.franja_horaria = {60, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};
        config.materias = {{"Matematica", "regular"}};
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

    // ── S1: el exceso sobre horas_aula es la válvula cuando no hay capacidad ──
    void solver_excesoDeHorasEsLaValvulaSinCapacidad() {
        // Misma configuración salvo la capacidad del docente: con 4 h cubre los 4 bloques sin
        // exceso; con 2 h el exceso se permite y el horario sigue siendo factible. La
        // penalización (PESO_HORAS) la demuestra S2 (`solver_titularAlTope_usaAlSuplente`).
        const auto resolverConCapacidad = [](int horasAula) {
            SolverConfig config;
            config.version = "2.0";
            config.aula_fija = true;
            config.dimensiones = {1, 1, 1, 1, 3, 4};
            config.franja_horaria = {60, 4};
            config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};
            config.materias = {{"Matematica", "regular"}};
            config.aulas = {{"Aula 1", "regular", 30}};
            config.planes = {{"Plan General", {{0, 1, 4}}}};
            config.cursos = {{1, "A", "manana", 20, "Plan General", 0}};
            config.profesores = {
                {"Prof A", 4, horasAula, "manana", "Plan General", {}, {0}, {}, dispCompleta()}};

            Solver solver;
            return solver.resolver(config);
        };

        const auto suficiente = resolverConCapacidad(4);
        QVERIFY2(suficiente.exito, qPrintable(suficiente.errores.join("\n")));
        int clasesSuficiente = 0;
        for (const auto& dia : suficiente.resultado.horarios[claveCurso(1, "A")].dias)
            clasesSuficiente += dia.asignaciones.size();
        QCOMPARE(clasesSuficiente, 4);

        const auto justa = resolverConCapacidad(2);
        QVERIFY2(justa.exito, qPrintable(justa.errores.join("\n")));
        int clasesJusta = 0;
        for (const auto& dia : justa.resultado.horarios[claveCurso(1, "A")].dias)
            clasesJusta += dia.asignaciones.size();
        QCOMPARE(clasesJusta, 4);
    }

    // ── S2: un titular con capacidad agotada deja la materia al suplente ──
    void solver_titularAlTope_usaAlSuplente() {
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {2, 1, 1, 1, 3, 4};
        config.franja_horaria = {60, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};
        config.materias = {{"Matematica", "regular"}};
        config.aulas = {{"Aula 1", "regular", 30}};
        config.planes = {{"Plan General", {{0, 1, 4}}}};
        config.cursos = {{1, "A", "manana", 20, "Plan General", 0}};
        config.profesores = {
            // Titular con 2 h de aula para 4 bloques: cubrirlo costaría exceso (PESO_HORAS).
            {"Prof Titular", 4, 2, "manana", "Plan General", {}, {0}, {}, dispCompleta()},
            // Suplente elegible con capacidad suficiente: cuesta COSTE_SUPLENTE una vez.
            {"Prof Suplente", 4, 4, "manana", "Plan General", {}, {}, {{0, 10}},
             dispCompleta()}};

        Solver solver;
        const auto resultado = solver.resolver(config);
        QVERIFY2(resultado.exito, qPrintable(resultado.errores.join("\n")));

        int delTitular = 0, delSuplente = 0;
        for (const auto& dia : resultado.resultado.horarios[claveCurso(1, "A")].dias)
            for (const auto& asig : dia.asignaciones) {
                if (asig.profesor == 0) ++delTitular;
                if (asig.profesor == 1) ++delSuplente;
            }
        QCOMPARE(delTitular, 0);
        QCOMPARE(delSuplente, 4);
    }

    // ── T4/RF-3: el aula de referencia se usa si su tipo coincide con el de la materia ──
    void solver_aulaFijaUsaLaReferenciaSiElTipoCoincide() {
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {1, 1, 2, 1, 3, 4};
        config.franja_horaria = {60, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};

        config.materias = {{"Matematica", "Aula de estudio"}};
        // Dos aulas del MISMO tipo: la clave normalizada ignora espacios y mayúsculas.
        config.aulas = {{"Aula 1", "Aula de estudio", 30}, {"Aula 2", " aula de ESTUDIO ", 25}};
        config.planes = {{"Plan General", {{0, 1, 4}}}};
        config.cursos = {{1, "A", "manana", 20, "Plan General", 0}};   // referencia = Aula 1
        config.profesores = {
            {"Prof A", 4, 4, "manana", "Plan General", {}, {0}, {}, dispCompleta(3, 4)}};

        Solver solver;
        auto resultado = solver.resolver(config);
        QVERIFY2(resultado.exito, qPrintable(resultado.errores.join("\n")));

        int clases = 0;
        for (const auto& dia : resultado.resultado.horarios[claveCurso(1, "A")].dias)
            for (const auto& asig : dia.asignaciones) {
                QCOMPARE(asig.aula, 0);   // la referencia, no la otra aula del mismo tipo
                ++clases;
            }
        QCOMPARE(clases, 4);
    }

    // ── T4/RF-3: con tipo coincidente la clase va a la referencia, no a otra aula del tipo ──
    void solver_aulaFijaFuerzaLaReferenciaFrenteAOtraAulaDelMismoTipo() {
        // Dos cursos comparten las 2 franjas del turno y cada uno tiene 2 clases: solo caben
        // si cada uno usa SU referencia (1|A → Aula B, 2|B → Aula A), no la del vecino.
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {2, 1, 2, 2, 1, 2};
        config.franja_horaria = {60, 2};
        config.turnos.turnos = {{"manana", {0, 1}}};

        config.materias = {{"Matematica", "Aula de estudio"}};
        config.aulas = {{"Aula A", "Aula de estudio", 30}, {"Aula B", "Aula de estudio", 25}};
        config.planes = {{"Plan A", {{0, 1, 2}}}, {"Plan B", {{0, 2, 2}}}};
        config.cursos = {{1, "A", "manana", 20, "Plan A", 1},    // referencia = Aula B
                         {2, "B", "manana", 20, "Plan B", 0}};   // referencia = Aula A
        config.profesores = {
            {"Prof A", 2, 2, "", "", {}, {0}, {}, dispCompleta(1, 2)},
            {"Prof B", 2, 2, "", "", {}, {0}, {}, dispCompleta(1, 2)}};

        Solver solver;
        auto resultado = solver.resolver(config);
        QVERIFY2(resultado.exito, qPrintable(resultado.errores.join("\n")));

        for (const auto& dia : resultado.resultado.horarios[claveCurso(1, "A")].dias)
            for (const auto& asig : dia.asignaciones)
                QCOMPARE(asig.aula, 1);
        for (const auto& dia : resultado.resultado.horarios[claveCurso(2, "B")].dias)
            for (const auto& asig : dia.asignaciones)
                QCOMPARE(asig.aula, 0);
    }

    // ── T4/RF-3 y RF-4: la materia de otro tipo se desplaza y el curso vuelve ──
    void solver_materiaDeOtroTipoSeDesplazaYVuelve() {
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {1, 2, 2, 1, 3, 4};
        config.franja_horaria = {60, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};

        config.materias = {{"Matematica", "Aula de estudio"}, {"Laboratorio", "Laboratorio"}};
        config.aulas = {{"Aula 1", "Aula de estudio", 30}, {"Lab 1", "Laboratorio", 20}};
        config.planes = {{"Plan General", {{0, 1, 4}, {1, 1, 2}}}};
        config.cursos = {{1, "A", "manana", 20, "Plan General", 0}};
        config.profesores = {
            {"Prof A", 6, 6, "manana", "Plan General", {}, {0, 1}, {}, dispCompleta(3, 4)}};

        Solver solver;
        auto resultado = solver.resolver(config);
        QVERIFY2(resultado.exito, qPrintable(resultado.errores.join("\n")));

        int enReferencia = 0, desplazadas = 0;
        QSet<int> aulasUsadas;
        for (const auto& dia : resultado.resultado.horarios[claveCurso(1, "A")].dias)
            for (const auto& asig : dia.asignaciones) {
                aulasUsadas.insert(asig.aula);
                if (asig.materia == 0) {
                    QCOMPARE(asig.aula, 0);   // vuelve a la referencia
                    ++enReferencia;
                } else {
                    QCOMPARE(asig.materia, 1);
                    QCOMPARE(asig.aula, 1);   // aula real del tipo requerido (RF-4)
                    ++desplazadas;
                }
            }
        QCOMPARE(enReferencia, 4);
        QCOMPARE(desplazadas, 2);
        // La salida informa aulas distintas para el mismo curso (no la referencia siempre).
        QCOMPARE(aulasUsadas.size(), 2);
    }

    // ── T4/RF-3: mientras hay una clase desplazada, la referencia queda vacía y bloqueada ──
    void solver_referenciaBloqueadaMientrasHayClaseDesplazada() {
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {2, 2, 3, 2, 3, 4};
        config.franja_horaria = {60, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};

        config.materias = {{"Matematica", "Aula de estudio"}, {"Laboratorio", "Laboratorio"}};
        config.aulas = {{"Aula 1", "Aula de estudio", 30},
                        {"Aula 2", "Aula de estudio", 25},
                        {"Lab", "Laboratorio", 20}};
        config.planes = {{"Plan A", {{0, 1, 4}, {1, 1, 2}}}, {"Plan B", {{0, 2, 4}}}};
        config.cursos = {{1, "A", "manana", 20, "Plan A", 0},     // referencia = Aula 1
                         {2, "B", "manana", 20, "Plan B", 0}};    // comparte la referencia
        config.profesores = {
            {"Prof A", 6, 6, "", "", {}, {0, 1}, {}, dispCompleta(3, 4)},
            {"Prof B", 4, 4, "", "", {}, {0}, {}, dispCompleta(3, 4)}};

        Solver solver;
        auto resultado = solver.resolver(config);
        QVERIFY2(resultado.exito, qPrintable(resultado.errores.join("\n")));

        // Franjas en las que 1|A tiene una clase desplazada al laboratorio.
        QSet<QPair<int, int>> desplazadas;
        for (const auto& dia : resultado.resultado.horarios[claveCurso(1, "A")].dias)
            for (const auto& asig : dia.asignaciones)
                if (asig.materia == 1) desplazadas.insert(qMakePair(dia.dia, asig.slot));
        QVERIFY2(!desplazadas.isEmpty(), "el fixture debe producir clases desplazadas");

        // ...y en ellas el aula de referencia (Aula 1) no aloja ninguna clase, ni de 1|A ni de 2|B.
        int usosDeLaReferenciaBloqueada = 0;
        for (const auto& curso : resultado.resultado.horarios)
            for (const auto& dia : curso.dias)
                for (const auto& asig : dia.asignaciones)
                    if (asig.aula == 0 && desplazadas.contains(qMakePair(dia.dia, asig.slot)))
                        ++usosDeLaReferenciaBloqueada;
        QCOMPARE(usosDeLaReferenciaBloqueada, 0);
    }

    // ── T4/RF-3: la referencia bloqueada no cede su franja a otro curso ──
    void solver_referenciaBloqueadaNoCedeSuFranja() {
        // 1|A solo tiene una materia de cancha: sus 2 clases están desplazadas y bloquean su
        // referencia (Aula 1) en 2 de las 3 franjas del turno. 2|B necesita el Aula 1 en 2
        // franjas y solo le queda una libre; sin el bloqueo habría solución.
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {2, 2, 2, 2, 1, 3};
        config.franja_horaria = {60, 3};
        config.turnos.turnos = {{"manana", {0, 1, 2}}};

        config.materias = {{"Matematica", "Aula de estudio"}, {"Educacion Fisica", "Cancha"}};
        config.aulas = {{"Aula 1", "Aula de estudio", 30}, {"Cancha", "Cancha", 40}};
        config.planes = {{"Plan A", {{1, 1, 2}}}, {"Plan B", {{0, 2, 2}}}};
        config.cursos = {{1, "A", "manana", 20, "Plan A", 0}, {2, "B", "manana", 20, "Plan B", 0}};
        config.profesores = {
            {"Prof EdFisica", 2, 2, "", "", {}, {1}, {}, dispCompleta(1, 3)},
            {"Prof Mate", 2, 2, "", "", {}, {0}, {}, dispCompleta(1, 3)}};

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY2(!resultado.exito,
                 "la referencia bloqueada no puede alojar la clase de otro curso en esa franja");
        QVERIFY(!resultado.errores.isEmpty());
        QVERIFY(resultado.errores[0].contains("INFEASIBLE"));
    }

    // ── T4/RF-3: la exclusividad de aula cubre las clases desplazadas ──
    void solver_exclusividadEntreClasesDesplazadas() {
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {2, 2, 3, 2, 2, 4};
        config.franja_horaria = {60, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};

        config.materias = {{"Matematica", "Aula de estudio"}, {"Quimica", "Laboratorio"}};
        config.aulas = {{"Aula 1", "Aula de estudio", 30},
                        {"Aula 2", "Aula de estudio", 25},
                        {"Lab unico", "Laboratorio", 20}};
        config.planes = {{"Plan A", {{0, 1, 4}, {1, 1, 2}}}, {"Plan B", {{0, 2, 4}, {1, 2, 2}}}};
        config.cursos = {{1, "A", "manana", 20, "Plan A", 0}, {2, "B", "manana", 20, "Plan B", 1}};
        config.profesores = {
            {"Prof A", 6, 6, "", "", {}, {0, 1}, {}, dispCompleta(2, 4)},
            {"Prof B", 6, 6, "", "", {}, {0, 1}, {}, dispCompleta(2, 4)}};

        Solver solver;
        auto resultado = solver.resolver(config);
        QVERIFY2(resultado.exito, qPrintable(resultado.errores.join("\n")));

        int enLaboratorio = 0;
        QSet<QPair<int, int>> franjasOcupadas;
        for (const auto& curso : resultado.resultado.horarios)
            for (const auto& dia : curso.dias)
                for (const auto& asig : dia.asignaciones)
                    if (asig.aula == 2) {
                        ++enLaboratorio;
                        const auto franja = qMakePair(dia.dia, asig.slot);
                        QVERIFY2(!franjasOcupadas.contains(franja),
                                 "el laboratorio no puede alojar dos clases desplazadas a la vez");
                        franjasOcupadas.insert(franja);
                    }
        QCOMPARE(enLaboratorio, 4);   // los 2 bloques de Química de cada curso

        // RF-3: las aulas de un tipo especializado NUNCA quedan reservadas. El laboratorio aloja
        // clases de los dos cursos mientras ambos están desplazados (solo se bloquea la
        // referencia general de cada curso).
        int labDeA = 0;
        int labDeB = 0;
        for (auto it = resultado.resultado.horarios.constBegin();
             it != resultado.resultado.horarios.constEnd(); ++it) {
            int enLab = 0;
            for (const DiaOutput& dia : it.value().dias)
                for (const AsignacionOutput& asignacion : dia.asignaciones)
                    if (asignacion.aula == 2) ++enLab;
            if (it.key() == QStringLiteral("1|A")) labDeA = enLab;
            if (it.key() == QStringLiteral("2|B")) labDeB = enLab;
        }
        QCOMPARE(labDeA, 2);
        QCOMPARE(labDeB, 2);
    }

    // ── T4/RF-3: en modo móvil cada clase elige un aula del tipo requerido ──
    void solver_modoMovilEligeAulaDelTipoRequerido() {
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = false;
        config.dimensiones = {1, 2, 3, 1, 3, 4};
        config.franja_horaria = {60, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};

        config.materias = {{"Matematica", "Aula de estudio"}, {"Laboratorio", "Laboratorio"}};
        config.aulas = {{"Aula 1", "Aula de estudio", 30},
                        {"Aula 2", "Aula de estudio", 25},
                        {"Lab", "Laboratorio", 20}};
        config.planes = {{"Plan General", {{0, 1, 4}, {1, 1, 2}}}};
        config.cursos = {{1, "A", "manana", 20, "Plan General", -1}};   // sin referencia
        config.profesores = {
            {"Prof A", 6, 6, "manana", "Plan General", {}, {0, 1}, {}, dispCompleta(3, 4)}};

        Solver solver;
        auto resultado = solver.resolver(config);
        QVERIFY2(resultado.exito, qPrintable(resultado.errores.join("\n")));

        int deMatematica = 0, deLaboratorio = 0;
        for (const auto& dia : resultado.resultado.horarios[claveCurso(1, "A")].dias)
            for (const auto& asig : dia.asignaciones) {
                if (asig.materia == 0) {
                    QVERIFY(asig.aula == 0 || asig.aula == 1);   // tipo «Aula de estudio»
                    ++deMatematica;
                } else {
                    QCOMPARE(asig.materia, 1);
                    QCOMPARE(asig.aula, 2);                      // único aula «Laboratorio»
                    ++deLaboratorio;
                }
            }
        QCOMPARE(deMatematica, 4);
        QCOMPARE(deLaboratorio, 2);
    }

    // ── T6/RF-6: la demanda de un tipo que supera la capacidad de sus aulas es conflicto de aula ──
    void solver_falloCausaConflictoDeAulaPorSaturacionDeTipo() {
        // Dos cursos con su propia referencia: ninguno comparte aula de referencia, pero ambos
        // desplazan 5 bloques al ÚNICO laboratorio (10 bloques frente a 8 franjas de laboratorio).
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {2, 2, 3, 2, 2, 4};
        config.franja_horaria = {60, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};

        config.materias = {{"Matematica", "Aula de estudio"}, {"Laboratorio", "Laboratorio"}};
        config.aulas = {{"Aula A", "Aula de estudio", 30},
                        {"Aula B", "Aula de estudio", 25},
                        {"Lab unico", "Laboratorio", 20}};
        config.planes = {{"Plan A", {{0, 1, 1}, {1, 1, 5}}},
                         {"Plan B", {{0, 2, 1}, {1, 2, 5}}}};
        config.cursos = {{1, "A", "manana", 20, "Plan A", 0},    // referencia = Aula A
                         {2, "B", "manana", 20, "Plan B", 1}};   // referencia = Aula B
        config.profesores = {
            {"Prof A", 6, 6, "", "", {}, {0, 1}, {}, dispCompleta(2, 4)},
            {"Prof B", 6, 6, "", "", {}, {0, 1}, {}, dispCompleta(2, 4)}};

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY2(!resultado.exito, "10 bloques de laboratorio no caben en 1 aula × 2 días × 4 franjas");
        const QString texto = resultado.errores.join("\n");
        // Conflicto de aula, con el tipo, la demanda y la capacidad (RF-6).
        QVERIFY2(texto.contains("Causa (2)"), qPrintable(texto));
        QVERIFY2(texto.contains("«Laboratorio»"), qPrintable(texto));
        QVERIFY2(texto.contains("10 bloques"), qPrintable(texto));
        QVERIFY2(texto.contains("8 franjas"), qPrintable(texto));
        // No se libera la referencia ni se reasigna a otro tipo para hacer factible el modelo.
        QVERIFY2(!texto.contains("Causa (4)"), qPrintable(texto));
    }

    // ── T6/RF-6: la misma saturación por tipo se informa también en modo móvil ──
    void solver_falloCausaConflictoDeAulaPorSaturacionDeTipoEnModoMovil() {
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = false;
        config.dimensiones = {2, 2, 3, 2, 2, 4};
        config.franja_horaria = {60, 4};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3}}};

        config.materias = {{"Matematica", "Aula de estudio"}, {"Laboratorio", "Laboratorio"}};
        config.aulas = {{"Aula A", "Aula de estudio", 30},
                        {"Aula B", "Aula de estudio", 25},
                        {"Lab unico", "Laboratorio", 20}};
        config.planes = {{"Plan A", {{0, 1, 1}, {1, 1, 5}}},
                         {"Plan B", {{0, 2, 1}, {1, 2, 5}}}};
        config.cursos = {{1, "A", "manana", 20, "Plan A", -1},
                         {2, "B", "manana", 20, "Plan B", -1}};
        config.profesores = {
            {"Prof A", 6, 6, "", "", {}, {0, 1}, {}, dispCompleta(2, 4)},
            {"Prof B", 6, 6, "", "", {}, {0, 1}, {}, dispCompleta(2, 4)}};

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY(!resultado.exito);
        const QString texto = resultado.errores.join("\n");
        QVERIFY2(texto.contains("Causa (2)"), qPrintable(texto));
        QVERIFY2(texto.contains("«Laboratorio»"), qPrintable(texto));
        QVERIFY2(texto.contains("10 bloques"), qPrintable(texto));
        QVERIFY2(texto.contains("8 franjas"), qPrintable(texto));
    }

    // ── RF-6: el tipo «Aula de estudio» demanda 10 y su capacidad es 6 ──
    void solver_falloCausaConflictoDeAulaPorCapacidadDeTipo() {
        // Dos cursos comparten la referencia Aula 1; cada uno tiene 5 bloques del tipo general y
        // desplaza 1: 10 bloques frente a 1 aula × 6 franjas de capacidad del tipo.
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {2, 2, 2, 2, 2, 3};
        config.franja_horaria = {60, 3};
        config.turnos.turnos = {{"manana", {0, 1, 2}}};

        config.materias = {{"Matematica", "Aula de estudio"}, {"Educacion Fisica", "Cancha"}};
        config.aulas = {{"Aula 1", "Aula de estudio", 30}, {"Cancha", "Cancha", 40}};
        config.planes = {{"Plan A", {{0, 1, 5}, {1, 1, 1}}},
                         {"Plan B", {{0, 2, 5}, {1, 2, 1}}}};
        config.cursos = {{1, "A", "manana", 20, "Plan A", 0},
                         {2, "B", "manana", 20, "Plan B", 0}};   // MISMA referencia
        config.profesores = {
            {"Prof A", 6, 6, "", "", {}, {0, 1}, {}, dispCompleta(2, 3)},
            {"Prof B", 6, 6, "", "", {}, {0, 1}, {}, dispCompleta(2, 3)}};

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY(!resultado.exito);
        const QString texto = resultado.errores.join("\n");
        QVERIFY2(texto.contains("Causa (2)"), qPrintable(texto));
        QVERIFY2(texto.contains("«Aula de estudio»"), qPrintable(texto));
        QVERIFY2(texto.contains("10 bloques"), qPrintable(texto));
        QVERIFY2(texto.contains("6 franjas"), qPrintable(texto));
    }

    // ── T6/RF-6: con capacidad de sobra en el tipo, el aula no oculta la causa real ──
    // «Aula de estudio» demanda 6 y «Laboratorio» 4 frente a las 12 franjas de capacidad de cada
    // tipo (6 días × 2 franjas por aula), así que el conteo de (2) no falla. Lo que comprueba es
    // que no se inventa un conflicto de aula y que la causa real —2|B sin docente elegible— se
    // informa como (4). La (2) decide solo por capacidad del tipo, sin descontar el bloqueo de la
    // referencia (quien aísla ese bloqueo es `solver_falloDocenteNoElegibleNoSeInformaComoConflictoDeAula`).
    void solver_causaDeAulaConSemanaAnchaNoInventaElConflicto() {
        // 1|A (2 bloques en la referencia + 4 de laboratorio) y 2|B (4 en la referencia)
        // comparten el Aula 1 durante 6 días × 2 franjas = 12 franjas. Los 6 bloques del tipo
        // «Aula de estudio» (= 2 + 4) caben de sobra en la capacidad del tipo, así que el aula no
        // es la causa. La causa real es de asignación (4): 2|B no tiene docente elegible.
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {1, 2, 2, 2, 6, 2};
        config.franja_horaria = {60, 2};
        config.turnos.turnos = {{"manana", {0, 1}}};

        config.materias = {{"Matematica", "Aula de estudio"}, {"Laboratorio", "Laboratorio"}};
        config.aulas = {{"Aula 1", "Aula de estudio", 30}, {"Lab", "Laboratorio", 20}};
        config.planes = {{"Plan A", {{0, 1, 2}, {1, 1, 4}}},
                         {"Plan B", {{0, 2, 4}}}};
        config.cursos = {{1, "A", "manana", 20, "Plan A", 0},
                         {2, "B", "manana", 20, "Plan B", 0}};   // MISMA referencia
        config.profesores = {
            // Solo cubre el año 1: la materia de 2|B no tiene docente elegible.
            {"Prof A", 6, 6, "", "", {1}, {0, 1}, {}, dispCompleta(6, 2)}};

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY(!resultado.exito);
        const QString texto = resultado.errores.join("\n");
        QVERIFY2(!texto.contains("Causa (2)"), qPrintable(texto));
        QVERIFY2(texto.contains("Causa (4)"), qPrintable(texto));
    }

    // ── T6/RF-6 y V17: un tipo requerido sin aulas es entrada inválida, no conflicto de aula ──
    void solver_falloTipoSinAulas_esEntradaInvalida() {
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {1, 1, 1, 1, 2, 3};
        config.franja_horaria = {60, 3};
        config.turnos.turnos = {{"manana", {0, 1, 2}}};

        config.materias = {{"Quimica", "Laboratorio"}};
        config.aulas = {{"Aula 1", "Aula de estudio", 30}};   // ningún aula «Laboratorio»
        config.planes = {{"Plan General", {{0, 1, 2}}}};
        config.cursos = {{1, "A", "manana", 20, "Plan General", 0}};
        config.profesores = {
            {"Prof A", 2, 2, "", "", {}, {0}, {}, dispCompleta(2, 3)}};

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY(!resultado.exito);
        const QString texto = resultado.errores.join("\n");
        QVERIFY2(texto.contains("Causa (1)"), qPrintable(texto));
        QVERIFY2(texto.contains("«Laboratorio»"), qPrintable(texto));
        QVERIFY2(!texto.contains("Causa (2)"), qPrintable(texto));
    }

    // ── RF-6: la referencia compartida no cabe y ninguna categoría lo explica ──
    // 1|A y 2|B comparten «A1» (Aula común): 1|A tiene 2 bloques comunes + 4 de laboratorio y 2|B
    // necesita 3 bloques comunes. El conteo del tipo no falla (5 ≤ 6) y cada curso cabe en su
    // turno, pero la contención en la referencia compartida deja la infactibilidad sin categoría:
    // «no atribuible», sin nombrar entidades.
    void solver_causaNoAtribuibleCuandoLaReferenciaCompartidaNoCabe() {
        // 1|A y 2|B comparten la referencia «A1» (Aula común). 1|A tiene 2 bloques comunes
        // + 4 de laboratorio: sus 6 clases ocupan las 6 franjas del turno (2 días × 3), así
        // que en las 4 franjas del laboratorio A1 queda vacía y bloqueada. 2|B necesita
        // 3 franjas en A1 y solo le quedan 2 libres (las de las clases comunes de 1|A).
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {2, 2, 2, 2, 2, 3};
        config.franja_horaria = {60, 3};
        config.turnos.turnos = {{"manana", {0, 1, 2}}};

        config.materias = {{"Matematica", "Aula común"}, {"Laboratorio", "Laboratorio"}};
        config.aulas = {{"A1", "Aula común", 30}, {"Lab", "Laboratorio", 20}};
        config.planes = {{"Plan A", {{0, 1, 2}, {1, 1, 4}}},
                         {"Plan B", {{0, 2, 3}}}};
        config.cursos = {{1, "A", "manana", 20, "Plan A", 0},
                         {2, "B", "manana", 20, "Plan B", 0}};   // MISMA referencia
        config.profesores = {
            {"Prof A", 6, 6, "manana", "Plan A", {}, {0, 1}, {}, dispCompleta(2, 3)},
            {"Prof B", 3, 3, "manana", "Plan B", {}, {0}, {}, dispCompleta(2, 3)}};

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY2(!resultado.exito, "la referencia compartida no aloja los 5 bloques de Aula común");
        const QString texto = resultado.errores.join("\n");
        // El conteo del tipo no falla (5 ≤ 6) y ninguna categoría explica la infactibilidad:
        // el desenlace es «no atribuible», sin nombrar entidades.
        QVERIFY2(!texto.contains("Causa ("), qPrintable(texto));
        QVERIFY2(texto.contains("no atribuible"), qPrintable(texto));
        QVERIFY2(!texto.contains("A1"), qPrintable(texto));
        QVERIFY2(!texto.contains("1|A"), qPrintable(texto));
    }

    // Control negativo de la corrección: con capacidad suficiente no se inventa la causa (2).
    void solver_causaDeAulaConCapacidadSuficienteNoInventaElConflicto() {
        // Mismo escenario, pero 2|B tiene su propia referencia (A2, otra aula común): A1
        // conserva sus 2 franjas libres y el dato es factible.
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {2, 2, 3, 2, 2, 3};
        config.franja_horaria = {60, 3};
        config.turnos.turnos = {{"manana", {0, 1, 2}}};

        config.materias = {{"Matematica", "Aula común"}, {"Laboratorio", "Laboratorio"}};
        config.aulas = {{"A1", "Aula común", 30},
                        {"A2", "Aula común", 30},
                        {"Lab", "Laboratorio", 20}};
        config.planes = {{"Plan A", {{0, 1, 2}, {1, 1, 4}}},
                         {"Plan B", {{0, 2, 3}}}};
        config.cursos = {{1, "A", "manana", 20, "Plan A", 0},
                         {2, "B", "manana", 20, "Plan B", 1}};   // referencias distintas
        config.profesores = {
            {"Prof A", 6, 6, "manana", "Plan A", {}, {0, 1}, {}, dispCompleta(2, 3)},
            {"Prof B", 3, 3, "manana", "Plan B", {}, {0}, {}, dispCompleta(2, 3)}};

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY2(resultado.exito, qPrintable(resultado.errores.join("\n")));
        QVERIFY(resultado.errores.isEmpty());
    }

    // ── RF-6: dos cursos desplazados que comparten referencia y caben son factibles ──
    // Comparten A1 y se desplazan a la misma franja (cada uno a un laboratorio distinto); el
    // conteo del tipo no falla y el bloqueo de la referencia no resta capacidad, así que el dato
    // es factible. La unión de franjas bloqueadas ya no forma parte de ninguna cuenta.
    void solver_causaDeAulaConDosCursosDesplazadosEsFactible() {
        SolverConfig config = configDosCursosDesplazadosALaMismaFranja();

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY2(resultado.exito, qPrintable(resultado.errores.join("\n")));
        QVERIFY(resultado.errores.isEmpty());
    }

    // Mismo dato, pero el docente de 2|B no cubre el laboratorio: la causa real es (4). La (2) no
    // aplica porque el conteo del tipo no falla y la referencia compartida no resta capacidad, así
    // que no se informa un conflicto de aula que taparía la causa de asignación.
    void solver_falloDocenteNoElegibleNoSeInformaComoConflictoDeAula() {
        SolverConfig config = configDosCursosDesplazadosALaMismaFranja();
        // El docente de 2|B solo opta por la materia común (índice 0), no por el laboratorio.
        config.profesores[1].materias_asignadas = {0};

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY2(!resultado.exito, "2|B no tiene docente de laboratorio");
        const QString texto = resultado.errores.join("\n");
        QVERIFY2(texto.contains("Causa (4)"), qPrintable(texto));
        QVERIFY2(!texto.contains("Causa (2)"), qPrintable(texto));
        QVERIFY2(texto.contains("2|B"), qPrintable(texto));
        QVERIFY2(texto.contains("«Laboratorio»"), qPrintable(texto));
    }

    // ── RF-6: 28 ≤ 30 en la referencia casi llena; el conteo por tipo pasa ──
    // Dos cursos comparten A1 (5 días × 6 franjas) y cada uno cursa 14 bloques comunes + 2 de
    // laboratorio. El tipo «Aula común» demanda 28 y tiene 30 franjas de capacidad, así que (2) no
    // falla; la contención en la referencia compartida deja la infactibilidad sin categoría.
    void solver_causaNoAtribuibleCuandoLaReferenciaCasiLlenaNoCabe() {
        SolverConfig config = configDosCursosCasiLlenosConLaboratorios(1);

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY2(!resultado.exito, "la referencia casi llena no aloja los 28 bloques de Aula común");
        const QString texto = resultado.errores.join("\n");
        // 28 ≤ 30 y 4 ≤ 30: el conteo por tipo pasa; ninguna categoría lo explica.
        QVERIFY2(!texto.contains("Causa ("), qPrintable(texto));
        QVERIFY2(texto.contains("no atribuible"), qPrintable(texto));
        QVERIFY2(!texto.contains("A1"), qPrintable(texto));
    }

    // Contraprueba del mismo dato: con 2 laboratorios los 4 bloques caben en 2 franjas, A1
    // conserva 28 y el horario existe. El aula no inventa el conflicto.
    void solver_causaDeAulaConLaboratoriosSuficientesNoInventaElConflicto() {
        SolverConfig config = configDosCursosCasiLlenosConLaboratorios(2);

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY2(resultado.exito, qPrintable(resultado.errores.join("\n")));
        QVERIFY(resultado.errores.isEmpty());
    }

    // ── RF-6: el tipo tiene capacidad (7 ≤ 12) y la infactibilidad no se atribuye ──
    // 1|A ocupa todas sus franjas con laboratorio y bloquea A1; 2|B llena A2 y 3|C necesita un
    // bloque de «Aula base». El conteo del tipo no falla (7 bloques ≤ 2 aulas × 6 franjas), así que
    // ninguna categoría lo explica. La referencia de 3|C sigue en la cancha: los fixtures del motor
    // no pasan por V18.
    void solver_causaNoAtribuibleCuandoElTipoTieneCapacidad() {
        // 1|A ocupa sus 6 franjas con laboratorio y bloquea su referencia A1 (tipo «Aula base»)
        // en todas ellas; 2|B llena A2 con 6 bloques de «Aula base» y 3|C necesita 1 bloque de ese
        // tipo, pero no le queda ninguna franja utilizable. El conteo del tipo no falla (7 ≤ 12).
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {3, 3, 4, 3, 1, 6};
        config.franja_horaria = {60, 6};
        config.turnos.turnos = {{"manana", {0, 1, 2, 3, 4, 5}}};

        config.materias = {{"Matematica", "Aula base"},
                           {"Laboratorio", "Laboratorio"},
                           {"Educacion Fisica", "Cancha"}};
        config.aulas = {{"A1", "Aula base", 30}, {"A2", "Aula base", 30},
                        {"Lab", "Laboratorio", 20}, {"Cancha", "Cancha", 40}};
        config.planes = {{"Plan A", {{1, 1, 6}}},
                         {"Plan B", {{0, 2, 6}}},
                         {"Plan C", {{0, 3, 1}}}};
        config.cursos = {{1, "A", "manana", 20, "Plan A", 0},     // referencia A1
                         {2, "B", "manana", 20, "Plan B", 1},     // referencia A2
                         {3, "C", "manana", 20, "Plan C", 3}};    // referencia Cancha
        config.profesores = {
            {"Prof A", 6, 6, "manana", "Plan A", {}, {1}, {}, dispCompleta(1, 6)},
            {"Prof B", 6, 6, "manana", "Plan B", {}, {0}, {}, dispCompleta(1, 6)},
            {"Prof C", 1, 1, "manana", "Plan C", {}, {0}, {}, dispCompleta(1, 6)}};

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY2(!resultado.exito, "A1 y A2 no dan cabida a los 7 bloques del tipo");
        const QString texto = resultado.errores.join("\n");
        // 7 ≤ 12: el conteo del tipo pasa (la referencia de 3|C sigue en la cancha: los fixtures
        // del motor no pasan por V18) y la infactibilidad se queda sin categoría.
        QVERIFY2(!texto.contains("Causa ("), qPrintable(texto));
        QVERIFY2(texto.contains("no atribuible"), qPrintable(texto));
    }

    // ── RF-6: con turnos distintos, el conteo del tipo no inventa un conflicto de aula ──
    // «Aula común» demanda 6 frente a 1 aula × 8 franjas (unión de T1 y T2 en 2 días) y
    // «Laboratorio» 4 frente a 2 aulas × 8, así que (2) no falla. La causa real es que 2|B no
    // tiene docente de laboratorio y se informa (4), no una (2) por la referencia A1.
    void solver_causaDeAulaNoSobreestimaConTurnosDistintos() {
        SolverConfig config = configDosCursosConTurnosDistintos();
        // El docente de 2|B solo opta por la materia común (índice 0), no por el laboratorio.
        config.profesores[1].materias_asignadas = {0};

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY2(!resultado.exito, "2|B no tiene docente de laboratorio");
        const QString texto = resultado.errores.join("\n");
        QVERIFY2(texto.contains("Causa (4)"), qPrintable(texto));
        QVERIFY2(!texto.contains("Causa (2)"), qPrintable(texto));
        QVERIFY2(texto.contains("2|B"), qPrintable(texto));
        QVERIFY2(texto.contains("«Laboratorio»"), qPrintable(texto));
    }

    // Control positivo del mismo dato: con el docente de 2|B cubriendo el laboratorio el horario
    // existe (A1 tiene sitio y los 4 bloques de laboratorio caben en los 2 laboratorios).
    void solver_causaDeAulaConTurnosDistintosNoInventaElConflicto() {
        SolverConfig config = configDosCursosConTurnosDistintos();

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY2(resultado.exito, qPrintable(resultado.errores.join("\n")));
        QVERIFY(resultado.errores.isEmpty());
    }

    // ── RF-6: la causa es (4) y no (2) cuando el tipo tiene capacidad ──
    // Variante alcanzable en producción: V13 solo exige solape entre la disponibilidad del docente
    // y el turno del curso, no capacidad, así que un titular con una sola franja pasa la validación
    // y el modelo es INFEASIBLE. «Aula base» demanda 8 = 2 aulas × 4 franjas (TB) y «Laboratorio» 2
    // = 1 aula × 2 franjas (TL), así que el conteo no falla y la causa es (4). (Fixture heredado de
    // la corrección 5, ya histórica.)
    void solver_causaDeDocenteEnTurnoDistintoNoSeInformaComoConflictoDeAula() {
        SolverConfig config = configLaboratorioEnTurnoDistinto();

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY2(!resultado.exito, "el titular de laboratorio solo tiene 1 franja para 2 bloques");
        const QString texto = resultado.errores.join("\n");
        QVERIFY2(!texto.contains("Causa (2)"), qPrintable(texto));
        QVERIFY2(texto.contains("Causa (4)"), qPrintable(texto));
        QVERIFY2(texto.contains("«Laboratorio»"), qPrintable(texto));
        QVERIFY2(texto.contains("1|A"), qPrintable(texto));
    }

    // Misma entrada con el docente SIN materias asignadas: la causa (4) es «no tiene ningún
    // docente elegible». V10/V13 la rechazan en el flujo real, pero el solver directo debe
    // informarla igual, sin convertirla en una causa (2) de capacidad del tipo.
    void solver_causaDeDocenteNoElegibleNoSeInformaComoConflictoDeAula() {
        SolverConfig config = configLaboratorioEnTurnoDistinto();
        config.profesores[0].materias_asignadas = {};

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY(!resultado.exito);
        const QString texto = resultado.errores.join("\n");
        QVERIFY2(!texto.contains("Causa (2)"), qPrintable(texto));
        QVERIFY2(texto.contains("Causa (4)"), qPrintable(texto));
        QVERIFY2(texto.contains("«Laboratorio»"), qPrintable(texto));
        QVERIFY2(texto.contains("1|A"), qPrintable(texto));
    }

    // Control positivo: con el titular de laboratorio disponible en todo el turno el horario
    // existe (2 bloques en L1 y 8 bloques de «Aula base» en A1/A2).
    void solver_causaSaturacionPorTipoConDocenteSuficienteNoInventaElConflicto() {
        SolverConfig config = configLaboratorioEnTurnoDistinto();
        config.profesores[0].disponibilidad = dispCompleta(1, 6);

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY2(resultado.exito, qPrintable(resultado.errores.join("\n")));
        QVERIFY(resultado.errores.isEmpty());
    }

    // ── T13/RF-6: el conteo por tipo no descuenta bloqueos; la causa real es de docente ──
    // Contraejemplo del reviewer: 2 días y R1 compartida. «Aula base» demanda 4 = capacidad
    // 1 aula × 2 días × 2 franjas y el laboratorio demanda 2 ≤ 4, así que el conteo por tipo no
    // falla; el docente de laboratorio solo tiene una franja para 2 bloques. La causa es (4), no
    // una (2) por el bloqueo de R1 (el descuento que aplicaba el diagnóstico anterior).
    void solver_causaDeAulaContraejemploDelReviewer_esCausaDeDocente() {
        SolverConfig config;
        config.version = "2.0";
        config.aula_fija = true;
        config.dimensiones = {2, 2, 2, 2, 2, 7};
        config.franja_horaria = {60, 7};
        config.turnos.turnos = {{"TB", {5, 6}}, {"TX", {4, 5}}};

        config.materias = {{"Ciencias", "Aula base"}, {"Laboratorio", "Laboratorio"}};
        config.aulas = {{"R1", "Aula base", 30}, {"L1", "Laboratorio", 20}};
        config.planes = {{"Plan Y", {{0, 2, 4}}},    // 2|Y: 4 bloques de Ciencias
                         {"Plan X", {{1, 1, 2}}}};   // 1|X: 2 bloques de Laboratorio
        config.cursos = {{2, "Y", "TB", 20, "Plan Y", 0},    // referencia R1
                         {1, "X", "TX", 20, "Plan X", 0}};   // MISMA referencia R1
        config.profesores = {
            // 4 celdas para los 4 bloques de 2|Y: (0,{5,6}) y (1,{5,6}).
            {"Prof Y", 4, 4, "", "", {2}, {0}, {}, {{0, {5, 6}}, {1, {5, 6}}}},
            // Una sola celda para los 2 bloques de laboratorio de 1|X.
            {"Prof Lab", 2, 2, "", "", {1}, {1}, {}, {{0, {4}}}}};

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY(!resultado.exito);
        const QString texto = resultado.errores.join("\n");
        QVERIFY2(!texto.contains("Causa (2)"), qPrintable(texto));
        QVERIFY2(texto.contains("Causa (4)"), qPrintable(texto));
        QVERIFY2(texto.contains("1|X"), qPrintable(texto));
        QVERIFY2(texto.contains("«Laboratorio»"), qPrintable(texto));
        // Sub-motivo: el docente de laboratorio existe y es elegible, pero no dispone de franjas
        // suficientes para los 2 bloques (no es «no tiene ningún docente elegible»).
        QVERIFY2(texto.contains("ningún docente elegible dispone de tantas franjas"),
                 qPrintable(texto));
        QVERIFY2(!texto.contains("no tiene ningún docente elegible"), qPrintable(texto));
    }

    // ── T14/RF-6: el conteo global es necesario pero no suficiente (condición de Hall) ──
    // 1|A y 2|B solo pueden ir a la celda (0,4) del aula A1 y D8 impide compartirla; 3|C tiene
    // (0,5) y (0,6). La demanda del tipo (3) es igual a su capacidad (1 aula × 3 franjas de la
    // unión TB∪TC), así que (2) no falla; (3) y (4) tampoco (cada curso cabe en su turno y cada
    // docente tiene su celda). La infactibilidad es de ventanas entre cursos y se informa sin
    // atribuirla, sin nombrar entidades.
    void solver_causaNoAtribuiblePorVentanasDeTurno() {
        const SolverConfig config = configVentanasDeTurnoHall();

        Solver solver;
        auto resultado = solver.resolver(config);

        QVERIFY2(!resultado.exito, "1|A y 2|B no caben en la única celda (0,4)");
        const QString texto = resultado.errores.join("\n");
        QVERIFY2(!texto.contains("Causa ("), qPrintable(texto));
        QVERIFY2(texto.contains("no atribuible"), qPrintable(texto));
        QVERIFY2(!texto.contains("A1"), qPrintable(texto));
        QVERIFY2(!texto.contains("1|A"), qPrintable(texto));
    }

    // ── T14/RNF-1: la misma entrada da la misma salida, también en el caso INFEASIBLE ──
    void solver_esReproducible_mismaEntradaMismaSalida() {
        // El motor fija num_search_workers(1) y random_seed(0): dos motores nuevos sobre el mismo
        // dato deben coincidir en éxito, en el resumen de cada clase y en los errores (texto
        // incluido).
        const auto resolverConMotorNuevo = [](const SolverConfig& config) {
            Solver solver;
            return solver.resolver(config);
        };

        const SolverConfig factible = crearConfigMinima();
        const auto factible1 = resolverConMotorNuevo(factible);
        const auto factible2 = resolverConMotorNuevo(factible);
        QVERIFY2(factible1.exito, qPrintable(factible1.errores.join("\n")));
        QVERIFY(!resumenDeSalida(factible1).isEmpty());
        QCOMPARE(factible2.exito, factible1.exito);
        QCOMPARE(resumenDeSalida(factible2), resumenDeSalida(factible1));
        QCOMPARE(factible2.errores, factible1.errores);

        const SolverConfig infactible = configVentanasDeTurnoHall();
        const auto infactible1 = resolverConMotorNuevo(infactible);
        const auto infactible2 = resolverConMotorNuevo(infactible);
        QVERIFY2(!infactible1.exito, "el fixture de ventanas debe ser INFEASIBLE");
        QVERIFY(!infactible1.errores.isEmpty());
        QCOMPARE(infactible2.exito, infactible1.exito);
        QCOMPARE(resumenDeSalida(infactible2), resumenDeSalida(infactible1));
        QCOMPARE(infactible2.errores, infactible1.errores);
    }

    // ── F3: reutilizar la instancia no arrastra modelo ni variables ──────────────
    void solver_reutilizarLaMismaInstancia_daElMismoResultado() {
        Solver solver;
        const SolverConfig config = crearConfigMinima();
        const auto primera = solver.resolver(config);
        const auto segunda = solver.resolver(config);

        QVERIFY2(primera.exito, qPrintable(primera.errores.join("\n")));
        QVERIFY2(segunda.exito, qPrintable(segunda.errores.join("\n")));
        QCOMPARE(segunda.exito, primera.exito);
        QCOMPARE(resumenDeSalida(segunda), resumenDeSalida(primera));
        QCOMPARE(segunda.errores, primera.errores);
    }

    void solver_reutilizarLaInstancia_noContaminaConOtraEntrada() {
        Solver solver;
        const SolverConfig factible = crearConfigMinima();
        const auto primera = solver.resolver(factible);
        QVERIFY2(primera.exito, qPrintable(primera.errores.join("\n")));

        // La misma instancia con una entrada no factible no hereda la solución anterior.
        const auto infactible = solver.resolver(configVentanasDeTurnoHall());
        QVERIFY(!infactible.exito);
        QVERIFY(!infactible.errores.isEmpty());
        QVERIFY(infactible.resultado.horarios.isEmpty());

        // Y volver al dato factible da lo mismo que la primera vez.
        const auto tercera = solver.resolver(factible);
        QVERIFY2(tercera.exito, qPrintable(tercera.errores.join("\n")));
        QCOMPARE(resumenDeSalida(tercera), resumenDeSalida(primera));
    }
};

QTEST_MAIN(TestSolverHorarios)
#include "test_solver_horarios.moc"
