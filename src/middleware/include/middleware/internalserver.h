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
