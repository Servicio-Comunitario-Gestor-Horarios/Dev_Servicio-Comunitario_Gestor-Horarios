## Exploration: PR #75 — Review, Resolve Conflicts, and Merge

### PR Summary

**Title**: feat(solver): Motor CP-SAT completo — restricciones, validacion, tests, benchmark
**URL**: https://github.com/Servicio-Comunitario-Gestor-Horarios/Dev_Servicio-Comunitario_Gestor-Horarios/pull/75
**Author**: LuisRojas260305 (Luis Rojas)
**Branch**: `feature/backend/#25-#32-Modelo-CP-SAT` → `main`
**State**: OPEN
**Size**: 176 files changed, +41,606 / -288 lines (extremely large PR)
**Review**: IWinterI reviewed but review was DISMISSED. `reviewDecision: REVIEW_REQUIRED`.

PR #75 implements the complete CP-SAT solver engine for academic schedule assignment. It includes:

- **8 hard constraints** (R1–R8): professor-per-turn, required blocks, weekly hours, non-overlapping classrooms, unique subject per classroom/slot, non-overlapping teacher, professor per shift, professor per plan
- **13 pre-solver validations** (V1–V13): structural integrity, JSON parsing, dimensions, indices, ranges, coverage
- **Full pipeline**: SolverConfig::fromJson → crearVariables → extraerSolucion → analizarFallo
- **12 tests**: 10 unitary + 2 benchmark
- **Documentation**: Informe-Motor-Solver.md

However, the PR also includes the ENTIRE history of the project (all frontend, middleware, backend services, etc.) because it was branched from an early state and has accumulated everything.

### Conflict Status

**Status**: CONFLICTING (`mergeStateStatus: DIRTY`, `mergeable: CONFLICTING`)

**3 files with merge conflicts** (all CMakeLists.txt):

| File | Conflict Description |
|---|---|
| `CMakeLists.txt` (root) | PR reorganized build options, added `BUILD_TESTING` option, moved `find_package(Qt6)` before `BUILD_BACKEND` check, added `Network` component. Main has original structure. |
| `src/middleware/CMakeLists.txt` | PR added real IPC implementation (`internalserver.cpp`, `internalclient.cpp`) replacing placeholder, added `Qt6::Network` linking, added `AUTOMOC`. Main still has placeholder (due to revert of PR #51). |
| `test/CMakeLists.txt` | PR added 14+ new test targets. Main doesn't have these. |

**Root cause of conflicts**: `main` has a revert commit (`3d875a8 Revert "feat(middleware): implementar IPC middleware con health-check (#51)"`) that `develop` doesn't have. The PR branch already merged `develop` into itself (commit `d3ee263`), so it inherited the non-reverted middleware code. When comparing against `main`, the middleware CMakeLists conflict.

### Files Affected

The PR modifies 176 files. Key categories:

**Solver core (NEW — the actual PR content):**
- `src/backend/include/backend/solver/config/solver_config.hpp`
- `src/backend/include/backend/solver/restricciones/restricciones_{aulas,profesores,turnos}.hpp`
- `src/backend/include/backend/solver/solver.hpp`
- `src/backend/include/backend/solver/variables.hpp`
- `src/backend/src/solver/config/solver_config.cpp`
- `src/backend/src/solver/restricciones/restricciones_{aulas,profesores,turnos}.cpp`
- `src/backend/src/solver/solver.cpp`
- `src/backend/src/solver/variables.cpp`
- `config/Solver.json`

**Tests (NEW):**
- `test/backend/test_solver_benchmark.cpp`
- `test/backend/test_solver_horarios.cpp`
- `test/backend/test_variables_cp_sat.cpp`

**Pre-existing code (inherited from develop):**
- Frontend forms, views, login, dashboard (from PR #73)
- Backend services (ServicioAula, ServicioMaterias, etc.)
- Middleware IPC implementation (from PR #51, reverted on main)
- All documentation files

### Risk Assessment

1. **Wrong target branch**: PR targets `main`, but should likely target `develop`. Main is behind develop (has a revert that develop undid). Merging into main would reintroduce middleware IPC that was explicitly reverted.

2. **Massive PR size**: 176 files / 41K+ lines. This makes review extremely difficult. The actual solver code is ~2000 lines; the rest is inherited project history.

3. **No CI checks**: No CI checks reported on the PR branch. No branch protection rules on `main`.

4. **No active reviews**: IWinterI's review was dismissed. CODEOWNERS requires approval from Tech-Leads team.

5. **Conflicts are trivial but numerous**: All 3 conflicts are in CMakeLists.txt files. The resolution is straightforward (take the PR version) since the PR branch is more up-to-date than main.

6. **Local uncommitted changes**: The local `develop` branch has staged changes (solver files) that haven't been committed yet. These appear to be the same solver implementation from the PR branch.

### Recommended Approach

**Option A (RECOMMENDED): Retarget PR to `develop`**

Since the PR branch already has develop merged in and is up-to-date with develop, retargeting to `develop` would eliminate all conflicts (the PR branch is a direct descendant of develop's HEAD).

```bash
gh pr edit 75 --repo Servicio-Comunitario-Gestor-Horarios/Dev_Servicio-Comunitario_Gestor-Horarios --base develop
```

Then merge into develop. This is cleanest because:
- PR branch = develop + solver commits (no conflicts)
- Develop is the integration branch, main is the release branch
- Matches the project's merge pattern (PRs #73, #74, #71, etc. all merged into develop)

**Option B: Resolve conflicts and merge into `main`**

If main is the intended target:
1. Resolve root `CMakeLists.txt` — take PR version (adds BUILD_TESTING, reorganizes options)
2. Resolve `src/middleware/CMakeLists.txt` — take PR version (real IPC implementation)
3. Resolve `test/CMakeLists.txt` — take PR version (adds test targets)
4. Request review from Tech-Leads
5. Merge

**Before any merge**, the local working tree needs to be cleaned:
```bash
git stash  # or git restore --staged . && git checkout .
```

### Ready for Proposal

Yes. The user needs to decide:
1. **Target branch**: `develop` (clean, recommended) vs `main` (conflicts, but matches PR config)
2. **Conflict resolution strategy**: If keeping `main` as target, all 3 conflicts resolve by taking the PR side
3. **Review requirement**: CODEOWNERS requires Tech-Leads approval — need to request review
