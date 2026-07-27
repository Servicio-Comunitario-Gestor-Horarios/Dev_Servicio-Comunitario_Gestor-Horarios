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
    enviarSolicitud(Middleware::OP_HEALTH_CHECK);
}

void InternalClient::enviarSolicitud(const QString &op, const QJsonObject &payload)
{
    m_operacionPendiente = op;
    m_payloadPendiente = payload;
    qDebug() << "Cliente: Conectando al servidor IPC para operación:" << op;
    m_socket->connectToServer(Middleware::SERVER_NAME);
}

void InternalClient::onConnected()
{
    qDebug() << "Cliente: Conectado al servidor. Enviando solicitud...";

    QJsonObject solicitud;
    solicitud["op"] = m_operacionPendiente;
    if (!m_payloadPendiente.isEmpty())
        solicitud["payload"] = m_payloadPendiente;

    QJsonDocument doc(solicitud);
    m_socket->write(doc.toJson(QJsonDocument::Compact));
    m_socket->flush();
}

void InternalClient::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        bool exito = (obj["status"].toString() == "ok");
        qDebug() << "Cliente: Respuesta recibida —" << (exito ? "ÉXITO" : "FALLO");
        emit healthCheckResponseReceived(exito);
        emit respuestaRecibida(obj);
    }
}

void InternalClient::onErrorOccurred(QLocalSocket::LocalSocketError error)
{
    Q_UNUSED(error)
    qCritical() << "Cliente: Error de conexión:" << m_socket->errorString();

    // Emitir respuesta de error con la operación pendiente
    QJsonObject respuesta;
    respuesta["status"] = "error";
    respuesta["code"] = -1;
    respuesta["op"] = m_operacionPendiente;
    emit respuestaRecibida(respuesta);
    emit healthCheckResponseReceived(false);
}
