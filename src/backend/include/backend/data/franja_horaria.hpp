#pragma once

#include <QTime>
#include <QJsonObject>

struct FranjaHoraria {
    
    int dia;
    QTime inicio;
    QTime fin;

    QJsonObject toJson() const;
    static FranjaHoraria fromJson(const QJsonObject& obj);
};