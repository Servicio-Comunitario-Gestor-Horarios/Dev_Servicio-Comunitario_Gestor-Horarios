#ifndef INTERNALSERVER_H
#define INTERNALSERVER_H

#include <QObject>
#include <QLocalServer>
#include <QTimer>
#include <QJsonObject>
#include <QHash>
#include <functional>

namespace Middleware {

/**
 * @class InternalServer
 * @brief Gestiona las peticiones mediante un sistema de rutas QHash.
 */
class InternalServer : public QObject {
    Q_OBJECT
public:
    explicit InternalServer(QObject *parent = nullptr);
    void processRequest(const QString &op, const QJsonObject &data);

private slots:
    void onNewConnection(); // Necesario para el punto C4

private:
    QLocalServer *m_server;
    QHash<QString, std::function<void(const QJsonObject&)>> m_routes;
    QTimer *m_timeoutTimer;

    void sendResponse(int status, const QJsonValue &data);

    // Handlers (Se deben mantener para ser implementados en el .cpp)
    void handleTeacherList();
    void handleTeacherGet(const QJsonObject &d);
    // ... (mantén todas las declaraciones de handlers que ya tienes)
};

} // namespace Middleware

#endif // INTERNALSERVER_H
