#pragma once

#include <QSqlDatabase>

namespace Migracion {
    bool runAll(QSqlDatabase& db);
}