#pragma once

#include <QString>
#include <QJsonObject>
#include <QVector>
#include "backend/data/materia.hpp"

struct PlanEstudio {
    
    QString nombre;
    QVector<Materia> materias;

    QJsonObject toJson() const;
    static PlanEstudio fromJson(const QJsonObject& obj);
};