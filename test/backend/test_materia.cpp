#include <QTest>
#include <QJsonObject>
#include <QJsonArray>
#include <backend/data/materia.hpp>

class TestMateria : public QObject {
    Q_OBJECT

private slots:
    void toJson_returnsCorrectFields() {
        Materia m;
        m.nombre = "Matematicas";
        m.horas_semanales = 5;
        m.requerimientos = {"Pizarron", "Proyector"};

        QJsonObject obj = m.toJson();

        QCOMPARE(obj["nombre"].toString(), QString("Matematicas"));
        QCOMPARE(obj["horas_semanales"].toInt(), 5);

        QJsonArray reqs = obj["requerimientos"].toArray();
        QCOMPARE(reqs.size(), 2);
        QCOMPARE(reqs[0].toString(), QString("Pizarron"));
        QCOMPARE(reqs[1].toString(), QString("Proyector"));
    }

    void fromJson_reconstructsObject() {
        QJsonObject obj;
        obj["nombre"] = "Fisica";
        obj["horas_semanales"] = 4;
        obj["requerimientos"] = QJsonArray{"Laboratorio", "Computadora"};

        Materia m = Materia::fromJson(obj);

        QCOMPARE(m.nombre, QString("Fisica"));
        QCOMPARE(m.horas_semanales, 4);
        QCOMPARE(m.requerimientos.size(), 2);
        QCOMPARE(m.requerimientos[0], QString("Laboratorio"));
        QCOMPARE(m.requerimientos[1], QString("Computadora"));
    }

    void requirimientos_emptyWhenNotPresent() {
        QJsonObject obj;
        obj["nombre"] = "Historia";
        obj["horas_semanales"] = 3;

        Materia m = Materia::fromJson(obj);

        QVERIFY(m.requerimientos.isEmpty());
    }

    void roundtrip_toJsonFromJson_identity() {
        Materia original;
        original.nombre = "Programacion";
        original.horas_semanales = 6;
        original.requerimientos = {"IDE", "Compilador"};

        QJsonObject json = original.toJson();
        Materia result = Materia::fromJson(json);

        QCOMPARE(result.nombre, original.nombre);
        QCOMPARE(result.horas_semanales, original.horas_semanales);
        QCOMPARE(result.requerimientos, original.requerimientos);
    }
};

QTEST_MAIN(TestMateria)
#include "test_materia.moc"
