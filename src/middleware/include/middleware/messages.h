#pragma once

/**
 * @file messages.h
 * @brief Constantes del protocolo IPC (QLocalSocket).
 *
 * Define los códigos de operación, nombres de servidor y códigos
 * de respuesta del middleware de comunicación entre frontend y backend.
 */

#include <QString>

namespace Middleware {

/** @brief Nombre del socket local para la comunicación IPC. */
inline const QString SERVER_NAME = QStringLiteral("GestorHorarios_Middleware");

// ─── Códigos de operación ──────────────────────────────────────────

/** @brief Verificación de salud del backend. */
inline const QString OP_HEALTH_CHECK = QStringLiteral("health_check");

/** @brief El backend está listo para recibir peticiones. */
inline const QString OP_LISTO = QStringLiteral("ready");

/** @friend Apagado graceful del backend. */
inline const QString OP_APAGAR = QStringLiteral("shutdown");

// ─── Operaciones CRUD Profesores ───────────────────────────────────

/** @brief Obtener lista de profesores. */
inline const QString OP_LISTA_PROFESORES   = QStringLiteral("teacher_list");
/** @brief Obtener un profesor por ID. */
inline const QString OP_OBTENER_PROFESOR   = QStringLiteral("teacher_get");
/** @brief Crear un nuevo profesor. */
inline const QString OP_CREAR_PROFESOR     = QStringLiteral("teacher_create");
/** @brief Actualizar un profesor existente. */
inline const QString OP_ACTUALIZAR_PROFESOR = QStringLiteral("teacher_update");
/** @brief Eliminar un profesor. */
inline const QString OP_ELIMINAR_PROFESOR   = QStringLiteral("teacher_delete");

// ─── Códigos de respuesta ──────────────────────────────────────────

/** @brief Operación exitosa. */
inline constexpr int RESP_EXITO         = 0;
/** @brief Error general en la operación. */
inline constexpr int RESP_ERROR         = -1;
/** @brief Recurso no encontrado. */
inline constexpr int RESP_NO_ENCONTRADO = -2;
/** @brief Tiempo de espera agotado en la operación. */
inline constexpr int RESP_TIEMPO_AGOTADO = -3;
/** @brief Solicitud inválida o mal formada. */
inline constexpr int RESP_INVALIDO      = -4;

} // namespace Middleware
