#ifndef INTERNALSERVER_H
#define INTERNALSERVER_H

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QJsonObject>
#include <QHash>
#include <functional>

// Incluimos el modelo de datos para poder usarlo en las firmas de los métodos
#include <backend/data/profesor.hpp>

// Si tienes una clase ServicioProfesor, asegúrate de tener su include aquí
// #include <backend/servicioprofesor.hpp>

class InternalServer : public QObject {
    Q_OBJECT
public:
    explicit InternalServer(QObject *parent = nullptr);

private:
    QLocalServer *m_server;

    // Aquí registramos las funciones que se ejecutarán según la operación recibida
    QHash<QString, std::function<void(const QJsonObject&)>> m_routes;

    // Puntero al servicio de Nicole (descomenta cuando lo necesites)
    // ServicioProfesor *m_servicioProfesor;

    // Manejadores CRUD solicitados en la Tarea 16
    void handleTeacherList();
    void handleTeacherGet(const QJsonObject &data);
    void handleTeacherCreate(const QJsonObject &data);
    void handleTeacherUpdate(const QJsonObject &data);
    void handleTeacherDelete(const QJsonObject &data);

    // Método auxiliar para enviar respuestas al frontend
    void sendResponse(int status, const QJsonValue &data);
};

#endif // INTERNALSERVER_H