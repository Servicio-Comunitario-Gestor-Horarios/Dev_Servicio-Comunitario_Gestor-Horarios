#include <middleware/internalserver.h>
#include <middleware/messages.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
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
        } else if (op == Middleware::OP_LISTA_PROFESORES) {
            handleTeacherList(clienteSocket);
            return;
        } else if (op == Middleware::OP_OBTENER_PROFESOR) {
            handleTeacherGet(obj["data"].toObject(), clienteSocket);
            return;
        } else if (op == Middleware::OP_CREAR_PROFESOR) {
            handleTeacherCreate(obj["data"].toObject(), clienteSocket);
            return;
        } else if (op == Middleware::OP_ACTUALIZAR_PROFESOR) {
            handleTeacherUpdate(obj["data"].toObject(), clienteSocket);
            return;
        } else if (op == Middleware::OP_ELIMINAR_PROFESOR) {
            handleTeacherDelete(obj["data"].toObject(), clienteSocket);
            return;
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

// ─── Helpers ────────────────────────────────────────────────────────

void InternalServer::sendResponse(int status, const QJsonValue &data,
                                  QLocalSocket *clienteSocket)
{
    QJsonObject respuesta;
    respuesta["status"] = status;
    respuesta["data"] = data;
    QJsonDocument doc(respuesta);
    clienteSocket->write(doc.toJson(QJsonDocument::Compact));
    clienteSocket->flush();
}

// ─── CRUD Profesores (stubs) ───────────────────────────────────────
// TODO: Conectar con backend/data/profesor.hpp cuando los servicios existan.

void InternalServer::handleTeacherList(QLocalSocket *clienteSocket)
{
    // TODO: Consultar servicio de profesores del backend
    sendResponse(Middleware::RESP_EXITO, QJsonArray(), clienteSocket);
    registrarConexion("Middleware -> Frontend", "teacher_list [stub]");
}

void InternalServer::handleTeacherGet(const QJsonObject &data,
                                      QLocalSocket *clienteSocket)
{
    QString id = data["id"].toString();
    if (id.isEmpty()) {
        sendResponse(Middleware::RESP_INVALIDO, "Falta campo id", clienteSocket);
        return;
    }
    // TODO: Buscar profesor por ID en el backend
    sendResponse(Middleware::RESP_NO_ENCONTRADO, QJsonObject(), clienteSocket);
    registrarConexion("Middleware -> Frontend", "teacher_get [stub]");
}

void InternalServer::handleTeacherCreate(const QJsonObject &data,
                                         QLocalSocket *clienteSocket)
{
    if (!data.contains("nombre") || !data.contains("correo")) {
        sendResponse(Middleware::RESP_INVALIDO, "Faltan campos requeridos", clienteSocket);
        return;
    }
    // TODO: Validar duplicados y crear en el backend
    sendResponse(Middleware::RESP_EXITO, data, clienteSocket);
    registrarConexion("Middleware -> Frontend", "teacher_create [stub]");
}

void InternalServer::handleTeacherUpdate(const QJsonObject &data,
                                         QLocalSocket *clienteSocket)
{
    if (!data.contains("id")) {
        sendResponse(Middleware::RESP_INVALIDO, "Falta campo id", clienteSocket);
        return;
    }
    // TODO: Actualizar profesor en el backend
    sendResponse(Middleware::RESP_EXITO, data, clienteSocket);
    registrarConexion("Middleware -> Frontend", "teacher_update [stub]");
}

void InternalServer::handleTeacherDelete(const QJsonObject &data,
                                         QLocalSocket *clienteSocket)
{
    QString id = data["id"].toString();
    if (id.isEmpty()) {
        sendResponse(Middleware::RESP_INVALIDO, "Falta campo id", clienteSocket);
        return;
    }
    // TODO: Eliminar profesor en el backend
    sendResponse(Middleware::RESP_EXITO, "Eliminado", clienteSocket);
    registrarConexion("Middleware -> Frontend", "teacher_delete [stub]");
}
