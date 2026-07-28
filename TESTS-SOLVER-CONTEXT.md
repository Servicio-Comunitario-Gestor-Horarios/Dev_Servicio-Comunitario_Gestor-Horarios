# Solver Tests — Context for Next Session

## PR #75: CP-SAT Solver

Branch: `feature/cp-sat-solver` → `develop`

## 3 Failing Tests (Fixed, Need Build Verification)

### 1. XOR Test (Milestone 7)
- **File**: `test/backend/test_solver_horarios.cpp` line ~131
- **Problem**: Original tested "only 1 prof teaches Matematica per curso" — but solver has NO such constraint
- **Fix**: Redesigned with 2 cursos × 2h each, 2 profs × 2h each. Verifies only 1 prof teaches per curso.

### 2. Suplente Test (Milestone 9)
- **File**: `test/backend/test_solver_horarios.cpp` line ~233
- **Problem**: Prof A had plan `"Ciencias"` but curso had `"Plan Ciencias"` → exact string mismatch → `restriccionProfesorPorPlan` forced all vars to 0 → INFEASIBLE
- **Fix**: Changed Prof A's plan to `"Plan Ciencias"`

### 3. Benchmark Test
- **File**: `test/backend/test_solver_benchmark.cpp` line ~78
- **Problem**: Comment said hours sum=77 but actual sum was 80 (matches materia total)
- **Fix**: Fixed comment only

## Key Constraint Rules (R5-R8)

- **R5** `restriccionHorasSemanalesProfesor`: Each prof teaches EXACTLY `ceil(horas_aula * 60 / duracionSlot)` blocks. **Total prof hours MUST equal total materia hours.**
- **R7** `restriccionProfesorPorTurno`: Forces vars to 0 for slots outside prof's turno
- **R8** `restriccionProfesorPorPlan`: Forces vars to 0 when `prof.plan != curso.plan` (exact string match, empty plan skips check)

## Build Command
```bash
docker exec gestor-dev bash -c "cd /workspace/build && cmake --build . --target backend test_solver_horarios test_solver_benchmark 2>&1 | tail -20"
```

## Test Command
```bash
docker exec gestor-dev bash -c "cd /workspace/build && QT_QPA_PLATFORM=offscreen ./test/test_solver_horarios 2>&1"
docker exec gestor-dev bash -c "cd /workspace/build && QT_QPA_PLATFORM=offscreen ./test/test_solver_benchmark 2>&1"
```

## Next Steps
1. Build all targets
2. Run `test_solver_horarios` — expect all 10 tests pass
3. Run `test_solver_benchmark` — expect both benchmarks pass
4. Commit and push to PR #75
