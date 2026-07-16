#ifndef MESSAGES_H
#define MESSAGES_H

#include <QString>

namespace Middleware {
// --- CRUD Materias ---
inline const QString OP_LISTA_MATERIAS = "subject_list";
inline const QString OP_OBTENER_MATERIA = "subject_get";
inline const QString OP_CREAR_MATERIA = "subject_create";
inline const QString OP_ACTUALIZAR_MATERIA = "subject_update";
inline const QString OP_ELIMINAR_MATERIA = "subject_delete";

// --- CRUD Aulas ---
inline const QString OP_LISTA_AULAS = "classroom_list";
inline const QString OP_OBTENER_AULA = "classroom_get";
inline const QString OP_CREAR_AULA = "classroom_create";
inline const QString OP_ACTUALIZAR_AULA = "classroom_update";
inline const QString OP_ELIMINAR_AULA = "classroom_delete";

// --- CRUD Planes de Estudio ---
inline const QString OP_LISTA_PLANES = "plan_list";
inline const QString OP_OBTENER_PLAN = "plan_get";
inline const QString OP_CREAR_PLAN = "plan_create";
inline const QString OP_ACTUALIZAR_PLAN = "plan_update";
inline const QString OP_ELIMINAR_PLAN = "plan_delete";

// --- Carga Horaria ---
inline const QString OP_ASIGNAR_CARGA = "load_assign";
inline const QString OP_ACTUALIZAR_CARGA = "load_update";
inline const QString OP_ELIMINAR_CARGA = "load_delete";
inline const QString OP_LISTAR_CARGA_PLAN = "load_list_by_plan";
inline const QString OP_OBTENER_COMPOSICION_PLAN = "plan_composition";
}

#endif // MESSAGES_H