#pragma once

#include <QString>
#include <QJsonObject>

struct Aula {
    
    QString nombre;
    int capacidad;
    QString locacion;

    QJsonObject toJson() const;
    static Aula fromJson(const QJsonObject& obj);

};
