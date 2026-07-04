#include "backend/data/plan_estudio.hpp"
#include <QJsonArray>

QJsonObject PlanEstudio::toJson() const {

    QJsonObject obj;
    obj["nombre"] = nombre;

    QJsonArray arreglo;
    for (const auto& aux : materias) {
        arreglo.append(aux.toJson());
    }

    obj["materias"] = arreglo;

    return obj;
}

PlanEstudio PlanEstudio::fromJson(const QJsonObject& obj) {

    PlanEstudio plan_estudio;
    plan_estudio.nombre = obj["nombre"].toString();

    for (const auto& aux : obj["materias"].toArray()) {
        plan_estudio.materias.append(Materia::fromJson(aux.toObject()));
    }

    return plan_estudio;
}