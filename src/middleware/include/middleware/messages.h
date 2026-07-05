#pragma once

#include <QString>

namespace Middleware {

// Nombre del socket local para la comunicación IPC
inline const QString SERVER_NAME = QStringLiteral("GestorHorarios_Middleware");

// Códigos de operación
inline const QString OP_HEALTH_CHECK = QStringLiteral("health_check");

} // namespace Middleware
