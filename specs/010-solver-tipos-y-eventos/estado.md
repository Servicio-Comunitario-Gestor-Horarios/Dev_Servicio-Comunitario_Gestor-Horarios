# Estado 010

Fase: cerrada
Aprobaciones: spec 2026-10-23 · plan/tareas 2026-10-25 · **spec del cambio de requisitos: 2026-10-26**
  (con la **enmienda de RF-6 del mismo día**: excepción de precedencia de (3) sobre (2)) ·
  **plan/tareas de la parte afectada: 2026-10-26** · **implementación y validación: 2026-10-27**
Base: **49d39dc** (HEAD al escribir este plan; T1–T10 en `0096b36`, `3a3e724`, `d2fbda1`, `4142939`,
  `0edc6f8`, `06fc70e` (P4), `37cd2e1`, `f054b2e` (T7), `4134209`, `9c5c301`, `35cd5f0`;
  correcciones de validación `aa7ee74`, `232bd19`, `45ac292`, `5fc40e0`, `de61050`; docs `d7728e6`)
Dudas abiertas: ninguna. La precedencia de (3) sobre (2) quedó **aprobada** el 2026-10-26 como
  excepción de RF-6 (ver «Excepción de precedencia (2026-10-26)»).
Siguiente paso: **nada pendiente en esta spec: cerrada.** Fuera de ella quedan pendientes que **no son
  trabajo de la 010**: las **deudas del frontend** de la 011 y de la 012
  (`docs/interfaz-frontend.md` §6), la **divergencia con `release/1.0`** y el **PR a `develop`**.
Veredicto: **APROBADO (2026-10-27) de la segunda entrega (T11–T16)** por el reviewer. La primera
  (T1–T10) ya estaba validada salvo **RF-6**, que es lo que se reescribió en esta entrega. Con este
  veredicto la 010 **queda implementada** (`spec.md`: `implementada (2026-10-27)`).
QA: APROBADA con las notas del re-QA aplicadas el 2026-10-23; spec aprobada por el usuario el 2026-10-23.
QA del plan: escrito el 2026-10-24, ajustado el 2026-10-25 (RF-1 con `Resultado<QJsonObject>`) y
  corregido tras el QA del plan (11 hallazgos, sección propia); aprobado por el usuario el 2026-10-25
  junto con `tasks.md`.
Notas: T1–T10 implementadas y commiteadas (T7 = `f054b2e`, desviación autorizada de P4 = `06fc70e`).
  Sobre el cálculo de causas se aplicaron **cinco correcciones sucesivas** (`aa7ee74`, `232bd19`,
  `45ac292`, `5fc40e0`, `de61050`): parches sobre una heurística aritmética, justo lo que el usuario
  quiere evitar; de ahí el cambio de requisitos. Detalle en «Cambio de requisitos (2026-10-26)»,
  «Validación 6 · defecto abierto» y «Correcciones 1-5».

## Cierre (2026-10-27)

**Veredicto del reviewer: APROBADO (2026-10-27)** para la **segunda entrega** (T11–T16). La primera
(T1–T10) ya estaba validada salvo **RF-6**, que es justo lo que se reescribió aquí. Con el veredicto, la
010 queda **implementada** y esta fase, **cerrada**.

| Tarea | Commit | Qué dejó |
|---|---|---|
| T11 | `227eacd` | `FranjaSemana` y las funciones puras de demanda/capacidad en `contrato_solver`; borrado de las dos copias anónimas de `solver.cpp` (ambigüedad ADL) |
| T12 | `736bbcd` | **V18**: el aula de referencia debe ser del tipo general (`Aula de estudio`), al final de la cadena V16 → V17 → V18 |
| T13 | `d3e8de0` | Causa (2) **solo** por `demanda(t) > capacidad(t)`, sin bloqueos ni conteo por referencia; fallback «no atribuible»; guard de la excepción |
| T14 | `b4491ad` | Tests de ventanas de turno (Hall), reproducibilidad y aula especializada no reservada |
| T15 | `da71419` | Contrato vivo, constitución y 005 alineados con V18, la capacidad del tipo y «no atribuible» |
| T16 | `a233afc` | Regresión global (**50/50**), benchmark factible y seed intacto |
| Notas | `072b6ee` | Notas del reviewer **M2 e I3** en los tests: comentarios al día y sub-motivo del contraejemplo |

**Notas del reviewer aplicadas.**

- **M1** — fila de «errores de lectura frecuentes» del contrato (`docs/contrato-solver-2.0.md:290`): la
  falta de laboratorio **no** es «siempre conflicto de aula»; la causa es (2) **solo si**
  `demanda(t) > capacidad(t)` y **no atribuible** si el conteo no falla (§5.1). **La aplica el
  coordinator** (el planner no escribe en `docs/`); el texto literal se entregó en este cierre.
- **M2 e I3** — comentarios de los tests y sub-motivo del contraejemplo: hechos en `072b6ee`.
- **I2 e I5** — cita del criterio 2 de la 005 y aclaración del `HEAD` de T16: hechas en este cierre.

**Lo que no se tocó:** el **esquema** (no hay columna ni versión nuevas), la **009**, el **seed**, el
**frontend** y el **contrato** (sigue en **2.0**, sin sección ni campo nuevos).

## Cambio de requisitos (2026-10-26): referencia de tipo general y causas por capacidad

**Decisión del usuario (2026-10-26):** el defecto de RF-6 **se arregla dentro de la 010**, no en una spec
nueva, pero **evaluando y planificando antes de implementar**. Esta fase solo toca `spec.md` y este
`estado.md`; `plan.md` y `tasks.md` se rehacen después, y solo en la parte afectada.

### Causa raíz, en tres capas

1. **Requisito frente a mecanismo.** La definición «Causa de un resultado no factible» (005:314-320,
   heredada por la 010) exige la categoría que **explica** por qué no hay solución, la primera de un
   orden fijo. La infactibilidad es global y conjunta. Las categorías (1), (3) y (4) tienen un test
   local exacto; la (2) **no**, porque con el aula de referencia bloqueada la capacidad de un tipo
   depende de *dónde caen* las clases desplazadas y el test exacto es un problema de empaquetamiento.
   El plan eligió cotas (palomar) y **una cota no es una causa**: puede no dispararse (entrada sin
   categoría) o dispararse de más (tapa la causa real).
2. **Modelo con el día implícito.** En el modelo el día es una dimensión (`x[p][m][c][d][s]`,
   `y[c][d][s][a]`), pero el diagnóstico colapsa las franjas a un `QSet<int>` de un día y reintroduce el
   día multiplicando a mano (`solver.cpp:114, 263, 288, 330`). De ahí el error de unidades: se resta un
   `QSet` de franjas de un día a un palomar medido en pares día×franja.
3. **El orden de precedencia convierte cualquier cota en un tapón.** Como (2) se evalúa antes que (3) y
   (4), una **sobreestimación** de (2) oculta la causa verdadera (el contraejemplo del reviewer: modelo
   infactible por docente informado como conflicto de aula) y una **subestimación** deja la entrada sin
   categoría.

### Decisiones del usuario (2026-10-26)

1. **El aula de referencia es siempre de un tipo general/ordinario.** Palabras del usuario: «cuando
   hablo de general es un tipo específico de aula: en el liceo solo hay aulas regulares (donde se ve
   clase la mayoría del tiempo) y la cancha; las aulas generales son las que se reservan en los
   desplazamientos». Laboratorios, talleres y canchas **nunca** son referencia; solo se desplazan hacia
   ellas las materias que las requieren.
2. **La reserva se mantiene.** Mientras el grupo está desplazado, su aula de referencia queda vacía y
   bloqueada; es la decisión del 2026-10-23 y no cambia.
3. **Se arregla dentro de la 010**, con evaluación y plan previos.
4. **Tipar día×franja** en las cuentas de capacidad (un tipo que represente `(día, franja)` en vez de
   multiplicar por `num_dias` a mano) es **requisito del plan**, no de la spec: la spec solo fija el QUÉ.
5. **El tipo general no lleva columna ni campo: es una convención fija por nombre.** Palabras del
   usuario: «no vamos a añadir una columna para eso, vamos a dejar esa entrada hardcodeada; no importa
   si se borra la BD, siempre que se genere estará ese tipo, y al final solo sería una coincidencia por
   índice o por nombre». Se concreta en: el tipo general/ordinario es el de **clave normalizada
   `aula de estudio`** (el nombre con el que se siembra el catálogo), se reconoce con la **misma**
   igualdad de clave que el contrato ya usa, **sin** columna nueva, **sin** subida de versión de esquema,
   **sin** reabrir la 009 y **sin** sección nueva en el contrato (la versión sigue siendo **2.0**).
   Limitación declarada: un centro cuyo tipo ordinario tenga otro nombre no está soportado hoy.
6. **La causa (2) se queda solo con el conteo de capacidad del tipo.** Palabras del usuario: «vamos a
   eliminar el conteo de las aulas generales, no lo necesitamos». **Se descarta el conteo por aula de
   referencia compartida**: la contención en los turnos solo permite afirmar que no falla cuando la
   referencia la usa un solo curso, y con varios cursos su cuenta exacta exige resolver el empaquetamiento
   de las clases desplazadas. Lo que solo ese conteo habría atribuido se informa como **causa no
   atribuible** (documentado como caso límite).
7. **Excepción de precedencia de (3) sobre (2) (decisión del usuario, 2026-10-26).** Palabras del
   usuario: «**Mantener el guard y enmendar RF-6**»: cuando un curso no cabe en su turno, la causa es
   **(3) conflicto de persona** aunque el conteo de capacidad del tipo también falle, porque es el motivo
   concreto y comprobable. La spec enmienda RF-6, la definición de «Causa», RF-7 y el criterio 6 con esa
   excepción, acotada a ese único caso: el orden (1)(2)(3)(4) no cambia fuera de él. El `guard` de
   `causaSaturacionPorTipo` deja de ser una desviación y pasa a ser la implementación de la excepción.

### Consecuencia que recoge la spec (verificada al redactar)

Con la referencia limitada al tipo general fijo, la categoría (2) deja de necesitar estimación:

- Un tipo **especializado** nunca es referencia ⇒ ninguna de sus aulas queda bloqueada ⇒ su capacidad se
  puede contar sin descontar reservas, con la definición de «capacidad de un tipo» de la spec: si la
  demanda del tipo supera esa capacidad, la infactibilidad es cierta.
- Un aula **general** no se sobrevende **por el bloqueo de un solo curso**: si el curso está en su
  referencia la ocupa y si está desplazado la bloquea; en ambos casos el aula es inutilizable en las
  franjas en que el curso tiene clase, y V14 impide que ese curso tenga más bloques que franjas de su
  turno. Con varios cursos compartiendo la referencia la suma sí puede agotarla; por eso ese conteo no se
  usa. El bloqueo, en todo caso, **no se resta** de la capacidad: es una regla del modelo, no una cifra
  de demanda.
- El único test de (2) es el **conteo de capacidad del tipo**, que es **necesario** (si falla, la
  infactibilidad es cierta), así que no puede producir falsos positivos como la cota anterior. Puede
  **no** probar la infactibilidad; para eso está el «no atribuible».
- **Se descarta el conteo por aula de referencia compartida** (antes se había conservado): con varios
  cursos su cuenta exacta exige decidir en qué franjas cae cada clase desplazada, que es justo lo que
  aproximaba la heurística retirada. Consecuencia aceptada: dos cursos que comparten referencia y no
  caben en ella se informan como **no atribuible**, no como conflicto de aula.

### Qué parte de la 010 sigue validada y qué queda pendiente

- **Sigue validada e implementada** (no se toca): contrato 2.0 y rechazo de versiones anteriores (RF-1,
  RF-2), V2/V16/V17 y eventos (RF-5), regla de tipo, desplazamiento con vuelta, reserva y salida con
  aula real (RF-3, RF-4, P4), supersesión de la 005 y enmienda constitucional (RF-7, RF-8), MER (RF-9),
  documentación del contrato (RF-10), benchmark bajo 60 s (RNF-2) y suite 49/49 en `de61050`.
- **Pendiente de rehacer**: RF-3 (referencia del tipo general, tipos especializados nunca reservados),
  RF-5/V18 y su rango V1–V18, RF-6 completo (causas), los textos de RF-7 (supersesión de RF-6 de la 005)
  y RF-8 (constitución: §7.1.5, §7.3, §7.4 y comportamiento), el MER (RF-9) y la documentación del
  contrato (RF-10), más la anotación de la 005 y de la constitución, que ejecuta el planner. **RF-1 no
  cambia de forma**: no hay catálogo ni marca en el contrato.
- **Lo que el plan deberá eliminar o reescribir** (ya no se sostiene): se **eliminan** `palomarPorTipo` y
  `franjasBloqueadasEnFranjasContadas`, y el **conteo por aula de referencia compartida** completo (no
  solo el descuento de bloqueos); **`causaSaturacionPorTipo` no se elimina**: implementa el conteo de
  capacidad del tipo que RF-6 conserva, y se **reescribe** para quitarle el descuento de bloqueos. El
  bloqueo deja de ser una cifra en cualquier cuenta de capacidad. La reserva **no** se elimina del
  modelo: solo deja de entrar en las cuentas.
- La spec **no** se marca `implementada`: el veredicto anterior queda superado y hace falta uno nuevo
  después de reimplementar y revalidar la parte afectada.

### Excepción de precedencia (3) sobre (2) — aprobada el 2026-10-26

- **Decisión del usuario:** «Mantener el guard y enmendar RF-6». Cuando un curso necesita más bloques que
  franjas tiene su turno, la causa es **(3) conflicto de persona** aunque el conteo de capacidad del tipo
  también falle, porque es el motivo concreto y comprobable.
- **La spec se enmienda** (mismo día, sigue `Estado: aprobada (2026-10-26)`): RF-6 gana la **excepción de
  precedencia** en EARS, la definición de «Causa» la recoge, RF-7 la añade como punto (c) de la
  supersesión de la 005 (con el aviso de que el orden no cambia fuera de ese caso) y el criterio 6 la
  incluye; se añade el caso límite «curso que no cabe en su turno y además un tipo sin capacidad → (3)».
- **Consecuencia en el código:** el `guard` de `causaSaturacionPorTipo` (`solver.cpp:191-194`) deja de ser
  una desviación y pasa a ser la implementación de la excepción; `solver_falloCausaConflictoDePersona`
  sigue siendo la prueba (su fixture ya tiene un tipo sin capacidad: 4 bloques frente a 2 franjas).
- **Lo que no cambia:** el orden (1)(2)(3)(4) **fuera de la excepción**, el conteo de capacidad del tipo
  como único test de (2) y el desenlace «no atribuible» cuando ninguna categoría aplica.

### Reescritura del plan y las tareas (2026-10-26)

- **Plan y tareas reescritos solo en la parte afectada** (`Base: 49d39dc`): `plan.md` §0 (evaluación de
  viabilidad, ejemplo de no-suficiencia verificado e inventario), §3.2 (V18), §4.4 (causas), §7
  (enmienda reducida a lo que este cambio añade) y T11–T16; `tasks.md` añade T11–T16 y marca T6 como
  superada. Lo ya ejecutado (T1–T10) se conserva y no se reabre.
- **Lo que el plan retira:** `palomarPorTipo`, `franjasBloqueadasEnFranjasContadas`,
  `DesplazamientosDeGrupo`, `causaConflictoDeAula` (sus dos bloques), `nombreAula` y `claveAula` de
  `solver.cpp` (el `claveAula` estático de `restricciones_aulas.cpp:26` es otro helper y se conserva).
  **`causaSaturacionPorTipo` se reescribe**, no se elimina: es el conteo que se conserva.
- **Inventario de tests de causas: 13 en total** (**12** `solver_causa*` + `solver_falloDocenteNoElegibleNoSeInformaComoConflictoDeAula`).
  Reparto: **3 se reescriben** (los que exigían (2) por el bloqueo pasan a **no atribuible**), **2
  reescriben solo la aserción** (siguen dando (2), ahora con el nombre del tipo), **3 se renombran** (la
  unión/palomar ya no existen), **2 solo actualizan el comentario** y el resto queda igual; **0 se
  eliminan**; **3 se añaden** (regresión del reviewer, no atribuible por ventanas de turno y
  reproducibilidad). Fuera de los 13, cambian **dos** aserciones: `solver_falloCausaConflictoDeAula`
  (005, asevera el tipo) y `solver_falloCausaConflictoDeAulaPorReferenciaSaturada` (T6, renombrado a
  `...PorCapacidadDeTipo`, asevera el tipo en vez del aula).
- **Corrección de alcance (2026-10-27).** El borrado de las dos copias anónimas de
  `aulasDelTipo`/`slotsTurno` de `solver.cpp` pasa a **T11** por **ambigüedad ADL** (las firmas
  globales nuevas y las copias del namespace anónimo son candidatas a la vez: no hay ocultamiento
  posible); el re-QA del plan afirmó lo contrario y era **incorrecto**. El resto del inventario de
  causas sigue en T13.
- **Plan y tareas aprobados por el usuario el 2026-10-26.** La spec está aprobada (2026-10-26, con la
  enmienda) y `plan.md` y `tasks.md` de esta segunda entrega también; la implementación arranca en T11.

## Validación 6 · defecto abierto (2026-10-25) — histórico, superado por el cambio de requisitos

**La 010 se para aquí a petición del usuario, no se cierra.** Decisión textual (2026-10-25): «vamos
a comitear, y dejar aquí esta spec, y vamos a ver bien cómo podemos hacerlo para no poner parches
sobre parches».

> **Superado por el cambio de requisitos del 2026-10-26.** La duda de esta sección ya no está abierta:
> se cierra con la decisión de que el aula de referencia sea siempre de tipo general/ordinario y con el
> diagnóstico por capacidad del tipo (sección «Cambio de requisitos (2026-10-26)»). El 2026-10-25 se dijo
> «rediseñar en una spec nueva»; el usuario decidió después **hacerlo dentro de la 010**. El
> contraejemplo se conserva como evidencia del defecto y como caso de prueba de la implementación
> nueva: con la regla nueva, ese fallo es de docente (categoría 4), no de aula.

### Dudas abiertas

1. **La estimación de franjas bloqueadas del aula de referencia mezcla unidades (RF-6, causas de un
   resultado no factible).** Para decidir la causa «conflicto de aula» (categoría 2) se estima cuántas
   franjas de la referencia dejan bloqueadas las clases desplazadas. Esa estimación usa el **palomar**
   —⌈bloques desplazados / aulas del tipo⌉—, que cuenta **pares día×franja** distintos que las clases
   desplazadas necesitan; pero el descuento resta esa cifra de un conjunto medido en **franjas del
   día** (valores de *slot*): `fuera` y `dentro` se cuentan sobre los slots alcanzables y el resultado
   se resta del palomar. Con una semana de **1 día** ambas unidades coinciden y el cálculo acierta;
   con **2 o más días** el descuento puede ser mayor que el bloqueo real, de modo que se declara
   **causa (2) conflicto de aula** cuando la causa real es **(4) conflicto de asignación de
   docente** —y, como la categoría 2 se evalúa antes que la 3 y la 4, tapa la causa verdadera.

   **Contraejemplo reproducible del reviewer** (2 días, turnos `TB={5,6}` y `TX={4,5}`; `R1` del tipo
   «Aula base» es la referencia de `2|Y`, que la llena con 2 bloques/día de ciencias, y de `1|X`, que
   solo tiene 2 bloques de laboratorio con **1** laboratorio en el centro; el docente de laboratorio
   dispone de **una** franja): el modelo es INFEASIBLE por docente, pero el diagnóstico dice «el tipo
   «Aula base» necesita 4 bloques y sus 1 aulas ofrecen 4 franjas (1 las bloquean clases
   desplazadas)». Con ese docente disponible en (día 0, franja 4) y (día 1, franja 4) —ambas **fuera**
   de lo contado— el mismo dato es **factible**: el bloqueo real dentro de lo contado es 0.

   No se corrige en la 010. Requiere rediseñar el diagnóstico de causas (ver «Siguiente paso»).
   > **Corregido por el cambio de requisitos del 2026-10-26:** el bloqueo de las aulas de referencia ya
   > no se resta de ninguna capacidad; la causa (2) es un conteo de capacidad del tipo y el
   > contraejemplo pasa a ser causa (4). Ver «Cambio de requisitos (2026-10-26)».

## Correcciones 1-5 del cálculo de causas: parches sucesivos (2026-10-25)

El mismo cálculo acumuló **cinco correcciones** el mismo día, cada una arreglando lo que la validación
anterior había destapado en la fórmula anterior:

| # | Commit | Qué cambió |
|---|---|---|
| 1 | `aa7ee74` | El aula de referencia descuenta un bloqueo por bloque desplazado (antes no descontaba nada): una entrada válida se quedaba sin categoría de causa. |
| 2 | `232bd19` | El bloqueo pasa de **suma** de bloques desplazados a **unión voraz** de franjas, para no descontar dos veces la misma franja. |
| 3 | `45ac292` | Se añade la **cota inferior por tipo (palomar)** y se toma el máximo con la unión: con pocas aulas del tipo desplazado la unión se quedaba corta. |
| 4 | `5fc40e0` | Se **elimina la unión** y queda solo el palomar: la unión no era cota inferior y sobreestimaba con turnos distintos. |
| 5 | `de61050` | El descuento se **limita a las franjas contadas** en la capacidad: antes restaba franjas de otro turno que nunca se habían sumado. |

**Son parches sucesivos sobre una heurística aritmética**, no un diseño: cada vuelta encontró que la
cota anterior no era lo que su comentario afirmaba (una suma que no descontaba, una resta de
conjuntos, una unión que no era cota, un `max` de dos términos, un descuento de franjas nunca
sumadas) y la siguiente ajustó la fórmula. Eso es exactamente lo que el usuario quiere evitar; de ahí
la decisión de **rediseñar el diagnóstico de causas** en una spec nueva en lugar de una sexta
corrección. La 6.ª validación encontró el defecto de unidades descrito arriba.
> **Rectificación del 2026-10-26:** el rediseño se hace **dentro de la 010** (decisión del usuario),
> no en una spec nueva. Estas cinco correcciones quedan como historia y la heurística se elimina.

### Lo que sí pasó la validación

El resto de la 010 se validó y no está en discusión: **contrato 2.0** y rechazo de versiones
anteriores (RF-1, RF-2), **validaciones de eventos y tipos** V2/V16/V17 (RF-5), **regla de tipo**,
**aula de referencia con bloqueo** y desplazamiento con vuelta (RF-3), **salida con aula real** y
arreglo de **P4** (RF-4), **supersesión de la 005** y **enmienda constitucional** (RF-7, RF-8),
**MER completo** (RF-9), **contrato 2.0 documentado** (RF-10) y **benchmark con tipos por debajo de
60 s** (RNF-2); suite **49/49** en `de61050`.

## Validación (2026-10-25)

> **Histórico.** Este fue el primer ciclo de validación; lo superan «Validación 6 · defecto abierto»
> y «Correcciones 1-5». Se conserva como registro del hallazgo original.

- **Veredicto del reviewer: CAMBIOS NECESARIOS.** Hallazgo principal (RF-6): `causaConflictoDeAula`
  contaba solo los bloques que se quedan en el aula de referencia y **no descontaba** las franjas que
  dejan bloqueadas las clases desplazadas del grupo; la saturación por tipo tampoco. Una entrada
  válida podía ser INFEASIBLE y quedarse sin categoría de causa («no se pudo identificar una
  categoría concreta»).
- **Corregido en `aa7ee74`** (solo `src/backend/src/solver/solver.cpp` y
  `test/backend/test_solver_horarios.cpp`): capacidad disponible = franjas del grupo menos un bloque
  por bloque desplazado (limitado al turno del curso) y, por tipo, se descuentan las franjas
  bloqueadas en las referencias de ese tipo. Tres casos nuevos y uno ajustado; `ctest --preset
  backend` **49/49** en `aa7ee74`.
- **Aplicado antes del veredicto**: la desviación **autorizada por el usuario fuera del plan** sobre
  P4 (`06fc70e`): `ValidadorSalidaSolver` indexa la ocupación por `(profesor, día, slot)` y
  `(aula, día, slot)` en lugar de solo por franja.
- **Falta la re-validación** del reviewer contra `aa7ee74`; todavía no hay `VEREDICTO: APROBADO`.

## Decisiones cerradas (2026-10-23 · ampliadas el 2026-10-26)

- **Reparto en dos specs.** La 010 recibe todo lo del **solver**: contrato 2.0, regla de tipo,
  validaciones, supersesión de la 005, enmienda constitucional, documentación del contrato y diagrama
  entidad-relación. La 009 conserva persistencia, servicios, esquema, seed y documentación de la API.
- **Regla de tipo dura siempre.** Toda clase se imparte en un aula del tipo exacto requerido por su
  materia, en los dos modos. El **aula fija es el aula de referencia**: se usa cuando su tipo coincide;
  si no, la clase se desplaza a un aula del tipo requerido y el curso **vuelve**.
- **Aula de referencia del tipo general/ordinario (decisión del usuario, 2026-10-26).** La referencia es
  siempre del tipo general, **identificado por convención fija: clave normalizada `aula de estudio`**
  (sin columna, sin sección de contrato y sin subida de esquema); laboratorio, taller y cancha nunca son
  referencia ni quedan reservados. La validación correspondiente es **V18** y el diagnóstico de causas
  pasa a **conteo de capacidad del tipo**.
- **Aula de referencia vacía y bloqueada (decisión del usuario).** Mientras el curso tiene una clase
  desplazada en una franja, su aula de referencia **no aloja ninguna clase y ningún otro curso puede
  usarla** en esa franja. La reserva no se extiende a franjas en las que el curso no tiene clase.
- **Exclusividad.** Dos cursos no comparten aula en la misma franja, incluidas las clases desplazadas.
  La competición por el mismo tipo no se rechaza antes de resolver: si no hay solución, el resultado es
  **no factible** y la causa se decide según RF-6 (conflicto de aula si falla el conteo de capacidad del
  tipo y no aplica la excepción de precedencia; (3) si aplica; **no atribuible** si no falla).
- **Contrato.** Solo se declara y acepta la versión `2.0`; 1.0, ausente o desconocida se rechaza como
  **entrada inválida**, sin conversión ni tipos por defecto. Los presets 1.0 en disco quedan intactos y
  se rechazan al usarlos.
- **Salida.** Cada clase informa el **aula real** donde se impartió, también en modo de aula fija.
- **Duda diferida (fuera de alcance): la cancha compartida.** Se deja con exclusividad; si el usuario
  decide compartirla, se abre un cambio de requisitos.

## Correcciones del QA del plan (2026-10-25)

Once hallazgos aplicados; el plan sigue con **10 tareas** (T1–T10, 30/30/30/30/20/20/20/20/20/20 min).

1. **Fixture de T2 versionado.** `datos-prueba/` está en `.gitignore:35`; se sustituye por
   `test/data/preset-1.0.json` (nuevo, versionado), cargado con `QFINDTESTDATA`, con hash de bytes
   antes/después. El preset real solo se menciona en la documentación.
2. **Bloqueo de la referencia.** El bucle recorre solo las claves de `vars.aulaAssignment` con
   `(d, s, r)` existentes (los turnos mixtos no crean `y` en todas las franjas).
3. **§7.4 sin detalle.** Solo rango `V1–V15` → `V1–V17`, constancia de que V1 habla de eventos y
   remisión a la 005/010; no se duplican V2/V16/V17.
4. **Documentación viva fuera de `docs/solver/`.** `docs/contrato-solver-2.0.md` (la constitución
   declara `docs/solver/` histórico y no obligatorio).
5. **T1/T2/T3 reequilibrados.** T1 = contrato 2.0 (incluida la sección `eventos`) + constructor +
   consumidores que compilan; T2 = versión y cargador; T3 = casos V1/V2/V16/V17 (V1 ya cambiado en
   T1, aquí solo sus pruebas); los `-R` ya no arrastran tests de otras tareas.
6. **Benchmark con estrategia fija.** 6 aulas (5 comunes + 1 laboratorio), solo `Tecnología` requiere
   laboratorio, referencias repartidas por turno; T10 exige factibilidad y < 60 s.
7. **Seed no generable.** ≈104 bloques de ciencias contra 1 laboratorio × 40 franjas; anotado en el
   plan y en T10, sin intentar generarlo.
8. **T5 es solo prueba.** `ValidadorSalidaSolver` ya usa `asignacion.aula`; no cambia producción.
9. **Un solo nombre de campo.** `SolverConfig` usa `tipo`; la guarda de RF-1 mira `tipoAula` del DTO y
   los fixtures deben fijar el nombre, no solo `idTipoAula`.
10. **`TurnoConfig.inicio/fin`** se rellenan en el constructor desde el `TurnoDTO`; sin eso V2 falla.
11. **Cita corregida:** `test/CMakeLists.txt:120-133`.

## Decisión de plan (2026-10-25)

- **RF-1: la construcción falla si falta un tipo.** `construirJsonEntrada` y la fachada
  `ConstructorEntradaSolver::construir` devuelven **`Resultado<QJsonObject>`**. Si un aula o una
  materia de la instantánea no declara tipo, la construcción devuelve `error` con un mensaje en
  español que nombra la entidad y **no emite JSON**. Se descartan «copiar el tipo vacío y dejar que
  V16 lo rechace» y «comprobación previa separada». El builder del frontend queda como **rotura
  conocida** (fuera de alcance; el preset `backend` no lo compila). Detalle en `plan.md` §1.1, §3.1 y
  decisión 12.

## Decisiones y correcciones del re-QA (2026-10-23)

- **N3 — Entrega y verificación.** Dentro: backend, datos, seed, consola y sus pruebas, que compilan y
  pasan. Fuera: la interfaz y los tests ligados a ella. La construcción debe poder excluir la interfaz
  (requisito de la 009, RNF-6); el detalle técnico es del plan. RNF-5 lo exige en esta spec.
- **N7 — V2 alineada con la 009.** Valida turno existente, nombre no vacío, posición 1..franjas,
  orden ≥ 1, duración > 0, par inicio/fin, `fin − inicio = duración`, ventana del turno y de la
  posición, orden cronológico y ausencia de solapes dentro y entre posiciones.
- **N8 — Documentación del contrato.** Se asigna a un **RF propio de la 010 (RF-10)**, no a la guía de
  servicios de la 009: forma 2.0, tipos por nombre y clave, sección de eventos, salida con aula real,
  rechazo de 1.0 y semántica del aula de referencia. La guía de la 009 sigue siendo la de servicios.
- **N9 — Dependencia.** La 009 precede a la 010; la 010 consume los tipos persistidos, los eventos con
  identidad y orden, los servicios, la huella y el esquema que la 009 deja listos.
- **N10 — Enmienda constitucional exacta.** §7.1.5 (línea 64, invariante del aula), §7.3 (línea 78,
  contrato de configuración), §7.4 (línea 87, rango V1–V15 → **V1–V17**; la constitución no define el
  contenido de V1, solo lo menciona: se actualiza el rango, se anota que V1 habla de eventos y el
  detalle de V1 se cambia en la 005), §7.5 (línea 91, «el receso es metadata» → eventos) y §5
  (línea 34, dominios protegidos: recesos → eventos y añadir tipos de aula).
- **N11 — Normalización.** La normalización es **solo para comparar**; el nombre visible se conserva
  tal cual. La equivalencia de ida y vuelta se evalúa sobre la clave de tipo.
- **N12 — RF-6 en EARS.** «Si no existe solución factible y el bloqueo es de aulas, la causa es
  conflicto de aula».
  > **Superada por el cambio de requisitos del 2026-10-26.** La categoría (2) ya no se decide por «el
  > bloqueo de aulas» sino por el **conteo de capacidad del tipo**, y si ese conteo no prueba la
  > infactibilidad la causa se informa como **no atribuible** (RF-6 de esta spec).
- **N13 — RF-2.** El aviso de archivo se limita a presets en disco; se elimina «abrir con una versión
  compatible» y la acción es generar una 2.0 desde los datos.
- **N14 — Duplicado.** La regla de competición por el mismo tipo queda solo en RF-3; se quitó de RF-5
  (no es validación previa).
- **N15 — Criterio 2 de la 005.** Se añade a la lista de supersesión: se mantiene salvo D10.
- **N16 — Numeración.** Se unifica «V16 y V17» en toda la spec; no queda «V16+».
- **N17 — Identidad del evento.** No viaja en el contrato ni forma parte de la equivalencia; los
  eventos se distinguen por nombre, posición y orden.
- **N19 — Base.** Fijada en **`e72c930`**, cierre de la 009; HEAD = `e72c930`. La 009 está
  implementada y el árbol limpio; no hay diff de esquema sin commitear que imputar a la
  implementación.

## Supersesión de la 005 (declarada en RF-7)

- Definición «Curso con aula fija: usa siempre su aula».
- Invariante **D10**.
- **Criterio de finalización 2** (005:745-748, «D1–D11 y D13»), salvo D10.
- Salida del horario «con aula fija, su aula fija».
- **RF-7** de la 005.
- Versión **1.0** (definición, RF-10 y criterio de finalización).
- **V1** que exige la sección de recesos.
- **Definición «Causa de un resultado no factible»** (2026-10-26): la categoría (2) pasa a **conteo de
  capacidad del tipo** y la promesa de que las categorías cubren todas las invariantes duras se sustituye
  por la **causa no atribuible** (RF-6).
- **RF-6 «Comportamiento y contrato del solver»** de la 005 (2026-10-26): el párrafo que informa la causa
  «con las categorías … en su orden» se sustituye por el orden y las condiciones de RF-6 y por la causa
  no atribuible cuando ninguna categoría aplica.
- **D9 se generaliza** y **D8 se extiende**; **V8 se mantiene**.
- El resto de la 005 sigue vigente. La anotación en el texto de la 005 la hizo el planner en T7
  (commiteada en `f054b2e`); el implementer no edita specs. La anotación del cambio del 2026-10-26
  (**V1–V18** y la definición de causa) está **pendiente** y la hará el planner al rehacer el plan.

## Notas de proceso

- **La enmienda constitucional la ejecuta el planner.** Hecha en `f054b2e` (T7) junto con la
  anotación de supersesión de la 005; el implementer solo verifica.
- **Plan y tareas aprobados (2026-10-25).** `plan.md` y `tasks.md` se escribieron el 2026-10-24 y el
  usuario los aprobó el 2026-10-25; la implementación se hizo en orden T1–T10 y está commiteada.
- **RF-1 cerrado (2026-10-25).** La construcción devuelve `Resultado<QJsonObject>` y falla si un aula
  o una materia no declara tipo; la confirmación que estaba pendiente queda resuelta. (La duda que se
  abrió el 2026-10-26 sobre cómo distinguir el tipo general se cerró ese mismo día: **convención fija por
  nombre normalizado**, sin cambios en el esquema ni en el contrato.)
- **Preset 1.0 de prueba.** `test/data/preset-1.0.json` (versionado) es el fixture de rechazo de T2,
  leído con `QFINDTESTDATA` y verificado por hash de bytes. `datos-prueba/preset-generacion-31059.json`
  no está versionado (`.gitignore:35`) y solo se cita en la documentación como ejemplo a regenerar.
- **T7 (2026-10-25), hecha y commiteada (`f054b2e`).** La enmienda de `docs/constitution.md` (§5,
  §7.1.5, §7.3, §7.4 y §7.5) **y** la anotación de supersesión de
  `specs/005-nucleo-solver/spec.md` están aplicadas: aula fija, **D8/D9 ampliadas**, **D10**, salida
  con **aula real**, **RF-7**, versión **1.0**, **V1** (eventos), **V2** (+ **011** futura en
  `duracion_minutos`), **V8 mantenida** y el catálogo a **V1–V17**. El parche del planner
  (`/tmp/opencode/constitution-t7.patch`, §5/§7.1.5/§7.3/§7.4/§7.5) lo aplicó el coordinator, porque
  el subagente planner no escribe fuera de `specs/`; el implementer lo verificó con los dos `grep` de
  la tarea. ~~No queda nada pendiente de la constitución ni de la 005.~~ **Rectificado el 2026-10-26:**
  el cambio de requisitos añade §7.1.5 y §7.3 (referencia del tipo general), §7.4 (**V1–V17 → V1–V18**)
  y §7 «Comportamiento» (causa no atribuible), y en la 005 el catálogo a **V1–V18** y la supersesión de su
  RF-6; esa anotación está pendiente y la hará el planner al rehacer la parte afectada del plan. §5 y
  §7.5 **no** se vuelven a tocar.
- **Desviación autorizada fuera del plan: P4 (2026-10-25, `06fc70e`).** El usuario autorizó arreglar
  el defecto preexistente de `ValidadorSalidaSolver` (la ocupación se indexaba solo por franja):
  ahora las claves son `(profesor, día, slot)` y `(aula, día, slot)`. Sin este arreglo, cualquier
  horario con dos cursos en paralelo se marcaba `CalculoFallido` en `ServicioGeneracion`. Detalle en
  `plan.md` §1.2 y en `journal.md` («P4 · claves por aula y docente»).

## Tamaño previsto

Diez RF en **diez tareas** (20-30 min), con build sin frontend y el benchmark de 60 s en T10.

> **Tras el cambio de requisitos del 2026-10-26** el plan y las tareas se rehacen **solo** en la parte
> afectada (RF-3, RF-5/V18, RF-6, textos de RF-7/RF-8/RF-9/RF-10 y eliminación de la heurística y del
> conteo por aula de referencia); RF-1 no cambia de forma. El límite de 10 tareas se mantiene y las
> tareas ya cumplidas no se reabren.

