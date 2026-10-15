#pragma once

#include <QSqlDatabase>

namespace Migracion
{
    /// Crea **de una sola vez** el catálogo completo del esquema único.
    ///
    /// No hay pasos versionados ni parámetro de versión: la base se crea entera.
    /// Retorna false si alguna sentencia falla. No gestiona transacciones: la
    /// transacción la abre y cierra `VersionEsquema::crearEsquema`.
    bool aplicarPaso(QSqlDatabase& db);
}
