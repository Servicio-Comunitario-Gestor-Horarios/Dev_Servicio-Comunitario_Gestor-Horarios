#pragma once

#include <QString>
#include <QJsonObject>
#include <QVector>

/// Materia o asignatura del pensum.
/// Value object del solver — sin id de persistencia.
struct Materia {

    QString nombre;
    int horas_semanales;              ///< Horas cátedra por semana
    QVector<QString> requerimientos;  ///< Requisitos previos (nombres)

    QJsonObject toJson() const;
    static Materia fromJson(const QJsonObject& obj);
};