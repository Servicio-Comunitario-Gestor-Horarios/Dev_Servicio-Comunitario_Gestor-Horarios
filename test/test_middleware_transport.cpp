#include <QCoreApplication>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QPointer>
#include <QSignalSpy>
#include <QTimer>
#include <QDebug>

#include <middleware/internalserver.h>
#include <middleware/internalclient.h>
#include <middleware/messages.h>

#include <QtTest>

class TestMiddlewareTransport : public QObject {
    Q_OBJECT
private:
    InternalServer *m_server = nullptr;

    static QByteArray frame(const QJsonObject& obj) {
        return QJsonDocument(obj).toJson(QJsonDocument::Compact) + '\n';
    }

    // El servidor IPC corre en el mismo hilo, así que hay que procesar eventos
    // (QTest::qWait) para que atienda la petición. Un waitForReadyRead bloqueante
    // sobre el socket crudo no atiende al servidor y termina sin respuestas.
    static QVector<QJsonObject> leerRespuestas(QLocalSocket& socket, int esperadas,
                                               int timeoutMs = 3000) {
        QVector<QJsonObject> out;
        QByteArray buffer;

        auto pump = [&]() {
            buffer.append(socket.readAll());
            int idx = -1;
            while ((idx = buffer.indexOf('\n')) >= 0) {
                const QByteArray line = buffer.left(idx);
                buffer.remove(0, idx + 1);
                if (line.isEmpty()) continue;
                const QJsonDocument doc = QJsonDocument::fromJson(line);
                if (doc.isObject()) out.append(doc.object());
            }
        };

        QElapsedTimer t; t.start();
        while (out.size() < esperadas && t.elapsed() < timeoutMs) {
            QTest::qWait(10);
            pump();
        }
        pump();
        return out;
    }

private slots:
    void initTestCase();
    void cleanupTestCase();

    void healthCheck();
    void ready();
    void shutdown();
    void echoRoute();
    void invalidOp();
    void malformedJson();
    void versionIncompatible();
    void framedFragmentado();
    void framedConcatenado();
    void legacyDataKey();
    void clienteEncola();
    void servidorSerializa();
    void timeout();
};

void TestMiddlewareTransport::initTestCase()
{
    m_server = new InternalServer(this);
    QVERIFY(m_server->start());

    m_server->registerRoute(QStringLiteral("echo"),
        [this](const QJsonObject& p, QLocalSocket* s) {
            m_server->responder(s, Middleware::RESP_EXITO, p);
        });

    m_server->registerRoute(Middleware::OP_LISTA_PROFESORES,
        [this](const QJsonObject&, QLocalSocket* s) {
            m_server->responder(s, Middleware::RESP_EXITO, QJsonArray());
        });
}

void TestMiddlewareTransport::cleanupTestCase()
{
    delete m_server;
    m_server = nullptr;
}

void TestMiddlewareTransport::healthCheck()
{
    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);
    QVERIFY(spy.isValid());

    client.enviarSolicitud(Middleware::OP_HEALTH_CHECK);
    QVERIFY(spy.wait(3000));

    const QJsonObject r = spy.at(0).at(0).toJsonObject();
    QCOMPARE(r["status"].toString(), QString("ok"));
    QCOMPARE(r["code"].toInt(), Middleware::RESP_EXITO);
    QCOMPARE(r["data"].toString(), QString("ok"));
}

void TestMiddlewareTransport::ready()
{
    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);

    client.enviarSolicitud(Middleware::OP_LISTO);
    QVERIFY(spy.wait(3000));

    const QJsonObject r = spy.at(0).at(0).toJsonObject();
    QCOMPARE(r["status"].toString(), QString("ok"));
    QCOMPARE(r["code"].toInt(), Middleware::RESP_EXITO);
}

void TestMiddlewareTransport::shutdown()
{
    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);

    client.enviarSolicitud(Middleware::OP_APAGAR);
    QVERIFY(spy.wait(3000));

    const QJsonObject r = spy.at(0).at(0).toJsonObject();
    QCOMPARE(r["status"].toString(), QString("ok"));
    QCOMPARE(r["code"].toInt(), Middleware::RESP_EXITO);
}

void TestMiddlewareTransport::echoRoute()
{
    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);

    QJsonObject payload;
    payload["valor"] = 42;
    client.enviarSolicitud("echo", payload);
    QVERIFY(spy.wait(3000));

    const QJsonObject r = spy.at(0).at(0).toJsonObject();
    QCOMPARE(r["status"].toString(), QString("ok"));
    QCOMPARE(r["data"].toObject()["valor"].toInt(), 42);
}

void TestMiddlewareTransport::invalidOp()
{
    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);

    client.enviarSolicitud("operacion_inexistente");
    QVERIFY(spy.wait(3000));

    const QJsonObject r = spy.at(0).at(0).toJsonObject();
    QCOMPARE(r["status"].toString(), QString("error"));
    QCOMPARE(r["code"].toInt(), Middleware::RESP_INVALIDO);
}

void TestMiddlewareTransport::malformedJson()
{
    QLocalSocket socket;
    socket.connectToServer(m_server->serverName());
    QVERIFY(socket.waitForConnected(3000));

    socket.write("esto no es json\n");
    socket.flush();

    const QVector<QJsonObject> resp = leerRespuestas(socket, 1);
    QCOMPARE(resp.size(), 1);
    QCOMPARE(resp.at(0)["code"].toInt(), Middleware::RESP_INVALIDO);
    socket.disconnectFromServer();
}

void TestMiddlewareTransport::versionIncompatible()
{
    QLocalSocket socket;
    socket.connectToServer(m_server->serverName());
    QVERIFY(socket.waitForConnected(3000));

    QJsonObject msg;
    msg["v"] = 999;
    msg["op"] = Middleware::OP_HEALTH_CHECK;
    socket.write(frame(msg));
    socket.flush();

    const QVector<QJsonObject> resp = leerRespuestas(socket, 1);
    QCOMPARE(resp.size(), 1);
    QCOMPARE(resp.at(0)["code"].toInt(), Middleware::RESP_VERSION_INCOMPATIBLE);
    socket.disconnectFromServer();
}

void TestMiddlewareTransport::framedFragmentado()
{
    QLocalSocket socket;
    socket.connectToServer(m_server->serverName());
    QVERIFY(socket.waitForConnected(3000));

    QJsonObject msg;
    msg["v"] = Middleware::PROTOCOL_VERSION;
    msg["op"] = "echo";
    QJsonObject payload; payload["n"] = 7;
    msg["payload"] = payload;

    const QByteArray f = frame(msg);
    socket.write(f.left(f.size() / 2));
    socket.flush();
    QTest::qWait(50);
    socket.write(f.mid(f.size() / 2));
    socket.flush();

    const QVector<QJsonObject> resp = leerRespuestas(socket, 1);
    QCOMPARE(resp.size(), 1);
    QCOMPARE(resp.at(0)["code"].toInt(), Middleware::RESP_EXITO);
    QCOMPARE(resp.at(0)["data"].toObject()["n"].toInt(), 7);
    socket.disconnectFromServer();
}

void TestMiddlewareTransport::framedConcatenado()
{
    QLocalSocket socket;
    socket.connectToServer(m_server->serverName());
    QVERIFY(socket.waitForConnected(3000));

    QJsonObject a; a["v"] = 1; a["op"] = "echo"; QJsonObject pa; pa["n"] = 1; a["payload"] = pa;
    QJsonObject b; b["v"] = 1; b["op"] = "echo"; QJsonObject pb; pb["n"] = 2; b["payload"] = pb;

    QByteArray buf = frame(a);
    buf.append(frame(b));
    socket.write(buf);
    socket.flush();

    const QVector<QJsonObject> resp = leerRespuestas(socket, 2);
    QCOMPARE(resp.size(), 2);
    QCOMPARE(resp.at(0)["data"].toObject()["n"].toInt(), 1);
    QCOMPARE(resp.at(1)["data"].toObject()["n"].toInt(), 2);
    socket.disconnectFromServer();
}

void TestMiddlewareTransport::legacyDataKey()
{
    QLocalSocket socket;
    socket.connectToServer(m_server->serverName());
    QVERIFY(socket.waitForConnected(3000));

    QJsonObject solicitud;
    solicitud["v"] = Middleware::PROTOCOL_VERSION;
    solicitud["op"] = Middleware::OP_LISTA_PROFESORES;
    solicitud["data"] = QJsonObject();  // clave legada

    socket.write(frame(solicitud));
    socket.flush();

    const QVector<QJsonObject> resp = leerRespuestas(socket, 1);
    QCOMPARE(resp.size(), 1);
    QCOMPARE(resp.at(0)["status"].toString(), QString("ok"));
    socket.disconnectFromServer();
}

void TestMiddlewareTransport::clienteEncola()
{
    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);
    QVERIFY(spy.isValid());

    client.enviarSolicitud(Middleware::OP_HEALTH_CHECK);
    client.enviarSolicitud(Middleware::OP_LISTO);
    client.enviarSolicitud(Middleware::OP_APAGAR);

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 3, 3000);

    QCOMPARE(spy.at(0).at(0).toJsonObject()["op"].toString(), Middleware::OP_HEALTH_CHECK);
    QCOMPARE(spy.at(1).at(0).toJsonObject()["op"].toString(), Middleware::OP_LISTO);
    QCOMPARE(spy.at(2).at(0).toJsonObject()["op"].toString(), Middleware::OP_APAGAR);
}

void TestMiddlewareTransport::servidorSerializa()
{
    m_server->registerRoute(QStringLiteral("slow_op"),
        [this](const QJsonObject&, QLocalSocket* s) {
            QPointer<QLocalSocket> guard(s);
            QTimer::singleShot(300, this, [this, guard]() {
                if (guard)
                    m_server->responder(guard, Middleware::RESP_EXITO, "lento");
            });
        });

    QLocalSocket socket;
    socket.connectToServer(m_server->serverName());
    QVERIFY(socket.waitForConnected(3000));

    QJsonObject a; a["v"] = 1; a["op"] = "slow_op";
    QJsonObject b; b["v"] = 1; b["op"] = "echo"; QJsonObject pb; pb["n"] = 9; b["payload"] = pb;

    QByteArray buf = frame(a);
    buf.append(frame(b));
    socket.write(buf);
    socket.flush();

    const QVector<QJsonObject> resp = leerRespuestas(socket, 2);
    QCOMPARE(resp.size(), 2);
    QCOMPARE(resp.at(0)["op"].toString(), QString("slow_op"));
    QCOMPARE(resp.at(1)["op"].toString(), QString("echo"));
    socket.disconnectFromServer();
}

void TestMiddlewareTransport::timeout()
{
    m_server->setTimeoutMs(100);

    m_server->registerRoute(QStringLiteral("very_slow_op"),
        [this](const QJsonObject&, QLocalSocket* s) {
            QPointer<QLocalSocket> guard(s);
            QTimer::singleShot(1000, this, [this, guard]() {
                if (guard)
                    m_server->responder(guard, Middleware::RESP_EXITO, "tarde");
            });
        });

    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);

    client.enviarSolicitud("very_slow_op");

    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 2000);
    QCOMPARE(spy.at(0).at(0).toJsonObject()["code"].toInt(),
             Middleware::RESP_TIEMPO_AGOTADO);

    // La respuesta tardía no debe duplicar
    QTest::qWait(1200);
    QCOMPARE(spy.count(), 1);

    m_server->setTimeoutMs(5000);
}

QTEST_MAIN(TestMiddlewareTransport)
#include "test_middleware_transport.moc"
