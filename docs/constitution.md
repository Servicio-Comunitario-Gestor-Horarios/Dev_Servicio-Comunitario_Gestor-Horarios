# Constitución del proyecto

> Principios rectores del desarrollo con Spec-Driven Development en Gestor-Horarios.
> Revisar con `/sdd-constitution`. Cambios aquí mandan sobre specs, planes y código.

## 1. Simplicidad del stack

- Se usa el stack ya establecido del proyecto: C++17, Qt6, OR-Tools, SQLite, CMake/Ninja, CTest.
- Cualquier nueva dependencia o tecnología necesita justificación explícita en el plan y aprobación del usuario.
- Se prefiere la solución más simple que cumpla la spec; no se anticipa complejidad que la spec no pida.

## 2. Relación entre spec y código

- La spec manda: si algo no está en la spec, no se implementa.
- La spec describe el QUÉ y el POR QUÉ; el plan describe el CÓMO. Nada de stack ni nombres de archivos en `spec.md`.
- Todo cambio de requisitos entra primero en la spec, luego en `plan.md`/`tasks.md` y al final en el código.
- Cada función o módulo nuevo debe poder trazarse a un RF de alguna spec.

## 3. Separación entre lógica e interfaz

- La lógica de negocio y el solver viven en `src/backend` (y `src/common`); la interfaz Qt vive en `src/frontend`; `src/middleware` solo transporta y valida.
- El backend no depende de Qt Widgets; la lógica es testeable sin levantar la UI.
- Reglas de negocio nuevas: primero en backend con tests, después se reflejan en la interfaz.

## 4. Política de tests

- Tests primero: en cada tarea, los tests se escriben en rojo antes que el código, y la tarea solo se marca al pasar en verde.
- Comando de tests del proyecto: `cmake --preset full && cmake --build build && ctest --preset full`.
- Toda RF debe tener al menos una comprobación verificable en `tasks.md` ("Hecho cuando: …"). Sin cobertura, la tarea no está hecha.
- Se usan los frameworks ya integrados: Google Test (lógica/backend) y Qt Test (middleware/frontend).

## 5. Protección de los datos del usuario

- Los datos del usuario (planes de estudio, docentes, aulas, horarios guardados en SQLite) nunca se borran ni se sobrescriben sin confirmación explícita.
- Toda operación destructiva o migración de esquema debe ser reversible o ir acompañada de respaldo.
- La información sensible no se incluye en logs, dumps ni specs; los fixtures de test usan datos ficticios.

## 6. Idioma del código y de los textos

- **Código, comentarios y identificadores nuevos:** español para comentarios, mensajes y nombres de tests; inglés para identificadores cuando siguen convención existente del módulo.
- **Docs, specs, planes, tareas y textos de usuario:** español en todo el proyecto.
- Consistencia por encima de preferencia personal: en un módulo existente se sigue el estilo que ya tenga.
