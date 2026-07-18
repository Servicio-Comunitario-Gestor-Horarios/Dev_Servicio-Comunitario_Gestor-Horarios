#include "internalserver.h"
#include "messages.h"
#include <QLocalSocket>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>

#include "servicioprofesor.h"
#include "serviciosubject.h"
#include "servicioaula.h"
#include "servicioplan.h"

namespace Middleware {

InternalServer::InternalServer(QObject *parent) : QObject(parent) {
    m_server = new QLocalServer(this);
    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setSingleShot(true);

    connect(m_server, &QLocalServer::newConnection, this, &InternalServer::onNewConnection);
    m_server->listen(QStringLiteral("gestor-horarios-middleware"));

    m_routes[QStringLiteral("teacher_list")] = [this](auto d) { handleTeacherList(); };
    m_routes[QStringLiteral("teacher_get")] = [this](auto d) { handleTeacherGet(d); };
    m_routes[QStringLiteral("teacher_create")] = [this](auto d) { handleTeacherCreate(d); };
    m_routes[QStringLiteral("teacher_update")] = [this](auto d) { handleTeacherUpdate(d); };
    m_routes[QStringLiteral("teacher_delete")] = [this](auto d) { handleTeacherDelete(d); };

    m_routes[QStringLiteral("subject_list")] = [this](auto d) { handleSubjectList(); };
    m_routes[QStringLiteral("subject_get")] = [this](auto d) { handleSubjectGet(d); };
    m_routes[QStringLiteral("subject_create")] = [this](auto d) { handleSubjectCreate(d); };
    m_routes[QStringLiteral("subject_update")] = [this](auto d) { handleSubjectUpdate(d); };
    m_routes[QStringLiteral("subject_delete")] = [this](auto d) { handleSubjectDelete(d); };

    m_routes[QStringLiteral("classroom_list")] = [this](auto d) { handleClassroomList(); };
    m_routes[QStringLiteral("classroom_get")] = [this](auto d) { handleClassroomGet(d); };
    m_routes[QStringLiteral("classroom_create")] = [this](auto d) { handleClassroomCreate(d); };
    m_routes[QStringLiteral("classroom_update")] = [this](auto d) { handleClassroomUpdate(d); };
    m_routes[QStringLiteral("classroom_delete")] = [this](auto d) { handleClassroomDelete(d); };

    m_routes[QStringLiteral("plan_list")] = [this](auto d) { handlePlanList(); };
    m_routes[QStringLiteral("plan_get")] = [this](auto d) { handlePlanGet(d); };
    m_routes[QStringLiteral("plan_create")] = [this](auto d) { handlePlanCreate(d); };
    m_routes[QStringLiteral("plan_update")] = [this](auto d) { handlePlanUpdate(d); };
    m_routes[QStringLiteral("plan_delete")] = [this](auto d) { handlePlanDelete(d); };
}

void InternalServer::onNewConnection() {
    QLocalSocket *socket = m_server->nextPendingConnection();
    if (socket) connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
}

void InternalServer::processRequest(const QString &op, const QJsonObject &data) {
    if (m_routes.contains(op)) {
        m_routes[op](data);
    } else {
        sendResponse(RESP_INVALIDO, QJsonValue(QStringLiteral("Operación no encontrada")));
    }
}

void InternalServer::sendResponse(int status, const QJsonValue &data) {
    QJsonObject res;
    res[QStringLiteral("status")] = status;
    res[QStringLiteral("data")] = data;
    qDebug() << "Enviando respuesta status:" << status;
}

// Implementación de Handlers con Conversión a JSON explícita

void InternalServer::handleTeacherList() {
    ServicioProfesor s(m_db);
    QJsonArray array;
    for (const auto& item : s.listarProfesores()) { array.append(item.toJson()); }
    sendResponse(RESP_OK, array);
}

void InternalServer::handleTeacherGet(const QJsonObject &d) {
    ServicioProfesor s(m_db);
    sendResponse(RESP_OK, s.getProfesor(d["id"].toInt()).toJson());
}

void InternalServer::handleTeacherCreate(const QJsonObject &d) {
    ServicioProfesor s(m_db);
    sendResponse(RESP_OK, s.createProfesor(Profesor::fromJson(d)).toJson());
}

void InternalServer::handleTeacherUpdate(const QJsonObject &d) {
    ServicioProfesor s(m_db);
    sendResponse(RESP_OK, s.updateProfesor(Profesor::fromJson(d)).toJson());
}

void InternalServer::handleTeacherDelete(const QJsonObject &d) {
    ServicioProfesor s(m_db);
    sendResponse(RESP_OK, QJsonValue(s.deleteProfesor(d["id"].toInt())));
}

void InternalServer::handleSubjectList() {
    ServicioSubject s(m_db);
    QJsonArray array;
    for (const auto& item : s.listarMaterias()) { array.append(item.toJson()); }
    sendResponse(RESP_OK, array);
}

void InternalServer::handleSubjectGet(const QJsonObject &d) {
    ServicioSubject s(m_db);
    sendResponse(RESP_OK, s.getMateria(d["id"].toInt()).toJson());
}

void InternalServer::handleSubjectCreate(const QJsonObject &d) {
    ServicioSubject s(m_db);
    sendResponse(RESP_OK, s.createMateria(Materia::fromJson(d)).toJson());
}

void InternalServer::handleSubjectUpdate(const QJsonObject &d) {
    ServicioSubject s(m_db);
    sendResponse(RESP_OK, s.updateMateria(Materia::fromJson(d)).toJson());
}

void InternalServer::handleSubjectDelete(const QJsonObject &d) {
    ServicioSubject s(m_db);
    sendResponse(RESP_OK, QJsonValue(s.deleteMateria(d["id"].toInt())));
}

void InternalServer::handleClassroomList() {
    ServicioAula s(m_db);
    QJsonArray array;
    for (const auto& item : s.listarAulas()) { array.append(item.toJson()); }
    sendResponse(RESP_OK, array);
}

void InternalServer::handleClassroomGet(const QJsonObject &d) {
    ServicioAula s(m_db);
    sendResponse(RESP_OK, s.getAula(d["id"].toInt()).toJson());
}

void InternalServer::handleClassroomCreate(const QJsonObject &d) {
    ServicioAula s(m_db);
    sendResponse(RESP_OK, s.createAula(Aula::fromJson(d)).toJson());
}

void InternalServer::handleClassroomUpdate(const QJsonObject &d) {
    ServicioAula s(m_db);
    sendResponse(RESP_OK, s.updateAula(Aula::fromJson(d)).toJson());
}

void InternalServer::handleClassroomDelete(const QJsonObject &d) {
    ServicioAula s(m_db);
    sendResponse(RESP_OK, QJsonValue(s.deleteAula(d["id"].toInt())));
}

void InternalServer::handlePlanList() {
    ServicioPlan s(m_db);
    QJsonArray array;
    for (const auto& item : s.listarPlanes()) { array.append(item.toJson()); }
    sendResponse(RESP_OK, array);
}

void InternalServer::handlePlanGet(const QJsonObject &d) {
    ServicioPlan s(m_db);
    sendResponse(RESP_OK, s.getPlan(d["id"].toInt()).toJson());
}

void InternalServer::handlePlanCreate(const QJsonObject &d) {
    ServicioPlan s(m_db);
    sendResponse(RESP_OK, s.createPlan(PlanEstudio::fromJson(d)).toJson());
}

void InternalServer::handlePlanUpdate(const QJsonObject &d) {
    ServicioPlan s(m_db);
    sendResponse(RESP_OK, s.updatePlan(PlanEstudio::fromJson(d)).toJson());
}

void InternalServer::handlePlanDelete(const QJsonObject &d) {
    ServicioPlan s(m_db);
    sendResponse(RESP_OK, QJsonValue(s.deletePlan(d["id"].toInt())));
}

} // namespace Middleware
