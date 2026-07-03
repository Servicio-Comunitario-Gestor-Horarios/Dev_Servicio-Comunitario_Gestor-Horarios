#include <QTest>
#include <QJsonObject>
#include <backend/data/aula.hpp>

class TestAula : public QObject {
    Q_OBJECT

private slots:
    void toJson_returnsCorrectFields() {
        Aula a;
        a.nombre = "A-101";
        a.capacidad = 30;
        a.locacion = "Edificio A";

        QJsonObject obj = a.toJson();

        QCOMPARE(obj["nombre"].toString(), QString("A-101"));
        QCOMPARE(obj["capacidad"].toInt(), 30);
        QCOMPARE(obj["locacion"].toString(), QString("Edificio A"));
    }

    void fromJson_reconstructsObject() {
        QJsonObject obj;
        obj["nombre"] = "B-203";
        obj["capacidad"] = 45;
        obj["locacion"] = "Edificio B";

        Aula a = Aula::fromJson(obj);

        QCOMPARE(a.nombre, QString("B-203"));
        QCOMPARE(a.capacidad, 45);
        QCOMPARE(a.locacion, QString("Edificio B"));
    }

    void roundtrip_toJsonFromJson_identity() {
        Aula original;
        original.nombre = "Lab-1";
        original.capacidad = 20;
        original.locacion = "Laboratorios";

        QJsonObject json = original.toJson();
        Aula result = Aula::fromJson(json);

        QCOMPARE(result.nombre, original.nombre);
        QCOMPARE(result.capacidad, original.capacidad);
        QCOMPARE(result.locacion, original.locacion);
    }
};

QTEST_MAIN(TestAula)
#include "test_aula.moc"
