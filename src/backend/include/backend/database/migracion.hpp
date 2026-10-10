#pragma once

#include <QSqlDatabase>

namespace Migracion
{
    /// Aplica el paso de migración correspondiente a `versionDestino`.
    ///
    /// Retorna false si esa versión no está implementada o si alguna sentencia
    /// falla. No gestiona transacciones: las abre y cierra
    /// `VersionEsquema::aplicarHasta` una por cada versión.
    bool aplicarPaso(QSqlDatabase& db, int versionDestino);
}
