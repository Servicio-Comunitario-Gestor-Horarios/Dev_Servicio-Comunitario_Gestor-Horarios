#ifndef MESSAGES_H
#define MESSAGES_H

#include <QString>

namespace Middleware {

// --- CRUD Materias ---
inline constexpr auto OP_LISTA_MATERIAS = QStringLiteral("subject_list");
inline constexpr auto OP_OBTENER_MATERIA = QStringLiteral("subject_get");
inline constexpr auto OP_CREAR_MATERIA = QStringLiteral("subject_create");
inline constexpr auto OP_ACTUALIZAR_MATERIA = QStringLiteral("subject_update");
inline constexpr auto OP_ELIMINAR_MATERIA = QStringLiteral("subject_delete");

// --- CRUD Aulas ---
inline constexpr auto OP_LISTA_AULAS = QStringLiteral("classroom_list");
inline constexpr auto OP_OBTENER_AULA = QStringLiteral("classroom_get");
inline constexpr auto OP_CREAR_AULA = QStringLiteral("classroom_create");
inline constexpr auto OP_ACTUALIZAR_AULA = QStringLiteral("classroom_update");
inline constexpr auto OP_ELIMINAR_AULA = QStringLiteral("classroom_delete");

// --- CRUD Planes de Estudio ---
inline constexpr auto OP_LISTA_PLANES = QStringLiteral("plan_list");
inline constexpr auto OP_OBTENER_PLAN = QStringLiteral("plan_get");
inline constexpr auto OP_CREAR_PLAN = QStringLiteral("plan_create");
inline constexpr auto OP_ACTUALIZAR_PLAN = QStringLiteral("plan_update");
inline constexpr auto OP_ELIMINAR_PLAN = QStringLiteral("plan_delete");

// --- Carga Horaria ---
inline constexpr auto OP_ASIGNAR_CARGA = QStringLiteral("load_assign");
inline constexpr auto OP_ACTUALIZAR_CARGA = QStringLiteral("load_update");
inline constexpr auto OP_ELIMINAR_CARGA = QStringLiteral("load_delete");
inline constexpr auto OP_LISTAR_CARGA_PLAN = QStringLiteral("load_list_by_plan");
inline constexpr auto OP_OBTENER_COMPOSICION_PLAN = QStringLiteral("plan_composition");

} // namespace Middleware

#endif // MESSAGES_H
