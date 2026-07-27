#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QLocalSocket>

#include <middleware/internalserver.h>
#include <middleware/internalclient.h>
#include <middleware/messages.h>

#include <QtTest>

class TestMiddlewareCrudRoutes : public QObject {
    Q_OBJECT
private:
    InternalServer *m_server = nullptr;

private slots:
    void initTestCase();
    void cleanupTestCase();

    void healthCheck();
    void ready();
    void shutdown();
    void teacherList();
    void subjectList();
    void classroomList();
    void classroomGet();
    void classroomGetMissingId();
    void invalidOp();
    void teacherCreateValid();
    void teacherCreateMissingFields();
    void malformedJson();
    void legacyDataKey();
    void timeout();
};

void TestMiddlewareCrudRoutes::initTestCase()
{
    m_server = new InternalServer(this);
    QVERIFY(m_server->start());
}

void TestMiddlewareCrudRoutes::cleanupTestCase()
{
    delete m_server;
    m_server = nullptr;
}

void TestMiddlewareCrudRoutes::healthCheck()
{
    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);
    QVERIFY(spy.isValid());

    client.enviarSolicitud(Middleware::OP_HEALTH_CHECK);
    QVERIFY(spy.wait(3000));

    QJsonObject respuesta = spy.at(0).at(0).toJsonObject();
    QCOMPARE(respuesta["status"].toString(), QString("ok"));
    QCOMPARE(respuesta["code"].toInt(), Middleware::RESP_EXITO);
    QCOMPARE(respuesta["data"].toString(), QString("ok"));
}

void TestMiddlewareCrudRoutes::ready()
{
    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);
    QVERIFY(spy.isValid());

    client.enviarSolicitud(Middleware::OP_LISTO);
    QVERIFY(spy.wait(3000));

    QJsonObject respuesta = spy.at(0).at(0).toJsonObject();
    QCOMPARE(respuesta["status"].toString(), QString("ok"));
    QCOMPARE(respuesta["code"].toInt(), Middleware::RESP_EXITO);
    QCOMPARE(respuesta["data"].toString(), QString("ok"));
}

void TestMiddlewareCrudRoutes::shutdown()
{
    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);
    QVERIFY(spy.isValid());

    client.enviarSolicitud(Middleware::OP_APAGAR);
    QVERIFY(spy.wait(3000));

    QJsonObject respuesta = spy.at(0).at(0).toJsonObject();
    QCOMPARE(respuesta["status"].toString(), QString("ok"));
    QCOMPARE(respuesta["code"].toInt(), Middleware::RESP_EXITO);
    QCOMPARE(respuesta["data"].toString(), QString("ok"));
}

void TestMiddlewareCrudRoutes::teacherList()
{
    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);
    QVERIFY(spy.isValid());

    client.enviarSolicitud(Middleware::OP_LISTA_PROFESORES);
    QVERIFY(spy.wait(3000));

    QJsonObject respuesta = spy.at(0).at(0).toJsonObject();
    QCOMPARE(respuesta["status"].toString(), QString("ok"));
    QCOMPARE(respuesta["code"].toInt(), Middleware::RESP_EXITO);
    QVERIFY(respuesta["data"].isArray());
}

void TestMiddlewareCrudRoutes::subjectList()
{
    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);
    QVERIFY(spy.isValid());

    client.enviarSolicitud(Middleware::OP_LISTA_MATERIAS);
    QVERIFY(spy.wait(3000));

    QJsonObject respuesta = spy.at(0).at(0).toJsonObject();
    QCOMPARE(respuesta["status"].toString(), QString("ok"));
    QCOMPARE(respuesta["code"].toInt(), Middleware::RESP_EXITO);
    QVERIFY(respuesta["data"].isArray());
}

void TestMiddlewareCrudRoutes::classroomList()
{
    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);
    QVERIFY(spy.isValid());

    client.enviarSolicitud(Middleware::OP_LISTA_AULAS);
    QVERIFY(spy.wait(3000));

    QJsonObject respuesta = spy.at(0).at(0).toJsonObject();
    QCOMPARE(respuesta["status"].toString(), QString("ok"));
    QCOMPARE(respuesta["code"].toInt(), Middleware::RESP_EXITO);
    QVERIFY(respuesta["data"].isArray());
}

void TestMiddlewareCrudRoutes::classroomGet()
{
    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);
    QVERIFY(spy.isValid());

    QJsonObject payload;
    payload["id"] = "aula-1";
    client.enviarSolicitud(Middleware::OP_OBTENER_AULA, payload);
    QVERIFY(spy.wait(3000));

    QJsonObject respuesta = spy.at(0).at(0).toJsonObject();
    QCOMPARE(respuesta["status"].toString(), QString("error"));
    QCOMPARE(respuesta["code"].toInt(), Middleware::RESP_NO_ENCONTRADO);
}

void TestMiddlewareCrudRoutes::classroomGetMissingId()
{
    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);
    QVERIFY(spy.isValid());

    QJsonObject payload;
    client.enviarSolicitud(Middleware::OP_OBTENER_AULA, payload);
    QVERIFY(spy.wait(3000));

    QJsonObject respuesta = spy.at(0).at(0).toJsonObject();
    QCOMPARE(respuesta["status"].toString(), QString("error"));
    QCOMPARE(respuesta["code"].toInt(), Middleware::RESP_INVALIDO);
}

void TestMiddlewareCrudRoutes::invalidOp()
{
    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);
    QVERIFY(spy.isValid());

    client.enviarSolicitud("operacion_inexistente");
    QVERIFY(spy.wait(3000));

    QJsonObject respuesta = spy.at(0).at(0).toJsonObject();
    QCOMPARE(respuesta["status"].toString(), QString("error"));
    QCOMPARE(respuesta["code"].toInt(), Middleware::RESP_INVALIDO);
}

void TestMiddlewareCrudRoutes::teacherCreateValid()
{
    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);
    QVERIFY(spy.isValid());

    QJsonObject payload;
    payload["nombre"] = "Prof. Ejemplo";
    payload["correo"] = "ejemplo@universidad.edu";
    client.enviarSolicitud(Middleware::OP_CREAR_PROFESOR, payload);
    QVERIFY(spy.wait(3000));

    QJsonObject respuesta = spy.at(0).at(0).toJsonObject();
    QCOMPARE(respuesta["status"].toString(), QString("ok"));
    QCOMPARE(respuesta["code"].toInt(), Middleware::RESP_EXITO);
}

void TestMiddlewareCrudRoutes::teacherCreateMissingFields()
{
    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);
    QVERIFY(spy.isValid());

    QJsonObject payload;
    payload["nombre"] = "Prof. SinCorreo";
    client.enviarSolicitud(Middleware::OP_CREAR_PROFESOR, payload);
    QVERIFY(spy.wait(3000));

    QJsonObject respuesta = spy.at(0).at(0).toJsonObject();
    QCOMPARE(respuesta["status"].toString(), QString("error"));
    QCOMPARE(respuesta["code"].toInt(), Middleware::RESP_INVALIDO);
}

void TestMiddlewareCrudRoutes::malformedJson()
{
    QLocalSocket socket;
    socket.connectToServer(m_server->serverName());
    QVERIFY(socket.waitForConnected(3000));

    socket.write("esto no es json");
    socket.flush();

    QSignalSpy spy(&socket, &QLocalSocket::readyRead);
    QVERIFY(spy.wait(3000));

    QByteArray data = socket.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject respuesta = doc.object();
    QCOMPARE(respuesta["status"].toString(), QString("error"));
    QCOMPARE(respuesta["code"].toInt(), Middleware::RESP_INVALIDO);
    socket.disconnectFromServer();
}

void TestMiddlewareCrudRoutes::legacyDataKey()
{
    QLocalSocket socket;
    socket.connectToServer(m_server->serverName());
    QVERIFY(socket.waitForConnected(3000));

    QJsonObject solicitud;
    solicitud["op"] = Middleware::OP_LISTA_PROFESORES;
    solicitud["data"] = QJsonObject();  // legacy key

    QJsonDocument doc(solicitud);
    socket.write(doc.toJson(QJsonDocument::Compact));
    socket.flush();

    QSignalSpy spy(&socket, &QLocalSocket::readyRead);
    QVERIFY(spy.wait(3000));

    QByteArray data = socket.readAll();
    QJsonDocument respuestaDoc = QJsonDocument::fromJson(data);
    QJsonObject respuesta = respuestaDoc.object();
    QCOMPARE(respuesta["status"].toString(), QString("ok"));
    QCOMPARE(respuesta["code"].toInt(), Middleware::RESP_EXITO);
    socket.disconnectFromServer();
}

void TestMiddlewareCrudRoutes::timeout()
{
    m_server->setTimeoutMs(100);

    m_server->registerRoute("slow_op", [](const QJsonObject &, QLocalSocket *s) {
        QTimer::singleShot(5000, s, [s]() {
            QJsonObject r;
            r["status"] = "ok";
            r["code"] = Middleware::RESP_EXITO;
            r["data"] = "slow response";
            QJsonDocument d(r);
            s->write(d.toJson(QJsonDocument::Compact));
            s->flush();
        });
    });

    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);
    QVERIFY(spy.isValid());

    client.enviarSolicitud("slow_op");
    QVERIFY(spy.wait(3000));

    QJsonObject respuesta = spy.at(0).at(0).toJsonObject();

    m_server->setTimeoutMs(5000);

    QCOMPARE(respuesta["status"].toString(), QString("error"));
    QCOMPARE(respuesta["code"].toInt(), Middleware::RESP_TIEMPO_AGOTADO);
}

QTEST_MAIN(TestMiddlewareCrudRoutes)
#include "test_crud_routes.moc"
