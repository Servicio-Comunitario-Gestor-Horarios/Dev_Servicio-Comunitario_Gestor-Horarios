#pragma once

#include <QString>
#include <QJsonObject>

/// Aula o salón disponible para asignar clases.
/// Value object del solver — sin id de persistencia.
struct Aula {
    
    QString nombre;
    int capacidad;     ///< Número máximo de estudiantes
    QString locacion;  ///< Edificio, piso o ubicación física

    QJsonObject toJson() const;
    static Aula fromJson(const QJsonObject& obj);

};
