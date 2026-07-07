#include "backend/data/plan_estudio.hpp"
#include <QJsonArray>

QJsonObject MateriaEnPlan::toJson() const {

    QJsonObject obj;
    obj["curso"] = curso;
    obj["horas"] = horas;
    obj["materia"] = materia.toJson();

    return obj;
}

MateriaEnPlan MateriaEnPlan::fromJson(const QJsonObject& obj) {

    MateriaEnPlan materia_plan;
    materia_plan.curso = obj["curso"].toInt();
    materia_plan.horas = obj["horas"].toInt();
    materia_plan.materia = Materia::fromJson(obj["materia"].toObject());

}

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
        plan_estudio.materias.append(MateriaEnPlan::fromJson(aux.toObject()));
    }

    return plan_estudio;
}