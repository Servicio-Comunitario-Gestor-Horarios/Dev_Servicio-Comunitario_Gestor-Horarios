#include "backend/data/profesor.hpp"
#include <QJsonArray>

QJsonObject Profesor::toJson() const {

    QJsonObject obj;
    obj["nombre"] = nombre;

    QJsonArray arreglo_disponibilidad;
    for (const auto& aux : disponibilidad) {
        arreglo_disponibilidad.append(aux.toJson());
    };

    obj["disponibilidad"] = arreglo_disponibilidad;

    QJsonArray arreglo_materias;
    for (const auto& aux : materias) {
        arreglo_materias.append(aux);
    };

    obj["materias"] = arreglo_materias;

    return obj;
};

Profesor Profesor::fromJson(const QJsonObject& obj) {

    Profesor profesor;
    profesor.nombre = obj["nombre"].toString();

    for (const auto& aux : obj["disponibilidad"].toArray()) {
        profesor.disponibilidad.append(FranjaHoraria::fromJson(aux.toObject()));
    }

    for (const auto& aux : obj["materias"].toArray()) {
        profesor.materias.append(aux.toString());
    }

    return profesor;
};