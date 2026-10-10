#include <QSet>
#include <QTest>

#include <backend/resultado.hpp>
#include <backend/services/CargadorConfiguracionSolver.hpp>
#include <backend/services/ServicioGeneracion.hpp>
#include <backend/services/ValidadorSalidaSolver.hpp>

using Estado = ResultadoGeneracion::Estado;

/// Test de contrato de la generación (RF-1, RF-3, RF-5): fija la semántica que
/// consume el equipo de frontend (estados, `Resultado<T>`, `AnalisisSalida`).
class TestContratoGeneracion : public QObject
{
    Q_OBJECT

private slots:

    void resultadoTieneSemanticaDeError()
    {
        const auto ok = Resultado<int>::exito(7);
        QVERIFY(ok.ok);
        QCOMPARE(ok.valor, 7);
        QVERIFY(ok.mensajeError.isEmpty());

        const auto err = Resultado<int>::error(QStringLiteral("motivo"), -42);
        QVERIFY(!err.ok);
        QCOMPARE(err.mensajeError, QStringLiteral("motivo"));
        QCOMPARE(err.codigoError, -42);
    }

    void estadosDeGeneracion_estanExponidos()
    {
        const QVector<Estado> estados = {
            Estado::Inactivo,        Estado::Calculando,      Estado::Listo,
            Estado::DatosAnteriores, Estado::NoFactible,      Estado::ContratoInvalido,
            Estado::CalculoFallido,  Estado::TiempoAgotado,   Estado::EntradaInvalida};

        QSet<int> unicos;
        for (Estado e : estados)
            unicos.insert(static_cast<int>(e));
        QCOMPARE(unicos.size(), estados.size());
    }

    void resultadoGeneracion_exponeSuContrato()
    {
        ResultadoGeneracion r;
        QCOMPARE(r.estado, Estado::Inactivo);
        QVERIFY(!r.presentable);
        QVERIFY(!r.datosAnteriores);
        QVERIFY(r.horario.horarios.isEmpty());
    }

    void analisisSalida_exponeAvisosYConflictos()
    {
        AnalisisSalida a;
        QVERIFY(a.presentable());
        QVERIFY(!a.hayConflictoSolapamiento());

        a.avisos.append(QStringLiteral("horas no cubiertas"));
        QVERIFY(a.presentable());  // un aviso no invalida el resultado

        a.conflictos.append(QStringLiteral("solapamiento"));
        QVERIFY(!a.presentable());
        QVERIFY(a.hayConflictoSolapamiento());
    }

    void serviciogeneracion_exponeElPlazoYElEstado()
    {
        // El frontend debe poder conectar la señal de fin de generación.
        QVERIFY(ServicioGeneracion::staticMetaObject.indexOfSignal("finalizada()") >= 0);
    }
};

QTEST_MAIN(TestContratoGeneracion)
#include "test_contrato_generacion.moc"
