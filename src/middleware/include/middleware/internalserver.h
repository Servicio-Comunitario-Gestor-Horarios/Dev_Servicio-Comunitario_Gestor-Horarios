#pragma once

#include <QObject>
#include <QLocalServer>
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

private slots:
    void onNewConnection();
    void onReadyRead();
    void onClientDisconnected();

private:
    QLocalServer *m_server;
    QHash<QString, std::function<void(const QJsonObject&, QLocalSocket*)>> m_rutas;
    QTimer *m_timeoutTimer;

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
