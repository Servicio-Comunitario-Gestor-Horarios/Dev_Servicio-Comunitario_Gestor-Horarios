# MEMORY.md — Estado del proyecto

> Plantilla de estado SDD. Se actualiza al cerrar cada fase.

## Fase actual

- **Fase:** spec 001 **implementada** (T1–T8 completas) — pendiente arreglar el ordenamiento de
  `ServicioProfesor`; siguiente spec 002
- **Fecha:** 2026-10-09
- **Rama:** `sdd/001-base-datos-local`
- **Specs en curso:** `specs/001-base-datos-local` (implementada), `specs/002-generacion-horarios-ipc` (pendiente)
- **Siguiente paso:** implementar `specs/002-generacion-horarios-ipc/tasks.md` (depende de 001)

## Specs activas

| Spec | Estado | Fase |
|---|---|---|
| 001-base-datos-local | aprobada | **implementada** (T1–T8) |
| 002-generacion-horarios-ipc | aprobada | tareas redactadas (depende de 001) |
| 003-cierre-001-base-datos | aprobada | **implementada** (T1–T5: cierra los 5 huecos de la 001) |

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

## Siguiente paso

1. `specs/002-generacion-horarios-ipc/tasks.md` (depende de 001): **T1 implementada**; siguiente **T2** (`ConstructorEntradaSolver`, JSON de 13 secciones).
2. **Deuda técnica pendiente:** arreglar el ordenamiento de `ServicioProfesor` (test
   `test_servicio_profesor` en rojo por ese motivo; es trabajo ajeno a 001, no se tocó).

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
- Tests ajenos que fallan (no tocar): `test_servicio_profesor` (ordenamiento pendiente), `test_solver_horarios`, `test_solver_benchmark` (solver).
- Entorno: tests dentro del contenedor `gestor-dev` → `docker exec gestor-dev bash -lc "cd /workspace && cmake --preset full && cmake --build build && ctest --preset full --output-on-failure"`. OR-Tools solo en el contenedor; el host no lo tiene.
