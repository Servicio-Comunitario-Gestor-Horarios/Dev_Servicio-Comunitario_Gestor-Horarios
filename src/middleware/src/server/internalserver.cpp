#include <middleware/internalserver.h>
#include <middleware/messages.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>

InternalServer::InternalServer(QObject *parent)
    : QObject(parent)
    , m_server(new QLocalServer(this))
{
}

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

void InternalServer::onNewConnection()
{
    QLocalSocket *clienteSocket = m_server->nextPendingConnection();
    if (!clienteSocket) return;

    connect(clienteSocket, &QLocalSocket::readyRead,
            this, &InternalServer::onReadyRead);
    connect(clienteSocket, &QLocalSocket::disconnected,
            this, &InternalServer::onClientDisconnected);
}

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

            // ─── PROFESORES ───
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

            // ─── AULAS ───
        } else if (op == Middleware::OP_LISTA_AULAS) {
            handleClassroomList(clienteSocket);
            return;
        } else if (op == Middleware::OP_CREAR_AULA) {
            handleClassroomCreate(obj["data"].toObject(), clienteSocket);
            return;
        } else if (op == Middleware::OP_ACTUALIZAR_AULA) {
            handleClassroomUpdate(obj["data"].toObject(), clienteSocket);
            return;
        } else if (op == Middleware::OP_ELIMINAR_AULA) {
            handleClassroomDelete(obj["data"].toObject(), clienteSocket);
            return;

            // ─── MATERIAS ───
        } else if (op == Middleware::OP_LISTA_MATERIAS) {
            handleSubjectList(clienteSocket);
            return;
        } else if (op == Middleware::OP_OBTENER_MATERIA) {
            handleSubjectGet(obj["data"].toObject(), clienteSocket);
            return;
        } else if (op == Middleware::OP_CREAR_MATERIA) {
            handleSubjectCreate(obj["data"].toObject(), clienteSocket);
            return;
        } else if (op == Middleware::OP_ACTUALIZAR_MATERIA) {
            handleSubjectUpdate(obj["data"].toObject(), clienteSocket);
            return;
        } else if (op == Middleware::OP_ELIMINAR_MATERIA) {
            handleSubjectDelete(obj["data"].toObject(), clienteSocket);
            return;

            // ─── SI NO ENCUENTRA NINGUNA OPERACIÓN ───
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

void InternalServer::onClientDisconnected()
{
    QLocalSocket *clienteSocket = qobject_cast<QLocalSocket*>(sender());
    if (clienteSocket) {
        clienteSocket->deleteLater();
    }
}

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
                                  QLocalSocket *clienteSocket, const QString &op)
{
    QJsonObject respuesta;
    respuesta["status"] = (status == Middleware::RESP_EXITO) ? "ok" : "error";
    respuesta["code"] = status;
    respuesta["data"] = data;
    respuesta["op"] = op;
    QJsonDocument doc(respuesta);
    clienteSocket->write(doc.toJson(QJsonDocument::Compact));
    clienteSocket->flush();
}

// ─── CRUD Profesores (stubs) ───────────────────────────────────────

void InternalServer::handleTeacherList(QLocalSocket *clienteSocket)
{
    sendResponse(Middleware::RESP_EXITO, QJsonArray(), clienteSocket,
                 Middleware::OP_LISTA_PROFESORES);
    registrarConexion("Middleware -> Frontend", "teacher_list [stub]");
}

void InternalServer::handleTeacherGet(const QJsonObject &data,
                                      QLocalSocket *clienteSocket)
{
    QString id = data["id"].toString();
    if (id.isEmpty()) {
        sendResponse(Middleware::RESP_INVALIDO, "Falta campo id", clienteSocket,
                     Middleware::OP_OBTENER_PROFESOR);
        return;
    }
    sendResponse(Middleware::RESP_NO_ENCONTRADO, QJsonObject(), clienteSocket,
                 Middleware::OP_OBTENER_PROFESOR);
    registrarConexion("Middleware -> Frontend", "teacher_get [stub]");
}

void InternalServer::handleTeacherCreate(const QJsonObject &data,
                                         QLocalSocket *clienteSocket)
{
    if (!data.contains("cedula") || !data.contains("nombre") || !data.contains("email")) {
        sendResponse(Middleware::RESP_INVALIDO, "Faltan campos requeridos", clienteSocket,
                     Middleware::OP_CREAR_PROFESOR);
        return;
    }
    sendResponse(Middleware::RESP_EXITO, data, clienteSocket,
                 Middleware::OP_CREAR_PROFESOR);
    registrarConexion("Middleware -> Frontend", "teacher_create [stub]");
}

void InternalServer::handleTeacherUpdate(const QJsonObject &data,
                                         QLocalSocket *clienteSocket)
{
    if (!data.contains("id")) {
        sendResponse(Middleware::RESP_INVALIDO, "Falta campo id", clienteSocket,
                     Middleware::OP_ACTUALIZAR_PROFESOR);
        return;
    }
    sendResponse(Middleware::RESP_EXITO, data, clienteSocket,
                 Middleware::OP_ACTUALIZAR_PROFESOR);
    registrarConexion("Middleware -> Frontend", "teacher_update [stub]");
}

void InternalServer::handleTeacherDelete(const QJsonObject &data,
                                         QLocalSocket *clienteSocket)
{
    QString id = data["id"].toString();
    if (id.isEmpty()) {
        sendResponse(Middleware::RESP_INVALIDO, "Falta campo id", clienteSocket,
                     Middleware::OP_ELIMINAR_PROFESOR);
        return;
    }
    sendResponse(Middleware::RESP_EXITO, "Eliminado", clienteSocket,
                 Middleware::OP_ELIMINAR_PROFESOR);
    registrarConexion("Middleware -> Frontend", "teacher_delete [stub]");
}

// ─── CRUD Aulas (stubs) ────────────────────────────────────────────

void InternalServer::handleClassroomList(QLocalSocket *clienteSocket)
{
    sendResponse(Middleware::RESP_EXITO, QJsonArray(), clienteSocket,
                 Middleware::OP_LISTA_AULAS);
    registrarConexion("Middleware -> Frontend", "classroom_list [stub]");
}

void InternalServer::handleClassroomCreate(const QJsonObject &data,
                                           QLocalSocket *clienteSocket)
{
    if (!data.contains("nombre") || !data.contains("capacidad")) {
        sendResponse(Middleware::RESP_INVALIDO, "Faltan campos requeridos", clienteSocket,
                     Middleware::OP_CREAR_AULA);
        return;
    }
    sendResponse(Middleware::RESP_EXITO, data, clienteSocket,
                 Middleware::OP_CREAR_AULA);
    registrarConexion("Middleware -> Frontend", "classroom_create [stub]");
}

void InternalServer::handleClassroomUpdate(const QJsonObject &data,
                                           QLocalSocket *clienteSocket)
{
    if (!data.contains("id")) {
        sendResponse(Middleware::RESP_INVALIDO, "Falta campo id", clienteSocket,
                     Middleware::OP_ACTUALIZAR_AULA);
        return;
    }
    sendResponse(Middleware::RESP_EXITO, data, clienteSocket,
                 Middleware::OP_ACTUALIZAR_AULA);
    registrarConexion("Middleware -> Frontend", "classroom_update [stub]");
}

void InternalServer::handleClassroomDelete(const QJsonObject &data,
                                           QLocalSocket *clienteSocket)
{
    QString id = data["id"].toString();
    if (id.isEmpty()) {
        sendResponse(Middleware::RESP_INVALIDO, "Falta campo id", clienteSocket,
                     Middleware::OP_ELIMINAR_AULA);
        return;
    }
    sendResponse(Middleware::RESP_EXITO, "Eliminado", clienteSocket,
                 Middleware::OP_ELIMINAR_AULA);
    registrarConexion("Middleware -> Frontend", "classroom_delete [stub]");
}

// ─── CRUD Materias (stubs) ────────────────────────────────────────

void InternalServer::handleSubjectList(QLocalSocket *clienteSocket)
{
    sendResponse(Middleware::RESP_EXITO, QJsonArray(), clienteSocket,
                 Middleware::OP_LISTA_MATERIAS);
    registrarConexion("Middleware -> Frontend", "subject_list [stub]");
}

void InternalServer::handleSubjectGet(const QJsonObject &data,
                                      QLocalSocket *clienteSocket)
{
    QString id = data["id"].toString();
    if (id.isEmpty()) {
        sendResponse(Middleware::RESP_INVALIDO, "Falta campo id", clienteSocket,
                     Middleware::OP_OBTENER_MATERIA);
        return;
    }
    sendResponse(Middleware::RESP_NO_ENCONTRADO, QJsonObject(), clienteSocket,
                 Middleware::OP_OBTENER_MATERIA);
    registrarConexion("Middleware -> Frontend", "subject_get [stub]");
}

void InternalServer::handleSubjectCreate(const QJsonObject &data,
                                         QLocalSocket *clienteSocket)
{
    if (!data.contains("nombre") || !data.contains("tipoAula")) {
        sendResponse(Middleware::RESP_INVALIDO, "Faltan campos requeridos", clienteSocket,
                     Middleware::OP_CREAR_MATERIA);
        return;
    }
    sendResponse(Middleware::RESP_EXITO, data, clienteSocket,
                 Middleware::OP_CREAR_MATERIA);
    registrarConexion("Middleware -> Frontend", "subject_create [stub]");
}

void InternalServer::handleSubjectUpdate(const QJsonObject &data,
                                         QLocalSocket *clienteSocket)
{
    if (!data.contains("id")) {
        sendResponse(Middleware::RESP_INVALIDO, "Falta campo id", clienteSocket,
                     Middleware::OP_ACTUALIZAR_MATERIA);
        return;
    }
    sendResponse(Middleware::RESP_EXITO, data, clienteSocket,
                 Middleware::OP_ACTUALIZAR_MATERIA);
    registrarConexion("Middleware -> Frontend", "subject_update [stub]");
}

void InternalServer::handleSubjectDelete(const QJsonObject &data,
                                         QLocalSocket *clienteSocket)
{
    QString id = data["id"].toString();
    if (id.isEmpty()) {
        sendResponse(Middleware::RESP_INVALIDO, "Falta campo id", clienteSocket,
                     Middleware::OP_ELIMINAR_MATERIA);
        return;
    }
    sendResponse(Middleware::RESP_EXITO, "Eliminado", clienteSocket,
                 Middleware::OP_ELIMINAR_MATERIA);
    registrarConexion("Middleware -> Frontend", "subject_delete [stub]");
}