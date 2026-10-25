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
        // Dos cursos que comparten el aula fija y, juntos, piden más bloques que franjas tiene
        // el aula (cada uno cabe por separado): la causa es el aula → categoría (2).
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
        QVERIFY2(texto.contains("Aula compartida"), qPrintable(texto));
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
};

QTEST_MAIN(TestSolverHorarios)
#include "test_solver_horarios.moc"
