# MEMORY.md — Estado del proyecto

> Plantilla de estado SDD. Se actualiza al cerrar cada fase.

## Fase actual

- **Fase:** implementación en curso — T1 y T2 de 001 hechas
- **Fecha:** 2026-10-08
- **Rama:** `sdd/001-base-datos-local`
- **Specs en curso:** `specs/001-base-datos-local` (implementando), `specs/002-generacion-horarios-ipc` (pendiente)
- **Siguiente paso:** implementar T3 de `specs/001-base-datos-local` (apertura con migración y respaldo obligatorio)

## Specs activas

| Spec | Estado | Fase |
|---|---|---|
| 001-base-datos-local | aprobada | implementando (T1–T2 hechas; siguiente T3) |
| 002-generacion-horarios-ipc | aprobada | tareas redactadas (depende de 001) |

> La antigua spec `002-frontend-db-ipc-solver` se dividió en dos (001 + 002) por tamaño
> (superaba las 10 tareas) y se renumeró desde 001.

## Decisiones

- Convenciones iniciales en `AGENTS.md`: C++17 + Qt6 + OR-Tools, tests con `ctest --preset full`, textos en español.
- Constitución en `docs/constitution.md` (revisada; se mantiene la versión de 6 principios con detalle).
- Limpieza de repo (2026-10-08):
  - `.gitignore`: + editors/IDE (`.vscode/`, `.idea/`, `*.user`), tooling de agentes (`.agents/`, `.claude/`, `.atl/`, `.codegraph/`) y generados (`repomix-output.xml`, `Registro.txt`, `__pycache__/`).
  - Fuera del índice (ya no existen en disco): `.agents/` (39), `.vscode/` (2), `.claude/` (2), `.atl/` (2) y `compile_commands.json` (se regenera).
  - Borrado físico: `repomix-output.xml` (2,1 MB, regenerable). `Registro.txt` solo ignorado, se queda en disco.
  - `TESTS-SOLVER-CONTEXT.md` movido a `docs/informes/`.
  - Archivos SDD (`AGENTS.md`, `MEMORY.md`, `specs/`, `docs/constitution.md`) trackeados: van en el repo.
  - Dos commits separados: `e434ed8` (chore: gitignore + des-tracking + SDD) y `f92c063` (docs: mover TESTS-SOLVER-CONTEXT).
  - Verificación: `git check-ignore` en los 10 patrones ✓; rebuild limpio en `build-test/` y `ctest` 2/2 ✓ (Qt middleware). El preset `full` (OR-Tools) solo corre en Docker: no hay OR-Tools en el host.
- Specs 001/002 (2026-10-08), decisiones de clarificación relevantes para el plan:
  - El **lado cliente** (programa salvo el proceso de cálculo) es dueño de la base de datos; el solver se mantiene separado por IPC.
  - **La base solo persiste entidades de dominio** (docentes, aulas, asignaturas/materias, cursos, planes de estudio, turnos, recesos). La **configuración del solver, los presets y los horarios generados** se gestionan como **archivos JSON**, NO como BD.
  - **Respaldo obligatorio** (no opcional) antes de migrar o descartar la base; si no es posible, no se migra/descarta.
  - Migración **por versión de esquema** (solo si es anterior); versión ausente/ilegible o posterior → fallo de apertura. No hay bases previas (las primeras se crean tras esta implementación).
  - Timeout del proceso de cálculo: **60 s**; salud y apagado: 5 s.
  - Borrado **en cascada con confirmación**, atómico.
  - **`docs/constitution.md` §5 enmendada**: se quitaron "horarios guardados en SQLite" del listado de datos; presets/horarios se gestionan como archivos JSON.
  - Limpieza de restos de frontend: eliminados `src/frontend/src/main.cpp`, `src/frontend/src/aplicacion_frontend.cpp` y su `.hpp` huérfano, más sus entradas del CMake (rompían `cmake --preset full`); el ejecutable real vive en `src/app`.

## Siguiente paso

1. Implementar T3 de `specs/001-base-datos-local` (apertura con migración y respaldo obligatorio), y seguir T4–T10.
2. Después, `specs/002-generacion-horarios-ipc/tasks.md` (depende de 001).

## Progreso de implementación

- 001: T1 (esquema versionado) ✅, T2 (respaldo) ✅. Tests rojo→verde por tarea.
- Tests ajenos que fallan (no tocar): `test_servicio_profesor` (trabajo sin commitear), `test_solver_horarios`, `test_solver_benchmark` (solver).
- Entorno: tests dentro del contenedor `gestor-dev` → `docker exec gestor-dev bash -lc "cd /workspace && cmake --preset full && cmake --build build && ctest --preset full --output-on-failure"`. OR-Tools solo en el contenedor; el host no lo tiene.
