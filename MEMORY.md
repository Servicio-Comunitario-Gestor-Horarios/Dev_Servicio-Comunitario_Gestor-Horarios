# MEMORY.md — Estado del proyecto

> Plantilla de estado SDD. Se actualiza al cerrar cada fase.

## Fase actual

- **Fase:** spec 001 **implementada** (T1–T8 completas) — pendiente arreglar el ordenamiento de
  `ServicioProfesor`; siguiente spec 002
- **Fecha:** 2026-10-09
- **Rama:** `sdd/001-base-datos-local`
- **Specs en curso:** `specs/001-base-datos-local` (implementada), `specs/002-generacion-horarios-ipc` (pendiente)
- **Siguiente paso:** implementar `specs/002-generacion-horarios-ipc/tasks.md` (depende de 001)

## Specs activas

| Spec | Estado | Fase |
|---|---|---|
| 001-base-datos-local | aprobada | **implementada** (T1–T8) |
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
- **Alcance (2026-10-09):** NO implementamos `src/frontend` (interfaz Qt: vistas, listas, diálogos e indicadores): lo hace **otro equipo**. Nuestros entregables: `src/backend`, `src/app` y `src/middleware`. Exponemos la lógica y el **contrato de consumo** documentado en `docs/interfaz-frontend.md`. Las specs 001/002 y sus planes/tareas se reajustaron (fuera las tareas de frontend; nueva tarea de documentar la interfaz).

## Siguiente paso

1. `specs/002-generacion-horarios-ipc/tasks.md` (depende de 001).
2. **Deuda técnica pendiente:** arreglar el ordenamiento de `ServicioProfesor` (test
   `test_servicio_profesor` en rojo por ese motivo; es trabajo ajeno a 001, no se tocó).

## Progreso de implementación

- 001: **T1–T8 completas** ✅. Tests rojo→verde por tarea.
  - T1 esquema versionado (`VERSION_ESQUEMA_ACTUAL = 2`), T2 respaldo, T3 apertura con migración y
    respaldo obligatorio, T4 núcleo de datos (CRUD de dominios) y migración v2, T5 cascada atómica,
    T6 cambios pendientes y reintento, T7 instancia única y arranque del cliente, T8 documentación
    del contrato para el frontend (`docs/interfaz-frontend.md` §2) + `test_backend_contrato_interfaz`.
  - `docs/interfaz-frontend.md` documenta el contrato real (implementado vs planificado); el test de
    contrato fija `Resultado<T>`, `AperturaBaseDatos::Resultado::ok()/detalle/rutaRespaldo` y
    `estadoPendienteDe` (verde).
- Tests ajenos que fallan (no tocar): `test_servicio_profesor` (ordenamiento pendiente), `test_solver_horarios`, `test_solver_benchmark` (solver).
- Entorno: tests dentro del contenedor `gestor-dev` → `docker exec gestor-dev bash -lc "cd /workspace && cmake --preset full && cmake --build build && ctest --preset full --output-on-failure"`. OR-Tools solo en el contenedor; el host no lo tiene.
