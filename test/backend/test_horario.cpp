#include <QTest>
#include <QJsonObject>
#include <QJsonArray>
#include <backend/data/horario.hpp>

class TestHorario : public QObject {
    Q_OBJECT

private slots:
    // ── Asignacion ──
    void asignacion_toJson_returnsCorrectFields() {
        Asignacion a;
        a.aula.nombre = "A-101";
        a.aula.capacidad = 30;
        a.aula.locacion = "A";

        a.franja_horaria.dia = 1;
        a.franja_horaria.inicio = QTime(8, 0);
        a.franja_horaria.fin = QTime(10, 0);

        a.materia.nombre = "Matematicas";
        a.materia.horas_semanales = 5;

        a.profesor.nombre = "Dr. Garcia";

        QJsonObject obj = a.toJson();

        QCOMPARE(obj["aula"].toObject()["nombre"].toString(), QString("A-101"));
        QCOMPARE(obj["franja_horaria"].toObject()["dia"].toInt(), 1);
        QCOMPARE(obj["materia"].toObject()["nombre"].toString(), QString("Matematicas"));
        QCOMPARE(obj["profesor"].toObject()["nombre"].toString(), QString("Dr. Garcia"));
    }

    void asignacion_fromJson_reconstructsObject() {
        QJsonObject aula, franja, materia, profesor;
        aula["nombre"] = "B-203"; aula["capacidad"] = 40; aula["locacion"] = "B";
        franja["dia"] = 2; franja["inicio"] = "09:00"; franja["fin"] = "11:00";
        materia["nombre"] = "Fisica"; materia["horas_semanales"] = 4;
        profesor["nombre"] = "Dra. Lopez";

        QJsonObject obj;
        obj["aula"] = aula;
        obj["franja_horaria"] = franja;
        obj["materia"] = materia;
        obj["profesor"] = profesor;

        Asignacion a = Asignacion::fromJson(obj);

        QCOMPARE(a.aula.nombre, QString("B-203"));
        QCOMPARE(a.franja_horaria.dia, 2);
        QCOMPARE(a.materia.nombre, QString("Fisica"));
        QCOMPARE(a.profesor.nombre, QString("Dra. Lopez"));
    }

    // ── Horario ──
    void horario_toJson_fromJson_singleAsignacion() {
        Horario h;

        Asignacion a;
        a.aula.nombre = "Lab-1";
        a.aula.capacidad = 25;
        a.franja_horaria.dia = 1;
        a.franja_horaria.inicio = QTime(8, 0);
        a.franja_horaria.fin = QTime(10, 0);
        a.materia.nombre = "Quimica";
        a.materia.horas_semanales = 3;
        a.profesor.nombre = "Dr. Perez";
        h.asignaciones = {a};

        QJsonObject json = h.toJson();
        Horario result = Horario::fromJson(json);

        QCOMPARE(result.asignaciones.size(), 1);
        QCOMPARE(result.asignaciones[0].aula.nombre, QString("Lab-1"));
        QCOMPARE(result.asignaciones[0].materia.nombre, QString("Quimica"));
        QCOMPARE(result.asignaciones[0].profesor.nombre, QString("Dr. Perez"));
        QCOMPARE(result.asignaciones[0].franja_horaria.dia, 1);
    }

    void horario_empty_asignaciones() {
        Horario h;
        QJsonObject json = h.toJson();
        Horario result = Horario::fromJson(json);
        QVERIFY(result.asignaciones.isEmpty());
    }
};

QTEST_MAIN(TestHorario)
#include "test_horario.moc"
