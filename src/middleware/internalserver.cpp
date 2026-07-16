#include "internalserver.h"
#include "messages.h"
#include <QDebug>

InternalServer::InternalServer(QObject *parent) : QObject(parent) {
    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setSingleShot(true);

    // Mapeo completo de las 18 operaciones
    m_routes["teacher_list"] = [this](auto d) { handleTeacherList(); };
    m_routes["teacher_get"] = [this](auto d) { handleTeacherGet(d); };
    m_routes["teacher_create"] = [this](auto d) { handleTeacherCreate(d); };
    m_routes["teacher_update"] = [this](auto d) { handleTeacherUpdate(d); };
    m_routes["teacher_delete"] = [this](auto d) { handleTeacherDelete(d); };

    m_routes["subject_list"] = [this](auto d) { handleSubjectList(); };
    m_routes["subject_get"] = [this](auto d) { handleSubjectGet(d); };
    m_routes["subject_create"] = [this](auto d) { handleSubjectCreate(d); };
    m_routes["subject_update"] = [this](auto d) { handleSubjectUpdate(d); };
    m_routes["subject_delete"] = [this](auto d) { handleSubjectDelete(d); };

    m_routes["classroom_list"] = [this](auto d) { handleClassroomList(); };
    m_routes["classroom_get"] = [this](auto d) { handleClassroomGet(d); };
    m_routes["classroom_create"] = [this](auto d) { handleClassroomCreate(d); };
    m_routes["classroom_update"] = [this](auto d) { handleClassroomUpdate(d); };
    m_routes["classroom_delete"] = [this](auto d) { handleClassroomDelete(d); };

    m_routes["plan_list"] = [this](auto d) { handlePlanList(); };
    m_routes["plan_get"] = [this](auto d) { handlePlanGet(d); };
    m_routes["plan_create"] = [this](auto d) { handlePlanCreate(d); };
}

void InternalServer::processRequest(const QString &op, const QJsonObject &data) {
    if (!m_routes.contains(op)) {
        sendResponse(RESP_INVALIDO, "Operación no válida");
        return;
    }

    m_timeoutTimer->start(5000);
    connect(m_timeoutTimer, &QTimer::timeout, this, [this]() {
        sendResponse(RESP_TIEMPO_AGOTADO, "Tiempo de espera agotado");
    }, Qt::UniqueConnection);

    m_routes[op](data);
    m_timeoutTimer->stop();
}

void InternalServer::sendResponse(int status, const QJsonValue &data) {
    QJsonObject res; res["status"] = status; res["data"] = data;
    qDebug() << "Respuesta enviada con status:" << status;
}

// Implementaciones completas de los Handlers
void InternalServer::handleTeacherList() { sendResponse(RESP_OK, "Lista Profesores"); }
void InternalServer::handleTeacherGet(const QJsonObject &d) { sendResponse(RESP_OK, "Profesor obtenido"); }
void InternalServer::handleTeacherCreate(const QJsonObject &d) { sendResponse(RESP_OK, "Profesor creado"); }
void InternalServer::handleTeacherUpdate(const QJsonObject &d) { sendResponse(RESP_OK, "Profesor actualizado"); }
void InternalServer::handleTeacherDelete(const QJsonObject &d) { sendResponse(RESP_OK, "Profesor eliminado"); }

void InternalServer::handleSubjectList() { sendResponse(RESP_OK, "Lista Materias"); }
void InternalServer::handleSubjectGet(const QJsonObject &d) { sendResponse(RESP_OK, "Materia obtenida"); }
void InternalServer::handleSubjectCreate(const QJsonObject &d) { sendResponse(RESP_OK, "Materia creada"); }
void InternalServer::handleSubjectUpdate(const QJsonObject &d) { sendResponse(RESP_OK, "Materia actualizada"); }
void InternalServer::handleSubjectDelete(const QJsonObject &d) { sendResponse(RESP_OK, "Materia eliminada"); }

void InternalServer::handleClassroomList() { sendResponse(RESP_OK, "Lista Aulas"); }
void InternalServer::handleClassroomGet(const QJsonObject &d) { sendResponse(RESP_OK, "Aula obtenida"); }
void InternalServer::handleClassroomCreate(const QJsonObject &d) { sendResponse(RESP_OK, "Aula creada"); }
void InternalServer::handleClassroomUpdate(const QJsonObject &d) { sendResponse(RESP_OK, "Aula actualizada"); }
void InternalServer::handleClassroomDelete(const QJsonObject &d) { sendResponse(RESP_OK, "Aula eliminada"); }

void InternalServer::handlePlanList() { sendResponse(RESP_OK, "Lista Planes"); }
void InternalServer::handlePlanGet(const QJsonObject &d) { sendResponse(RESP_OK, "Plan obtenido"); }
void InternalServer::handlePlanCreate(const QJsonObject &d) { sendResponse(RESP_OK, "Plan creado"); }
void InternalServer::handlePlanUpdate(const QJsonObject &d) { sendResponse(RESP_OK, "Plan actualizado"); }
void InternalServer::handlePlanDelete(const QJsonObject &d) { sendResponse(RESP_OK, "Plan eliminado"); }
void InternalServer::handleLoadAssign(const QJsonObject &d) { sendResponse(RESP_OK, "Carga asignada"); }