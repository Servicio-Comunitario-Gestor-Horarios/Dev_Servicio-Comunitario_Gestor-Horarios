#pragma once

#include <QObject>
#include <QLocalSocket>

class InternalClient : public QObject {
    Q_OBJECT
public:
    explicit InternalClient(QObject *parent = nullptr);

    /// Envía una solicitud health-check al servidor IPC.
    Q_INVOKABLE void sendHealthCheck();

signals:
    /// Se emite cuando se recibe la respuesta del health-check.
    void healthCheckResponseReceived(bool success);

private slots:
    void onConnected();
    void onReadyRead();
    void onErrorOccurred(QLocalSocket::LocalSocketError error);

private:
    QLocalSocket *m_socket;
};
