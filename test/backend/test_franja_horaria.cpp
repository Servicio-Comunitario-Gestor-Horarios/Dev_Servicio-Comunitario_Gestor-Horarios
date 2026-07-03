#include <QTest>
#include <QJsonObject>
#include <backend/data/franja_horaria.hpp>

class TestFranjaHoraria : public QObject {
    Q_OBJECT

private slots:
    void toJson_returnsCorrectFields() {
        FranjaHoraria f;
        f.dia = 1;
        f.inicio = QTime(8, 0);
        f.fin = QTime(10, 30);

        QJsonObject obj = f.toJson();

        QCOMPARE(obj["dia"].toInt(), 1);
        QCOMPARE(obj["inicio"].toString(), QString("08:00"));
        QCOMPARE(obj["fin"].toString(), QString("10:30"));
    }

    void fromJson_reconstructsObject() {
        QJsonObject obj;
        obj["dia"] = 3;
        obj["inicio"] = "09:00";
        obj["fin"] = "11:00";

        FranjaHoraria f = FranjaHoraria::fromJson(obj);

        QCOMPARE(f.dia, 3);
        QCOMPARE(f.inicio, QTime(9, 0));
        QCOMPARE(f.fin, QTime(11, 0));
    }

    void roundtrip_toJsonFromJson_identity() {
        FranjaHoraria original;
        original.dia = 5;
        original.inicio = QTime(14, 0);
        original.fin = QTime(15, 30);

        QJsonObject json = original.toJson();
        FranjaHoraria result = FranjaHoraria::fromJson(json);

        QCOMPARE(result.dia, original.dia);
        QCOMPARE(result.inicio, original.inicio);
        QCOMPARE(result.fin, original.fin);
    }
};

QTEST_MAIN(TestFranjaHoraria)
#include "test_franja_horaria.moc"
