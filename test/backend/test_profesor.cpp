#include <QTest>
#include <QJsonObject>
#include <QJsonArray>
#include <backend/data/profesor.hpp>

class TestProfesor : public QObject {
    Q_OBJECT

private slots:
    void toJson_returnsCorrectFields() {
        Profesor p;
        p.nombre = "Dr. Garcia";

        FranjaHoraria f1;
        f1.dia = 1; f1.inicio = QTime(8,0); f1.fin = QTime(12,0);
        p.disponibilidad = {f1};
        p.materias = {"Matematicas", "Fisica"};

        QJsonObject obj = p.toJson();

        QCOMPARE(obj["nombre"].toString(), QString("Dr. Garcia"));

        QJsonArray disp = obj["disponibilidad"].toArray();
        QCOMPARE(disp.size(), 1);

        QJsonArray mats = obj["materias"].toArray();
        QCOMPARE(mats.size(), 2);
        QCOMPARE(mats[0].toString(), QString("Matematicas"));
    }

    void fromJson_reconstructsObject() {
        QJsonObject f;
        f["dia"] = 2; f["inicio"] = "14:00"; f["fin"] = "18:00";

        QJsonObject obj;
        obj["nombre"] = "Dra. Lopez";
        obj["disponibilidad"] = QJsonArray{f};
        obj["materias"] = QJsonArray{"Quimica", "Biologia"};

        Profesor p = Profesor::fromJson(obj);

        QCOMPARE(p.nombre, QString("Dra. Lopez"));
        QCOMPARE(p.disponibilidad.size(), 1);
        QCOMPARE(p.disponibilidad[0].dia, 2);
        QCOMPARE(p.materias.size(), 2);
    }

    void disponibilidad_emptyWhenNotPresent() {
        QJsonObject obj;
        obj["nombre"] = "Ing. Martinez";
        obj["materias"] = QJsonArray{"Algebra"};

        Profesor p = Profesor::fromJson(obj);

        QVERIFY(p.disponibilidad.isEmpty());
        QCOMPARE(p.materias.size(), 1);
    }

    void roundtrip_toJsonFromJson_identity() {
        Profesor original;
        original.nombre = "Dr. Perez";

        FranjaHoraria f;
        f.dia = 3; f.inicio = QTime(7,0); f.fin = QTime(9,0);
        original.disponibilidad = {f};
        original.materias = {"Calculo", "Algebra", "Geometria"};

        QJsonObject json = original.toJson();
        Profesor result = Profesor::fromJson(json);

        QCOMPARE(result.nombre, original.nombre);
        QCOMPARE(result.disponibilidad.size(), original.disponibilidad.size());
        QCOMPARE(result.disponibilidad[0].dia, original.disponibilidad[0].dia);
        QCOMPARE(result.materias, original.materias);
    }
};

QTEST_MAIN(TestProfesor)
#include "test_profesor.moc"
