#include "backend/data/materia.hpp"
#include <QJsonObject>
#include <QJsonArray>

QJsonObject Materia::toJson() const{

    QJsonObject obj;
    obj["nombre"] = nombre;
    obj["horas_semanales"] = horas_semanales;

    QJsonArray arreglo;
    for (const auto& aux : requerimientos) {
        arreglo.append(aux);
    }

    obj["requerimientos"] = arreglo;

    return obj;
}

Materia Materia::fromJson(const QJsonObject& obj) {

    Materia materia;
    materia.nombre = obj["nombre"].toString();
    materia.horas_semanales = obj["horas_semanales"].toInt();
    
    for (const auto& aux : obj["requerimientos"].toArray()) {
        materia.requerimientos.append(aux.toString());
    }

    return materia;
}