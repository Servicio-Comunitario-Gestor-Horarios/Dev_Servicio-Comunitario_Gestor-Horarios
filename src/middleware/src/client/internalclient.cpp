#include <middleware/internalclient.h>
#include <middleware/messages.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

/**
 * @brief Constructor — prepara el socket y conecta señales.
 */
InternalClient::InternalClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QLocalSocket(this))
{
    connect(m_socket, &QLocalSocket::connected, this, &InternalClient::onConnected);
    connect(m_socket, &QLocalSocket::readyRead, this, &InternalClient::onReadyRead);
    connect(m_socket, &QLocalSocket::errorOccurred, this, &InternalClient::onErrorOccurred);
}

/**
 * @brief Envía una solicitud de verificación de salud.
 *
 * Delega en enviarSolicitud() con la operación OP_HEALTH_CHECK.
 */
void InternalClient::sendHealthCheck()
{
    enviarSolicitud(Middleware::OP_HEALTH_CHECK);
}

/**
 * @brief Envía una solicitud IPC genérica al backend.
 *
 * 1. Almacena la operación y payload como pendientes.
 * 2. Conecta al servidor IPC.
 * 3. Al establecerse la conexión (onConnected), construye el JSON
 *    y lo envía.
 *
 * @param op Código de operación.
 * @param payload Datos adicionales (objeto JSON vacío por defecto).
 */
void InternalClient::enviarSolicitud(const QString &op, const QJsonObject &payload)
{
    m_operacionPendiente = op;
    m_payloadPendiente = payload;
    qDebug() << "Cliente: Conectando al servidor IPC para operación:" << op;
    m_socket->connectToServer(Middleware::SERVER_NAME);
}

/**
 * @brief Slot llamado cuando se establece la conexión con el servidor.
 *
 * Construye el JSON de solicitud:
 * ~~~{.json}
 * {"op": "teacher_list", "payload": {...}}
 * ~~~
 * y lo envía por el socket.
 */
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

/**
 * @brief Slot llamado cuando hay datos disponibles en el socket.
 *
 * Parsea el JSON de respuesta y emite las señales correspondientes:
 * - healthCheckResponseReceived() para operaciones de verificación.
 * - respuestaRecibida() con el objeto completo.
 */
void InternalClient::onReadyRead()
{
    QByteArray data = m_socket->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        bool exito = (obj["status"].toInt(-1) == 0);
        qDebug() << "Cliente: Respuesta recibida —" << (exito ? "ÉXITO" : "FALLO");
        emit healthCheckResponseReceived(exito);
        emit respuestaRecibida(obj);
    }
}

/**
 * @brief Slot llamado cuando ocurre un error de conexión.
 *
 * Emite healthCheckResponseReceived(false) para notificar el fallo.
 */
void InternalClient::onErrorOccurred(QLocalSocket::LocalSocketError error)
{
    Q_UNUSED(error)
    qCritical() << "Cliente: Error de conexión:" << m_socket->errorString();
    emit healthCheckResponseReceived(false);
}
