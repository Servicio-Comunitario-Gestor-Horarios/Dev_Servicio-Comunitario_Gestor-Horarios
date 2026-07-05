#pragma once

#include <QString>

namespace Middleware {

// Nombre del socket local para la comunicación IPC
inline const QString SERVER_NAME = QStringLiteral("GestorHorarios_Middleware");

// Códigos de operación
inline const QString OP_HEALTH_CHECK = QStringLiteral("health_check");
inline const QString OP_READY        = QStringLiteral("ready");
inline const QString OP_SHUTDOWN     = QStringLiteral("shutdown");

// Operaciones CRUD
inline const QString OP_TEACHER_LIST   = QStringLiteral("teacher_list");
inline const QString OP_TEACHER_GET    = QStringLiteral("teacher_get");
inline const QString OP_TEACHER_CREATE = QStringLiteral("teacher_create");
inline const QString OP_TEACHER_UPDATE = QStringLiteral("teacher_update");
inline const QString OP_TEACHER_DELETE = QStringLiteral("teacher_delete");

// Códigos de respuesta
inline constexpr int RESP_OK        = 0;
inline constexpr int RESP_ERROR     = -1;
inline constexpr int RESP_NOT_FOUND = -2;
inline constexpr int RESP_TIMEOUT   = -3;
inline constexpr int RESP_INVALID   = -4;

} // namespace Middleware
