# Estado 019

Fase: **clarificacion (spec aprobada; plan y tareas pendientes, decidido el 2026-11-10)**
Aprobaciones: **spec 2026-11-10** · plan/tareas —
Base: `e8d4e2445313e569a1023a357b5df77e7318dc28` (`git rev-parse HEAD` el **2026-11-10**, rama
`feat/Motor`, merge local `merge(frontend): ensamblar feat/frontend-generacion con el backend 013-018`).
**Nada commiteado ni pusheado.**
Dudas abiertas: **ninguna** (Q1, la referencia del exceso de horas, se cerró el **2026-11-10**; ver P-5).
Siguiente paso: **escribir `plan.md` y `tasks.md` (mañana) y pasar el QA del plan**. Después, la
implementación, una tarea cada vez.
Veredicto: pendiente
Notas: esta spec **no** implementa **el resto de H7 y H9–H16** (nueve hallazgos), **salvo** las dos partes
ya declaradas de **H10** (comprobación de la cédula, RF-1) y **H14** (distinción de causas al borrar
materias, RF-6). La 019 **regulariza** las pantallas afectadas por H1, H2, H3, H4, H6 y H8 y **formaliza
H5**; el resto de la deuda de proceso (CRUD de planes, menú sin cuenta, ventana/leyenda; y la edición de
disponibilidad y materias del docente) queda **escrita**, no cerrada.

## Decisiones cerradas (2026-11-10)

| # | Resolución | Origen |
|---|---|---|
| D-1 | Spec nueva **019-integracion-frontend-backend**: **regulariza** las pantallas afectadas por H1, H2, H3, H4, H6 y H8, **formaliza H5** y **deja escrito como deuda** el resto del proceso de las pantallas (CRUD de planes, menú sin cuenta, ventana/leyenda), sin declararlo cerrado. | Usuario |
| D-2 | Alcance: **los seis desajustes prioritarios (H1, H2, H3, H4, H6 y H8) más la formalización H5**. La deuda es el **resto de H7 y H9–H16** (nueve hallazgos), salvo las dos partes de **H10** y **H14** que cierran RF-1 y RF-6; **no** son RF. | Usuario |
| D-3 | **H1**: eximir la validación cuando la cédula **no cambia** (en vez de aceptar alfanumérico general); test que **abra y guarde `P033`**. | Usuario |
| D-4 | **H2**: ramificar por `datosAnteriores`, pedir confirmación y **no guardar hasta que el usuario lo pida** (002 RF-1/RF-4; definición de huella de 004). | Usuario |
| D-5 | **H3**: propagar la causa real (categorías (1)–(5) del solver, o «no atribuible») hasta la UI. | Usuario |
| D-6 | **H4**: usar `horasSugeridasPorDocente` (ya existe en la composición de entrada) y mostrar el aviso de exceso **frente a las horas en aula declaradas**, manteniendo la coherencia de la 005; las **36 h** son el valor sembrado de referencia del centro, editable por convenio (P-5). | Usuario |
| D-7 | **H5**: `Profesor_Materia_Suplente` al **esquema versionado v7**, catálogo y `HUELLAS_CONOCIDAS` actualizados, borrado de materia referenciada **bloqueado/con cascada según corresponda** (resuelto como **bloqueo** en P-2), cascada de docente, **sin migración** (una base v6 deja de abrir: respaldo y base nueva). | Usuario |
| D-8 | **H6**: llevar el **aforo** (o las canchas) al documento de visualización y eximir los pares de cursos/docentes distintos conforme al **aforo 2** (018). | Usuario |
| D-9 | **H8**: rejilla de preferencias con **ninguno → preferida → evitada** (o modo explícito) y **persistencia del sentido**, con test. | Usuario |
| D-10 | Del **resto de H7 y H9–H16** (nueve) **no se implementa nada aquí**, salvo las partes de **H10** (cédula, RF-1) y **H14** (causas al borrar materias, RF-6). El resto de la deuda de proceso (planes, menú, ventana/leyenda; disponibilidad y materias del docente) y **002 RF-5** tampoco se cierran. | Usuario |

### Decisiones de redacción que el planner tomó para poder especificar

Se recogen en la spec como **P-1…P-5**; si el usuario no las comparte, se corrigen antes de aprobar.

| # | Resolución | Motivo |
|---|---|---|
| P-1 | Solo se exime el identificador **sin cambios**; las altas y los identificadores modificados siguen exigiendo **solo dígitos**. | Es la opción que el usuario nombró primero; la alternativa «alfanumérico siempre» queda descartada y anotada en la spec. |
| P-2 | El borrado de una materia referenciada como **suplente** se **bloquea** con la causa (como ya ocurre con plan y titular), no en cascada. | Coherencia con `ServicioMaterias::eliminarMateria` y con la §5 de la constitución; reversible y sin tocar datos de otros docentes. |
| P-3 | En un horario guardado **sin aforo declarado**, se asume **aforo 1** (dos clases en la cancha se señalan como conflicto) y se documenta que hay que regenerar. | No adivinar el aforo ni abrir un hueco (018 RF-7). |
| P-4 | La exención vale **solo para bloques completos**; compartir una sola franja de la cancha sigue siendo conflicto. | Es el requisito de pares cruzados de 018 RF-5/RF-7. |
| P-5 | La referencia del exceso son las **horas en aula declaradas por fila**; **no** hay tope de 36 h ni validación legal, y el control admite valores por encima y por debajo (convenios). Las **36 h** son el valor sembrado de **referencia** del centro (017 D-3), no el valor inicial de la tabla ni un máximo; se editan respetando la coherencia de la **005** (horas en aula ≤ horas requeridas). | El centro firma convenios de más o menos horas (decisión del usuario, 2026-11-10); la coherencia de la 005 es un chequeo del dato y **no** se relaja. |

## Qué se comprobó en el código (para el plan)

- **H1.** `src/frontend/src/forms/teacher_form_dialog.cpp` pone un validador `^[0-9]*$` en la cédula
  (líneas 83–85) y vuelve a validar con `^[0-9]+$` en `validarCampos()` (150–155); el mensaje exige
  «únicamente números». `cargarDatos()` escribe `P033` en el campo, pero el guardado lo rechaza. El
  backend **no** exige dígitos (`ServicioProfesor::validarProfesor` solo comprueba no vacío), así que el
  arreglo es de formulario. La ruta de edición (`teacher_list_widget.cpp`, ~1300) llama a
  `actualizarDocente(nCedula, …)` con el mismo identificador. Ojo: el validador también **impide teclear**
  mientras el texto cargado no sea válido.
- **H2.** `ServicioGeneracion::alResponder` marca `datosAnteriores = true` y `estado = DatosAnteriores`
  pero deja `presentable = true`; `ScheduleGenerationWidget::onGeneracionFinalizada`
  (`schedule_generation_widget.cpp` ~1080–1115) solo mira `presentable`, llama a `guardarHorario()` y
  anuncia «Horario generado y guardado». `MainWindow` ignora la ruta y carga el JSON en memoria, así que
  no guardar el archivo **no** rompe el flujo de visualización. `docs/interfaz-frontend.md` §3.1 ya
  documenta «`DatosAnteriores` → aviso destacado + confirmación obligatoria antes de guardar»: la
  interfaz no cumple el contrato documentado.
- **H3.** El motor devuelve la causa en `respuesta.error` (`solver.cpp` → `analizarFallo` con las
  categorías y la línea «Entorno: …»), `PuertoSolverIpc` la copia a `RespuestaSolver::error` y
  `ServicioGeneracion` la **descarta** en la rama `!factible` y escribe un mensaje fijo.
- **H4.** `schedule_generation_widget.cpp` (~715) usa `porDefecto = qMax(1, docente.materias.size() * 2)`
  para las dos columnas; el botón «Recalcular horas sugeridas» ya llama a
  `GeneracionSolver::horasSugeridasPorDocente` (~440). La función vive en `entrada_solver_builder.cpp`
  (~322) y suma horas por curso según el plan.
- **H5.** `ServicioProfesor::asegurarTablaSuplentes` (`ServicioProfesor.cpp:37-62`) crea
  `Profesor_Materia_Suplente` con `CREATE TABLE IF NOT EXISTS`; **no** está en `Migracion`, ni en el
  catálogo de `version_esquema.cpp`, ni en `HUELLAS_CONOCIDAS` (`test/backend/test_version_esquema.cpp`
  ~1021). `VERSION_ESQUEMA_ACTUAL = 6`. `ServicioMaterias::eliminarMateria` comprueba plan y
  `Profesor_Materia`, **no** suplentes. El sembrador (`seed/src/sembrador.cpp` ~539-565) la trata como
  tabla opcional y la omite si no existe. La cascada (`cascada.cpp` 55–72) y `eliminarProfesor`
  (360–361) ya borran las filas del docente, y `huellaDatos` ya incluye `sup:` (ServicioGeneracion
  60–63), así que **la huella de datos no cambia**.
- **H6.** `schedule_model.hpp` (`findConflicts`, 225–260) marca conflicto por `sameAula` o `sameProf`
  sin aforo. `aFormatoVisualizacion` (`entrada_solver_builder.cpp` ~390-470) emite `teachers` y `years`
  con materia, profesor, aula y curso, **sin aforo**; `ContextoHorario.aulas` solo lleva nombres. La
  regla del aforo está en un único punto: `src/common/include/common/aforo_aula.hpp`
  (`aforoDeClave`/`claveCancha`, cancha 2 / resto 1).
- **H8.** `teacher_list_widget.cpp` ya carga, pinta y guarda `m_evitadas` (735–760, 840–860, 1190–1215) y
  distingue los tres estados; lo que falta es la interacción: `onPreferenciasCellClicked` (1007–1031)
  solo alterna «preferida» y **borra** la evitada. La pestaña se llama «Horas preferidas».

## Encaje previsto en tareas (no es `tasks.md`)

Las **siete** correcciones del alcance (seis prioritarios + H5) más la línea base caben en **8 tareas**,
todas con su comprobación escrita antes del código:

| # | Contenido previsto | RF |
|---|---|---|
| T1 | Cédula: exención cuando no cambia, validador y mensajes; comprobación que abre y guarda `P033` (cierra **solo** la parte de H10 de la cédula) | RF-1 |
| T2 | Resultado: no guardar sin petición, rama de datos anteriores con confirmación y acción de guardado | RF-2 |
| T3 | NoFactible: propagar el diagnóstico completo y su respaldo «no atribuible» | RF-3 |
| T4 | Horas sugeridas reales, aviso de exceso frente a las horas en aula y coherencia 005 (horas en aula ≤ requeridas) | RF-4 |
| T5 | Esquema v7 completo: tabla, catálogo, huella congelada, guardia y sembrador sin casos especiales; **apertura fallida de una base v6**, **creación de base nueva con respaldo**, **repoblación del centro** y **documentación** de la consecuencia | RF-5, RF-7 |
| T6 | Borrado de materia con causas distinguibles (plan / titular / suplente), regla titular-y-suplente en el servicio, cascada del docente y **listado de dependencias** antes de confirmar | RF-6 |
| T7 | Aforo en el documento de visualización y detección de conflictos con aforo y pares completos | RF-8, RF-9 |
| T8 | Rejilla de preferencias con tres estados, leyenda, persistencia y cierre de suites | RF-10, RNF-1 |

T5 concentra **RF-5 y RF-7** porque la marca de versión, el catálogo y la comprobación de apertura se
mueven en el mismo cambio; por eso también cubre la v6 que deja de abrir, la base nueva con respaldo, la
repoblación y su documentación. La cifra puede bajar a 7 si T6 se une a T5; **no** puede subir de 10
(RNF-6).

## Deuda documentada (H7 y H9–H16, resto del proceso y 002 RF-5) — **no** son RF de esta spec

De la deuda solo se cierran **dos partes concretas**: la **comprobación de la cédula** dentro de **H10**
(RF-1) y la **distinción de causas** al borrar materias dentro de **H14** (RF-6). Todo lo demás sigue
abierto y se lista aquí.

| # | Hallazgo | Referencia |
|---|---|---|
| H7 | El servicio calcula el **cumplimiento de preferencias** (013) y el **informe de bloques** (014), pero ninguna pantalla los muestra: el usuario no ve preferencias incumplidas, desvío, huecos ni separación. | `ServicioGeneracion::alResponder` (`ServicioGeneracion.cpp` ~245-260); `ResultadoGeneracion::cumplimiento`/`bloques`; la vista solo usa `presentable`/`horario`. |
| H9 | El cálculo tiene plazo de **300 s** y la vista no ofrece **cancelar**; el texto de progreso dice «puede tardar unos segundos» cuando el centro real mide 161–264 s (y hubo 369,7 s). | `schedule_generation_widget.cpp:1066`; `ServicioGeneracion(m_puerto, 300000, …)` en `schedule_generation_widget.cpp:208`; `ServicioGeneracion.cpp:185-199`. |
| H10 | Las pantallas de **planes de estudio** y el **preset maestro del centro** siguen sin pruebas/entrega. *(La parte de la **cédula** se cierra en RF-1: la ficha abre y guarda `P033`.)* | `forms/plan_form_dialog.cpp`; `test/CMakeLists.txt` (no hay `add_qtest` para planes); deuda ya anotada en 017/018. |
| H11 | Texto **obsoleto del turno** en la ayuda de configuración: sigue diciendo «elige el turno, el plan de estudio y el número de estudiantes» cuando el turno vive en el curso (004/017), el plan es por curso en la configuración (005) y el número de estudiantes ya no tiene regla. | `schedule_generation_widget.cpp:365` (y el texto gemelo de la tarjeta de docentes, ~404). |
| H12 | El indicador **«Ocupación de Aulas»** estima `aulas ocupadas / total` en **1:1 materia/aula**, sin tipos, aforo ni turnos: da porcentajes que no significan nada. | `subject_list_widget.cpp:251, 424-430`. |
| H13 | La **disponibilidad guardada** del docente se dibuja con los turnos y sus recesos, pero **sin los eventos** del turno (acto cívico), que sí pinta el horario: la rejilla y el horario no coinciden. | `teacher_list_widget.cpp` (`construirRejillas` y `m_filas[…].receso`) frente a `_cuadricula`/eventos de `entrada_solver_builder.cpp`. |
| H14 | **Cascada de aulas y materias**: borrar un aula deja cursos apuntando a ella. *(La **distinción de causas** al borrar una materia se cierra en RF-6: plan, titular o suplente, con mensajes distintos.)* | `ServicioMaterias::eliminarMateria`; `ServicioAulas`/`cascada.cpp`; `subject_list_widget.cpp:507-517`. |
| H15 | Un **plan de un solo año** se lee como «global» por la heurística `cursosPlan.size() == 1 && curso ∈ 1..5`, que replica la carga en los años 1–5 (afecta a las horas sugeridas y a la composición). Con el centro real (planes de 5 años) no se manifiesta; **H4 hereda el defecto** al usar la misma función. | `entrada_solver_builder.cpp` (`horasSugeridasPorDocente`, rama `planGlobal`). |
| H16 | Las **ediciones de la visualización** (mover clases, arrastrar entre rejillas) se aplican solo en memoria: no se escriben en el archivo guardado ni en ningún otro sitio, y se pierden al cerrar. | `schedule_grid_dialog.cpp` + `schedule_model.hpp` (`applyMove`); `schedule_visualization_widget.cpp` solo carga. |

### Deuda de proceso de las pantallas (escrita, **no** cerrada)

Estas pantallas se fusionaron sin spec, plan ni tareas, y esta spec **no** las regulariza: solo deja
constancia para que no se pierdan.

| Elemento | Estado | Nota |
|---|---|---|
| CRUD de **planes de estudio** | Sin spec ni pruebas | `forms/plan_form_dialog.cpp`; ver H10. |
| **Menú sin cuenta** | Sin spec | Entrada de menú sin cuenta ni sesión; no se toca aquí. |
| **Ventana/leyenda** del horario | Sin spec | Rejilla que se reescala, pantalla completa, leyenda lateral; H16 (persistencia) sigue abierto. |
| **Disponibilidad y materias** del docente | Sin spec | Hoy en `teacher_list_widget`; los RF de la 019 solo regularizan **preferencias** (RF-10) y **cédula** (RF-1). |
| **002 RF-5** — aviso al cerrar con horario sin guardar | Sin implementar | Al quitar el guardado automático (RF-2) es relevante; **fuera de alcance** con justificación en `spec.md`. |

## Riesgos y arrastres

| # | Riesgo | Efecto | Tratamiento |
|---|---|---|---|
| 1 | **Esquema v7 sin migración** | Cualquier base v6 en uso (la del usuario y la del paquete demo) **deja de abrir**; restaurar un respaldo v6 tampoco sirve. | Es la política vigente (009/015/016) y el usuario la asume: mensaje que nombra la versión, acción «crear base nueva con respaldo» y aviso de que el respaldo anterior fallará; documentación y siembra del centro como vía de recuperación (RF-7). |
| 2 | **Aforo en el documento de visualización** | Un horario guardado antes del cambio no trae aforo: con P-3 se asumen aforo 1 y dos clases en la cancha vuelven a señalarse como conflicto. | Declarado en RF-8 y en los casos límite; se regenera el horario para obtener el documento nuevo. Nunca se adivina el aforo. |
| 3 | **Pantallas sin pruebas** | H1, H2, H4, H8 y H6 se editan sobre vistas y diálogos que hoy no tienen suite propia (solo `test_selector_generacion` y `test_conversion_entrada`). | RNF-1/RNF-8: la regla se resuelve en funciones puras o en la lógica y se prueba sin interfaz donde sea posible; las vistas se prueban con la infraestructura de `add_qtest` ya usada por `test_selector_generacion` (siembra, `QTest`, `QSettings` aislado). |
| 4 | **Quitar el guardado automático (H2)** | A partir de ahora todo horario generado es «no guardado» hasta que el usuario lo pida; **002 RF-5** (avisar al cerrar con trabajo sin guardar) pasa a ser relevante. | Declarado **fuera de alcance** en `spec.md` con su justificación y anotado aquí como arrastre de la 002; no se implementa en la 019. |
| 5 | **H4 sin tope fijo (flexibilidad por convenio)** | El sistema no avisa «por encima de 36 h»: el aviso es «más horas que las declaradas en aula». Quien espere las 36 h como validación no la encontrará. | Decisión del usuario (2026-11-10, P-5): las 36 h son el **valor sembrado de referencia** del centro, editable; el exceso se informa. La coherencia de la **005** (horas en aula ≤ horas requeridas) **no** se relaja: al subir las horas en aula por encima de las requeridas, el sistema sube las requeridas o pide ajustarlas. |
| 6 | **H1 deja un hueco de identidad** | Sigue sin poder darse de alta un docente con código `Pxxx`; solo se editan los sembrados. | Declarado en P-1; si el centro exige códigos alfanuméricos para altas nuevas, es otro RF (no en esta spec). |
| 7 | **H4 y H15 comparten función** | Si se arregla H4 (que usa `horasSugeridasPorDocente`) y no H15, un plan de un solo año seguiría dando sugerencias infladas. | Con el centro real (planes de 5 años) no se manifiesta; H15 queda en la deuda y se anota la dependencia. |
| 8 | **Recuentos de la suite** | Los tests nuevos cambian los recuentos (hoy 62/62 `backend`, 67/67 `full-release`, 2/2 lento). | RNF-1: línea base = todo verde y sin degradación; los recuentos se anotan al cerrar y **no** son requisito. |
| 9 | **Borrado de materia bloqueado (P-2)** | Un catálogo con suplencias exige quitar antes la asignación; puede parecer «no deja borrar». | Mensajes con la causa concreta (suplente + docente / titular / plan), que hoy no se distinguen (H14). |

## Supersesiones aprobadas (2026-11-10)

El usuario aprobó las **cinco notas** el **2026-11-10** y quedan **aplicadas** en las specs
correspondientes, fechadas `019 (2026-11-10)`:

1. **015 y 016** — «la versión de esquema vigente pasa a **7** por la 019 (materias suplentes); su
   política sin migración, su catálogo y su guardia no cambian».
2. **018** — «la **vista de horario y la detección de conflictos al mover clases** pasan a la **019**
   (RF-8/RF-9): el aforo se declara en el documento de visualización y la detección lo respeta». *(No
   incluye la estimación de ocupación: es H12, en deuda.)*
3. **002** — «la 019 cierra la parte de interfaz de RF-1/RF-4 (no guardar sin petición y confirmación de
   datos anteriores); **RF-5** (aviso al cerrar con trabajo sin guardar) sigue abierta y queda como
   arrastre declarado».
4. **004** — sin cambios: su definición de huella ya exige la confirmación que la 019 implementa.
5. **017** — «el **tope sembrado de 36 h** deja de ser un tope único y pasa a ser el **valor de
   referencia editable** de las horas en aula (convenios por encima o por debajo, sin imponerse como
   inicio de la tabla); el cálculo no se modifica y el valor sembrado no cambia».

`docs/constitution.md` **no se toca** en esta spec: no hay regla nueva del solver ni cambio de
principios. `MEMORY.md` **no** se ha editado (el planner solo escribe en `specs/`): al aprobar, el
coordinator añadirá la fila de la 019 y la deuda H7/H9–H16.

## Preguntas al usuario

**Ninguna.** **Q1** (referencia del exceso de horas de RF-4) quedó cerrada el **2026-11-10**: la
referencia son las **horas en aula declaradas por fila**, sin tope legal fijo, con las **36 h** como
**valor sembrado de referencia** del centro —no el inicio de la tabla ni un máximo— (P-5). El sistema
**no** bloquea por superar 36 h ni por declarar menos horas en aula que las sugeridas: solo informa el
exceso; la coherencia de la 005 se mantiene.

## Historial

- **2026-11-10 — spec redactada (borrador).** Se leen `AGENTS.md`, `MEMORY.md`, las specs 002, 004, 012,
  013, 015, 016 y 017 (y la 018 completa), `docs/interfaz-frontend.md` (§2.4.5, §3.1, §3.6) y
  `docs/constitution.md` (§3, §4, §5, §7), más el código de las ocho piezas afectadas
  (`teacher_form_dialog`, `teacher_list_widget`, `schedule_generation_widget`,
  `entrada_solver_builder`, `puerto_solver_ipc`, `schedule_model`/`schedule_grid_dialog`,
  `subject_list_widget`, `ServicioGeneracion`, `ServicioProfesor`, `ServicioMaterias`,
  `aplicacion_backend`, `seed/docentes`, `common/aforo_aula.hpp`). Se confirman los seis desajustes con su
  línea exacta y se comprueba que H2/H3/H4 restauran requisitos escritos (002 RF-1/RF-4, 004, 018) y que
  H1/H6/H8 son comportamiento de pantallas sin requisito previo. Se retira de la spec el detalle de
  implementación (queda aquí, en «Qué se comprobó en el código»). Se crean `spec.md` y este `estado.md`.
  **No** se ha escrito plan ni tareas y **no** se ha tocado código. Queda **Q1** abierta.
- **2026-11-10 — Q1 cerrada (flexibilidad por convenio).** El usuario responde a Q1 con su propia
  práctica: «si bien por ley las horas máximas en aula son 36, en el liceo suelen hacer convenios de más
  o menos horas». Se incorpora como **P-5**: la referencia del exceso son las **horas en aula declaradas
  por fila**, **sin tope fijo de 36 h ni validación legal**; el control admite valores por encima y por
  debajo y las 36 h quedan como **referencia informativa** del centro (017 D-3) *(lectura corregida en la
  entrada siguiente, B5: el 36 h es el valor sembrado de referencia, no el inicio de la tabla ni un
  máximo)*. Se actualizan en la spec
  las definiciones («Horas en aula declaradas», «Exceso», «Referencia legal de 36 horas»), **RF-4**
  (quitada la marca `[NECESITA ACLARACIÓN]`, redactado el comportamiento flexible y el criterio de
  aceptación con convenio de 40 h y con horas en aula por debajo de las sugeridas), **cuatro casos
  límite**, el criterio de finalización 6, **D-6** y la nueva **P-5**. `Dudas abiertas: ninguna`. Siguen
  **8 tareas previstas** (T4 ya no bloqueada) y **no** se ha escrito plan, tareas ni código.
- **2026-11-10 — QA de la spec aplicado (20 hallazgos, B1–B20).** Sin cambiar el alcance acordado ni el
  contenido de las decisiones del usuario. **B1** fija el alcance en **H1, H2, H3, H4, H6 y H8 + H5** y
  la deuda en **H7 y H9–H16** (nueve); **B2** mantiene la coherencia de la 005 (horas en aula ≤ horas
  requeridas) y aclara que el exceso se mide frente a las horas en aula; **B3** reescribe D-1 (deuda de
  proceso **escrita**, no cerrada); **B4** declara que de H10 solo se cierra la cédula y de H14 solo la
  distinción de causas; **B5** corrige la lectura de la 017 (36 h = valor sembrado de referencia, no el
  inicio de la tabla) y añade
  la supersesión propuesta; **B6** deja **002 RF-5** fuera de alcance con justificación; **B7** pone
  RF-5+RF-7 en T5 con apertura v6, base nueva, repoblación y documentación; **B8** añade la tabla H→RF;
  **B9** quita la promesa «mismo curso» de RF-9 y añade clase sin aula y aula desconocida; **B10** aclara
  que «conservar el estado anterior» es en pantalla (guardado atómico); **B11** reformula RNF-9;
  **B12** corrige H1 (se abre, falla el guardado); **B13** P-1…P-5; **B14** retira «estimación de
  ocupación» de la nota de la 018; **B15** criterio 5 «resultado no factible»; **B16** quita «8 workers»
  de RNF-7; **B17** declara el conjunto sin suplentes (tabla vacía); **B18** exige la regla
  titular-y-suplente en el servicio; **B19** aclara que la preferencia en hora no disponible es política
  de pantalla; **B20** lleva el listado de dependencias a los criterios y a T6. `Dudas abiertas:
  ninguna`. Sin plan, tareas ni código.
- **2026-11-10 — pasada editorial final (N1–N5).** Sin cambios de alcance ni de decisiones. **N1**: RF-9
  aclara que el «solo» del conflicto por ocupación vale «sin perjuicio del par cruzado y del mismo
  docente» y que la exención de la cancha exige **bloque completo** (se mantienen clase sin aula y aula
  desconocida). **N2**: el alcance se redacta como «del resto de H7 y H9–H16 no se implementa nada,
  salvo las dos partes declaradas de H10 (cédula, RF-1) y H14 (causas al borrar materias, RF-6)» en
  `spec.md`, en D-2/D-10 y en la nota de este estado. **N3**: el exceso es siempre **horas sugeridas
  frente a horas en aula** (se quita «o requeridas»), las dos columnas arrancan con las **horas
  sugeridas** y las 36 h quedan como referencia sembrada, nunca como inicio ni máximo. **N4**: queda
  dicho que la «ficha» regularizada es identidad/cédula y preferencias; la edición de disponibilidad y
  materias sigue en la deuda de proceso. **N5**: la sección de supersesiones reconoce que **H5 cambia la
  versión de esquema** y necesita nota en 015/016 (sin cambiar la política ni el valor sembrado) y cita
  **005/017/018** para H4. Sin plan, tareas ni código.
- **2026-11-10 — spec APROBADA; plan y tareas aplazados a mañana.** El usuario aprueba la spec **019**
  completa y las **cinco notas de supersesión** propuestas. `spec.md` pasa a **`aprobada (2026-11-10)`**;
  `Fase: clarificacion (spec aprobada; plan y tareas pendientes)`, con `Aprobaciones: spec 2026-11-10 ·
  plan/tareas —`. Las notas quedan **aplicadas** en **015**, **016**, **018**, **002** y **017**, fechadas
  `019 (2026-11-10)`, sin reescribir requisitos. Decisión del usuario: **el plan y las tareas se escriben
  mañana**; hoy no se crean `plan.md` ni `tasks.md` y no se toca código. `MEMORY.md` se intenta actualizar
  (fila de la 019 y nota de fase); si el permiso lo impide, el parche va en la entrega.
