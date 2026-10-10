#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QLocalServer>
#include <QSignalSpy>
#include <QString>
#include <QTest>
#include <QUuid>

#include <atomic>
#include <thread>

#include "app/instancia_unica.hpp"

using Resultado = InstanciaUnica::Resultado;

/// Tests de la instancia única del cliente (RF-5).
///
/// Comprueban el protocolo entre instancias: la principal queda a la escucha y
/// enfoca (emite `activarSolicitada`) cuando otra pide activar; la secundaria
/// sale con acuse o avisa si no lo recibe en plazo. La detección sigue activa
/// durante toda la sesión, incluida la migración del arranque.
class TestInstanciaUnica : public QObject
{
    Q_OBJECT

private:
    /// Nombre único por caso para no interferir entre tests ni con la app real.
    QString nombreUnico() const
    {
        return QStringLiteral("GestorHorarios_Test_%1")
            .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    }

private slots:

    // ─── Segundo arranque enfoca al primero ────────────────────────────────

    void segundoArranque_enfocaAlPrimero()
    {
        const QString nombre = nombreUnico();

        InstanciaUnica primaria(nombre);
        QCOMPARE(primaria.iniciar(), Resultado::Primaria);
        QVERIFY(primaria.estaActiva());

        QSignalSpy spy(&primaria, &InstanciaUnica::activarSolicitada);

        // La secundaria corre en otro hilo para que el bucle de eventos
        // principal pueda atender la conexión de la primaria.
        std::atomic<Resultado> resultadoSecundaria{Resultado::SinAcuse};
        std::thread hilo([&]() {
            InstanciaUnica secundaria(nombre, 3000);
            resultadoSecundaria.store(secundaria.iniciar());
        });

        QTRY_VERIFY_WITH_TIMEOUT(spy.count() >= 1, 5000);
        hilo.join();

        // La secundaria recibió el acuse y sale enfocando a la primaria.
        QCOMPARE(resultadoSecundaria.load(), Resultado::Secundaria);
        // La primaria sigue a la escucha tras atender la solicitud.
        QVERIFY(primaria.estaActiva());
    }

    // ─── El enfoque conectado se invoca (cableado de la ventana) ───────────

    void enfoque_receptorConectado_seInvoca()
    {
        const QString nombre = nombreUnico();

        InstanciaUnica primaria(nombre);
        QCOMPARE(primaria.iniciar(), Resultado::Primaria);

        // Equivale al `connect` que hace el arranque del cliente con la ventana.
        int enfoques = 0;
        QObject::connect(&primaria, &InstanciaUnica::activarSolicitada, [&enfoques]() { ++enfoques; });

        std::atomic<Resultado> resultadoSecundaria{Resultado::SinAcuse};
        std::thread hilo([&]() {
            InstanciaUnica secundaria(nombre, 3000);
            resultadoSecundaria.store(secundaria.iniciar());
        });

        QTRY_VERIFY_WITH_TIMEOUT(enfoques >= 1, 5000);
        hilo.join();

        QCOMPARE(resultadoSecundaria.load(), Resultado::Secundaria);
        QVERIFY(primaria.estaActiva());
    }

    // ─── Sin acuse en el plazo → avisa y no arranca ────────────────────────

    void sinAcuse_enElPlazo_avisaYNoArranca()
    {
        const QString nombre = nombreUnico();

        // Instancia «viva» que acepta la conexión pero nunca responde el acuse.
        QLocalServer servidorMudo;
        QVERIFY(servidorMudo.listen(nombre));

        InstanciaUnica secundaria(nombre, 200); // plazo corto para el test
        const Resultado resultado = secundaria.iniciar();

        QCOMPARE(resultado, Resultado::SinAcuse);
        QVERIFY(!secundaria.esPrimaria());
        // Se avisa con un detalle legible en español (RNF-1).
        QVERIFY(!secundaria.detalle().isEmpty());

        servidorMudo.close();
    }

    // ─── Detección activa durante la migración ─────────────────────────────

    void deteccion_activaDuranteLaMigracion()
    {
        const QString nombre = nombreUnico();

        InstanciaUnica primaria(nombre);
        QCOMPARE(primaria.iniciar(), Resultado::Primaria);

        QSignalSpy spy(&primaria, &InstanciaUnica::activarSolicitada);

        // Simula la fase de migración del arranque: la instancia principal está
        // ocupada en una operación larga (aquí, bombeando eventos) mientras la
        // detección debe seguir atendiendo solicitudes.
        std::atomic<Resultado> resultadoSecundaria{Resultado::SinAcuse};
        std::thread hilo([&]() {
            InstanciaUnica secundaria(nombre, 3000);
            resultadoSecundaria.store(secundaria.iniciar());
        });

        QElapsedTimer reloj;
        reloj.start();
        while (spy.isEmpty() && reloj.elapsed() < 4000)
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

        hilo.join();

        QVERIFY(spy.count() >= 1);
        QCOMPARE(resultadoSecundaria.load(), Resultado::Secundaria);
        // Tras la ventana de migración la detección continúa activa.
        QVERIFY(primaria.estaActiva());
    }
};

QTEST_MAIN(TestInstanciaUnica)
#include "test_instancia_unica.moc"
