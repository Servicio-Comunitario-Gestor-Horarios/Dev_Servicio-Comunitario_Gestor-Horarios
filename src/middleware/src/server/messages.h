#ifndef MESSAGES_H
#define MESSAGES_H

#include <QString>

namespace Middleware {
// Nombre del servidor IPC
inline const QString SERVER_NAME = "GestorHorariosIPC";

// Constantes para las operaciones de profesores
inline const QString teacher_list   = "teacher_list";
inline const QString teacher_get    = "teacher_get";
inline const QString teacher_create = "teacher_create";
inline const QString teacher_update = "teacher_update";
inline const QString teacher_delete = "teacher_delete";
}

#endif // MESSAGES_H