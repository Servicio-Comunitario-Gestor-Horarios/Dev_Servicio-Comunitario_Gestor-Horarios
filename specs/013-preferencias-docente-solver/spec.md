# Spec 013 - Preferencias del docente en la generación

Estado: implementada (2026-11-06)

> **Alcance.** Esta spec hace que la generación **consuma** las preferencias horarias del docente que
> la **012** ya persiste, valida y expone. Cubre el **efecto en el modelo de generación**, la
> **representación de las preferencias en el contrato**, la **proyección** de los rangos de reloj sobre
> la rejilla, la **validación de entrada**, la **huella de datos**, el **estado de cada preferencia en
> el resultado** y la **coherencia normativa y documental**.
>
> **Fuera de alcance:** la interfaz de usuario (spec 001, otro equipo), el modelo persistido y sus
> servicios (012), el empaquetado de clases en bloques de dos horas (spec **014**, posterior a esta) y la
> generación parcial (descartada/diferida el 2026-11-04).
>
> **Estado de la spec.** Las ocho dudas (D-1 a D-8) las resolvió el usuario el **2026-11-04**; los
> hallazgos **H1–H17** del QA están incorporados en esta revisión. Las **decisiones del usuario no se han
> cambiado**: donde un hallazgo chocaba con una decisión, se ha corregido la **formulación** para que la
> decisión sea coherente y verificable. La spec está **aprobada (2026-11-04)**; el plan y las tareas
> siguen su propio camino de QA y aprobación.
>
> **Resumen de lo decidido.** Las preferencias son una **prioridad blanda** que **nunca** altera la
> factibilidad, la **elección de docente** ni el **exceso de horas** (se decide primero lo estructural y
> la preferencia solo decide **dónde** caen las clases); pesan **más que la agrupación y menos que un
> suplente**; **penalizan lo evitado y premian lo preferido** con el mismo peso y **por franja**; la
> franja debe caber **entera** en el rango y un evento **no** corta el rango; el contrato **sigue en
> 2.0** con una **sección opcional** que lleva **día, franjas y sentido por índice de docente**, ya
> proyectada; las preferencias **siempre vienen de los datos** (el preset no las aporta); a la huella
> entran **solo las que intervienen**, ya proyectadas; la validación es **V19** (forma y referencias); las
> preferencias que **no intervienen** se informan como **no aplicadas** con su motivo; y el efecto se
> limita a **las clases del propio docente**.
>
> **Orden de trabajo (decisión del usuario, 2026-11-04).** Primero **esta** spec (013) y después la
> **014** (las materias se asignan en bloques de dos horas). La 014 no se diseña aquí.

> **Superada en parte por la 014 (2026-11-07).** Quedan superados: su **RF-5** (jerarquía con S3), sus
> **criterios 6 y 10**, sus definiciones de **S3**, **`PESO_AGRUPACION`** y **generación de referencia** en
> lo que dependa de S3, y la **prueba de contrato que fija `PESO_AGRUPACION == 10`**. Las **fases** de
> generación pasan a **1** (referencia: reglas duras + S1 + S2), **2a** (separación + viernes, con `z*`/`e*`
> fijados) y **2b** (preferencia, con el valor de 2a fijado). La **garantía estructural** (reparto de
> docentes y exceso de horas) se mantiene **respecto de la nueva generación de referencia**. La **huella no
> cambia** respecto de la 013 y la **reproducibilidad del horario** queda superada (8 workers; misma
> entrada ⇒ válido, no idéntico).

## Contexto y objetivo

El centro ya guarda **cuándo puede** dar clase cada docente (su disponibilidad) y **cuándo prefiere**
darla o qué franjas quiere evitar (sus preferencias, spec 012), pero la generación las **ignora**. El
resultado es un horario formalmente correcto que puede caer entero en las peores horas de un docente: la
preferencia existe en los datos y no llega a quien decide.

El objetivo es que el horario generado **tenga en cuenta** las preferencias de cada docente y que eso
sea **verificable y explicable**: el usuario debe poder saber qué preferencias se respetaron, cuáles no,
cuáles no llegaron a aplicarse y por qué; y dos generaciones con los mismos datos deben dar el mismo
resultado.

**Por qué blanda, y por qué con una jerarquía y no solo con un peso por unidad.** La generación no puede
perder un horario posible por un dato de calidad: una preferencia no convierte una franja en prohibida ni
añade causas de infactibilidad. Pero tampoco puede mandar sobre las decisiones estructurales. Un orden de
pesos **por unidad** no basta para garantizarlo, porque el objetivo es aditivo: una materia-curso de
cinco horas en franjas evitadas acumula varias unidades y podría superar el coste de un suplente. Por eso
la garantía que se pide es **sobre el resultado**, no sobre la unidad: **con las mismas preferencias, la
asignación de docentes por materia-curso y el exceso de horas por docente son los de la generación de
referencia** (la misma entrada sin preferencias), y la preferencia solo elige **dónde** caen las clases.
La relación por unidad (`PESO_AGRUPACION < PESO_PREFERENCIA < COSTE_SUPLENTE`) expresa la importancia
relativa entre niveles; la garantía la da la **jerarquía** (RF-5), y su coste de cálculo lo mide el plan.

**Por qué ahora.** La 012 cerró el dato y dejó anotadas dos deudas explícitas que esta spec **debe
saldar**: (1) las preferencias **no** entran en la huella de datos mientras el solver no las consuma —
condición que se cumple en cuanto esta spec las haga intervenir —; y (2) el efecto en la generación, su
peso y su prioridad quedaron **fuera de alcance** de la 012 a propósito, para decidirse aquí.

**Qué no es.** No es un cambio del modelo persistido ni de sus servicios, ni una pantalla, ni una
recurrencia o calendario de excepciones, ni un cambio de la disponibilidad, de la elegibilidad de los
docentes ni del reparto de docentes.

## Usuarios

- **Usuario académico que genera horarios:** espera que el horario respete, en la medida de lo posible,
  las horas que cada docente prefiere o evita, y entender cuándo no ha podido ser.
- **Usuario que mantiene docentes:** registra las preferencias y espera que tengan efecto, sin que
  introducirlas rompa la generación ni le cambie el reparto de docentes.
- **Equipo del solver:** necesita una regla única, con la garantía formulada sobre el resultado,
  proyección sin ambigüedad y pruebas por caso.
- **Equipo del frontend:** necesita la forma de la sección opcional de preferencias y saber que la
  versión del contrato no cambia.
- **Equipo de desarrollo:** necesita que la constitución y las specs 005, 010, 012 y la de agrupación no
  contradigan el comportamiento nuevo y que la detección de «datos anteriores» siga siendo fiable.

## Historias de usuario

- HU-1. Como usuario académico, quiero que las franjas que un docente evita queden para el final, para
  que el horario respete su jornada.
- HU-2. Como usuario académico, quiero seguir generando horario aunque un docente tenga preferencias
  incompatibles con el resto, y que no me cambie el reparto de docentes, para no quedarme sin horario ni
  tener que rehacer la plantilla.
- HU-3. Como usuario que mantiene docentes, quiero que una preferencia expresada fuera de la semana o
  del turno no rompa la generación y me diga que no se aplicó, para poder registrar datos sin miedo.
- HU-4. Como usuario académico, quiero saber qué preferencias se respetaron, cuáles no y cuáles no
  llegaron a aplicarse, para decidir si cambio los datos o las acepto.
- HU-5. Como responsable de los datos, quiero que un cambio de preferencias que afectan al horario marque
  el horario guardado como «datos anteriores», para no presentar como vigente un horario desactualizado.
- HU-6. Como miembro del equipo del solver, quiero que las preferencias viajen en la configuración con
  una forma y una versión estables, para no depender de la base de datos.

## Definiciones

- **Preferencia horaria:** la de la spec 012: docente, **día** (0 = domingo … 6 = sábado), hora de
  inicio, hora de fin y **sentido** (**preferida** o **evitada**). Es una inclinación, **no**
  factibilidad, y no se mezcla con la disponibilidad.
- **Disponibilidad:** los rangos en los que el docente **puede** dar clase (modelo existente). Sigue
  siendo la única fuente de elegibilidad horaria del docente.
- **Franja:** unidad de la rejilla, identificada por **(día de la generación, slot absoluto del día)**,
  la misma unidad que la disponibilidad persiste en la configuración. `slot` es el índice absoluto dentro
  del día y suma los slots de todos los turnos del día, ordenados por hora de inicio.
- **Hora académica y bloque:** una **hora académica** es la unidad pedagógica de carga de una materia y
  equivale a **un bloque** y a **una franja** (`bloques(horas) = horas`, sin reparto ni redondeo; spec
  005). En esta spec, «bloque» **nunca** significa «dos horas»: el bloque de dos horas es la unidad de la
  spec 014.
- **Agrupación (S3):** la prioridad blanda que penaliza la fragmentación de cada materia-curso dentro del
  día, definida en `specs/009-agrupacion-bloques-materias/`; su peso es `PESO_AGRUPACION`.
- **Proyección (reloj → franja):** poner el rango de reloj de una preferencia sobre la rejilla de franjas
  de su día —la rejilla se ancla en el inicio de cada turno y avanza en pasos de la duración de la hora
  académica— y quedarse con las franjas que **caben enteras** en el rango. **No** es una conversión de
  horas académicas a bloques (esa conversión es la identidad): es situar un intervalo de reloj sobre una
  rejilla ya definida. Es responsabilidad de la **construcción de la entrada**, no del solver.
- **Día de la generación:** la generación numera los días **desde el lunes** (`0 = lunes`, …, hasta
  `num_dias - 1`). El día de dominio de una preferencia es **0 = domingo … 6 = sábado**; la conversión
  entre ambos es parte de la proyección y del filtro de día (RF-3).
- **Semana declarada:** los días que la generación considera (la dimensión de días de la configuración;
  hoy, de lunes a viernes). Un día que la semana declarada no incluye no se programa.
- **Docente participante:** docente que es **candidato elegible (titular o suplente) de al menos una
  materia-curso de esta generación**, con independencia de si aparece en la configuración y de si acaba
  con clases. Es la noción de elegibilidad previa de la spec 005 (V13), no una consecuencia del horario.
- **Preferencia que interviene:** preferencia cuyo docente **participa**, cuyo **día está en la semana
  declarada** y que, tras la proyección, **alcanza al menos una franja del turno de un curso donde su
  docente es candidato elegible**. Se calcula **antes de resolver** y no depende del horario obtenido
  (RF-3).
- **Preferencia no aplicada:** preferencia que no interviene. Sus motivos son **exhaustivos, disjuntos y
  con precedencia explícita**; se informa **un solo** motivo, el primero que aplique en este orden:
  1. **(c) El docente no participa:** no es candidato elegible de ninguna materia-curso de la generación.
  2. **(a) Día fuera de la semana declarada.**
  3. **(b) El rango no alcanza ninguna franja del turno de un curso donde el docente es candidato
     elegible:** incluye que el rango no coincida con ninguna franja de la rejilla **y** que coincida solo
     con franjas de turnos de cursos donde el docente **no** es candidato (por ejemplo, preferencia en
     franjas de la tarde para un docente que solo es candidato en cursos de la mañana).
- **Generación de referencia:** la que produce el motor con **la misma configuración sin la sección de
  preferencias** (o con la sección vacía). Es el patrón contra el que se comprueba la garantía de RF-1 y
  la no regresión de RNF-4.
- **Peso de preferencia (`PESO_PREFERENCIA`):** constante entera del motor con
  `PESO_AGRUPACION < PESO_PREFERENCIA < COSTE_SUPLENTE`. No viaja en la configuración.
- **Contrato de configuración:** la configuración versionada que recibe el solver (specs 005 y 010); es
  la **entrada completa** de una generación y no consulta los datos persistidos.
- **Huella de datos:** el resumen determinista de los datos que intervienen en una generación (specs 004
  y 012). Un cambio de los datos que la componen marca un horario guardado como «datos anteriores».
- **Causa no atribuible:** la de la spec 010; se mantiene tal cual.

## Requisitos funcionales

### RF-1. Ámbito y garantías

- **EL SISTEMA aplicará las preferencias de un docente únicamente a las clases que ese docente
  imparte**, en cualquier materia y curso que tenga asignados, y **nunca** a las clases de otro docente.
- **EL SISTEMA aplicará las preferencias igual a las clases en que el docente es titular y a las que
  cubre como suplente.**
- **EL SISTEMA mantendrá las preferencias separadas de la disponibilidad:** una preferencia **no**
  convierte a un docente sin disponibilidad en elegible ni quita elegibilidad a quien la tiene.
- **Garantía estructural (verificable): CUANDO se genere con preferencias, EL SISTEMA mantendrá la
  asignación de docente por materia-curso y el exceso de horas de cada docente iguales a los de la
  generación de referencia**, de modo que la preferencia solo decida **dónde** caen las clases.
  - **SI no existe ningún horario que respete esa garantía y mejore el cumplimiento, ENTONCES EL SISTEMA
    devolverá el de la generación de referencia:** la preferencia se incumple antes que cambiar de
    docente o exceder horas.
  - **La comprobación es comparativa:** para un mismo escenario, la asignación de docentes y el exceso
    por docente con preferencias son **idénticos** a los de la generación de referencia.
    > **Nota (014, 2026-11-07): la referencia es la nueva.** «La generación de referencia» es la **fase 1
    > de la 014** (reglas duras + S1 + S2, con `z*`/`e*`), no la generación previa a la 014: la igualdad se
    > exige **dentro de cada ejecución** y contra esa referencia, no contra un horario anterior. La
    > preferencia sigue sin cambiar **dónde** se asigna cada docente ni el exceso de horas.
- **EL SISTEMA no usará las preferencias para elegir qué docente dicta una materia-curso:** la
  elegibilidad por plan, turno, año y disponibilidad y la preferencia **titular antes que suplente (S2)**
  se mantienen **intactas**.
- **CUANDO un docente de la generación no tenga preferencias, EL SISTEMA lo tratará como un docente sin
  preferencias**, sin error y sin valor por defecto.
- **Riesgo declarado:** cumplir la garantía estructural puede exigir una jerarquía que domine **en
  agregado** y no solo por unidad, con coste en tiempo de cálculo; la estrategia y su medición son del
  plan (RF-5, RNF-3).

### RF-2. Representación de las preferencias en la configuración

- **EL SISTEMA mantendrá el contrato en la versión `2.0`** y añadirá la sección de preferencias como
  **opcional**:
  - **SI la configuración no declara la sección de preferencias, ENTONCES EL SISTEMA la aceptará** y la
    interpretará como «sin preferencias». Un contrato sin la sección y uno con la sección vacía son
    equivalentes.
  - **EL SISTEMA no cambiará la regla de rechazo de versiones de la spec 010:** la única versión
    aceptada sigue siendo `2.0`, y las anteriores o desconocidas se rechazan sin conversión.
- **CUANDO una preferencia viaje en la configuración, EL SISTEMA la expresará como su docente por
  índice, su día de la generación y las franjas alcanzadas, más su sentido** —la misma forma que la
  disponibilidad (`día` + `slot`), con el docente referenciado por **índice** como las aulas y las
  materias—. **La configuración no transporta rangos de reloj.**
- **CUANDO se construya la configuración, EL SISTEMA proyectará las preferencias de los datos sobre la
  rejilla** y emitirá solo las preferencias **intervinientes** (RF-3); un día que la semana declarada no
  incluye nunca llega a la configuración.
- **EL SISTEMA identificará cada preferencia por su docente dentro de la configuración** y mantendrá la
  configuración **autosuficiente**: el solver no consultará los datos persistidos.
- **CUANDO se guarde un preset, EL SISTEMA omitirá la sección de preferencias:** un preset **no persiste
  preferencias**.
- **CUANDO se cargue un preset que contenga la sección de preferencias, EL SISTEMA la ignorará** y volverá
  a leer las preferencias de los datos, del mismo modo que ocurre con la duración de la hora académica
  (spec 011).
- **CUANDO se serialice la configuración —no un preset—, EL SISTEMA conservará la sección de
  preferencias**, y es **en la configuración donde se cumple la equivalencia de ida y vuelta sin pérdida**:
  guardar y volver a cargar una configuración reproduce sus preferencias; los campos desconocidos siguen
  tolerándose.
- **EL SISTEMA aplicará este orden:** ignorar la sección del preset → construir la sección desde los
  datos → **validar** la configuración resultante (V19, RF-7).

### RF-3. Proyección de preferencias y filtro de día (construcción)

- **CUANDO se construya la entrada, EL SISTEMA proyectará cada preferencia de los datos sobre las
  franjas de su día** con una regla única: una franja queda alcanzada **solo si cabe entera** dentro del
  rango (`[inicio_franja, fin_franja) ⊆ [inicio, fin)`). Una franja que se solapa parcialmente con un
  borde **no** cuenta: no hay crédito parcial.
- **CUANDO se proyecte un rango, EL SISTEMA recorrerá todos los turnos de ese día**, de modo que un rango
  puede alcanzar franjas de la mañana y de la tarde; las franjas se acumulan en el identificador
  `(día de la generación, slot absoluto del día)`.
- **MIENTRAS un rango empiece antes del inicio de un turno, EL SISTEMA solo alcanzará las franjas de ese
  turno que caben enteras**; lo mismo si el rango acaba después del final del turno o cruza el final de
  un turno y el inicio del siguiente: cada turno se proyecta con su propia rejilla y el rango no se
  recorta entre turnos.
- **CUANDO el rango contenga un evento del turno (receso, acto), EL SISTEMA no lo tratará como un
  corte:** el rango se proyecta como un intervalo continuo, coherente con la disponibilidad y con que el
  solver ignora los eventos.
- **CUANDO se aplique el filtro de día, EL SISTEMA convertirá el día de dominio (0 = domingo … 6 =
  sábado) al día de la generación (0 = lunes …) y descartará la preferencia si el día resultante queda
  fuera de la semana declarada.** Reglas: **domingo no aplica nunca** (la semana declarada empieza en
  lunes); **sábado aplica solo si la semana declarada lo incluye**; los días descartados nunca llegan a
  la configuración (coherente con V12).
- **CUANDO el rango de una preferencia no alcance ninguna franja del turno de un curso donde su docente
  es candidato elegible, EL SISTEMA la conservará como no aplicada** —por el motivo **(b)**, salvo que
  aplique antes **(c)** o **(a)** por la precedencia de la definición— y **no** la incluirá en la
  configuración.
- **CUANDO termine la proyección, EL SISTEMA determinará qué preferencias intervienen** (las de RF-2),
  **calculado antes de resolver**: interviene si su docente **participa** (es candidato elegible de al
  menos una materia-curso), su **día está en la semana declarada** y, tras la proyección, **alcanza al
  menos una franja del turno de un curso donde ese docente es candidato elegible**.
- **EL SISTEMA no emitirá en la configuración ninguna preferencia no interviniente** y le asignará **un
  solo motivo**, el primero que aplique en este orden de precedencia:
  1. **(c) el docente no participa** — no es candidato elegible de ninguna materia-curso;
  2. **(a) día fuera de la semana declarada**;
  3. **(b) el rango no alcanza ninguna franja del turno de un curso donde el docente es candidato
     elegible** — cubre tanto que no coincida con la rejilla como que coincida solo con franjas de turnos
     de cursos donde el docente no es candidato (caso mañana/tarde).

  Los tres motivos son **exhaustivos y disjuntos** con esa precedencia: toda preferencia que no interviene
  recibe exactamente uno.
- **EL SISTEMA no fusionará ni reescribirá las preferencias persistidas:** la fusión de rangos del mismo
  sentido, si hace falta, es de presentación (spec 012). Una franja alcanzada por dos preferencias del
  mismo sentido se cuenta **una sola vez**.

### RF-4. Efecto del sentido en la generación

- **EL SISTEMA tratará las preferencias como una prioridad blanda:** nunca serán una restricción dura,
  **no** cambiarán la factibilidad del problema y **no** introducirán causas de infactibilidad nuevas.
- **CUANDO una clase de un docente caiga en una franja alcanzada por una preferencia «evitada», EL
  SISTEMA la penalizará** en el objetivo.
- **CUANDO una clase de un docente caiga en una franja alcanzada por una preferencia «preferida», EL
  SISTEMA la premiará** con el **mismo peso** y en la **misma unidad**. El premio se expresa como un
  **coste negativo equivalente**, coherente con «minimizar una penalización»; una franja no alcanzada por
  ninguna preferencia no aporta ni resta.
- **EL SISTEMA medirá el cumplimiento por franja de clase:** cada franja ocupada por una clase en una
  franja alcanzada suma o resta una unidad de peso. Como una hora académica es un bloque y una franja,
  «por franja» y «por clase» coinciden hoy; la unidad normativa es la **franja**.
- **EL SISTEMA no alterará ninguna invariante dura ni las prioridades superiores** para mejorar el
  cumplimiento: ni la exclusividad de curso, profesor y aula, ni el turno, plan o año del docente, ni las
  horas de la materia-curso, ni el exceso de horas, ni el reparto de docentes (RF-1, RF-5).
- **SI la configuración no declara preferencias intervinientes, ENTONCES EL SISTEMA producirá exactamente
  el mismo horario que la generación de referencia**, sin cambio de coste ni de desempate.
- **CUANDO existan preferencias, EL SISTEMA sí podrá cambiar el horario** (dónde caen las clases)
  respecto de la generación de referencia; eso es su propósito. El resultado sigue siendo determinista y
  reproducible (RNF-2).
- **EL SISTEMA no alterará las preferencias persistidas para mejorar el cumplimiento.**

### RF-5. Jerarquía de prioridades y peso

- **EL SISTEMA aplicará una jerarquía de prioridades, de mayor a menor peso, en este orden: S1 exceso de
  horas de aula (`PESO_HORAS`) · S2 suplentes (`COSTE_SUPLENTE`) · preferencia horaria del docente
  (`PESO_PREFERENCIA`) · S3 agrupación de bloques (`PESO_AGRUPACION`).**
- **MIENTRAS las preferencias convivan con las prioridades superiores, EL SISTEMA decidirá primero S1 y
  S2 y solo después el cumplimiento de las preferencias**, con el resultado exigido en RF-1: los valores
  de S1 y S2 y la asignación de docente por materia-curso son los de la generación de referencia.
  - La **relación de orden por unidad** (`PESO_AGRUPACION < PESO_PREFERENCIA < COSTE_SUPLENTE`) expresa
    la importancia relativa entre niveles, pero **por sí sola no garantiza** la igualdad de S1/S2, porque
    el objetivo es aditivo y varias unidades de preferencia se acumulan. **La garantía se exige sobre el
    resultado**, y su logro (jerarquía en agregado, resolución por etapas o cualquier otra técnica) es del
    plan.
- **EL SISTEMA fijará `PESO_PREFERENCIA` como constante entera del motor**, con
  `PESO_AGRUPACION < PESO_PREFERENCIA < COSTE_SUPLENTE` (hoy, entre 20 y 50, con agrupación 10 y suplente
  100). El valor exacto lo fija el plan. El peso **no** viaja en la configuración y **no** se declara por
  rango ni por docente.
- **EL SISTEMA aplicará el mismo peso a la penalización de lo evitado y al premio de lo preferido**, de
  modo que mover una clase de una franja evitada a una preferida mejora el objetivo en dos unidades de
  peso, y moverla de una evitada a una franja neutra en una.
- **EL SISTEMA expresará el cumplimiento en la misma escala entera que las prioridades existentes**, sin
  aritmética fraccionaria.
- **CUANDO dos horarios tengan el mismo exceso de horas, el mismo número de suplentes y el mismo
  cumplimiento, EL SISTEMA no exigirá un desempate determinado:** cualquiera de ellos es válido (como en
  la spec 005), sin perjuicio de la reproducibilidad de RNF-2.
- **SI el cumplimiento de la jerarquía exigiera un coste de cálculo desproporcionado, ENTONCES EL SISTEMA
  devolverá el mejor horario que cumpla la garantía estructural dentro del plazo**, y el plan medirá y
  documentará ese margen (RNF-3).

### RF-6. Huella de datos

- **MIENTRAS exista al menos una preferencia interviniente, EL SISTEMA la incluirá en la huella de
  datos**, de modo que un cambio de preferencias entre el inicio de una generación y la recepción de su
  resultado marque el horario como **«datos anteriores»**.
- **EL SISTEMA compondrá la huella de datos con:** la instantánea de datos del dominio, la duración de la
  hora académica y la **proyección de las preferencias intervinientes** sobre la semana declarada; la
  huella se compone del docente, el día y la franja del par alcanzado, más su sentido, **no** del rango de
  reloj declarado.
- **SI una preferencia no interviene —por cualquiera de los motivos (a)–(c) de RF-3—, ENTONCES EL SISTEMA
  no la incluirá en la huella** y un cambio en ella **no** marcará el horario como «datos anteriores».
- **CUANDO un cambio de preset altere el conjunto de preferencias intervinientes** (por ejemplo, los días
  de la semana declarada o el turno de un curso), **ENTONCES la huella cambiará**, porque cambia la
  proyección.
- **SI la generación no tiene ninguna preferencia interviniente, ENTONCES EL SISTEMA no alterará el valor
  de la huella** respecto del que produce hoy: sin preferencias intervinientes, la huella es la de la
  generación de referencia.
- **EL SISTEMA dejará constancia documental de qué preferencias entran en la huella y de la forma de la
  proyección**, para que la detección de «datos anteriores» sea explicable.
- **El valor de la huella de toda generación con al menos una preferencia interviniente cambia respecto
  de las anteriores a esta spec**: los horarios guardados antes se considerarán de datos anteriores. Es
  una consecuencia declarada y no se migran horarios guardados.

### RF-7. Validación de la entrada (V19)

- **EL SISTEMA incorporará una sola validación nueva, V19, al catálogo de las specs 005 y 010**; el
  catálogo vigente pasa a **V1–V19**.
- **V19 → SI una preferencia de la configuración incumple la forma o referencia algo inexistente,
  ENTONCES EL SISTEMA rechazará la entrada como entrada inválida e informará el primer defecto en este
  orden:**
  1. **índice de docente** fuera del rango de docentes de la configuración;
  2. **día** fuera del rango de días de la generación (`0 … num_dias − 1`);
  3. **franja** fuera del rango de slots del día (`0 … num_slots_dia − 1`);
  4. **sentido** distinto de «preferida» o «evitada».
- **V19 no comprobará rangos de reloj, coherencia con el turno, el plan o los años, ni solapes:** la
  sección llega ya proyectada, así que «hora de inicio anterior a la de fin» pertenece a la proyección y a
  la validación de la 012 al guardar, **no** al contrato.
- **V19 será alcanzable por invocación directa del solver** con una configuración elaborada a mano; en el
  camino normal, la construcción descarta los días no aplicables y proyecta los rangos, de modo que solo
  un defecto de los cuatro anteriores puede llegar.
- **EL SISTEMA no rechazará la entrada por preferencias que no llegan a aplicar:** las no intervinientes
  no se emiten en la configuración (RF-2, RF-3) y nunca son un rechazo.

### RF-8. Estado por preferencia y artefacto observable

- **EL SISTEMA asignará a cada preferencia de la generación un estado, con esta precedencia:**
  1. **No aplicada**, si no interviene; con **un solo motivo**, el primero que aplique en la precedencia
     (c) docente que no participa → (a) día fuera de la semana declarada → (b) rango sin franjas del turno
     de un curso donde el docente es candidato elegible, y su explicación.
  2. **Incumplida**, si interviene y: es **evitada** y al menos una de sus franjas lleva una clase de su
     docente; o es **preferida** y ninguna de sus franjas alcanzables lleva una clase de su docente.
  3. **Respetada**, en el resto de los casos: **evitada** sin ninguna clase en sus franjas; **preferida**
     con al menos una clase en sus franjas alcanzables.
- **CUANDO una preferencia evitada esté parcialmente ocupada, EL SISTEMA la informará como incumplida**
  e indicará cuántas franjas y qué clases la incumplen.
- **CUANDO un docente no tenga disponibilidad bastante para respetar una preferencia, EL SISTEMA no la
  contará como incumplida si no intervino** (por ejemplo, porque no es candidato elegible): no aplicada
  con su motivo. Si intervino, se informa con el estado que resulte del horario.
- **EL SISTEMA expondrá el estado de cada preferencia en el resultado de la generación que publica el
  backend** —el artefacto observable es ese resultado, no la interfaz, que está fuera de alcance—, con el
  docente, el día, las franjas y el sentido de cada preferencia y su motivo cuando no aplique.
- **EL SISTEMA calculará ese estado sobre el horario devuelto por el solver y la proyección ya
  calculada**, sin cambiar el contrato de salida del solver; el desglose por preferencia no viaja en la
  salida del solver, y el contrato de salida se mantiene tal cual.
- **CUANDO termine una generación, EL SISTEMA informará además el cumplimiento agregado**: número de
  preferencias respetadas, incumplidas y no aplicadas.
- **EL SISTEMA no incluirá las preferencias entre las causas de un resultado no factible**, no atribuirá
  a las preferencias ningún fallo de factibilidad ni emitirá avisos que sugieran corregirlas para poder
  generar; las categorías de la spec 010 se mantienen tal cual.

### RF-9. Coherencia normativa y documentación

- **EL SISTEMA actualizará la documentación viva del contrato de configuración** con la sección opcional
  de preferencias (día, franjas, sentido, índice de docente), la regla de proyección, el efecto en el
  objetivo, la jerarquía de prioridades, el peso y V19, y con la constancia de que la versión sigue
  siendo `2.0` y de que el contrato pasa a tener **dos secciones opcionales**: **`aula_fija`** (ya
  existente) y **`preferencias`** (nueva). El objeto **`meta`** **no** es una sección del contrato: es un
  objeto tolerado/extra que añade el constructor y que se documenta como tal.
- **EL SISTEMA acotará la equivalencia de ida y vuelta de las specs 005 y 010 para la sección de
  preferencias:** la equivalencia se cumple en la **serialización de la configuración**, **no** en el
  preset —el preset se guarda **sin** la sección y la **ignora** al cargarse—, y la documentación de ambas
  specs recogerá esa acotación para que la regla general de equivalencia no se lea como si el preset
  tuviera que conservar preferencias.
- **EL SISTEMA alineará `docs/constitution.md` §7.2 con el objetivo vigente:** hoy solo recoge **S1** y
  **S2** y **no menciona S3**; la enmienda debe listar **S1, S2, la preferencia horaria del docente y
  S3** en su orden de pesos (`PESO_HORAS > COSTE_SUPLENTE > PESO_PREFERENCIA > PESO_AGRUPACION`), dejar
  constancia de que las preferencias son blandas y no alteran la factibilidad, y de que no cambian el
  reparto de docentes ni el exceso de horas. La enmienda es **requisito de esta spec y tarea del plan**;
  no se aplica al redactar la spec.
- **EL SISTEMA mantendrá coherente §7.4** con **V1–V19** y **§7.5** con las preferencias que no
  intervienen.
- **EL SISTEMA corregirá las citas obsoletas del rango de validaciones** en `docs/constitution.md` §7.4,
  en `docs/contrato-solver-2.0.md` —que cita «V1–V15» en un punto y «V1–V18» en otro— y en las citas de
  las specs **005** y **010**, de modo que todas digan **V1–V19**.
- **EL SISTEMA anotará las supersesiones** en las specs afectadas: en la **012**, que su RF-5 («las
  preferencias no entran en la huella») y su nota de «fuera de alcance» quedan superadas por esta spec; en
  la **005 y la 010**, lo que cambie del catálogo de prioridades, validaciones o causas **y la acotación
  de su equivalencia de ida y vuelta** (005, definición «Configuración equivalente»; 010, RF de ida y
  vuelta): para la sección de preferencias, la equivalencia se cumple en la configuración, no en el
  preset; y en `specs/009-agrupacion-bloques-materias/`, el lugar que ocupa la preferencia en el objetivo
  (por debajo de S2 y por encima de S3), **sin** cambiar su definición de S3.
- **Riesgo documental declarado (no se resuelve aquí):** la colisión de numeración entre las dos «009» del
  repositorio (`009-tipos-de-aula-y-eventos` y `009-agrupacion-bloques-materias`) y el uso de «S3» para
  dos cosas distintas (la agrupación de bloques y la antigua cobertura de emergencia de la 005) deben
  quedar anotados en la documentación para que ninguna cita sea ambigua.
- **La documentación advertirá de que las preferencias no convierten en elegible a un docente, no cambian
  el reparto de docentes, no alteran la factibilidad y no intervienen fuera de la semana declarada.**

## Requisitos no funcionales

- **RNF-1. Mensajes en español:** todo rechazo identifica el campo, el valor recibido, el rango admitido y
  la acción disponible; todo estado de preferencia identifica docente, día, franjas, sentido y motivo.
- **RNF-2. Determinismo y reproducibilidad:** con preferencias y sin ellas, la misma configuración y los
  mismos datos producen el mismo horario; el orden en que se declaren las preferencias no altera el
  resultado (se canonizan antes de construir el objetivo). Las preferencias **sí** cambian el horario
  respecto de la generación de referencia (RF-4); lo que no cambia es que el resultado sea único para una
  entrada dada.
  > **Superado en la reproducibilidad del horario por la 014 (2026-11-06).** La 014 busca con **8 workers**
  > y **no promete el mismo horario entre ejecuciones**: la misma entrada produce un horario **válido**, no
  > necesariamente idéntico. La validez y la garantía estructural **dentro de cada ejecución** (respecto de
  > su propia referencia) se mantienen.
- **RNF-3. Desempeño:** el escenario de referencia del proyecto (13 cursos, 2 turnos, 16 franjas) se
  resuelve por debajo del **plazo vigente del proceso de cálculo, 300 s** (002; la 014b supersede el plazo
  anterior de un minuto), y su tiempo de cálculo se mide y documenta con su margen (RF-5).
  > **Superado el «+50 %» por la 014 (2026-11-07).** La comparación «no supera en más del 50 % el tiempo de
  > la generación de referencia» valía con **1 worker** y una sola referencia. La 014 usa **8 workers** y
  > **tres fases** (1 referencia, 2a separación+viernes, 2b preferencia) con presupuestos 60/20/10, y mide
  > **172,6 s** en el centro completo (848 clases). El requisito vigente es el **plazo de 300 s**; la
  > comparación relativa se sustituye por **documentar el margen medido** en `journal.md`.
- **RNF-4. Sin regresión:** sin preferencias intervinientes, el horario generado y su coste son idénticos
  a los actuales; las invariantes duras, las prioridades S1/S2/S3 y las validaciones existentes no
  cambian. La comprobación se ancla al escenario de datos de prueba y su preset vigentes en `bb1767a`.
  > **Ancla de la verificación — supersesión (2026-11-05).** El requisito de RNF-4 **no cambia**; cambia
  > el **escenario contra el que se comprueba**: con la 015 (esquema **v6** y tipo general garantizado
  > por el esquema) y la 016, el ancla pasa a ser el **escenario posterior a la 015/016** —perfil del seed
  > **sin** «Aula de estudio» y base **v6** con el tipo general garantizado— en lugar del escenario de
  > `bb1767a`. Es un cambio de **referencia de comprobación**, no de comportamiento.
  > **Superado por la 014 (2026-11-07): S1/S2/S3 no se mantienen y «idéntico a los actuales» cambia.** La
  > 014 **elimina S3** y sustituye la agrupación por el emparejamiento y la alineación; el motor busca con
  > **8 workers**, así que la comparación «mismo horario y mismo coste» se sustituye por **validez e
  > invariantes** (RNF-1 de la 014) y por la **garantía estructural respecto de la nueva generación de
  > referencia** (fases 1/2a/2b). S1 y S2 se conservan; la igualdad **bit a bit** con la salida anterior no
  > se exige (RF-13 de la 014).
- **RNF-5. Verificación sin la interfaz:** la construcción puede excluir la interfaz de usuario y sus
  pruebas; con esa configuración, backend, datos, seed, consola y sus pruebas compilan y pasan.
- **RNF-6. Protección de datos:** no se modifica ni migra ninguna preferencia persistida; no hay
  migración de esquema ni de presets guardados (constitución §5).
- **RNF-7. Trazabilidad:** cada RF tendrá al menos una comprobación verificable en las tareas; sin
  cobertura, la tarea no está hecha.

## Casos límite

- **Docente sin preferencias:** se genera igual que la generación de referencia, con el mismo horario y el
  mismo coste; no cuenta como incumplimiento (RF-1, RF-4).
- **Preferencia «evitada» que cubre todas las franjas de un docente en un día:** el motor la usa solo si
  no hay alternativa y el resultado la informa como incumplida; **nunca** declara no factible (RF-4,
  RF-8).
- **Preferencia «evitada» que cubre todas las franjas de todos sus cursos:** el horario se genera
  igualmente, con todas sus clases incumplidas, y se informa; el reparto de docentes y el exceso de horas
  siguen siendo los de la referencia (RF-1, RF-8).
- **Preferencias contradictorias entre dos docentes sobre la misma franja:** el modelo resuelve por
  optimización; no hay rechazo de entrada (RF-4).
- **Preferencia con rango que empieza o termina a mitad de una franja:** solo cuentan las franjas que
  caben enteras (RF-3).
- **Preferencia cuyo rango no cubre ninguna franja completa:** no aplica, motivo (b), o (c) si el docente
  no participa (RF-3, RF-8).
- **Rango que empieza antes del inicio del turno o acaba después de su final:** solo alcanza las franjas
  del turno que caben enteras; el resto del rango no aporta franjas (RF-3).
- **Rango que cruza el final del turno de la mañana y el inicio del de la tarde:** recorre **los dos
  turnos** con su propia rejilla y puede alcanzar franjas de mañana y de tarde (RF-3).
- **Rango que abarca el receso o un acto cívico:** el evento no corta el rango (RF-3).
- **Dos turnos con franjas que se solapan en el reloj:** cada turno se proyecta sobre su propia rejilla y
  la franja se identifica por su slot absoluto del día; no hay colisión de identificadores (RF-3).
- **Preferencia en domingo:** no aplica nunca, motivo (a) (RF-3).
- **Preferencia en sábado con semana declarada de lunes a viernes:** no aplica, motivo (a) (RF-3).
- **Preferencia en sábado con semana declarada que incluye el sábado:** aplica con normalidad (RF-3).
- **Preferencia en horas fuera de los turnos de todos los cursos:** no aplica; motivo (b) si el docente
  participa, y (c) si no participa (RF-3, RF-8).
- **Preferencia de un docente que no participa en la generación** (no es candidato elegible de ninguna
  materia-curso): no aplica, motivo **(c)**, que **tiene precedencia** sobre (a) y (b) (RF-3, RF-8).
- **Preferencia en franjas de la tarde de un docente que solo es candidato en cursos de la mañana:** no
  aplica, motivo **(b)**: el rango sí coincide con la rejilla, pero no con el turno de un curso donde el
  docente es candidato (RF-3).
- **Nota de precedencia de los casos anteriores:** si el docente **no participa**, el motivo informado es
  siempre **(c)**, aunque concurra también (a) o (b); el resto de los casos recibe el primer motivo
  aplicable de la lista (RF-3).
- **Preferencia de un docente elegible que acaba sin ninguna clase asignada:** si intervino y es
  preferida, se informa como incumplida (no hay ninguna clase dentro); si es evitada, como respetada (no
  hay ninguna clase fuera ni dentro) (RF-8).
- **Preferencia evitada parcialmente ocupada:** incumplida, con el número de franjas y clases que la
  incumplen (RF-8).
- **Rangos contiguos del mismo sentido:** se proyectan por separado sin fusionar; una franja alcanzada
  por dos preferencias del mismo sentido se cuenta una sola vez (RF-3, RF-4).
- **Preferencia preferida y evitada del mismo docente que se solapan:** no puede venir de los datos
  persistidos (la 012 lo impide al guardar); si llega en una configuración hecha a mano, V19 no la
  rechaza (no comprueba solapes) y **ambos términos se aplican y se compensan** en las franjas comunes;
  en la huella aparecen las dos entradas. Los datos del centro no producen este caso (RF-4, RF-6, RF-7).
- **Clase que se mueve de una franja evitada a una preferida:** mejora el objetivo en dos unidades de
  peso; a una franja neutra, en una (RF-5).
- **Un horario donde respetar una preferencia obligaría a poner un suplente:** el motor **no** lo hace;
  prefiere incumplir la preferencia (RF-1, RF-5).
- **Un horario donde respetar una preferencia obligaría a exceder las horas de un docente:** el motor
  **no** lo hace (RF-1, RF-5).
- **Un horario donde respetar una preferencia obligaría a cambiar de titular:** el motor **no** lo hace:
  la asignación de docente es la de la referencia (RF-1).
- **Un horario donde respetar una preferencia obligaría a romper la agrupación:** el motor **sí** lo hace:
  la preferencia pesa más que la agrupación (RF-5).
- **Cinco horas de una materia en franjas evitadas (acumulación):** la preferencia no puede desbancar a
  S1 ni a S2 aunque la suma supere el coste de un suplente; la garantía es sobre el resultado (RF-1,
  RF-5).
- **Cambio de preferencias que intervienen entre el inicio y la recepción de una generación:** el horario
  se marca como «datos anteriores» (RF-6).
- **Cambio de una preferencia que no interviene:** la huella no cambia y el horario no se marca como
  «datos anteriores» (RF-6).
- **Cambio de preset que cambia los días declarados o el turno de un curso:** cambia el conjunto de
  preferencias intervinientes y, con él, la huella (RF-6).
- **Generación sin ninguna preferencia interviniente:** la huella vale lo mismo que hoy (RF-6).
- **Horario guardado antes de esta spec, generado con preferencias intervinientes:** su huella no
  coincide con la actual; se considera de datos anteriores y no se migra (RF-6).
- **Configuración 2.0 sin la sección de preferencias:** se acepta y equivale a «sin preferencias» (RF-2).
- **Configuración 2.0 con la sección vacía:** equivale a «sin preferencias» (RF-2).
- **Preset que contiene la sección de preferencias:** se ignora y se sobrescribe con las proyecciones de
  los datos; el preset no es fuente de preferencias (RF-2).
- **Contrato 1.0 o sin versión:** se rechaza como entrada inválida, sin conversión (regla de la 010, sin
  cambios) (RF-2).
- **Invocación directa del solver con una preferencia de día o franja fuera de rango, sentido inválido o
  índice de docente inexistente:** rechazo por V19 con el primer defecto en su orden (RF-7).
- **Configuración con una preferencia cuya franja no pertenece al turno de ningún curso:** V19 no la
  rechaza (no comprueba coherencia); en el camino normal no llega porque no interviene (RF-7).

## Fuera de alcance

- **La interfaz de usuario de preferencias:** la implementa otro equipo (spec 001); esta spec no toca
  vistas ni formularios. Hoy no hay pantalla para declarar preferencias: los datos llegan por los
  servicios de la 012.
- **El modelo persistido, sus validaciones y sus servicios:** son de la 012 y no se reabren; solo se anota
  la supersesión de su RF-5.
- **El contrato de salida del solver:** no cambia; el estado por preferencia lo calcula el backend (RF-8).
- **Recurrencias, excepciones por fecha, calendarios y pesos por día, por docente o por rango:** no
  existen en el modelo; si se necesitan, será otra spec.
- **Que las preferencias influyan en qué docente dicta una materia-curso:** queda fuera por decisión
  (D-8); S2 se mantiene tal cual.
- **Las preferencias como restricción dura:** queda fuera por decisión (D-1).
- **La validación de solapes de sentidos opuestos en el contrato:** la garantiza la 012 al persistir; el
  contrato no la repite (RF-7).
- **Un cambio incompatible de versión del contrato, de su regla de rechazo o de la forma de los presets:**
  no lo hay. La sección de preferencias es **opcional** y el preset **la omite**, de modo que un preset
  2.0 guardado antes sigue cargando; lo único que deja de valer es leer «preset = configuración» como
  igualdad completa, porque la preferencia vive en la configuración y no en el preset (RF-2, RF-9).
- **El empaquetado de las materias en bloques de dos horas:** es la spec **014**, posterior a esta.
- **La asignación automática de turno y la generación parcial:** la generación parcial queda
  **descartada/diferida** por decisión del usuario (2026-11-04).
- **La migración de bases, de presets o de horarios guardados.**
- **El aforo y la capacidad física de las aulas**, que siguen sin regla.

## Criterios de finalización

1. **Garantía estructural:** con las mismas preferencias, la asignación de docente por materia-curso y el
   exceso de horas por docente son **idénticos** a los de la generación de referencia (**la nueva: fase 1
   de la 014**); una prueba
   comparativa lo comprueba (RF-1, RF-5).
2. La sección de preferencias es **opcional** en el contrato `2.0`, lleva **día, franjas y sentido por
   índice de docente**, sin rangos de reloj, y sin ella equivale a «sin preferencias»; la
   **serialización de la configuración** la conserva sin pérdida, mientras que **guardar un preset la
   omite** y **cargarlo la ignora** (RF-2).
3. La proyección reloj → franja es única, con **franja entera** dentro del rango, recorre **todos los
   turnos** del día, no corta por eventos, y el **filtro de día** descarta domingo y el sábado no
   declarado **antes** de construir la entrada (RF-3).
4. Cada preferencia queda en **no aplicada** (con **un solo motivo**, por la precedencia
   **(c) > (a) > (b)**), **incumplida** o **respetada**, con las reglas de RF-8, calculadas **antes de
   resolver** para «interviene» y sobre el horario para el resto.
5. El sentido tiene efecto **solo** como prioridad blanda: penaliza lo evitado y premia lo preferido (como
   coste negativo equivalente), se mide **por franja** y nunca cambia la factibilidad (RF-4).
6. `PESO_PREFERENCIA` es una constante del motor con `PESO_AGRUPACION < PESO_PREFERENCIA <
   COSTE_SUPLENTE`, y la jerarquía **S1 > S2 > preferencia > S3** está fijada, con la garantía de
   criterio 1 exigida sobre el resultado y no solo por unidad (RF-5).
7. La huella incluye **solo las preferencias intervinientes proyectadas**; un cambio de una que interviene
   marca «datos anteriores», uno de una que no interviene no lo marca, y sin preferencias intervinientes
   la huella no cambia respecto de hoy (RF-6).
8. **V19** es la única validación nueva y el catálogo queda en **V1–V19**, con los cuatro defectos en su
   orden y sin comprobar rangos de reloj ni coherencia con el turno; es alcanzable por invocación directa
   del solver (RF-7).
9. El **resultado de la generación del backend** expone el estado de cada preferencia y el cumplimiento
   agregado, y ninguna preferencia aparece como causa de no factibilidad (RF-8).
10. La documentación del contrato (con **`aula_fija`** y **`preferencias`** como secciones opcionales,
    siendo `meta` un objeto tolerado y no una sección), la **constitución §7.2 con
    S1/S2/preferencia/S3**, §7.4 (**V1–V19**) y §7.5, las citas obsoletas corregidas en
    `docs/contrato-solver-2.0.md`, la **acotación de la equivalencia de ida y vuelta** en la 005 y la 010
    y las anotaciones de supersesión en la 012, la 005, la 010 y
    `specs/009-agrupacion-bloques-materias/` están alineadas; el riesgo de las dos «009» y del doble uso
    de «S3» queda anotado (RF-9).
11. Sin preferencias intervinientes, el horario y su coste son idénticos a los actuales, anclado al
    escenario de datos de prueba y preset de `bb1767a` (RNF-4).
    > **Ancla supersedida (2026-11-05):** la comparación se hace contra el escenario **posterior a la
    > 015/016** (seed sin «Aula de estudio», base v6 con el tipo general garantizado), no contra `bb1767a`.
    > El criterio es el mismo.
    > **Superado por la 014 (2026-11-07):** «idénticos a los actuales» ya no se exige. El criterio pasa a
    > **validez e invariantes** con **8 workers** (puede cambiar el horario) y a la **garantía estructural
    > frente a la nueva generación de referencia** (fase 1 de la 014). S3 eliminada.
12. El resultado es determinista y reproducible con y sin preferencias, y el orden de declaración de las
    preferencias no influye (RNF-2).
13. El escenario de referencia queda bajo el **plazo de 300 s** y el margen medido se documenta (RNF-3);
    el «+50 %» respecto de la generación de referencia queda **superado por la 014** (8 workers y fases
    1/2a/2b).
14. Cada RF tiene al menos una comprobación verificable en las tareas (RNF-7).

## Dudas abiertas

**Ninguna.** Las ocho dudas quedaron resueltas por el usuario el **2026-11-04** y su contenido está
incorporado a los RF (D-1 blanda · D-2 peso entre 20 y 50, premio y penalización con el mismo peso, unidad
por franja · D-3 franja entera y eventos que no cortan · D-4 contrato 2.0 con sección opcional y
preferencias siempre desde los datos · D-5 huella solo con las intervinientes proyectadas · D-6 validación
de forma y existencia · D-7 ignorar e informar como no aplicadas · D-8 solo las clases del propio
docente). Los hallazgos del QA (H1–H17) están resueltos en la redacción sin cambiar esas decisiones.

Quedan como **cuestiones diferidas ajenas a esta spec**: si en el futuro las preferencias deben influir en
qué docente dicta una materia-curso (cambio de requisitos), y la unificación de la numeración de las dos
«009» y del doble uso de «S3» (deuda documental anotada en RF-9).
