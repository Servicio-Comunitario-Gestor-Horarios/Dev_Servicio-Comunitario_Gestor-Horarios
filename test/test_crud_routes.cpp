#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>

#include <middleware/internalserver.h>
#include <middleware/internalclient.h>
#include <middleware/messages.h>

#include <QtTest>

class TestCrudRoutes : public QObject {
    Q_OBJECT
private slots:
    void healthCheck();
    void teacherList();
    void subjectList();
    void classroomList();
    void classroomGet();
    void invalidOp();
    void teacherCreateValid();
    void teacherCreateMissingFields();
};

void TestCrudRoutes::healthCheck()
{
    InternalServer server;
    QVERIFY(server.start());

    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);
    QVERIFY(spy.isValid());

    client.enviarSolicitud(Middleware::OP_HEALTH_CHECK);
    QVERIFY(spy.wait(3000));

    QJsonObject respuesta = spy.at(0).at(0).toJsonObject();
    QCOMPARE(respuesta["code"].toInt(), Middleware::RESP_EXITO);
    QCOMPARE(respuesta["status"].toString(), QString("ok"));
}

void TestCrudRoutes::teacherList()
{
    InternalServer server;
    QVERIFY(server.start());

    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);
    QVERIFY(spy.isValid());

    client.enviarSolicitud(Middleware::OP_LISTA_PROFESORES);
    QVERIFY(spy.wait(3000));

    QJsonObject respuesta = spy.at(0).at(0).toJsonObject();
    QCOMPARE(respuesta["status"].toInt(), Middleware::RESP_EXITO);
    QVERIFY(respuesta["data"].isArray());
}

void TestCrudRoutes::subjectList()
{
    InternalServer server;
    QVERIFY(server.start());

    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);
    QVERIFY(spy.isValid());

    client.enviarSolicitud(Middleware::OP_LISTA_MATERIAS);
    QVERIFY(spy.wait(3000));

    QJsonObject respuesta = spy.at(0).at(0).toJsonObject();
    QCOMPARE(respuesta["status"].toInt(), Middleware::RESP_EXITO);
    QVERIFY(respuesta["data"].isArray());
}

void TestCrudRoutes::classroomList()
{
    InternalServer server;
    QVERIFY(server.start());

    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);
    QVERIFY(spy.isValid());

    client.enviarSolicitud(Middleware::OP_LISTA_AULAS);
    QVERIFY(spy.wait(3000));

    QJsonObject respuesta = spy.at(0).at(0).toJsonObject();
    QCOMPARE(respuesta["status"].toInt(), Middleware::RESP_EXITO);
    QVERIFY(respuesta["data"].isArray());
}

void TestCrudRoutes::classroomGet()
{
    InternalServer server;
    QVERIFY(server.start());

    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);
    QVERIFY(spy.isValid());

    QJsonObject payload;
    payload["id"] = "aula-1";
    client.enviarSolicitud(Middleware::OP_OBTENER_AULA, payload);
    QVERIFY(spy.wait(3000));

    QJsonObject respuesta = spy.at(0).at(0).toJsonObject();
    QCOMPARE(respuesta["status"].toInt(), Middleware::RESP_NO_ENCONTRADO);
}

void TestCrudRoutes::invalidOp()
{
    InternalServer server;
    QVERIFY(server.start());

    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);
    QVERIFY(spy.isValid());

    client.enviarSolicitud("operacion_inexistente");
    QVERIFY(spy.wait(3000));

    QJsonObject respuesta = spy.at(0).at(0).toJsonObject();
    QCOMPARE(respuesta["code"].toInt(), Middleware::RESP_INVALIDO);
}

void TestCrudRoutes::teacherCreateValid()
{
    InternalServer server;
    QVERIFY(server.start());

    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);
    QVERIFY(spy.isValid());

    QJsonObject payload;
    payload["nombre"] = "Prof. Ejemplo";
    payload["correo"] = "ejemplo@universidad.edu";
    client.enviarSolicitud(Middleware::OP_CREAR_PROFESOR, payload);
    QVERIFY(spy.wait(3000));

    QJsonObject respuesta = spy.at(0).at(0).toJsonObject();
    QCOMPARE(respuesta["status"].toInt(), Middleware::RESP_EXITO);
}

void TestCrudRoutes::teacherCreateMissingFields()
{
    InternalServer server;
    QVERIFY(server.start());

    InternalClient client;
    QSignalSpy spy(&client, &InternalClient::respuestaRecibida);
    QVERIFY(spy.isValid());

    QJsonObject payload;
    payload["nombre"] = "Prof. SinCorreo";
    client.enviarSolicitud(Middleware::OP_CREAR_PROFESOR, payload);
    QVERIFY(spy.wait(3000));

    QJsonObject respuesta = spy.at(0).at(0).toJsonObject();
    QCOMPARE(respuesta["status"].toInt(), Middleware::RESP_INVALIDO);
}

QTEST_MAIN(TestCrudRoutes)
#include "test_crud_routes.moc"
