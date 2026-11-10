# Spec 018 - Aforo simultáneo de la cancha

Estado: implementada (2026-11-10)

> **Alcance.** Esta spec define **cuántos cursos pueden estar a la vez en una misma aula**, con la
> cancha del centro como único caso: **dos cursos distintos, con docentes distintos, y solo cuando su
> bloque de dos horas coincide entero**. El aforo de la cancha queda **fijado en 2** como **regla del
> sistema por tipo de aula** (hardcodeado: **no** viaja en el contrato, **no** toca el esquema, **no**
> entra en los presets y **no** es configurable); el resto de tipos tiene aforo **1**. El aforo es de
> **cursos**, no la capacidad física de alumnos (88). Todo lo demás del modelo vigente se mantiene: un
> curso y un docente siguen sin poder estar en dos sitios a la vez, las 12 aulas generales siguen
> alojando **una sola clase por franja**, el tipo exacto sigue siendo exacto, la cancha sigue sin ser
> aula de referencia, y las reglas de bloques, huecos, separación y horas del docente no cambian.
>
> **Por qué ahora.** La **017 (cerrada el 2026-11-09)** sustituyó el conjunto sintético por el **centro
> real** y lo dejó **sin declarar generable** precisamente por esto: **96 clases de Educación Física
> (48 por turno)** frente a **40 franjas de cancha por turno** con la exclusividad vigente. Con dos
> cursos por franja la oferta es de **80 plazas por turno** y **160 en la semana**: la cuenta cuadra
> (**48 ≤ 80**; **96 ≤ 160**) y, si se cuentan **pares completos**, sobran plazas de par (**24 pares
> por turno frente a 40 plazas de par**). Esta spec cierra la **aceptación de generación diferida** por
> la 017 y la deuda documental del «no generable».
>
> **Petición del usuario (2026-11-07, literal).** «EF en cancha, despues de crear el seed, vamos a hacer
> el aforo simultaneo de la cancha, este aforo sera hardcodeado por nosotros, definiendo cuantos cursos
> pueden ver en la cancha en simultaneo (con distinto docente), usaremos un aforo de 2».
>
> **Relación con las specs anteriores.**
> - **005/010 (exclusividad de aula).** Se **matiza**: un aula aloja una clase por franja **salvo que su
>   aforo sea mayor**; las aulas generales conservan el aforo 1. La regla de tipo exacto, el aula de
>   referencia, el desplazamiento, la vuelta y el bloqueo de la referencia **no cambian**.
> - **010 (capacidad del tipo y causa (2)).** Su aritmética se **amplía**: capacidad = **aulas × franjas
>   × aforo** por tipo; el mensaje de la causa (2) cita las cuatro cifras. Sigue siendo **necesaria y no
>   suficiente**, no descuenta bloqueos y no altera el orden ni la precedencia de causas.
> - **010 (comprobación de la salida, P4).** Deja de marcar conflicto cuando dos clases comparten un
>   aula **hasta su aforo**; por encima del aforo sigue siendo conflicto. Sin este ajuste, un horario
>   **válido** se marcaría como fallo.
> - **014 (bloques de dos horas).** La regla del **par** y la de **misma aula en las dos franjas** son
>   las que hacen que la simultaneidad sea **siempre por bloques completos**; se exige y se verifica
>   explícitamente. Su nota «la cancha admite dos cursos; el aforo es spec futura» pasa a ser
>   **exigible**. Su fixture de aceptación **sin EF** se **complementa**, no se retira.
> - **017 (datos reales del centro).** Se **supersede** su declaración de «no generable» (RF-9 y RF-10)
>   y su aceptación diferida (D-5 y D-9): el conjunto pasa a declararse **generable**. Sus datos **no
>   cambian de modelo ni de forma**, salvo la **corrección del 2026-11-10** (docente de apoyo **P033**):
>   **33 docentes** (30 + 2 añadidos + 1 de apoyo), **165 disponibilidades**, **272 asignaciones** y
>   capacidad **1188** (**576** mañana + **612** tarde); 2 planes, 85 materias, 24 cursos, 12 aulas
>   generales + 1 cancha y **876 clases** se mantienen, y **no** hay cambios de modelo, contrato ni
>   esquema.
> - **013, 011, 015 y 016.** Sin cambios.
>
> **Qué no es.** No es una pantalla ni un parámetro de configuración de aforos; no es una dotación nueva
> de canchas, laboratorios o talleres; no es la identidad del curso con el plan; no es la spec de
> rendimiento/descomposición; no es un importador de datos. Tampoco cambia la capacidad física de las
> aulas ni el contrato de configuración.

## Decisiones cerradas (2026-11-09)

| # | Decisión | Resolución |
|---|---|---|
| D-1 | Dónde vive el aforo | **Regla del sistema por tipo de aula**: `Cancha` → **2**, el resto → **1**. **Hardcodeado**: no viaja en el contrato, no toca el esquema, no entra en los presets y no es configurable. |
| D-2 | A qué aulas se aplica | **Solo a la cancha.** Las **12 aulas generales** conservan **una clase por franja**. |
| D-3 | Cómo se reconoce | Por la **clave normalizada `cancha`** del tipo. Si hay materias que exigen cancha y el tipo no se reconoce, **el sistema falla con un mensaje claro**; **nunca** aplica aforo 1 en silencio. |
| D-4 | Condiciones de simultaneidad | **Cursos distintos con docentes distintos**, sin condiciones añadidas: un curso y un docente ya no pueden estar en dos sitios a la vez. |
| D-5 | Interacción con el bloque de 2 h | **Solo pares completos**: dos cursos comparten la cancha **solo cuando su bloque de dos horas ocupa exactamente las mismas dos franjas**. **No se admiten pares cruzados**. Es un **requisito del modelo**, no de presentación (la alineación la imponen **R2**, **R7** y **R9** de la 014: emparejamiento, misma aula del par y huecos al final). |
| D-6 | Aula de referencia | **No cambia**: mientras un curso está desplazado, su referencia queda **vacía y bloqueada**, también con dos cursos desplazados a la vez. |
| D-7 | Capacidad y causa (2) | Capacidad de un tipo = **aulas × franjas × aforo** (cancha **1 × 80 × 2 = 160** frente a **96** clases de EF); el mensaje cita **demanda, aulas, franjas y aforo**. |
| D-8 | Contrato y presets | **Contrato 2.0 intacto**, presets sin cambios y **huella de datos sin cambios**. |
| D-9 | Seed | Los **datos no se tocan**; el informe pasa a **«Generable: sí»** con las cifras nuevas, y el seed **comprueba que el tipo de la cancha se reconoce** y falla con mensaje claro si no. |
| D-10 | Aceptación | **Test lento** (etiqueta de lentos, como la 014): siembra el centro, **compone la entrada desde la base** y **resuelve**: **876 clases**, EF en la cancha con aforo 2, **desvío 0**, huecos al final y **< 300 s**. **Complementa** al fixture sin EF de la 014. |
| D-11 | Normativa | Enmendar la constitución **§7.1.3** (sin solapamientos: «hasta su aforo»), **§7.1.5** (exclusividad, también en desplazamientos), **§7.3** (qué declara un aula) y **§7 «Comportamiento»** (capacidad del tipo con aforo y causa (2)); **§7.4 solo si se añade una validación nueva** (no prevista). **Supersesiones** en **005 (D8)**, **010 (D8, capacidad, causa (2), comprobación de salida)**, **014 (cancha compartida exigible)** y **017 (RF-9/RF-10 superadas)**. |

## Contexto y objetivo

El centro tiene **una sola cancha** para toda la Educación Física. En el conjunto real, la Educación
Física son **4 horas académicas por curso**: **96 clases**, **48 por turno**. Con la regla vigente de
**una clase por aula y franja**, la cancha ofrece **40 franjas por turno** (5 días × 8 franjas) y **la
demanda no cabe**; el conjunto se validó **por datos** y quedó **sin declarar generable**, con la causa
escrita en su informe y en sus guías.

La realidad del centro es otra: **la cancha admite dos grupos a la vez** (dos cursos con profesores
distintos), y comparten el patio **por bloques completos**: dos grupos no entran media hora cada uno,
entran las dos horas del bloque. El objetivo es que el motor **exprese** esa simultaneidad —sin aflojar
ninguna otra regla— y que el **centro real genere un horario válido en menos de 300 s** con la
Educación Física íntegramente en la cancha.

**Por qué el aforo no es la capacidad física.** El aula «cancha» del centro tiene capacidad **88**
alumnos (dos grupos completos de 44). Esa cifra es **de alumnos** y **no** dice cuántos **cursos**
pueden coincidir. El aforo que esta spec introduce es **de cursos simultáneos** y vale **2**; ambas
cifras conviven y no se sustituyen.

**Por qué solo pares completos.** Si dos cursos compartieran una sola franja, uno de ellos estaría
«partiendo» su bloque de dos horas en la cancha, lo que choca con la forma dura de la 014 y no describe
lo que ocurre en el patio. La alineación del bloque sale de **tres reglas de la 014**: **R2**
(*emparejamiento*: `ocupada[s] = par[s] + par[s−1]`, con `par` solo entre franjas del mismo segmento),
**R7** (*misma aula en las dos franjas del par*) y **R9** (*huecos al final del día*, que hace que las
franjas ocupadas sean un prefijo y, por tanto, que los pares empiecen alineados al inicio del segmento).
El requisito se enuncia como regla del modelo y se **verifica** en dos capas: **no** existe solución con
pares cruzados y, si una salida los tuviera, la comprobación los marca.

## Usuarios

- **Usuario académico que genera horarios del centro**, que necesita que el horario del liceo se pueda
  generar de verdad, con la Educación Física en la cancha y sin mover nada más.
- **Dirección y coordinación del liceo**, que saben que la cancha se comparte **por bloques** y no
  quieren un sistema que lo prohíba ni que invente aulas para la Educación Física.
- **Equipo de desarrollo y de pruebas**, que necesita una regla **estable**, **medible** y **acotada**
  (aforo 2, un solo recurso afectado) y un criterio de aceptación ejecutable.
- **Responsable de los datos del centro**, que necesita que la capacidad declarada de la cancha y el
  aforo de cursos no se confundan en los informes.

## Historias de usuario

- HU-1. Como usuario académico, quiero que el horario del centro **se genere** con la Educación Física
  en la cancha, para dejar de tener un conjunto que solo sirve de escaparate.
- HU-2. Como dirección, quiero que **dos cursos puedan compartir la cancha** con profesores distintos y
  **el mismo bloque de dos horas**, porque es lo que ocurre en el patio.
- HU-3. Como usuario académico, quiero que el resto de aulas siga **sin admitir dos clases a la vez**,
  para que la simultaneidad no se convierta en un desorden general.
- HU-4. Como miembro del equipo de pruebas, quiero un criterio **automático** que demuestre que el
  centro real genera, que la cancha nunca tiene tres cursos y que nadie comparte media franja, para no
  depender de una revisión a ojo.
- HU-5. Como responsable de los datos, quiero que los informes distingan **capacidad de alumnos** (88)
  de **aforo de cursos** (2) y que la capacidad de un tipo se explique con sus tres factores.

## Definiciones

- **Aforo simultáneo de un tipo de aula:** número máximo de **cursos distintos** que pueden tener clase
  en un aula de ese tipo **en la misma franja**. **Cancha = 2**; cualquier otro tipo = **1**.
- **Plaza de aula por franja:** cada uno de los huecos que un aula ofrece en una franja. Un aula con
  aforo 2 aporta **dos plazas** en cada franja.
- **Simultaneidad válida:** dos cursos **distintos**, con **docentes distintos**, en la **cancha**, la
  **misma franja** y **el mismo bloque de dos horas completo**.
- **Par completo compartido:** dos cursos cuyos bloques de dos horas ocupan **exactamente las mismas dos
  franjas adyacentes** en la cancha.
- **Par cruzado (prohibido):** dos cursos en la cancha que coinciden en **una sola franja** porque sus
  bloques empiezan en franjas distintas (por ejemplo, uno en 1–2 y otro en 2–3). **No es una solución.**
- **Capacidad física:** alumnos que caben en el aula (cancha **88**). **No** es el aforo y no interviene
  en la generación.
- **Cancha:** la única instalación deportiva del centro, de tipo cuya **clave normalizada es `cancha`**,
  que acoge toda la Educación Física. **Nunca** es aula de referencia de un curso.
- **Educación Física (EF):** materia que exige el tipo de la cancha; **4 horas académicas por curso (2
  bloques de 2 franjas)**; **96 clases**, **48 por turno**.
- **Generable:** el motor encuentra un horario que respeta **todas** las reglas duras (005/010/014) con
  los datos del centro.
- **Capacidad de un tipo de aula:** plazas que ofrecen sus aulas en las franjas que aportan los turnos
  de los cursos que lo requieren: **aulas × franjas × aforo**.
- **Posición de par:** par de franjas **adyacentes y alineado al inicio del segmento**, es decir, la
  posición que puede ocupar un bloque de dos horas. En un segmento de longitud par, las posiciones
  empiezan en los índices 0, 2, 4… **dentro del segmento**. En el centro, cada turno tiene dos segmentos
  (mañana `[0, 1]` y `[2 … 7]`; tarde `[8, 9]` y `[10 … 15]`): hay **6 pares adyacentes por día pero solo
  4 posiciones de par** (1 en el primer segmento + 3 en el segundo).
- **Plaza de par:** **posición de par × aforo**. Un turno ofrece **20 posiciones de par** (4 por día × 5
  días) y, con aforo 2, **40 plazas de par**. Un par de un curso ocupa **una posición**; dos cursos que
  comparten el bloque ocupan **la misma posición**, no dos distintas.

## Requisitos funcionales

### RF-1. Aforo fijado en 2 para la cancha y 1 para el resto

- **EL SISTEMA fijará el aforo simultáneo en 2 para el tipo de aula de la cancha y en 1 para cualquier
  otro tipo**, como regla propia del motor, sin que el usuario tenga que declararlo.
- **EL SISTEMA admitirá hasta dos cursos distintos en la cancha en la misma franja.**
- **EL SISTEMA mantendrá una sola clase por franja en las 12 aulas generales y en cualquier otro tipo.**
- **EL SISTEMA no derivará el aforo** de la capacidad física del aula (88 alumnos), del número de
  estudiantes de los cursos ni de ninguna otra cifra de los datos.
- **EL SISTEMA no admitirá tres cursos en la cancha en la misma franja** ni dos clases del mismo curso
  en la cancha en la misma franja.
- **EL SISTEMA mantendrá el aforo fuera del contrato de configuración, del esquema y de los presets**: un
  preset vigente se acepta sin cambios y el aforo **no** entra en la huella de datos.

### RF-2. Reconocimiento del tipo y fallo explícito

- **EL SISTEMA reconocerá la cancha por la clave normalizada `cancha` del tipo de aula** (recorte y
  plegado Unicode), igual que la convención del tipo general, y aplicará el aforo 2 solo a ese tipo.
- **SI el conjunto tiene materias que exigen el tipo de la cancha y ninguna aula declara ese tipo,
  ENTONCES EL SISTEMA rechazará la generación con un mensaje claro** que nombre la materia y el tipo, y
  **no** aplicará el aforo 1 en silencio.
- **SI el tipo de la cancha no se normaliza a `cancha`, ENTONCES EL SISTEMA no le aplicará el aforo 2**:
  usará aforo 1, y la capacidad y la causa (2) citarán su **nombre visible**, de modo que el fallo sea
  explicable; **nunca** afirmará que la cancha tiene aforo 2 sin haber reconocido el tipo.
- **EL SISTEMA identificará la cancha declarada del conjunto por su entrada y su nombre** («Cancha»; en
  el centro, la 13.ª aula) y **comparará su tipo por la clave normalizada**: la igualdad de texto exacta
  **no** es el predicado válido.
- **SI el conjunto que se va a sembrar declara una cancha cuyo tipo no tiene la clave `cancha` y hay
  materias que exigen ese tipo, ENTONCES EL SISTEMA rechazará la aplicación** con un mensaje que incluya
  el **tipo recibido**, la **clave esperada** (`cancha`) y la **acción** («el tipo de la cancha debe
  llamarse «Cancha»»), sin aplicar el conjunto con aforo 1.
- **EL SISTEMA no deducirá el aforo** por el nombre del aula, por su capacidad física ni por el número
  de aulas del tipo.

### RF-3. Condición de la simultaneidad: docentes distintos

- **MIENTRAS dos cursos ocupen la cancha en la misma franja, EL SISTEMA exigirá que sean cursos
  distintos y que los imparta docentes distintos.**
- **EL SISTEMA no creará una regla nueva para «docente distinto»**: es consecuencia de que un docente no
  puede dar dos clases a la vez; el requisito se enuncia para poder verificarlo.
- **EL SISTEMA no exigirá** que los dos cursos sean de años, planes, secciones o materias distintas.
- **SI un docente es el único candidato de Educación Física de dos cursos del mismo turno, ENTONCES EL
  SISTEMA no colocará esas dos clases en la misma franja** y usará otra posición de par, si la hay.

### RF-4. Nada más se relaja

- **EL SISTEMA mantendrá intactas** las invariantes y reglas vigentes: un docente por materia-curso,
  horas exactas de cada materia-curso, un curso sin dos clases a la vez, un docente sin dos clases a la
  vez, turno/plan/año del docente, tipo exacto de aula, aula de referencia con desplazamiento y vuelta,
  y la referencia **vacía y bloqueada** mientras el curso está desplazado.
- **EL SISTEMA mantendrá intactas** las reglas de la 014: bloques de dos franjas adyacentes dentro de un
  segmento, materia en bloques enteros salvo su resto, un bloque por materia y día, **misma aula en las
  dos franjas del par**, resto compartido entre dos materias impares del mismo tipo, **huecos al final
  del día**, separación blanda de un día y preferencia por el viernes para los huecos.
- **EL SISTEMA no reservará la cancha** como aula de referencia ni permitirá que sea referencia de un
  curso, y **no liberará la referencia** de un curso desplazado para otro curso.
- **EL SISTEMA no alterará el tope de horas de aula del docente** ni su tratamiento **(blando)**: el
  exceso se paga y se informa, no se convierte en infactibilidad.

### RF-5. Simultaneidad solo por bloques completos

- **EL SISTEMA solo admitirá dos cursos en la cancha cuando sus bloques de dos horas ocupen exactamente
  las mismas dos franjas adyacentes**, es decir, **la misma posición de par** (alineada al inicio del
  segmento).
- **EL SISTEMA no admitirá pares cruzados**: ninguna solución tendrá dos cursos en la cancha que
  coincidan en **una sola** franja.
- **SI un curso tuviera su bloque de cancha en las franjas 1–2 de su turno, ENTONCES otro curso podrá
  compartirlo solo en 1–2**, y no en 2–3.
- **EL SISTEMA contará la ocupación de la cancha por posición de par**: en cada posición caben **dos
  cursos** (su aforo) y, por tanto, en cada franja hay **como máximo dos clases, ambas del mismo par**.
- **EL SISTEMA mantendrá la invariante de la 014 de «un bloque por materia y día»** (es **por materia**,
  no por curso). En el centro hay **una sola materia de Educación Física por curso** (4 h = 2 bloques de
  2 franjas), así que cada curso tiene como máximo **un bloque de cancha por día** y sus **dos bloques
  caen en días distintos**; el sistema **no** añade una regla más estricta que la de la 014.
- **EL SISTEMA contará las posiciones de par disponibles**: hay **6 pares adyacentes por día** en cada
  turno, pero solo **4 posiciones de par** (1 en el primer segmento y 3 en el segundo); el turno ofrece
  **20 posiciones** y, con aforo 2, **40 plazas de par**, frente a **24 pares** de Educación Física.
- **EL SISTEMA demostrará que el modelo no admite pares cruzados**: forzando una configuración con un
  par cruzado, la instancia **no tiene solución** (**comprobación de modelo**). No se da por supuesto por
  la alineación de la 014: se prueba.

### RF-6. Capacidad del tipo y causa (2)

- **EL SISTEMA calculará la capacidad de un tipo de aula como aulas × franjas × aforo**, donde las
  franjas son las que aportan los turnos de los cursos que requieren el tipo y el aforo es el del tipo.
- **EL SISTEMA informará la capacidad de la cancha como 160 plazas semanales** (1 aula × 80 franjas × 2)
  y **80 por turno**, frente a **96** y **48** clases de Educación Física.
- **SI la demanda de un tipo supera su capacidad, ENTONCES EL SISTEMA declarará la causa (2) conflicto
  de aula** indicando **demanda, número de aulas, franjas y aforo**, y la acción disponible.
- **EL SISTEMA mantendrá que la capacidad es condición necesaria y no prueba** de generabilidad y **no
  relajará** ninguna validación por que la cuenta cuadre.
- **EL SISTEMA mantendrá el orden y la precedencia de causas** de la 010: si un curso no cabe en su
  turno, la causa sigue siendo **(3)** aunque la capacidad del tipo también falle.
- **EL SISTEMA no descontará** de la capacidad los bloqueos de las aulas de referencia.

### RF-7. Comprobación de la salida

- **CUANDO la comprobación de un horario generado encuentre dos clases en la misma aula y franja, EL
  SISTEMA solo lo marcará como conflicto si el número de clases supera el aforo de esa aula.**
- **EL SISTEMA aceptará como válido** que dos cursos distintos con docentes distintos compartan la
  cancha dentro del aforo.
- **SI un aula de aforo 1 recibe dos clases en la misma franja, ENTONCES EL SISTEMA lo marcará como
  conflicto** nombrando aula, día y franja.
- **SI la cancha recibe tres clases en la misma franja, ENTONCES EL SISTEMA lo marcará como conflicto**
  nombrando aula, día, franja, número de clases y aforo.
- **EL SISTEMA seguirá marcando como conflicto** que un mismo curso tenga dos clases a la vez y que un
  mismo docente esté en dos clases a la vez.
- **EL SISTEMA comprobará también que la cancha no tiene pares cruzados** en la salida y lo marcará como
  conflicto si los hubiera (**comprobación de salida**).
- **EL SISTEMA aplicará aforo 1 a toda aula cuyo tipo no tenga la clave `cancha`** —incluidos los tipos
  que no reconozca— **y a un aula cuyo índice no exista** en el conjunto; dos clases en cualquiera de
  ellas siguen siendo conflicto.
- **EL SISTEMA no contará como ocupación** una clase **sin aula** declarada (índice ausente o negativo):
  no ocupa franja de aula ni puede crear ni aliviar un conflicto de aforo.

### RF-8. El conjunto del centro pasa a ser generable

- **EL SISTEMA dejará de declarar el conjunto del centro como no generable** y lo declarará
  **generable**, informando la capacidad de la cancha con el aforo (**160 semanales, 80 por turno**)
  frente a sus **96** clases de Educación Física (**48 por turno**).
- **EL SISTEMA informará también las plazas de par** (**20 posiciones → 40 plazas por turno**, con aforo
  2) frente a los pares de Educación Física (**24 por turno**), como segunda comprobación de que la
  simultaneidad por bloques cabe.
- **EL SISTEMA no cambiará el modelo, el contrato ni el esquema, ni alterará los datos del centro**: 2
  planes, 85 materias, 24 cursos con turno, 12 aulas generales, 1 cancha, 40 minutos de hora académica y
  **876 clases** se mantienen; la **plantilla** incorpora **P033** por la **corrección de datos del
  2026-11-10** (**33 docentes**, **165 disponibilidades**, **272 asignaciones**, capacidad **1188**).
- **EL SISTEMA documentará** que la capacidad con aforo sigue siendo **necesaria y no prueba** de
  generabilidad.

### RF-9. Aceptación de generación del centro real

- **CUANDO se genere el horario del centro real** (24 cursos, 876 clases, **33 docentes**, 40 min, reglas de
  la 014), **EL SISTEMA devolverá un horario válido en menos de 300 s** para el proceso completo de
  cálculo.
- **EL SISTEMA garantizará en ese horario**: todas las clases de Educación Física **en la cancha**,
  ninguna aula con más clases por franja que su aforo, **ningún par cruzado** en la cancha, **desvío 0**
  entre las horas configuradas y las programadas por docente, y **huecos al final del día**.
- **EL SISTEMA informará en el resultado la separación** alcanzada (blanda) sin exigir un valor
  concreto.
- **EL SISTEMA verificará la aceptación de forma automática, con siembra del centro y composición de la
  entrada desde la base**, y **fuera de la suite normal** por ser lenta, como la aceptación de la 014.
- **EL SISTEMA mantendrá la aceptación sin Educación Física de la 014** como comprobación
  complementaria del efecto del aforo; **no** se retira.

### RF-10. Diagnosticar sin inventar causas

- **SI el centro real no genera, ENTONCES EL SISTEMA informará una causa del catálogo (1)–(5)** con sus
  cifras o declarará el fallo **«no atribuible»**, sin inventar una categoría nueva por el aforo.
- **EL SISTEMA no convertirá el aforo en una causa de no factibilidad**: un aforo mayor solo puede
  ampliar la capacidad, nunca reducirla.

### RF-11. Documentación, normativa y supersesiones

- **EL SISTEMA documentará en español**: el aforo por tipo y su valor fijo; su diferencia con la
  capacidad de alumnos; la capacidad de un tipo como aulas × franjas × aforo; la simultaneidad válida
  (cursos y docentes distintos, **bloque completo**); la prohibición de pares cruzados; la nueva lectura
  de la comprobación de la salida; y el estado **generable** del centro.
- **EL SISTEMA enmendará la constitución** en **§7.1.3** (sin solapamientos: «hasta su aforo»),
  **§7.1.5** (exclusividad de aula, también en los desplazamientos), **§7.3** (qué declara un aula) y
  **§7 «Comportamiento»** (capacidad del tipo con aforo y causa (2)). **§7.4 no se toca** salvo que
  aparezca una validación nueva (no prevista: el reconocimiento del tipo y la guarda del conjunto no
  añaden una V).
- **EL SISTEMA anotará las supersesiones** en **005** (D8), **010** (D8, capacidad, causa (2) y
  comprobación de la salida), **014** (cancha compartida exigible) y **017** (RF-9/RF-10 y la aceptación
  diferida superadas; ver RF-11 en esta spec).
- **EL SISTEMA actualizará las guías vivas de datos de prueba** y la documentación del contrato y de la
  interfaz, de modo que **ninguna** siga prometiendo un aforo pendiente ni un «no generable».
- **EL SISTEMA no añadirá pantallas ni diálogos nuevos**: el aforo se aplica al generar y se informa en
  los informes, las guías y la documentación de la interfaz.

## Requisitos no funcionales

- **RNF-1. Plazo:** el horario del centro real se obtiene en **menos de 300 s** de proceso completo.
- **RNF-2. Sin degradación:** el aforo **no** cambia el comportamiento de las aulas de aforo 1, ni las
  reglas de bloques, huecos, separación, turnos, tipos o referencia. Su efecto se limita a la cancha.
- **RNF-3. Determinismo del diagnóstico:** las cuentas de demanda, capacidad y plazas de par son
  deterministas y no dependen del reloj ni del orden de entrada; el **horario** resultante es válido
  pero no necesariamente idéntico entre ejecuciones (8 workers, semilla fija).
- **RNF-4. Mensajes en español:** toda causa, aviso o rechazo identifica el tipo o el aula, las cifras
  implicadas y, cuando exista, la acción disponible; nunca se muestra texto crudo del almacenamiento.
- **RNF-5. Datos intactos:** esta spec **no cambia** el modelo, el contrato ni el esquema, ni modifica los
  datos sembrados; la **plantilla** incorpora **P033** por la **corrección de datos de la 017
  (2026-11-10)** —**33 docentes**, **165 disponibilidades**, **272 asignaciones**, capacidad **1188**— y
  planes, materias, cursos, aulas y **876 clases** no cambian. **Ninguna verificación** de esta spec
  modifica los datos sembrados.
- **RNF-6. Trazabilidad:** cada RF tendrá al menos una comprobación verificable en las tareas.
- **RNF-7. Coste de la suite:** la aceptación de generación es **lenta** y se mantiene **fuera de la
  suite normal** (etiqueta de lentos). **«Línea base» significa que los tests existentes siguen pasando**
  y que la **suite normal no se degrada** (no se alarga de forma apreciable); se admiten **tests rápidos
  nuevos**. Los **recuentos concretos de tests** (hoy **61/61** en `backend` y **66/66** en `full-release`)
  **cambiarán** con los tests nuevos y se anotan al cerrar: **no** son un requisito de esta spec.
- **RNF-8. Contrato y presets intactos:** el contrato sigue en **2.0**, los presets vigentes se aceptan
  sin cambios y el aforo **no** entra en la huella de datos; un cambio en el aforo del motor no marca
  «datos anteriores».

## Casos límite

- **Un único docente de Educación Física en un turno:** los dos cursos no pueden coincidir con él; la
  capacidad real es menor que las plazas de par y, si no cabe, el fallo es **«no atribuible»** o del
  catálogo vigente, **no** una causa nueva por el aforo (RF-10).
- **Tres cursos en la cancha en la misma franja:** no se admite; el sistema usa otra posición de par o
  declara el fallo con la causa vigente (RF-1, RF-6).
- **Par cruzado:** un curso en 1–2 y otro en 2–3 (o cualquier par que comparta una sola franja) **no es
  solución** (comprobación de modelo) y, si apareciera en una salida, la **comprobación de salida** lo
  marca (RF-5, RF-7).
- **Par completo compartido:** dos cursos con su bloque de dos franjas en la cancha al mismo tiempo;
  válido; la cancha tiene dos clases en cada una de las dos franjas (RF-5).
- **Dos bloques de cancha del mismo curso el mismo día:** la 014 lo impide **por materia** (un bloque por
  materia y día), **no por curso**; en el centro hay **una sola materia de Educación Física por curso**
  (4 h), así que cada curso tiene a lo sumo **un bloque de cancha al día**. Si un plan futuro tuviera dos
  materias del mismo tipo, el límite seguiría siendo **por materia** y el aforo no añade otra regla
  (RF-5).
- **Curso desplazado a la cancha:** su aula de referencia queda **vacía y bloqueada** en esas franjas,
  también cuando el otro curso está simultáneamente en la cancha; no se libera la referencia para otro
  curso (RF-4).
- **Dos cursos desplazados a la vez:** ocurre con el aforo 2; las dos referencias quedan vacías y
  bloqueadas; ningún otro curso las usa (RF-4).
- **Cancha con una sola clase en el par:** válido; el aforo es un máximo, no un mínimo (RF-1).
- **Cancha sin ninguna clase:** válido (RF-1).
- **Aula general con dos cursos:** nunca; el aforo 1 se mantiene aunque el aula tenga capacidad física
  para dos grupos (RF-1, RNF-2).
- **Materia de otro tipo en la cancha:** no se admite; el tipo exacto sigue vigente (RF-4).
- **Centro con más de una cancha:** cada aula de ese tipo aporta su aforo a la capacidad del tipo
  (RF-6).
- **Renombrar el tipo de la cancha:** el aforo deja de reconocerse; el conjunto que se siembra se
  rechaza con mensaje claro y, en datos ya existentes, la capacidad y la causa (2) usan el nombre
  visible del tipo, sin aplicar aforo 1 en silencio (RF-2).
- **Conjunto con materias que exigen cancha y sin ninguna aula de ese tipo:** entrada inválida con el
  mensaje que nombra materia y tipo (RF-2).
- **Curso de 40 h (31060) con Educación Física:** el curso llena las 40 franjas del turno; sus dos
  bloques de EF siguen siendo pares completos en la cancha y no crean huecos (014).
- **Curso de 34/36 h:** sus huecos siguen al final del día; el aforo no los mueve (014).
- **Resto compartido por dos materias impares:** ninguna es Educación Física (4 h, par); el aforo no
  interviene en los restos compartidos (014).
- **Aforo 2 en un turno con una sola posición de par con plazas libres en la cancha:** dos cursos la
  comparten si sus docentes son distintos; si alguno necesita dos pares, no cabe y el fallo se informa
  con las causas vigentes (RF-5, RF-10).
- **Preset vigente sin aforo:** se sigue aceptando; el aforo no se exige al usuario ni viaja en el
  preset (RF-1, RNF-8).
- **Horario guardado con dos clases en la cancha:** es válido y no se marca en conflicto al comprobarlo
  si comparten el bloque completo (RF-7).

## Fuera de alcance

- **Configurar el aforo** por aula o por tipo, y **aforos mayores que 2** (D-1).
- **Aplicar el aforo a otros tipos** (laboratorios, talleres, aulas grandes) y **la dotación** de más
  canchas, laboratorios o talleres (D-2).
- **Cambiar la capacidad física** de la cancha (88 alumnos) o de las aulas (44 alumnos).
- **La identidad del curso con el plan** (año, sección, plan) y el plan persistido en el curso: specs
  propias.
- **El rendimiento y la descomposición del cálculo**: spec propia; aquí solo se exige el plazo de 300 s
  del centro real.
- **La interfaz de usuario**: no hay pantallas nuevas; se actualiza la documentación de la interfaz y se
  retira la promesa del aforo pendiente.
- **El preset maestro del centro** como archivo entregable: la aceptación compone la entrada desde la
  base (D-10).
- **La generación parcial, los turnos automáticos y la cobertura de emergencia** (antiguas 006/007).

## Criterios de finalización

1. La cancha admite **dos cursos distintos con docentes distintos** en la misma franja y **no** admite
   tres, ni dos del mismo curso; las 12 aulas generales siguen con **una** clase por franja (RF-1).
2. El aforo es **2 para el tipo de clave `cancha` y 1 para el resto**, sin viajar en el contrato ni en
   los presets, y sin entrar en la huella (RF-1, RNF-8).
3. **Comprobación de modelo:** una configuración forzada con un **par cruzado** en la cancha **no tiene
   solución**. **Comprobación de salida:** un horario con un par cruzado en la cancha se **marca como
   conflicto**. En cambio, dos cursos de Educación Física simultáneos **en la misma posición de par** son
   válidos (RF-5, RF-7).
4. Las demás reglas no cambian: un curso y un docente siguen sin solaparse; el tipo exacto, la
   referencia y el bloqueo, los bloques de dos horas, la misma aula del par y los huecos al final se
   comportan como antes del aforo; la separación sigue siendo blanda (RF-3, RF-4, RNF-2).
5. La capacidad de un tipo se informa como **aulas × franjas × aforo**: la cancha da **160** semanales
   y **80** por turno; la cuenta de pares usa **4 posiciones por día (1 + 3)** → **20 posiciones** → **40
   plazas de par** por turno; la causa (2), cuando aplica, cita demanda, aulas, franjas y aforo; el orden
   y la precedencia de causas se mantienen (RF-5, RF-6).
6. La comprobación de la salida **no** marca conflicto por dos clases en la cancha dentro del aforo y
   **sí** por tres, por dos en un aula de aforo 1 y por pares cruzados; los solapamientos de curso y de
   docente se siguen marcando (RF-7).
7. El tipo de la cancha se reconoce por su clave `cancha`; si no se reconoce, el fallo es **explícito**
   (mensaje con **tipo recibido**, **clave esperada** y **acción**), tanto al sembrar como al generar con
   materias que exigen cancha (RF-2).
8. El informe del conjunto declara **generable** el centro, con **48 ≤ 80 por turno**, **96 ≤ 160** y
   **24 pares ≤ 40 plazas de par por turno**; los datos del centro no cambian salvo la **corrección del
   2026-11-10** (**33 docentes**, **165 disponibilidades**, **272 asignaciones**, capacidad **1188**), y
   sin cambios de modelo, contrato ni esquema (RF-8, RNF-5).
9. El centro real genera en **menos de 300 s** un horario válido: EF en la cancha, ninguna aula por
   encima de su aforo, ningún par cruzado, **desvío 0**, huecos al final y separación informada (RF-9,
   RNF-1).
10. La aceptación siembra el centro, compone la entrada desde la base y está fuera de la suite normal por
    ser lenta; **los tests existentes siguen pasando** y la suite normal no se degrada (los recuentos
    pueden subir con tests rápidos nuevos); la aceptación sin EF de la 014 se mantiene (RF-9, RNF-7).
11. Si el centro no genera, el diagnóstico usa el catálogo (1)–(5) o «no atribuible», sin causas nuevas
    (RF-10).
12. La constitución queda enmendada en **§7.1.3**, **§7.1.5**, **§7.3** y **§7 «Comportamiento»**
    (capacidad con aforo y causa (2)); **§7.4** solo si aparece una validación nueva. **005, 010, 014 y
    017** llevan su nota de supersesión; las guías y la documentación de la interfaz ya no anuncian aforo
    pendiente ni «no generable» (RF-11).
13. Cada RF tiene al menos una comprobación verificable en las tareas (RNF-6).

## Dudas abiertas

**Ninguna.** Las once decisiones (D-1 … D-11) se cerraron el **2026-11-09** y están incorporadas a los
requisitos, las definiciones, los casos límite y los criterios. La spec quedó **aprobada el
2026-11-10**; el plan y las tareas viven en esta misma carpeta.
