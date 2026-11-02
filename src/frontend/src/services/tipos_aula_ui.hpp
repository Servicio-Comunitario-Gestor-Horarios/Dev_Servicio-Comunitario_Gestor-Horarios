#pragma once

/**
 * @file tipos_aula_ui.hpp
 * @brief Ayudas de interfaz para el catálogo de tipos de aula (spec 009).
 *
 * Desde la 009 las aulas y las materias referencian el catálogo `TiposAula`
 * por id; la interfaz ya no puede inventarse el tipo. Estas ayudas rellenan
 * los combos y resuelven el tipo general («Aula de estudio») para las altas
 * rápidas que solo piden un nombre.
 */

#include <QComboBox>
#include <QString>
#include <QVector>

#include "backend/services/ServicioTiposAula.hpp"

namespace TiposAulaUI {

/// Nombre del tipo general del catálogo (el que exige el aula de referencia).
inline QString nombreTipoGeneral()
{
    return QStringLiteral("Aula de estudio");
}

/// Rellena `combo` con el catálogo; deja seleccionado `idSeleccionado` si existe.
inline void poblar(QComboBox* combo, const QVector<TipoAulaDTO>& tipos,
                   int idSeleccionado = -1)
{
    if (!combo)
        return;

    combo->clear();
    for (const TipoAulaDTO& tipo : tipos)
        combo->addItem(tipo.nombre, tipo.id);

    if (idSeleccionado > 0) {
        const int indice = combo->findData(idSeleccionado);
        if (indice >= 0)
            combo->setCurrentIndex(indice);
    }
}

/// Id del tipo general; si no está, el primero del catálogo (o -1 si está vacío).
inline int idTipoGeneral(const QVector<TipoAulaDTO>& tipos)
{
    for (const TipoAulaDTO& tipo : tipos) {
        if (tipo.nombre.compare(nombreTipoGeneral(), Qt::CaseInsensitive) == 0)
            return tipo.id;
    }
    return tipos.isEmpty() ? -1 : tipos.front().id;
}

} // namespace TiposAulaUI
