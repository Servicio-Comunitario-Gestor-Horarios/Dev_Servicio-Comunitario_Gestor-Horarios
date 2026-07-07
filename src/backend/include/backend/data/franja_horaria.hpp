#pragma once

#include <QTime>
#include <QJsonObject>

/// Bloque de tiempo para disponibilidad o asignación de clases.
/// Value object del solver — sin id de persistencia.
struct FranjaHoraria {
    
    int dia;          ///< 0=domingo, 1=lunes... 6=sábado
    QTime inicio;     ///< HH:mm
    QTime fin;        ///< HH:mm

    QJsonObject toJson() const;
    static FranjaHoraria fromJson(const QJsonObject& obj);
};