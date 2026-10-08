#include "../../include/middleware/internalclient.h"
#include "../../include/middleware/messages.h"
#include <middleware/internalclient.h>
#include <middleware/messages.h>
#include <middleware/ipc_framing.hpp>

#include <QLocalSocket>
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
    connect(m_socket, &QLocalSocket::disconnected, this, &InternalClient::onDisconnected);
}

void InternalClient::sendHealthCheck()
{
    enviarSolicitud(Middleware::OP_HEALTH_CHECK);
}

void InternalClient::enviarSolicitud(const QString &op, const QJsonObject &payload)
{
    m_cola.enqueue(qMakePair(op, payload));

    if (m_socket->state() == QLocalSocket::ConnectedState) {
        enviarSiguiente();
        return;
    }
    if (m_socket->state() == QLocalSocket::ConnectingState)
        return;

    qDebug() << "Cliente: Conectando al servidor IPC para operación:" << op;
    m_socket->connectToServer(Middleware::SERVER_NAME);
}

void InternalClient::enviarSiguiente()
{
    if (!m_operacionEnVuelo.isEmpty()) return;
    if (m_cola.isEmpty()) return;
    if (m_socket->state() != QLocalSocket::ConnectedState) return;

    const QPair<QString, QJsonObject> solicitud = m_cola.dequeue();
    m_operacionEnVuelo = solicitud.first;

    QJsonObject mensaje;
    mensaje["v"] = Middleware::PROTOCOL_VERSION;
    mensaje["op"] = solicitud.first;
    if (!solicitud.second.isEmpty())
        mensaje["payload"] = solicitud.second;

    m_socket->write(Middleware::encodeFrame(mensaje));
    m_socket->flush();
}

void InternalClient::onConnected()
{
    qDebug() << "Cliente: Conectado al servidor. Enviando solicitudes...";
    enviarSiguiente();
}

void InternalClient::onDisconnected()
{
    qWarning() << "Cliente: Desconectado del servidor IPC";
    m_buffer.clear();
}

void InternalClient::onReadyRead()
{
    m_buffer.append(m_socket->readAll());

    bool overflow = false;
    const QVector<QByteArray> frames = Middleware::takeCompleteFrames(m_buffer, &overflow);
    Q_UNUSED(overflow)

    for (const QByteArray &frame : frames) {
        const QJsonDocument doc = QJsonDocument::fromJson(frame);
        if (!doc.isObject()) continue;

        QJsonObject respuesta = doc.object();
        if (respuesta.value("v").toInt(0) != Middleware::PROTOCOL_VERSION) {
            respuesta["status"] = "error";
            respuesta["code"]   = Middleware::RESP_VERSION_INCOMPATIBLE;
            respuesta["data"]   = "Versión de protocolo no soportada";
        }

        const bool exito = (respuesta["status"].toString() == "ok");
        if (respuesta["op"].toString() == Middleware::OP_HEALTH_CHECK)
            emit healthCheckResponseReceived(exito);

        emit respuestaRecibida(respuesta);

        m_operacionEnVuelo.clear();
        enviarSiguiente();
    }
}

void InternalClient::onErrorOccurred(QLocalSocket::LocalSocketError error)
{
    Q_UNUSED(error)
    qCritical() << "Cliente: Error de conexión:" << m_socket->errorString();

    QString opEnVuelo = m_operacionEnVuelo;
    if (opEnVuelo.isEmpty() && !m_cola.isEmpty())
        opEnVuelo = m_cola.head().first;   // falló al conectar, aún sin enviar
    m_operacionEnVuelo.clear();

    QJsonObject respuesta;
    respuesta["v"]      = Middleware::PROTOCOL_VERSION;
    respuesta["status"] = "error";
    respuesta["code"]   = Middleware::RESP_ERROR;
    respuesta["op"]     = opEnVuelo;

    if (opEnVuelo == Middleware::OP_HEALTH_CHECK)
        emit healthCheckResponseReceived(false);

    emit respuestaRecibida(respuesta);
}
