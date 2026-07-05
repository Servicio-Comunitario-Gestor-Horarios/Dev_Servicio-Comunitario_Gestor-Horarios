#pragma once

#include <QObject>
#include <QLocalSocket>
#include <QJsonObject>

class InternalClient : public QObject {
    Q_OBJECT
public:
    explicit InternalClient(QObject *parent = nullptr);

    /// Envía una solicitud health-check al servidor IPC.
    Q_INVOKABLE void sendHealthCheck();

    /// Envía una solicitud IPC genérica.
    Q_INVOKABLE void sendRequest(const QString &op, const QJsonObject &payload = QJsonObject());

signals:
    /// Se emite cuando se recibe la respuesta del health-check.
    void healthCheckResponseReceived(bool success);

    /// Se emite con la respuesta completa de cualquier operación.
    void responseReceived(const QJsonObject &response);

private slots:
    void onConnected();
    void onReadyRead();
    void onErrorOccurred(QLocalSocket::LocalSocketError error);

private:
    QLocalSocket *m_socket;
    QString m_pendingOp;
    QJsonObject m_pendingPayload;
};
