#include <middleware/internalserver.h>
#include <middleware/messages.h>
#include <middleware/ipc_framing.hpp>

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>
#include <QPointer>

// ─── Ciclo de vida ─────────────────────────────────────────────────

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

    inicializarRutas();

    qDebug() << "Servidor Middleware IPC escuchando en:" << Middleware::SERVER_NAME;
    connect(m_server, &QLocalServer::newConnection, this, &InternalServer::onNewConnection);
    return true;
}

/// Solo rutas de sistema. Las rutas de negocio (CRUD, solver) las registra `app`.
void InternalServer::inicializarRutas()
{
    m_rutas[Middleware::OP_HEALTH_CHECK] = [this](const QJsonObject &, QLocalSocket *s) {
        registrarConexion("Middleware -> Frontend", "health-check [ok]");
        responder(s, Middleware::RESP_EXITO, "ok");
    };
    m_rutas[Middleware::OP_LISTO] = [this](const QJsonObject &, QLocalSocket *s) {
        registrarConexion("Middleware -> Frontend", "ready [ok]");
        responder(s, Middleware::RESP_EXITO, "ok");
    };
    m_rutas[Middleware::OP_APAGAR] = [this](const QJsonObject &, QLocalSocket *s) {
        registrarConexion("Middleware -> Frontend", "shutdown [ok]");
        responder(s, Middleware::RESP_EXITO, "ok");
    };
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

// ─── Recepción ─────────────────────────────────────────────────────

void InternalServer::onReadyRead()
{
    QLocalSocket *socket = qobject_cast<QLocalSocket*>(sender());
    if (!socket) return;

    QByteArray &buffer = m_buffers[socket];
    buffer.append(socket->readAll());

    bool overflow = false;
    const QVector<QByteArray> frames = Middleware::takeCompleteFrames(buffer, &overflow);
    if (overflow) {
        rechazar(socket, QString(), Middleware::RESP_INVALIDO, "Frame demasiado grande");
    }

    for (const QByteArray &frame : frames) {
        const QJsonDocument doc = QJsonDocument::fromJson(frame);
        if (!doc.isObject()) {
            rechazar(socket, QString(), Middleware::RESP_INVALIDO, "JSON malformado");
            continue;
        }

        const QJsonObject obj = doc.object();
        const QString op = obj["op"].toString();

        if (obj.value("v").toInt(0) != Middleware::PROTOCOL_VERSION) {
            rechazar(socket, op, Middleware::RESP_VERSION_INCOMPATIBLE,
                     QString("Versión de protocolo no soportada (se espera %1)")
                         .arg(Middleware::PROTOCOL_VERSION));
            continue;
        }

        if (!m_rutas.contains(op)) {
            registrarConexion("Middleware -> Frontend", "operación desconocida: " + op);
            rechazar(socket, op, Middleware::RESP_INVALIDO, "Operación desconocida: " + op);
            continue;
        }

        QJsonObject payload = obj["payload"].toObject();
        if (payload.isEmpty())
            payload = obj["data"].toObject();

        registrarConexion("Frontend -> Middleware", op);
        m_colas[socket].enqueue(Peticion{op, payload});
    }

    despacharSiguiente(socket);
}

// ─── Despacho (una petición en vuelo por socket) ───────────────────

void InternalServer::despacharSiguiente(QLocalSocket* cliente)
{
    if (!cliente) return;
    if (m_despachando.contains(cliente)) return;
    m_despachando.insert(cliente);

    QQueue<Peticion> &cola = m_colas[cliente];
    while (true) {
        auto itPend = m_pendientes.find(cliente);
        if (itPend != m_pendientes.end() && !itPend->respondido)
            break;                          // ya hay una en vuelo
        if (cola.isEmpty())
            break;

        const Peticion peticion = cola.dequeue();
        const quint64 generacion = ++m_generacion;
        m_pendientes[cliente] = Pending{peticion.op, false, generacion};

        QPointer<QLocalSocket> guard(cliente);
        const int timeoutMs = m_timeoutMs;
        QTimer::singleShot(timeoutMs, this, [this, guard, generacion, timeoutMs]() {
            if (!guard || guard->state() != QLocalSocket::ConnectedState)
                return;
            auto it = m_pendientes.find(guard.data());
            if (it == m_pendientes.end() || it->respondido || it->generacion != generacion)
                return;
            it->respondido = true;
            const QString op = it->op;
            m_pendientes.erase(it);
            enviarFrame(guard, op, Middleware::RESP_TIEMPO_AGOTADO,
                        QString("Operacion tardo mas de %1 s").arg(timeoutMs / 1000));
            despacharSiguiente(guard.data());
        });

        m_rutas[peticion.op](peticion.payload, cliente);

        // Síncrono → el pending ya se respondió: seguir con la cola.
        // Asíncrono → el pending sigue vivo: cortar.
        auto it = m_pendientes.find(cliente);
        if (it != m_pendientes.end() && !it->respondido)
            break;
    }

    m_despachando.remove(cliente);
}

// ─── Respuestas ────────────────────────────────────────────────────

void InternalServer::responder(QLocalSocket* cliente, int status, const QJsonValue& data)
{
    if (!cliente) return;

    auto it = m_pendientes.find(cliente);
    if (it == m_pendientes.end() || it->respondido)
        return;                             // sin petición en vuelo o respuesta tardía

    const QString op = it->op;
    m_pendientes.erase(it);
    enviarFrame(cliente, op, status, data);
    despacharSiguiente(cliente);
}

void InternalServer::rechazar(QLocalSocket* cliente, const QString& op, int status,
                              const QJsonValue& data)
{
    if (!cliente) return;
    enviarFrame(cliente, op, status, data);  // no altera el slot en vuelo
}

void InternalServer::enviarFrame(QLocalSocket* cliente, const QString& op, int status,
                                 const QJsonValue& data)
{
    QJsonObject respuesta;
    respuesta["v"]      = Middleware::PROTOCOL_VERSION;
    respuesta["op"]     = op;
    respuesta["status"] = (status == Middleware::RESP_EXITO) ? "ok" : "error";
    respuesta["code"]   = status;
    respuesta["data"]   = data;

    cliente->write(Middleware::encodeFrame(respuesta));
    cliente->flush();
}

// ─── Utilidades ────────────────────────────────────────────────────

void InternalServer::onClientDisconnected()
{
    QLocalSocket *clienteSocket = qobject_cast<QLocalSocket*>(sender());
    if (clienteSocket) {
        m_buffers.remove(clienteSocket);
        m_colas.remove(clienteSocket);
        m_pendientes.remove(clienteSocket);
        m_despachando.remove(clienteSocket);
        clienteSocket->deleteLater();
    }
}

void InternalServer::setTimeoutMs(int ms)
{
    m_timeoutMs = ms;
}

QString InternalServer::serverName() const
{
    return m_server->serverName();
}

void InternalServer::registerRoute(const QString &op,
                                   std::function<void(const QJsonObject&, QLocalSocket*)> handler)
{
    if (m_rutas.contains(op)) {
        qWarning() << "Sobrescribiendo ruta existente:" << op;
    }
    m_rutas[op] = std::move(handler);
}

void InternalServer::registrarConexion(const QString &direccion, const QString &operacion)
{
    QString timestamp = QDateTime::currentDateTime()
                            .toString("yyyy-MM-dd hh:mm:ss.zzz");
    qDebug() << QStringLiteral("[%1] [%2] Operación: %3")
                    .arg(timestamp, direccion, operacion);
}
