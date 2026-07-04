#include "backend/data/aula.hpp"

QJsonObject Aula::toJson() const {

    QJsonObject obj;
    obj["nombre"] = nombre;
    obj["capacidad"] = capacidad;
    obj["locacion"] = locacion;

    return obj;
};

Aula Aula::fromJson(const QJsonObject& obj) {

    Aula aula;
    aula.nombre = obj["nombre"].toString();
    aula.capacidad = obj["capacidad"].toInt();
    aula.locacion = obj["locacion"].toString();

    return aula;
}