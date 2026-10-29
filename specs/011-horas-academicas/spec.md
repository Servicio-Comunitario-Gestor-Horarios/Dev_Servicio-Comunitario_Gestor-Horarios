# Spec 011 - Duración de la hora académica

Estado: implementada (2026-10-29)

> **Alcance y reparto.** Esta spec cubre la **persistencia y el backend** de la duración de la hora
> académica: la configuración global, su validación centralizada, los servicios, la franja de clase, el
> encaje del turno, la alimentación del contrato, la huella, el esquema y su versión, el seed y la
> documentación de la API. **No** cambia el modelo del solver, ni el tope de 40 horas, ni la interfaz.
> Depende de la **009** (esquema, versión, catálogo, servicios, huella y guía) y de la **010** (contrato
> 2.0), y se implementa **después** de la 010 para no tocar dos veces la construcción de la entrada.
>
> **Decisión del usuario (2026-10-23).** La **configuración global manda** sobre la duración de la hora
> académica; el **turno es un dato externo del plantel y se valida**. Valor por defecto **35 minutos**
> (hora académica de Educación Básica según la Gaceta 42.505; antes 45), rango **30–90** para admitir el
> cambio normativo y variantes regionales. La **franja de clase se deriva de la duración de la hora
> académica**, no de repartir la ventana del turno. Invariante de encaje:
> `nº franjas × duración + tiempo de eventos ≤ ventana del turno`; si no se cumple, se rechaza indicando
> turno, duración y déficit. El **número de franjas utilizable** de un turno se calcula con la ventana,
> los eventos y la duración. Los **bloques de clase distintos de la hora académica** (80 minutos en
> Educación Media) quedan como **duda diferida y fuera de alcance**: el sistema asume hoy
> 1 hora académica = 1 bloque = 1 franja, y esa distinción llegará con el conjunto de datos real.
>
> **Supersesiones que declara esta spec.** Quedan superados por la 011:
> - De la **009**: la definición de «duración estimada de la franja de clase» y el cálculo
>   `(ventana del turno − eventos) / nº de franjas` (RF-3 y RF-11); la franja de clase pasa a durar la
>   hora académica y el sobrante de la ventana no se reparte.
> - De la **008**: el inventario de turnos de 8 franjas de 40 minutos y la rejilla
>   `8 × 40 + 10 = 330` (RF-4 y criterios 1 y 3); los turnos se revalidan con la duración de la config.
> - De la **005** y la **010**: la validación V2 de `duracion_minutos` solo exige `> 0`; la 011 la
>   **estrecha a 30–90** para todo contrato que llegue sin pasar por la construcción.
>
> **Frontera con el frontend.** La interfaz queda **fuera de alcance** (spec 001). La vista que elige la
> duración por generación y hace aritmética en minutos, el `horasAula` del frontend y el **selector de
> duración del editor de presets y de la vista de generación** quedan como **deuda documentada** de su
> equipo: dejan de ofrecerse, pero no se exige su cambio en esta spec. Esta spec exige el servicio
> centralizado y la guía de API.
>
> **Duración del preset (decisión del 2026-10-23).** La duración **deja de ser un dato del usuario en el
> preset**: la construcción de la entrada **sobrescribe siempre** el valor que traiga el preset con el
> de la configuración global. El preset puede seguir conteniendo el campo porque comparte formato con el
> contrato y el solver no lee la base, pero **su valor nunca se usa**. Si la configuración cambió desde
> que se guardó el preset, la generación usa el valor nuevo; no es un error ni requiere aviso.
>
> **Lo que no cambia.** El tope de **40 horas académicas semanales por año** se mantiene; la identidad
> **1 hora académica = 1 bloque** del solver se mantiene. Si hubiera que tocar el tope, sería otro
> requisito.
>
> **Estado de entrega y verificación.** Dentro: **backend, datos, seed, consola y sus pruebas**, que
> compilan y pasan **sin la interfaz**. Fuera: la interfaz y los tests ligados a ella. La construcción
> debe poder excluir la interfaz (requisito de la 009, RNF-6). Las pruebas del constructor de la entrada
> que hoy fijan **45** por coincidencia con el default anterior y las de generación con el seed que
> afirman **40** deben **reescribirse** al nuevo default **35** y a la invariante de encaje, no
> ajustarse a ciegas.
>
> **Sin migración automática.** El esquema cambia (configuración persistida) y se aplica la política de
> la 009: una base con esquema no esperado se rechaza al abrir, sin migrar ni borrar.

## Contexto y objetivo

El solver ya trata **una hora académica como un bloque**: la conversión es la identidad. Pero la
duración real de esa hora se elige hoy por generación en la interfaz (45 minutos por defecto), la spec
008 modela franjas de 40 minutos y la vista hace aritmética en minutos reales; conviven así tres
duraciones distintas. La norma vigente (Gaceta 42.505) fija **35 minutos** para Educación Básica y
organiza Educación Media en **bloques de 80 minutos**, pero el modelo actual no distingue hora
académica de bloque de clase.

El objetivo es **estandarizar la hora académica**: una configuración global única por base, de 35
minutos por defecto y rango 30–90, que fija la duración de la franja de clase; los turnos son datos
externos del plantel y se validan contra esa duración. Todo el backend habla en horas académicas; los
minutos son una representación derivada de un único punto. El contrato del solver sigue llevando la
duración real porque no consulta la base, pero la toma de la configuración, no del preset.

**Qué no es.** No es un cambio del modelo del solver, ni del tope de 40 horas, ni de la interfaz, ni
una migración, ni la introducción de bloques de clase distintos de la hora académica.

## Usuarios

- **Usuario académico:** configura la duración de la hora académica de su centro y espera que todas las
  generaciones usen ese valor y que los turnos encajen.
- **Equipo del solver:** necesita la duración efectiva en el contrato y que el turno validado tenga
  franjas suficientes.
- **Equipo de frontend:** necesita la API de configuración y el valor efectivo; su aritmética en
  minutos es deuda.
- **Equipo de pruebas y QA:** necesita una base con la configuración por defecto y turnos válidos.
- **Equipo de desarrollo:** necesita una sola definición de hora académica y un solo punto de
  conversión a minutos.

## Historias de usuario

- HU-1. Como usuario académico, quiero fijar la duración de la hora académica una vez, para que todas
  las generaciones y turnos la usen.
- HU-2. Como usuario académico, quiero que el sistema rechace una duración fuera del rango y un turno
  que no encaje, para no generar horarios imposibles.
- HU-3. Como miembro del equipo del solver, quiero que el contrato lleve la duración efectiva y que un
  turno validado tenga franjas suficientes, para no depender del preset.
- HU-4. Como miembro del equipo de frontend, quiero una API documentada con el valor efectivo y los
  minutos derivados, para no repetir conversiones.
- HU-5. Como miembro del equipo de pruebas, quiero una base con 35 minutos y turnos válidos, para probar
  sin configurar nada.

## Definiciones

- **Hora académica:** unidad pedagógica de carga; **una hora académica = un bloque = una franja de
  clase** en esta versión (spec 005). No es una hora de reloj.
- **Duración de la hora académica (D):** minutos reales de una hora académica y, por tanto, de una
  franja de clase. Entero en minutos, global por base, **35 por defecto**, rango **30–90**.
- **Franja de clase:** intervalo de duración **D**, no el resultado de repartir la ventana del turno.
- **Turno:** dato externo del plantel con una **ventana** (inicio y fin) y un **número de franjas
  declarado**. Se valida contra D y el tiempo de sus eventos.
- **Tiempo de eventos:** suma de las duraciones de los eventos del turno (spec 009).
- **Franjas utilizables de un turno:** máximo que cabe en su ventana,
  `floor((ventana − tiempo de eventos) / D)`. El número declarado no puede superarlo.
- **Déficit de encaje:** `nº franjas declarado × D + tiempo de eventos − ventana`, cuando es positivo.
- **Configuración global:** valor único persistido por base; no hay configuración por turno, curso,
  docente ni generación.
- **Valor efectivo:** el de la configuración si existe; si la base tiene el esquema pero no el valor,
  **35 por defecto**. Es el que usan la generación y la API.
- **Minutos derivados:** horas académicas × D; solo se obtienen en el **punto único** del backend.
- **Preset:** configuración de cálculo reutilizable (spec 005). Lleva la duración porque el contrato la
  necesita, pero es un **campo derivado**: no puede sobreescribir la configuración.
- **Huella de datos:** resumen determinista de los datos que intervienen en una generación (spec 004);
  esta spec **extiende** su definición para incluir D.
- **Bloque de clase distinto de la hora académica:** unidad de 80 minutos de Educación Media; **duda
  diferida**, fuera de alcance. Hoy 1 hora = 1 bloque = 1 franja.

## Requisitos funcionales

### RF-1. Configuración global de la duración

- **EL SISTEMA mantendrá una única configuración global de la duración de la hora académica por base**,
  en minutos enteros.
- **EL SISTEMA usará 35 minutos como valor por defecto** cuando la configuración no exista.
- **EL SISTEMA aceptará valores enteros entre 30 y 90 minutos, ambos incluidos.**
- **SI el valor no es un entero, es menor que 30 o es mayor que 90, ENTONCES EL SISTEMA rechazará la
  operación** indicando el rango válido y el valor recibido.
- **EL SISTEMA no admitirá más de una configuración global.**
- **EL SISTEMA validará el rango con el mismo punto único que usa la validación del contrato** (RF-5),
  de modo que un contrato con 5 minutos también se rechace.

### RF-2. Servicios de lectura y edición

- **EL SISTEMA ofrecerá un servicio para consultar la configuración** que devuelva el **valor efectivo**
  y si proviene del valor por defecto o de una configuración guardada.
- **EL SISTEMA ofrecerá un servicio para editar la configuración** que valide el rango y guarde el
  valor; una edición inválida no modifica el anterior.
- **CUANDO se cree la base, EL SISTEMA dejará la configuración con el valor por defecto.**
- **SI la base tiene el esquema nuevo pero no la fila de configuración, ENTONCES EL SISTEMA devolverá y
  usará 35 minutos** como valor efectivo, sin fallar; la primera edición crea el valor.
- **EL SISTEMA solo aceptará escrituras de la configuración por su servicio.**

### RF-3. Derivación única de minutos y terminología

- **EL SISTEMA tendrá un único punto que derive minutos desde horas académicas** multiplicando por D;
  ninguna otra pieza del backend hace esa conversión.
- **CUANDO se necesiten minutos para presentación o estimación, EL SISTEMA los obtendrá de ese punto
  único.**
- **El punto único no sustituye la conversión de rangos horarios a franjas del constructor de la
  entrada**, que sigue existiendo y usa D para alinear la rejilla (spec 005).
- **EL SISTEMA expresará la carga, los topes y las horas de aula en horas académicas** en su dominio y
  su documentación; los minutos no se usan como unidad de negocio.
- **EL SISTEMA mantendrá el tope de 40 horas académicas semanales por año** y no lo recalcula con D.
- **EL SISTEMA no hará conversiones de horas académicas a horas de reloj** en ningún otro punto.
- **La terminología en horas académicas se exige en el backend**; el `horasAula` del frontend queda como
  deuda de su equipo.

### RF-4. Franja de clase y encaje del turno

- **EL SISTEMA fijará la duración de la franja de clase en D**, no en el resultado de repartir la
  ventana del turno entre sus franjas.
- **EL SISTEMA calculará las franjas utilizables de un turno** como
  `floor((ventana − tiempo de eventos) / D)`; si el resultado es menor que 1, el turno no admite
  generación.
- **SI el número de franjas declarado por el turno supera las franjas utilizables, ENTONCES EL SISTEMA
  rechazará el registro o la actualización del turno y la generación** indicando el turno, la duración
  D y el déficit.
- **EL SISTEMA contará el tiempo de eventos del turno como la suma de las duraciones de sus eventos**
  (spec 009) al comprobar el encaje.
- **EL SISTEMA no repartirá el sobrante de la ventana entre las franjas de clase:** el sobrante queda
  como tiempo no usado al final del turno.
- **EL SISTEMA mantendrá la definición de ventana de posición de la 009**, pero con el ancho de franja
  D en lugar de la duración estimada.
- **EL SISTEMA no admitirá bloques de clase distintos de la hora académica** en esta versión; los 80
  minutos de Educación Media quedan fuera de alcance.

### RF-5. Contrato del solver alimentado desde la configuración

- **CUANDO se construya la entrada del solver, EL SISTEMA tomará D de la configuración global y
  sobrescribirá siempre la duración que traiga el preset**, esté presente o ausente, y sea cual sea su
  valor.
- **EL SISTEMA no usará nunca el valor de duración del preset para el cálculo:** el contrato efectivo
  lleva siempre el de la configuración.
- **SI la configuración cambió desde que se guardó el preset, ENTONCES la generación usará el valor
  nuevo**, sin error y sin aviso; la consulta de configuración devuelve siempre el valor efectivo.
- **EL SISTEMA aplicará la sobrescritura antes de validar la duración del contrato resultante**, de modo
  que un preset con 0, 5 o cualquier otro valor no bloquea la construcción.
- **EL SISTEMA validará la duración de todo contrato que llegue sin pasar por la construcción** con el
  **mismo punto único** y el rango 30–90; esta validación **estrecha V2** de la 005 y de la 010 y
  rechaza un contrato con 5 minutos.
- **La 010 mantiene el campo en el contrato 2.0 y su validación; esta spec no cambia su forma**, solo
  garantiza que el valor efectivo proviene siempre de la configuración y fija el rango 30–90.
- **EL SISTEMA seguirá construyendo la entrada sin que el solver consulte la base** (spec 005).

### RF-6. Huella de datos

- **EL SISTEMA incluirá D en la huella de datos** de la generación; esta spec **extiende** la
  definición de huella de la 004.
- **SI D cambia entre el inicio de una generación y la recepción del resultado, ENTONCES EL SISTEMA
  marcará el horario como «datos anteriores»** y pedirá confirmación antes de guardarlo (spec 004).

### RF-7. Esquema, versión y apertura

- **EL SISTEMA persistirá la configuración global en el esquema esperado** y la creará con el valor por
  defecto al crear la base.
- **CUANDO cambie el esquema esperado, EL SISTEMA incrementará la marca de versión de esquema** y
  actualizará en el mismo cambio el catálogo de tablas y columnas esperadas y la huella de esquema que
  lo vigila, según la política de la 009.
- **SI una base existente no tiene el esquema con la configuración, ENTONCES EL SISTEMA tratará la
  apertura como fallo**, sin migrar, sin borrar y con la opción de crear una base nueva con respaldo.
- **EL SISTEMA no supondrá la configuración en bases anteriores ni la creará por migración.**

### RF-8. Datos de prueba

- **EL SISTEMA no contará la configuración global como dato de dominio para la política de base vacía
  del seed:** su presencia no impide sembrar una base recién creada.
- **CUANDO se aplique el seed, EL SISTEMA asegurará la configuración en 35 minutos de forma
  idempotente**, con una sola fila y sin duplicarla; si ya existe, la deja en 35.
- **EL SISTEMA informará en el resultado de la siembra el valor efectivo (35) y el recuento de
  configuraciones (1).**
- **EL SISTEMA declarará turnos válidos con D = 35 y la invariante de encaje**, y sustituirá el
  inventario de 40 minutos de la 008.
- **EL SISTEMA mantendrá el determinismo:** dos siembras del mismo perfil dejan el mismo valor, el
  mismo recuento y los mismos turnos.
- **SI la base ya tiene datos de dominio, la siembra fallará o exigirá reemplazo** con la política de
  la 008, sin dejar la configuración a medio escribir.

### RF-9. Documentación de la API para el frontend

- **EL SISTEMA actualizará la guía de integración del frontend** con el servicio de consulta y edición,
  el valor efectivo y su origen, el rango 30–90 y los errores.
- **La guía documentará el punto único de derivación de minutos** y advertirá de que la aritmética en
  minutos de la vista y su `horasAula` son **deuda del equipo de frontend**.
- **La guía indicará que el campo de duración del preset es **derivado y nunca se usa**: la construcción
  lo sobrescribe siempre con el valor de la configuración, y un cambio de configuración desde que se
  guardó el preset no es un error ni genera aviso.
- **La guía documentará que el selector de duración del editor de presets y de la vista de generación
  deja de ofrecerse** como deuda del equipo de frontend, sin exigir su cambio en esta spec.
- **La guía enumerará los cambios de firma que rompen la compilación del frontend**, si los hay.

## Requisitos no funcionales

- **RNF-1. Mensajes en español:** todo rechazo identifica el campo, el valor, el rango o el déficit y la
  acción disponible.
- **RNF-2. Protección de datos:** no hay migración automática; una base con esquema no esperado no se
  borra ni se sobrescribe, y las operaciones destructivas conservan el respaldo (constitución §5).
- **RNF-3. Determinismo:** el valor por defecto y la siembra no dependen del reloj, del azar ni del
  orden de inserción.
- **RNF-4. Punto único:** la validación del rango y la derivación horas→minutos existen en un solo
  lugar; el resto del backend no las replica.
- **RNF-5. Verificación sin la interfaz:** la construcción debe poder excluir la interfaz y sus pruebas
  (009, RNF-6); con esa configuración, backend, datos, seed, consola y sus pruebas compilan y pasan.
- **RNF-6. Trazabilidad:** cada RF de esta spec tendrá al menos una comprobación verificable en las
  tareas; sin cobertura, la tarea no está hecha.

## Casos límite

- **Duración de 29 o 91 minutos:** rechazo con el rango 30–90 (RF-1).
- **Duración de 30 o 90 minutos:** se aceptan, extremos inclusivos (RF-1).
- **Duración no entera:** rechazo (RF-1).
- **Base con el esquema nuevo pero sin fila de configuración:** el valor efectivo es 35 y la API
  informa que es el defecto (RF-2).
- **Base con el esquema nuevo, sin configuración, y edición válida:** se crea el valor y deja de ser
  defecto (RF-2).
- **Base anterior sin la configuración en el esquema:** fallo de apertura, sin migración (RF-7).
- **Cambio de D entre generaciones:** la huella cambia y el horario se marca «datos anteriores»
  (RF-6).
- **Edición de la configuración durante una generación en curso:** la generación termina con el valor
  inicial y, al cambiar D, el resultado se marca «datos anteriores» (RF-6).
- **Preset con 0, 5, 45 minutos o sin el campo:** el valor se ignora siempre; se usa D de la
  configuración; no es error ni requiere aviso (RF-5).
- **Preset guardado con 45 minutos y configuración cambiada después a 35:** la generación usa 35, sin
  error ni aviso; la consulta devuelve 35 (RF-5).
- **Contrato 2.0 con 5 minutos cargado sin pasar por la construcción:** rechazo por la validación
  estrecha de V2 (RF-5).
- **Configuración de 45 y un turno dimensionado para 35** (8 franjas, ventana 290, 10 de eventos):
  `8 × 45 + 10 = 370 > 290`, déficit 80; rechazo con turno, D y déficit (RF-4).
- **Configuración de 35 y un turno dimensionado para 45** (8 franjas, ventana 370, 10 de eventos):
  `8 × 35 + 10 = 290 ≤ 370`; encaja con sobrante no usado (RF-4).
- **Configuración de 35 con un turno corto** (5 franjas, ventana 150, 10 de eventos):
  `5 × 35 + 10 = 185 > 150`, déficit 35; rechazo (RF-4).
- **Turno sin ninguna franja utilizable:** rechazo de la generación (RF-4).
- **Cálculo de minutos de 40 horas académicas con 35 minutos:** 1400 minutos; el tope de 40 horas no
  cambia (RF-3).
- **Cálculo con 30 minutos:** 40 horas académicas son 1200 minutos; el tope sigue en 40 horas (RF-3).
- **Dos configuraciones globales:** no puede existir más de una (RF-1).
- **Siembra sobre una base recién creada:** la configuración existe o se asegura; no bloquea la
  siembra ni duplica filas (RF-8).
- **Siembra repetida:** deja una sola configuración con 35 y el mismo recuento (RF-8).
- **Siembra sobre una base con datos de dominio:** falla o exige reemplazo, sin dejar la configuración
  a medias (RF-8).
- **Bloques de 80 minutos de Educación Media:** no se modelan; duda diferida (definiciones).
- **Franja calculada repartiendo la ventana:** no ocurre; la franja dura D y el sobrante no se reparte
  (RF-4).

## Fuera de alcance

- **El modelo del solver y el contrato 2.0:** son de la 005 y la 010; esta spec solo cambia la
  procedencia y el rango de la duración.
- **Los bloques de clase distintos de la hora académica (80 minutos de Media):** duda diferida; hoy
  1 hora = 1 bloque = 1 franja.
- **La interfaz de usuario, su aritmética en minutos y su `horasAula`:** deuda documentada del equipo
  de frontend (RF-9).
- **El tope de 40 horas académicas semanales por año:** no se cambia.
- **Calendarios con duraciones distintas por día, turno o curso:** no existen; la configuración es
  global.
- **La migración de bases existentes y la conversión de valores anteriores:** no hay migración (RF-7).
- **Cambiar la versión del contrato:** sigue en 2.0.
- **Presets que sobreescriban la duración:** no se admiten; el campo es derivado (RF-5).

## Criterios de finalización

1. Existe una configuración global única, con 35 por defecto y rango 30–90, que rechaza valores fuera
   de rango y usa el mismo punto que la validación del contrato (RF-1, RF-2).
2. La consulta devuelve el valor efectivo y su origen; la edición valida y no modifica nada si falla;
   la base con esquema y sin fila usa 35 (RF-2).
3. Hay un único punto de horas académicas a minutos; el backend habla en horas académicas y el tope de
   40 no cambia; la conversión de rangos a franjas del constructor sigue existiendo (RF-3).
4. La franja de clase dura D; las franjas utilizables se calculan con la ventana, los eventos y D; un
   turno que no encaja se rechaza con turno, D y déficit (RF-4).
5. La entrada toma D de la configuración y **sobrescribe siempre** la duración del preset; un contrato
   con D fuera de 30–90 se rechaza; el contrato sigue en 2.0 (RF-5).
6. La huella incluye D y un cambio entre generaciones marca «datos anteriores» (RF-6).
7. El esquema incluye la configuración, la marca de versión se incrementó con su catálogo y su huella,
   y una base anterior falla sin migrar (RF-7).
8. El seed asegura 35 de forma idempotente, no lo cuenta como dato de dominio, informa el valor y el
   recuento 1, y declara turnos válidos con D = 35 (RF-8).
9. La guía de API documenta la configuración, el punto único, la sobrescritura del campo del preset y
   la deuda del frontend (RF-9).
10. Las pruebas del constructor y del seed afectadas se reescriben a 35 y a la invariante, y la
    construcción sin interfaz pasa (RNF-5).
11. Cada RF tiene al menos una comprobación verificable en las tareas (RNF-6).

## Dudas abiertas

Ninguna. Queda una **duda diferida fuera de alcance**: los bloques de clase distintos de la hora
académica (80 minutos de Educación Media). Las decisiones de default 35, rango 30–90, autoridad de la
configuración, validación del turno, sobrescritura del preset y exclusión del frontend están cerradas.
