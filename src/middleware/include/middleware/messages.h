#pragma once

/**
 * @file messages.h
 * @brief Constantes del protocolo IPC (QLocalSocket).
 */

#include <QString>

namespace Middleware {

inline const QString SERVER_NAME = QStringLiteral("GestorHorarios_Middleware");

// ─── Códigos de operación ──────────────────────────────────────────

inline const QString OP_HEALTH_CHECK = QStringLiteral("health_check");
inline const QString OP_LISTO        = QStringLiteral("ready");
inline const QString OP_APAGAR       = QStringLiteral("shutdown");

// ─── Operaciones CRUD Profesores ───────────────────────────────────

inline const QString OP_LISTA_PROFESORES    = QStringLiteral("teacher_list");
inline const QString OP_OBTENER_PROFESOR    = QStringLiteral("teacher_get");
inline const QString OP_CREAR_PROFESOR      = QStringLiteral("teacher_create");
inline const QString OP_ACTUALIZAR_PROFESOR = QStringLiteral("teacher_update");
inline const QString OP_ELIMINAR_PROFESOR   = QStringLiteral("teacher_delete");

// ─── Operaciones CRUD Aulas ────────────────────────────────────────

inline const QString OP_LISTA_AULAS      = QStringLiteral("classroom_list");
inline const QString OP_OBTENER_AULA     = QStringLiteral("classroom_get");
inline const QString OP_CREAR_AULA       = QStringLiteral("classroom_create");
inline const QString OP_ACTUALIZAR_AULA  = QStringLiteral("classroom_update");
inline const QString OP_ELIMINAR_AULA    = QStringLiteral("classroom_delete");

// ─── Operaciones CRUD Materias ────────────────────────────────────

inline const QString OP_LISTA_MATERIAS       = QStringLiteral("subject_list");
inline const QString OP_OBTENER_MATERIA      = QStringLiteral("subject_get");
inline const QString OP_CREAR_MATERIA        = QStringLiteral("subject_create");
inline const QString OP_ACTUALIZAR_MATERIA   = QStringLiteral("subject_update");
inline const QString OP_ELIMINAR_MATERIA     = QStringLiteral("subject_delete");

// ─── Códigos de respuesta ──────────────────────────────────────────

inline constexpr int RESP_EXITO          = 0;
inline constexpr int RESP_ERROR          = -1;
inline constexpr int RESP_NO_ENCONTRADO  = -2;
inline constexpr int RESP_TIEMPO_AGOTADO = -3;
inline constexpr int RESP_INVALIDO       = -4;

} // namespace Middleware
