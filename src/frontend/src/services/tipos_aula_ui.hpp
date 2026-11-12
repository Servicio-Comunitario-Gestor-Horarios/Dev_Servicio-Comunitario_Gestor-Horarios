#pragma once

/**
 * @file tipos_aula_ui.hpp
 * @brief Ayudas de interfaz para el catálogo de tipos de aula (spec 009).
 *
 * Desde la 009 las aulas y las materias referencian el catálogo `TiposAula`
 * por id; la interfaz ya no puede inventarse el tipo. Estas ayudas rellenan
 * los combos y resuelven el tipo general para las altas rápidas que solo piden
 * un nombre.
 */

#include <QComboBox>
#include <QString>
#include <QVector>

#include "backend/resultado.hpp"
#include "backend/services/ServicioTiposAula.hpp"
#include "common/tipo_general.hpp"

namespace TiposAulaUI {

/// Nombre del tipo general del catálogo (el que exige el aula de referencia).
inline QString nombreTipoGeneral()
{
    return tipo_general::nombre();
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

/// Id del tipo general del catálogo. Si no está (o el catálogo está vacío), error con el
/// mensaje de qué falta y qué hacer. NUNCA devuelve otro tipo (016 RF-2).
inline Resultado<int> idTipoGeneral(const QVector<TipoAulaDTO>& tipos)
{
    for (const TipoAulaDTO& tipo : tipos)
        if (tipo_general::esNombreCanonico(tipo.nombre))
            return Resultado<int>::exito(tipo.id);

    return Resultado<int>::error(QStringLiteral(
        "El catálogo de tipos de aula no contiene el tipo general «%1»: la base no está en el "
        "estado esperado. Restaure un respaldo de la versión vigente o cree una base nueva "
        "(con respaldo obligatorio). No se ha guardado ningún dato.").arg(tipo_general::nombre()));
}

} // namespace TiposAulaUI
