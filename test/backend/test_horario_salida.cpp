#include <QTest>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <backend/data/horario_salida.hpp>

class TestOutputSchedule : public QObject {
    Q_OBJECT

private slots:
    // ── AsignacionOutput ──
    void asignacion_output_round_trip() {
        AsignacionOutput a;
        a.slot = 0;
        a.materia = 1;
        a.profesor = 2;
        a.aula = 3;

        QJsonObject json = a.toJson();
        QCOMPARE(json["slot"].toInt(), 0);
        QCOMPARE(json["materia"].toInt(), 1);
        QCOMPARE(json["profesor"].toInt(), 2);
        QCOMPARE(json["aula"].toInt(), 3);

        AsignacionOutput b = AsignacionOutput::fromJson(json);
        QCOMPARE(b.slot, 0);
        QCOMPARE(b.materia, 1);
        QCOMPARE(b.profesor, 2);
        QCOMPARE(b.aula, 3);
    }

    void asignacion_output_default_values() {
        AsignacionOutput a;
        QCOMPARE(a.slot, -1);
        QCOMPARE(a.materia, -1);
        QCOMPARE(a.profesor, -1);
        QCOMPARE(a.aula, -1);
    }

    // ── DiaOutput ──
    void dia_output_round_trip() {
        DiaOutput d;
        d.dia = 1;

        AsignacionOutput a;
        a.slot = 0;
        a.materia = 5;
        a.aula = 2;
        d.asignaciones = {a};

        QJsonObject json = d.toJson();
        QCOMPARE(json["dia"].toInt(), 1);
        QCOMPARE(json["asignaciones"].toArray().size(), 1);

        DiaOutput e = DiaOutput::fromJson(json);
        QCOMPARE(e.dia, 1);
        QCOMPARE(e.asignaciones.size(), 1);
        QCOMPARE(e.asignaciones[0].slot, 0);
        QCOMPARE(e.asignaciones[0].materia, 5);
    }

    void dia_output_empty_assignments() {
        DiaOutput d;
        d.dia = 0;
        QJsonObject json = d.toJson();
        DiaOutput e = DiaOutput::fromJson(json);
        QCOMPARE(e.dia, 0);
        QVERIFY(e.asignaciones.isEmpty());
    }

    // ── CursoOutput ──
    void curso_output_round_trip() {
        CursoOutput c;
        c.turno = "matutino";

        DiaOutput d;
        d.dia = 0;
        c.dias = {d};

        QJsonObject json = c.toJson();
        QCOMPARE(json["turno"].toString(), QString("matutino"));
        QCOMPARE(json["dias"].toArray().size(), 1);

        CursoOutput e = CursoOutput::fromJson(json);
        QCOMPARE(e.turno, QString("matutino"));
        QCOMPARE(e.dias.size(), 1);
    }

    // ── MetadataSalida ──
    void metadata_round_trip() {
        MetadataSalida m;
        m.fecha_generacion = "2026-07-19T10:00:00";
        m.configuracion = "default";
        m.tiempo_ejecucion_ms = 1500;
        m.total_asignaciones = 45;
        m.cursos_generados = 12;
        m.profesores_asignados = 20;
        m.conflictos = 0;

        QJsonObject json = m.toJson();
        MetadataSalida n = MetadataSalida::fromJson(json);

        QCOMPARE(n.fecha_generacion, QString("2026-07-19T10:00:00"));
        QCOMPARE(n.configuracion, QString("default"));
        QCOMPARE(n.tiempo_ejecucion_ms, 1500);
        QCOMPARE(n.total_asignaciones, 45);
        QCOMPARE(n.cursos_generados, 12);
        QCOMPARE(n.profesores_asignados, 20);
        QCOMPARE(n.conflictos, 0);
    }

    void metadata_default_values() {
        MetadataSalida m;
        QCOMPARE(m.tiempo_ejecucion_ms, 0);
        QCOMPARE(m.total_asignaciones, 0);
        QVERIFY(m.fecha_generacion.isEmpty());
    }

    // ── HorarioSalida full round-trip ──
    void horario_salida_round_trip() {
        HorarioSalida h;

        h.metadata.fecha_generacion = "2026-07-19T10:00:00";
        h.metadata.configuracion = "default";
        h.metadata.tiempo_ejecucion_ms = 1500;
        h.metadata.total_asignaciones = 2;

        CursoOutput c1;
        c1.turno = "matutino";
        AsignacionOutput a1;
        a1.slot = 0; a1.materia = 1; a1.profesor = 2; a1.aula = 3;
        DiaOutput d1;
        d1.dia = 0;
        d1.asignaciones = {a1};
        c1.dias = {d1};
        h.horarios["1A"] = c1;

        CursoOutput c2;
        c2.turno = "vespertino";
        AsignacionOutput a2;
        a2.slot = 1; a2.materia = 4; a2.profesor = 5; a2.aula = 6;
        DiaOutput d2;
        d2.dia = 1;
        d2.asignaciones = {a2};
        c2.dias = {d2};
        h.horarios["2B"] = c2;

        QJsonObject json = h.toJson();
        QVERIFY(json.contains("metadata"));
        QVERIFY(json.contains("horarios"));
        QCOMPARE(json["horarios"].toObject().size(), 2);

        HorarioSalida h2 = HorarioSalida::fromJson(json);

        QCOMPARE(h2.metadata.fecha_generacion, QString("2026-07-19T10:00:00"));
        QCOMPARE(h2.metadata.tiempo_ejecucion_ms, 1500);
        QCOMPARE(h2.horarios.size(), 2);
        QVERIFY(h2.horarios.contains("1A"));
        QVERIFY(h2.horarios.contains("2B"));
        QCOMPARE(h2.horarios["1A"].turno, QString("matutino"));
        QCOMPARE(h2.horarios["1A"].dias[0].asignaciones[0].slot, 0);
        QCOMPARE(h2.horarios["2B"].turno, QString("vespertino"));
        QCOMPARE(h2.horarios["2B"].dias[0].asignaciones[0].slot, 1);
    }

    void horario_salida_empty_schedule() {
        HorarioSalida h;
        QJsonObject json = h.toJson();
        HorarioSalida h2 = HorarioSalida::fromJson(json);
        QVERIFY(h2.horarios.isEmpty());
        QCOMPARE(h2.metadata.tiempo_ejecucion_ms, 0);
    }
};

QTEST_MAIN(TestOutputSchedule)
#include "test_horario_salida.moc"
