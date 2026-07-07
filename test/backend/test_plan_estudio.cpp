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

        MateriaEnPlan mep1, mep2;
        mep1.curso = 1;
        mep1.horas = 6;
        mep1.materia = m1;
        mep2.curso = 2;
        mep2.horas = 4;
        mep2.materia = m2;

        plan.materias = {mep1, mep2};

        QJsonObject obj = plan.toJson();

        QCOMPARE(obj["nombre"].toString(), QString("Ingenieria en Sistemas"));
        QJsonArray materiasArr = obj["materias"].toArray();
        QCOMPARE(materiasArr.size(), 2);

        QJsonObject primera = materiasArr[0].toObject();
        QCOMPARE(primera["curso"].toInt(), 1);
        QCOMPARE(primera["horas"].toInt(), 6);
        QCOMPARE(primera["materia"].toObject()["nombre"].toString(), QString("Programacion"));

        QJsonObject segunda = materiasArr[1].toObject();
        QCOMPARE(segunda["materia"].toObject()["requerimientos"].toArray()[0].toString(), QString("Laboratorio"));
    }

    void fromJson_reconstructsObject() {
        QJsonObject m1, m2;
        m1["nombre"] = "Redes"; m1["horas_semanales"] = 5;
        m2["nombre"] = "Seguridad"; m2["horas_semanales"] = 4;

        QJsonObject mep1, mep2;
        mep1["curso"] = 1; mep1["horas"] = 5; mep1["materia"] = m1;
        mep2["curso"] = 2; mep2["horas"] = 4; mep2["materia"] = m2;

        QJsonObject obj;
        obj["nombre"] = "Ingenieria en Computacion";
        obj["materias"] = QJsonArray{mep1, mep2};

        PlanEstudio plan = PlanEstudio::fromJson(obj);

        QCOMPARE(plan.nombre, QString("Ingenieria en Computacion"));
        QCOMPARE(plan.materias.size(), 2);
        QCOMPARE(plan.materias[0].curso, 1);
        QCOMPARE(plan.materias[0].horas, 5);
        QCOMPARE(plan.materias[0].materia.nombre, QString("Redes"));
        QCOMPARE(plan.materias[1].materia.nombre, QString("Seguridad"));
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

        MateriaEnPlan mep;
        mep.curso = 1;
        mep.horas = 8;
        mep.materia = m;

        original.materias = {mep};

        QJsonObject json = original.toJson();
        PlanEstudio result = PlanEstudio::fromJson(json);

        QCOMPARE(result.nombre, original.nombre);
        QCOMPARE(result.materias.size(), original.materias.size());
        QCOMPARE(result.materias[0].curso, original.materias[0].curso);
        QCOMPARE(result.materias[0].horas, original.materias[0].horas);
        QCOMPARE(result.materias[0].materia.nombre, original.materias[0].materia.nombre);
        QCOMPARE(result.materias[0].materia.horas_semanales, original.materias[0].materia.horas_semanales);
        QCOMPARE(result.materias[0].materia.requerimientos, original.materias[0].materia.requerimientos);
    }
};

QTEST_MAIN(TestPlanEstudio)
#include "test_plan_estudio.moc"
