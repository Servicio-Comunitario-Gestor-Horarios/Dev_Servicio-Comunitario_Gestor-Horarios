## Exploration: CP-SAT Solver Foundations (Issues #25 + #32)

### Current State

**Project**: C++17/Qt6/OR-Tools desktop app for academic schedule management at Liceo Nacional "Robert Serra" (41 staff, 933 students).

**3-layer architecture** (single binary):
- Frontend (Qt6 Widgets) → IPC (QLocalSocket) → Backend (OR-Tools + SQLite)
- Backend links `ortools::ortools` but has ZERO solver code — just empty `core/` and `constraints/` directories under `src/backend/src/solver/`

**Existing data structs** (all in `src/backend/include/backend/data/`):
- `Aula`, `Materia`, `Profesor`, `FranjaHoraria`, `PlanEstudio`/`MateriaEnPlan`, `Horario`/`Asignacion`
- All have `toJson()`/`fromJson()` round-trip serialization

**Existing services** (CRUD + solver extraction):
- `ServicioAula` → has `obtenerTodasParaSolver()`
- `ServicioMaterias` → has `obtenerTodasParaSolver()`
- `ServicioComposicionPlan` → has `componerPlanCompleto()`
- `ServicioCargaHoraria` → has query methods for hours
- **NO ServicioProfesor** — DB tables exist but no service assembles Profesor structs

**Config** (`config/Solver.json`): 40 max weekly hours, morning shift 07:00-12:30, afternoon 12:35-18:10, recess breaks defined.

**Sprint 5 design** (`docs/sprint5-contexto-tecnico.md`): Detailed spec for Issue #32 exists — IConstraint, HorasSemanalesConstraint, RestriccionManager, SolverEngine, DataContext, SolverVariables.

### Affected Areas

- `src/backend/src/solver/core/` — empty, needs SolverEngine + DataContext
- `src/backend/src/solver/constraints/` — empty, needs IConstraint + implementations
- `src/backend/include/backend/solver/` — empty, needs all solver headers
- `src/backend/CMakeLists.txt` — needs solver files added to target_sources
- `test/CMakeLists.txt` — needs solver test registration
- **MISSING**: `ServicioProfesor` (must be built before solver works end-to-end)

### Approaches

1. **Follow sprint5 design exactly** (from docs/sprint5-contexto-tecnico.md)
   - Pros: Already approved design, matches project conventions, detailed class specs
   - Cons: Design is from prior session, may need minor updates
   - Effort: Medium (7 files to create, 2 to modify)

2. **Simplified solver first** (MVP with just room + shift constraints, defer IConstraint abstraction)
   - Pros: Faster to working prototype, validate CP-SAT integration early
   - Cons: Refactor later to add IConstraint pattern, technical debt
   - Effort: Low-Medium

3. **Full constraint system from start** (IConstraint + RestriccionManager + all constraints)
   - Pros: Extensible from day one, matches design
   - Cons: More upfront work, harder to validate incrementally
   - Effort: High

### Recommendation

**Option 1: Follow sprint5 design exactly.** The design in `docs/sprint5-contexto-tecnico.md` is already approved and detailed. The IConstraint abstraction is essential for the constraint system's extensibility (adding soft constraints post-MVP). Build ServicioProfesor as a prerequisite task.

### Risks

- **ServicioProfesor gap**: Without it, solver can't load teacher data from DB. Must be built first or alongside solver.
- **CP-SAT complexity**: Highest risk item in sprint5 matrix. Mitigate by starting with DataContext + SolverVariables definitions early.
- **FranjaHoraria generation**: Time blocks must be computed from Solver.json config — this generator doesn't exist yet.
- **No solver tests**: Must decide between QTest (project convention) and GTest (scaffolded in CMakeLists.txt).

### Ready for Proposal

Yes — exploration is complete. The sprint5 document provides a detailed design ready for proposal/spec phases.
