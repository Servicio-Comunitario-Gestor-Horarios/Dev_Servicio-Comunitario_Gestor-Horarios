#include <middleware/internalserver.h>
#include <middleware/messages.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QDebug>

InternalServer::InternalServer(QObject *parent) : QObject(parent) {
    m_server = new QLocalServer(this);
}

bool InternalServer::start() {
    QLocalServer::removeServer(Middleware::SERVER_NAME);

    if (!m_server->listen(Middleware::SERVER_NAME)) {
        qCritical() << "No se pudo iniciar el servidor IPC:" << m_server->errorString();
        return false;
    }
    qDebug() << "Servidor Middleware IPC escuchando en:" << Middleware::SERVER_NAME;
    connect(m_server, &QLocalServer::newConnection, this, &InternalServer::onNewConnection);
    return true;
}

void InternalServer::onNewConnection() {
    QLocalSocket *clientSocket = m_server->nextPendingConnection();
    if (!clientSocket) return;

    connect(clientSocket, &QLocalSocket::readyRead, this, &InternalServer::onReadyRead);
    connect(clientSocket, &QLocalSocket::disconnected, this, &InternalServer::onClientDisconnected);
}

void InternalServer::onReadyRead() {
    QLocalSocket *clientSocket = qobject_cast<QLocalSocket*>(sender());
    if (!clientSocket) return;

    QByteArray data = clientSocket->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isNull() && doc.isObject()) {
        QJsonObject obj = doc.object();
        QString op = obj["op"].toString();

        logConnection("Frontend -> Middleware", op);

        QJsonObject response;

        if (op == Middleware::OP_HEALTH_CHECK) {
            response["status"] = "ok";
            response["code"] = Middleware::RESP_OK;
            logConnection("Middleware -> Frontend", "Respuesta health-check [ok]");
        } else if (op == Middleware::OP_READY) {
            response["status"] = "ok";
            response["code"] = Middleware::RESP_OK;
            logConnection("Middleware -> Frontend", "Respuesta ready [ok]");
        } else if (op == Middleware::OP_SHUTDOWN) {
            response["status"] = "ok";
            response["code"] = Middleware::RESP_OK;
            logConnection("Middleware -> Frontend", "Respuesta shutdown [ok]");
        } else if (op == Middleware::OP_TEACHER_LIST
                || op == Middleware::OP_TEACHER_GET
                || op == Middleware::OP_TEACHER_CREATE
                || op == Middleware::OP_TEACHER_UPDATE
                || op == Middleware::OP_TEACHER_DELETE) {
            response["status"] = "ok";
            response["code"] = Middleware::RESP_OK;
            response["op"] = op;
            logConnection("Middleware -> Frontend", "Respuesta CRUD [ok] — " + op);
        } else {
            response["status"] = "error";
            response["code"] = Middleware::RESP_INVALID;
            logConnection("Middleware -> Frontend", "Respuesta [error] — operación desconocida: " + op);
        }

        QJsonDocument responseDoc(response);
        clientSocket->write(responseDoc.toJson(QJsonDocument::Compact));
        clientSocket->flush();
    }
}

void InternalServer::onClientDisconnected() {
    QLocalSocket *clientSocket = qobject_cast<QLocalSocket*>(sender());
    if (clientSocket) {
        clientSocket->deleteLater();
    }
}

void InternalServer::logConnection(const QString &direction, const QString &operation) {
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    qDebug() << QString("[%1] [%2] Operación: %3").arg(timestamp, direction, operation);
}
