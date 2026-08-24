# Auditoría qa-audit — índice de entregables

Change SDD: `qa-audit` (diagnose-only) | Pin: `develop @ e3a7a737f9aa19a6a4d33cfed1cbb79a16eeda54`
Fecha: 2026-08-24 | Ejecución íntegra dentro del contenedor gestor-dev como root.
Única edición de código del change: `test/CMakeLists.txt` (+3 suites huérfanas registradas).

---

## 1. Entregables

| ID | Archivo | Contenido |
|---|---|---|
| D1 | [`mapa-arquitectura-e2e.md`](mapa-arquitectura-e2e.md) | Arquitectura as-is (4 módulos), binario dual-mode, frontera IPC, secuencia de arranque CORREGIDA por evidencia (F-CRASH1: hang, no qFatal), 4 eslabones E2E rotos, puntos de enganche de cada sonda |
| D2 | [`matriz-pruebas.md`](matriz-pruebas.md) | Infra + build/warnings, matriz serial de 23 suites con razón y evidencia, 14 probes IPC, repro crash ×2, prueba DB-untouched, ruta de rollback |
| D3 | [`plan-hotfix.md`](plan-hotfix.md) | Top-3 hotfixes priorizados (effort S/M/L + blast radius + regla de decisión), descartados justificados, orden de ejecución |
| D4 | [`estado-frontend.md`](estado-frontend.md) | Informe por vista solicitado por el usuario: 12 filas {render, fuente de datos, bloqueo, effort, blast radius, evidencia, veredicto}, señales fix/no-fix, limitaciones headless |
| — | [`evidencia/`](evidencia/) | 39 archivos: logs de build/ctest, pares IPC request/response, trazas crash ×2 byte-idénticas, PNG/xwd de UI, análisis estáticos |

Espejos engram: `sdd/qa-audit/d1` … `sdd/qa-audit/d4` (resúmenes condensados).

## 2. Veredicto global en 5 líneas

1. **16/23 suites pasan**; 2 fallan determinista ×2 (solver, F-SOLVER1); 5 no corren por build-blocked (F-BUILD1/F-BUILD2).
2. **Zero-warning REFUTADO**: exactamente 1 warning (`solver_config.cpp:486`).
3. **IPC vivo probado** contra el binario real: middleware es echo-stub (14/14 probes OK, DB intacta).
4. **Arranque integrado roto** por causa distinta a la asumida: hang determinista por socket sin reset (F-CRASH1) + SIGSEGV teardown ×2; MainWindow inalcanzable (F-UI1 + gap post-login).
5. **Frontend**: 5 FUNCTIONAL / 4 BROKEN / 3 PLACEHOLDER / 0 UNTESTABLE (D4). Con 2 hotfixes effort-S el testing GUI puede continuar (D3).

## 3. Ledger de hallazgos

| ID | Resumen | Severidad | Detalle |
|---|---|---|---|
| F-CRASH1 | Hang arranque (retry-storm, socket sin reset) + SIGSEGV teardown ×2 firma idéntica | MAJOR | D1 §5-6, D2 §5 |
| F-SOLVER1 | `resultado.exito=false` determinista ×4 funciones solver | MAJOR | D2 §3 (#17,#18) |
| F-UI1 | FRONTEND_STANDALONE no compila (includes middleware ausentes) | MAJOR | D2 §2, D4 §2 |
| F-BUILD2 | 3 impl backend fuera de CMakeLists → suites huérfanas sin link (~52 tests) | MAJOR (testing) | D2 §2 |
| F-BUILD1 | Drift API GTest en 2 suites preexistentes | MINOR | D2 §2 |
| W-BASELINE | 1 warning (unused totalSlots) — zero-warning refutado | INFO | D2 §2 |
| F-INFRA1 | Probe OR-Tools de verify-dev-env.sh roto en origen (falta -DMODE) | MINOR | D2 §1 |

## 4. Verificación de aceptación (Phase 8)

| AC (spec) | Criterio | Veredicto | Evidencia |
|---|---|---|---|
| AC1 | Contenedor healthcheck-green; build fresco exitoso en contenedor; inventario completo de warnings con captura verbatim | **PASS** | Gate de infra abierto (D2 §1; defecto de script F-INFRA1 documentado, entorno demostrado sano por configure/build/probes directos); configure exit 0 + inventario exacto de 1 warning verbatim (D2 §2; `evidencia/configure.log`, `build-warnings.log`, `build-baseline-statement.txt`) |
| AC2 | `ctest -N` = 23 suites; toda suite con estado final; ~52 tests huérfanos ejecutados O exclusión documentada con razón | **PASS** (rama documented-exclusion) | `evidencia/ctest-list-N.log` (Total Tests: 23); estados finales 16/7/0 (D2 §3); huérfanos ejecutados=0 con razón F-BUILD1/F-BUILD2 documentada |
| AC3 | Reproducción determinista ×2 con trazas; ≥1 CRUD stub evidenciado sobre IPC vivo contra binario real con prueba de aislamiento de DB | **PASS** (narrativa corregida) | Trazas byte-idénticas ×2 offscreen (`evidencia/crash-run{1,2}.log`, `crash-diff.txt`) + SIGSEGV ×2 Xvfb (`ui-xvfb-postlogin-run{2,3}.log`): modo de fallo REAL es hang+segfault, qFatal asumido REFUTADO y documentado; 14/14 probes IPC incl. CRUD echo-stub ×3 entidades (`ipc-probe-pairs.txt`); EMPTY_DIFF + 0 archivos DB (`ipc-db-isolation-proof.txt`) |
| AC4 | D1-D4 existen en docs/auditoria/ + espejos engram; D3 prioriza top-3 con causas trazadas; D4 soporta fix/no-fix sin re-test | **PASS** | Este directorio (5 docs); espejos `sdd/qa-audit/d1..d4`; D3 §2-4 (root cause file:line + traza vinculada por hotfix); D4 §3-4 (veredicto+bloqueo+effort+blast radius por fila) |
| AC5 | `git diff e3a7a73` muestra SOLO test/CMakeLists.txt cambiado (código) + nuevos docs/auditoria/*; revertir el commit único restaura el pin exacto | **PASS** | `git diff --stat` = únicamente `test/CMakeLists.txt \| 21 +++` ; árbol según contrato (§5 abajo); edición aún SIN commitear por contrato de batch — estrategia single-commit documentada en D2 §7 para que `git revert <sha>` restaure el pin al cerrar |

Resultado: **5/5 PASS**.

## 5. Regla de árbol (tree-rule check) — salida verificada

```
$ git status --short
 M test/CMakeLists.txt          ← única edición de código (contrato)
?? docs/auditoria/              ← producto de este change (nuevo)
?? .agents/skills/framer-motion/ .agents/skills/slideshow/ .claude/
?? docs/Diagrama-MER.pdf  docs/analisis-actividad/  docs/landing-page/
?? openspec/changes/landing-page-avances/
?? scripts/__pycache__/  scripts/traqueo-horas-por-sprint.py   ← ruido pre-existente tolerado (tarea 1.1)
```

Cumple la regla: solo `test/CMakeLists.txt` modificado + `docs/auditoria/**` nuevo; el resto es ruido untracked PRE-EXISTENTE al pin, documentado como tolerado desde Preflight.

## 6. Limpieza y rollback

- `build-ui/` eliminado (tarea 8.6 adelantada en Batch B; verificado ausente).
- Logs de `/tmp` ya replicados en `evidencia/` (durable).
- Rollback: commitear SOLO `test/CMakeLists.txt` (commit único) → `git revert <sha>` restaura pin exacto; `docs/auditoria/` queda untracked (registro de auditoría; su destino se decide en archive). Detalle: D2 §7.
