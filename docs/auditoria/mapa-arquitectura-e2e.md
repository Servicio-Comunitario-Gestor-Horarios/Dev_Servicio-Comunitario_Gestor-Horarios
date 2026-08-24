# D1 — Mapa de arquitectura as-is + flujo E2E

Change: `qa-audit` | Pin: `develop @ e3a7a737f9aa19a6a4d33cfed1cbb79a16eeda54`
Naturaleza: documento de diagnóstico (evidencia-only, cero edición de producto).
Toda afirmación factual lleva cita a `evidencia/` o `file:line`.

---

## 1. Capas del sistema (as-is)

Binario único `gestor-horarios` con dos modos; 4 módulos + `common`:

| Módulo | Contenido | Depende de | Estado observado |
|---|---|---|---|
| `src/app` | Punto de entrada dual (`main.cpp`), gestor de ciclo de vida del backend (`GestorProcesoBackend`), delegados frontend/backend | frontend, middleware, backend | Compila y arranca; flujo integrado roto post-login (ver §5) |
| `src/frontend` | Qt Widgets: LoginDialog, MainWindow, vistas de lista/formulario, dashboard | middleware (solo cabeceras `internalclient.h`) | Compila en integrado; standalone NO compila (F-UI1); listas nunca se pueblan (L4) |
| `src/middleware` | Cliente/servidor IPC sobre QLocalSocket (UDS) | Qt6 Network (NO enlaza backend — por diseño actual) | Servidor vivo responde; handlers son echo-stub (L2); cliente con bug de estado de socket (F-CRASH1) |
| `src/backend` | DatabaseManager, servicios (Servicio*), solver CP-SAT (OR-Tools 9.15), data | Qt6 Sql, OR-Tools | 16 suites pasan; solver falla determinista (F-SOLVER1); 3 fuentes huérfanas sin compilar (F-BUILD2) |
| `src/common` | Tipos compartidos (Resultado<T>, entidades) | — | Estable; drift de API detectado solo en tests GTest antiguos (F-BUILD1) |

Acoplamiento real verificado: **middleware NO enlaza backend hoy**; `DatabaseManager` y `Solver` no tienen llamadores en producción (spec Non-Goals / design AD5). El puente middleware→backend no existe.

## 2. Binario dual-mode (`src/app/main.cpp:28-48`)

| Invocación | Rama | App object | Flujo |
|---|---|---|---|
| `--backend` | `main.cpp:31-33` | `QCoreApplication` (:32) | `ejecutarAplicacionBackend()` → InternalServer headless |
| default | `main.cpp:46-47` | `QApplication` (:46) | `ejecutarAplicacionFrontend()` → lanza hijo `--backend` vía `GestorProcesoBackend` + UI login |

Punto crítico: la rama default crea `QApplication` en `main.cpp:46` y `ejecutarAplicacionFrontendImpl` crea una SEGUNDA `QApplication` en `src/frontend/src/aplicacion_frontend.cpp:20`. Empíricamente Qt 6.4.2 lo tolera en silencio (no qFatal) — ver §5.

## 3. Frontera IPC

| Aspecto | Valor | Evidencia |
|---|---|---|
| Transporte | Unix Domain Socket, nombre resuelto `/tmp/GestorHorarios_Middleware` | `evidencia/ipc-backend.log` |
| Protocolo request | JSON compacto `{"op", "payload"}` | `evidencia/ipc-probe-pairs.txt`; wire verificado vivo |
| Protocolo response | JSON compacto `{"status", "code", "data"}` | idem |
| Serialización | `internalserver.cpp:106` (fromJson), `:193` (write compacto); `internalclient.cpp:40` | lectura estática (design AD1) |
| Handlers actuales | echo-stub: devuelven el payload recibido, `code=0`, SIN persistencia ni acceso a DB | `evidencia/ipc-probe-pairs.txt` (14/14 probes) |
| Errores | `code=-4` para op desconocida / JSON malformado / campo faltante | `evidencia/ipc-probe-pairs.txt:45-56` |

## 4. Sondas: dónde se engancharon

| Stream | Vehículo | Punto de enganche | Evidencia |
|---|---|---|---|
| S1 build/warnings | cmake+ninja fresco en contenedor | nivel build system | `evidencia/configure.log`, `build-full.log`, `build-warnings.log`, `build-baseline-statement.txt` |
| S2 test matrix | ctest serial sobre binarios QTest de `test/` (enlazan libbackend/libmiddleware) | unit/regression layer | `evidencia/ctest.log`, `ctest-summary.md` |
| S3 IPC live | `socat - UNIX-CONNECT:/tmp/GestorHorarios_Middleware` contra el binario REAL `--backend` | frontera UDS (§3) | `evidencia/ipc-probe-pairs.txt`, `ipc-backend.log`, `ipc-db-isolation-proof.txt` |
| S4a crash repro | `QT_QPA_PLATFORM=offscreen ./build/src/app/gestor-horarios` ×2 | proceso integrado completo | `evidencia/crash-run1.log`, `crash-run2.log`, `crash-diff.txt`, `crash-static-analysis.md` |
| S4b UI | Xvfb :99 + xdotool sobre binario integrado + harness `probe-main.cpp` enlazado a libfrontend.a+libmiddleware.a (cero edits al repo) | widgets individuales offscreen | `evidencia/ui-probe-run.log`, `ui-xvfb-postlogin-run{2,3}.log`, PNGs `ui-view*.png`, `ui-probe-verdicts.md` |

## 5. Secuencia de arranque integrado — narrativa CORREGIDA por evidencia

> La spec/design asumían qFatal por doble QApplication. La reproducción empírica REFUTÓ esa hipótesis
> (`evidencia/crash-diff.txt`: TRACES_IDENTICAL=yes, 0 líneas fatal/abort en ambos runs). Esta es la
> secuencia REAL observada:

```
1. main.cpp:46        QApplication creada (1ª)
2. app/aplicacion_frontend.cpp:18-19   GestorProcesoBackend::iniciar()
                      → lanza hijo `gestor-horarios --backend`
3. gestor_proceso_backend.cpp:120      primer health_check…
4. internalclient.cpp:27               enviarSolicitud() hace connectToServer()
   ✓ primer health_check OK ("Cliente: Respuesta recibida — ÉXITO")
5. PERO InternalClient NUNCA llama disconnectFromServer() (verificado por grep:
   cero ocurrencias en internalclient.cpp) y el server retiene sockets abiertos
   (internalserver.cpp:83-92) → el QLocalSocket queda ConnectedState para siempre
6. gestor_proceso_backend.hpp:106      INTERVALO_VERIFICACION_MS=5000 → timer periódico;
   cada tick re-emite health_check sobre el MISMO socket conectado
   → "QLocalSocket::setServerName() called while not in unconnected state"
   → errorOccurred → ruta de reintento (gestor_proceso_backend.cpp:123-125)
   → TORMENTA DE REINTENTOS infinita (128 de 137 líneas del log son este loop)
7. frontend/aplicacion_frontend.cpp:20  QApplication (2ª) construida EN SILENCIO
8. :22-23              LoginDialog.exec() modal bloquea; offscreen no recibe input
                       → el proceso queda COLGADO hasta kill (exit 124 ×2, trazas byte-idénticas)
9. Con input real (Xvfb+xdotool): login aceptado → teardown (gestor.detener())
   → "COLAPSó con código 15" → "reintento 1 de 3" → warning startTimer/QThread
   → SIGSEGV 139 con firma idéntica ×2  (ui-xvfb-postlogin-run{2,3}.log)
10. MainWindow JAMÁS aparece: aplicacion_frontend.cpp:23-27 retorna tras login;
    MainWindow solo se instancia en src/frontend/src/main.cpp:14-15, que pertenece
    al target standalone que NO COMPILA (F-UI1)   [static-evidence]
```

Etiqueta: pasos 1-8 deterministic-reproduced ×2 (`crash-run1.log` ≡ `crash-run2.log`); paso 9 deterministic-reproduced ×2 (`ui-xvfb-postlogin-run{2,3}.log`); paso 10 static-evidence (inalcanzable en runtime integrado).

## 6. Cadena E2E rota — los 4 eslabones

| # | Eslabón roto | Root cause (file:line) | Hallazgo |
|---|---|---|---|
| L1 | Arranque integrado se cuelga; teardown post-login SIGSEGV ×2 | Sin reset de socket en InternalClient (`internalclient.cpp:27`; cero disconnect) + health-timer 5s (`gestor_proceso_backend.hpp:106`) re-dispara sobre socket conectado | **F-CRASH1** |
| L2 | Escrituras/lecturas IPC no persisten; 15 rutas nunca cableadas al dominio | Middleware es echo-stub y deliberadamente NO enlaza backend; DatabaseManager/Servicio\*/Solver sin llamadores en producción | puente ausente (feature-scale) |
| L3 | MainWindow inalcanzable para el usuario | Integrado retorna tras login (`aplicacion_frontend.cpp:23-27`); standalone entrypoint no compila por includes middleware ausentes (`src/frontend/CMakeLists.txt:42-60` vs `teacher_list_widget.cpp:3`, `main_window.cpp:18`) | **F-UI1** + gap post-login [static-evidence] |
| L4 | Las 3 listas nunca se pueblan | Cero call-sites de `*_list` en todo `src/frontend/` (grep-verificado): las vistas solo envían create/update/delete (`teacher_list_widget.cpp:202/217/225`, `classroom_list_widget.cpp:182/196/204`, `subject_list_widget.cpp:202/213/221`); además cada 2º uso del cliente compartido chocaría con F-CRASH1 | parte del puente (L2) + F-CRASH1 |

## 7. Resumen del ledger de hallazgos

| ID | Descripción | Severidad | Evidencia primaria |
|---|---|---|---|
| F-CRASH1 | Hang determinista en arranque + SIGSEGV teardown ×2 (retry-storm por socket sin reset) | MAJOR | `crash-static-analysis.md`, `crash-diff.txt` |
| F-SOLVER1 | Solver devuelve `resultado.exito=false` determinista en 2 suites (4 funciones) | MAJOR | `ctest-retry-failed-twice.log` |
| F-UI1 | FRONTEND_STANDALONE falla en COMPILE (includes middleware ausentes) | MAJOR | `ui-F-UI1-standalone-build-failure.md` |
| F-BUILD2 | 3 implementaciones backend excluidas de `src/backend/CMakeLists.txt` → suites huérfanas sin enlazar | MAJOR (testing) | `build-baseline-statement.txt` |
| F-BUILD1 | Drift API GTest en 2 suites preexistentes (Resultado<QString>, ProfesorSolverConfig) | MINOR | `build-errors.log` |
| W-BASELINE | Zero-warning REFUTADO: exactamente 1 warning (`solver_config.cpp:486` unused `totalSlots`) | INFO | `build-baseline-statement.txt`, `build-warnings.log` |
| F-INFRA1 | Probe OR-Tools de `verify-dev-env.sh` roto en origen (falta arg -DMODE) | MINOR | Batch A log; superado por probe directo de librería |

Detalle completo por suite/probe: **D2** (`matriz-pruebas.md`). Plan de remediación priorizado: **D3** (`plan-hotfix.md`).
