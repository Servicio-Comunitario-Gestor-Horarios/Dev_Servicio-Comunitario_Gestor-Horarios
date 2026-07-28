## Exploration: Code Review - Nicole's PR (pr-62-review)

### Current State

The PR adds 5 service classes implementing CRUD operations for aulas, materias, planes de estudio, carga horaria, and plan composition. It touches 23 files (+2548 lines) across headers, implementations, tests, and CMake configuration. The services bridge the existing database schema (SQLite via `DatabaseManager`) with the solver's domain structs (`Aula`, `Materia`, `PlanEstudio`, `MateriaEnPlan`).

---

### Affected Areas

**New Service Headers** (in `src/backend/include/backend/services/`):
- `ServicioAula.hpp` -- CRUD for Aulas table + solver bridge
- `ServicioMaterias.hpp` -- CRUD for Materias table + solver bridge
- `ServicioPlanesEstudio.hpp` -- CRUD for PlanEstudio table
- `ServicioCargaHoraria.hpp` -- CRUD for PlanEstudio_Materia join table + hour limit enforcement
- `ServicioComposicionPlan.hpp` -- Assembles complete PlanEstudio from DB for solver

**New Service Implementations** (in `src/backend/src/services/`):
- `ServicioAula.cpp` (198 lines)
- `ServicioMaterias.cpp` (220 lines)
- `ServicioPlanesEstudio.cpp` (162 lines)
- `ServicioCargaHoraria.cpp` (289 lines)
- `ServicioComposicionPlan.cpp` (155 lines)

**New DTO Headers** (inline in service headers):
- `AulaDTO` (in ServicioAula.hpp)
- `MateriaDTO` (in ServicioMaterias.hpp)
- `PlanDTO` (in ServicioPlanesEstudio.hpp)
- `CargaHorariaDTO` (in ServicioCargaHoraria.hpp)

**Orphaned/Conflicting DTO Files** (in `src/backend/include/backend/services/`):
- `MateriaDTO.hpp` -- defines MateriaDTO with DIFFERENT fields (has `descripcion`, missing `requisitos`)
- `PlanDTO.hpp` -- defines PlanDTO with DIFFERENT fields (has `activo`, missing `nombre`)
- `CargaHorariaDTO.hpp` -- defines CargaHorariaDTO with DIFFERENT fields (`año` vs `curso`, `horasSemanales` vs `horas`)

**Three Conflicting Resultado Templates**:
- `src/backend/include/backend/resultado.hpp` -- USED by services (`ok`, `valor`, `mensajeError`, `codigoError`, `exito()`, `error()`)
- `src/backend/include/backend/common/Resultado.hpp` -- UNUSED (`exito`, `valor`, `error`, `correcto()`, `fallo()`)
- `src/backend/include/backend/services/Resultado.hpp` -- UNUSED (`ok`, `valor`, `error`, `correcto()`, `fallo()`)

**New Tests** (in `test/backend/`):
- `test_servicio_aula.cpp` (179 lines, 14 tests)
- `test_servicio_materias.cpp` (182 lines, 12 tests)
- `test_servicio_planes_estudio.cpp` (166 lines, 12 tests)
- `test_servicio_carga_horaria.cpp` (213 lines, 14 tests)
- `test_composicion_plan.cpp` (178 lines, 8 tests)

**Modified CMake**:
- `src/backend/CMakeLists.txt` -- Added sources (BROKEN: see issues)
- `test/CMakeLists.txt` -- Added test executable (BROKEN: see issues)

---

### Convention Analysis

| Convention | Status | Notes |
|---|---|---|
| Naming (PascalCase classes, camelCase methods) | PASS | Consistent across all services |
| File organization (hpp in include/, cpp in src/) | PASS | Follows existing pattern |
| Include paths (`"backend/services/..."` in .cpp) | PASS | Matches existing `DatabaseManager.cpp` pattern |
| `#pragma once` in headers | PASS | All headers use it |
| Doxygen-style comments | MIXED | Service headers use `/** @brief */`, DTO files use `/* */`, existing code uses `///` |
| Brace style (Allman for classes, K&R for functions) | MIXED | Existing code uses Allman for everything; new code mixes K&R function bodies |
| Newline at EOF | FAIL | ALL new files missing trailing newline |
| Compiler warnings (-Wall -Wextra -Wpedantic) | LIKELY PASS | No obvious issues, but can't verify without build |
| Test helper usage (`add_qtest()`) | FAIL | Tests use raw `add_executable` instead of project's `add_qtest()` helper |

---

### Code Quality Assessment

**CRITICAL Issues (Must Fix):**

1. **CMakeLists.txt triple `add_library(backend STATIC)`**: The PR appends a second `add_library(backend STATIC)` block instead of modifying the existing one. The develop branch already has TWO `target_sources` blocks for the same target. The PR adds a THIRD `add_library` call. This will cause a CMake error at configure time. **All source files are listed three times across three separate blocks.**

2. **Missing `#include <QSqlDatabase>` in ALL 5 service headers**: Every service header declares `QSqlDatabase& m_db` and `ServicioFoo(QSqlDatabase& db)` but none include `<QSqlDatabase>`. They rely on transitive includes through `backend/resultado.hpp` -> `<QVariant>` -> (maybe QSqlDatabase?). This is fragile and will likely fail on some compilers/platforms.

3. **Orphaned DTO files create name collisions**: `MateriaDTO.hpp`, `PlanDTO.hpp`, `CargaHorariaDTO.hpp` define structs with the same names as the DTOs in the service headers but with DIFFERENT fields. If anyone includes both the service header and the standalone DTO header, they'll get ODR violations or compilation errors. These files appear to be leftover from an earlier design iteration and should be deleted.

**HIGH Issues:**

4. **Three conflicting `Resultado` templates**: The PR introduces `backend/resultado.hpp` (the one actually used) while also adding `backend/common/Resultado.hpp` and `backend/services/Resultado.hpp` with different APIs (`exito()`/`error()` vs `correcto()`/`fallo()`). Only `backend/resultado.hpp` is used by the services. The other two are dead code that creates confusion.

5. **N+1 query in `ServicioComposicionPlan::obtenerMateriasConCarga`**: For each materia in the plan, a separate query is executed to get requisitos. With 20 materias per plan, this is 21 queries instead of 2. Should use a JOIN or batch the queries.

6. **`eliminarAula` doesn't check referential integrity**: `eliminarMateria` checks `PlanEstudio_Materia` and `Profesor_Materia` before deleting. `eliminarPlan` checks `PlanEstudio_Materia`. But `eliminarAula` just deletes directly. If aulas are referenced from other tables (or will be in the future), this could fail at runtime with foreign key constraint errors.

7. **Error detection via string matching**: `query.lastError().text().contains("UNIQUE")` is fragile. SQLite error messages can vary by version and locale. Better to check `query.lastError().type()` or use `sqlite3_errstr()` directly.

**MEDIUM Issues:**

8. **`PlanDTO` inconsistency**: The standalone `PlanDTO.hpp` has `activo` field; the service's `PlanDTO` doesn't. The schema (`PlanEstudio` table) doesn't have an `activo` column either. The standalone file is wrong AND orphaned.

9. **No transaction wrapping**: `eliminarMateria` performs multiple SELECT queries (checking associations) then a DELETE, all without a transaction. Under concurrent access, a materia could be associated between the check and the delete.

10. **`test/CMakeLists.txt` doesn't use `add_qtest()`**: The project has a well-defined `add_qtest()` helper that handles AUTOMOC, linking Qt6::Test, and adding to CTest. The PR creates a manual `add_executable(backend-tests)` instead. This is inconsistent AND means all 5 test files are compiled into a single binary (can't run individual test files).

**LOW Issues:**

11. **`CargaHorariaDTO::id` is misleading**: Set to `id_Materia` in `mapearARecord`, but the comment says "id único autoincrementable". There's no auto-increment ID in `PlanEstudio_Materia` (it's a composite PK).

12. **Missing `obtenerAulaParaSolver` test with invalid ID**: Tests exist for happy path only.

13. **`test_composicion_plan.cpp` line 71**: Accesses `resultado.valor.materias[0]` without checking size first - could crash if query returns wrong data.

---

### Test Analysis

**Coverage by Service:**

| Service | CRUD Tests | Edge Cases | Solver Bridge Tests | Total |
|---|---|---|---|---|
| ServicioAula | 7 | 4 (empty, cap 0, cap 600, dup) | 2 | 13 |
| ServicioMaterias | 6 | 3 (empty, dup, plan FK) | 2 (toMateria) | 11 |
| ServicioPlanesEstudio | 6 | 3 (empty code, empty name, dup) | N/A | 9 |
| ServicioCargaHoraria | 6 | 6 (plan FK, materia FK, curso bounds, hours 0, dup, limit) | 2 | 14 |
| ServicioComposicionPlan | 3 | 4 (empty code, nonexistent plan, empty plan) | 2 | 9 |

**Test Quality:**
- GOOD: Each test uses its own `QTemporaryDir` + `DatabaseManager` for full isolation
- GOOD: Tests cover both happy paths and error paths
- GOOD: Cleanup is properly implemented (reverse order of init)
- GOOD: Tests verify error message content, not just pass/fail

**Missing Tests:**
- No test for `ServicioAula::actualizarAula` with duplicate name
- No test for `ServicioMaterias::eliminarMateria` associated with a professor
- No test for `ServicioCargaHoraria::eliminarCarga` of non-existent record
- No test for `ServicioComposicionPlan::obtenerMateriasConCarga` when plan has materias with no requisitos
- No integration test that exercises multiple services together (e.g., create plan -> add materias -> assign carga -> compose plan)
- No test for concurrent access or transaction behavior

---

### Architecture Consistency

**Fits well:**
- Services follow the same 4-layer architecture (app/frontend/backend/middleware)
- Services use `QSqlDatabase&` dependency injection matching `DatabaseManager` pattern
- DTOs bridge between DB schema and solver domain structs (`Aula`, `Materia`, `PlanEstudio`)
- Table names in SQL match the existing migration schema exactly
- Solver bridge methods (`obtenerTodasParaSolver`, `obtenerMateriaParaSolver`) follow a consistent pattern

**Doesn't fit:**
- The test registration pattern breaks the project convention (`add_qtest` helper)
- The CMakeLists structure is completely wrong (triple `add_library`)
- Dead/orphaned files (`common/Resultado.hpp`, `services/Resultado.hpp`, standalone DTOs) add confusion
- Brace style is inconsistent with existing code (Allman in data structs vs K&R in services)

---

### Risks

1. **BUILD WILL FAIL**: The CMakeLists.txt changes will cause CMake configuration errors due to duplicate `add_library` calls. This must be fixed before merge.

2. **COMPILATION MAY FAIL**: Missing `#include <QSqlDatabase>` in headers means the build depends on transitive includes that may not exist on all platforms/compilers.

3. **ODR VIOLATIONS**: If anyone includes both a service header and the corresponding standalone DTO header (e.g., `ServicioMaterias.hpp` and `MateriaDTO.hpp`), they'll get `MateriaDTO` defined twice with different fields.

4. **CONFUSION FOR FUTURE DEVELOPERS**: Three `Resultado` templates with different APIs will cause confusion about which one to use.

5. **PERFORMANCE**: N+1 query pattern in `obtenerMateriasConCarga` will be slow with many materias per plan.

6. **INTEGRATION**: No middleware or frontend integration yet - these services are standalone. The PR doesn't verify they work with the rest of the application.

---

### Recommendation

**Do NOT merge as-is.** The PR has solid business logic and good test coverage, but has several structural issues that must be addressed:

**Must Fix Before Merge:**
1. Fix CMakeLists.txt -- merge the three `add_library` blocks into one, add service sources to existing `target_sources`
2. Add `#include <QSqlDatabase>` to all 5 service headers
3. Delete orphaned files: `common/Resultado.hpp`, `services/Resultado.hpp`, `MateriaDTO.hpp`, `PlanDTO.hpp`, `CargaHorariaDTO.hpp`
4. Refactor `test/CMakeLists.txt` to use `add_qtest()` helper or at least register each test as a separate CTest target
5. Add trailing newlines to all files

**Should Fix:**
6. Fix N+1 query in `obtenerMateriasConCarga` with a JOIN
7. Add referential integrity check to `eliminarAula`
8. Standardize `Resultado` template usage -- keep only `backend/resultado.hpp`
9. Add missing edge case tests (duplicate name on update, empty requisitos, etc.)

**Nice to Have:**
10. Wrap multi-step operations in transactions
11. Replace string-based UNIQUE error detection with error code checking
12. Add integration test across multiple services
