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

    // ─── T1: PESO_AGRUPACION y funciones puras de agrupación ───

    void constantes_jerarquiaConAgrupacion()
    {
        using namespace ContratoSolver;
        // Jerarquía: 1000 > 100 > 10 > 0
        QVERIFY(PESO_HORAS > COSTE_SUPLENTE);
        QVERIFY(COSTE_SUPLENTE > PESO_AGRUPACION);
        QVERIFY(PESO_AGRUPACION > 0);
        QCOMPARE(PESO_AGRUPACION, 10);
    }

    // Helper: config de prueba con turnos y recesos reales
    static SolverConfig configPrueba()
    {
        SolverConfig cfg;
        cfg.turnos.turnos = {
            TurnoConfig{QStringLiteral("Mañana"), {0,1,2,3,4,5,6,7}},
            TurnoConfig{QStringLiteral("Tarde"), {8,9,10,11,12,13,14,15}}
        };
        cfg.recesos = {
            RecesoConfig{QStringLiteral("Mañana"), 2, 10, QStringLiteral("08:20"), QStringLiteral("08:30")},
            RecesoConfig{QStringLiteral("Tarde"), 2, 10, QStringLiteral("14:00"), QStringLiteral("14:10")}
        };
        return cfg;
    }

    // ─── segmentosTurno ───

    void segmentosTurno_mananaCortaEnSlot1()
    {
        const SolverConfig cfg = configPrueba();
        const TurnoConfig* manana = nullptr;
        for (const TurnoConfig& t : cfg.turnos.turnos)
            if (t.nombre == QStringLiteral("Mañana")) { manana = &t; break; }
        QVERIFY(manana);

        QVector<QVector<int>> segs = segmentosTurno(*manana, cfg.recesos);
        QCOMPARE(segs.size(), 2);
        QCOMPARE(segs[0], (QVector<int>{0,1}));
        QCOMPARE(segs[1], (QVector<int>{2,3,4,5,6,7}));
    }

    void segmentosTurno_tardeCortaEnSlot9()
    {
        const SolverConfig cfg = configPrueba();
        const TurnoConfig* tarde = nullptr;
        for (const TurnoConfig& t : cfg.turnos.turnos)
            if (t.nombre == QStringLiteral("Tarde")) { tarde = &t; break; }
        QVERIFY(tarde);

        QVector<QVector<int>> segs = segmentosTurno(*tarde, cfg.recesos);
        QCOMPARE(segs.size(), 2);
        QCOMPARE(segs[0], (QVector<int>{8,9}));
        QCOMPARE(segs[1], (QVector<int>{10,11,12,13,14,15}));
    }

    void segmentosTurno_sinRecesoUnSoloSegmento()
    {
        TurnoConfig t{QStringLiteral("Test"), {0,1,2,3,4,5,6,7}};
        QVector<RecesoConfig> recesosVacios;
        QVector<QVector<int>> segs = segmentosTurno(t, recesosVacios);
        QCOMPARE(segs.size(), 1);
        QCOMPARE(segs[0], (QVector<int>{0,1,2,3,4,5,6,7}));
    }

    void segmentosTurno_recesoAlFinalNoCreaVacio()
    {
        TurnoConfig t{QStringLiteral("Test"), {0,1,2,3,4,5,6,7}};
        QVector<RecesoConfig> recesos = {
            RecesoConfig{QStringLiteral("Test"), 8, 10, QString(), QString()}
        };
        QVector<QVector<int>> segs = segmentosTurno(t, recesos);
        QCOMPARE(segs.size(), 1);
        QCOMPARE(segs[0], (QVector<int>{0,1,2,3,4,5,6,7}));
    }

    // ─── esInicioSegmento ───

    void esInicioSegmento_primeraFranjaSiempreInicio()
    {
        const SolverConfig cfg = configPrueba();
        const TurnoConfig* manana = nullptr;
        const TurnoConfig* tarde = nullptr;
        for (const TurnoConfig& t : cfg.turnos.turnos) {
            if (t.nombre == QStringLiteral("Mañana")) manana = &t;
            else if (t.nombre == QStringLiteral("Tarde")) tarde = &t;
        }
        QVERIFY(manana);
        QVERIFY(tarde);

        QVector<QVector<int>> segsManana = segmentosTurno(*manana, cfg.recesos);
        QVector<QVector<int>> segsTarde = segmentosTurno(*tarde, cfg.recesos);

        QVERIFY(esInicioSegmento(0, segsManana));
        QVERIFY(esInicioSegmento(8, segsTarde));
    }

    void esInicioSegmento_despuesDeReceso()
    {
        const SolverConfig cfg = configPrueba();
        const TurnoConfig* manana = nullptr;
        const TurnoConfig* tarde = nullptr;
        for (const TurnoConfig& t : cfg.turnos.turnos) {
            if (t.nombre == QStringLiteral("Mañana")) manana = &t;
            else if (t.nombre == QStringLiteral("Tarde")) tarde = &t;
        }
        QVERIFY(manana);
        QVERIFY(tarde);

        QVector<QVector<int>> segsManana = segmentosTurno(*manana, cfg.recesos);
        QVector<QVector<int>> segsTarde = segmentosTurno(*tarde, cfg.recesos);

        // Mañana: receso tras 1 → slot 2 es inicio del 2º segmento
        QVERIFY(esInicioSegmento(2, segsManana));
        // Tarde: receso tras 9 → slot 10 es inicio
        QVERIFY(esInicioSegmento(10, segsTarde));
    }

    void esInicioSegmento_noInicioEnMedioDeSegmento()
    {
        const SolverConfig cfg = configPrueba();
        const TurnoConfig* manana = nullptr;
        const TurnoConfig* tarde = nullptr;
        for (const TurnoConfig& t : cfg.turnos.turnos) {
            if (t.nombre == QStringLiteral("Mañana")) manana = &t;
            else if (t.nombre == QStringLiteral("Tarde")) tarde = &t;
        }
        QVERIFY(manana);
        QVERIFY(tarde);

        QVector<QVector<int>> segsManana = segmentosTurno(*manana, cfg.recesos);
        QVector<QVector<int>> segsTarde = segmentosTurno(*tarde, cfg.recesos);

        // Mañana: 1,3,4,5,6,7 no son inicios
        QVERIFY(!esInicioSegmento(1, segsManana));
        QVERIFY(!esInicioSegmento(3, segsManana));
        QVERIFY(!esInicioSegmento(4, segsManana));
        // Tarde: 9 no (es la última del 1º segmento), 11-15 no
        QVERIFY(!esInicioSegmento(9, segsTarde));
        QVERIFY(!esInicioSegmento(11, segsTarde));
    }

    // ─── calcularPenalizacionGrupo ───

    void calcularPenalizacionGrupo_tablaRF2()
    {
        // | tamaño | horasDía | penalización |
        // | 1      | 1        | 0 (D-4)    |
        // | 1      | >1       | 1          |
        // | 2,3    | cualquier| 0          |
        // | 4      | cualquier| 1 (4-3)    |
        // | 5      | cualquier| 2 (5-3)    |
        // | 8      | cualquier| 5 (8-3)    |

        QCOMPARE(calcularPenalizacionGrupo(1, 1), 0);  // D-4: 1h/día perdonado
        QCOMPARE(calcularPenalizacionGrupo(1, 2), 1);  // grupo 1, horas día > 1
        QCOMPARE(calcularPenalizacionGrupo(1, 3), 1);
        QCOMPARE(calcularPenalizacionGrupo(1, 5), 1);

        QCOMPARE(calcularPenalizacionGrupo(2, 1), 0);  // 2-3 = ideal
        QCOMPARE(calcularPenalizacionGrupo(2, 2), 0);
        QCOMPARE(calcularPenalizacionGrupo(3, 1), 0);
        QCOMPARE(calcularPenalizacionGrupo(3, 5), 0);

        QCOMPARE(calcularPenalizacionGrupo(4, 1), 1);  // 4-3
        QCOMPARE(calcularPenalizacionGrupo(4, 4), 1);
        QCOMPARE(calcularPenalizacionGrupo(5, 2), 2);  // 5-3
        QCOMPARE(calcularPenalizacionGrupo(8, 8), 5);  // 8-3
    }

    void calcularPenalizacionGrupo_tamanoCeroDevuelveCero()
    {
        QCOMPARE(calcularPenalizacionGrupo(0, 1), 0);
        QCOMPARE(calcularPenalizacionGrupo(0, 5), 0);
    }
};

QTEST_MAIN(TestContratoSolver)
#include "test_contrato_solver.moc"
