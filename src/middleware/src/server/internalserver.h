#ifndef INTERNALSERVER_H
#define INTERNALSERVER_H

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>

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
    void logConnection(const QString &direction, const QString &operation);
};

#endif // INTERNALSERVER_H