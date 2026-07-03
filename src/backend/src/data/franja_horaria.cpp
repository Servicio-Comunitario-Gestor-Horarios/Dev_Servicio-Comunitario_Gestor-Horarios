#include "backend/data/franja_horaria.hpp"

QJsonObject FranjaHoraria::toJson() const {

    QJsonObject obj;
    obj["dia"] = dia;
    obj["inicio"] = inicio.toString("HH:mm");
    obj["fin"] = fin.toString("HH:mm");

    return obj;
}

FranjaHoraria FranjaHoraria::fromJson(const QJsonObject& obj) {

    FranjaHoraria franja_horaria;
    franja_horaria.dia = obj["dia"].toInt();
    franja_horaria.inicio = QTime::fromString(obj["inicio"].toString(), "HH:mm");
    franja_horaria.fin = QTime::fromString(obj["fin"].toString(), "HH:mm");

    return franja_horaria;
}