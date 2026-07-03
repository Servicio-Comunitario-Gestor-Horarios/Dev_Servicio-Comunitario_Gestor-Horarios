#include "backend/data/horario.hpp"
#include <QJsonArray>

// --- Asignacion ---
QJsonObject Asignacion::toJson() const {

    QJsonObject obj;
    obj["aula"] = aula.toJson();
    obj["franja_horaria"] = franja_horaria.toJson();
    obj["materia"] = materia.toJson();
    obj["profesor"] = profesor.toJson();

    return obj;
}

Asignacion Asignacion::fromJson(const QJsonObject& obj) {

    Asignacion asignacion;
    asignacion.aula = Aula::fromJson(obj["aula"].toObject());
    asignacion.franja_horaria = FranjaHoraria::fromJson(obj["franja_horaria"].toObject());
    asignacion.materia = Materia::fromJson(obj["materia"].toObject());
    asignacion.profesor = Profesor::fromJson(obj["profesor"].toObject());

    return asignacion;
}

// --- Horario ---
QJsonObject Horario::toJson() const {

    QJsonObject obj;

    QJsonArray arreglo;
    for (const auto& aux : asignaciones) {
        arreglo.append(aux.toJson());
    }

    obj["asignaciones"] = arreglo;

    return obj;
}

Horario Horario::fromJson(const QJsonObject& obj) {

    Horario horario;

    for (const auto& aux : obj["asignaciones"].toArray()) {
        horario.asignaciones.append(Asignacion::fromJson(aux.toObject()));
    }

    return horario;
}