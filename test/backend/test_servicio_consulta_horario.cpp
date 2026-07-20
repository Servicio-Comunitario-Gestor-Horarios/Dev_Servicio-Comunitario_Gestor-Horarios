#include <QTest>
#include <backend/data/output_schedule.hpp>
#include <backend/services/ServicioConsultaHorario.hpp>

/**
 * @brief Tests del servicio de consulta de horarios.
 *
 * Construye un HorarioSalida en memoria y verifica cada método
 * de consulta, detección de conflictos y estadísticas.
 */
class TestServicioConsultaHorario : public QObject {
    Q_OBJECT

private:
    HorarioSalida crearHorarioPrueba() const {
        HorarioSalida h;

        // Curso "1A" — turno matutino
        CursoOutput curso1A;
        curso1A.turno = "matutino";

        // Día 0: profesor=1, aula=10
        DiaOutput dia0;
        dia0.dia = 0;
        AsignacionOutput a1;
        a1.slot = 0;
        a1.materia = 100;
        a1.profesor = 1;
        a1.aula = 10;
        dia0.asignaciones.append(a1);
        curso1A.dias.append(dia0);

        // Día 1: profesor=2, aula=11
        DiaOutput dia1;
        dia1.dia = 1;
        AsignacionOutput a2;
        a2.slot = 1;
        a2.materia = 101;
        a2.profesor = 2;
        a2.aula = 11;
        dia1.asignaciones.append(a2);
        curso1A.dias.append(dia1);

        h.horarios["1A"] = curso1A;

        // Curso "2B" — turno vespertino
        CursoOutput curso2B;
        curso2B.turno = "vespertino";

        // Día 0: profesor=3, aula=12 (mismo día que 1A, pero otro slot)
        DiaOutput dia0b;
        dia0b.dia = 0;
        AsignacionOutput a3;
        a3.slot = 1;
        a3.materia = 200;
        a3.profesor = 3;
        a3.aula = 12;
        dia0b.asignaciones.append(a3);
        curso2B.dias.append(dia0b);

        // Día 1: profesor=1, aula=10 (MISMO profesor y aula que 1A día 0, slot 0 — sin conflicto porque día/slot difieren)
        DiaOutput dia1b;
        dia1b.dia = 1;
        AsignacionOutput a4;
        a4.slot = 0;
        a4.materia = 201;
        a4.profesor = 1;
        a4.aula = 10;
        dia1b.asignaciones.append(a4);
        curso2B.dias.append(dia1b);

        // Día 2: profesor=4, aula=13
        DiaOutput dia2;
        dia2.dia = 2;
        AsignacionOutput a5;
        a5.slot = 0;
        a5.materia = 202;
        a5.profesor = 4;
        a5.aula = 13;
        dia2.asignaciones.append(a5);
        curso2B.dias.append(dia2);

        h.horarios["2B"] = curso2B;

        return h;
    }

private slots:
    // ── buscarPorProfesor ──
    void buscar_profesor_con_asignaciones() {
        HorarioSalida h = crearHorarioPrueba();
        ServicioConsultaHorario servicio(h);

        auto resultado = servicio.buscarPorProfesor(1);
        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.size(), 2); // 1A día 0 + 2B día 1
        QCOMPARE(resultado.valor[0].curso, QString("1A"));
        QCOMPARE(resultado.valor[1].curso, QString("2B"));
    }

    void buscar_profesor_sin_asignaciones() {
        HorarioSalida h = crearHorarioPrueba();
        ServicioConsultaHorario servicio(h);

        auto resultado = servicio.buscarPorProfesor(99);
        QVERIFY(resultado.ok);
        QVERIFY(resultado.valor.isEmpty());
    }

    // ── buscarPorAula ──
    void buscar_aula_con_asignaciones() {
        HorarioSalida h = crearHorarioPrueba();
        ServicioConsultaHorario servicio(h);

        auto resultado = servicio.buscarPorAula(10);
        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.size(), 2); // 1A día 0 + 2B día 1
    }

    void buscar_aula_no_usada() {
        HorarioSalida h = crearHorarioPrueba();
        ServicioConsultaHorario servicio(h);

        auto resultado = servicio.buscarPorAula(99);
        QVERIFY(resultado.ok);
        QVERIFY(resultado.valor.isEmpty());
    }

    // ── buscarPorDia ──
    void buscar_dia_con_asignaciones() {
        HorarioSalida h = crearHorarioPrueba();
        ServicioConsultaHorario servicio(h);

        auto resultado = servicio.buscarPorDia(0);
        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.size(), 2); // 1A + 2B
    }

    void buscar_dia_sin_asignaciones() {
        HorarioSalida h = crearHorarioPrueba();
        ServicioConsultaHorario servicio(h);

        auto resultado = servicio.buscarPorDia(5);
        QVERIFY(resultado.ok);
        QVERIFY(resultado.valor.isEmpty());
    }

    // ── buscarPorCurso ──
    void buscar_curso_existente() {
        HorarioSalida h = crearHorarioPrueba();
        ServicioConsultaHorario servicio(h);

        auto resultado = servicio.buscarPorCurso("1A");
        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.size(), 2); // día 0 + día 1
        QCOMPARE(resultado.valor[0].curso, QString("1A"));
    }

    void buscar_curso_no_existente() {
        HorarioSalida h = crearHorarioPrueba();
        ServicioConsultaHorario servicio(h);

        auto resultado = servicio.buscarPorCurso("99Z");
        QVERIFY(!resultado.ok);
        QVERIFY(resultado.mensajeError.contains("99Z"));
    }

    // ── detectarConflictos ──
    void detectar_conflictos_profesor() {
        HorarioSalida h;

        CursoOutput c1;
        c1.turno = "matutino";
        DiaOutput d1;
        d1.dia = 0;
        AsignacionOutput a1;
        a1.slot = 0; a1.materia = 1; a1.profesor = 1; a1.aula = 10;
        d1.asignaciones.append(a1);
        c1.dias.append(d1);
        h.horarios["1A"] = c1;

        CursoOutput c2;
        c2.turno = "vespertino";
        DiaOutput d2;
        d2.dia = 0;
        AsignacionOutput a2;
        a2.slot = 0; a2.materia = 2; a2.profesor = 1; a2.aula = 20; // mismo profesor, distinta aula
        d2.asignaciones.append(a2);
        c2.dias.append(d2);
        h.horarios["2B"] = c2;

        ServicioConsultaHorario servicio(h);
        auto resultado = servicio.detectarConflictos();
        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.size(), 1);
        QCOMPARE(resultado.valor[0].tipo, QString("profesor"));
    }

    void detectar_conflictos_aula() {
        HorarioSalida h;

        CursoOutput c1;
        c1.turno = "matutino";
        DiaOutput d1;
        d1.dia = 0;
        AsignacionOutput a1;
        a1.slot = 0; a1.materia = 1; a1.profesor = 1; a1.aula = 10;
        d1.asignaciones.append(a1);
        c1.dias.append(d1);
        h.horarios["1A"] = c1;

        CursoOutput c2;
        c2.turno = "vespertino";
        DiaOutput d2;
        d2.dia = 0;
        AsignacionOutput a2;
        a2.slot = 0; a2.materia = 2; a2.profesor = 2; a2.aula = 10; // misma aula, distinto profesor
        d2.asignaciones.append(a2);
        c2.dias.append(d2);
        h.horarios["2B"] = c2;

        ServicioConsultaHorario servicio(h);
        auto resultado = servicio.detectarConflictos();
        QVERIFY(resultado.ok);
        QCOMPARE(resultado.valor.size(), 1);
        QCOMPARE(resultado.valor[0].tipo, QString("aula"));
    }

    void detectar_conflictos_ambos() {
        HorarioSalida h;

        CursoOutput c1;
        c1.turno = "matutino";
        DiaOutput d1;
        d1.dia = 0;
        AsignacionOutput a1;
        a1.slot = 0; a1.materia = 1; a1.profesor = 1; a1.aula = 10;
        d1.asignaciones.append(a1);
        c1.dias.append(d1);
        h.horarios["1A"] = c1;

        CursoOutput c2;
        c2.turno = "vespertino";
        DiaOutput d2;
        d2.dia = 0;
        AsignacionOutput a2;
        a2.slot = 0; a2.materia = 2; a2.profesor = 1; a2.aula = 10; // ambos coinciden
        d2.asignaciones.append(a2);
        c2.dias.append(d2);
        h.horarios["2B"] = c2;

        ServicioConsultaHorario servicio(h);
        auto resultado = servicio.detectarConflictos();
        QVERIFY(resultado.ok);
        // Ambos profesor y aula coinciden → 2 conflictos
        QCOMPARE(resultado.valor.size(), 2);
    }

    void detectar_conflictos_ninguno() {
        HorarioSalida h = crearHorarioPrueba();
        ServicioConsultaHorario servicio(h);

        auto resultado = servicio.detectarConflictos();
        QVERIFY(resultado.ok);
        QVERIFY(resultado.valor.isEmpty());
    }

    // ── hayConflicto ──
    void hay_conflicto_true() {
        HorarioSalida h;

        CursoOutput c1;
        c1.turno = "matutino";
        DiaOutput d1;
        d1.dia = 0;
        AsignacionOutput a1;
        a1.slot = 0; a1.materia = 1; a1.profesor = 1; a1.aula = 10;
        d1.asignaciones.append(a1);
        c1.dias.append(d1);
        h.horarios["1A"] = c1;

        CursoOutput c2;
        c2.turno = "vespertino";
        DiaOutput d2;
        d2.dia = 0;
        AsignacionOutput a2;
        a2.slot = 0; a2.materia = 2; a2.profesor = 1; a2.aula = 20;
        d2.asignaciones.append(a2);
        c2.dias.append(d2);
        h.horarios["2B"] = c2;

        ServicioConsultaHorario servicio(h);
        auto resultado = servicio.hayConflicto("1A", 0, 0);
        QVERIFY(resultado.ok);
        QVERIFY(resultado.valor);
    }

    void hay_conflicto_false() {
        HorarioSalida h = crearHorarioPrueba();
        ServicioConsultaHorario servicio(h);

        auto resultado = servicio.hayConflicto("1A", 0, 0);
        QVERIFY(resultado.ok);
        QVERIFY(!resultado.valor);
    }

    void hay_conflicto_curso_no_existente() {
        HorarioSalida h = crearHorarioPrueba();
        ServicioConsultaHorario servicio(h);

        auto resultado = servicio.hayConflicto("99Z", 0, 0);
        QVERIFY(!resultado.ok);
    }

    void hay_conflicto_asignacion_no_encontrada() {
        HorarioSalida h = crearHorarioPrueba();
        ServicioConsultaHorario servicio(h);

        auto resultado = servicio.hayConflicto("1A", 0, 99);
        QVERIFY(!resultado.ok);
    }

    // ── obtenerEstadisticas ──
    void estadisticas_horario_completo() {
        HorarioSalida h = crearHorarioPrueba();
        ServicioConsultaHorario servicio(h);

        auto resultado = servicio.obtenerEstadisticas();
        QVERIFY(resultado.ok);

        EstadisticasHorario stats = resultado.valor;
        QCOMPARE(stats.total_asignaciones, 5);

        QCOMPARE(stats.asignaciones_por_profesor[1], 2);
        QCOMPARE(stats.asignaciones_por_profesor[2], 1);
        QCOMPARE(stats.asignaciones_por_profesor[3], 1);
        QCOMPARE(stats.asignaciones_por_profesor[4], 1);

        QCOMPARE(stats.asignaciones_por_aula[10], 2);
        QCOMPARE(stats.asignaciones_por_aula[11], 1);
        QCOMPARE(stats.asignaciones_por_aula[12], 1);
        QCOMPARE(stats.asignaciones_por_aula[13], 1);

        QCOMPARE(stats.asignaciones_por_curso["1A"], 2);
        QCOMPARE(stats.asignaciones_por_curso["2B"], 3);

        QCOMPARE(stats.asignaciones_por_dia[0], 2);
        QCOMPARE(stats.asignaciones_por_dia[1], 2);
        QCOMPARE(stats.asignaciones_por_dia[2], 1);
    }

    void estadisticas_horario_vacio() {
        HorarioSalida h;
        ServicioConsultaHorario servicio(h);

        auto resultado = servicio.obtenerEstadisticas();
        QVERIFY(resultado.ok);

        EstadisticasHorario stats = resultado.valor;
        QCOMPARE(stats.total_asignaciones, 0);
        QVERIFY(stats.asignaciones_por_profesor.isEmpty());
        QVERIFY(stats.asignaciones_por_aula.isEmpty());
        QVERIFY(stats.asignaciones_por_curso.isEmpty());
        QVERIFY(stats.asignaciones_por_dia.isEmpty());
    }
};

QTEST_MAIN(TestServicioConsultaHorario)
#include "test_servicio_consulta_horario.moc"
