# D4 — Informe de estado del frontend (por vista)

Change: `qa-audit` | Pin: `develop @ e3a7a73` | Informe solicitado por el usuario.
Barra contractual: un maintainer debe poder decidir fix/no-fix por vista SOLO leyendo este informe,
sin re-ejecutar ninguna prueba.

---

## 1. Resumen ejecutivo

| Veredicto | Nº | Filas |
|---|---|---|
| **FUNCTIONAL** | **5** | LoginDialog, MainWindow en render aislado, TeacherFormDialog, ClassroomFormDialog, SubjectFormDialog |
| **BROKEN** | **4** | MainWindow como ruta de usuario (inalcanzable), TeacherListWidget, ClassroomListWidget, SubjectListWidget (nunca se pueblan) |
| **PLACEHOLDER** | **3** | DashboardWidget (métricas hardcodeadas), ViewPlaceholder "Generación", ViewPlaceholder "Visualización" |
| **UNTESTABLE-headless** | **0** | — |

**Lectura en una línea**: el login funciona, los formularios renderizan y emiten operaciones IPC
reales, pero **ningún usuario puede llegar a la ventana principal** (el flujo integrado muere tras
login con SIGSEGV ×2; el standalone ni compila) y **las tres listas nunca mostrarán datos** (no
existe ruta de fetch y el backend es echo-stub).

Nota metodológica sobre las 12 filas: MainWindow se evalúa en DOS dimensiones porque la evidencia lo
exige — (i) capacidad de render/comportamiento interno: correcta en aislamiento (FUNCTIONAL);
(ii) alcanzabilidad por el usuario: cero rutas vivas (BROKEN). Ambas filas citan su evidencia.

## 2. Vehículos de prueba usados y su fiabilidad

| Vehículo | Resultado | Evidencia |
|---|---|---|
| Binario FRONTEND_STANDALONE | NO compila (fallo COMPILE, pre-link) → hallazgo F-UI1 | `evidencia/ui-F-UI1-standalone-build-failure.md`, `ui-standalone-{configure,build}.log` |
| Harness widget-level offscreen (`evidencia/probe-main.cpp` enlazado a libfrontend.a+libmiddleware.a; cero edits al repo) | exit 0: login aceptado por ruta REAL de click (`logindialog.cpp:298-318`); MainWindow 1280x720; 6 slots sidebar conmutan stack idx 0..5 vía invokeMethod (todo ok=true); inventario de tablas capturado | `evidencia/ui-probe-run.log`; PNGs `ui-login-dialog.png`, `ui-view{0..5}-*.png`, `ui-dialog-*-form.png` |
| Xvfb :99 + xdotool sobre binario INTEGRADO | attempt1: teclas perdidas sin WM (preservado); runs 2-3 idénticos: login ACEPTADO ("Login exitoso") luego SIGSEGV 139 ×2 firma idéntica ("COLAPSó código 15" → reintento → startTimer/QThread warning). MainWindow JAMÁS aparece | `evidencia/ui-xvfb-postlogin-run{2,3}.log`, `ui-xvfb-postlogin.xwd`; análisis: `crash-static-analysis.md` |

## 3. Matriz por vista (12 filas)

Esquema: `{vista | render | fuente de datos: real-service\|echo-stub\|placeholder\|none | bloqueo | effort | blast radius | evidencia | veredicto}`

| # | Vista / dimensión | Render | Fuente de datos | Bloqueo | Effort fix | Blast radius del fix | Evidencia | Veredicto |
|---|---|---|---|---|---|---|---|---|
| 1 | LoginDialog | OK (PNG + xwd) | none (fake-auth acepta credenciales no vacías, ~300ms) | ninguno propio; el flujo posterior muere, pero el diálogo cumple su función | — (no requiere) | n/a | `ui-login-dialog.png`; `ui-probe-run.log` LOGIN accepted=true | **FUNCTIONAL** |
| 2 | MainWindow — render aislado | OK (1280x720, sidebar 6 botones, stack idx 0..5 conmutan ok) | n/a (contenedor) | ninguno interno detectado en probes | — | n/a | `ui-view*.png`; SIDEBAR_BUTTONS inventory en `ui-probe-run.log` | **FUNCTIONAL** |
| 3 | MainWindow — alcanzabilidad usuario | n/a para usuario: JAMÁS aparece en ningún run | n/a | integrado retorna tras login [static: `src/frontend/src/aplicacion_frontend.cpp:23-27`] + SIGSEGV dinámico ×2; standalone entrypoint (`main.cpp:14-15`) no compila [F-UI1] | **S** | `aplicacion_frontend.cpp` (mostrar MainWindow post-login) + `src/frontend/CMakeLists.txt` rama standalone (include/link middleware) | `ui-xvfb-postlogin-run{2,3}.log`; `ui-F-UI1-standalone-build-failure.md`; D1 §5 pasos 9-10 | **BROKEN** |
| 4 | DashboardWidget ("Inicio") | OK (view0 PNG) | **placeholder** — stats hardcodeadas "124"/"36"/"58" (`dashboard_widget.cpp:152-154`; notificaciones :284+; conflictos :371+) | cero fuente de datos; métricas muestra engañosas | M (cuando exista puente) | puente middleware→backend + handlers dashboard-side | `ui-view0-dashboard-inicio.png`; refs grep | **PLACEHOLDER** |
| 5 | TeacherListWidget | OK shell (view1 PNG); tabla principal **0x6 VACÍA** | read: **none** (cero call-sites teacher_list, grep-verificado); write: echo-stub vía InternalClient; grid disponibilidad = muestra hardcodeada 12x5 (`teacher_list_widget.cpp:104-127`) | lista jamás se puebla; writes poco fiables desde la 2ª op (F-CRASH1); stubs no persisten | **M** | `internalclient.cpp` (reset), fetch+populate en vista, handlers teacher_* en server, link middleware→backend | TABLE[0] rows=0 cols=6 + TABLE[1] rows=12 cols=5 en `ui-probe-run.log`; call-sites write-only `teacher_list_widget.cpp:202/217/225`; `crash-static-analysis.md` §4 | **BROKEN** |
| 6 | ClassroomListWidget | OK shell (view2 PNG); tabla **0x5 VACÍA** | mismo patrón que teacher (sin classroom_list call-site) | ídem | **M** | mismo conjunto para aulas | TABLE[2] rows=0 cols=5; `classroom_list_widget.cpp:182/196/204` | **BROKEN** |
| 7 | SubjectListWidget | OK shell (view3 PNG); tabla **0x3 VACÍA** | mismo patrón (sin subject_list call-site) | ídem | **M** | mismo conjunto para materias | TABLE[3] rows=0 cols=3; `subject_list_widget.cpp:202/213/221` | **BROKEN** |
| 8 | ViewPlaceholder "Generación de Horarios" | OK pantalla título (view4 PNG) | none | placeholder por diseño | — | — | `ui-view4-generacion.png`; `main_window.cpp:146` | **PLACEHOLDER** |
| 9 | ViewPlaceholder "Visualización de Horarios" | OK pantalla título (view5 PNG) | none | placeholder por diseño | — | — | `ui-view5-visualizacion.png`; `main_window.cpp:149` | **PLACEHOLDER** |
| 10 | TeacherFormDialog | OK (450x350 PNG, construye y muestra) | **echo-stub target** (teacher_create/update/delete existen server-side pero solo eco) | submit no persiste (stub + fallo 2ª op por F-CRASH1) | depende del puente | handlers server + reset cliente | `ui-dialog-teacher-form.png`; pares teacher_* en `ipc-probe-pairs.txt` | **FUNCTIONAL** |
| 11 | ClassroomFormDialog | OK (400x277 PNG) | echo-stub target | mismo gap de persistencia | ídem | ídem | `ui-dialog-classroom-form.png`; par classroom_create | **FUNCTIONAL** |
| 12 | SubjectFormDialog | OK (400x200 PNG) | echo-stub target | mismo gap | ídem | ídem | `ui-dialog-subject-form.png`; par subject_create | **FUNCTIONAL** |

## 4. Señales de decisión fix/no-fix por fila

Regla contractual: *workaround existe → seguir testeando; encolar hotfix si bloquea ≥2 flujos downstream O effort=S*.

| Fila | ¿Workaround existe? | ¿Bloquea ≥2 flujos? | Decisión |
|---|---|---|---|
| #3 MainWindow inalcanzable | No (ninguna ruta alternativa llega a ventana principal) | Sí — TODOS los flujos GUI | **FIX YA** (effort S) → `hotfix-flujo-postlogin` (D3 TOP-2) |
| #5-7 listas vacías | Parcial (formularios emiten ops; datos solo visibles vía probes IPC directos) | Sí — 3 vistas comparten causa raíz | **NO parche**: change propio `feat-puente-middleware-backend` (effort L compartido, D3 §5); pre-requisito: F-CRASH1 |
| F-CRASH1 (subyace a #3, #5-7, #10-12) | No para flujos multi-op | Sí — arranque integrado + writes >1 op + teardown SIGSEGV | **FIX YA** (effort S) → `hotfix-internalclient-socket-reset` (D3 TOP-1) |
| #4, #8, #9 placeholders | Sí (por diseño) | No | NO FIX ahora; wiring cuando exista el puente |
| #10-12 formularios | Sí como widgets (renderizan, emiten ops reales) | No per-se | NO FIX propio; cubiertos por TOP-1 + puente |

Conclusión operativa: **con los 2 hotfixes de effort S** (socket-reset + post-login/standalone-CMake)
el frontend pasa de "cero flujos GUI alcanzables" a "app navegable con datos stub"; los datos reales
requieren el change de puente (effort L).

## 5. Limitaciones headless (declaración obligatoria)

- Las sondas corrieron offscreen/Xvfb: la fidelidad visual (fuentes, glifos, sombras, colores) NO
  está afirmada; los PNG/xwd prueban estructura y presencia de layout, no estilo pixel-correcto.
- Sin window manager real en Xvfb: `windowactivate` no disponible; el teclado requirió focus
  explícito (attempt1 demuestra pérdida silenciosa de keys — preservada como evidencia).
- Probes unitarios por-widget (interacción QTest dentro de las vistas) fueron imposibles dentro de
  la sanción diagnose-only (única edición permitida: `test/CMakeLists.txt`); todo comportamiento más
  allá de construir/mostrar/click-path/inventario se infiere de lectura estática citada file:line.
- Los SUBMIT de formularios no se ejercitaron end-to-end desde UI (emitirían ops IPC hacia stubs
  echo; comportamiento cubierto por probes directos de protocolo de Phase 4).
- Filas UNTESTABLE-headless: 0 — ningún check quedó sin vehículo de prueba.
