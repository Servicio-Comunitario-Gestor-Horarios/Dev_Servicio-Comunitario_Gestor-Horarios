#ifndef INTERNALSERVER_H
#define INTERNALSERVER_H

#include <QObject>
#include <QLocalServer>
#include <QTimer>
#include <QJsonObject>
#include <QJsonArray>
#include <QHash>
#include <QSqlDatabase>
#include <functional>

namespace Middleware {

class InternalServer : public QObject {
    Q_OBJECT
public:
    explicit InternalServer(QObject *parent = nullptr);
    void processRequest(const QString &op, const QJsonObject &data);
    
    // C4: Método necesario para la prueba QVERIFY
    bool isListening() const { return m_server ? m_server->isListening() : false; }

private slots:
    void onNewConnection();

private:
    QLocalServer *m_server;
    QTimer *m_timeoutTimer;
    QSqlDatabase m_db;
    QHash<QString, std::function<void(const QJsonObject&)>> m_routes;

    void sendResponse(int status, const QJsonValue &data);

    // C3: Declaraciones de handlers (Mantén las 18 firmas aquí)
    void handleTeacherList(); 
    void handleTeacherGet(const QJsonObject &d); 
    void handleTeacherCreate(const QJsonObject &d); 
    void handleTeacherUpdate(const QJsonObject &d); 
    void handleTeacherDelete(const QJsonObject &d);
    
    void handleSubjectList(); void handleSubjectGet(const QJsonObject &d); void handleSubjectCreate(const QJsonObject &d); void handleSubjectUpdate(const QJsonObject &d); void handleSubjectDelete(const QJsonObject &d);
    void handleClassroomList(); void handleClassroomGet(const QJsonObject &d); void handleClassroomCreate(const QJsonObject &d); void handleClassroomUpdate(const QJsonObject &d); void handleClassroomDelete(const QJsonObject &d);
    void handlePlanList(); void handlePlanGet(const QJsonObject &d); void handlePlanCreate(const QJsonObject &d); void handlePlanUpdate(const QJsonObject &d); void handlePlanDelete(const QJsonObject &d);
};

} // namespace Middleware

#endif // INTERNALSERVER_H
