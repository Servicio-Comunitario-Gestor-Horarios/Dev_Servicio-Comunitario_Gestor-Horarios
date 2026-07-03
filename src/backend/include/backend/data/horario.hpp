#pragma once

#include <QString>
#include <QJsonObject>
#include <QVector>
#include <backend/data/aula.hpp>
#include <backend/data/franja_horaria.hpp>
#include <backend/data/materia.hpp>
#include <backend/data/profesor.hpp>

struct Asignacion {

    Aula aula;
    FranjaHoraria franja_horaria;
    Materia materia;
    Profesor profesor;

};

struct Horario {

    QVector<Asignacion> asignaciones;

    QJsonObject toJson() const;
    static Horario fromJson(const QJsonObject& obj);
};
