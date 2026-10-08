# Phase 5 (R5/S4a) — Crash reproduction: expected vs actual, and post-login static analysis

Pin: develop @ e3a7a73. Binary: /workspace/build/src/app/gestor-horarios (integrated).
Platform: gestor-dev container, QT_QPA_PLATFORM=offscreen.

## 1. Expected per spec R5 (double-QApplication qFatal) — EMPIRICALLY REFUTED

Spec expectation (from proposal/design): `QApplication app` at src/app/main.cpp:46 followed by a
second `QApplication app` at src/frontend/src/aplicacion_frontend.cpp:20 would abort with a qFatal
trace. Executed twice offscreen:

```
cmd: QT_QPA_PLATFORM=offscreen timeout 25 /workspace/build/src/app/gestor-horarios
run1 exit code: 124 (timeout killed it — process still alive at 25s)
run2 exit code: 124 (identical)
diff crash-run1.log crash-run2.log -> TRACES_IDENTICAL=yes (byte-for-byte, 137 lines each)
grep -ci "fatal|abort" both logs -> 0 / 0
```

Evidence: evidencia/crash-run1.log, evidencia/crash-run2.log, evidencia/crash-diff.txt

CONCLUSION: Qt 6.4.2 does NOT abort on double QApplication construction in this binary.
The second QApplication is constructed silently (undefined behavior per Qt docs but non-fatal
here). The startup failure mode is NOT a crash — it is a DETERMINISTIC HANG.

## 2. Actual deterministic defect — F-CRASH1 (NEW MAJOR FINDING)

Verbatim behavior chain (both runs identical, see logs):

```
GestorProcesoBackend: lanzando ".../gestor-horarios" QList("--backend")
GestorProcesoBackend: proceso iniciado, verificando salud...
Cliente: Conectando al servidor IPC para operación: "health_check"
Cliente: Conectado al servidor. Enviando solicitud...
Cliente: Respuesta recibida — ÉXITO            <- FIRST health_check SUCCEEDS
GestorProcesoBackend: backend listo (verificación OK)
Frontend: backend listo
Cliente: Conectando al servidor IPC para operación: "health_check"
QLocalSocket::setServerName() called while not in unconnected state
Cliente: Error de conexión: "QLocalSocket::connectToserver: Operation not permitted when socket is in this state"
GestorProcesoBackend: verificación falló, reintentando...
<... loop repeats every ~5s until kill; 128 of 137 log lines are this loop ...>
```

Root cause (static, file:line):
- src/middleware/src/client/internalclient.cpp:27 — `enviarSolicitud()` always calls
  `connectToServer()` but the class NEVER calls `disconnectFromServer()`/`abort()`/`close()`
  after a completed exchange (verified: no such call anywhere in internalclient.cpp).
- The InternalServer side also never closes client sockets (internalserver.cpp:83-92 keeps them),
  so after exchange #1 the QLocalSocket stays ConnectedState forever.
- src/app/src/gestor_proceso_backend.cpp:120 — first OK starts a 5s periodic timer
  (`INTERVALO_VERIFICACION_MS = 5000`, gestor_proceso_backend.hpp:106).
- gestor_proceso_backend.cpp:179-182 — each tick re-issues sendHealthCheck() on the SAME
  still-connected socket → Qt warning + errorOccurred → failure path at :123-125 ALSO schedules
  retries → unbounded retry storm. Every attempt fails identically because the socket state is
  never reset.
- Meanwhile the UI thread is blocked inside the modal `LoginDialog.exec()` offscreen
  (aplicacion_frontend.cpp:22), which can never receive input headlessly → integrated app hangs.

Determinism label justification: identical exit condition (still-running at timeout) +
byte-identical stderr/stdout traces across two independent runs → deterministic-reproduced ×2.

## 3. Post-login-no-window — STATIC-EVIDENCE ONLY (no runtime claim)

Integrated mode has NO runtime path to any main window:

- src/frontend/src/aplicacion_frontend.cpp:18-29 (`ejecutarAplicacionFrontendImpl`): constructs
  QApplication (:20), shows modal LoginDialog (:22-23); if accepted simply `return 0` (:23-27).
  No MainWindow is ever instantiated on this path.
- MainWindow is instantiated ONLY in the standalone entry point:
  src/frontend/src/main.cpp:14-15 (`MainWindow w; w.show();`) — compiled solely under
  FRONTEND_STANDALONE (see Phase 6).

Label: static-evidence (code reading only; not runtime-reproduced — consistent with R5 rule that
post-login-no-window is unreachable at runtime in integrated mode).

## 4. Corollary for D1/D3 (bridge gap)

InternalClient's public API surface (internalclient.h:31-61) exposes ONLY `sendHealthCheck()` and
generic `enviarSolicitud(op, payload)`. CORRECTED after grep verification (initial draft claimed
zero frontend call sites — WRONG): the three list views DO wire InternalClient and DO send
create/update/delete ops:
- src/frontend/src/views/main_window.cpp:45-48 creates one InternalClient and passes it to
  Teacher/Classroom/Subject list widgets via setClient().
- teacher_list_widget.cpp:202/217/225, classroom_list_widget.cpp:182/196/204,
  subject_list_widget.cpp:202/213/221 call enviarSolicitud with OP_CREAR_/OP_ACTUALIZAR_/
  OP_ELIMINAR_* payloads.
HOWEVER zero call sites exist anywhere in src/frontend/ for OP_LISTA_PROFESORES / OP_LISTA_AULAS /
OP_LISTA_MATERIAS (teacher_list/classroom_list/subject_list) — verified by grep over all
frontend sources. Lists can therefore never populate: every view that needs data has no fetch
path. This is the precise form of the D1 broken link "views never send *_list ops". Additionally,
each create/update/delete issued through the shared client would hit the F-CRASH1 socket-state bug
on its SECOND use (no disconnect between requests), so even write ops are unreliable at runtime.
