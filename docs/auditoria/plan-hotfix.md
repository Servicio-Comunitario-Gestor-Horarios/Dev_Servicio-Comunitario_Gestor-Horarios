# D3 — Plan top-3 hotfixes priorizado

Change: `qa-audit` | Pin: `develop @ e3a7a73` | Entrada: ledger de hallazgos D1 §7 / D2.
Objetivo del usuario que gobierna el ranking: *"saber qué es funcional para decidir qué arreglar y
que el testing pueda continuar"*.

---

## 1. Regla de decisión y criterios de ranking

Regla contractual (spec/design): *workaround existe → seguir testeando; encolar hotfix cuando el
hallazgo bloquea ≥2 flujos downstream O effort=S*.

Criterios aplicados a cada candidato:

| Candidato | Effort | Blast radius | Flujos que desbloquea | ¿Top-3? |
|---|---|---|---|---|
| **F-CRASH1** socket sin reset | **S** (1 archivo) | `internalclient.cpp` | TODOS los flujos multi-op: cualquier 2º uso del cliente IPC falla; hang de arranque integrado; SIGSEGV teardown | **SÍ #1** |
| **MainWindow inalcanzable + F-UI1** | **S** (2 sub-fixes) | 2 archivos (`aplicacion_frontend.cpp`, `src/frontend/CMakeLists.txt`) | TODO flujo GUI E2E (ningún usuario llega a ventana principal hoy) | **SÍ #2** |
| **F-BUILD2** fuentes backend sin registrar | **S** (+3 líneas CMake) | `src/backend/CMakeLists.txt` | 3 suites huérfanas ≈52 funciones de test | **SÍ #3** |
| Puente middleware→backend (15 rutas stub) | **L** | middleware+backend+3 vistas (feature-scale) | persistencia real de CRUD + población de listas | NO — cambio propio (§4) |
| F-SOLVER1 solver exito=false | **M/L** (diagnóstico CP-SAT previo) | dominio solver aislado | no bloquea testing de otras capas (16/23 suites pasan) | NO — follow-up (§4) |
| F-BUILD1 drift API GTest ×2 | S/M | 2 archivos test | solo esas 2 suites | NO (§4) |
| F-INFRA1 probe script | S | 1 script docker | nada en runtime | NO (§4) |

Justificación contra el objetivo: los tres elegidos son effort=S con máximo efecto desbloqueante y
mínimo blast radius; tras ellos, "el testing puede continuar" en las tres dimensiones rotas
(integrado runtime, GUI e2e, cobertura unitaria huérfana). El puente queda fuera por ser
feature-scale (L) y depender de #1/#2 para ser testeable.

## 2. TOP-1 — `hotfix-internalclient-socket-reset` (F-CRASH1)

| Campo | Valor |
|---|---|
| Root cause | `InternalClient::enviarSolicitud()` siempre llama `connectToServer()` (`src/middleware/src/client/internalclient.cpp:27`) pero NUNCA desconecta (cero llamadas disconnectFromServer/abort/close en el archivo); el server retiene sockets abiertos (`internalserver.cpp:83-92`). Tras el 1er intercambio el socket queda ConnectedState; el health-timer de 5s (`gestor_proceso_backend.hpp:106`, `gestor_proceso_backend.cpp:120,179-182`) re-dispara sobre ese estado → warning `setServerName called while not in unconnected state` → retry-storm infinito. Teardown post-login encadena reintento → SIGSEGV 139 ×2 firma idéntica. |
| Traza vinculada | `evidencia/crash-run1.log` ≡ `crash-run2.log` (137 líneas byte-idénticas); `evidencia/crash-static-analysis.md` §2; SIGSEGV: `evidencia/ui-xvfb-postlogin-run{2,3}.log` |
| Fix sketch | En `enviarSolicitud()`: antes de `setServerName/connectToServer`, si `state() != UnconnectedState` → `disconnectFromServer()` (o `abort()`); tras leer respuesta con éxito o error, desconectar explícitamente. Re-ejecutar crash repro ×2 offscreen y run Xvfb post-login para verificar desaparición de storm y segfault. |
| Effort | **S** — un archivo, lógica local, sin cambios de protocolo |
| Blast radius | `src/middleware/src/client/internalclient.cpp` únicamente; protocolo y server intactos |
| Desbloquea | arranque integrado completo; >1 op por proceso cliente; fiabilidad de writes desde vistas (hoy la 2ª op falla); elimina sospechoso primario del SIGSEGV de teardown |
| Verificación posterior | re-correr Phases 4+5 completas como regresión |

## 3. TOP-2 — `hotfix-flujo-postlogin` (MainWindow inalcanzable + F-UI1)

| Campo | Valor |
|---|---|
| Root cause | (a) Modo integrado: `ejecutarAplicacionFrontendImpl` retorna 0 justo tras aceptar login (`src/frontend/src/aplicacion_frontend.cpp:23-27`) — MainWindow jamás se instancia [static-evidence]. (b) Ruta standalone: entrypoint sí instancia MainWindow (`src/frontend/src/main.cpp:14-15`) pero el target FRONTEND_STANDALONE no compila: rama CMake (:42-60) omite include/link de middleware mientras 3 TUs hacen `#include <middleware/internalclient.h>` (`teacher_list_widget.cpp:3`, `main_window.cpp:18`). |
| Traza vinculada | compile: `evidencia/ui-F-UI1-standalone-build-failure.md` + `ui-standalone-build.log`; inalcanzabilidad dinámica: `evidencia/ui-probe-verdicts.md` ("MainWindow NEVER appears any run") + D1 §5 pasos 9-10 |
| Fix sketch | (a) en `aplicacion_frontend.cpp`: instanciar y mostrar `MainWindow` tras login aceptado en lugar de `return 0`. (b) en `src/frontend/CMakeLists.txt` rama standalone: `target_include_directories(... src/middleware/include)` + `target_link_libraries(... middleware)`. |
| Effort | **S** (dos sub-fixes pequeños e independientes) |
| Blast radius | `src/frontend/src/aplicacion_frontend.cpp` + `src/frontend/CMakeLists.txt`; sin cambios en vistas ni middleware |
| Desbloquea | TODO flujo GUI E2E: es el prerrequisito de las 3 listas, dashboard y formularios como producto usable; habilita además probar standalone como binario de desarrollo |
| Nota | No confunde con el puente (TOP-deferred): con este fix la app abre ventanas reales aunque los datos sigan siendo stub/vacíos |

## 4. TOP-3 — `hotfix-registro-fuentes-backend` (F-BUILD2)

| Campo | Valor |
|---|---|
| Root cause | Las implementaciones existen en árbol pero faltan en las fuentes de `src/backend/CMakeLists.txt`: `src/backend/src/data/horario_salida.cpp`, `src/backend/src/services/ServicioConsultaHorario.cpp`, `src/backend/src/services/ServicioHorarioSalida.cpp`. Prueba: `nm -C libbackend.a` → 0 símbolos T de esos métodos (solo 18 W implícitos). Por eso los 3 tests estaban huérfanos: registrarlos (nuestro único edit) expuso el link roto. |
| Traza vinculada | `evidencia/build-errors.log` (5 edges) + `evidencia/build-baseline-statement.txt` (proof nm) |
| Fix sketch | Añadir los 3 .cpp a la lista de sources de libbackend en `src/backend/CMakeLists.txt`; rebuild → ctest -N sigue en 23 y los 3 pasan de "Not Run" a ejecutados (~52 funciones). |
| Effort | **S** — 3 líneas |
| Blast radius | 1 archivo build-system; ningún código de producto |
| Desbloquea | 3 suites / ≈52 funciones de test hoy excluidas — directamente "para que el testing pueda continuar" |
| Sinergia | Mismo patrón de causa que deja ver F-BUILD1 (tests GTest con API vieja); conviene revisar ambos en la misma pasada de CI |

## 5. Descartados del top-3 (con justificación)

| Hallazgo | Por qué NO es hotfix ahora | Follow-up propuesto |
|---|---|---|
| Puente middleware→backend (echo-stub, 15 rutas, listas vacías) | Effort **L**: requiere enlazar middleware con backend, rutear handlers a DatabaseManager/Servicio\*, añadir fetch `*_list` en 3 vistas y poblar tablas. Es una feature, no un parche; además su testing depende de TOP-1/TOP-2. Un solo change futuro desbloquea Teacher/Classroom/SubjectList (blast radius compartido, ver D4). | `feat-puente-middleware-backend` |
| F-SOLVER1 (exito=false determinista ×4 funciones) | Defecto real de producto pero aislado al dominio solver: no bloquea ninguna otra capa (16/23 suites verdes). Necesita diagnóstico CP-SAT previo (traza interna de restricciones) → esfuerzo M/L incierto. | `investigacion-solver-cpsat-exito-falso` |
| F-BUILD1 (drift API GTest ×2 suites) | Baja prioridad: afecta solo a esas suites; actualizarlas puede chocar con cambios de dominio aún no decididos (dependen del resultado de la investigación solver para variables_cp_sat). | `mant-gtest-api-drift` |
| F-INFRA1 (probe OR-Tools del verify script) | No afecta runtime ni testing del producto; entorno demostrado sano por configure/build/probes directos. | `fix-verify-dev-env-tools-probe` |
| W-BASELINE (1 warning unused var) | Trivial; candidata a incluirse en cualquier PR que toque `solver_config.cpp`. | inline |

## 6. Orden de ejecución recomendado

```
1. hotfix-internalclient-socket-reset   (desbloquea runtime integrado)
2. hotfix-flujo-postlogin               (desbloquea GUI e2e; verificable ya con stubs)
3. hotfix-registro-fuentes-backend      (desbloquea cobertura unitaria huérfana)
4. feat-puente-middleware-backend       (change propio; datos reales en UI)
5. investigacion-solver-cpsat           (tras 1-4, sin dependencia dura)
```

Pasos 1-3 son independientes entre sí (archivos disjuntos) y podrían paralelizarse; el orden dado
maximiza señal de verificación temprana (cada uno re-habilita una sonda concreta de esta auditoría).
