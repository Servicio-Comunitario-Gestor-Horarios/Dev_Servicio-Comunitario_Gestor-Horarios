#pragma once

#include <QString>
#include <QJsonObject>
#include <QVector>
#include "backend/data/materia.hpp"

/// Materia dentro de un plan de estudio, con año/curso y horas específicas.
/// Value object del solver — sin id de persistencia.
struct MateriaEnPlan {
    int curso;       ///< Año o semestre donde se dicta
    int horas;       ///< Horas semanales en este plan
    Materia materia;

    QJsonObject toJson() const;
    static MateriaEnPlan fromJson(const QJsonObject& obj);
};

/// Plan de estudio que agrupa materias por curso.
/// Value object del solver — sin id de persistencia.
struct PlanEstudio {
    
    QString nombre;
    QVector<MateriaEnPlan> materias;

    QJsonObject toJson() const;
    static PlanEstudio fromJson(const QJsonObject& obj);
};