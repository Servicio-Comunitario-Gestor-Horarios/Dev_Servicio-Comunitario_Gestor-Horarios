#include <middleware/internalserver.h>
#include <middleware/messages.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>

/**
 * @brief Constructor — crea el servidor local y configura el timeout.
 */
InternalServer::InternalServer(QObject *parent)
    : QObject(parent)
    , m_server(new QLocalServer(this))
    , m_timeoutTimer(new QTimer(this))
{
    m_timeoutTimer->setSingleShot(true);
    m_timeoutTimer->setInterval(5000);
    connect(m_timeoutTimer, &QTimer::timeout, this, []() {
        qWarning() << "Timeout: operación tardó más de 5 segundos";
    });
}

/**
 * @brief Inicia el servidor IPC y registra las rutas CRUD.
 */
bool InternalServer::start()
{
    QLocalServer::removeServer(Middleware::SERVER_NAME);

    if (!m_server->listen(Middleware::SERVER_NAME)) {
        qCritical() << "No se pudo iniciar el servidor IPC:" << m_server->errorString();
        return false;
    }

    inicializarRutas();

    qDebug() << "Servidor Middleware IPC escuchando en:" << Middleware::SERVER_NAME;
    connect(m_server, &QLocalServer::newConnection, this, &InternalServer::onNewConnection);
    return true;
}

/**
 * @brief Registra todas las rutas CRUD en el QHash de dispatch.
 */
void InternalServer::inicializarRutas()
{
    // ─── Sistema ───
    m_rutas["health_check"] = [this](const QJsonObject &, QLocalSocket *s) {
        QJsonObject r; r["status"] = "ok"; r["code"] = Middleware::RESP_EXITO;
        registrarConexion("Middleware -> Frontend", "health-check [ok]");
        QJsonDocument d(r); s->write(d.toJson(QJsonDocument::Compact)); s->flush();
    };
    m_rutas["ready"] = [this](const QJsonObject &, QLocalSocket *s) {
        QJsonObject r; r["status"] = "ok"; r["code"] = Middleware::RESP_EXITO;
        registrarConexion("Middleware -> Frontend", "ready [ok]");
        QJsonDocument d(r); s->write(d.toJson(QJsonDocument::Compact)); s->flush();
    };
    m_rutas["shutdown"] = [this](const QJsonObject &, QLocalSocket *s) {
        QJsonObject r; r["status"] = "ok"; r["code"] = Middleware::RESP_EXITO;
        registrarConexion("Middleware -> Frontend", "shutdown [ok]");
        QJsonDocument d(r); s->write(d.toJson(QJsonDocument::Compact)); s->flush();
    };

    // ─── Profesores ───
    m_rutas[Middleware::OP_LISTA_PROFESORES]    = [this](const QJsonObject &, QLocalSocket *s) { handleTeacherList(s); };
    m_rutas[Middleware::OP_OBTENER_PROFESOR]    = [this](const QJsonObject &d, QLocalSocket *s) { handleTeacherGet(d, s); };
    m_rutas[Middleware::OP_CREAR_PROFESOR]      = [this](const QJsonObject &d, QLocalSocket *s) { handleTeacherCreate(d, s); };
    m_rutas[Middleware::OP_ACTUALIZAR_PROFESOR] = [this](const QJsonObject &d, QLocalSocket *s) { handleTeacherUpdate(d, s); };
    m_rutas[Middleware::OP_ELIMINAR_PROFESOR]   = [this](const QJsonObject &d, QLocalSocket *s) { handleTeacherDelete(d, s); };

    // ─── Aulas ───
    m_rutas[Middleware::OP_LISTA_AULAS]      = [this](const QJsonObject &, QLocalSocket *s) { handleClassroomList(s); };
    m_rutas[Middleware::OP_OBTENER_AULA]     = [this](const QJsonObject &d, QLocalSocket *s) { handleClassroomGet(d, s); };
    m_rutas[Middleware::OP_CREAR_AULA]       = [this](const QJsonObject &d, QLocalSocket *s) { handleClassroomCreate(d, s); };
    m_rutas[Middleware::OP_ACTUALIZAR_AULA]  = [this](const QJsonObject &d, QLocalSocket *s) { handleClassroomUpdate(d, s); };
    m_rutas[Middleware::OP_ELIMINAR_AULA]    = [this](const QJsonObject &d, QLocalSocket *s) { handleClassroomDelete(d, s); };

    // ─── Materias ───
    m_rutas[Middleware::OP_LISTA_MATERIAS]       = [this](const QJsonObject &, QLocalSocket *s) { handleSubjectList(s); };
    m_rutas[Middleware::OP_OBTENER_MATERIA]      = [this](const QJsonObject &d, QLocalSocket *s) { handleSubjectGet(d, s); };
    m_rutas[Middleware::OP_CREAR_MATERIA]        = [this](const QJsonObject &d, QLocalSocket *s) { handleSubjectCreate(d, s); };
    m_rutas[Middleware::OP_ACTUALIZAR_MATERIA]   = [this](const QJsonObject &d, QLocalSocket *s) { handleSubjectUpdate(d, s); };
    m_rutas[Middleware::OP_ELIMINAR_MATERIA]     = [this](const QJsonObject &d, QLocalSocket *s) { handleSubjectDelete(d, s); };
}

/**
 * @brief Slot llamado cuando un nuevo cliente se conecta.
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
 * Parsea el JSON, busca la operación en el route map QHash,
 * y ejecuta el handler correspondiente con timeout de 5s.
 */
void InternalServer::onReadyRead()
{
    QLocalSocket *clienteSocket = qobject_cast<QLocalSocket*>(sender());
    if (!clienteSocket) return;

    QByteArray data = clienteSocket->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (doc.isNull() || !doc.isObject()) {
        QJsonObject err;
        err["status"] = "error";
        err["code"] = Middleware::RESP_INVALIDO;
        QJsonDocument d(err);
        clienteSocket->write(d.toJson(QJsonDocument::Compact));
        clienteSocket->flush();
        return;
    }

    QJsonObject obj = doc.object();
    QString op = obj["op"].toString();
    QJsonObject payload = obj["payload"].toObject();

    registrarConexion("Frontend -> Middleware", op);

    if (m_rutas.contains(op)) {
        m_timeoutTimer->start();
        m_rutas[op](payload, clienteSocket);
        m_timeoutTimer->stop();
    } else {
        QJsonObject respuesta;
        respuesta["status"] = "error";
        respuesta["code"] = Middleware::RESP_INVALIDO;
        registrarConexion("Middleware -> Frontend", "operación desconocida: " + op);
        QJsonDocument d(respuesta);
        clienteSocket->write(d.toJson(QJsonDocument::Compact));
        clienteSocket->flush();
    }
}

/**
 * @brief Slot llamado cuando un cliente se desconecta.
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

void InternalServer::handleTeacherList(QLocalSocket *clienteSocket)
{
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
    sendResponse(Middleware::RESP_EXITO, "Eliminado", clienteSocket);
    registrarConexion("Middleware -> Frontend", "teacher_delete [stub]");
}

// ─── CRUD Aulas (stubs) ────────────────────────────────────────────

void InternalServer::handleClassroomList(QLocalSocket *clienteSocket)
{
    sendResponse(Middleware::RESP_EXITO, QJsonArray(), clienteSocket);
    registrarConexion("Middleware -> Frontend", "classroom_list [stub]");
}

void InternalServer::handleClassroomGet(const QJsonObject &data,
                                        QLocalSocket *clienteSocket)
{
    QString id = data["id"].toString();
    if (id.isEmpty()) {
        sendResponse(Middleware::RESP_INVALIDO, "Falta campo id", clienteSocket);
        return;
    }
    sendResponse(Middleware::RESP_NO_ENCONTRADO, QJsonObject(), clienteSocket);
    registrarConexion("Middleware -> Frontend", "classroom_get [stub]");
}

void InternalServer::handleClassroomCreate(const QJsonObject &data,
                                           QLocalSocket *clienteSocket)
{
    if (!data.contains("nombre") || !data.contains("capacidad")) {
        sendResponse(Middleware::RESP_INVALIDO, "Faltan campos requeridos", clienteSocket);
        return;
    }
    sendResponse(Middleware::RESP_EXITO, data, clienteSocket);
    registrarConexion("Middleware -> Frontend", "classroom_create [stub]");
}

void InternalServer::handleClassroomUpdate(const QJsonObject &data,
                                           QLocalSocket *clienteSocket)
{
    if (!data.contains("id")) {
        sendResponse(Middleware::RESP_INVALIDO, "Falta campo id", clienteSocket);
        return;
    }
    sendResponse(Middleware::RESP_EXITO, data, clienteSocket);
    registrarConexion("Middleware -> Frontend", "classroom_update [stub]");
}

void InternalServer::handleClassroomDelete(const QJsonObject &data,
                                           QLocalSocket *clienteSocket)
{
    QString id = data["id"].toString();
    if (id.isEmpty()) {
        sendResponse(Middleware::RESP_INVALIDO, "Falta campo id", clienteSocket);
        return;
    }
    sendResponse(Middleware::RESP_EXITO, "Eliminado", clienteSocket);
    registrarConexion("Middleware -> Frontend", "classroom_delete [stub]");
}

// ─── CRUD Materias (stubs) ────────────────────────────────────────

void InternalServer::handleSubjectList(QLocalSocket *clienteSocket)
{
    sendResponse(Middleware::RESP_EXITO, QJsonArray(), clienteSocket);
    registrarConexion("Middleware -> Frontend", "subject_list [stub]");
}

void InternalServer::handleSubjectGet(const QJsonObject &data,
                                      QLocalSocket *clienteSocket)
{
    QString id = data["id"].toString();
    if (id.isEmpty()) {
        sendResponse(Middleware::RESP_INVALIDO, "Falta campo id", clienteSocket);
        return;
    }
    sendResponse(Middleware::RESP_NO_ENCONTRADO, QJsonObject(), clienteSocket);
    registrarConexion("Middleware -> Frontend", "subject_get [stub]");
}

void InternalServer::handleSubjectCreate(const QJsonObject &data,
                                         QLocalSocket *clienteSocket)
{
    if (!data.contains("nombre") || !data.contains("tipoAula")) {
        sendResponse(Middleware::RESP_INVALIDO, "Faltan campos requeridos", clienteSocket);
        return;
    }
    sendResponse(Middleware::RESP_EXITO, data, clienteSocket);
    registrarConexion("Middleware -> Frontend", "subject_create [stub]");
}

void InternalServer::handleSubjectUpdate(const QJsonObject &data,
                                         QLocalSocket *clienteSocket)
{
    if (!data.contains("id")) {
        sendResponse(Middleware::RESP_INVALIDO, "Falta campo id", clienteSocket);
        return;
    }
    sendResponse(Middleware::RESP_EXITO, data, clienteSocket);
    registrarConexion("Middleware -> Frontend", "subject_update [stub]");
}

void InternalServer::handleSubjectDelete(const QJsonObject &data,
                                         QLocalSocket *clienteSocket)
{
    QString id = data["id"].toString();
    if (id.isEmpty()) {
        sendResponse(Middleware::RESP_INVALIDO, "Falta campo id", clienteSocket);
        return;
    }
    sendResponse(Middleware::RESP_EXITO, "Eliminado", clienteSocket);
    registrarConexion("Middleware -> Frontend", "subject_delete [stub]");
}
