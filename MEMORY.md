# MEMORY.md — Estado del proyecto

> Plantilla de estado SDD. Se actualiza al cerrar cada fase.

## Fase actual

- **Fase:** **012 (preferencias horarias del docente)** **cerrada e implementada**; las 009, 010 y 011 también. Suite **54/54** en verde (`ctest --preset backend`, 54 tests registrados).
- **Fecha:** 2026-10-29
- **Rama:** `feat/Base-datos` (HEAD `e12641b`; **71 commits** por delante de `develop` = `5d6b1da`; sin PR abierto).
- **Specs en curso:** ninguna. **Ya no queda ninguna spec sin plan ni tareas**; `specs/005-nucleo-solver` sigue **implementada con T10 abierta** por pendientes conocidos. El resto, implementadas (ver tabla).
- **Siguiente paso:** no hay specs pendientes de abrir. Ver «Siguiente paso» (005-T10, deudas del frontend, `release/1.0` y PR a `develop`).

## Specs activas

| Spec | Estado | Fase |
|---|---|---|
| 001-base-datos-local | aprobada | **implementada** (T1–T8) |
| 002-generacion-horarios-ipc | aprobada | **implementada** (T1–T9) |
| 003-cierre-001-base-datos | aprobada | **implementada** (T1–T5: cierra los 5 huecos de la 001) |
| 004-persistencia-campos-curso | aprobada | **implementada** (T1–T9) |
| 005-nucleo-solver | aprobada | **implementada** (T1–T9 + T11; **T10 abierta** por pendientes conocidos) |
| 008-datos-de-prueba | aprobada | **implementada** (T1–T9; 57 materias, 4 planes, 13 cursos, 4 variantes) |
| 009-tipos-de-aula-y-eventos | aprobada (2026-10-23) | **implementada** (T1–T9; APROBADO 2026-10-24) |
| 010-solver-tipos-y-eventos | aprobada (2026-10-23; cambio de requisitos 2026-10-26) | **implementada (2026-10-27)** (T1–T10 + T11–T16; APROBADO de la 2.ª entrega 2026-10-27) |
| 011-horas-academicas | aprobada (2026-10-23) | **implementada (2026-10-29)** (T1–T10; **T8 adelantada**; APROBADO 2026-10-29) |
| 012-preferencias-docente | aprobada (2026-10-23) | **implementada (2026-10-29)** (T1–T9; APROBADO 2026-10-29) |

> Épico **solver** (numeración real del trabajo): **004 (persistencia de campos) → 005 (núcleo) → 008 (datos de prueba) → 009 (tipos de aula y eventos) → 010 (tipos en el solver) → 011 (horas académicas, **cerrada el 2026-10-29**) → 012 (preferencias del docente, **cerrada el 2026-10-29**)**. Las antiguas **006 (turnos automáticos)** y **007 (generación parcial)** **no se han creado** y no existen en `specs/`; si se retoman, se numerarán al abrirlas.

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
- **Tipos de aula y eventos (spec 009, cerrada el 2026-10-24):**
  - **Reparto en dos specs.** La **009** se queda con **persistencia y servicios** (esquema, marca de versión, catálogo de esquema, servicios, cascada, huella, seed y documentación de la API); todo lo del **solver** pasa a la **010** (contrato 2.0, regla de tipo, validaciones, supersesión de la 005, constitución, documentación del contrato y MER). La 009 **precede** a la 010.
  - **Clave normalizada almacenada en el catálogo** (`TiposAula.clave`: recorte + `toCaseFolded()` Unicode, calculada por el servicio) con **índice único sobre la clave**; el `lower()`/`trim()` de SQLite solo pliega ASCII. Corrige la definición y RF-5 de la 009 sin reabrirla.
  - **Eventos del turno con identidad propia** (varios por franja); el **orden persistente** dentro de la posición es consecuencia de la presentación estable. Sustituyen a los recesos: **sin capa de compatibilidad**.
  - **Contrato del solver a 2.0** (lo implementa la 010): la 009 solo persiste y expone tipos y eventos.
  - **Interfaz fuera de alcance**, como en la 001; se documenta la API en `docs/interfaz-frontend.md`. El frontend no compila hasta que su equipo lo adapte; las heurísticas de la interfaz pasan a deuda documentada. La **consola** no es frontend y se actualiza aquí.
  - **Sin migración:** una base con el **esquema anterior** (sin catálogo/eventos) es **fallo de apertura** —no se migra, no se borra, no se convierten recesos—; la marca de versión sube (esquema **v3**).
  - **Seed sin promesa de generabilidad:** se siembra de forma **determinista** con tipos y eventos coherentes, pero **no** se exige generar horario con la regla dura de tipo (supersede la promesa de la 008). Cada materia recibe el tipo especializado de su área si el perfil tiene aulas de ese tipo; si no, el genérico. El conjunto se sustituirá por los datos reales.
- **Tipos y eventos en el solver (spec 010, 1.ª entrega T1–T10, 2026-10-25):**
  - **Contrato 2.0 único**: se acepta solo `2.0`; `1.0`, ausente o desconocida se rechaza como entrada inválida, **sin conversión** ni tipos por defecto; los presets 1.0 en disco no se tocan.
  - **Regla de tipo dura siempre** (D8/D9 ampliadas): toda clase en un aula del **tipo exacto** requerido; en modo fijo el aula del curso es su **aula de referencia**: se usa cuando el tipo coincide, las demás se **desplazan** y el curso **vuelve**; mientras está desplazado la referencia queda **vacía y bloqueada**.
  - **Salida con el aula real** de cada clase (también en modo fijo), no el aula de referencia. **V16/V17** (tipo declarado en aulas y materias; el tipo requerido tiene aulas, en ambos modos).
  - **P4** (`ValidadorSalidaSolver`): la ocupación se indexa por **(recurso, día, slot)**, no por franja. Desviación **autorizada por el usuario** fuera del plan (`06fc70e`); sin ella, cualquier horario con dos cursos en paralelo se marcaba `CalculoFallido`.
- **Diagnóstico de causas y referencia general (spec 010, 2.ª entrega T11–T16, cerrada el 2026-10-27):**
  - **El aula de referencia es siempre del tipo general/ordinario**, identificado por **convención de nombre**: el tipo de clave normalizada **`aula de estudio`** (`tipoGeneralAula()`). **No hay columna, campo, sección ni cambio de esquema**; un centro cuyo tipo ordinario se llame de otra forma no está soportado. **V18** lo valida al final de la cadena (V16 → V17 → V18).
  - **La reserva del aula de referencia se mantiene** como regla del modelo, pero **no se descuenta** de la capacidad de ningún tipo.
  - **La causa (2) conflicto de aula es un conteo exacto por tipo**: `demanda(t) > nº de aulas del tipo × nº de días × nº de franjas distintas de la unión de los turnos de los cursos que requieren t`, sin cotas y **sin descontar bloqueos**. Es **necesario, no suficiente**.
  - **Si ningún test aplica, la causa es «no atribuible»**: el mensaje no nombra una entidad y no se inventa categoría. El sistema sigue sin corregir la entrada.
  - **Excepción de precedencia**: si un curso necesita más bloques que franjas tiene su turno, la causa es **(3) conflicto de persona** aunque la capacidad del tipo también falle (es el motivo concreto y comprobable). Fuera de ese caso, el orden (1)(2)(3)(4) no cambia.
  - **Se eliminaron** `palomarPorTipo`, `franjasBloqueadasEnFranjasContadas`, el conteo por aula de referencia compartida y `causaConflictoDeAula`; **`causaSaturacionPorTipo` se reescribe** (es el conteo que se conserva) y `analizarFallo` informa el desenlace «no atribuible».
  - **`FranjaSemana` (día, franja)** en `contrato_solver`, con las funciones puras de demanda/capacidad; elimina los productos manuales por `num_dias` (origen del error de unidades) y no refactoriza `x`/`y`.
  - **Documentación y normas alineadas**: `docs/contrato-solver-2.0.md` (tipo general, capacidad, «no atribuible», preset 2.0 con referencia especializada inválido por V18), `docs/constitution.md` (§7.1.5, §7.3, §7.4 y «Comportamiento») y la 005 (**V1–V18**, (2) por capacidad, «no atribuible» y excepción).
- **Superado por la 010 (2026-10-27).** La definición de causa de la **005** (categorías con (2) = D8/D9/D10 y «primera categoría aplicable» sin desenlace alternativo), la versión **1.0** del contrato y la invariante **D10** quedan supersedidas por la 010; la 005 sigue vigente en todo lo demás (D1–D9, D11, D13, S1/S2, V1–V15 en lo no citado). El «contrato JSON de 002» que supersedía la 005 también queda superado por el 2.0 de la 010.

- **Duración de la hora académica (spec 011, cerrada el 2026-10-29):**
  - **Configuración global de una fila** (`ConfiguracionHoraria`, `CHECK(id = 1)`), **35 por defecto**, rango **30–90**; **esquema v4**, catálogo y `HUELLAS_CONOCIDAS` en el mismo cambio; **sin migración** (una base v3 se rechaza al abrir).
  - **Punto único `duracion_horaria`** (rango, horas académicas → minutos, `franjasUtilizables`, `deficitDeEncaje`), usado por la configuración, **V2 del contrato**, el servicio de turnos, el seed y el constructor. **Ninguna otra pieza hace horas → minutos**; multiplicar por D para el ancho de franja o el encaje **no** es esa conversión.
  - **La franja dura D**: se elimina `duracionFranjaEstimada` y el reparto de la ventana; el sobrante queda al final. **V2 pasa de `> 0` a 30–90** y valida el **encaje de todos los turnos**.
  - **El preset no manda**: `construirJsonEntrada(datos, preset, D, ahora)` **sobrescribe siempre** la duración antes de validar (un preset con 0, 5 o 45 no bloquea).
  - **La huella de datos incluye D**; un cambio de D entre el inicio y la recepción marca «datos anteriores».
  - **Seed con D = 35**: turnos **07:00–11:50 / 12:40–17:30**, recesos **08:10–08:20 / 13:50–14:00**, `8 × 35 + 10 = 290`; rejilla de clases hasta **11:40/17:20** y `fin` que añade los 10 min del evento. La configuración se asegura de forma idempotente y **no cuenta como dato de dominio**.
  - **Si falla la lectura de la configuración** (error de SQL, no «sin fila»), **la operación de turnos/eventos falla**; el 35 solo se usa cuando hay esquema y no hay fila (decisión del usuario, `19ceccb`).
  - **Supersesiones**: 009 (franja estimada y su cálculo), 008 (8×40+10), 005/010 (V2 solo `> 0`), anotadas en sus specs con fecha `011 (2026-10-29)`.

- **Preferencias horarias del docente (spec 012, cerrada el 2026-10-29):**
  - **Modelo que replica la disponibilidad y añade sentido.** Tabla `Preferencia_Profesor` con docente, día, inicio, fin y **sentido** (`preferida`/`evitada`), con identidad propia. **No se mezcla** con la disponibilidad: una cosa es poder y otra querer.
  - **Día 0–6** (0 = domingo … 6 = sábado), el mismo rango que la disponibilidad; sábado y domingo se guardan y se exponen pero **no intervienen** en la generación actual.
  - **Validación:** docente existente, día 0–6, `inicio < fin`, sentido válido, **sin solapes de sentidos opuestos**, **sin duplicado exacto**; los solapes y rangos contiguos del **mismo sentido** se admiten; **cambiar el sentido o el día se revalida** y, si contradice, se rechaza **conservando el anterior**.
  - **Servicios y DTO:** `ServicioPreferencias` (`agregar`/`listar`/`actualizar`/`eliminar`/`limpiar`) con `Resultado<T>` y mensajes con valor y acción; `ProfesorDTO` gana `preferencias` **separadas** de `disponibilidad` (cambios aditivos) y **`toProfesor()` las ignora**.
  - **Cascada en los dos caminos:** `eliminarProfesor` y `eliminarConCascada` borran preferencias y disponibilidad de forma atómica; las dependencias se listan antes de confirmar.
  - **Huella intacta:** no entran en `DatosDominio`/`huellaDatos` y cambiarlas no marca «datos anteriores». **Condición futura:** cuando el solver las consuma, deberán entrar en ambos.
  - **Esquema v5** con catálogo y `HUELLAS_CONOCIDAS` en el mismo cambio; **sin migración** (una base v4 se rechaza al abrir). `CHECK(hora_inicio < hora_fin)` **depende del formato `HH:mm`** (barrera lexicográfica, no un parser), y el `UNIQUE` frena el duplicado exacto.
  - **Seed con 0 preferencias** (`Preferencias: 0`, 12 tablas de dominio); `--solo-docentes` y el reemplazo total **borran preferencias antes que docentes** y disponibilidad, en una transacción; **supersede RF-9 y la definición de reemplazo de docentes de la 008**.

## Siguiente paso

1. **No hay specs pendientes de abrir**: la 012 se cerró el 2026-10-29 y ninguna queda sin plan ni tareas.
2. **Pendientes de la 005-T10** (ver abajo): RF-13, RNF-4, avisos retirados (I2) y residuales del validador de salida.
3. **Deudas del frontend** (documentadas en `docs/interfaz-frontend.md` §6): el builder no compila con la firma nueva `construirJsonEntrada(datos, preset, D, ahora)`/`huellaDatos(datos, D)`; el selector de duración del editor y de la vista deja de ofrecerse; `horasAula` y la aritmética en minutos de la vista; «datos anteriores» sin cablear (`huellaActual = {}`); `test_generacion_seed` aún fija 40. De la 012, **no hay UI de preferencias** (solo API); sus cambios de firma son aditivos.
4. **Divergencia con `release/1.0`:** esa rama tiene **16 commits propios** (spec `009-agrupacion-bloques-materias` con la penalización S3, frontend y empaquetado Windows) **sin integrar** en `feat/Base-datos`; ambas ramas tocan el **solver**, así que el conflicto será real. Ojo: su `009` **no es** nuestra `009-tipos-de-aula-y-eventos` (colisión de numeración entre ramas).
5. **Riesgo latente:** `Solver::resolver` **no reinicia `model`/`vars`** (son miembros); no afecta al camino real (una instancia por ejecución), pero conviene arreglarlo cuando se toque.
6. **71 commits de `feat/Base-datos` sin PR a `develop`** (rama protegida: por PR, sin merge commits).
7. **MEMORY actualizado** (2026-10-29) con el cierre de la 012.

### Pendientes conocidos de la 005 (diferidos y documentados)

- **RF-13 — planes de los docentes:** `ConstructorEntradaSolver` embebe los planes de los **cursos** pero no los que usan los **docentes**. Sin impacto funcional hoy (el plan del docente se compara como cadena), pero es una cláusula explícita del RF.
- **RNF-4 — cobertura de tests por regla:** faltan casos rojo→verde de V2 (`peso` negativo; el rango de `duracion_minutos` lo cubre la 011), V7 (turno ausente), V8 (modo inactivo), V14, V15, D13 (`anios`), S1/S2 (exceso y titular a tope), reproducibilidad (RNF-1), aula de salida como índice (fija/móvil) y `peso` en el ida y vuelta. Por esto los «Hecho cuando» de T3/T5/T7/T8 no quedan cerrados y **T10 sigue abierta**. La reproducibilidad del solver ya tiene un caso en la 010 (`solver_esReproducible_mismaEntradaMismaSalida`), pero no cubre el ida y vuelta ni el resto.
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
- 002: **T1–T9 completas** ✅ (`CargadorConfiguracionSolver` + `SolverConfig::toJson`; `solver_config` movido al target `backend` sin OR-Tools; `ConstructorEntradaSolver`/`construirJsonEntrada` + `DatosDominio`; `ValidadorEntradaSolver` con V1–V13; `ValidadorSalidaSolver` P1–P4; `ServicioGeneracion` + `PuertoSolver`; `messages.h` y transporte; `servicio_solver`/`resolverEntradaSolver`; contrato de `ServicioHorarioSalida`; `docs/interfaz-frontend.md` §3 + `test_contrato_generacion`). **Spec 002 implementada.**
- 005: **T1–T9 completas** ✅ (T10 abierta por pendientes conocidos). Contrato nuevo (`aula_fija` global, curso por `anio`+`seccion`, planes embebidos, `anios`, centinela `-1`), funciones puras (`bloques`/`bloquesAula`/`claveCurso`/`materiasDelCurso`/elegibilidad), validación V1–V15, construcción de la entrada desde BD+preset, variables CP-SAT, restricciones duras (D1–D11 y D13; D12 eliminada), objetivo S1+S2 (`PESO_HORAS > COSTE_SUPLENTE`), extracción por identidad (`claveCurso`, aula = índice) y causas de no factibilidad por categorías (RF-6). Sellaba `version="1.0"` al producir/guardar (**superado por la 010: 2.0**).
  - **Validación del reviewer (ronda 2): APROBADA.** Bloqueantes corregidos y commiteados: `37666a2` (D1/D3 siempre publicadas + V13 cruza disponibilidad×turno), `454a53b` (RF-10 versión no reconocida), `7a7ff1b` (RF-11 centinela `-1`, coherencia de horas, V9 estricto), `4171a4d` (RF-6 categorías de causa), `f063f39` (sello `version="1.0"`).
  - **T11 (rectificación 2026-10-17):** hora académica = bloque; `bloques`/`bloquesAula` identidad; `duracion_minutos` solo describe la franja; constitución §7 enmendada. Suite **46/46** entonces. **T10 sigue abierta.**
- 008: **T1–T9 completas** ✅ (APROBADO por el reviewer; 57 materias, 4 planes, 13 cursos sin turno, 16 aulas, 4 variantes de docentes; suite 46/46).
- 009: **T1–T9 completas** ✅ (**APROBADO 2026-10-24**). Esquema **v3** con `TiposAula` (clave normalizada + índice único), `id_tipo_aula` obligatorio y `Eventos` con identidad/orden (`059168f`, `357102d`); servicios de eventos con coherencia temporal y ventanas por posición (`08067ce`, `693fa18`), catálogo de tipos y huella (`83d7e7f`), fachada/cascada/puente de eventos en la entrada (`e79d227`); seed con tipos y eventos y reemplazo seguro (`f438079`, `af1ac8a`); preset `backend` sin interfaz (`8cfa211`); API, guías y deuda del frontend (`221e563`); mensajes de rechazo con el valor recibido (`1835399`); cierre (`e72c930`).
- 010: **implementada (2026-10-27)** ✅. Dos entregas validadas:
  - **1.ª (T1–T10, 2026-10-25):** contrato 2.0 y rechazo de versiones, V16/V17, regla de tipo con aula de referencia/desplazamiento/vuelta, salida con aula real, **P4**, supersesión de la 005, enmienda constitucional, MER y documentación del contrato (`0096b36` plan/tareas, `3a3e724` T1, `d2fbda1` T2, `4142939` T3, `0edc6f8` T4, `06fc70e` P4, `37cd2e1` T6, `f054b2e` T7, `4134209` T8, `9c5c301` T9, `35cd5f0` T10) y **cinco correcciones** de la heurística de causas (`aa7ee74`, `232bd19`, `45ac292`, `5fc40e0`, `de61050`; suite **49/49** en `de61050`), que llevaron al **cambio de requisitos del 2026-10-26**.
  - **2.ª (T11–T16, 2026-10-27):** `FranjaSemana` y funciones puras (`227eacd`), **V18** (`736bbcd`), causa (2) por capacidad sin descuentos y «no atribuible» (`d3e8de0`), tests de ventanas de turno/reproducibilidad/aula especializada (`b4491ad`), contrato-constitución-005 alineados (`da71419`), regresión global con el seed intacto (`a233afc`), notas del reviewer en los tests (`072b6ee`) y **cierre** (`dea89db`). Suite **50/50**; benchmark factible en **~185–277 ms**.
- 011: **implementada y cerrada (2026-10-29)** ✅ (APROBADO 2026-10-29 contra `8e0602c`; suite **52/52**).
  - T1 `fc86368` · T2 `11077f8` · T3 `4262b49` · T4 `36c28b2` · **T8 `c913d87` (adelantada)** · T5 `bc7122b` · T6 `fbb1f68` · T7 `15db0d2` · T9 `c62653c` · T10 `059815b` · correcciones `19ceccb` · cierre `0703052`.
  - **T8 se adelantó a T5** porque T5 dejaba `test_seed_aplicacion` en rojo hasta que el perfil pasara a 35; la regla «ninguna tarea cierra con el árbol sin compilar» lo exigió.
  - **No se tocó** el modelo del solver, el tope de 40 h ni la interfaz; las deudas del frontend quedan en `docs/interfaz-frontend.md` §6.
- 012: **implementada y cerrada (2026-10-29)** ✅ (APROBADO 2026-10-29 contra `3007c54`; suite **54/54**).
  - T1 `4993be0` · T2 `3871488` · T3 `7405351` · T4 `d2f2147` · T5 `8acce25` · T6 `3b8ba90` · T7 `43c222c` · T8 `94647ef` · T9 `98c9dbc` · notas de la validación (M1–M6) `e12641b`.
  - **Alcance:** persistencia, validación, servicios, cascada, seed y documentación. **No se tocó** el solver, el contrato ni la interfaz; las preferencias **no alteran la generación** y quedan **fuera de la huella** hasta que el solver las consuma.
- Tests ajenos que fallan: **ninguno**. Queda un **flaky preexistente** (`test_middleware_transport`, ~1 de cada 6 ejecuciones, `QLocalSocket: Invalid name`), ajeno al solver. El ordenamiento con acentos de `test_servicio_profesor` quedó **resuelto** con `QCollator` (ver decisión).
- **Incidente y hotfix de apertura de base (2026-10-17):** una base creada con el DDL anterior de `Cursos` (`nombre`/`aula_fija`) y la tabla retirada `Curso_Materia`, pero con `user_version = 2`, dejó de abrir con «El esquema … no corresponde al esperado». Causa: la spec 004 cambió el DDL de `Cursos` **sin subir `VERSION_ESQUEMA_ACTUAL`**; la marca ya no distinguía una base vieja de una nueva. Medidas:
  - `VersionEsquema::esquemaEsperado(db, QString* motivo)` explica exactamente lo que no cuadra (tablas/columnas faltantes y tablas retiradas presentes).
  - `AperturaBaseDatos` distingue versión ≠ esperada de catálogo ≠ esperado (nombra el motivo) en `detalle`; sigue sin migrar ni tocar datos (RF-6).
  - El arranque del cliente implementa el **diálogo de recuperación RF-4** (Reintentar / Restaurar respaldo / Crear base nueva con respaldo obligatorio / Salir) sobre las APIs ya probadas de `Respaldo`.
  - **Guardia anti-regresión:** `test_backend_version_esquema` congela la huella SHA-256 del catálogo por versión (`HUELLAS_CONOCIDAS`); cambiar el DDL obliga a subir la versión y registrar la huella nueva. `test_backend_apertura_base_datos` reproduce el incidente.
  - Base del usuario recreada tras respaldo (`gestor_horarios.db.respaldo-20261017-090000`).
  - Verificación de entonces en host (MSVC + Ninja + OR-Tools): 36/41, con los rojos conocidos de la 005 y los de solver/IPC por las DLL de OR-Tools.
- Entorno: la verificación es el preset **`backend`** en el host (`cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend`), con OR-Tools en `~/.local/ortools`; **54/54**. El contenedor `gestor-dev` (`--preset full`) queda como vía alternativa.
- **Vista de generación en el frontend (2026-10-17, cierre parcial de 005):** implementada la vista «Generación de horario» (`ScheduleGenerationWidget`, índice 4 de `MainWindow`) y su capa de servicios en `src/frontend/src/services/` (`generacion_tipos.hpp`, `entrada_solver_builder`, `puerto_solver_ipc`). Arma la entrada reutilizando `construirJsonEntrada` con el contrato nuevo (planes embebidos, curso por `anio`+`seccion`, `aula_fija` global, `anios`), la envía por IPC (`OP_RESOLVER_HORARIO`) y traduce el `HorarioSalida` al formato de visualización. `MainWindow` guarda el `InternalClient`, inyecta el `NucleoDatos` y, al emitirse `horarioGenerado`, guarda el JSON y lo carga en `ScheduleVisualizationWidget`. Doc: `docs/interfaz-frontend.md` §3.6. Test `test_frontend_conversion`. **Rotura conocida:** el builder del frontend no compila con `construirJsonEntrada` devolviendo `Resultado<QJsonObject>` (deuda del equipo del frontend; el preset `backend` no lo compila).
- **Frontend + datos de prueba: generar con el motor (2026-10-17, cierre de la 008 en host):** se probó el flujo completo de la vista «Generación de horario» con la base sembrada (`datos-prueba/centro-31059-holgada.db`, variante `holgada`), copiada sobre la base de la app con respaldo previo. Cambios:
  - **Turno por curso en el preset.** `ParametrosCurso` gana `turno`; la columna **Turno** pasa a desplegable con los turnos de la base + «Sin turno»; `construirEntrada` lo inyecta y `ConstructorEntradaSolver` **prefiere el `turno` del preset** y cae al de la base si viene vacío. `validarConfiguracion` exige turno.
  - **Duración del bloque y slots derivados.** Al cargar la base, la vista deriva de los turnos el nº de slots/día y `duracion_minutos` (40 min en el conjunto).
  - **Fix de mapeo de disponibilidad (`ConstructorEntradaSolver::slotsDeFranja`).** Emite cada bloque (anclado en el `inicio` del turno, pasos de `duracion_minutos`) que cae **íntegramente** dentro de la franja, acumulando por turno — la semántica del seed. Antes la disponibilidad 07:00–18:00 se mapeaba a cero slots y V13 rechazaba.
  - **Verificación de integración** `test_generacion_seed` (`test/frontend/`): siembra una base temporal, compone la entrada con el reparto plan/turno de la guía y **resuelve**; **Release en host: ~1,5 s**. En **Debug sobre Windows** el motor no arranca (CRT de depuración contra DLL Release de OR-Tools → `0xc0000374`). Para generar en host: compilar en Release (`build-rel`) y poner `C:/ortools-v9.15-vs2022/or-tools_x64_VisualStudio2022_cpp_v9.15.6755/bin` en `PATH`.
