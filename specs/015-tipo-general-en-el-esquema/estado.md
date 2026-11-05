# Estado 015

Fase: **cerrada** (**mitad 1 de la antigua 015**; spec aprobada, plan y tareas aprobados, **T1–T8 implementadas**, validación **APROBADA**)
Aprobaciones: **spec 2026-11-05** · **plan y tareas 2026-11-05** (el usuario aprobó la spec y ordenó seguir sin parada de aprobación del plan) · **implementación 2026-11-05** (T1–T8) · **validación 2026-11-05** (APROBADO; correcciones V1–V3 aplicadas). Decisiones del usuario: **D-1 a D-10 (2026-11-05)**, dos decisiones nuevas del mismo día y la **división en dos specs (2026-11-05)**.
Base: **`bb1767af343c108b20de2b2b4de7ebe1d7658c2f`** (base de la implementación; HEAD de `feat/Motor` al planificar, árbol limpio, esquema vigente **v5** en la base, contrato vigente **2.0**). Dependencias cerradas e implementadas: 009-tipos-de-aula-y-eventos, 010-solver-tipos-y-eventos, 011-horas-academicas, 012-preferencias-docente y `009-agrupacion-bloques-materias` (S3). Esquema resultante: **v6**. **Sin commits:** el trabajo quedó en el árbol de trabajo; el detalle por tarea está en `journal.md`.
Dudas abiertas: **ninguna**.
Siguiente paso: **nada pendiente en esta spec.** Fuera de ella: escribir el plan de la **016** (mitad 2) sobre el esquema **v6**, replanificar la **013** sobre v6, y **recrear la base real del usuario** (está en v5 y se rechazará al abrir) con respaldo.
Veredicto: **APROBADO (2026-11-05)** por el reviewer; correcciones de la validación (V1–V3) aplicadas y suites **55/55** (`backend`) y **59/59** (`full-release`).
QA: spec: CAMBIOS NECESARIOS (2026-11-05) → H1–H16 corregidos; **re-QA: B1–B4**, cerrados **B1, B3 y B4 en esta mitad** y **B2 en la 016** (el arreglo del frontend es de la mitad 2). **QA del plan (2026-11-05): P1–P12 aplicados.** **Validación (2026-11-05): APROBADO** con V1–V3 aplicadas.
Notas: ver «Implementado», «Deudas y notas», «Reparto de la división», «Decisiones cerradas», «Historial», «Riesgos» y «Tamaño previsto».

## Implementado

- **T1** — **definición única** en `common/tipo_general.hpp` (nombre + normalización + tres literales y seis comentarios barridos), **alta del tipo general en la creación** (atómica, idempotente, `INSERT OR IGNORE`) y **marca de esquema v6**.
- **T2** — **disparador de borrado** (`TiposAula_general_no_borrar`): el tipo general no se elimina por ninguna vía.
- **T3** — **disparador de renombrado** (`TiposAula_general_no_renombrar`): no cambia nombre ni clave; un `UPDATE` que no toca ninguno no se bloquea.
- **T4** — **predicado puro de la fila** (`evaluar`: sin tipo, varias filas, clave distinta, válida) y **`traducirRechazo`** puro.
- **T5** — **guardia v6**: catálogo por nombre con índice y los dos disparadores, la fila, ausencia de tablas retiradas, tolerancia; comprobación en **transacción de solo lectura**; huella congelada ampliada a índice y disparadores.
- **T6** — **mensajes del catálogo**: prioridad «es el tipo general» sobre los dependientes, rechazo total del renombrado y traducción del rechazo del almacenamiento, en español.
- **T7** — **siembra**: reutiliza el tipo general, la base nueva cuenta como vacía, el reemplazo total lo conserva, recuento **3** leído de la base y perfil sin declararlo.
- **T8** — **documentación**: versión y guardia al día (`interfaz-frontend.md`, `migracion-frontend.md`) y supersesiones verificadas en 001/004/008/009/010/011/012; regresión completa.

## Deudas y notas

- **Cuerpo del disparador.** No forma parte de la **validez** (la guardia compara presencia por nombre); sí entra en la **huella de release** del test de esquema. Reescribir un disparador conservando el nombre no invalida la base.
- **Normalización duplicada.** Los dos envoltorios `claveTipo` (servicio y cálculo) siguen **delegando** en `common/tipo_general.hpp`: una sola implementación del algoritmo, con dos firmas heredadas. Duplicación **aceptada** (el requisito cubre el literal, no el algoritmo).
- **Cobertura pendiente en tests.** Faltan dos casos: «un disparador **reformateado** sigue siendo válido» (el cuerpo no es identidad) y «**objetos de más**» más allá del índice extra ya probado (p. ej. un disparador adicional).
- **Coordinación con la 016.** La 016 hereda parte de la **documentación de conducta del catálogo** (`docs/interfaz-frontend.md` §2.4.3). Al escribir su plan hay que coordinarlo para que su **RF-5** no la duplique ni la contradiga.
- **Base real del usuario.** Está en **v5** y **se rechazará al abrir**: no hay migración (D-3). Hay que **recrearla con respaldo** y reintroducir tipos, aulas, materias y cursos.

## División (decisión del usuario, 2026-11-05)

**015 y 016 son las dos mitades de la antigua 015.** **La 015 precede a la 016.**

- **015 — garantía estructural (esta).** Todo lo sin lo cual el producto se rompe en cuanto se crea la
  base: **fila en la creación, identidad/unicidad, borrado prohibido, renombrado prohibido, convivencia
  con el seed, validez de la base y guardia v6**, más la **documentación de la versión y de la guardia** y
  las **supersesiones que la propia garantía deja obsoletas**.
- **016 — comprobación y cierre** (`specs/016-tipo-general-comprobacion-y-cierre/`). **Cuándo se
  comprueba** (al abrir, antes de generar y antes de guardar la configuración), el **frontend sin
  respaldo silencioso**, el **mensaje de la validación del aula de referencia**, el **tipo general fuera
  de la huella de datos**, la **documentación de la API** y las **supersesiones restantes de la 013**.

### Reparto final de RF

| Antigua 015 | Dónde queda | Motivo |
|---|---|---|
| RF-1 existencia en la creación | **015 RF-1** | sin ella no hay garantía |
| RF-2 identidad, unicidad y definición única | **015 RF-2** | idem |
| RF-3 borrado prohibido | **015 RF-3** | idem |
| RF-4 renombrado prohibido | **015 RF-4** | idem |
| RF-5 convivencia con el seed | **015 RF-5** | rompe la siembra en cuanto se crea la base |
| RF-6 validez y guardia v6 | **015 RF-6** | es el predicado que la 016 consume |
| RF-7 «cuándo se comprueba» (al abrir) | **mitad en 015 RF-6 y mitad en 016 RF-1** | la 015 fija **qué es** válido; la 016, **cuándo** se exige y **qué ocurre**. Sin la 015, la 016 no tendría predicado; sin la 016, la 015 no tendría consecuencias explícitas |
| RF-7 (antes de generar y antes de guardar) | **016 RF-1** | no cambia la estructura de la base |
| RF-8 mensaje del aula de referencia | **016 RF-3** | no cambia la estructura; reserva V19 para la 013 |
| RF-9 huella de datos | **016 RF-4** | no cambia la estructura |
| RF-10 servicios y mensajes | **015 RF-7** (servicios y mensajes del catálogo) | son la cara visible de la protección |
| RF-10 documentación de la API y deuda de la 009 | **016 RF-5** | es documentación de consumo |
| Respaldo silencioso del frontend (era caso y nota) | **016 RF-2 (RF nuevo)** | **B2**: exigía un RF verificable |
| Supersesiones de la 013 | **016 RF-6** | planificación posterior |
| Supersesiones de la 008 y 009 RF-10 (seed) | **015** (no estaban en la lista del reparto) | **por dependencia**: la 015 implementa la convivencia con la siembra; si sus supersesiones viviesen en la 016, la 015 dejaría la 008 contradiciendo el código |

## Respuesta al re-QA (B1–B4)

| # | Bloqueante | Mitad | Cómo se cerró |
|---|---|---|---|
| **B1** | Criterio 6 contradictorio | **015** | Criterio 6 reescrito: «dos bases que difieran en la **presencia** de los objetos exigidos o de la fila no corresponden igual al esquema esperado». RF-6 aclara que, si la huella congelada del test incluye el **cuerpo**, es una **guardia de release** y **no** parte de la validez. Se retiró la formulación «difieran en esos objetos» a secas |
| **B2** | El arreglo del frontend no estaba en ningún RF | **016** | **016 RF-2** nuevo y verificable: al preparar un alta sin el tipo general, la operación **falla con mensaje y no usa otro tipo**; la ayuda deja de devolver el primero del catálogo. El criterio 2 de la 016 apunta a RF-2 (antes apuntaba a un caso) y RF-5 anota que la deuda de la 009 (RF-11/§2.4.3) queda **atendida** |
| **B3** | «Los objetos de más se toleran» aceptaba las tablas retiradas | **015** | Tolerancia acotada en la definición de validez, en la de guardia y en RF-6: **salvo las tablas retiradas, que siguen invalidando** (004 RF-6 y 009 RF-9, que **no** se superseden). Caso límite propio y criterio 6 actualizado |
| **B4** | La identidad tenía dos lecturas y la spec decía que eran la misma | **015** | Predicado con **una sola formulación**: «existe **exactamente una** fila cuyo **nombre visible normalizado** sea `aula de estudio` **y esa fila (la única) tiene la clave almacenada** `aula de estudio`». **Caso límite corregido:** con la fila canónica presente, una segunda fila con nombre canónico y clave `AULA DE ESTUDIO` **falla la condición de fila única** (no la de clave); la de clave cubre el caso de una **sola** fila con nombre canónico y clave distinta. Se retiró la frase «la misma comparación» y se declaró en riesgos que contrato/V18 y guardia miran cosas distintas y que ese predicado es lo que las alinea |

## Decisiones cerradas (2026-11-05)

- **D-1 → Alta en la creación + disparadores + mensajes en el servicio**, con **una sola definición del literal** en el código de producción.
- **D-2 → Esquema v6 con guardia ampliada**: presencia **por nombre** del índice y de los dos disparadores, además de tablas y columnas.
- **D-3 → Sin migración**: una base v5 (o distinta de v6) se rechaza; base nueva con respaldo.
- **D-4 → Renombrado prohibido del todo**; disparador para toda escritura y servicio que rechaza cualquier petición.
- **D-5 → Borrado prohibido siempre**, con prioridad del motivo «es el tipo general».
- **D-6 → Seed**: reutiliza el tipo general, el chequeo de «base con datos» lo excluye, el reemplazo total lo conserva, el perfil ya no lo declara y el recuento es **3** leído de la base.
- **D-7 → Sin cambios en la API del catálogo** (firmas y DTO).
- **D-8 → Rechazo al abrir** si la base no es válida; la apertura no repara ni escribe.
- **D-9 → Tipo general fuera de la huella de datos**.
- **D-10 → Solo mejora del mensaje de la validación del aula de referencia**, sin validación nueva.
- **Decisión nueva 1 — validez:** presencia **por nombre** + **la fila**; el **cuerpo** no es identidad; objetos extra tolerados (**salvo tablas retiradas**, B3); automáticos no exigidos; **distinción en la guardia**, no solo en la huella del test.
- **Decisión nueva 2 — cuándo:** al abrir, antes de generar y antes de guardar, **sin reparar** y **sin respaldo silencioso** en el frontend (esta segunda parte se implementa en la 016).

## Supersesiones (normativas; su anotación es tarea del plan)

- **Global:** toda descripción del esquema esperado como «tablas y columnas» (o «tablas y columnas más la marca») queda supersedida por el predicado de **015 RF-6**.
- **001:** «esquema esperado (catálogo de tablas/columnas y marca de versión)».
- **004, RF-6 y criterio de cierre:** + índice, dos disparadores y la fila; **se mantiene** la ausencia de tablas retiradas.
- **009:** RF-5 (renombrado/borrado incondicionales) para el tipo general + alta por duplicado; definición «Esquema esperado» y RF-9; definición «Catálogo de tipos de aula»; RF-10 (siembra).
- **010:** «sin cambio de esquema» y «el nombre que debe usar la semilla»; se mantienen convención, regla del aula de referencia y contrato 2.0.
- **011 y 012:** marca de esquema y descripción del catálogo → v6 y predicado nuevo.
- **008 (por dependencia):** «Base vacía», RF-10 y RF-12 (aplicación inicial, reemplazo y recuentos).
- **013:** se ajusta en la **016** (allí se planifica sobre v6).

## Historial

- **2026-11-05 — apertura de la spec (planner)** con 9 RF y 10 dudas abiertas; sin plan ni código.
- **2026-11-05 — dudas D-1 a D-10 resueltas**; spec reescrita sin condicionalidad y con RF de guardia y de coherencia documental.
- **2026-11-05 — QA: CAMBIOS NECESARIOS (H1–H16)**; incorporados los dieciséis hallazgos y las dos decisiones nuevas.
- **2026-11-05 — re-QA: CAMBIOS NECESARIOS (B1–B4) y división en dos specs.** El usuario decide **dividir la 015**: esta spec es la **mitad 1** (garantía estructural, 7 RF) y se abre la **016** (comprobación y cierre, 6 RF). Se cerraron **B1, B3 y B4** aquí (criterio 6 por presencia, tolerancia acotada a las tablas retiradas, doble condición de la fila) y **B2** en la 016 (RF-2 verificable para la interfaz). La 015 queda en **borrador**, pendiente de aprobación.
- **2026-11-05 — QA del plan: P1–P12 aplicados.** P1: T1 barre los **3 literales** y reescribe los **6 comentarios** (el `grep` excluye comentarios y se repite en T8); P2: `test_servicio_carga_horaria` y `test_servicio_profesor` entran en T1; P3: los tres casos de renombrado/borrado en uso pasan a un tipo especializado y `test_servicio_tipos_aula` entra en el `-R` de T3; P4: la traducción del rechazo se extrae a `traducirRechazo` (puro) y el disparador se declara solo para escrituras directas; P5: la guardia corre en una transacción de solo lectura; P6: las notas de supersesión las redacta el planner (aplicadas en 001/004/008/009/010/011/012) y T8 solo verifica; P7: el struct `kAulas` pasa a `QString`; P8: se separa la línea 96 de `test_nucleo_datos`; P9: el fallo a medias afirma «no hay tablas nuevas»; P10: los montajes de la fila hacen `DROP TRIGGER` antes de mutar; P11: el `grep` de T8 cubre las frases de versión de `interfaz-frontend.md`; P12: dos variantes (fija del disparador y con valor del servicio).
- **2026-11-05 — spec APROBADA; `plan.md` y `tasks.md` escritos sin parada de aprobación.** El usuario aprueba la 015 y la 016 y ordena seguir hasta el plan. `plan.md` fija la **Base `bb1767a`**, el diseño de la definición única (`common/tipo_general.hpp`), el alta en la creación, los dos disparadores, el predicado puro, la guardia v6 con huella extendida y la siembra; `tasks.md` lista **T1–T8** con trazabilidad y comandos. Exceso declarado de T1, T5, T6, T7 y T8 (30 min) y **aceptado** con esa indicación. La implementación **no ha comenzado**.
- **2026-11-05 — T1–T8 implementadas y en verde; validación con CAMBIOS NECESARIOS.** La 015 se implementa completa (definición única, alta en la creación, dos disparadores, predicado, guardia **v6** con huella extendida, mensajes y siembra) y la suite queda en verde. El reviewer devuelve **CAMBIOS NECESARIOS**. En esta fase el planner corrige **solo los de coherencia documental/spec**: **H3** (RF-1 y criterio 1: si la creación falla, no se reintenta sobre el archivo residual; se descarta y se crea una base nueva con respaldo), **H4** (este `estado.md`: fase **validación**, veredicto de la validación y historial al día) y **H5** (criterio del tercer `grep` de T8: `Anotado por la 015`, siete archivos). El resto de los hallazgos los aplica el implementer. La spec **no** se marca implementada hasta el APROBADO.
- **2026-11-05 — cierre: VEREDICTO APROBADO.** El reviewer aprueba la 015 tras aplicar las correcciones V1–V3 (mensaje del fallo de creación sin esquema parcial y su test, rollback real probado y `detalle` de apertura con objetos, fila y retiradas). Suites **55/55** (`backend`) y **59/59** (`full-release`). La spec pasa a `implementada (2026-11-05)` y la fase a **cerrada**. Sin commits: el trabajo quedó en el árbol de trabajo. Quedan las deudas de «Deudas y notas» (cuerpo del disparador, envoltorios `claveTipo`, dos casos de test, coordinación con la 016 y recreación de la base del usuario).

## Orden entre specs (vigente)

1. **015** (esta) — garantía estructural, esquema **v6**. **Siguiente a implementar.**
2. **016** — comprobación y cierre. Después de la 015.
3. **013** — preferencias del docente (aprobada, sin implementar). **Se replanifica sobre v6** tras la 015/016 (`Base`, ancla de no regresión y restricción «Esquema (v5)»).
4. **014** — materias en bloques de dos horas (planificada, sin escribir).

## Riesgos declarados

- **La base real del usuario (v5) se rechazará al abrir.** No hay migración; el contenido es mínimo y se recrea con respaldo, pero el respaldo no devuelve el trabajo de rehacer tipos, aulas, materias y cursos.
- **Cuerpo del disparador fuera de la identidad (aceptado).** Reemplazar un disparador por otro del mismo nombre y distinto cuerpo no se detecta.
- **Identidad con dos lecturas (B4).** El contrato y V18 comparan el **nombre** normalizado; la guardia exige **nombre + clave**. La doble condición las alinea; si el contrato cambiara a comparar la clave, habría que revisar la decisión.
- **Duplicado por escritura directa.** Una clave sin normalizar puede convivir; la validez exige la clave correcta, así que esa escritura deja la base inválida, pero la unicidad Unicode no se garantiza (límite de la 009).
- **Normalización duplicada.** RNF-5 cubre el literal, no el algoritmo.
- **Definición única y capas.** Hoy el literal está en el cálculo, el frontend y el perfil del seed; unificarlo exige decidir dónde vive sin invertir dependencias entre estructura, cálculo e interfaz.
- **Definición única y DDL.** El literal debe llegar al texto de los disparadores sin copiarse: el plan debe interpolar la definición o justificar la excepción con el criterio ejecutable.
- **Recuentos del seed en 3.** El perfil deja de declarar «Aula de estudio»; la comprobación de recuentos debe cambiar de fuente junto con el perfil o la siembra revierte entera.
- **Centros con otro nombre para el tipo ordinario.** Con el renombrado prohibido no hay vía de arreglo por el catálogo.
- **Dos mensajes para el mismo rechazo.** La traducción del rechazo del almacenamiento debe cubrir todos los caminos del producto.
- **Numeración.** Sigue el hueco de la 014.

## Tamaño previsto

**Siete RF en 8 tareas** (`tasks.md`), con esta estimación y **sin afirmar que todas caben en 30 min**:

1. **T1 — Definición única del literal + alta del tipo general en la creación + marca v6.** **Por encima de 30 min.**
2. T2 — Disparador de borrado. 20–30 min.
3. T3 — Disparador de renombrado. 20–30 min.
4. T4 — Predicado puro de validez de la fila. 25–30 min.
5. **T5 — Guardia: catálogo con índice y los dos disparadores, motivos y huella extendida.** **Por encima de 30 min.**
6. **T6 — Mensajes del servicio y traducción del rechazo del almacenamiento.** **Por encima de 30 min.**
7. **T7 — Siembra.** **Por encima de 30 min.**
8. **T8 — Documentación, supersesiones y regresión.** **Por encima de 30 min.**

Se mantiene el límite de **10 tareas**. **Los excesos de T1, T5, T6, T7 y T8 quedan declarados y aceptados** con la indicación del usuario del 2026-11-05 de seguir sin parada de aprobación del plan. Si el QA de la implementación añade trabajo, primero se **re-corta dentro del límite** fusionando T2 y T3 y, si aún no cabe, se **propone dividir** la spec.

## Notas

- **El planner no escribe fuera de `specs/`.** El parche de `MEMORY.md` se entrega al **coordinator**; su estado de aplicación se indica en la respuesta.
- **Las supersesiones las redacta el planner** cuando la spec se apruebe; el implementer solo verifica.
- **Nada de la 013 ni de la 014 se toca aquí.**
