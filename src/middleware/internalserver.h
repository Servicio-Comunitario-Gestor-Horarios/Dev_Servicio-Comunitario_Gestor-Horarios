#ifndef INTERNALSERVER_H
#define INTERNALSERVER_H

#include <QObject>
#include <QLocalServer>
#include <QTimer>
#include <QJsonObject>
#include <QHash>
#include <functional>

// Constantes de respuesta
const int RESP_OK = 0;
const int RESP_INVALIDO = -1;
const int RESP_TIEMPO_AGOTADO = -2;

class InternalServer : public QObject {
    Q_OBJECT
public:
    explicit InternalServer(QObject *parent = nullptr);
    void processRequest(const QString &op, const QJsonObject &data);

private:
    QLocalServer *m_server;
    QHash<QString, std::function<void(const QJsonObject&)>> m_routes;
    QTimer *m_timeoutTimer;

    void sendResponse(int status, const QJsonValue &data);

    // Handlers (Todos los requeridos)
    void handleTeacherList(); void handleTeacherGet(const QJsonObject &d);
    void handleTeacherCreate(const QJsonObject &d); void handleTeacherUpdate(const QJsonObject &d);
    void handleTeacherDelete(const QJsonObject &d);
    void handleSubjectList(); void handleSubjectGet(const QJsonObject &d);
    void handleSubjectCreate(const QJsonObject &d); void handleSubjectUpdate(const QJsonObject &d);
    void handleSubjectDelete(const QJsonObject &d);
    void handleClassroomList(); void handleClassroomGet(const QJsonObject &d);
    void handleClassroomCreate(const QJsonObject &d); void handleClassroomUpdate(const QJsonObject &d);
    void handleClassroomDelete(const QJsonObject &d);
    void handlePlanList(); void handlePlanGet(const QJsonObject &d);
    void handlePlanCreate(const QJsonObject &d); void handlePlanUpdate(const QJsonObject &d);
    void handlePlanDelete(const QJsonObject &d);
    void handleLoadAssign(const QJsonObject &d);
};
#endif