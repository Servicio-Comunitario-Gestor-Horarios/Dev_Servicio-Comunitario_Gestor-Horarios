# MEMORY.md — Estado del proyecto

> Plantilla de estado SDD. Se actualiza al cerrar cada fase.

## Fase actual

- **Fase:** mantenimiento — limpieza de repo
- **Fecha:** 2026-10-08
- **Spec en curso:** ninguna (cambio de infraestructura, sin spec)
- **Siguiente paso:** commits de limpieza → `/sdd-spec 001-<idea>`

## Specs activas

| Spec | Estado | Fase |
|---|---|---|
| — | — | — |

## Decisiones

- Convenciones iniciales en `AGENTS.md`: C++17 + Qt6 + OR-Tools, tests con `ctest --preset full`, textos en español.
- Constitución en `docs/constitution.md` (revisada; se mantiene la versión de 6 principios con detalle).
- Limpieza de repo (2026-10-08):
  - `.gitignore`: + editors/IDE (`.vscode/`, `.idea/`, `*.user`), tooling de agentes (`.agents/`, `.claude/`, `.atl/`, `.codegraph/`) y generados (`repomix-output.xml`, `Registro.txt`, `__pycache__/`).
  - Fuera del índice (ya no existen en disco): `.agents/` (39), `.vscode/` (2), `.claude/` (2), `.atl/` (2) y `compile_commands.json` (se regenera).
  - Borrado físico: `repomix-output.xml` (2,1 MB, regenerable). `Registro.txt` solo ignorado, se queda en disco.
  - `TESTS-SOLVER-CONTEXT.md` movido a `docs/informes/`.
  - Archivos SDD (`AGENTS.md`, `MEMORY.md`, `specs/`, `docs/constitution.md`) trackeados: van en el repo.
  - Dos commits separados (chore/docs); `ipc_framing.hpp` queda fuera, pendiente del commit de middleware.

## Siguiente paso

1. Commit 1 (chore) y commit 2 (docs) de la limpieza.
2. Abrir la primera spec con `/sdd-spec 001-<idea>`.
