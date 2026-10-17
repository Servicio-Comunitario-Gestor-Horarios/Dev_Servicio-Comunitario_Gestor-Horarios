#include <QTest>
#include <QString>

#include <backend/solver/config/contrato_solver.hpp>

/// Tests de las funciones puras y constantes del contrato (RF-1, RF-3, RF-5).
class TestContratoSolver : public QObject
{
    Q_OBJECT

private slots:

    void bloques_redondeaHaciaArriba()
    {
        QCOMPARE(bloques(2, 60), 2);   // 120/60
        QCOMPARE(bloques(3, 45), 4);   // 180/45
        QCOMPARE(bloques(2, 45), 3);   // 120/45 = 2.66 -> 3
        QCOMPARE(bloques(1, 60), 1);
    }

    void bloques_duracionInvalida_devuelveCero()
    {
        QCOMPARE(bloques(2, 0), 0);
        QCOMPARE(bloques(2, -30), 0);
    }

    void bloquesAula_igualQueBloques()
    {
        QCOMPARE(bloquesAula(8, 45), bloques(8, 45));
        QCOMPARE(bloquesAula(8, 0), 0);
    }

    void claveCurso_canonicaYDeterminista()
    {
        QCOMPARE(claveCurso(1, QStringLiteral("A")), claveCurso(1, QStringLiteral("A")));
        QVERIFY(claveCurso(1, QStringLiteral("A")) != claveCurso(2, QStringLiteral("A")));
        QVERIFY(claveCurso(1, QStringLiteral("A")) != claveCurso(1, QStringLiteral("B")));
        QVERIFY(!claveCurso(1, QStringLiteral("A")).isEmpty());
    }

    void constantes_jerarquiaDelObjetivo()
    {
        QVERIFY(ContratoSolver::PESO_HORAS > ContratoSolver::COSTE_SUPLENTE);
        QVERIFY(ContratoSolver::COSTE_SUPLENTE > 0);
    }
};

QTEST_MAIN(TestContratoSolver)
#include "test_contrato_solver.moc"
