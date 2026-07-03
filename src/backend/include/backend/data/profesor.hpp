#pragma once

#include <QTime>
#include <QJsonObject>
#include <QVector>
#include <backend/data/franja_horaria.hpp>

struct Profesor {

    QString nombre;
    QVector<FranjaHoraria> disponibilidad;
    QVector<QString> materias;

    QJsonObject toJson() const;
    static Profesor fromJson(const QJsonObject& obj);
};