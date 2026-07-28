# Tasks: Sprint 6 — Backend Services, Solver Soft Constraints, Frontend Admin

## Phase 1: Foundation — ServicioProfesor + ServicioDisponibilidad (Luis)

> Prerequisites for #36 (Frontend Admin) and #40 (Integration Tests)

- [ ] T-1.1 Create `ServicioProfesor.hpp` — class with CRUD (crear, obtener, listar, actualizar, eliminar), validation (nombre not empty, email format), Profesor_Materia relationship (asignarMateria, listarMateriasProfesor, eliminarMateriaProfesor), obtenerTodosParaSolver(). Follow ServicioAula pattern.
- [ ] T-1.2 Create `ServicioProfesor.cpp` — implement CRUD against Profesores table (TEXT PK = cedula), cascade delete Profesor_Materia and Disponibilidad_Profesor on delete. Return Resultado with error codes.
- [ ] T-1.3 Create `ServicioDisponibilidad.hpp` — class with CRUD (agregar, listar por profesor, actualizar, eliminar), overlap validation, obtenerParaSolver(). Overlap = same teacher + same day + time ranges intersect.
- [ ] T-1.4 Create `ServicioDisponibilidad.cpp` — implement against Disponibilidad_Profesor table. Convert dia string ("lunes") to int (1). Conversion helper: `QString diaIntToString(int)` shared utility.
- [ ] T-1.5 Add availability OP constants to `messages.h` — `OP_LISTA_DISPONIBILIDAD`, `OP_AGREGAR_DISPONIBILIDAD`, `OP_ACTUALIZAR_DISPONIBILIDAD`, `OP_ELIMINAR_DISPONIBILIDAD`.
- [ ] T-1.6 Add availability handler declarations to `internalserver.h` — handleAvailabilityList, handleAvailabilityAdd, handleAvailabilityUpdate, handleAvailabilityDelete.
- [ ] T-1.7 Replace teacher stubs in `internalserver.cpp` — use ServicioProfesor for all 5 teacher handlers. Register availability routes in `inicializarRutas()`.
- [ ] T-1.8 Implement availability handlers in `internalserver.cpp` — parse teacher_id from JSON, call ServicioDisponibilidad, return JSON response.
- [ ] T-1.9 Update `src/backend/CMakeLists.txt` — add ServicioProfesor.cpp, ServicioDisponibilidad.cpp to target_sources.
- [ ] T-1.10 Write `test/backend/test_servicio_profesor.cpp` — unit tests: create, duplicate rejection, get non-existent, email validation, obtenerTodosParaSolver, Profesor_Materia relationship.
- [ ] T-1.11 Write `test/backend/test_servicio_disponibilidad.cpp` — unit tests: add availability, overlap rejection, list by teacher, obtenerParaSolver conversion, delete cascading.
- [ ] T-1.12 Update `test/CMakeLists.txt` — add new test targets.

## Phase 2: Soft Constraints — PenalizacionConfig Expansion (Luis)

> Prerequisite for #37 (Tuning) and #41 (Performance)

- [ ] T-2.1 Expand `PenalizacionConfig` in `solver_config.hpp` — add 3 new fields: `preferencia_profesor` (int, default 10), `gaps_horarios` (int, default 20), `distribucion_dias` (int, default 15). Keep existing fields.
- [ ] T-2.2 Update `solver_config.cpp` — parse the 3 new penalty fields from JSON (with defaults if missing for backward compat).
- [ ] T-2.3 Create `restricciones_blandas.hpp` — declare `agregarRestriccionesBlandas(CpModelBuilder&, const SolverConfig&, VariablesSolver&)`.
- [ ] T-2.4 Create `restricciones_blandas.cpp` — implement 3 penalty constraints: (a) preferencia_profesor: penalize assignments where professor has no preference for that day/slot, (b) gaps_horarios: penalize gaps between assignments for same professor on same day, (c) distribucion_dias: penalize uneven day distribution across professors. Each creates BoolVars + penalties, accumulates into objective.
- [ ] T-2.5 Wire into `solver.cpp` — add `model.Add(agregarRestriccionesBlandas(...))` call, change `SolveCpModel` to use `model.Maximize(sum_penalties)` after all hard constraints.
- [ ] T-2.6 Update `src/backend/CMakeLists.txt` — add `restricciones/restricciones_blandas.cpp`.
- [ ] T-2.7 Write `test/backend/test_restricciones_blandas.cpp` — verify penalties are added to model, verify Maximize objective is set, verify backward compat (old JSON without new fields still works).

## Phase 3: Solver Tuning + Performance (Nicole)

> Depends on T-2.1–T-2.7 (soft constraints must be wired first)

- [ ] T-3.1 Add `CpSolverParameters` to `Solver::resolver()` in `solver.cpp` — set `max_time_in_seconds` from config (add `tiempo_limite_s` field to GeneracionConfig if not present), set `num_workers` for parallelism.
- [ ] T-3.2 Add `tiempo_limite_s` (double, default 60.0) to `GeneracionConfig` in `solver_config.hpp` and parse in `solver_config.cpp`.
- [ ] T-3.3 Add lookup maps to `VariablesSolver` struct — `QMap<QPair<int,int>, QVector<BoolVar>>` indexed by (profesor, dia) to replace O(n²) iteration in `restricciones_profesores.cpp`.
- [ ] T-3.4 Refactor `restricciones_profesores.cpp` — use lookup maps instead of nested loops over vars.assignment. This is the primary performance win.
- [ ] T-3.5 Write `test/backend/test_solver_benchmark_extended.cpp` — benchmark: measure solver time with and without lookup maps, with and without time limit. Verify time limit is respected.
- [ ] T-3.6 Tune CpSolverParameters iteratively — adjust `max_time_in_seconds`, `num_workers`, and CP-SAT heuristics. Document optimal values in commit message.

## Phase 4: Frontend Admin — Teacher Management (Dani)

> Depends on T-1.1–T-1.8 (services + middleware must be wired)

- [ ] T-4.1 Update `teacher_list_widget.hpp/cpp` — connect to real middleware routes (OP_LISTA_PROFESORES), display teacher data in table, add buttons for add/edit/delete.
- [ ] T-4.2 Update `teacher_form_dialog.hpp/cpp` — connect to OP_CREAR_PROFESOR and OP_ACTUALIZAR_PROFESOR, validate fields client-side before sending, show success/error feedback.
- [ ] T-4.3 Add teacher delete confirmation dialog — modal QMessageBox with confirm/cancel, call OP_ELIMINAR_PROFESOR on confirm, refresh list.
- [ ] T-4.4 Add availability sub-view — when selecting a teacher, show availability table with add/edit/delete rows. Wire to availability OP routes.
- [ ] T-4.5 Add subject assignment sub-view — multi-select or checklist for assigning subjects to teacher. Wire to Profesor_Materia middleware routes (if middleware routes exist, otherwise stub).
- [ ] T-4.6 Verify all frontend CRUD operations work end-to-end with real backend.

## Phase 5: Config Shell — Aesthetic Placeholder (Paola)

> Independent — no dependencies on other tasks

- [ ] T-5.1 Create `configuracion_widget.hpp/cpp` — QStackedWidget with sidebar navigation (General, Visual, Futuro). Purely aesthetic, no solver config.
- [ ] T-5.2 Create placeholder panels — GeneralPanel (school name, logo placeholder), VisualPanel (theme/color picker placeholder), FuturePanel (text saying "próximamente"). Each is a QWidget with labels.
- [ ] T-5.3 Register config button in `main_window.hpp/cpp` — add "Configuración" button to sidebar, connect to mostrarConfiguracion() slot, add ConfiguracionWidget to m_contenedorVistas.
- [ ] T-5.4 Style the config shell — match existing app styling (sidebar dark, content light, consistent fonts). Use QSS similar to main_window.cpp patterns.
- [ ] T-5.5 Verify config shell opens, navigates between panels, and returns to main views.

## Phase 6: Integration Tests (Manuel)

> Depends on T-1.1–T-1.8 (backend services) + T-4.1–T-4.6 (frontend wired)

- [ ] T-6.1 Write `test/test_integration_profesor_crud.cpp` — end-to-end: InternalClient sends OP_CREAR_PROFESOR → InternalServer processes → ServicioProfesor writes to SQLite → InternalClient reads response → verify data matches.
- [ ] T-6.2 Write `test/test_integration_disponibilidad_crud.cpp` — end-to-end: add availability, list, update, delete. Verify overlap rejection works through full stack.
- [ ] T-6.3 Write `test/test_integration_teacher_list.cpp` — create 3 teachers, list all, verify count and data integrity through middleware.
- [ ] T-6.4 Write `test/test_integration_solver_with_soft_constraints.cpp` — build SolverConfig with PenalizacionConfig, run solver, verify soft constraint penalties are reflected in output (non-zero penalty score).
- [ ] T-6.5 Generate JUnit XML report — configure test runner to output XML, verify report format.
- [ ] T-6.6 Verify all existing tests still pass — run full test suite, no regressions.

## Phase 7: Build + Polish

- [ ] T-7.1 Update `src/backend/CMakeLists.txt` — ensure all new sources are listed (restricciones_blandas, ServicioProfesor, ServicioDisponibilidad).
- [ ] T-7.2 Update `src/frontend/CMakeLists.txt` — add configuracion_widget.cpp and any new headers.
- [ ] T-7.3 Update `test/CMakeLists.txt` — add all new test targets.
- [ ] T-7.4 Full build verification — `cmake --build` with zero warnings, zero errors.
- [ ] T-7.5 Run full test suite — all tests pass, no regressions.

---

## Summary

| Phase | Tasks | Assignee | Depends On |
|-------|-------|----------|------------|
| 1. Foundation | 12 | Luis | — |
| 2. Soft Constraints | 7 | Luis | — |
| 3. Solver Tuning | 6 | Nicole | Phase 2 |
| 4. Frontend Admin | 6 | Dani | Phase 1 |
| 5. Config Shell | 5 | Paola | — |
| 6. Integration Tests | 6 | Manuel | Phases 1 + 4 |
| 7. Build + Polish | 5 | All | All |
| **Total** | **47** | | |

## Critical Path

```
Phase 1 (Luis) ──→ Phase 4 (Dani) ──→ Phase 6 (Manuel)
Phase 2 (Luis) ──→ Phase 3 (Nicole)
Phase 5 (Paola) ── independent
```

## Parallelization Opportunities

- Phase 1 and Phase 2 can run in parallel (Luis splits time)
- Phase 5 (Paola) runs fully independent
- Phase 3 (Nicole) can start as soon as Phase 2 completes
- Phase 4 (Dani) can start as soon as Phase 1 middleware wiring is done (T-1.8)
