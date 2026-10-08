# Phase 6 (R6/S4b) — Per-view UI probe verdicts @ develop e3a7a73

## Probe vehicles actually used
1. **Standalone binary**: DOES NOT BUILD → F-UI1 (see ui-F-UI1-standalone-build-failure.md).
2. **Fallback widget-level offscreen probe** (sanctioned, zero code edits): harness
   `evidencia/probe-main.cpp` compiled against prebuilt `/workspace/build/src/frontend/libfrontend.a`
   + `libmiddleware.a` + Qt6 6.4.2, run with `QT_QPA_PLATFORM=offscreen`, exit 0.
   Transcript: ui-probe-run.log; renders: ui-login-dialog.png + ui-view0..view5-*.png +
   ui-dialog-{teacher,classroom,subject}-form.png. Login exercised through the REAL
   QPushButton click path (onLoginClicked fake-auth, logindialog.cpp:298-318) — accepted=true.
3. **Xvfb :99 + xdotool against the INTEGRATED binary**:
   - attempt1 (keys typed without WM focus): creds never reached fields; dialog stayed open,
     process alive — preserved verbatim: ui-xvfb-login.xwd, ui-xvfb-postlogin-attempt1.xwd.
   - runs 2 & 3 (explicit windowfocus): login ACCEPTED ("Login exitoso", log line 9 both runs)
     then BOTH SEGFAULTED exit 139 with identical teardown signature
     ("backend COLAPSó con código 15" → "reintento 1 de 3 en 2000 ms" →
     "QObject::startTimer: Timers can only be used with threads started with QThread").
     Deterministic-reproduced ×2: ui-xvfb-postlogin-run2.log, ui-xvfb-postlogin-run3.log,
     screenshot post-crash desktop: ui-xvfb-postlogin.xwd. MainWindow NEVER appears in any run.

## Verdict table (D4 row schema)

| View | Render | Data source | Blocking issue | Evidence | Verdict |
|---|---|---|---|---|---|
| LoginDialog | OK (1280x800 PNG + Xvfb xwd) | none (fake-auth any non-empty creds) | integrated flow exits/segfaults right after accept; no WM focus under raw Xvfb lost typing (attempt1) | ui-login-dialog.png; ui-probe-run.log LOGIN accepted=true; run2/run3 logs | FUNCTIONAL |
| MainWindow | OK in isolation (1280x720, sidebar 6 btns, stack idx 0..5 switch via invokeMethod all ok=true) | n/a container view | UNREACHABLE for users: integrated exits(139) after login [run2/run3 dynamic]; standalone entry does not compile [F-UI1] | ui-view*.png series; SIDEBAR_BUTTONS inventory in ui-probe-run.log; F-UI1 doc | BROKEN |
| DashboardWidget | OK (view0 PNG) | hardcoded mock (stats "124"/"36"/"58" dashboard_widget.cpp:152-154; notifications :284+; conflictos :371+) | zero data source; misleading sample metrics | ui-view0-dashboard-inicio.png; grep refs | PLACEHOLDER |
| TeacherListWidget | OK shell (view1 PNG); main table 0x6 EMPTY | read: none (no teacher_list op call site anywhere); write: echo-stub via client; availability grid = hardcoded 12x5 sample (teacher_list_widget.cpp:104-127) | list can never populate; writes unreliable past 1st op (F-CRASH1 socket-state bug); backend stubs don't persist anyway | TABLE[0] rows=0 cols=6 + TABLE[1] rows=12 cols=5 in ui-probe-run.log; crash-static-analysis.md §4 | BROKEN |
| ClassroomListWidget | OK shell (view2 PNG); table 0x5 EMPTY | same as teacher (no classroom_list call site) | same as teacher | TABLE[2] rows=0 cols=5; classroom_list_widget.cpp:182/196/204 write-only wiring | BROKEN |
| SubjectListWidget | OK shell (view3 PNG); table 0x3 EMPTY | same pattern (no subject_list call site) | same as teacher | TABLE[3] rows=0 cols=3; subject_list_widget.cpp:202/213/221 write-only wiring | BROKEN |
| ViewPlaceholder "Generación de Horarios" | OK title screen (view4 PNG) | none | placeholder by design | ui-view4-generacion.png; main_window.cpp:146 | PLACEHOLDER |
| ViewPlaceholder "Visualización de Horarios" | OK title screen (view5 PNG) | none | placeholder by design | ui-view5-visualizacion.png; main_window.cpp:149 | PLACEHOLDER |
| TeacherFormDialog | OK (450x350 PNG, constructs+shows) | echo-stub target (teacher_create/update/delete ops exist server-side but only echo) | submit cannot persist (stub + F-CRASH1 second-op failure) | ui-dialog-teacher-form.png; ipc-probe-pairs.txt teacher_* echoes | FUNCTIONAL |
| ClassroomFormDialog | OK (400x277 PNG) | echo-stub target | same persistence gap | ui-dialog-classroom-form.png; classroom_create echo pair | FUNCTIONAL |
| SubjectFormDialog | OK (400x200 PNG) | echo-stub target | same persistence gap | ui-dialog-subject-form.png; subject_create echo pair | FUNCTIONAL |

Counts: FUNCTIONAL 5 · BROKEN 4 · PLACEHOLDER 3 · UNTESTABLE-headless 0.

## Decision signals for BROKEN rows (effort / blast radius of the middleware→backend bridge)

Common bridge work first: InternalClient socket reset between requests (fixes F-CRASH1 retry storm
AND makes >1 op per connection usable), add *_list fetch on view show + wire respuestaRecibida to
table population, and make InternalServer handlers call DatabaseManager/Servicio* instead of echo
(middleware deliberately does NOT link backend today — that link is the core effort).

| Row | Effort | Blast radius if bridge wired |
|---|---|---|
| MainWindow unreachable | S | src/app/src/aplicacion_frontend.cpp (+src/frontend/src/aplicacion_frontend.cpp): instantiate/show MainWindow post-login instead of return 0; plus F-UI1 CMake fix for standalone route |
| TeacherListWidget | M | internalclient.cpp (reset), teacher_list_widget.cpp (fetch+populate), internalserver.cpp teacher_* handlers, app layer link middleware→backend |
| ClassroomListWidget | M | same set for aulas |
| SubjectListWidget | M | same set for materias |

Decision rule (per D4 contract): workaround exists → keep testing; queue hotfix when row blocks ≥2
downstream flows or effort=S. MainWindow-unreachable is effort=S AND blocks every downstream UI
flow → prime hotfix candidate. List views share one root cause (no fetch path + no persistence);
one bridge change unblocks all three → single follow-up change recommended.

## Headless limitations (mandatory statement)

- Probes ran offscreen/Xvfb: visual fidelity (fonts, emoji glyphs, drop shadows, colors) NOT
  asserted; screenshots prove structure/layout presence, not pixel-correct styling.
- No real window manager on Xvfb: windowactivate unsupported; keyboard input needed explicit
  windowfocus (attempt1 shows silent key loss). Coordinate-based clicking avoided entirely.
- Per-widget unit probes (QTest interaction inside views) were not possible within the
  diagnose-only edit sanction; behavior beyond construct/show/click-path/inventory is inferred
  from static reading (cited file:line).
- Form dialog SUBMIT paths were not exercised end-to-end (would emit IPC ops toward echo stubs;
  covered instead by Phase-4 direct protocol probes).
