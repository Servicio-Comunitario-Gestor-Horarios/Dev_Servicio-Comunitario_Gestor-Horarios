#include <middleware/internalclient.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

#include <middleware/messages.h>

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
    qDebug() << "Cliente: Conectando al servidor IPC...";
    m_socket->connectToServer(Middleware::SERVER_NAME);
}

void InternalClient::onConnected()
{
    qDebug() << "Cliente: Conectado al servidor. Enviando health-check...";

    QJsonObject request;
    request["op"] = Middleware::OP_HEALTH_CHECK;

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
    }
}

void InternalClient::onErrorOccurred(QLocalSocket::LocalSocketError error)
{
    qCritical() << "Cliente: Error de conexión:" << error << m_socket->errorString();
    emit healthCheckResponseReceived(false);
}
