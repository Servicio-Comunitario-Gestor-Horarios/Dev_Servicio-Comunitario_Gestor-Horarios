#pragma once

#include <QString>
#include <QJsonObject>
#include <QVector>
#include "backend/data/materia.hpp"

struct MateriaEnPlan {
    int curso;
    int horas;
    Materia materia;

    QJsonObject toJson() const;
    static MateriaEnPlan fromJson(const QJsonObject& obj);
};

struct PlanEstudio {
    
    QString nombre;
    QVector<MateriaEnPlan> materias;

    QJsonObject toJson() const;
    static PlanEstudio fromJson(const QJsonObject& obj);
};