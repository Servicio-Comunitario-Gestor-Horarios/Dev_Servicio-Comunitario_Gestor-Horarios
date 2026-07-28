# Exploration: PR Review Skill + Sprint 5 Issue Context

## 1. PR Review Skill Assessment

### Current State

The project has a 706-line `pr-review` skill at `.agents/skills/pr-review/SKILL.md`. It covers:
- Build & CMake (5 items), C++ Code (7 items), Architecture (5 items), Tests (6 items), Security (4 items), Style (4 items), GitHub (4 items)
- Common errors from PR #62 (8 items with examples)
- Correction instructions with same-branch rule
- Test strategy (QTest vs GTest decision table)
- Full project context (architecture, stack, conventions, build commands)

### Comparison with Generic Skills

| Aspect | `pr-review` (project) | `code-review-checklist` (generic) |
|--------|----------------------|-----------------------------------|
| Scope | C++17/Qt6/CMake specific | Language-agnostic, web-focused |
| Lines | 706 | 109 |
| Checklist items | 35+ items with severity | 20 items, no severity |
| Project context | Full architecture, DB schema, conventions | None |
| Fix examples | Concrete C++/CMake fixes | TypeScript anti-patterns |
| Error patterns | 8 common errors with frequency data | None |
| Severity levels | CRITICAL/HIGH/MEDIUM/LOW | Blocking/Suggestion/Nit |
| Correction workflow | Full decision tree + same-branch rule | Comment templates only |
| Test guidance | QTest vs GTest decision table, helper signatures | Generic "unit tests for new code" |

### Strengths

1. **Deeply project-specific** — every checklist item maps to a real pattern in this codebase
2. **Actionable severity levels** — CRITICAL vs HIGH vs MEDIUM vs LOW with clear "que buscar"
3. **Concrete fix examples** — triple add_library, N+1 queries, missing transactions all have before/after code
4. **Common errors section** — frequency data from PR #62 review, actionable fixes
5. **Test strategy** — QTest vs GTest decision table with helper signatures and usage examples
6. **Correction workflow** — same-branch rule, decision tree, comment templates in Spanish
7. **Architecture awareness** — 4-layer modular monolith, dependency flow, DTO pattern

### Weaknesses & Suggestions

| # | Issue | Severity | Suggestion |
|---|-------|----------|------------|
| 1 | **Missing `tr()` i18n check** | HIGH | Frontend items don't mention `tr()` for translatable strings. Add to Style or Frontend-specific section |
| 2 | **Missing `backend_placeholder.cpp` awareness** | MEDIUM | `src/backend/src/data/backend_placeholder.cpp` exists but isn't referenced in CMakeLists — potential orphaned file issue to flag |
| 3 | **No IPC-specific checklist** | HIGH | Middleware IPC review (messages.h registration, request/response format, QLocalSocket patterns) is mentioned in passing but has no dedicated checklist items |
| 4 | **Missing `set_project_warnings()` check** | HIGH | All targets MUST call `set_project_warnings(target)`. Not in the checklist. Add as B6 |
| 5 | **No `QSqlTransaction` rollback on error** | MEDIUM | The transaction example shows commit but not rollback-on-error path. The codebase uses `m_db.rollback()` — add this pattern |
| 6 | **Missing `PR template` checklist item detail** | LOW | G1 mentions PR template but doesn't list the actual checklist items from `.github/PULL_REQUEST_TEMPLATE.md` |
| 7 | **No mention of `FRONTEND_STANDALONE` build check** | MEDIUM | Frontend-only changes should verify `BUILD_BACKEND=OFF` still compiles. Not in checklist |
| 8 | **Missing `AUTOUIC`/`AUTORCC` check** | MEDIUM | B4 checks AUTOMOC but not AUTOUIC or AUTORCC. Frontend uses all three |
| 9 | **No `Q_INVOKABLE` review guidance** | LOW | InternalClient uses `Q_INVOKABLE` — if a review touches IPC, this should be checked |
| 10 | **Missing `inline const QString` ODR guidance** | MEDIUM | messages.h uses `inline const QString` — C2 mentions ODR but doesn't show this specific pattern as correct |
| 11 | **No Docker-specific build check** | LOW | The skill mentions Docker but doesn't remind to verify build inside container |
| 12 | **Missing `MateriaEnPlan` DTO check** | LOW | The DTO↔solver mapping pattern (`AulaDTO::toAula()`, `MateriaDTO::toMateria()`) should be explicitly reviewed |

### Recommended Improvements

1. Add a "Frontend-Specific" subsection to the checklist (currently only backend-specific items exist)
2. Add `set_project_warnings()` as a mandatory CMake check item
3. Add IPC-specific checklist items (messages.h registration, request/response format consistency)
4. Add `FRONTEND_STANDALONE` build verification for frontend changes
5. Update the transaction example to include rollback-on-error path
6. Add the PR template checklist items inline for quick reference

---

## 2. Per-Issue Technical Context for Sprint 5

### Issue #27: [Frontend] Formularios aula + materia + tabla datos básica

**Depends on**: #13 (Sprint 2 — existing TeacherListWidget/TeacherFormDialog pattern)

**Files to create**:
- `src/frontend/src/forms/aula_form_dialog.hpp` — QDialog subclass for aula creation/edit
- `src/frontend/src/forms/aula_form_dialog.cpp` — Implementation with fields: nombre, capacidad, edificio, piso
- `src/frontend/src/forms/materia_form_dialog.hpp` — QDialog subclass for materia creation/edit
- `src/frontend/src/forms/materia_form_dialog.cpp` — Implementation with fields: nombre, requisitos
- `src/frontend/src/views/aula_list_widget.hpp` — QWidget with QTableWidget + register button (follows TeacherListWidget pattern)
- `src/frontend/src/views/aula_list_widget.cpp` — Table setup, button connection to form dialog
- `src/frontend/src/views/materia_list_widget.hpp` — QWidget with QTableWidget + register button
- `src/frontend/src/views/materia_list_widget.cpp` — Table setup, button connection to form dialog

**Files to modify**:
- `src/frontend/CMakeLists.txt` — Add new source files to `FRONTEND_SOURCES` and `FRONTEND_HEADERS`
- `src/frontend/src/views/main_window.cpp` — Replace ViewPlaceholder(2) and ViewPlaceholder(3) with AulaListWidget and MateriaListWidget

**Implementation approach**:
Follow the exact pattern from `TeacherListWidget` + `TeacherFormDialog`:
- Dialog uses `QFormLayout` with `QLineEdit` fields and validators
- List widget uses `QTableWidget` with columns matching DB schema
- Signal `profesorGuardado(...)` pattern → adapt to `aulaGuardada(...)` / `materiaGuardada(...)`
- No IPC yet (depends on #29 for middleware connection) — local-only for now

**Why**: Reuses proven pattern, low risk, consistent UI across all entity forms

**Project context needed**:
- `TeacherListWidget` and `TeacherFormDialog` as reference patterns
- `AulaDTO` fields (nombre, capacidad, edificio, piso) from `ServicioAula.hpp`
- `MateriaDTO` fields (nombre, requisitos) from `ServicioMaterias.hpp`
- `MainWindow` stacked widget indices (2 = aulas, 3 = asignaturas)

---

### Issue #29: [Frontend] Refinamiento formularios + conexión a middleware

**Depends on**: #18 (Sprint 3 — IPC server/client), #22 (Sprint 3 — teacher CRUD via IPC)

**Files to create**:
- `src/frontend/src/views/aula_list_widget_mw.hpp` — Middleware-connected version (or modify existing)
- `src/frontend/src/views/aula_list_widget_mw.cpp` — Uses InternalClient to send CRUD operations

**Files to modify**:
- `src/frontend/src/views/teacher_list_widget.cpp` — Connect to InternalClient for real CRUD (currently placeholder)
- `src/frontend/src/views/teacher_form_dialog.cpp` — Send save via IPC instead of local signal
- `src/frontend/src/views/aula_list_widget.cpp` (created in #27) — Add IPC connection
- `src/frontend/src/views/materia_list_widget.cpp` (created in #27) — Add IPC connection
- `src/middleware/include/middleware/messages.h` — Add OP constants for aula/materia CRUD
- `src/middleware/src/server/internalserver.cpp` — Add handler methods for aula/materia operations
- `src/middleware/src/server/internalserver.h` — Declare new handler methods

**Implementation approach**:
1. Add IPC operation constants to `messages.h`:
   ```cpp
   inline const QString OP_LISTA_AULAS = QStringLiteral("aula_list");
   inline const QString OP_CREAR_AULA = QStringLiteral("aula_create");
   // ... etc
   ```
2. In frontend widgets, instantiate `InternalClient` and connect `respuestaRecibida` signal
3. On button click → serialize payload to JSON → `cliente.enviarSolicitud(op, payload)`
4. On response received → parse JSON → update table
5. In middleware server, add handler stubs that return placeholder data (actual backend connection deferred)

**Why**: IPC pattern is established in teacher CRUD (#22), extending to aulas/materias follows same flow

**Project context needed**:
- `InternalClient` API: `enviarSolicitud(op, payload)`, `respuestaRecibida(QJsonObject)` signal
- `InternalServer` handler pattern: `handleTeacherList()`, `handleTeacherCreate()`, etc.
- `messages.h` constants pattern
- TeacherListWidget IPC connection (from #22) as reference

---

### Issue #31: [Middleware/QA] Validador de datos + tests de restricciones automatizados

**Depends on**: #26 (Sprint 4 — unit tests for solver model)

**Files to create**:
- `src/middleware/include/middleware/validation/DataValidator.hpp` — Validation class for business rules
- `src/middleware/src/validation/DataValidator.cpp` — Implementation of validation logic
- `test/middleware/test_data_validator.cpp` — QTest for DataValidator
- `test/middleware/test_solver_restrictions.cpp` — GTest for solver constraint validation (if using GTest for pure logic)

**Files to modify**:
- `src/middleware/CMakeLists.txt` — Add `src/validation/DataValidator.cpp` to target_sources
- `test/CMakeLists.txt` — Register new test files with `add_qtest()` or `add_gtest()`

**Implementation approach**:
```cpp
class DataValidator {
public:
    // Business rule validations
    Resultado<bool> validarHorasSemanales(int horas, int curso);
    Resultado<bool> validarDisponibilidad(const QVector<FranjaHoraria>& franjas);
    Resultado<bool> validarCargaHoraria(const QVector<MateriaEnPlan>& materias, int maxHoras);
    Resultado<bool> validarRestriccionesSolver(const Horario& horario);
};
```
- Use `Resultado<bool>` for validation results (consistent with service pattern)
- Validation rules: max hours per year (40), no overlapping time slots, professor availability check
- Tests: happy path + boundary values (0 hours, 41 hours, overlapping slots, empty availability)

**Why**: Centralizes validation logic in middleware (between frontend and backend), prevents invalid data from reaching solver

**Project context needed**:
- `Resultado<T>` pattern from `resultado.hpp`
- `FranjaHoraria` struct (dia, inicio, fin)
- `MateriaEnPlan` struct (curso, horas, materia)
- `MAX_HORAS_SEMANALES_POR_ANIO = 40` from `ServicioCargaHoraria.hpp`
- Test patterns from existing tests (QTemporaryDir, DatabaseManager)

---

### Issue #32: [Backend] Restricciones: horas semanales + integrar restricciones + validación simulada

**Depends on**: #25 (Sprint 4 — CP-SAT solver model)

**Files to create**:
- `src/backend/include/backend/solver/constraints/HorasSemanalesConstraint.hpp` — Constraint definition
- `src/backend/src/solver/constraints/HorasSemanalesConstraint.cpp` — CP-SAT constraint implementation
- `src/backend/include/backend/solver/constraints/RestriccionManager.hpp` — Manages all constraints
- `src/backend/src/solver/constraints/RestriccionManager.cpp` — Constraint registration and application
- `src/backend/include/backend/solver/core/SolverEngine.hpp` — Main solver entry point
- `src/backend/src/solver/core/SolverEngine.cpp` — Orchestrates model building, constraints, solving
- `test/backend/test_solver_constraints.cpp` — GTest for constraint logic

**Files to modify**:
- `src/backend/CMakeLists.txt` — Add new solver source files to `target_sources(backend PRIVATE ...)`
- `test/CMakeLists.txt` — Register solver tests with `add_gtest()`

**Implementation approach**:
```cpp
class SolverEngine {
public:
    Resultado<Horario> resolver(const QVector<Aula>& aulas,
                                const QVector<Profesor>& profesores,
                                const PlanEstudio& plan);
private:
    CpModelBuilder m_model;
    RestriccionManager m_restricciones;
    void agregarVariables();
    void agregarRestricciones();
    Horario extraerSolucion(const CpSolver& solver);
};
```
- OR-Tools CP-SAT: binary variables `x[profesor, materia, aula, franja]` (0/1)
- Horas semanales constraint: `sum(x[p,m,a,f]) <= horas_semanales` per professor
- Integrar with `ServicioComposicionPlan` to get plan data, `ServicioAula` for rooms, `ServicioMaterias` for subjects
- Simulated validation: run solver with timeout, return partial solution if no optimal found

**Why**: Core optimization logic — constraints make the solver produce realistic schedules

**Project context needed**:
- OR-Tools CP-SAT API (CpModelBuilder, CpSolver)
- Data structs: Aula, Profesor, Materia, FranjaHoraria, Horario, PlanEstudio
- `ServicioComposicionPlan::componerPlanCompleto()` to get plan data
- `ServicioAula::obtenerTodasParaSolver()` to get rooms
- `ServicioMaterias::obtenerTodasParaSolver()` to get subjects
- `ServicioCargaHoraria::totalHorasPorAnio()` for hour limits

---

### Issue #34: [Backend] Exportar solución solver a JSON/CSV

**Depends on**: #32 (Sprint 5 — solver solution)

**Files to create**:
- `src/backend/include/backend/solver/export/SolucionExporter.hpp` — Export interface
- `src/backend/src/solver/export/SolucionExporter.cpp` — JSON/CSV export implementation
- `test/backend/test_solucion_exporter.cpp` — QTest for export (toJson roundtrip + CSV format)

**Files to modify**:
- `src/backend/CMakeLists.txt` — Add export source files
- `test/CMakeLists.txt` — Register export test

**Implementation approach**:
```cpp
class SolucionExporter {
public:
    // JSON export (uses existing Horario::toJson())
    Resultado<QString> exportarJSON(const Horario& horario);
    Resultado<bool> guardarJSON(const Horario& horario, const QString& ruta);
    
    // CSV export
    Resultado<QString> exportarCSV(const Horario& horario);
    Resultado<bool> guardarCSV(const Horario& horario, const QString& ruta);
};
```
- JSON: leverage existing `Horario::toJson()` and `Asignacion::toJson()` methods
- CSV: columns = Aula, Franja, Materia, Profesor, Dia, HoraInicio, HoraFin
- Use `QFile` + `QTextStream` for file writing
- Use `QFileDialog` integration (frontend concern, not this service)

**Why**: Users need to export optimized schedules for printing/sharing. JSON for system integration, CSV for spreadsheets.

**Project context needed**:
- `Horario::toJson()` and `Asignacion::toJson()` methods (already implemented in `horario.cpp`)
- `Aula`, `Materia`, `Profesor`, `FranjaHoraria` structs with their field names
- `Resultado<T>` pattern for error handling
- `QFile`, `QTextStream`, `QJsonDocument` for file I/O

**INTER-SPRINT DEPENDENCY**: #34 depends on #32 (solver solution must exist before exporting it)

---

### Issue #35: [Backend] Exportación horarios manuales a JSON/CSV

**Depends on**: #28 (Sprint 4 — CRUD horarios)

**Files to create**:
- `src/backend/include/backend/services/ServicioExportHorarios.hpp` — Export service for manual schedules
- `src/backend/src/services/ServicioExportHorarios.cpp` — Implementation
- `test/backend/test_servicio_export_horarios.cpp` — QTest for export service

**Files to modify**:
- `src/backend/CMakeLists.txt` — Add new service source
- `test/CMakeLists.txt` — Register test

**Implementation approach**:
```cpp
class ServicioExportHorarios {
public:
    explicit ServicioExportHorarios(QSqlDatabase& db);
    
    Resultado<QString> exportarJSON(const QString& codigoPlan);
    Resultado<QString> exportarCSV(const QString& codigoPlan);
    Resultado<bool> guardarJSON(const QString& codigoPlan, const QString& ruta);
    Resultado<bool> guardarCSV(const QString& codigoPlan, const QString& ruta);
};
```
- Reads manual schedule data from DB (tables created in #28)
- Converts DB records to `Horario` struct using existing `fromJson`/serialization
- Reuses `SolucionExporter` from #34 for actual file writing (DRY)
- Differs from #34: this exports *manually created* schedules, #34 exports *solver-optimized* schedules

**Why**: Users who create schedules manually (without solver) also need export capability

**Project context needed**:
- `ServicioCargaHoraria` patterns for DB queries
- DB schema for manual schedules (created in #28)
- `Horario`/`Asignacion` structs for data representation
- `SolucionExporter` from #34 for file writing (reuse)

---

## 3. Dependency Graph

### Within-Sprint 5 Dependencies

```
Sprint 5 Issues:
─────────────────

#27 [Frontend] Formularios aula + materia
  └── depends on #13 (Sprint 2) ✓ NOT IN SPRINT 5

#29 [Frontend] Refinamiento formularios + conexión middleware
  ├── depends on #18 (Sprint 3) ✓ NOT IN SPRINT 5
  └── depends on #22 (Sprint 3) ✓ NOT IN SPRINT 5

#31 [Middleware/QA] Validador de datos + tests
  └── depends on #26 (Sprint 4) ✓ NOT IN SPRINT 5

#32 [Backend] Restricciones solver
  └── depends on #25 (Sprint 4) ✓ NOT IN SPRINT 5

#34 [Backend] Exportar solución solver
  └── depends on #32 (Sprint 5) ⚠️ WITHIN-SPRINT DEPENDENCY

#35 [Backend] Exportación horarios manuales
  └── depends on #28 (Sprint 4) ✓ NOT IN SPRINT 5
```

### Critical Finding: #34 → #32

**#34 (Export solver solution) depends on #32 (Solver constraints)**

This is the ONLY within-sprint dependency. This means:
- **#32 MUST be completed before #34 can start**
- #32 is on the critical path for #34
- If #32 is delayed, #34 is automatically delayed

### Recommended Execution Order

```
Phase 1 (parallel, no cross-dependencies):
├── #27 [Frontend] Formularios aula + materia
├── #29 [Frontend] Refinamiento formularios + middleware
├── #31 [Middleware/QA] Validador de datos + tests
└── #32 [Backend] Restricciones solver

Phase 2 (after #32 completes):
└── #34 [Backend] Exportar solución solver

Phase 3 (independent, can run in parallel with Phase 2):
└── #35 [Backend] Exportación horarios manuales
```

### Risk: #34 is Blocked by #32

If #32 takes longer than expected, #34 has no workaround. Consider:
- Starting #34 with a mock `Horario` object (test the export logic independently)
- Completing #32 first, then #34 sequentially

---

## 4. Risk Assessment

### Technical Risks

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| OR-Tools CP-SAT solver complexity (#32) | HIGH | MEDIUM | Start with simple constraints, iterate. Use OR-Tools examples as reference |
| IPC message format inconsistency (#29) | MEDIUM | LOW | Follow existing `messages.h` pattern strictly. Review PR #22 patterns |
| Missing `backend_placeholder.cpp` in CMake | LOW | HIGH | Check if file is referenced. If orphaned, remove or add to target_sources |
| Frontend form validation not matching backend rules (#27) | MEDIUM | MEDIUM | Use same validation constants as services (CAPACIDAD_MINIMA, etc.) |
| Solver export format mismatch (#34) | LOW | LOW | Reuse existing `toJson()`/`fromJson()` methods — formats are already defined |

### Dependency Risks

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| #32 delayed → #34 blocked | HIGH | MEDIUM | Start #32 early, parallelize with other Phase 1 issues |
| #28 (Sprint 4) not complete → #35 blocked | HIGH | LOW | Verify #28 status before starting #35 |
| #25 (Sprint 4) not complete → #32 blocked | HIGH | LOW | Verify #25 status before starting #32 |

### Build Risks

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| Missing `set_project_warnings()` on new targets | MEDIUM | MEDIUM | Add to every new CMakeLists.txt section |
| New files not added to CMakeLists.txt | HIGH | MEDIUM | Use `target_sources()` consistently, check with `find src -name "*.cpp"` |
| OR-Tools linking issues | HIGH | LOW | Use `ortools::ortools` target, not manual paths |

---

## 5. Ready for Proposal

**Yes** — all context gathered. The orchestrator should:

1. For the **PR Review Skill**: Proceed to update the skill with the 12 improvement items identified
2. For **Sprint 5 Issue Context**: Use the per-issue technical context above to fill in the "Contexto Técnico" column in the GitHub issues
3. For **Dependency Graph**: Highlight the #34 → #32 within-sprint dependency as the critical path
4. For **Risk Assessment**: Prioritize #32 (solver constraints) as the highest-risk item to start first
