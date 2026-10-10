# AGENTS.md — Convenciones del proyecto Gestor-Horarios

## Identidad del proyecto

- **Qué es:** sistema de gestión y optimización de horarios académicos (OR-Tools CP-SAT), aplicación de escritorio.
- **Stack:** C++17 · Qt6 (Widgets/Sql/Network) · OR-Tools v9.15+ · SQLite · CMake 3.24+ / Ninja · CTest + Google Test / Qt Test.
- **Estructura de módulos:** `src/app`, `src/common`, `src/backend` (dominio, solver, services, database), `src/datos` (ciclo de vida de la base del cliente), `src/middleware` (server/client/validation), `src/frontend` (views, dialogs, widgets, models). Tests en `test/`.

## Idioma

- **Código y comentarios:** español (nombres de identificadores en inglés convencional cuando ya existen; comentarios, mensajes y nombres de tests en español).
- **Textos de usuario, docs y specs:** español.

## Comando de tests

```bash
cmake --preset full
cmake --build build
ctest --preset full        # o: ctest --test-dir build --output-on-failure
```

- Presets en `CMakePresets.json`: `full` (backend + tests) y `dev-frontend` (sin OR-Tools, sin tests).
- Tests: Google Test (lógica/backend) y Qt Test (middleware/frontend). Añadir tests con `add_gtest` / `add_qtest` en `test/CMakeLists.txt`.
- **Tests primero:** en cada tarea SDD, escribe los tests en rojo antes del código.

## Spec-Driven Development

Flujo: **Constitución → Spec → Clarificación → Plan → Tareas → Implementación → Validación → Cambio.**

- Nunca pasar a la fase siguiente sin aprobación explícita del usuario.
- La spec manda: si algo no está en la spec, no se implementa. Si falta una decisión, se para y se pregunta.
- Un cambio de requisitos entra primero en la spec, luego en el plan y las tareas, y al final en el código.
- La spec describe el QUÉ y el POR QUÉ: nada de stack, arquitectura ni nombres de archivos (eso va en `plan.md`).
- Principios en `docs/constitution.md`. Estado global en `MEMORY.md`.

### Estructura de specs

```
specs/
├── README.md
└── NNN-nombre/
    ├── spec.md     # QUÉ y POR QUÉ (aprobada | borrador | implementada), RF en EARS
    ├── plan.md     # CÓMO: archivos, funciones puras, pseudocódigo, decisiones, tests
    └── tasks.md    # Tn con "Hecho cuando:", ≤20-30 min por tarea, en orden de dependencia
```

- `NNN` es un número de 3 dígitos correlativo (`001-`, `002-`, …); `nombre` en kebab-case.
- Máximo 10 tareas por spec; si salen más, proponer dividirla.
- El implementador hace **una tarea cada vez**: tests rojo → código → tests verde → marcar → parar.
- Al cerrar una fase, actualizar `MEMORY.md`.

## Build y repo

- No commitear `build/`, `build-fe/` ni binarios. No generar archivos fuera de la raíz del proyecto.
- Documentación viva en `docs/` (guías, diseño, manuales). No duplicar lo que ya existe en `docs/`.
- Ignorados en `.gitignore` (nunca commitear): editors/IDE (`.vscode/`, `.idea/`, `*.user`), tooling de agentes (`.agents/`, `.claude/`, `.atl/`, `.codegraph/`) y generados (`compile_commands.json`, `repomix-output.xml`, `Registro.txt`, `__pycache__/`).
- Los archivos SDD (`AGENTS.md`, `MEMORY.md`, `specs/`, `docs/constitution.md`) **sí se versionan**.
- Detalle completo de la limpieza de repo: `MEMORY.md` → Decisiones.
