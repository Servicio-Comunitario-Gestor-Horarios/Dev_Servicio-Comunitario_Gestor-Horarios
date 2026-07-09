#include "internalserver.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <backend/data/profesor.hpp> // Incluimos el modelo de Nicole

// IMPORTANTE: Asegúrate de tener el puntero a ServicioProfesor declarado en internalserver.h

InternalServer::InternalServer(QObject *parent) : QObject(parent) {
    m_server = new QLocalServer(this);
    // m_servicioProfesor = new ServicioProfesor(); // Inicializa tu backend aquí

    // Registro de rutas (Middleware)
    m_routes["teacher_list"] = [this](const QJsonObject &data) { handleTeacherList(); };
    m_routes["teacher_get"]  = [this](const QJsonObject &data) { handleTeacherGet(data); };
    m_routes["teacher_create"] = [this](const QJsonObject &data) { handleTeacherCreate(data); };
    m_routes["teacher_update"] = [this](const QJsonObject &data) { handleTeacherUpdate(data); };
    m_routes["teacher_delete"] = [this](const QJsonObject &data) { handleTeacherDelete(data); };
}

void InternalServer::sendResponse(int status, const QJsonValue &data) {
    QJsonObject response;
    response["status"] = status;
    response["data"] = data;
    qDebug() << "Respuesta enviada:" << QJsonDocument(response).toJson(QJsonDocument::Compact);
}

void InternalServer::handleTeacherList() {
    sendResponse(0, QJsonArray());
}

void InternalServer::handleTeacherGet(const QJsonObject &data) {
    sendResponse(-2, QJsonObject());
}

void InternalServer::handleTeacherCreate(const QJsonObject &data) {
    // 1. Validar campos (Criterio de Aceptación)
    if (!data.contains("nombre") || !data.contains("correo")) {
        sendResponse(-4, "Faltan campos requeridos");
        return;
    }

    // 2. Convertir JSON a objeto Profesor (el "molde" de Nicole)
    Profesor nuevoProfesor = Profesor::deJson(data);

    // 3. Integración con el backend (Descomenta cuando tengas el puntero)
    /*
    if (m_servicioProfesor->existe(nuevoProfesor.nombre)) {
        sendResponse(-1, "Error: Profesor ya existe");
    } else {
        m_servicioProfesor->agregar(nuevoProfesor);
        sendResponse(0, nuevoProfesor.aJson());
    }
    */

    // Por ahora, dejamos esto para que compile y pase tus pruebas actuales:
    sendResponse(0, data);
}

void InternalServer::handleTeacherUpdate(const QJsonObject &data) {
    sendResponse(0, data);
}

void InternalServer::handleTeacherDelete(const QJsonObject &data) {
    sendResponse(0, "Eliminado");
}