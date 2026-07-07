#pragma once

#include <QTime>
#include <QJsonObject>
#include <QVector>
#include <backend/data/franja_horaria.hpp>

/// Profesor o facilitador que dicta materias.
/// Value object del solver — sin id de persistencia.
/// Las materias se almacenan como nombres (string) porque
/// el solver no necesita el objeto Materia completo.
struct Profesor {

    QString nombre;
    QVector<FranjaHoraria> disponibilidad;  ///< Bloques donde está disponible
    QVector<QString> materias;              ///< Nombres de materias que dicta

    QJsonObject toJson() const;
    static Profesor fromJson(const QJsonObject& obj);
};