# Estado 016

Fase: **cerrada** (**mitad 2 de la antigua 015**; spec **aprobada**, plan y tareas aprobados, **T1–T9 implementadas**, validación **APROBADA**)
Aprobaciones: **spec 2026-11-05** · **plan y tareas 2026-11-05** (el usuario ordenó seguir **sin parada de aprobación del plan**; los excesos de T1, T2, T4 y T7 quedan declarados y aceptados con esa indicación) · **implementación 2026-11-05** (T1–T7 y T9 por el implementer; **T8 por el planner**; suites **56/56** `backend` y **61/61** `full-release`) · **validación 2026-11-05** (**APROBADO**; notas menores aplicadas). Decisiones del usuario: **D-1 a D-10 (2026-11-05)**, dos decisiones nuevas del mismo día y la **división en dos specs (2026-11-05)**.
Base: **`bb1767af343c108b20de2b2b4de7ebe1d7658c2f`** (HEAD de `feat/Motor`) **+ la 015 aplicada en el árbol de trabajo, sin commits** (esquema **v6**; punto de partida de la 016: 55/55 `backend` y 59/59 `full-release`). La **016 está implementada en el mismo árbol, también sin commits** (tras ella: **56/56** y **61/61**). **`HEAD` sigue en `bb1767a`**: cuando la 015/016 se commiteen, el `Base` de referencia **pasa a ser ese sha**.
Dependencias: **015-tipo-general-en-el-esquema (mitad 1)**, **implementada y cerrada (2026-11-05)**. Cerradas e implementadas: 009-tipos-de-aula-y-eventos, 010-solver-tipos-y-eventos, 011-horas-academicas, 012-preferencias-docente y `009-agrupacion-bloques-materias` (S3).
Dudas abiertas: **ninguna**.
Siguiente paso: **nada pendiente en esta spec.** Fuera de ella: implementar la **013** sobre v6, escribir la **014**, **recrear la base real del usuario** (sigue en v5 y se rechazará al abrir) con respaldo y **decidir los commits** (la 015 y la 016 están en el árbol, sin commits).
Veredicto: **APROBADO (2026-11-05)** por el reviewer; notas menores de la validación aplicadas y suites **56/56** (`backend`) y **61/61** (`full-release`).
QA: el re-QA de la antigua 015 (**B1–B4**) se cerró con **B1, B3 y B4 en la 015** y **B2 aquí** (RF-2). **QA del plan (2026-11-05): CAMBIOS NECESARIOS (P1–P3 + ocho notas menores) aplicados** —registro del binario de frontend dentro del `if` y con su include, alcance de T8 por instantánea (el directorio no está bajo seguimiento), calificador «de la versión vigente» también en la rama de esquema de la apertura, líneas de los call sites, seis casos `v18_*`, `BaseInvalida` en el contrato de generación, `Causa` interna, montaje P10, T5/T7 repartidos y §2.4.3 sin descripción previa—. **Implementación (2026-11-05): T1–T9 ejecutadas; T8 por el planner; 56/56 y 61/61. Validación (2026-11-05): APROBADO**, con las notas menores aplicadas (comentarios v5→v6, aserción fuerte en `test_tipos_aula_ui` y evidencia de T8 en el `journal`).
Notas: ver «Implementado», «Deudas y notas», «Plan y tareas», «Qué cubre», «Respuesta al re-QA (B2)», «Decisiones heredadas», «Supersesiones», «Historial», «Riesgos» y «Tamaño previsto».

## Plan y tareas (2026-11-05)

- **`plan.md`** con la `Base` de arriba. Diseño:
  - **Comprobación única** `validez_base::comprobar` (`backend/database/comprobacion_validez`): decide la
    **causa** (`Valida`/`Version`/`Esquema`), devuelve el **mensaje** con causa y acción y **reutiliza
    `VersionEsquema::esquemaEsperado`** (la guardia de la 015) en una transacción de solo lectura. No hay
    una segunda guardia ni un recorrido de datos de dominio. **`Causa` es diagnóstico interno**: los tres
    consumidores usan solo `valida()`/`mensaje` (el fallo de lectura de versión se informa como `Version`).
    Al abrir se alinean **las dos ramas**: la de versión advierte de que un respaldo anterior volverá a
    fallar y la de esquema exige «restaurar un respaldo **de la versión vigente** o crear una base nueva»;
    se prueba que apertura y comprobación coinciden para el mismo defecto.
  - **Antes de generar**: `ServicioGeneracion` gana `Estado::BaseInvalida` y un puerto
    `std::function<QString()> revisarBase` (colocado **antes de `padre`** para que olvidarlo no compile);
    comprueba **antes** de validar la entrada y de enviar. La vista comprueba además **antes de componer**
    (para no atribuir el fallo a los datos) y `NucleoDatos` gana `comprobarValidezBase()`.
    `test_contrato_generacion` enumera también `BaseInvalida` (contrato observable completo).
  - **Antes de guardar**: `ServicioConfiguracionHoraria::guardar` comprueba antes del rango y del *upsert*;
    `consultar` no comprueba. El preset (archivo) queda fuera y se declara.
  - **Frontend**: `TiposAulaUI::idTipoGeneral` pasa a `Resultado<int>` y **nunca** devuelve otro tipo; el
    mensaje es único y las tres vistas abortan **antes** de abrir el diálogo (el combo caería al primer
    ítem) y antes de escribir en las altas rápidas de materia desde docente. El binario de prueba nuevo va
    **dentro de `if(TARGET frontend AND TARGET backend)`** y con `target_include_directories(... src/frontend/src)`
    (el target `frontend` no exporta esa ruta).
  - **V18**: solo el texto (convención + acción doble), mismo identificador y misma regla; V19 sigue libre.
    El contrato lo edita **T5**; T7 solo verifica.
  - **Huella**: ya excluye el catálogo; se fija con prueba y `grep`, sin tocar código.
  - **Documentación**: §2.2, §2.4.3, §2.4.4, §3.1 y §5 de `docs/interfaz-frontend.md`, la guía de
    migración y la verificación de la lectura nueva de V18 en el contrato (sin cambiar 2.0).
    `docs/constitution.md` intacto. **§2.4.3 no tenía descripción previa de la ayuda**: se añade, no se retira.
  - **013**: re-base documental (Base, restricción «Esquema (v6)» y nota de supersesión del ancla de
    RNF-4/criterio 11), **sin tocar sus requisitos y sin código**; el alcance se comprueba por
    **instantánea** de `git status` (el directorio no está bajo seguimiento).
- **`tasks.md`**: **T1–T9** en orden de dependencia, con «Hecho cuando» verificable y los dos comandos
  exactos del proyecto (`--preset backend` y `--preset full-release`). **T1, T2, T4 y T7 superan los
  30 min** y quedan declaradas.

## Implementado (2026-11-05)

**T1–T9 ejecutadas y en verde** (`tasks.md`): **56/56** `backend` y **61/61** `full-release`. Resumen por
tarea (el detalle y la evidencia están en `journal.md`):

| Tarea | Qué dejó |
|---|---|
| T1 | `validez_base::comprobar` (causa + mensaje, solo lectura, reutilizando `esquemaEsperado`) y las **dos ramas** de la apertura alineadas (versión con el aviso del respaldo anterior; esquema con «de la versión vigente») |
| T2 | `ServicioGeneracion` con `Estado::BaseInvalida` y `revisarBase`; la vista comprueba antes de componer; `NucleoDatos::comprobarValidezBase()`; `BaseInvalida` en el contrato de generación |
| T3 | `ServicioConfiguracionHoraria::guardar` rechaza con base inválida sin escribir; `consultar` sigue leyendo |
| T4 | `TiposAulaUI::idTipoGeneral` → `Resultado<int>` (nunca otro tipo) y las tres vistas abortan antes de abrir el diálogo o de escribir |
| T5 | Mensaje de V18 con convención y acción, identificador intacto; lectura nueva en el contrato |
| T6 | Huella sin el catálogo, fijada con prueba y `grep` (sin cambio de código) |
| T7 | Guía de la API al día (§2.2, §2.4.3, §2.4.4, §3.1, §5), deuda de la 009 atendida y guía de migración |
| T8 | **Re-base de la 013 sobre v6, hecha por el planner** (Base, «Esquema (v6)» y nota de supersesión del ancla), sin código |
| T9 | Regresión global y `journal.md` con la evidencia |

**Validación: APROBADO (2026-11-05).** El reviewer aprobó con notas menores, ya aplicadas: comentarios
v5→v6 alineados, aserción fuerte en `test_tipos_aula_ui` y evidencia de T8 en el `journal`. La spec pasa a
`implementada (2026-11-05)` y la fase a **cerrada**.

## Deudas y notas

Deudas aceptadas al cerrar (no bloquean; están declaradas para quien retome la zona):

- **`Causa::Version` también cubre un fallo de lectura de versión.** `leerVersionEsquema` devuelve `-1`
  cuando no puede leer `PRAGMA user_version`, y `comprobar` lo informa con `Causa::Version` y un mensaje
  propio («no se pudo leer la versión»). Es **diagnóstico interno**: ningún consumidor ramifica por
  `Causa` (solo usan `valida()`/`mensaje`). Si alguna vez se ramifica, separar `Causa::Lectura`.
- **`revisarBase` tiene valor por defecto (`{}`).** El único camino de producción
  (`schedule_generation_widget.cpp`) lo pasa; un llamador futuro que lo omitiera perdería la comprobación
  **antes de generar** (la apertura y el guardado tienen la suya). No es un fallo de la 016, pero conviene
  vigilarlo al añadir consumidores de `ServicioGeneracion`.
- **El mensaje de versión posterior (p. ej. v7) no está matizado.** La rama `Version` usa el mismo texto
  para anterior y posterior y dice «restaurar un respaldo de la versión anterior», que apunta al caso v5;
  no hay test con una base de versión **posterior** a la actual.
- **`cerrar()` no propaga un fallo de `commit()`.** El ayudante de transacción de `comprobar` cierra la
  lectura sin mirar el resultado: al ser una transacción **de solo lectura**, el valor leído se devuelve
  igual; no se ha considerado un fallo de validez.
- **Cobertura faltante en tests de la comprobación.** No hay caso de base con la **conexión cerrada** ni de
  base que se **rompe después de abrir** (p. ej. disparador borrado entre dos comprobaciones). El
  comportamiento es el esperado, pero no está fijado por prueba.
- **`docs/solver/Informe-Motor-Solver.md` sigue citando «V1–V13»** (pre-existente y fuera de alcance de la
  016; la 013 corregirá las citas del contrato y la constitución).
- **Coste de la comprobación: ≈ 1,03 ms** por llamada (base válida en memoria, 1000 iteraciones). Un clic
  de «Generar» paga **≈ 2 ms** por la doble capa (vista antes de componer + servicio antes de enviar): es
  deliberado (diagnóstico correcto y garantía para cualquier llamador).

## Qué cubre

**015 y 016 son las dos mitades de la antigua 015.** **La 015 precede a la 016**: la 015 deja la fila, la
protección y el predicado de **validez de la base**; esta spec hace que **se usen**:

- **RF-1** — cuándo se comprueba la validez (al abrir, antes de generar y antes de guardar la
  configuración) y qué ocurre en cada momento: no migrar, no reparar, no escribir, no calcular, no
  guardar; causa y acción separadas para versión y para objetos/fila/tabla retirada.
- **RF-2** — la interfaz **no sustituye** el tipo general: si no lo encuentra, falla con mensaje.
- **RF-3** — el **mensaje** de la validación del aula de referencia explica la convención y la acción; sin
  validaciones nuevas (el identificador siguiente queda para la 013).
- **RF-4** — el tipo general queda **fuera de la huella de datos**.
- **RF-5** — documentación de la **API del catálogo** (§2.4.3) y cierre de la **deuda de la 009** sobre la
  ayuda del tipo general.
- **RF-6** — coherencia con la **013**: `Base`, ancla de no regresión y restricción «Esquema (v5)».

**No entra aquí** la garantía estructural (fila, protección, siembra, validez y guardia v6): es de la
015, y esta spec no la redefine.

## Respuesta al re-QA (B2)

| # | Bloqueante | Cómo se cerró aquí |
|---|---|---|
| **B2** | El arreglo del frontend no estaba en ningún RF; solo en un caso y una nota | **RF-2 nuevo y verificable**: la operación **falla con mensaje** y **no usa otro tipo**; la ayuda deja de devolver el primero del catálogo; vale para toda alta que la use; RF-5 anota que la deuda de la 009 (RF-11/§2.4.3) queda **atendida**; el **criterio 2** apunta a RF-2 (antes apuntaba a un caso límite) |

Los otros tres (B1, B3 y B4) se cerraron en la 015: criterio 6 por **presencia**, tolerancia acotada a las
**tablas retiradas** y **doble condición** de la fila (nombre normalizado + clave almacenada).

## Decisiones heredadas (2026-11-05)

- **De la 015, sin reinterpretar:** D-1 (alta + disparadores + mensajes), D-2 (v6 y guardia por nombre),
  D-3 (sin migración), D-4 (renombrado prohibido), D-5 (borrado prohibido), D-6 (siembra), D-7 (API sin
  cambios), D-8 (rechazo al abrir sin reparar), D-9 (huella), D-10 (solo mensaje del aula de referencia).
- **Decisiones nuevas que se implementan aquí:**
  - **Cuándo se comprueba:** al abrir, **antes de generar** y **antes de guardar la configuración**, con
    una comprobación **barata y de solo lectura** → RF-1, RNF-2.
  - **Sin respaldo silencioso:** la interfaz falla en vez de usar otro tipo → RF-2.
- **Lo que no cambia:** modelo del solver, contrato 2.0, regla del aula de referencia, API del catálogo
  (firmas y campos) y huella de datos.

## Supersesiones y notas documentales (normativas; su anotación es tarea del plan)

- **013-preferencias-docente-solver:** **no se cambian sus requisitos de comportamiento** (qué hace con
  las preferencias, sus RF y sus criterios). **Sí se re-basa** su **`Base` `bb1767a`**, el **ancla de
  verificación de su RNF-4 y criterio 11** y la restricción **«Esquema (v5)»** de su plan al escenario
  posterior a la 015. **Nota de supersesión del ancla:** el ancla fijada en `bb1767a` queda **supersedida**
  por el escenario posterior a la 015; la 013 conserva su RNF-4 y su criterio, pero se verifican contra el
  ancla nueva. Se declara que **la 013 no tiene huella de esquema propia**: la única congelada está en
  `test/backend/test_version_esquema.cpp` y la actualiza la 015. **Se aplica en T8 y lo hace el planner**
  (documental, sin código); el implementer solo verifica.
- **009-tipos-de-aula-y-eventos, RF-11 y §2.4.3 de la guía:** la deuda sobre la ayuda del tipo general
  queda **atendida**; la ayuda ya no sustituye el tipo.
- **010-solver-tipos-y-eventos y 005:** la regla y el identificador de la validación del aula de
  referencia **no cambian**; solo el texto del mensaje y su documentación en el contrato. **No hay
  supersesión de requisito.**
- **Documentación viva:** `docs/interfaz-frontend.md` §2.4.3 y `docs/contrato-solver-2.0.md`. La
  **versión de esquema y la guardia** las documenta la **015**.

## Historial

- **2026-11-05 — apertura de la 016 (planner)** como **mitad 2** de la antigua 015, tras la decisión de
  dividirla al cerrar el re-QA. Seis RF, seis RNF, casos límite y criterios; **sin plan ni tareas**. El
  bloqueante **B2** (arreglo del frontend sin RF) se cierra aquí con **RF-2**; los otros tres (B1, B3,
  B4) quedaron cerrados en la 015. Ambas specs en **borrador** y con dudas cerradas.
- **2026-11-05 — spec APROBADA.** El usuario aprueba la 016 junto con la 015 y ordena seguir hasta el plan
  **de la 015** sin parada de aprobación. Esta spec queda **aprobada** y con la fase **plan pendiente**:
  su `plan.md`/`tasks.md` se escriben **después de implementar la 015**, sobre la `Base` de cierre y el
  esquema ya en v6.
- **2026-11-05 — 015 cerrada (APROBADO) y plan/tareas de la 016 escritos.** La 015 queda implementada en el
  árbol (**esquema v6**, 55/55 y 59/59) **sin commits**: `HEAD` sigue en `bb1767a`, y el `Base` de la 016
  se fija a ese HEAD **+ la 015 aplicada** (si se commitea antes, al sha de cierre). `plan.md` fija la
  comprobación única (`validez_base::comprobar` sobre el predicado de la 015), los tres momentos, la firma
  estricta de `idTipoGeneral` (`Resultado<int>`), el mensaje de V18, la huella sin cambio de código, la
  documentación y la re-base de la 013. `tasks.md` lista **T1–T9**; **T1, T2, T4 y T7 se declaran por encima
  de 30 min** y se aceptan con la indicación del usuario de seguir sin parada de aprobación del plan. **La
  implementación no ha comenzado**: no hay casillas marcadas.
- **2026-11-05 — QA del plan: P1–P3 y notas menores aplicadas.** **P1**: el registro de
  `test_tipos_aula_ui` va dentro de `if(TARGET frontend AND TARGET backend)` y con
  `target_include_directories(... src/frontend/src)` (el target `frontend` no lo exporta; el preset
  `backend` no debe verlo). **P2**: el alcance de T8 se comprueba por **instantánea** de `git status`
  (`specs/013-…/` no está bajo seguimiento; `git diff --stat` daba un falso positivo). **P3**: la rama de
  **esquema** de la apertura también exige «un respaldo **de la versión vigente**», con su aserción.
  Notas: líneas corregidas de los call sites (286/376/407/468), **seis** casos `v18_*`, `BaseInvalida` en
  `test_contrato_generacion`, `Causa` como diagnóstico interno (consumo por `valida()`/`mensaje`), montaje
  **P10** de la 015 en el caso «sin la fila del general», contrato editado por **T5** y solo verificado por
  T7, §2.4.3 **sin descripción previa** que retirar, y `Base` explícita si la 015 se commitea. **Siguen
  nueve tareas y los totales 56/56 y 61/61.**
- **2026-11-05 — T1–T9 implementadas; fase de validación.** El implementer cierra **T1–T7 y T9** y el
  **planner** ejecuta **T8** (re-base de la **013** sobre v6: `Base`, restricción «Esquema (v6)» y nota de
  supersesión del ancla de RNF-4/criterio 11) con la verificación por instantánea. Suites **56/56**
  (`backend`) y **61/61** (`full-release`). La fase pasa a **validación**, el veredicto queda **pendiente** y
  **la spec no se marca `implementada`** hasta el `VEREDICTO: APROBADO`. `HEAD` sigue en `bb1767a`: 015 y 016
  están en el árbol **sin commits**.
- **2026-11-05 — cierre: VEREDICTO APROBADO.** El reviewer aprueba la 016 con **notas menores ya
  aplicadas**: comentarios de esquema **v5 → v6** alineados, **aserción fuerte** en `test_tipos_aula_ui` y
  **evidencia de T8** en el `journal`. Suites **56/56** (`backend`) y **61/61** (`full-release`). La spec
  pasa a `implementada (2026-11-05)` y la fase a **cerrada**. Quedan las deudas de «Deudas y notas» (Causa
  de lectura, valor por defecto de `revisarBase`, mensaje de versión posterior, `commit()` de la lectura,
  cobertura de conexión cerrada/base rota, cita V1–V13 del informe del motor y coste ≈1,03 ms). **Sin
  commits:** la 015 y la 016 viven en el árbol de trabajo.

## Orden entre specs (vigente)

1. **015** — garantía estructural, esquema **v6**. **Implementada y cerrada (2026-11-05), sin commits.**
2. **016** (esta) — comprobación y cierre. **Cerrada e implementada (2026-11-05)**; validación **APROBADA**; sin commits.
3. **013** — preferencias del docente (aprobada, sin implementar). **Re-baseada a v6 en T8 (2026-11-05)**
   por el planner (Base, ancla de RNF-4/criterio 11 y restricción «Esquema (v6)»).
4. **014** — materias en bloques de dos horas (planificada, sin escribir).

## Riesgos declarados

- **Un camino sin comprobar.** La validez debe comprobarse en **tres** puntos; si el guardado de
  configuración es un archivo y no la base, el plan debe fijar dónde se comprueba.
- **La interfaz no distingue nombre y clave.** Publica id y nombre; la ayuda compara el nombre. Como el
  renombrado está prohibido, coincide en la práctica; con una fila de clave válida y otro nombre la
  guardia invalidaría la base (015) y la ayuda **fallaría** en vez de sustituir el tipo (RF-2).
- **Mensaje del almacenamiento.** Si algún camino del frontend escribe directamente, verá el texto del
  disparador (en español) o un error genérico; la 015 fija la traducción y aquí se verifica en los tres
  momentos.
- **La 013 condicionada.** Quedó **re-baseada** en **T8 (2026-11-05)** por el **planner** (Base, restricción
  «Esquema (v6)» y nota de supersesión del ancla de RNF-4/criterio 11); el implementer solo verificó con
  `grep` y la instantánea de `git status`. No se tocó código de la 013.
- **Coste de la comprobación.** Debe ser barata y de solo lectura (RNF-2) para poder ejecutarse en cada
  apertura, generación y guardado; si el plan la convierte en un recorrido de datos, se incumple.
- **Cierre de la deuda de la 009.** Retirar la descripción anterior de la ayuda (§2.4.3) sin dejar dos
  verdades en la guía.

## Tamaño previsto

**Seis RF en 9 tareas** (`tasks.md`), en orden de dependencia y **sin afirmar que todas caben en 30 min**:

| Tarea | Contenido | Estimación |
|---|---|---|
| T1 | Comprobación única de validez + causa de versión al abrir | **> 30 min (declarado)** |
| T2 | Antes de generar: servicio, fachada y vista | **> 30 min (declarado)** |
| T3 | Antes de guardar la configuración de la hora académica | 20–30 |
| T4 | Frontend sin respaldo silencioso | **> 30 min (declarado)** |
| T5 | Mensaje de V18 + lectura en el contrato | 20–30 |
| T6 | Huella: pruebas y `grep` (sin código) | 15–20 |
| T7 | Documentación de la API y deuda de la 009 | **> 30 min (declarado)** |
| T8 | Re-base de la 013 (**planner**, sin código) | 25–30 |
| T9 | Regresión global | 20–30 |

Se mantiene el límite de **10 tareas**, con una de holgura para el QA. **Los excesos de T1, T2, T4 y T7
quedan declarados y aceptados** con la indicación del usuario del 2026-11-05 de seguir sin parada de
aprobación del plan. Si el QA de la implementación añade trabajo, el primer re-corte es **unir T5 y T6**
(una sola pieza, pruebas en binarios existentes); si aún no cabe, se **propone dividir la spec**.

## Notas

- **El planner no escribe fuera de `specs/`.** El parche de `MEMORY.md` se entrega al **coordinator**; su
  estado de aplicación se indica en la respuesta.
- **Las supersesiones las redacta el planner**; el implementer solo verifica. La re-base de la **013** fue
  **T8**, la ejecutó el **planner** y el implementer la verificó con los `grep` y la instantánea.
- **La 016 no toca código de la 013 ni de la 014.** T8 se acotó a `specs/013*/`.
- **La 015 y la 016 siguen sin commitear**: el trabajo de las dos mitades vive en el árbol. Cuando se
  commiteen, el `Base` de referencia pasa a ser el sha de cierre.
