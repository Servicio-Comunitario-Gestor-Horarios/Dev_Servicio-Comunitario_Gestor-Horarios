#pragma once
#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QHash>
#include <QQueue>
#include <QSet>
#include <QTimer>
#include <functional>

class InternalServer : public QObject {
    Q_OBJECT
public:
    explicit InternalServer(QObject *parent = nullptr);
    bool start();
    void setTimeoutMs(int ms);
    QString serverName() const;
    void registerRoute(const QString &op,
                       std::function<void(const QJsonObject&, QLocalSocket*)> handler);
    void responder(QLocalSocket* cliente, int status, const QJsonValue& data);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onClientDisconnected();

private:
    struct Peticion { QString op; QJsonObject payload; };
    struct Pending  { QString op; bool respondido = false; quint64 generacion = 0; };

    QLocalServer *m_server;
    QHash<QString, std::function<void(const QJsonObject&, QLocalSocket*)>> m_rutas;
    int m_timeoutMs = 5000;

    QHash<QLocalSocket*, QByteArray>       m_buffers;
    QHash<QLocalSocket*, QQueue<Peticion>> m_colas;
    QHash<QLocalSocket*, Pending>          m_pendientes;
    QSet<QLocalSocket*>                    m_despachando;
    quint64 m_generacion = 0;

    void inicializarRutas();
    void registrarConexion(const QString &direccion, const QString &operacion);
    void despacharSiguiente(QLocalSocket* cliente);
    void rechazar(QLocalSocket* cliente, const QString& op, int status, const QJsonValue& data);
    void enviarFrame(QLocalSocket* cliente, const QString& op, int status, const QJsonValue& data);
};
```


/**#pragma once
//#include <QObject>
/*#include <QLocalServer>
#include <QLocalSocket>
#include <QJsonObject>
#include <QHash>
#include <QTimer>
#include <functional>

class InternalServer : public QObject {
    Q_OBJECT
public:
    explicit InternalServer(QObject *parent = nullptr);
    bool start();
    void setTimeoutMs(int ms);
    QString serverName() const;
    void registerRoute(const QString &op,
                       std::function<void(const QJsonObject&, QLocalSocket*)> handler);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onClientDisconnected();

private:
    QLocalServer *m_server;
    QHash<QString, std::function<void(const QJsonObject&, QLocalSocket*)>> m_rutas;
    int m_timeoutMs = 5000;

    struct Pending { 
        QString op;
        bool respondido = false;
    };
    QHash<QLocalSocket *, Pending> m_pendientes;
     
    void inicializarRutas();
    void registrarConexion(const QString &direccion, const QString &operacion);
    void sendResponse(int status, const QJsonValue &data, QLocalSocket *clienteSocket);

    // ─── Handlers Profesores ──────────────────────────────────────
    void handleTeacherList(QLocalSocket *clienteSocket);
    void handleTeacherGet(const QJsonObject &data, QLocalSocket *clienteSocket);
    void handleTeacherCreate(const QJsonObject &data, QLocalSocket *clienteSocket);
    void handleTeacherUpdate(const QJsonObject &data, QLocalSocket *clienteSocket);
    void handleTeacherDelete(const QJsonObject &data, QLocalSocket *clienteSocket);

    // ─── Handlers Aulas ───────────────────────────────────────────
    void handleClassroomList(QLocalSocket *clienteSocket);
    void handleClassroomGet(const QJsonObject &data, QLocalSocket *clienteSocket);
    void handleClassroomCreate(const QJsonObject &data, QLocalSocket *clienteSocket);
    void handleClassroomUpdate(const QJsonObject &data, QLocalSocket *clienteSocket);
    void handleClassroomDelete(const QJsonObject &data, QLocalSocket *clienteSocket);

    // ─── Handlers Materias ────────────────────────────────────────
    void handleSubjectList(QLocalSocket *clienteSocket);
    void handleSubjectGet(const QJsonObject &data, QLocalSocket *clienteSocket);
    void handleSubjectCreate(const QJsonObject &data, QLocalSocket *clienteSocket);
    void handleSubjectUpdate(const QJsonObject &data, QLocalSocket *clienteSocket);
    void handleSubjectDelete(const QJsonObject &data, QLocalSocket *clienteSocket);
};
*/