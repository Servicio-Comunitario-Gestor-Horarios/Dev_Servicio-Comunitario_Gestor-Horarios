## Exploration: Nicole's Module — Solver Implementation (Issues #32, #28, #35)

### Current State

**Solver code is 100% empty.** All three solver directories have zero files:
- `src/backend/include/backend/solver/` — 0 headers
- `src/backend/src/solver/core/` — 0 files
- `src/backend/src/solver/constraints/` — 0 files

`ortools::ortools` is already linked in `CMakeLists.txt` as a PUBLIC dependency of the `backend` static library.

**Issue #28 (ServicioCargaHoraria)** is already completed (Sprint 4). The service exists at `src/backend/include/backend/services/ServicioCargaHoraria.hpp` with CRUD + solver query methods.

**Issue #32 (Restricciones CP-SAT)** is the critical-path predecessor. Full design exists in `docs/planificacion/sprint5-contexto-tecnico.md` (Section 4, lines 139-265). This is assigned to Luis.

**Issue #35 (Exportar horarios manuales)** is Nicole's task. Phase 3 in the execution plan — depends on #34 (Export solver), which depends on #32.

### Affected Areas

- `src/backend/include/backend/solver/` — needs all solver headers (IConstraint, HorasSemanalesConstraint, RestriccionManager, SolverEngine)
- `src/backend/src/solver/core/` — needs SolverEngine.cpp, DataContext definition
- `src/backend/src/solver/constraints/` — needs IConstraint interface, HorasSemanalesConstraint implementation
- `src/backend/CMakeLists.txt` — needs solver .cpp files added to `target_sources`
- `src/backend/include/backend/data/profesor.hpp` — struct needs extension for solver fields
- `src/backend/include/backend/data/horario.hpp` — output struct, already exists with Asignacion/Horario
- `src/backend/include/backend/resultado.hpp` — Resultado<T> template, already exists

### Interfaces/Classes Nicole Needs (from sprint5 design)

**IConstraint** — abstract interface for all constraints:
```cpp
class IConstraint {
public:
    virtual ~IConstraint() = default;
    virtual void addConstraint(const DataContext& data,
                               ortools::sat::CpModelBuilder& model,
                               const SolverVariables& vars) = 0;
};
```

**HorasSemanalesConstraint** — ensures professor doesn't exceed weekly hours:
```cpp
class HorasSemanalesConstraint : public IConstraint {
public:
    explicit HorasSemanalesConstraint(int maxHorasSemanales = 40);
    void addConstraint(const DataContext& data,
                       ortools::sat::CpModelBuilder& model,
                       const SolverVariables& vars) override;
private:
    int m_maxHoras;
};
```

**RestriccionManager** — factory + aggregator:
```cpp
class RestriccionManager {
public:
    void addConstraint(std::unique_ptr<IConstraint> constraint);
    void applyAll(const DataContext& data,
                  ortools::sat::CpModelBuilder& model,
                  const SolverVariables& vars);
private:
    std::vector<std::unique_ptr<IConstraint>> m_constraints;
};
```

**SolverEngine** — top-level orchestrator:
```cpp
class SolverEngine {
public:
    explicit SolverEngine(std::chrono::seconds timeout = 30);
    Resultado<Horario> solve(const DataContext& data);
private:
    RestriccionManager m_constraints;
    std::chrono::seconds m_timeout;
};
```

**DataContext** — aggregated data for solver:
```cpp
struct DataContext {
    QVector<Aula> aulas;
    QVector<Materia> materias;
    QVector<Profesor> profesores;
    QVector<FranjaHoraria> franjasDisponibles;
    QVector<MateriaEnPlan> planEstudio;
};
```

**SolverVariables** — CP-SAT model variables:
```cpp
struct SolverVariables {
    std::map<std::tuple<int,int,int>, ortools::sat::BoolVar> assignment;
    std::map<int, ortools::sat::IntVar> totalHours;
};
```

### Dependencies on Existing Data Structs

| Struct | Header | Key Fields | Solver Relevance |
|--------|--------|------------|------------------|
| `Aula` | `data/aula.hpp` | nombre, capacidad, locacion | Room assignment, capacity constraint |
| `Materia` | `data/materia.hpp` | nombre, horas_semanales, requerimientos | Subject pool, hours per course |
| `Profesor` | `data/profesor.hpp` | nombre, disponibilidad, materias | Teacher assignment, availability constraint |
| `FranjaHoraria` | `data/franja_horaria.hpp` | dia, inicio, fin | Time slot definitions |
| `MateriaEnPlan` | `data/plan_estudio.hpp` | curso, horas, materia | Course-subject mapping |
| `Horario/Asignacion` | `data/horario.hpp` | aula, franja_horaria, materia, profesor | Solver OUTPUT |
| `Resultado<T>` | `resultado.hpp` | ok, valor, mensajeError | Error handling pattern |

### Critical Gap: Profesor Struct Mismatch

The sprint5 design defines a richer `Profesor` for the solver:
```cpp
// Design spec (sprint5):
struct ProfesorSolver {
    QString nombre;
    QVector<FranjaHoraria> disponibilidad;
    QVector<QString> materias;        // assigned subjects
    QVector<MateriasSuplente> suplente; // substitute subjects with weights
    int horas_requeridas;  // total weekly hours required
    int horas_aula;        // classroom hours (difference = planning)
};
```

**Actual struct** (`profesor.hpp`) only has: `nombre`, `disponibilidad`, `materias`. Missing: `horas_requeridas`, `horas_aula`, substitute subjects. This means the Profesor struct needs extension or a solver-specific adapter.

### CRITICAL GAP: No ServicioProfesor

Confirmed by grep: `ServicioProfesor` does not exist anywhere. The DB schema has:
- `Profesores` table (id TEXT, nombre, email, telefono)
- `Profesor_Materia` table (N:M join)
- `Disponibilidad_Profesor` table (dia, hora_inicio, hora_fin)

But no service assembles these into `Profesor` structs. This blocks building `DataContext` from the database.

### Execution Order (from sprint5)

```
Phase 1 (Parallel): #32 (Luis) + #31 (Manuel) + #27 (Paola)
Phase 2: #34 (Luis, after #32) + #29 (Dani, after #31)
Phase 3: #35 (Nicole, after #34) ← Nicole's task
```

**Critical path**: #32 → #34 → #35. If #32 slips, everything downstream slips.

### Recommendation

Nicole's work on Issue #35 (ServicioExportHorarios) is straightforward — it's a thin wrapper around SolucionExporter (#34) + DB access. The real risk is #32 and #34 being on time. The design doc is complete and approved. The Profesor struct gap and ServicioProfesor absence must be resolved before the solver can work end-to-end, but those are Luis's problems (Issue #32), not Nicole's.

### Risks

- **#32 is highest-risk item** — CP-SAT complexity, blocks entire chain
- **Profesor struct mismatch** — needs extension for solver fields (horas_requeridas, horas_aula, suplente)
- **No ServicioProfesor** — DataContext can't be populated from DB without it
- **GitHub issues #32 and #28 not accessible** — `gh auth login` needed for issue details
- **config/Solver.json is minimal** — only has shifts and recess; the full schema from Motor-Solver-Plan-Completo.md is much richer

### Ready for Proposal

Yes — exploration complete. The sprint5 document provides a detailed, approved design. The key finding is that Issue #32 has all the design work done but zero implementation, and Nicole's Issue #35 is downstream with low risk (simple wrapper).
