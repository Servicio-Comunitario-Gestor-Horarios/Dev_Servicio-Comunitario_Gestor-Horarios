# D2 — Matriz de pruebas y evidencia

Change: `qa-audit` | Pin: `develop @ e3a7a737f9aa19a6a4d33cfed1cbb79a16eeda54`
Regla de la matriz: toda celda lleva comando + ruta de log resoluble; sin PENDING sin justificar.

---

## 0. Contexto de ejecución

| Parámetro | Valor | Evidencia |
|---|---|---|
| Pin | `develop @ e3a7a73` (SHA completo en baseline statement) | `evidencia/build-baseline-statement.txt` |
| Entorno | contenedor gestor-dev como root; volumen build/ root-owned | `evidencia/configure.log` |
| Herramientas dinámicas | socat / Xvfb / xdotool / xwd DISPONIBLES (network OK) | Batch A log; probes S3/S4 exitosos abajo |
| Política ctest | SERIAL (sin `-j`), `--timeout 300`; suites IPC comparten nombre de QLocalServer | `evidencia/ctest-summary.md` header |

## 1. Fila Infra (R1)

| Check | Resultado | Comando | Evidencia |
|---|---|---|---|
| Contenedor + healthcheck | **PASSED** (gate abierto) | `docker compose -f docker/docker-compose.yml up -d --build` → `bash docker/verify-dev-env.sh` | Batch A `/tmp/verify-env.log` → copiado a `evidencia/` |
| Probe OR-Tools del script | **DEFECTUOSO** (defecto DEL SCRIPT, no del entorno) | ídem | F-INFRA1: falta arg `-DMODE`; superado por probe directo de librería OR-Tools 9.15 en configure (`evidencia/configure.log`) |

## 2. Fila Build fresco + inventario de warnings (R2)

| Check | Resultado | Comando | Evidencia |
|---|---|---|---|
| Configure | PASSED exit 0 (OR-Tools 9.15, GTest ON) | `cmake -S /workspace -B /workspace/build -G Ninja` | `evidencia/configure.log` |
| Build pass 1 (default) | exit 1 al primer fallo | `cmake --build /workspace/build` | `evidencia/build-full.log` |
| Build pass 2 (`-k 0`, superficie total) | exit 1 con **5 edges FAILED** deterministas | `cmake --build /workspace/build -- -k 0` | `evidencia/build-errors.log` |
| Baseline zero-warning | **REFUTADA** — exactamente **1 warning** | grep sobre build log | `evidencia/build-warnings.log`, statement íntegro en `build-baseline-statement.txt` |

### Inventario de warnings (completo)

| # | Archivo:línea | Warning | Flag |
|---|---|---|---|
| 1 | `src/backend/src/solver/config/solver_config.cpp:486:15` | unused variable 'totalSlots' | -Wunused-variable |

### Inventario de errores de build (5 edges)

| Target | Tipo | Detalle | Hallazgo |
|---|---|---|---|
| test_servicio_exportacion | compile (8 errores) | `Resultado<QString>` sin esExitoso/obtenerValor/obtenerError | F-BUILD1 (drift API) |
| test_backend_variables_cp_sat | compile (3 errores) | `ProfesorSolverConfig` sin horas_planificacion | F-BUILD1 (drift API) |
| test_horario_salida | link | undefined toJson/fromJson de AsignacionOutput/DiaOutput/CursoOutput | F-BUILD2 |
| test_servicio_consulta_horario | link | undefined ServicioConsultaHorario::<ctor>/buscarPor\* | F-BUILD2 |
| test_servicio_horario_salida | link | undefined ServicioHorarioSalida::<ctor>/listarArchivos | F-BUILD2 |

Root cause F-BUILD2 probado con `nm -C libbackend.a`: 0 símbolos T; las 3 implementaciones existen en árbol pero NO están en las fuentes de `src/backend/CMakeLists.txt` (`evidencia/build-baseline-statement.txt`).

## 3. Matriz serial ctest — 23 suites (R3)

Comando: `ctest --test-dir /workspace/build --output-on-failure --timeout 300` (serial). Registro previo: `ctest --test-dir build -N` → **Total Tests: 23** (`evidencia/ctest-list-N.log`). Resultado global: **16 PASSED / 7 FAILED / 0 SKIPPED — 466.88s** (`evidencia/ctest-summary.md`, raw `evidencia/ctest.log`).

| # | Suite | Estado | Duración | Razón / detalle | Evidencia |
|---|---|---|---|---|---|
| 1 | test_backend_franja_horaria | PASSED | 1.96s | — | `evidencia/ctest.log:4` |
| 2 | test_backend_aula | PASSED | 0.01s | — | `evidencia/ctest.log:6` |
| 3 | test_backend_materia | PASSED | 0.01s | — | `evidencia/ctest.log:8` |
| 4 | test_backend_profesor | PASSED | 0.01s | — | `evidencia/ctest.log:10` |
| 5 | test_backend_horario | PASSED | 0.01s | — | `evidencia/ctest.log:12` |
| 6 | test_backend_plan_estudio | PASSED | 0.01s | — | `evidencia/ctest.log:14` |
| 7 | test_backend_database | PASSED | 10.52s | — | `evidencia/ctest.log:16` |
| 8 | test_backend_entity_graph | PASSED | 0.01s | — | `evidencia/ctest.log:18` |
| 9 | test_middleware_health_check | PASSED | 1.30s | — | `evidencia/ctest.log:20` |
| 10 | test_backend_variables_cp_sat | FAILED(build-blocked, Not Run) | — | binario ausente ×2 runs; drift API GTest | `evidencia/build-errors.log` (F-BUILD1) |
| 11 | test_servicio_aula | PASSED | 89.71s | — | `evidencia/ctest.log:55` |
| 12 | test_servicio_materias | PASSED | 81.19s | — | `evidencia/ctest.log:57` |
| 13 | test_servicio_planes_estudio | PASSED | 83.34s | — | `evidencia/ctest.log:59` |
| 14 | test_servicio_carga_horaria | PASSED | 131.24s | — | `evidencia/ctest.log:61` |
| 15 | test_composicion_plan | PASSED | 67.02s | — | `evidencia/ctest.log:63` |
| 16 | test_edge_cases | PASSED | 0.01s | — | `evidencia/ctest.log:65` |
| 17 | test_solver_horarios | FAILED(reproduced ×2) | — | FAIL solver_xorUnDocentePorMateria (:158) + solver_suplenteReemplaza (:253); `resultado.exito` FALSE; Totals 9 passed/2 failed | `evidencia/ctest.log:67` + `evidencia/ctest-retry-failed-twice.log` (**F-SOLVER1**) |
| 18 | test_solver_benchmark | FAILED(reproduced ×2) | <1s | FAIL benchmark_solverGrandeResuelve (:103) + verificaConsistencia (:111); Totals 2/2; timebox NO aplicable (termina en <1s) | `evidencia/ctest.log:87` + retry log + `evidencia/ctest-benchmark-timebox600.log` (**F-SOLVER1**) |
| 19 | test_middleware_crud_routes | PASSED | 0.11s | corroboración in-process de los stubs S3 | `evidencia/ctest.log:105` |
| 20 | test_servicio_exportacion | FAILED(build-blocked, Not Run) | — | binario ausente ×2 runs; drift API GTest | `evidencia/build-errors.log` (F-BUILD1) |
| 21 | test_horario_salida *(huérfano registrado)* | FAILED(build-blocked, Not Run) | — | impl nunca compilada en libbackend.a | `evidencia/build-errors.log` + `build-baseline-statement.txt` (F-BUILD2) |
| 22 | test_servicio_consulta_horario *(huérfano)* | FAILED(build-blocked, Not Run) | — | ídem | ídem |
| 23 | test_servicio_horario_salida *(huérfano)* | FAILED(build-blocked, Not Run) | — | ídem | ídem |

Notas de clasificación:
- **FAILED-twice honrado**: #17/#18 re-ejecutados y fallan idéntico (`ctest-retry-failed-twice.log`); #10/#20/#21/#22/#23 "Not Run" determinista entre pase 1 y re-run (sin dimensión de flake).
- **Timebox**: política `timeout 600` aplicada a benchmark; completó en 0.20s → FAILED genuino, no SKIPPED(timebox) (`ctest-benchmark-timebox600.log`).
- **~52 funciones huérfanas ejecutadas = 0**: exclusión documentada vía F-BUILD1/F-BUILD2 (rama "exclusion documented w/ reason" del AC2).

## 4. Probes IPC en vivo contra binario real (R4)

Vehículo: `printf '<json>' | socat - UNIX-CONNECT:/tmp/GestorHorarios_Middleware` contra `/workspace/build/src/app/gestor-horarios --backend` (binario REAL, no mock). Resultado: **14/14 SOCAT_EXIT=0**. Raw: `evidencia/ipc-probe-pairs.txt`; log servidor: `evidencia/ipc-backend.log`.

| # | op | Request | Response | Clasificación |
|---|---|---|---|---|
| 1 | health_check | `{"op":"health_check"}` | `{"code":0,"data":"ok","status":"ok"}` | real handler OK |
| 2 | ready | `{"op":"ready"}` | `{"code":0,"data":"ok","status":"ok"}` | real handler OK |
| 3 | teacher_list | `{"op":"teacher_list"}` | `{"code":0,"data":[],"status":"ok"}` | stub lista vacía |
| 4 | teacher_create | `{"op":"teacher_create","payload":{"nombre":"Probe Teacher","correo":"probe@test.local"}}` | echo verbatim del payload, code 0 | **stub-echo** |
| 5 | teacher_update | `{"op":"teacher_update","payload":{"id":"t-1",...}}` | echo verbatim | **stub-echo** |
| 6 | classroom_list | `{"op":"classroom_list"}` | `data:[]` | stub lista vacía |
| 7 | classroom_create | payload {nombre, capacidad:30} | echo verbatim | **stub-echo** |
| 8 | classroom_update | payload {id:"c-1", capacidad:45} | echo verbatim | **stub-echo** |
| 9 | subject_list | `{"op":"subject_list"}` | `data:[]` | stub lista vacía |
| 10 | subject_create | payload {nombre, tipoAula} | echo verbatim | **stub-echo** |
| 11 | subject_update | payload {id:"s-1",...} | echo verbatim | **stub-echo** |
| 12 | teacher_get sin id | `{"op":"teacher_get","payload":{}}` | `{"code":-4,"data":"Falta campo id"}` | error-path OK |
| 13 | op desconocida | `{"op":"nonexistent_op"}` | `code:-4 "Operación desconocida"` | error-path OK |
| 14 | JSON malformado | `<<<not-json{{{` | `code:-4 "JSON malformado"` | error-path OK |

Confirmación stub-echo: ×3 entidades (teacher/classroom/subject), create+update devuelven EXACTAMENTE el payload recibido sin IDs reales ni persistencia.

### Prueba de aislamiento de DB

| Check | Resultado | Evidencia |
|---|---|---|
| Snapshot antes/después (sha256+mtime de \*.db/\*.sqlite) | 0 archivos antes Y después; **EMPTY_DIFF=yes**; ninguno creado en todo el workspace | `evidencia/ipc-db-isolation-proof.txt` |

## 5. Reproducción de fallo de arranque (R5)

| Run | Modo | Exit | Traza | Evidencia |
|---|---|---|---|---|
| run1 | offscreen integrado, timeout 25s | 124 (kill — proceso vivo) | 137 líneas, 0 fatal/abort | `evidencia/crash-run1.log` |
| run2 | idéntico, proceso fresco | 124 | byte-idéntica a run1 (TRACES_IDENTICAL=yes) | `evidencia/crash-run2.log`, `crash-diff.txt` |
| xvfb run2/run3 | Xvfb :99 + xdotool, login aceptado | SIGSEGV 139 ambos | firma idéntica: "COLAPSó código 15" → reintento 1/3 → startTimer/QThread warning | `evidencia/ui-xvfb-postlogin-run2.log`, `-run3.log`, screenshot `ui-xvfb-postlogin.xwd` |

Veredicto: qFatal por doble QApplication **REFUTADO** (0 líneas fatal/abort; Qt 6.4.2 tolera la 2ª QApplication). Defecto real: **F-CRASH1** hang determinista (retry-storm) + SIGSEGV teardown ×2 firma idéntica → FAILED(deterministic-reproduced ×2). Narrativa corregida y trazada en D1 §5 y `evidencia/crash-static-analysis.md`.

## 6. Veredictos UI (R6) — resumen

Matriz detallada de 12 filas en **D4** (`estado-frontend.md`). Resumen: FUNCTIONAL 5 / BROKEN 4 / PLACEHOLDER 3 / UNTESTABLE-headless 0. Vehículos y logs: `evidencia/ui-probe-verdicts.md`, `ui-probe-run.log`, `ui-F-UI1-standalone-build-failure.md`.

## 7. Rollback (ruta documentada — tarea 8.6)

- Única edición de código: `test/CMakeLists.txt` (+21 líneas, 3 bloques `add_qtest`), hoy SIN commitear según contrato de batch (working tree).
- Estrategia single-commit: al cerrar el change, commitear SOLO `git add test/CMakeLists.txt && git commit` → el revert `git revert <sha>` restaura el pin exacto e3a7a73.
- `docs/auditoria/**` queda UNTRACKED por defecto (registro de auditoría; su borrado se decide solo en archive).
- Artefactos generados ya limpios: `build-ui/` eliminado (tarea 8.6 adelantada en Batch B); `build/` permanece solo dentro del contenedor; logs de /tmp ya replicados en `evidencia/`.
