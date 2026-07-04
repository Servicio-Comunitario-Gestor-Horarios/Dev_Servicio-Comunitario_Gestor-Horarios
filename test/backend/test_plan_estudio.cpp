#include <QTest>
#include <QJsonObject>
#include <QJsonArray>
#include <backend/data/plan_estudio.hpp>

class TestPlanEstudio : public QObject {
    Q_OBJECT

private slots:
    void toJson_returnsCorrectFields() {
        PlanEstudio plan;
        plan.nombre = "Ingenieria en Sistemas";

        Materia m1;
        m1.nombre = "Programacion";
        m1.horas_semanales = 6;

        Materia m2;
        m2.nombre = "Base de Datos";
        m2.horas_semanales = 4;
        m2.requerimientos = {"Laboratorio"};

        plan.materias = {m1, m2};

        QJsonObject obj = plan.toJson();

        QCOMPARE(obj["nombre"].toString(), QString("Ingenieria en Sistemas"));
        QJsonArray materias = obj["materias"].toArray();
        QCOMPARE(materias.size(), 2);
        QCOMPARE(materias[0].toObject()["nombre"].toString(), QString("Programacion"));
        QCOMPARE(materias[1].toObject()["requerimientos"].toArray()[0].toString(), QString("Laboratorio"));
    }

    void fromJson_reconstructsObject() {
        QJsonObject m1, m2;
        m1["nombre"] = "Redes"; m1["horas_semanales"] = 5;
        m2["nombre"] = "Seguridad"; m2["horas_semanales"] = 4;

        QJsonObject obj;
        obj["nombre"] = "Ingenieria en Computacion";
        obj["materias"] = QJsonArray{m1, m2};

        PlanEstudio plan = PlanEstudio::fromJson(obj);

        QCOMPARE(plan.nombre, QString("Ingenieria en Computacion"));
        QCOMPARE(plan.materias.size(), 2);
        QCOMPARE(plan.materias[0].nombre, QString("Redes"));
        QCOMPARE(plan.materias[1].nombre, QString("Seguridad"));
    }

    void plan_withoutSubjects() {
        QJsonObject obj;
        obj["nombre"] = "Plan Vacio";

        PlanEstudio plan = PlanEstudio::fromJson(obj);

        QCOMPARE(plan.nombre, QString("Plan Vacio"));
        QVERIFY(plan.materias.isEmpty());
    }

    void roundtrip_toJsonFromJson_identity() {
        PlanEstudio original;
        original.nombre = "Licenciatura en Informatica";

        Materia m;
        m.nombre = "Algoritmos";
        m.horas_semanales = 8;
        m.requerimientos = {"IDE", "Pizarron"};

        original.materias = {m};

        QJsonObject json = original.toJson();
        PlanEstudio result = PlanEstudio::fromJson(json);

        QCOMPARE(result.nombre, original.nombre);
        QCOMPARE(result.materias.size(), original.materias.size());
        QCOMPARE(result.materias[0].nombre, original.materias[0].nombre);
        QCOMPARE(result.materias[0].horas_semanales, original.materias[0].horas_semanales);
        QCOMPARE(result.materias[0].requerimientos, original.materias[0].requerimientos);
    }
};

QTEST_MAIN(TestPlanEstudio)
#include "test_plan_estudio.moc"
