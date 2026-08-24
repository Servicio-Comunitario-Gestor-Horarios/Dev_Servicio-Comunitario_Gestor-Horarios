# ctest Serial Matrix — Batch A @ develop e3a7a73
# Command: ctest --test-dir /workspace/build --output-on-failure --timeout 300   (NO -j, serial)
# Context: inside gestor-dev container as root; fresh build; 5 suites lack binaries (see build-errors.log)
# Raw logs: ctest.log (full run), ctest-retry-failed-twice.log, ctest-benchmark-timebox600.log, ctest-list-N.log

RESULT: 16 passed / 7 failed / 0 skipped — Total 466.88 sec

| #  | Suite                          | Status                        | Evidence |
|----|--------------------------------|-------------------------------|----------|
| 1  | test_backend_franja_horaria    | PASSED 1.96s                  | ctest.log:4 |
| 2  | test_backend_aula              | PASSED 0.01s                  | ctest.log:6 |
| 3  | test_backend_materia           | PASSED 0.01s                  | ctest.log:8 |
| 4  | test_backend_profesor          | PASSED 0.01s                  | ctest.log:10 |
| 5  | test_backend_horario           | PASSED 0.01s                  | ctest.log:12 |
| 6  | test_backend_plan_estudio      | PASSED 0.01s                  | ctest.log:14 |
| 7  | test_backend_database          | PASSED 10.52s                 | ctest.log:16 |
| 8  | test_backend_entity_graph      | PASSED 0.01s                  | ctest.log:18 |
| 9  | test_middleware_health_check   | PASSED 1.30s                  | ctest.log:20 |
| 10 | test_backend_variables_cp_sat  | FAILED(build-blocked, Not Run)| build-errors.log (ProfesorSolverConfig API drift); binary absent ×2 runs |
| 11 | test_servicio_aula             | PASSED 89.71s                 | ctest.log:55 |
| 12 | test_servicio_materias         | PASSED 81.19s                 | ctest.log:57 |
| 13 | test_servicio_planes_estudio   | PASSED 83.34s                 | ctest.log:59 |
| 14 | test_servicio_carga_horaria    | PASSED 131.24s                | ctest.log:61 |
| 15 | test_composicion_plan          | PASSED 67.02s                 | ctest.log:63 |
| 16 | test_edge_cases                | PASSED 0.01s                  | ctest.log:65 |
| 17 | test_solver_horarios           | FAILED(reproduced x2)         | ctest.log:67 + ctest-retry-failed-twice.log; FAIL! solver_xorUnDocentePorMateria (test_solver_horarios.cpp:158), solver_suplenteReemplaza (:253) — 'resultado.exito' FALSE; Totals 9/2 |
| 18 | test_solver_benchmark          | FAILED(reproduced x2)         | ctest.log:87 + retry log + benchmark-timebox600 (0.20s, NOT timebox-killed); FAIL! benchmark_solverGrandeResuelve (:103), benchmark_verificaConsistencia (:111) — 'resultado.exito' FALSE; Totals 2/2 |
| 19 | test_middleware_crud_routes    | PASSED 0.11s                  | ctest.log:105 |
| 20 | test_servicio_exportacion      | FAILED(build-blocked, Not Run)| build-errors.log (Resultado<QString> API drift); binary absent ×2 runs |
| 21 | test_horario_salida            | FAILED(build-blocked, Not Run)| F-BUILD2: impl horario_salida.cpp not compiled into libbackend.a (nm: 0 T-symbols) |
| 22 | test_servicio_consulta_horario | FAILED(build-blocked, Not Run)| F-BUILD2: ServicioConsultaHorario.cpp not compiled into libbackend.a |
| 23 | test_servicio_horario_salida   | FAILED(build-blocked, Not Run)| F-BUILD2: ServicioHorarioSalida.cpp not compiled into libbackend.a |

NOTES
-----
- Registration verified: ctest -N => "Total Tests: 23" (#21-23 = newly registered orphans; ctest-list-N.log).
- ~52 orphan test functions EXECUTED: 0 — excluded per AC2 documented-exclusion branch: link failure F-BUILD2
  (implementations exist in tree but absent from src/backend/CMakeLists.txt sources; sole-edit sanction forbids fixing).
- Benchmark timebox policy applied (task 3.4): suite finished in <1s both runs => genuine FAILED, SKIPPED(timebox) NOT applicable.
- FAILED-twice rule honored: #17/#18 re-run deterministically Failed (ctest-retry-failed-twice.log);
  #10/#20/#21/#22/#23 "Not Run" deterministic across pass1/re-run (missing executables, no flake dimension).
