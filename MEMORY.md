# MEMORY.md — Estado del proyecto

> Plantilla de estado SDD. Se actualiza al cerrar cada fase.

## Fase actual

- **Fase:** spec **008 (datos de prueba) implementada** y **005 implementada con T11** (rectificación hora académica = bloque). **Visualización de horarios dirigida por datos** (rejilla, turnos, recesos y bloques de varias franjas según el motor). Suite **47/47** en verde.
- **Fecha:** 2026-10-17
- **Rama:** `sdd/005-nucleo-solver` (local, sin pushear; `develop` tiene la 004 mergeada)
- **Specs en curso:** `specs/005-nucleo-solver` (implementada; T10 abierta por pendientes) y `specs/008-datos-de-prueba` (implementada; APROBADA por el reviewer)
- **Siguiente paso:** resolver los **pendientes conocidos de la 005** (ver abajo), los menores de la 008 y abrir **PR** de `sdd/005-nucleo-solver` → `develop` (rama protegida: por PR, sin merge commits).

## Specs activas

| Spec | Estado | Fase |
|---|---|---|
| 001-base-datos-local | aprobada | **implementada** (T1–T8) |
| 002-generacion-horarios-ipc | aprobada | **implementada** (T1–T9) |
| 003-cierre-001-base-datos | aprobada | **implementada** (T1–T5: cierra los 5 huecos de la 001) |
| 004-persistencia-campos-curso | aprobada | **implementada** (T1–T9; suite 38/43) |
| 005-nucleo-solver | aprobada | **implementada** (T1–T9 + T11; T10 abierta por pendientes conocidos; suite 46/46) |
| 008-datos-de-prueba | aprobada | **implementada** (T1–T9; 57 materias, 4 planes, 13 cursos, 4 variantes) |

> Épico **solver** (la numeración sigue el orden de trabajo): **004 (persistencia de campos) → 005 (núcleo del solver) → 006 (turnos automáticos) → 007 (generación parcial)**. 006 y 007 reservadas, sin crear.

> La antigua spec `002-frontend-db-ipc-solver` se dividió en dos (001 + 002) por tamaño
> (superaba las 10 tareas) y se renumeró desde 001.

## Decisiones

- Convenciones iniciales en `AGENTS.md`: C++17 + Qt6 + OR-Tools, tests con `ctest --preset full`, textos en español.
- Constitución en `docs/constitution.md` (revisada; se mantiene la versión de 6 principios con detalle).
- Limpieza de repo (2026-10-08):
  - `.gitignore`: + editors/IDE (`.vscode/`, `.idea/`, `*.user`), tooling de agentes (`.agents/`, `.claude/`, `.atl/`, `.codegraph/`) y generados (`repomix-output.xml`, `Registro.txt`, `__pycache__/`).
  - Fuera del índice (ya no existen en disco): `.agents/` (39), `.vscode/` (2), `.claude/` (2), `.atl/` (2) y `compile_commands.json` (se regenera).
  - Borrado físico: `repomix-output.xml` (2,1 MB, regenerable). `Registro.txt` solo ignorado, se queda en disco.
  - `TESTS-SOLVER-CONTEXT.md` movido a `docs/informes/`.
  - Archivos SDD (`AGENTS.md`, `MEMORY.md`, `specs/`, `docs/constitution.md`) trackeados: van en el repo.
  - Dos commits separados: `e434ed8` (chore: gitignore + des-tracking + SDD) y `f92c063` (docs: mover TESTS-SOLVER-CONTEXT).
  - Verificación: `git check-ignore` en los 10 patrones ✓; rebuild limpio en `build-test/` y `ctest` 2/2 ✓ (Qt middleware). El preset `full` (OR-Tools) solo corre en Docker: no hay OR-Tools en el host.
- Specs 001/002 (2026-10-08), decisiones de clarificación relevantes para el plan:
  - El **lado cliente** (programa salvo el proceso de cálculo) es dueño de la base de datos; el solver se mantiene separado por IPC.
  - **La base solo persiste entidades de dominio** (docentes, aulas, asignaturas/materias, cursos, planes de estudio, turnos, recesos). La **configuración del solver, los presets y los horarios generados** se gestionan como **archivos JSON**, NO como BD.
  - **Respaldo obligatorio** (no opcional) antes de migrar o descartar la base; si no es posible, no se migra/descarta.
  - Migración **por versión de esquema** (solo si es anterior); versión ausente/ilegible o posterior → fallo de apertura. No hay bases previas (las primeras se crean tras esta implementación).
  - Timeout del proceso de cálculo: **60 s**; salud y apagado: 5 s.
  - Borrado **en cascada con confirmación**, atómico.
  - **`docs/constitution.md` §5 enmendada**: se quitaron "horarios guardados en SQLite" del listado de datos; presets/horarios se gestionan como archivos JSON.
  - Limpieza de restos de frontend: eliminados `src/frontend/src/main.cpp`, `src/frontend/src/aplicacion_frontend.cpp` y su `.hpp` huérfano, más sus entradas del CMake (rompían `cmake --preset full`); el ejecutable real vive en `src/app`.
- **Alcance (2026-10-09):** NO implementamos `src/frontend` (interfaz Qt: vistas, listas, diálogos e indicadores): lo hace **otro equipo**. Nuestros entregables: `src/backend`, `src/app` y `src/middleware`. Exponemos la lógica y el **contrato de consumo** documentado en `docs/interfaz-frontend.md`. Las specs 001/002 y sus planes/tareas se reajustaron (fuera las tareas de frontend; nueva tarea de documentar la interfaz).
- **Refactor de la base del cliente (2026-10-10):** se extrajo el ciclo de vida de la BD del arranque de la interfaz al módulo nuevo **`src/datos`** (`ContextoBaseDatos`), que abre/migra/respalda y construye el `NucleoDatos`. `src/app` lo crea y lo **inyecta** en `MainWindow` (`setNucleoDatos`); se eliminó el singleton global `nucleoDatosActivo()`. La UI no abre ni instancia la base (RF-2 intacto; el proceso de cálculo sigue sin tocarla). Documentado en `docs/interfaz-frontend.md` §2.7. Nuevo test `test_datos_contexto_base_datos`. Verificado en Docker: 30/33 (mismos 3 rojos conocidos) y `dev-frontend` compila.
- **Cierre de los huecos de la 001 (spec 003, 2026-10-10):** implementados T1–T5. T1: `Respaldo::descartarBaseYCrearNueva` (respaldo obligatorio antes de descartar la base) + `ResultadoDescarte`. T2: los `listar*` de los seis servicios de dominio y de `NucleoDatos` devuelven `Resultado<QVector<Dto>>` (error ≠ vacío), con call sites de tests actualizados. T3: `AperturaBaseDatos::Opciones::progreso` (callback por paso de migración). T4: el arranque conecta `InstanciaUnica::activarSolicitada()` al enfoque de `MainWindow`. T5: mensajes de respaldo/apertura en español con causa y acciones (sin texto crudo de Qt). Contrato actualizado en `docs/interfaz-frontend.md` §2.1/§2.3/§2.4/§2.6. Verificado en Docker: 30/33 (mismos 3 rojos ajenos).
- **Ordenamiento con acentos (2026-10-10):** los `listar*` de dominio ordenan con `QCollator` (`QLocale(es_ES)`, `CaseInsensitive`) vía el helper `backend/services/ordenacion.hpp`, en vez del orden binario de SQLite o `QString::localeAwareCompare` (que con locale POSIX degradaba a codepoint: «Á» tras «Z»). Sin dependencia nueva: `QCollator` ∈ Qt6Core (ya enlazado por el ejecutable; `ldd` sin cambios). `test_servicio_profesor` en verde, con el nuevo caso `listarProfesores_ordenConAcentosYMayusculas`.
- **Consola de datos (2026-10-10):** módulo `src/consola` → ejecutable `gestor-horarios-consola` (herramienta de desarrollo, `option(BUILD_CONSOLA)`, se compila con `BUILD_BACKEND=ON`). Reutiliza el contrato del frontend (`ContextoBaseDatos` + `NucleoDatos`); comandos `docentes/aulas/materias/planes/cursos/turnos`, `dependientes`, `cascada`, `pendientes`, `reintentar`, `estado`; modo REPL y `--cmd` no interactivo; `--db <ruta>`. Documentada en `docs/guias/consola-datos.md`. Sin dependencia nueva (solo enlaza `datos`).
- **Épico solver — comportamiento definido (2026-10-13):** se añadió la **§7 "El solver y su contrato"** a `docs/constitution.md` (y se enmendó: §7.2 con **peso por par** para suplentes; §7.4 con **V14** y **V15**). Decisiones de modelado aprobadas:
  - **Horas de aula del profesor = tope + exceso penalizado** (`PESO_HORAS`). Minimizar "déficit" es **inerte** (Σ déficit constante, porque las horas totales asignadas las fijan las horas de las materias-curso); por eso no se declara INFEASIBLE por horas.
  - **Nuevas invariantes duras:** un docente por materia-curso (D1/D2) y enlace clase↔aula (D9). **D12 eliminada** (redundante con D8 aula no solapada).
  - **Suplente = preferencia en el objetivo** (S2): titular antes que suplente; entre suplentes, mayor `peso` del par (docente, materia-suplente). Constantes del motor: `PESO_HORAS` y `coste_máximo_de_suplente`; el `peso` por par es dato de entrada.
  - **Contrato de configuración:** los cursos se identifican por **`anio`+`seccion`** (el nombre se **deriva**: `"1ro A"`, ordinal `{1:ro,2:do,3:ro,resto:to}`) y `aula_fija` pasa a **booleano + `aula` (índice)**; los docentes ganan **`anios`** (vacío = cualquiera). La BD `Cursos` sumará `anio`/`seccion` (**spec 004**). Validaciones **V13/V14/V15**.
  - La spec **005** supersede el contrato JSON de 002 (anotado en `specs/002-generacion-horarios-ipc/spec.md`). `docs/solver/` queda como **referencia histórica**.
  - **Años del docente: viven en el preset/config, NO en la BD** (no hay columna nueva en `docentes`). **No hay presets antiguos** → sin compatibilidad hacia atrás. La persistencia (spec 004) solo añade `anio`/`seccion` a `Cursos` y expone los campos en el dominio/DTO (el mapeo DTO→JSON es de la spec 005).
  - **Modo didáctico:** en el solver, **el usuario implementa**; el agente explica comandos/lógica y revisa. (En la implementación inicial implementó el usuario; en las correcciones de la validación se usó el subagente @implementer a petición expresa del usuario.)
  - Tests del solver que fallaban (39/41 en Docker entonces): quedaron **verdes** al cerrar la spec **005** (suite 44/44).

- **Hora académica = bloque (2026-10-17):** `bloques(horas) = horas` y `bloques_aula(horas_aula) = horas_aula`; `duracion_minutos` define **solo** la duración real de la franja. D3/S1/V14 sin conversión. Constitución §7 enmendada. Implementado en **005-T11**.
- **Seed de datos de prueba (spec 008):** `gestor-horarios-seed` + librería `seed`; **solo puebla la BD, no genera presets**. Plan 31059 adaptado a **40 h académicas/año** (4 planes GEN/EXA/NAT/SOC), 57 materias-año, 210 cargas, 13 cursos **sin turno**, 16 aulas, 2 turnos de 8 franjas de 40 min con receso, 4 variantes de docentes (65/43/64/65; demanda 520, Σ topes 461). Las 210 cargas y los docentes se insertan por **SQL directo** dentro de una transacción única (no `asignarCarga`). Guía: `docs/guias/datos-prueba.md`; suite 46/46.
- **Empaquetado portable (2026-10-18):** `cmake/Packaging.cmake` (incluido desde el `CMakeLists.txt` raíz) añade `install()` + CPack (`ZIP`) para distribuir el ejecutable con todas sus dependencias, **sin instalación ni administrador**. Las DLL y plugins de Qt los copia `windeployqt` (`--no-translations`, `--no-system-d3d-compiler`, `--no-system-dxc-compiler`, `--no-opengl-sw` y sin los drivers SQL distintos de SQLite, porque la interfaz solo usa Widgets/Network/Sql y solo `QSQLITE`); las de OR-Tools se copian desde `<ortools_DIR>/../../../bin`; el runtime de Visual C++ se instala **app-local** desde `VCToolsRedistDir` en vez de arrastrar `vc_redist.x64.exe`. ICU no se empaqueta: lo aporta Windows (`System32\icuuc.dll`). Se genera solo en Windows y solo en Release; en Docker se omite. Guía: `docs/guias/desarrollo/CMAKE_GUIDE.md` §Empaquetado. Verificado en el host: 44 archivos, 119,6 MB expandido / 38,1 MB el ZIP; `--version` y `--backend` (6 s) con el `PATH` limpio; sonda `QSQLITE` dentro del ZIP descomprimido; auditoría `dumpbin` sin dependencias sin resolver.

## Siguiente paso

1. **Pendientes conocidos de la 005** (ver abajo) → planificar en la próxima spec del épico (**006/007**) o en una nueva.
2. **Orden del épico solver:** 004 (persistencia) ✅ → 005 (núcleo + T11) ✅ → 006 (turnos automáticos) → 007 (generación parcial).
3. **Menores de la 008:** ignorar `.clangd`; confirmar la convención **1-based** de `despues_de_slot` en 004/005 (hoy asumida y verificada por las horas del receso).
4. **Abrir PR de `sdd/005-nucleo-solver` → `develop`** (rama protegida: por PR, sin merge commits). La 004 ya está mergeada y pusheada.
5. **Cableado de `ServicioGeneracion` con el cliente IPC real** (`PuertoSolver`): **implementado en el frontend** (2026-10-17, ver Progreso): `PuertoSolverIpc` sobre `InternalClient` + vista «Generación de horario». `docs/interfaz-frontend.md` §3.6.
6. **Distribución:** el **ZIP portable ya se genera** (`cmake --build build-rel --target package`, ver `docs/guias/desarrollo/CMAKE_GUIDE.md` §Empaquetado). Falta lo que venga después: un instalador (Inno Setup o NSIS) que envuelva ese ZIP, y firmar el binario con `signtool` para evitar el aviso de SmartScreen.

### Pendientes conocidos de la 005 (diferidos y documentados)

- **RF-13 — planes de los docentes:** `ConstructorEntradaSolver` embebe los planes de los **cursos** pero no los que usan los **docentes**. Sin impacto funcional hoy (el plan del docente se compara como cadena), pero es una cláusula explícita del RF.
- **RNF-4 — cobertura de tests por regla:** faltan casos rojo→verde de V2 (`peso` negativo, `duracion_minutos ≤ 0`), V7 (turno ausente), V8 (modo inactivo), V14, V15, D13 (`anios`), S1/S2 (exceso y titular a tope), reproducibilidad (RNF-1), aula de salida como índice (fija/móvil) y `peso` en el ida y vuelta. Por esto los «Hecho cuando» de T3/T5/T7/T8 no quedan cerrados y **T10 sigue abierta**.
- **Decisión I2 — avisos retirados:** `ValidadorSalidaSolver` sigue emitiendo los avisos de **capacidad de aula** y **horas no cubiertas**, y `test_validador_salida` los exige; la spec 005 los **retira hasta nuevo aviso**. Hay que alinear código/tests (tocando la spec si se cambia la decisión).
- **Residuales varios:** P1/P2 de `ValidadorSalidaSolver` comparan horas vs bloques; `metadata.total_asignaciones` cuenta variables (no asignaciones); `profesores_asignados`/`tiempo_ejecucion_ms` sin rellenar; `Solver` acumula estado si se reutiliza la instancia; `.clangd` sin ignorar; no se validan materias duplicadas ni de 0 horas en un plan.

## Progreso de implementación

- 001: **T1–T8 completas** ✅. Tests rojo→verde por tarea.
  - T1 esquema versionado (`VERSION_ESQUEMA_ACTUAL = 2`), T2 respaldo, T3 apertura con migración y
    respaldo obligatorio, T4 núcleo de datos (CRUD de dominios) y migración v2, T5 cascada atómica,
    T6 cambios pendientes y reintento, T7 instancia única y arranque del cliente, T8 documentación
    del contrato para el frontend (`docs/interfaz-frontend.md` §2) + `test_backend_contrato_interfaz`.
  - `docs/interfaz-frontend.md` documenta el contrato real (implementado vs planificado); el test de
    contrato fija `Resultado<T>`, `AperturaBaseDatos::Resultado::ok()/detalle/rutaRespaldo` y
    `estadoPendienteDe` (verde).
- 002: **T1 completa** ✅ (`CargadorConfiguracionSolver` + `SolverConfig::toJson`); `solver_config` movido al target `backend` (sin OR-Tools). Test `test_cargador_configuracion` (verde).
  - **T2 completa** ✅ (`ConstructorEntradaSolver`/`construirJsonEntrada` + `DatosDominio`). Decisión de contrato: el JSON de entrada sigue el `SolverConfig` **implementado** (12 secciones) + `meta` (extra que `fromJson` ignora); la BD aporta entidades y el preset los parámetros del solver (horas por docente casadas por nombre). Test `test_constructor_entrada`. Divergencia con `docs/solver/Motor-Solver-Plan-Completo.md` §3 anotada.
  - **T3 completa** ✅ (`ValidadorEntradaSolver`/`validarEntradaSolver`, envoltura de `SolverConfig::fromJson` con V1–V13). Test `test_validador_entrada` (un caso por regla).
  - **T4 completa** ✅ (`ValidadorSalidaSolver`/`analizarSalidaSolver` + `AnalisisSalida`): P1–P3 avisos, P4 solapamiento (profesor/aula/curso) ⇒ no presentable. Test `test_validador_salida`.
  - **T5 completa** ✅ (`ServicioGeneracion` + `PuertoSolver` inyectable, `huellaDatos`/`estamparFechaGeneracion` puras): estados Listo/DatosAnteriores/NoFactible/ContratoInvalido/CalculoFallido/TiempoAgotado/EntradaInvalida, timeout con `QTimer` e ignorado de respuestas tardías. `backend` con `AUTOMOC`. Test `test_servicio_generacion`.
  - **T6 completa** ✅ (`messages.h`: código `RESP_SIN_SOLUCION`, ops de negocio marcadas obsoletas; `InternalClient::intentosConexion()` para verificar reutilización de socket). Test `test_middleware_transport` extendido con ruta `solver_resolve` y «sin tormenta de reconexiones».
  - **T7 completa** ✅ (`servicio_solver`/`resolverEntradaSolver` en `backend_solver`; `aplicacion_backend` registra `solver_resolve`; `GestorProcesoBackend` sin reinicio automático). Test `test_ipc_aislamiento_bd` (no abre BD, rechaza ops de negocio).
  - **T8 completa** ✅ (contrato de `ServicioHorarioSalida` fijado con `test_archivos_horario`: round-trip JSON y fallo de escritura sin perder el contenido).
  - **T9 completa** ✅ (`docs/interfaz-frontend.md` §3 completa: `ServicioGeneracion`, `CargadorConfiguracionSolver`, `HorarioSalida`/archivos, `AnalisisSalida`, proceso `--backend`/salud) + `test_contrato_generacion`. **Spec 002 implementada.**
- 005: **T1–T9 completas** ✅ (T10 abierta por pendientes conocidos). Contrato nuevo (`aula_fija` global, curso por `anio`+`seccion`, planes embebidos, `anios`, centinela `-1`), funciones puras (`bloques`/`bloquesAula`/`claveCurso`/`materiasDelCurso`/elegibilidad), validación V1–V15, construcción de la entrada desde BD+preset, variables CP-SAT, restricciones duras (D1–D11 y D13; D12 eliminada), objetivo S1+S2 (`PESO_HORAS > COSTE_SUPLENTE`), extracción por identidad (`claveCurso`, aula = índice) y causas de no factibilidad por categorías (RF-6). Seller: `version="1.0"` al producir/guardar.
  - **Validación del reviewer (ronda 2): APROBADA.** Bloqueantes corregidos y commiteados: `37666a2` (D1/D3 siempre publicadas + V13 cruza disponibilidad×turno), `454a53b` (RF-10 versión no reconocida), `7a7ff1b` (RF-11 centinela `-1`, coherencia de horas, V9 estricto), `4171a4d` (RF-6 categorías de causa), `f063f39` (sello `version="1.0"`).
- Tests ajenos que fallan: **ninguno**. Queda un **flaky preexistente** (`test_middleware_transport`, ~1 de cada 6 ejecuciones, `QLocalSocket: Invalid name`), ajeno a la 005 y reproducible en HEAD. El ordenamiento con acentos de `test_servicio_profesor` quedó **resuelto** con `QCollator` (ver decisión).
- **Incidente y hotfix de apertura de base (2026-10-17):** una base creada con el DDL anterior de
  `Cursos` (`nombre`/`aula_fija`) y la tabla retirada `Curso_Materia`, pero con `user_version = 2`
  (la marca actual), dejó de abrir con «El esquema … no corresponde al esperado». Causa: la spec 004
  cambió el DDL de `Cursos` **sin subir `VERSION_ESQUEMA_ACTUAL`**; la marca ya no distingue una base
  vieja de una nueva y solo la verificación de catálogo la rechaza. Medidas:
  - `VersionEsquema::esquemaEsperado(db, QString* motivo)`: explica exactamente lo que no cuadra
    (tablas/columnas faltantes y tablas retiradas presentes).
  - `AperturaBaseDatos` distingue versión ≠ esperada (nombra ambas) de catálogo ≠ esperado (nombra el
    motivo) en `detalle`; sigue sin migrar ni tocar datos (RF-6).
  - El arranque del cliente implementa el **diálogo de recuperación RF-4** (Reintentar / Restaurar
    respaldo / Crear base nueva con respaldo obligatorio / Salir) sobre las APIs ya probadas de
    `Respaldo`; antes solo mostraba un `QMessageBox` fatal y salía.
  - **Guardia anti-regresión**: `test_backend_version_esquema` congela la huella SHA-256 del catálogo
    por versión (`HUELLAS_CONOCIDAS`); cambiar el DDL obliga a subir la versión y registrar la huella
    nueva, o el test falla. `test_backend_apertura_base_datos` reproduce el incidente (catálogo viejo
    con marca actual → `FalloEsquema` con motivo).
  - Verificación en host (MSVC + Ninja + OR-Tools): suite **36/41**; los 5 rojos son los conocidos de
    la spec 005 (`constructor_entrada`, `validador_entrada`) y los de solver/IPC (`0xc0000135` sin las
    DLL de OR-Tools en `PATH`; ya crashean en `restriccionBloquesRequeridos` con ellas). Todos los
    tests de BD/versión/respaldo/apertura/contexto en verde.
  - Base del usuario recreada (estaba vacía; respaldada como `gestor_horarios.db.respaldo-20261017-090000`).
- Entorno: tests dentro del contenedor `gestor-dev` → `docker exec gestor-dev bash -lc "cd /workspace && cmake --preset full && cmake --build build && ctest --preset full --output-on-failure"`. OR-Tools solo en el contenedor; el host no lo tiene.
- **Vista de generación en el frontend (2026-10-17, cierre parcial de 005):** implementada la vista
  «Generación de horario» (`ScheduleGenerationWidget`, índice 4 de `MainWindow`) y su capa de servicios
  en `src/frontend/src/services/` (`generacion_tipos.hpp`, `entrada_solver_builder`, `puerto_solver_ipc`).
  Arma la entrada del solver reutilizando `construirJsonEntrada` con el **contrato nuevo**: planes
  **embebidos** (compuestos con `NucleoDatos::componerPlanCompleto`), curso por `anio`+`seccion`,
  `aula_fija` **global** y `anios` por docente; ya **no** inyecta `materias` por curso ni
  planificación/penalizaciones. La envía por IPC (`OP_RESOLVER_HORARIO`) con `ServicioGeneracion` y
  traduce el `HorarioSalida` (clave `claveCurso`) al formato de visualización (`teachers`/`years`).
  El preset usa el formato de la propia configuración (`CargadorConfiguracionSolver`). `MainWindow`
  guarda el `InternalClient` como miembro, inyecta el `NucleoDatos` en la vista y, al emitirse
  `horarioGenerado`, guarda el JSON en la carpeta de horarios, lo carga en
  `ScheduleVisualizationWidget` (`loadSchedules`) y salta a esa sección. Se corrigió el cross-talk de
  `ScheduleVisualizationWidget` (ahora solo responde a sus propias ops). `src/CMakeLists.txt` declara
  `backend` **antes** que `frontend` para que la interfaz use la lógica de dominio; `frontend` enlaza
  `backend` y define `GESTOR_TIENE_BACKEND` (sin backend, la vista muestra un aviso). Compila en
  los presets `full` y `dev-frontend` (verificado en host MSVC+Ninja; `gestor-horarios.exe` enlaza).
  Doc: `docs/interfaz-frontend.md` §3.6.
  Añadido el test `test_frontend_conversion` (`test/frontend/test_conversion_entrada.cpp`) para la
  traducción `HorarioSalida`→visualización; pasa en local (`ctest -R test_frontend_conversion`).

- 008: **T1–T9 completas** ✅ (APROBADO por el reviewer; 57 materias, 4 planes, 13 cursos sin turno, 16 aulas, 4 variantes de docentes; suite 46/46).
- 005: **T11 completa** ✅ (hora académica = bloque; `bloques`/`bloquesAula` identidad; T10 sigue abierta).

- **Visualización dirigida por datos (2026-10-18):** la rejilla de horarios (ventanas de profesores y de
  alumnos) deja de suponer 12 franjas de 45 min y un receso de 30 min y se dibuja con lo que el motor
  recibe. Hallazgos y cambios:
  - **`aFormatoVisualizacion` descartaba las franjas de la tarde.** El filtro era `slot > 11` (plantilla
    antigua): con el contrato actual (16 franjas/día, mañana 0..7 y tarde 8..15) los cursos de la tarde
    perdían sus clases en la vista. Ahora el rango válido lo declara la configuración
    (`num_slots_dia`/`num_dias`) y el filtro es `slot >= numSlotsDia`.
  - **La rejilla viaja en el horario (`_cuadricula`).** `aFormatoVisualizacion` emite duración del bloque,
    días, franjas por turno (con su hora de inicio) y recesos (posición y duración), leídos de la propia
    entrada ya construida (`leerCuadricula`), así que la vista no adivina: si mañana el bloque es de
    50 min o el receso de 15, se ve tal cual. Cada sección declara además su `turno`.
  - **Rejilla por turno y con varias franjas.** Un curso ve solo las franjas de su turno (con su receso);
    un docente, las de todos (puede dar clase en ambos). Una clase que ocupa franjas seguidas (misma
    materia, profesor y aula) se pinta como **un solo bloque** («12:40 - 14:00 · 2 h») y se arrastra
    entero (`planMoveRun`); un receso en medio corta la unión. Si el documento no trae `_cuadricula`
    (archivo de ejemplo integrado y horarios guardados por versiones anteriores) se usa la plantilla
    histórica de 12×45 min + receso de 30.
  - **Leyenda y subtítulos.** La leyenda se arma con las materias reales del horario (ordenadas por peso,
    en una tira desplazable) y los subtítulos muestran turno, clases pintadas y horas académicas.
  - Pruebas: nueva `test_cuadricula_horario` (filas por turno, receso, horas, tramos y movimiento del
    bloque completo), `test_frontend_conversion` actualizada (conserva la tarde, emite `_cuadricula` y el
    turno) y `test_generacion_seed` ampliada (el documento de visualización conserva las 520 clases,
    incluidas las de la tarde). **Suite 47/47 en verde.** Verificado a ojo con capturas de las ventanas.

- **Frontend + datos de prueba: generar con el motor (2026-10-17, cierre de la 008 en host):** se
  probó el flujo completo de la vista «Generación de horario» con la base sembrada
  (`datos-prueba/centro-31059-holgada.db`, variante `holgada`), copiada sobre la base de la app
  (`%APPDATA%/LiceoRobertSerra/GestorHorarios/gestor_horarios.db`) con respaldo previo
  (`gestor_horarios.db.respaldo-antes-seed-*`). Cambios:
  - **Turno por curso en el preset.** `ParametrosCurso` gana `turno`; la columna **Turno** de la tabla
    de cursos pasa de texto de la base (siempre vacío en los datos sembrados) a un **desplegable** con
    los turnos de la base + «Sin turno»; `construirEntrada` lo inyecta en el preset y
    `ConstructorEntradaSolver` **prefiere el `turno` del preset** y cae al de la base si viene vacío
    (la 005 I1 sigue valiendo cuando el preset no lo declara). `validarConfiguracion` exige turno.
  - **Duración del bloque y slots derivados.** Al cargar la base por primera vez, la vista deriva de
    los turnos el nº de slots/día (suma de slots) y `duracion_minutos` (franja menos recesos sobre
    sus slots; 40 min en el conjunto). Antes solo derivaba los slots y fijaba 45 min.
  - **Fix de mapeo de disponibilidad (`ConstructorEntradaSolver::slotsDeFranja`).** Antes exigía que
    la franja del docente estuviese **contenida** en un turno, así que la disponibilidad por defecto
    del seed (07:00–18:00, que abarca ambos turnos) se mapeaba a **cero** slots y **V13** rechazaba
    (materia sin candidato). Ahora se emite cada bloque (anclado en el `inicio` del turno, pasos de
    `duracion_minutos`) que cae **íntegramente** dentro de la franja, acumulando por turno — la misma
    semántica que la cobertura del seed («la franja contiene un bloque completo»). `test_constructor_entrada`
    sigue en verde.
  - **Verificación de integración** `test_generacion_seed` (`test/frontend/`, con `frontend` +
    `backend_solver` + `seed`): siembra una base temporal, compone la entrada con el reparto
    plan/turno de la guía y **resuelve**; **Release en host: horario en ~1,5 s**. En **Debug sobre
    Windows** el motor no arranca (CRT de depuración contra DLL Release de OR-Tools → `0xc0000374`),
    igual que `test_solver_horarios`; el preset `full` es para Docker. Para generar en host hay que
    **compilar en Release** (`build-rel`) y poner
    `C:/ortools-v9.15-vs2022/or-tools_x64_VisualStudio2022_cpp_v9.15.6755/bin` en `PATH` (el proceso
    `--backend` hereda el `PATH` del frontend).
