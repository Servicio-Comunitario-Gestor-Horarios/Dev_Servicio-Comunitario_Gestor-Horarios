# Design: schedule-crud-queries

## Technical Approach

Two new Qt6 backend services operating on output JSON files (`output/*.json`) with integer-index data model. No SQLite — pure JSON file I/O via `QJsonDocument`. Follows existing patterns: `Resultado<T>` for error handling, struct-based data models with `toJson()`/`fromJson()`, QTest with `QTemporaryDir` for tests.

## Architecture Decisions

### Decision: Separate output structs from input structs

**Choice**: New `AsignacionOutput`, `DiaOutput`, `CursoOutput`, `MetadataSalida`, `HorarioSalida` structs — distinct from existing `Asignacion`/`Horario`.
**Alternatives**: Reuse existing structs with optional fields; add a parallel namespace.
**Rationale**: Output JSON uses integer indices (`slot`, `materia`, `profesor`, `aula` as ints), while input structs hold full entity objects (`Aula`, `Profesor`, etc.). Mixing them creates confusion. Single source of truth for the JSON schema.

### Decision: ServicioHorarioSalida takes directory path, not file

**Choice**: Constructor takes `QString directorio` and each method takes a filename parameter.
**Alternatives**: Constructor takes a single file path; file path per-method only.
**Rationale**: Multiple output files may exist (one per solver run). Directory + filename allows `listarArchivos()` and flexible load/save.

### Decision: ServicioConsultaHorario is pure in-memory query

**Choice**: Constructor takes `const HorarioSalida&` reference. No file I/O.
**Alternatives**: Constructor takes a path and loads internally.
**Rationale**: Separation of concerns — CRUD owns the lifecycle, queries are stateless analyzers. Enables testing queries independently.

### Decision: Delete entire solver directory (not just 4 files)

**Choice**: Delete `DataContext.hpp`, `SolverVariables.hpp`, `HorasSemanalesConstraint.hpp/.cpp` AND `IConstraint.hpp`, `RestriccionManager.hpp/.cpp`.
**Alternatives**: Keep IConstraint/RestriccionManager, only delete HorasSemanalesConstraint.
**Rationale**: `IConstraint.hpp` includes both `DataContext.hpp` and `SolverVariables.hpp` — deleting those two makes IConstraint non-compilable. RestriccionManager depends on IConstraint. All are premature (no real solver implementation exists). Clean slate for Issue #25.

### Decision: Conflict detection uses simple O(n²) scan

**Choice**: Flat iteration over all assignments, comparing `(slot, dia)` pairs.
**Alternatives**: Index by `(slot, dia)` in a map for O(1) lookup.
**Rationale**: Output files are small (one solver run). O(n²) is simpler and sufficient. Optimization deferred if needed.

## Data Flow

```
ServicioHorarioSalida          ServicioConsultaHorario
  cargar(json_file)  ──→        (const HorarioSalida&)
  guardar(horario)   ──→        buscarPorProfesor(idx)
  agregar/eliminar/   ──→        buscarPorAula(idx)
  modificarAsignacion             buscarPorDia(dia)
                                  detectarConflictos()
                                  obtenerEstadisticas()
```

## File Changes

| File | Action | Description |
|------|--------|-------------|
| `src/backend/include/backend/data/output_schedule.hpp` | Create | Output data structs: `AsignacionOutput`, `DiaOutput`, `CursoOutput`, `MetadataSalida`, `HorarioSalida`, `EstadisticasHorario` + `toJson()`/`fromJson()` |
| `src/backend/src/data/output_schedule.cpp` | Create | Serialization implementation |
| `src/backend/include/backend/services/ServicioHorarioSalida.hpp` | Create | CRUD service: cargar, guardar, agregar/modificar/eliminar, listar |
| `src/backend/src/services/ServicioHorarioSalida.cpp` | Create | Implementation |
| `src/backend/include/backend/services/ServicioConsultaHorario.hpp` | Create | Query service: buscar, detectarConflictos, obtenerEstadisticas |
| `src/backend/src/services/ServicioConsultaHorario.cpp` | Create | Implementation |
| `src/backend/CMakeLists.txt` | Modify | Add 2 new data + 2 new service source files; remove 2 solver files |
| `test/CMakeLists.txt` | Modify | Add 3 new test targets |
| `test/backend/test_output_schedule.cpp` | Create | Data struct round-trip tests |
| `test/backend/test_servicio_horario_salida.cpp` | Create | CRUD service tests with QTemporaryDir |
| `test/backend/test_servicio_consulta_horario.cpp` | Create | Query + conflict detection tests |
| `src/backend/include/backend/solver/DataContext.hpp` | Delete | Premature, referenced by deleted IConstraint |
| `src/backend/include/backend/solver/SolverVariables.hpp` | Delete | Premature, referenced by deleted IConstraint |
| `src/backend/include/backend/solver/IConstraint.hpp` | Delete | Depends on deleted DataContext/SolverVariables |
| `src/backend/include/backend/solver/RestriccionManager.hpp` | Delete | Depends on deleted IConstraint |
| `src/backend/src/solver/constraints/RestriccionManager.cpp` | Delete | Implementation of deleted class |
| `src/backend/src/solver/constraints/HorasSemanalesConstraint.cpp` | Delete | Premature constraint |

## Interfaces / Contracts

```cpp
// output_schedule.hpp — key structs
struct AsignacionOutput {
    int slot, materia, profesor, aula;
    QJsonObject toJson() const;
    static AsignacionOutput fromJson(const QJsonObject& obj);
};

struct DiaOutput { int dia; QVector<AsignacionOutput> asignaciones; };

struct CursoOutput {
    QString turno;
    QVector<DiaOutput> dias;
    QJsonObject toJson() const;
    static CursoOutput fromJson(const QJsonObject& obj);
};

struct MetadataSalida {
    QString fechaGeneracion, configuracion;
    int tiempoEjecucionMs, totalAsignaciones, cursosGenerados,
        profesoresAsignados, conflictos;
};

struct HorarioSalida {
    MetadataSalida metadata;
    QMap<QString, CursoOutput> horarios;
    QJsonObject toJson() const;
    static Resultado<HorarioSalida> fromJson(const QJsonObject& obj);
};

struct EstadisticasHorario {
    int totalAsignaciones;
    QMap<int, int> porProfesor;    // idx → count
    QMap<int, int> porAula;        // idx → count
    QMap<QString, int> porCurso;   // name → count
    QMap<int, int> porDia;         // dia → count
};
```

## Testing Strategy

| Layer | What | Approach |
|-------|------|----------|
| Unit (output_schedule) | JSON round-trip, fromJson error cases | QTest + QJsonObject literals |
| Unit (ServicioHorarioSalida) | CRUD on temp files, conflict check, error paths | QTest + QTemporaryDir + sample JSON |
| Unit (ServicioConsultaHorario) | Query by profesor/aula/dia, conflict detection, stats | QTest with in-memory HorarioSalida |

## Migration / Rollout

No migration required. Pure new code + deletion of premature solver files.

## Open Questions

- [ ] Should `listarArchivos()` filter by extension (`.json`) or return all files?
- [ ] Middleware IPC wiring (OP constants + server routing) — out of scope per proposal, but should we add placeholder OP constants now?
