#pragma once

#include <QString>
#include <QJsonObject>
#include <QVector>

struct Materia {

    QString nombre;
    int horas_semanales;
    QVector<QString> requerimientos;

    QJsonObject toJson() const;
    static Materia fromJson(const QJsonObject& obj);
};