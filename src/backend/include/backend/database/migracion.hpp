#pragma once

#include <QSqlDatabase>

namespace Migracion {
    /// Ejecuta todas las migraciones CREATE TABLE IF NOT EXISTS.
    /// Retorna false si alguna falla, true si todas fueron exitosas.
    bool runAll(QSqlDatabase& db);
}