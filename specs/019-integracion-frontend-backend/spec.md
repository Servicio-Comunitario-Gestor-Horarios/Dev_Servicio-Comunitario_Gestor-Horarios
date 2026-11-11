# Spec 019 - Integración del frontend con el backend

Estado: aprobada (2026-11-10)

> **Alcance.** Esta spec cierra los **seis desajustes prioritarios** entre la interfaz y la lógica
> —**H1, H2, H3, H4, H6 y H8**— y **formaliza H5** en el esquema versionado. Todos salieron de la
> revisión de integración posterior al ensamblaje de las dos ramas de trabajo (backend 013–018 y
> frontend generación):
>
> | # | Desajuste | Gravedad |
> |---|---|---|
> | H1 | La cédula del docente exige solo dígitos, pero el conjunto del centro identifica a los docentes con `P001`…`P033`: **la ficha se abre, pero la edición y el guardado se rechazan en todos los docentes sembrados** | Bloqueante |
> | H2 | El resultado marcado como **«datos anteriores»** se trata como éxito: se escribe el archivo y se anuncia «guardado», sin confirmación y sin que el usuario lo pida | Bloqueante |
> | H3 | La **causa del NoFactible** que produce el cálculo se descarta y se sustituye por un mensaje genérico | Visible |
> | H4 | La tabla de docentes arranca con un valor derivado del **número de materias** y **no muestra el exceso** de horas | Visible |
> | H5 | Las **materias suplentes** viven en una relación que se crea al vuelo: no está en el esquema versionado, ni en el catálogo, ni en la huella, y el borrado de una materia no la comprueba | Estructural |
> | H6 | La detección de conflictos al mover clases marca como conflicto **cualquier aula compartida**, aunque la cancha admita dos cursos (018) | Visible |
> | H8 | La rejilla de preferencias solo alterna «preferida»: **no se puede expresar «evitada»** desde la pantalla aunque el dato existe y se guarda | Visible |
>
> Además, esta spec **regulariza el comportamiento de las pantallas afectadas** por esos desajustes: la
> **identidad/cédula y las preferencias** del docente, los **conflictos de la rejilla del horario** y la
> **vista de generación**. La **edición de disponibilidad y materias** del docente **no** se regulariza
> aquí y sigue en la deuda de proceso de `estado.md`. Del mismo modo, el resto de la **deuda de proceso**
> de las pantallas que se fusionaron sin especificación —**CRUD de planes, menú sin cuenta y
> ventana/leyenda**— **no se cierra aquí**: queda escrito en `estado.md` como deuda, sin darla por
> atendida.
>
> **Del resto de los nueve hallazgos —H7 y H9–H16— no se implementa nada en esta spec**, salvo las **dos
> partes ya declaradas** de **H10** (comprobación de la cédula) y **H14** (distinción de causas al borrar
> materias). Todo lo demás queda documentado como **deuda** con su referencia de código en `estado.md`,
> para una spec futura. **No** son requisitos de esta spec y no se convierten en RF.
>
> **Por qué ahora.** El ensamblaje de ambas ramas compila y las suites están verdes, pero la
> integración **no es usable**: dos de los desajustes impiden el trabajo normal (no se puede mantener a
> los 33 docentes del centro y un horario desactualizado se guarda solo, sin avisar), cuatro enseñan
> información falsa o incompleta al usuario y uno deja una relación de datos fuera del esquema. Cerrar
> estos seis y formalizar H5 antes de la spec de **rendimiento/descomposición** evita planificar sobre
> una interfaz que miente y sobre un esquema que no describe todo lo que guarda.

## Correspondencia hallazgo → requisito

| Hallazgo | Requisito |
|---|---|
| H1 | RF-1 |
| H2 | RF-2 |
| H3 | RF-3 |
| H4 | RF-4 |
| H5 | RF-5, RF-6, RF-7 |
| H6 | RF-8, RF-9 |
| H8 | RF-10 |

Ningún hallazgo del alcance queda sin requisito y ningún RF cubre un hallazgo fuera del alcance, **salvo
las dos partes ya declaradas de H10 (RF-1) y H14 (RF-6)**. El resto de la deuda (**H7 y H9–H16**) no
tiene RF: se describe en `estado.md`.

## Decisiones cerradas (2026-11-10)

| # | Decisión | Resolución | Origen |
|---|---|---|---|
| D-1 | Vehículo del cambio | Spec nueva **019-integracion-frontend-backend**. **Regulariza el comportamiento** de las pantallas afectadas por **H1, H2, H3, H4, H6 y H8**, más la formalización **H5**, y **deja escrito el resto de la deuda de proceso** (CRUD de planes, menú sin cuenta y ventana/leyenda, fuera de estos arreglos) como deuda en `estado.md`, **sin declararla cerrada**. | Usuario |
| D-2 | Alcance | **Los seis desajustes prioritarios (H1, H2, H3, H4, H6 y H8) más la formalización H5.** La deuda es **H7 y H9–H16** (nueve hallazgos), documentada en `estado.md`, no requisitos. | Usuario |
| D-3 | H1 cédula | **Eximir la validación cuando la cédula no cambia**, en lugar de aceptar identificadores alfanuméricos en todos los casos. Se exige una comprobación que **abra y guarde un docente sembrado** (`P033`). | Usuario |
| D-4 | H2 datos anteriores | **Ramificar por «datos anteriores»**, **pedir confirmación** al usuario y **no guardar hasta que el usuario lo pida**. Restaura 002 RF-1/RF-4 y la definición de huella de la 004. | Usuario |
| D-5 | H3 causa | **Propagar la causa real** del cálculo (categorías (1)–(5) o «no atribuible») hasta la interfaz, sin mensaje genérico que la tape. | Usuario |
| D-6 | H4 horas sugeridas | Usar las **horas sugeridas por docente** que ya calcula la composición (suma de las horas de sus materias en los cursos configurados) y **avisar del exceso** frente a las **horas en aula declaradas**, manteniendo la coherencia de la 005 y sin tope legal fijo (P-5). | Usuario |
| D-7 | H5 materias suplentes | Formalizar la relación en el **esquema versionado v7**: tabla en el esquema, **catálogo de esquema esperado** y **huella congelada** actualizados; el **borrado de una materia referenciada** se bloquea o encadena **según corresponda**; **cascada del docente**; **sin migración** (una base v6 deja de abrir y exige respaldo y base nueva). | Usuario |
| D-8 | H6 cancha | Llevar el **aforo** (o las canchas) al documento de visualización y **eximir los pares de cursos y docentes distintos** conforme al **aforo 2** de la 018. | Usuario |
| D-9 | H8 preferencias | La rejilla debe permitir **ninguno → preferida → evitada** (o modo explícito) y **persistir el sentido**, con comprobación. | Usuario |
| D-10 | Deuda | **Del resto de H7 y H9–H16** (nueve hallazgos) **no se implementa nada aquí**, salvo las partes de **H10** (cédula, RF-1) y **H14** (causas al borrar materias, RF-6); se documentan con su hallazgo y su referencia. La deuda de proceso de las pantallas (CRUD de planes, menú sin cuenta, ventana/leyenda) tampoco se cierra aquí. | Usuario |

### Decisiones de redacción (coherencia con las specs vigentes)

| # | Decisión | Por qué | Alternativa descartada |
|---|---|---|---|
| P-1 | Se mantiene la regla de **solo dígitos para identificadores nuevos o modificados**; se exime únicamente el identificador **sin cambios**. | Es la opción que el usuario pidió primero y la que menos afloja la identidad; el conjunto sembrado vuelve a ser editable sin convertir la cédula en texto libre. | Aceptar identificadores alfanuméricos siempre: deja de haber una regla de identidad verificable y no hay datos que la exijan hoy. |
| P-2 | El borrado de una materia referenciada como **suplente** se **bloquea** con la causa, igual que hoy se bloquea si está en un plan o es titular de un docente. | La materia es un registro compartido: borrarla en cascada dejaría filas de otros docentes alteradas sin que el usuario lo vea; bloquear es reversible y conserva los datos (§5 de la constitución). | Cascada: borrar las asignaciones suplentes de otros docentes en silencio. |
| P-3 | Si el aforo de un aula **no** viene declarado en un horario guardado antes de este cambio, se asume **aforo 1** y dos clases en la cancha se señalan como conflicto. | El sistema no adivina el aforo ni abre un hueco silencioso (018 RF-7); el documento se puede regenerar. | Asumir aforo 2 en toda aula sin dato: abriría la puerta a tres cursos donde solo caben uno. |
| P-4 | La exención del aforo vale **solo para bloques completos**: si dos clases de la cancha comparten una sola franja, sigue siendo conflicto. | Es el requisito de la 018 (pares cruzados prohibidos); la pantalla no puede admitir lo que el modelo prohíbe. | Eximir por franja suelta: permitiría editar un horario que el motor no habría producido. |
| P-5 | La referencia del exceso son las **horas en aula que el usuario declara en cada fila**; **no** hay tope fijo de 36 h ni validación legal. Las **36 h** son el **valor sembrado de referencia** del centro (017, D-3): **no** son el inicio de la tabla (las dos columnas arrancan con las horas sugeridas) ni un máximo, y el usuario las usa como contexto al editarlas por convenio, siempre respetando la coherencia de la 005 (horas en aula **no** por encima de las horas requeridas). | El centro firma **convenios** de más o menos horas; un máximo duro impediría representar la práctica real. La coherencia de la 005 es un chequeo del dato, no una regla del motor, y se mantiene. | Tope fijo de 36 h como validación: contradice los convenios y obliga a falsear las horas. Relajar la coherencia de la 005: supersedería un requisito vigente sin necesidad. |

## Contexto y objetivo

Las dos mitades del producto —la lógica (base, motor, contrato) y la interfaz de escritorio— se
desarrollaron en paralelo y solo se han juntado al final. La base, el motor y el contrato están
cerrados y verificados por las specs 001–018; la interfaz de generación y visualización creció en su
propia rama, con pantallas que **nunca pasaron por spec** (docentes con disponibilidad, preferencias y
materias; alta y edición de la cédula; rejillas y ventana del horario). El ensamblaje compila y las
suites están verdes, pero la revisión de integración encontró **16 desajustes de comportamiento**.

Seis de ellos hacen daño hoy, y un séptimo dejó una relación de datos fuera del esquema:

- **No se puede mantener a los docentes del centro.** El formulario exige una cédula numérica y el
  conjunto identifica a los 33 docentes con `P001`…`P033`: la ficha se abre, pero al guardar cualquier
  cambio el sistema la rechaza, de modo que tampoco se pueden ajustar sus preferencias de generación.
- **Un horario viejo se guarda como si fuera bueno.** Cuando los datos cambian mientras el motor
  calcula, el resultado se marca «datos anteriores»; la vista lo trata como éxito, escribe el archivo y
  anuncia «guardado». El usuario conserva en disco un horario que no corresponde a sus datos y la
  confirmación que exigen la 002 y la 004 nunca llega a verse.
- **El fallo del cálculo no explica nada.** Si el motor concluye que no hay solución, la causa que
  calculó —conflicto de aula, de persona, de asignación, de bloques o entrada inválida, con sus
  cifras— se descarta y el usuario solo lee «no existe una solución factible». Sin la causa no puede
  corregir nada.
- **La carga docente se propone mal.** La tabla arranca con «dos horas por materia asignada», una
  cifra que no guarda relación con las horas reales del plan, y nunca advierte de que un docente acumula
  más horas de las que se le van a declarar en aula. El usuario ajusta 33 filas a ciegas.
- **La cancha se lee como si fuera un aula normal.** Al mover una clase, dos cursos distintos en la
  cancha con profesores distintos se señalan como conflicto, aunque la 018 acaba de establecer que
  caben dos.
- **No se puede expresar una preferencia negativa.** La rejilla solo enciende «preferida»; la
  preferencia «evitada» existe en el modelo, se guarda y se usa en la generación (012/013), pero
  desde la pantalla es imposible crearla.

**Objetivo.** Devolver a la interfaz la fidelidad con lo que la lógica ya garantiza: que los docentes
del centro se puedan mantener, que un horario solo se guarde cuando el usuario lo pida y sepa si
corresponde a datos anteriores, que un fallo explique su causa, que las horas propuestas sean las
reales y muestren el exceso, que la cancha se lea con su aforo y que la rejilla permita las dos
preferencias. De paso, dejar **descrita en el esquema versionado** la relación de materias suplentes
que hoy se crea al vuelo y que ninguna spec recoge.

**Qué no es.** No es un rediseño de la interfaz ni una spec de rendimiento; no cambia el modelo del
solver, el contrato 2.0, los presets, la huella de datos ni los datos del centro; no aplica el aforo a
otros tipos de aula; no introduce migraciones de esquema; y no convierte en requisitos el resto de los
**nueve** hallazgos (**H7 y H9–H16**), salvo las dos partes ya declaradas de **H10** (RF-1) y **H14**
(RF-6).

## Usuarios

- **Usuario académico que mantiene los docentes del centro**, que necesita abrir la ficha, corregir su
  identidad (cédula), nombre, correo y preferencias y guardar sin que el identificador del centro le
  bloquee. *(La edición de disponibilidad y materias no se regulariza en esta spec; ver `estado.md`.)*
- **Usuario académico que genera horarios**, que necesita saber si el horario que ve corresponde a sus
  datos, decidir cuándo se guarda y entender por qué el motor no encontró solución.
- **Dirección y coordinación del liceo**, que ajustan la carga de 33 docentes y saben cuántas horas
  puede asumir cada uno.
- **Usuario que revisa y edita un horario generado**, que mueve clases entre franjas y necesita que la
  cancha se trate como cancha.
- **Equipo de datos y de pruebas**, que necesita un esquema **descrito y verificable** en el que la
  relación de materias suplentes no sea un añadido invisible y que pueda afirmar sobre bases nuevas.
- **Equipo de desarrollo**, que necesita que cada desajuste quede con un criterio de aceptación
  ejecutable y que la deuda restante esté escrita para no perderla.

## Historias de usuario

- HU-1. Como usuario que mantiene docentes, quiero poder abrir y guardar un docente identificado con
  el código del centro, para no tener que renombrarlo para poder trabajar con él.
- HU-2. Como usuario que genera horarios, quiero que un horario calculado con datos que ya cambiaron
  no se guarde por su cuenta y que se me pida confirmación, para no conservar un horario que no
  corresponde a mis datos.
- HU-3. Como usuario que genera horarios, quiero decidir cuándo se guarda el archivo, para que solo se
  conserve lo que yo apruebo.
- HU-4. Como usuario que genera horarios, quiero leer la causa real de un fallo del cálculo, para saber
  qué dato corregir.
- HU-5. Como usuario que ajusta la carga docente, quiero que las horas propuestas salgan de las horas
  reales de las materias y ver cuándo un docente acumula más horas de las que se le declaran en aula,
  para decidir con información y poder reflejar convenios por encima o por debajo de 36 horas.
- HU-6. Como usuario que revisa un horario, quiero poder mover una clase a la cancha junto a otro curso
  sin que el sistema lo llame conflicto, para editar lo que el motor ya admite.
- HU-7. Como usuario que mantiene docentes, quiero marcar horas como preferidas o evitadas con la misma
  rejilla, para expresar las dos caras de la preferencia.
- HU-8. Como equipo de pruebas, quiero que la relación de materias suplentes esté en el esquema
  versionado y con una comprobación de apertura, para que ninguna base se quede a medias.

## Definiciones

- **Identificador del docente (cédula):** el valor con el que se registra y se busca a un docente; es
  su identidad en la base. El conjunto del centro usa códigos como `P001`…`P033`; la interfaz admite
  además cédulas numéricas para altas nuevas.
- **Horario presentado:** resultado del cálculo que la interfaz muestra al usuario. No está guardado.
- **Horario guardado:** archivo que el usuario decide crear a partir de un horario presentado (002, 004).
- **Datos anteriores:** condición de un resultado cuyo cálculo se hizo sobre una instantánea de los
  datos que ya no coincide con la actual (definición de huella de datos de 004). Exige confirmación
  explícita antes de guardarlo.
- **Causa de un resultado no factible:** la categoría (1)–(5) del catálogo vigente —entrada inválida,
  conflicto de aula, conflicto de persona, conflicto de asignación de docente y conflicto de bloques—
  con sus cifras, o la declaración de **«no atribuible»** (010/014/018).
- **Horas sugeridas de un docente:** suma de las horas semanales de las materias que imparte, contadas
  **por curso** en que las imparte, según los planes de los cursos configurados.
- **Horas en aula declaradas:** horas académicas semanales que el usuario declara para un docente en su
  fila de configuración. Es la **referencia del exceso** y se entrega al cálculo como el tope de horas de
  aula de ese docente. Admite **convenios** por encima o por debajo de las horas sugeridas (y del valor
  de referencia del centro), **siempre que no queden por encima de las horas requeridas**: esa coherencia
  es un chequeo del dato de la 005 y se mantiene.
- **Horas requeridas:** horas académicas semanales de la carga del docente. Es un dato **editable** que
  sirve de cota superior a las horas en aula; **no** es la referencia del exceso.
- **Exceso:** diferencia, cuando la hay, entre las horas sugeridas de un docente y las **horas en aula**
  que el usuario le declaró. Se informa; **no** impide guardar la configuración ni generar (005/018).
- **Tope sembrado de 36 horas académicas:** valor que el conjunto del centro declara para las horas en
  aula de todos los docentes (017, D-3). La 019 **no cambia ese valor** y **no lo impone como inicio de
  la tabla** (las dos columnas arrancan con las horas sugeridas): es la referencia del centro y el valor
  que el usuario puede bajar o subir por convenio. **No** es un máximo que el sistema valide.
- **Materia suplente:** materia que un docente puede impartir **en sustitución** del titular. Es un
  dato distinto de las materias titulares y se guarda como una relación entre docente y materia.
- **Aforo simultáneo:** número máximo de **cursos distintos** que pueden tener clase en un aula de un
  tipo en la misma franja; **cancha 2**, resto **1**, como regla del sistema (018).
- **Par completo:** dos cursos que comparten la cancha en **exactamente las mismas dos franjas** de su
  bloque de dos horas. **Par cruzado:** comparten una sola franja; no es una solución (018).
- **Preferencia horaria:** inclinación del docente sobre una franja; **preferida** o **evitada**, con
  la identidad y la validación de la 012. No es disponibilidad.
- **Esquema versionado:** la marca de versión de la base junto con el catálogo de estructura esperada y
  su huella congelada, que se mueven **juntos** (009/015/016).
- **Sin migración:** una base cuya marca o catálogo no coincide con el esperado **no se abre**, no se
  repara y no se convierte; la salida es respaldar y crear una base nueva (001/009/015/016).

## Requisitos funcionales

### RF-1. Un docente sembrado se puede abrir y guardar aunque su identificador no sea numérico

- **CUANDO se abra el formulario de un docente ya registrado cuyo identificador no está formado
  únicamente por dígitos, EL SISTEMA lo mostrará tal como está almacenado** y permitirá corregir el
  nombre, el correo, el teléfono y sus preferencias de generación.
- **CUANDO se guarde ese docente sin modificar su identificador, EL SISTEMA aceptará el guardado sin
  exigir que el identificador sea numérico** y sin volver a comprobar que el identificador ya existe.
- **CUANDO se registre un docente nuevo o se modifique el identificador de uno existente, EL SISTEMA
  exigirá que esté formado únicamente por dígitos** y lo rechazará explicando la regla si no lo está.
- **SI el identificador propuesto ya pertenece a otro docente, ENTONCES EL SISTEMA rechazará el
  guardado** indicando a qué docente pertenece y conservará el valor anterior.
- **SI el identificador queda vacío, ENTONCES EL SISTEMA rechazará el guardado.**
- **EL SISTEMA no alterará el identificador almacenado** por el simple hecho de abrir y guardar: la
  identidad del docente, su disponibilidad, sus preferencias y sus materias se conservan.
- **Criterio de aceptación:** con el conjunto del centro aplicado, abrir el docente identificado como
  `P033`, cambiar su nombre y guardar: el guardado se acepta, la lista muestra el nombre nuevo, el
  identificador sigue siendo `P033` y su ficha (disponibilidad, preferencias y materias) no pierde
  filas.

> **Alcance de este requisito.** Cierra **solo** la parte de **H10** relativa a la comprobación de la
> cédula. Las pruebas de las **pantallas de planes** y el **preset maestro del centro** siguen en deuda
> (`estado.md`).

### RF-2. El horario no se guarda por su cuenta y «datos anteriores» exige confirmación

- **CUANDO el cálculo termine con un horario válido y los datos no hayan cambiado, EL SISTEMA
  presentará el horario y no escribirá ningún archivo** hasta que el usuario lo solicite.
- **EL SISTEMA ofrecerá una acción explícita de guardado mientras haya un horario presentado** y solo
  entonces lo escribirá como archivo; mientras no se pida, el horario permanece disponible en pantalla.
- **SI el resultado llega marcado como de datos anteriores, ENTONCES EL SISTEMA no escribirá ningún
  archivo, no lo anunciará como guardado e informará de que los datos cambiaron durante el cálculo**,
  pidiendo confirmación explícita para conservar el horario.
- **CUANDO el usuario confirme conservar un horario de datos anteriores, EL SISTEMA lo presentará
  identificado como correspondiente a datos anteriores** y seguirá sin escribirlo hasta que el usuario
  pida guardarlo.
- **SI el usuario no confirma conservar el horario de datos anteriores, ENTONCES EL SISTEMA lo
  descartará**, informará de que no se conservó y ofrecerá volver a generar.
- **SI la escritura del archivo falla, ENTONCES EL SISTEMA informará la causa y permitirá reintentar
  sin retirar el horario de pantalla** (002 RF-4).
- **EL SISTEMA no ofrecerá guardar** cuando el resultado no sea presentable: sin solución factible,
  respuesta inválida, conflictos de solapamiento o plazo agotado (002 RF-1/RF-3).
- **Criterio de aceptación:** con un resultado marcado como de datos anteriores, al terminar la
  generación **no existe ningún archivo nuevo** y el estado no anuncia «guardado»; tras confirmar y
  pedir el guardado, existe **exactamente un** archivo; con un resultado válido sin cambios, el
  archivo aparece **solo** cuando se usa la acción de guardado.

### RF-3. El fallo del cálculo muestra su causa real

- **CUANDO el cálculo informe que no hay solución factible, EL SISTEMA mostrará el diagnóstico que
  acompañó al fallo** —la causa del catálogo (1)–(5) con sus cifras o la declaración de «no
  atribuible»— **en lugar de un mensaje genérico**.
- **EL SISTEMA conservará el diagnóstico completo**, incluidas sus líneas de contexto (entorno del
  fallo), sin resumirlo ni sustituirlo por otro texto.
- **SI el fallo no trae diagnóstico, ENTONCES EL SISTEMA informará que no hay solución y que no hay una
  causa atribuible**, sin inventar una categoría nueva.
- **EL SISTEMA no ofrecerá guardar el horario** ni cambiará su estado por mostrar la causa.
- **EL SISTEMA mantendrá el texto de la causa tal como lo produce la generación**, de modo que las
  cifras citadas (demanda, aulas, franjas, aforo, turno, disponibilidad) coincidan con el diagnóstico
  del motor.
- **Criterio de aceptación:** con un fallo que trae la causa `(2) conflicto de aula … demanda … aulas …
  franjas … aforo …`, el mensaje mostrado al usuario **contiene** ese texto y no se reduce a «no existe
  una solución factible»; con un fallo sin causa, el mensaje indica expresamente que no es atribuible.

### RF-4. Las horas propuestas salen de las horas reales y el exceso se ve, sin tope fijo

- **EL SISTEMA propondrá para cada docente sus horas sugeridas** —la suma de las horas de las materias
  que imparte en los cursos configurados— **en lugar de un valor derivado del número de materias**.
- **EL SISTEMA aplicará la misma propuesta a las dos columnas de horas de la fila** —que **arrancan con
  las horas sugeridas**— y actualizará ambas cuando el usuario pida recalcular.
- **EL SISTEMA permitirá ajustar las dos columnas —horas requeridas y horas en aula— para reflejar el
  convenio del centro**, y **medirá el exceso siempre frente a las horas en aula declaradas**, nunca
  frente a las horas requeridas.
- **SI las horas sugeridas de un docente superan las horas en aula que el usuario le declaró, ENTONCES
  EL SISTEMA mostrará el exceso de forma visible en su fila**, indicando las horas sugeridas, las horas
  en aula y la diferencia.
- **SI el usuario declara horas en aula por encima de las horas requeridas, ENTONCES EL SISTEMA
  mantendrá la coherencia del dato de la 005**: subirá las horas requeridas al menos hasta las horas en
  aula (o pedirá al usuario que las ajuste) y lo señalará en la fila; **no** guardará ni compondrá una
  configuración con horas en aula por encima de las requeridas.
- **EL SISTEMA no impondrá el tope sembrado del centro (36 horas académicas) como valor inicial de la
  tabla**: las dos columnas arrancan con las horas sugeridas; las 36 h pueden mostrarse como referencia
  del centro y las horas en aula se editan por encima o por debajo.
- **EL SISTEMA no tratará las 36 horas académicas como máximo**: no rechazará ni bloqueará valores
  distintos, dentro del rango que admita el control de la columna, **sin impedir guardar la
  configuración ni generar**.
- **EL SISTEMA no recortará el valor propuesto ni impedirá generar por el exceso**: el exceso se informa
  y el motor lo penaliza, no lo convierte en infactibilidad (005/018); la coherencia horas en aula ≤
  horas requeridas **sí** se mantiene.
- **MIENTRAS el usuario no pida recalcular, EL SISTEMA conservará los valores de horas que haya
  ajustado a mano**, tanto en las horas requeridas como en las horas en aula.
- **SI un docente no imparte ninguna materia con horas en los cursos configurados, ENTONCES EL SISTEMA
  mantendrá el valor mínimo admitido** en sus horas y no mostrará exceso.
- **EL SISTEMA contará las horas por curso**: un docente que imparte la misma materia en dos cursos
  suma las horas de ambos, porque su carga es por curso.
- **EL SISTEMA no contará las materias sin carga horaria** en el plan.
- **Criterio de aceptación:** con el conjunto del centro, las horas propuestas de cada docente igualan
  la suma de las horas de sus materias en los cursos configurados (no `2 × nº de materias`); si el
  usuario baja las horas en aula por debajo de esa suma, la fila muestra el exceso con las tres cifras y
  la generación sigue disponible; con un convenio de 40 horas en aula sobre una carga de 36, el sistema
  deja las horas requeridas en 40 (o pide ajustarlas) y **no queda ninguna combinación con horas en
  aula por encima de las requeridas**; en ningún caso se rechaza por superar las 36 horas de referencia
  sembrada.

### RF-5. Las materias suplentes forman parte del esquema versionado

- **EL SISTEMA incluirá las materias suplentes en el esquema de la base**, como relación entre un
  docente y una materia, con identidad propia por el par y referencias a ambos registros.
- **EL SISTEMA creará esa relación con la base y no bajo demanda**: ninguna operación creará estructura
  al abrirse ni al usarse.
- **EL SISTEMA describirá la relación en el catálogo de esquema esperado y en la huella congelada del
  esquema**, junto con el incremento de la marca de versión a **7**, todo en el mismo cambio.
- **EL SISTEMA seguirá exigiendo que una base tenga el catálogo completo**: la ausencia de la relación
  es base inválida y se informa con la causa y la acción.
- **EL SISTEMA admitirá varias materias suplentes por docente y rechazará que una misma materia sea a
  la vez titular y suplente del mismo docente.**
- **EL SISTEMA aplicará ese rechazo en el servicio de docentes y de materias, no solo en la ficha**: la
  comprobación vale para cualquier camino que registre la suplencia (ficha, asignación directa o
  importación) y no deja pasar una contradicción.
- **EL SISTEMA dejará la relación sin filas al sembrar el conjunto del centro**, porque ese conjunto
  **no declara suplentes** (017, RF-6): «datos del centro intactos» incluye que la relación quede
  **vacía**.
- **EL SISTEMA no cambiará el contrato de configuración, los presets ni la huella de datos** por este
  cambio: el suplente sigue llegando al cálculo como hasta hoy.
- **Criterio de aceptación:** una base recién creada contiene la relación y, tras sembrar el conjunto del
  centro, queda **vacía** (el conjunto no declara suplentes); una base a la que se le quita la relación
  deja de ser válida y no se abre; una asignación que haría a una materia titular y suplente del mismo
  docente se rechaza desde el servicio; la marca de versión es 7 y la huella congelada del esquema
  corresponde a la nueva estructura.

### RF-6. Borrar una materia referenciada se bloquea con su causa; el docente arrastra sus suplencias

- **SI se intenta eliminar una materia que un docente tiene como suplente, ENTONCES EL SISTEMA
  rechazará el borrado e indicará que está asignada como materia suplente y a qué docente**, sin
  borrar nada.
- **SI se intenta eliminar una materia que pertenece a un plan de estudio o que un docente tiene como
  titular, ENTONCES EL SISTEMA rechazará el borrado con la causa correspondiente**, distinta de la de
  suplente, para que el usuario sepa qué deshacer primero.
- **CUANDO se elimine un docente, EL SISTEMA borrará sus materias suplentes junto con sus demás datos
  dependientes en la misma operación**, sin dejar filas huérfanas.
- **CUANDO se consulte qué depende de un docente antes de borrarlo, EL SISTEMA listará también sus
  materias suplentes.**
- **CUANDO se elimine una materia suplente de un docente y ya no quede ninguna referencia a esa
  materia, EL SISTEMA permitirá eliminarla.**
- **Criterio de aceptación:** con una materia asignada como suplente a un docente, el borrado de la
  materia se rechaza con un mensaje que nombra la causa y el docente, y la materia sigue existiendo; tras
  quitar la suplencia, el borrado se acepta; al borrar el docente, no queda ninguna fila de suplencia
  suya y **la lista de dependencias que se muestra antes de confirmar incluye sus materias suplentes**.

> **Alcance de este requisito.** Cierra **solo** la parte de **H14** relativa a **distinguir las causas**
> al borrar una materia (plan, titular o suplente). La **cascada de aulas y materias** y el resto de H14
> siguen en deuda (`estado.md`).

### RF-7. Una base del esquema anterior deja de abrir y la salida es una base nueva

- **SI se abre una base con la marca de versión 6 (o anterior) o con el catálogo anterior, ENTONCES la
  apertura fallará sin migrar, sin reparar y sin escribir** en la base, informando la causa y la acción.
- **EL SISTEMA ofrecerá crear una base nueva con respaldo obligatorio** y advertirá de que **restaurar
  un respaldo de la versión anterior volverá a fallar**, porque no hay conversión entre esquemas.
- **EL SISTEMA no convertirá, copiará ni reparará** datos de una base anterior; tampoco conservará la
  relación de materias suplentes que la versión anterior creaba al vuelo.
- **EL SISTEMA permitirá repoblar una base nueva con el conjunto del centro** y dejará documentado que
  esa es la vía de recuperación de los datos del centro.
- **EL SISTEMA documentará la consecuencia en las guías de datos y en la documentación de la interfaz**,
  de modo que ninguna prometa que una base anterior se abre o se migra.
- **Criterio de aceptación:** abrir una base de versión 6 produce un fallo de apertura con mensaje que
  nombra la versión y la acción; el mensaje advierte de que restaurar un respaldo anterior volverá a
  fallar; una base nueva creada con esta versión se siembra y genera como hoy.

### RF-8. El documento de visualización declara el aforo de cada aula

- **EL SISTEMA incluirá, en el documento que alimenta la visualización del horario, el aforo simultáneo
  de cada aula** según la regla del sistema (cancha 2, resto 1), junto con su nombre.
- **EL SISTEMA tomará el aforo de la misma regla que usa la generación** y no de la capacidad física del
  aula, del número de estudiantes del curso ni del nombre del aula.
- **EL SISTEMA no pedirá el aforo al usuario, no lo incluirá en el contrato de configuración y no lo
  guardará en los presets.**
- **SI el documento no declara el aforo de un aula, ENTONCES EL SISTEMA asumirá aforo 1** para esa aula
  (horarios guardados antes de este cambio), sin abrir ningún hueco.
- **Criterio de aceptación:** en el documento de un horario generado del centro, la cancha declara
  aforo 2 y cada aula general declara aforo 1; el contrato 2.0 y los presets no incorporan ningún
  campo nuevo.

### RF-9. Al mover clases, el aforo decide si hay conflicto

- **CUANDO se mueva una clase a una franja en la que el aula destino ya tiene clase, EL SISTEMA solo
  lo marcará como conflicto de aula por ocupación si la ocupación de esa aula en esa franja supera su
  aforo**, **sin perjuicio del par cruzado y del mismo docente**, que se marcan siempre.
- **EL SISTEMA no marcará conflicto cuando dos cursos distintos, con docentes distintos, compartan la
  cancha en la misma franja dentro de su aforo y con el bloque completo** —exactamente las mismas dos
  franjas—; una sola franja compartida no está exenta (par cruzado).
- **SI el aula tiene aforo 1 y ya tiene una clase en esa franja, ENTONCES EL SISTEMA lo marcará como
  conflicto** nombrando aula, día y franja.
- **SI la cancha recibiera una tercera clase en la misma franja, ENTONCES EL SISTEMA lo marcará como
  conflicto** nombrando aula, día, franja, número de clases y aforo.
- **SI dos clases de la cancha comparten solo una de las dos franjas de su bloque, ENTONCES EL SISTEMA
  lo marcará como conflicto**, aunque no se supere el aforo.
- **EL SISTEMA seguirá marcando conflicto cuando el mismo docente esté en dos clases a la vez**, con
  independencia del aula y del aforo.
- **EL SISTEMA no contará como ocupación una clase sin aula declarada**: no ocupa franja de aula, no
  crea un conflicto de aforo ni lo alivia (018, RF-7).
- **EL SISTEMA aplicará aforo 1 a toda aula cuyo aforo no conozca**, incluida un aula que no exista en
  el documento.
- **Criterio de aceptación:** mover la Educación Física de una sección sobre la de otra sección del
  mismo bloque (misma cancha, docentes distintos) no produce conflicto; sobre una tercera clase en la
  cancha sí; sobre un aula general ya ocupada sí; mover solo una franja del bloque sobre otra clase de
  la cancha sí; mover una clase sin aula declarada no produce conflicto de aula; mover sobre un aula que
  no existe en el documento se comporta como aforo 1.

### RF-10. La rejilla de preferencias expresa los dos sentidos y los conserva

- **CUANDO el usuario pulse una celda de la rejilla de preferencias de un docente, EL SISTEMA
  alternará, en este orden, sin preferencia → preferida → evitada → sin preferencia.**
- **EL SISTEMA persistirá el sentido elegido** y, al volver a abrir la ficha del docente, mostrará los
  tres estados tal como se guardaron.
- **SI la celda corresponde a una franja en la que el docente no está disponible, ENTONCES EL SISTEMA
  no admitirá ni preferida ni evitada** y explicará que la hora está bloqueada. Es **política de la
  pantalla**, no una validación de la 012: la preferencia es una inclinación y la 012 no la cruza con la
  disponibilidad.
- **EL SISTEMA distinguirá visualmente los tres estados** y lo explicará en la propia pantalla, con una
  leyenda de los tres estados.
- **SI al guardar una preferencia se rechaza por contradicción con otra del sentido opuesto, ENTONCES EL
  SISTEMA mostrará la causa y conservará en pantalla el estado anterior**, sin perder los demás cambios
  de la ficha. El guardado sigue siendo **atómico** (012): no se persisten unos cambios sí y otros no.
- **EL SISTEMA no creará ni modificará la disponibilidad del docente al cambiar una preferencia.**
- **EL SISTEMA mantendrá la validación de la 012**: solapes de sentidos opuestos y duplicado exacto se
  rechazan; solapes y rangos contiguos del mismo sentido se admiten.
- **Criterio de aceptación:** pulsar una celda una vez y guardar deja una preferencia **preferida**;
  pulsarla dos veces y guardar deja una **evitada**; pulsarla tres veces y guardar **elimina** la
  preferencia; al reabrir el docente se ven los tres estados con su marca y la leyenda los explica;
  una celda no disponible no cambia de estado.

## Requisitos no funcionales

- **RNF-1. Tests primero y suites verdes.** Cada corrección se acompaña de su comprobación escrita
  antes del código. Ninguna tarea cierra con el árbol sin compilar ni con pruebas en rojo, incluidas
  las lentas cuando se toquen. «Línea base» significa que **todo lo que hoy pasa sigue pasando** y la
  suite normal no se degrada de forma apreciable; los recuentos de pruebas **cambiarán** con las nuevas
  y no son un requisito.
- **RNF-2. Contrato, presets y aforo intactos.** El contrato de configuración sigue en **2.0**, los
  presets vigentes se aceptan sin cambios, la huella de datos no cambia por estos arreglos y el aforo
  de la visualización **no** viaja en el contrato, no entra en los presets ni en la huella.
- **RNF-3. Datos del centro intactos.** Los 2 planes, 85 materias, 24 cursos con turno, 13 aulas,
  33 docentes y 876 clases no cambian; ninguna verificación de esta spec modifica los datos sembrados.
  El conjunto del centro **no declara suplentes** (017, RF-6), así que la relación de materias suplentes
  queda **vacía** tras sembrar; eso es el estado esperado, no un cambio de datos.
- **RNF-4. Esquema v7 inseparable.** La marca de versión, el catálogo de estructura esperada y la huella
  congelada se mueven en el mismo cambio; no puede quedar una base con la marca nueva y el catálogo
  viejo. Sin migración.
- **RNF-5. Mensajes en español con causa y acción.** Todo rechazo, aviso o causa identifica el dato
  implicado y, cuando existe, la acción disponible; nunca se muestra texto crudo del almacenamiento.
- **RNF-6. Trazabilidad.** Cada RF tiene al menos una comprobación verificable en las tareas y la spec
  debe caber en **10 tareas como máximo**.
- **RNF-7. Determinismo de lo que se informa.** Las horas sugeridas, el exceso, el aforo declarado en el
  documento y los diagnósticos de fallo son deterministas para los mismos datos y no dependen del reloj;
  el horario generado puede variar entre ejecuciones.
- **RNF-8. La regla vive una sola vez.** El aforo, la validez del esquema y el bloqueo de borrado se
  resuelven en la lógica y se prueban sin interfaz; la pantalla solo presenta el resultado y **no
  recalcula ni duplica** ninguna de esas reglas.
- **RNF-9. Sin dependencias ni pantallas nuevas.** No se añaden bibliotecas ni **pantallas nuevas**; sí
  se admiten los elementos que exigen los RF: la **acción explícita de guardado**, la **confirmación de
  datos anteriores**, el **indicio de exceso de horas** en la fila del docente y la **leyenda de los tres
  estados** de la rejilla. No se rediseñan las pantallas existentes más allá de eso.

## Casos límite

- **Docente sembrado con identificador no numérico al que solo se le cambia el nombre:** se guarda; el
  identificador no se revalida ni se reescribe (RF-1).
- **Docente sembrado al que se le intenta cambiar el identificador por uno alfanumérico:** se rechaza
  con la regla de solo dígitos y se conserva el anterior (RF-1).
- **Docente sembrado al que se le corrige el identificador a uno numérico libre:** se acepta y se
  revalida la unicidad (RF-1).
- **Alta nueva con identificador alfanumérico:** se rechaza con la regla (RF-1).
- **Identificador de otro docente:** se rechaza nombrando al docente que ya lo tiene (RF-1).
- **Guardar sin cambios en la ficha:** se acepta; disponibilidad, preferencias y materias se conservan
  (RF-1).
- **Horario válido sin cambios de datos:** no se escribe nada hasta que el usuario lo pida; si cierra la
  vista sin pedirlo, no queda archivo (RF-2).
- **Datos anteriores y el usuario rechaza conservarlo:** no queda archivo, no se presenta como vigente y
  se ofrece regenerar (RF-2).
- **Datos anteriores y el usuario confirma:** se presenta marcado como de datos anteriores y sigue sin
  archivo hasta que lo pida (RF-2).
- **Fallo al escribir el archivo:** se informa la causa, se permite reintentar y el horario no se
  retira (RF-2).
- **Resultado no presentable:** no se ofrece guardar en ningún caso (RF-2).
- **No factible con causa (1)–(5):** el mensaje la incluye con sus cifras (RF-3).
- **No factible sin causa atribuible:** el mensaje lo dice expresamente y no inventa categoría (RF-3).
- **Docente con materias sin carga horaria:** no suman a las horas sugeridas (RF-4).
- **Docente que imparte la misma materia en dos cursos:** suma las horas de ambos (RF-4).
- **Docente por encima de sus horas en aula:** se muestra el exceso con las horas propuestas, las horas
  en aula y la diferencia; la generación sigue disponible (RF-4).
- **Convenio por encima de la referencia del centro (40 h en aula con 36 h de carga):** las horas requeridas
  suben a 40 (o el sistema pide ajustarlas) para no incumplir la coherencia de la 005; no hay bloqueo ni
  rechazo por superar las 36 h (RF-4).
- **Horas en aula por encima de las horas requeridas:** el sistema no guarda ni compone esa combinación:
  sube las requeridas o pide al usuario que las ajuste (RF-4).
- **Convenio por debajo de las horas sugeridas (24 h en aula para 30 h de materias):** se muestra el
  exceso de 6 horas y no se recorta nada (RF-4).
- **Horas en aula iguales a las sugeridas:** no hay exceso y no se muestra aviso (RF-4).
- **Docente sin materias con horas:** conserva el valor mínimo y no muestra exceso (RF-4).
- **Ajuste manual de horas seguido de recálculo:** el recálculo sustituye los valores; sin recálculo, se
  respeta lo ajustado (RF-4).
- **Materia asignada como suplente a dos docentes:** el borrado de la materia se bloquea nombrando la
  causa; se puede quitar la suplencia de cada docente (RF-6).
- **Materia que es titular de un docente y suplente de otro:** el borrado se bloquea por ambas causas,
  con mensajes distinguibles (RF-6).
- **Borrado de un docente con suplencias:** se listan antes de confirmar y se borran con él (RF-6).
- **Base de versión 6:** fallo de apertura, sin migración; la acción es crear base nueva con respaldo
  (RF-7).
- **Respaldo de versión 6 restaurado sobre la versión 7:** vuelve a fallar (RF-7).
- **Base de versión 7 a la que le falta la relación de suplentes:** base inválida; no se abre (RF-5).
- **Horario guardado antes de este cambio (sin aforo):** dos clases en la cancha se señalan como
  conflicto; se puede regenerar el horario (RF-8, P-3).
- **Cancha con tres clases en una franja al mover:** conflicto con número de clases y aforo (RF-9).
- **Cancha con dos clases de cursos distintos y docentes distintos en el mismo bloque:** no es conflicto
  (RF-9).
- **Cancha con dos clases que comparten una sola franja:** conflicto (RF-9, P-4).
- **Aula general (aforo 1) con dos clases:** conflicto (RF-9).
- **Clase movida sobre una franja de receso:** sigue sin poder ocuparse; el receso no es una franja de
  clase (RF-9).
- **Mismo docente en dos clases a la vez:** conflicto, aunque el aula admita dos (RF-9).
- **Misma materia y mismo docente movidos a la misma aula y franja:** conflicto por docente (RF-9).
- **Clase sin aula declarada:** no ocupa franja de aula; moverla no crea ni alivia un conflicto de aforo
  (RF-9).
- **Aula que no existe en el documento:** se trata con aforo 1 y dos clases en ella son conflicto (RF-9).
- **Celda de preferencia en receso:** no admite ningún estado (RF-10).
- **Celda de preferencia no disponible:** no admite preferida ni evitada; es política de la pantalla y no
  una validación de la 012 (RF-10).
- **Preferencia evitada guardada y luego abierta:** se muestra como evitada y un clic la devuelve a «sin
  preferencia»; otro clic la vuelve a marcar como preferida (RF-10).
- **Preferencia evitada contigua a una preferida el mismo día:** se admiten (no se solapan), según la
  012 (RF-10).
- **Rechazo por solape de sentidos opuestos:** se muestra la causa y se conserva en pantalla lo anterior;
  el guardado es atómico y no deja unos cambios sí y otros no (RF-10).
- **Cambio de preferencia en una hora que se vuelve no disponible:** la preferencia de esa celda se
  descarta, como hoy (RF-10).

## Fuera de alcance

- **Del resto de los nueve hallazgos —H7 y H9–H16— no se implementa nada**, salvo las **dos partes ya
  declaradas de H10** (comprobación de la cédula, RF-1) y **H14** (distinción de causas al borrar
  materias, RF-6). Se documentan con su referencia en `estado.md` (D-2, D-10). En particular: la
  superficie del cumplimiento y del informe de bloques (H7), la cancelación del cálculo y el plazo de
  300 s (H9), el texto obsoleto del turno (H11), la estimación de ocupación 1:1 materia/aula (H12), los
  eventos en la disponibilidad (H13) y el plan de un solo año leído como global (H15).
- **De H10 se cierra solo la comprobación de la cédula** (RF-1). Las **pruebas de las pantallas de
  planes** y el **preset maestro del centro** siguen en deuda.
- **De H14 se cierra solo la distinción de causas al borrar una materia** (RF-6). La **cascada de aulas
  y materias** sigue en deuda.
- **El aviso al cerrar con un horario sin guardar (002 RF-5).** No estaba entre los seis hallazgos y no se
  implementa aquí; al quitar el guardado automático (RF-2) ese aviso pasa a ser relevante y queda como
  **arrastre declarado** en `estado.md`.
- **La persistencia en disco de las ediciones de la visualización (H16)** y el resto de la deuda de
  proceso de las pantallas: **CRUD de planes, menú sin cuenta y ventana/leyenda**.
- **Cambiar el modelo del solver, el contrato 2.0, los presets o la huella de datos.**
- **Aplicar el aforo a otros tipos de aula, hacerlo configurable o cambiar la capacidad física.**
- **Imponer un máximo legal de horas de aula o validar los convenios** contra las 36 horas académicas:
  ese valor es la **referencia sembrada** del centro, no un máximo ni el inicio de la tabla (P-5); el
  exceso solo se informa.
- **Introducir migraciones de esquema, conversiones de datos o compatibilidad con bases anteriores.**
- **Rediseñar pantallas, rejillas o ventanas**, más allá de los elementos que piden los RF (acción de
  guardado, confirmación de datos anteriores, indicio de exceso y leyenda de los tres estados).
- **El rendimiento y la descomposición del cálculo** (spec propia).
- **Los datos del centro:** planes, materias, cursos, aulas, docentes y clases no se tocan.

## Criterios de finalización

1. Un docente sembrado con identificador `P033` se abre, se edita y se guarda; la lista y su ficha
   conservan la identidad y los datos asociados (RF-1).
2. Un identificador nuevo o modificado se sigue exigiendo numérico y único; uno vacío se rechaza
   (RF-1).
3. Solicitar la generación no escribe ningún archivo: el horario se guarda solo cuando el usuario usa la
   acción de guardado (RF-2).
4. Un resultado marcado como de datos anteriores no se escribe, no se anuncia como guardado y exige
   confirmación; si el usuario no confirma, no se conserva (RF-2).
5. Un **resultado no factible** muestra la causa del catálogo (1)–(5) o «no atribuible», con sus cifras
   y sin mensaje genérico que la tape (RF-3).
6. Las horas propuestas de cada docente son la suma de las horas de sus materias en los cursos
   configurados y las dos columnas arrancan con ellas; las horas en aula se pueden ajustar por convenio
   por encima o por debajo, tomando las 36 h del centro como referencia sembrada y no como máximo ni
   como inicio impuesto, y sin incumplir la coherencia de la 005 (horas en aula ≤ horas requeridas); el
   exceso de las horas sugeridas frente a lo declarado se muestra con sus tres cifras y no bloquea la
   generación (RF-4).
7. Las materias suplentes están en el esquema, el catálogo y la huella congelada de la **versión 7**, y
   una base sin esa relación es inválida (RF-5).
8. El borrado de una materia referenciada se bloquea con la causa (plan, titular o suplente); el borrado
   del docente arrastra sus suplencias y **la lista de dependencias previa incluye esas suplencias**
   (RF-6).
9. Una base de la versión 6 no se abre, no se migra y la acción ofrecida es crear una base nueva con
   respaldo; el mensaje advierte de que restaurar el respaldo anterior volverá a fallar (RF-7).
10. El documento de visualización declara el aforo por aula (cancha 2, resto 1) sin tocar el contrato ni
    los presets; sin dato, se asume aforo 1 (RF-8).
11. Al mover clases, dos cursos distintos con docentes distintos en la cancha no son conflicto; tres
    clases, una sola franja compartida o un aula de aforo 1 sí; el mismo docente siempre; una clase sin
    aula no ocupa y un aula desconocida se trata con aforo 1 (RF-9).
12. La rejilla de preferencias alterna los tres estados, distingue y explica cada uno y persiste el
    sentido elegido; el rechazo por contradicción conserva en pantalla lo anterior y el guardado es
    atómico (RF-10).
13. Todas las suites que hoy pasan siguen pasando, incluida la lenta cuando se toque, y cada RF tiene al
    menos una comprobación verificable (RNF-1, RNF-6).
14. La spec cabe en **10 tareas como máximo** (RNF-6).

## Dudas abiertas

**Ninguna.** La única duda (Q1, la referencia del exceso de horas de RF-4) se cerró el **2026-11-10**
con la decisión **P-5**: la referencia son las horas en aula declaradas por fila, sin tope legal fijo; las
**36 h** son el **valor sembrado de referencia** del centro (017), no el inicio de la tabla ni un máximo,
y se editan por convenio manteniendo la coherencia de la 005 (horas en aula ≤ horas requeridas).
