#include <QTest>
#include <QString>

#include <backend/solver/config/contrato_solver.hpp>

/// Tests de las funciones puras y constantes del contrato (RF-1, RF-3, RF-5).
class TestContratoSolver : public QObject
{
    Q_OBJECT

private slots:

    void bloques_esIdentidad()
    {
        // Rectificación (2026-10-17): 1 hora académica = 1 bloque. `duracion_minutos` no
        // interviene en la conversión; no hay multiplicación por 60 ni redondeo.
        QCOMPARE(bloques(3, 45), 3);   // antes 4 (ceil(180/45))
        QCOMPARE(bloques(2, 45), 2);   // antes 3 (ceil(120/45))
        QCOMPARE(bloques(4, 40), 4);
        QCOMPARE(bloques(6, 60), 6);
    }

    void bloques_duracionInvalida_devuelveCero()
    {
        QCOMPARE(bloques(2, 0), 0);
        QCOMPARE(bloques(2, -30), 0);
    }

    void bloquesAula_esIdentidad()
    {
        // El tope del docente se mide en bloques = horas_aula (S1), sin ×60 ni redondeo.
        QCOMPARE(bloquesAula(8, 45), 8);
        QCOMPARE(bloquesAula(8, 0), 0);
        QCOMPARE(bloquesAula(8, 45), bloques(8, 45));
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
