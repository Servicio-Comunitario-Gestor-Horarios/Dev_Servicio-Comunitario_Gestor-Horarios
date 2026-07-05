#include <middleware/internalclient.h>
#include <middleware/messages.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

InternalClient::InternalClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QLocalSocket(this))
{
    connect(m_socket, &QLocalSocket::connected, this, &InternalClient::onConnected);
    connect(m_socket, &QLocalSocket::readyRead, this, &InternalClient::onReadyRead);
    connect(m_socket, &QLocalSocket::errorOccurred, this, &InternalClient::onErrorOccurred);
}

void InternalClient::sendHealthCheck()
{
    sendRequest(Middleware::OP_HEALTH_CHECK);
}

void InternalClient::sendRequest(const QString &op, const QJsonObject &payload)
{
    m_pendingOp = op;
    m_pendingPayload = payload;
    qDebug() << "Cliente: Conectando al servidor IPC para operación:" << op;
    m_socket->connectToServer(Middleware::SERVER_NAME);
}

void InternalClient::onConnected()
{
    qDebug() << "Cliente: Conectado al servidor. Enviando solicitud...";

    QJsonObject request;
    request["op"] = m_pendingOp;
    if (!m_pendingPayload.isEmpty())
        request["payload"] = m_pendingPayload;

    QJsonDocument doc(request);
    m_socket->write(doc.toJson(QJsonDocument::Compact));
    m_socket->flush();
}

void InternalClient::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        bool success = (obj["status"].toString() == "ok");
        qDebug() << "Cliente: Respuesta recibida —" << (success ? "ÉXITO" : "FALLO");
        emit healthCheckResponseReceived(success);
        emit responseReceived(obj);
    }
}

void InternalClient::onErrorOccurred(QLocalSocket::LocalSocketError error)
{
    Q_UNUSED(error)
    qCritical() << "Cliente: Error de conexión:" << m_socket->errorString();
    emit healthCheckResponseReceived(false);
}
