#include <middleware/internalserver.h>
#include <middleware/messages.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QDebug>

/**
 * @brief Constructor — crea el servidor local.
 */
InternalServer::InternalServer(QObject *parent)
    : QObject(parent)
    , m_server(new QLocalServer(this))
{
}

/**
 * @brief Inicia el servidor IPC.
 *
 * 1. Elimina cualquier instancia previa del servidor (removeServer).
 * 2. Escucha en el nombre configurado (SERVER_NAME).
 * 3. Conecta la señal newConnection al slot onNewConnection.
 *
 * @return true si el servidor inició correctamente.
 */
bool InternalServer::start()
{
    QLocalServer::removeServer(Middleware::SERVER_NAME);

    if (!m_server->listen(Middleware::SERVER_NAME)) {
        qCritical() << "No se pudo iniciar el servidor IPC:" << m_server->errorString();
        return false;
    }
    qDebug() << "Servidor Middleware IPC escuchando en:" << Middleware::SERVER_NAME;
    connect(m_server, &QLocalServer::newConnection, this, &InternalServer::onNewConnection);
    return true;
}

/**
 * @brief Slot llamado cuando un nuevo cliente se conecta.
 *
 * Obtiene el socket del cliente y conecta las señales readyRead
 * y disconnected para manejar la comunicación.
 */
void InternalServer::onNewConnection()
{
    QLocalSocket *clienteSocket = m_server->nextPendingConnection();
    if (!clienteSocket) return;

    connect(clienteSocket, &QLocalSocket::readyRead,
            this, &InternalServer::onReadyRead);
    connect(clienteSocket, &QLocalSocket::disconnected,
            this, &InternalServer::onClientDisconnected);
}

/**
 * @brief Procesa los mensajes entrantes de los clientes IPC.
 *
 * Parsea el JSON recibido, identifica la operación por el campo "op",
 * y construye la respuesta apropiada.
 *
 * ## Operaciones soportadas
 * - health_check → confirma que el backend está vivo
 * - ready → confirmación de disponibilidad
 * - shutdown → prepara el apagado graceful
 * - teacher_* → operaciones CRUD (placeholder)
 * - cualquier otra → RESP_INVALIDO
 */
void InternalServer::onReadyRead()
{
    QLocalSocket *clienteSocket = qobject_cast<QLocalSocket*>(sender());
    if (!clienteSocket) return;

    QByteArray data = clienteSocket->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isNull() && doc.isObject()) {
        QJsonObject obj = doc.object();
        QString op = obj["op"].toString();

        registrarConexion("Frontend -> Middleware", op);

        QJsonObject respuesta;

        if (op == Middleware::OP_HEALTH_CHECK) {
            respuesta["status"] = "ok";
            respuesta["code"] = Middleware::RESP_EXITO;
            registrarConexion("Middleware -> Frontend", "health-check [ok]");
        } else if (op == Middleware::OP_LISTO) {
            respuesta["status"] = "ok";
            respuesta["code"] = Middleware::RESP_EXITO;
            registrarConexion("Middleware -> Frontend", "ready [ok]");
        } else if (op == Middleware::OP_APAGAR) {
            respuesta["status"] = "ok";
            respuesta["code"] = Middleware::RESP_EXITO;
            registrarConexion("Middleware -> Frontend", "shutdown [ok]");
        } else if (op == Middleware::OP_LISTA_PROFESORES
                || op == Middleware::OP_OBTENER_PROFESOR
                || op == Middleware::OP_CREAR_PROFESOR
                || op == Middleware::OP_ACTUALIZAR_PROFESOR
                || op == Middleware::OP_ELIMINAR_PROFESOR) {
            respuesta["status"] = "ok";
            respuesta["code"] = Middleware::RESP_EXITO;
            respuesta["op"] = op;
            registrarConexion("Middleware -> Frontend", "CRUD [ok] — " + op);
        } else {
            respuesta["status"] = "error";
            respuesta["code"] = Middleware::RESP_INVALIDO;
            registrarConexion("Middleware -> Frontend",
                              "operación desconocida: " + op);
        }

        QJsonDocument docRespuesta(respuesta);
        clienteSocket->write(docRespuesta.toJson(QJsonDocument::Compact));
        clienteSocket->flush();
    }
}

/**
 * @brief Slot llamado cuando un cliente se desconecta.
 *
 * Elimina el socket del cliente para liberar recursos.
 */
void InternalServer::onClientDisconnected()
{
    QLocalSocket *clienteSocket = qobject_cast<QLocalSocket*>(sender());
    if (clienteSocket) {
        clienteSocket->deleteLater();
    }
}

/**
 * @brief Registra una operación en el log de depuración.
 *
 * Formato: [timestamp] [direccion] Operación: nombre
 *
 * @param direccion "Frontend -> Middleware" o "Middleware -> Frontend".
 * @param operacion Nombre o descripción de la operación.
 */
void InternalServer::registrarConexion(const QString &direccion,
                                        const QString &operacion)
{
    QString timestamp = QDateTime::currentDateTime()
                            .toString("yyyy-MM-dd hh:mm:ss.zzz");
    qDebug() << QStringLiteral("[%1] [%2] Operación: %3")
                    .arg(timestamp, direccion, operacion);
}
