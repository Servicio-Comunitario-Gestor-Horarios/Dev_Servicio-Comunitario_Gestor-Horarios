# Spec 020 - Experimento: tope duro de horas del docente y plazo de una hora

Estado: aprobada (2026-11-12)

> **Naturaleza del cambio.** Esta spec **no** es una evolución del producto: es un **experimento
> aislado** para responder una pregunta concreta —«¿se obtiene horario con el centro real si el tope de
> horas del docente deja de ser blando y se concede hasta una hora de cálculo?»—. Se redacta y se
> implementa en una **rama aislada**; nada de lo que aquí se decide se fusiona en la rama de trabajo sin
> una decisión posterior y sin reconciliar los conflictos declarados al final.
>
> **Pregunta del experimento.** Hoy el tope de horas del docente es una **preferencia blanda**: el
> modelo permite que un docente supere sus horas de aula y penaliza el exceso en el objetivo, de modo
> que ninguna combinación de horas declara el horario imposible. Con el centro real y 36 horas por
> docente, la generación superó el plazo vigente de 300 s y la interfaz informó de que el cálculo no
> había respondido. El experimento cambia el **tope** (blando → duro) y el **plazo** (300 s → una hora),
> y añade lo mínimo para que el resultado se pueda interpretar: **conservar la causa del motor** cuando
> no hay solución y **distinguir los desenlaces** (horario, NoFactible, no concluyente, plazo agotado).
> El resto del modelo, del contrato, del esquema y de los datos queda intacto.
>
> **Corrección tras el QA (2026-11-12).** Esta versión corrige cinco bloqueantes del reviewer: la causa
> del NoFactible se propaga (excepción parcial y declarada a la 019), el reparto de la hora es 60 min de
> proceso con 55 min de solver en total y 5 min de reserva para extracción y comunicación, una solución
> factible al límite se presenta, el presupuesto agotado sin prueba no se confunde con NoFactible, y la
> unidad del plazo es **tiempo de reloj**.
>
> **Tercera pasada (2026-11-12).** Fija una **sola regla de margen** (los 55 min son del solver en total;
> las fases blandas usan lo que quede; los 5 min son reserva de extracción y comunicación), separa
> **«sin respuesta»** (pérdida de comunicación) de **«respuesta inválida»**, define el **error de
> entrada/contrato** como camino propio, y acota la **no optimalidad a la fase** que agotó su
> presupuesto.

## Alcance

**Dentro del alcance:**

1. Convertir el tope de horas de aula del docente en **invariante dura**, sin exceso y sin relajación
   automática, para titulares y suplentes.
2. Elevar a **60 minutos de reloj** el plazo del proceso y a **55 minutos de reloj** el presupuesto del
   solver **en total** (referencia y fases blandas), con **5 minutos de reserva** para extracción y
   comunicación.
3. **Conservar y presentar la causa** que produce el motor cuando no hay solución (propagación mínima
   del diagnóstico), en lugar de un mensaje genérico.
4. **Distinguir los desenlaces:** horario (óptimo o no, con la optimalidad declarada por fase),
   NoFactible con causa, no concluyente por presupuesto agotado sin prueba, sin respuesta por pérdida de
   comunicación y plazo agotado sin respuesta; y tratar aparte los errores de entrada o de contrato.
5. Informar del **reparto de horas** resultante y comprobar el tope contra las horas de aula.
6. Mantener intactas las demás invariantes duras y todo lo que no sea la regla de horas, el plazo, la
   causa y el reparto.

**Fuera del alcance:** el resto de los hallazgos H1–H16 de la 019 (solo se admite la excepción parcial
de H3 descrita), el modelo de materias genéricas, el rendimiento y la descomposición del cálculo, el
esquema, el contrato 2.0, los presets, la huella de datos, los datos del centro y cualquier pantalla
nueva. El detalle está en «Fuera de alcance».

## Decisiones cerradas (2026-11-12)

| # | Decisión | Resolución | Origen |
|---|---|---|---|
| D-1 | Regla de horas | **Tope duro para todos los docentes, sin exceso.** `Σ clases ≤ horas_aula` para titulares y suplentes. Un docente con `horas_aula = 0` **no puede dar clase**. No hay válvula de exceso ni relajación automática. | Usuario |
| D-2 | Plazo **(corregida)** | **60 minutos de reloj de proceso y 55 minutos de reloj de solver en total.** El servicio espera hasta **60 min** desde la petición. El **solver dispone de 55 min en total**: la fase de referencia puede consumir hasta **55** y las fases blandas usan **lo que quede de esos 55**, no un margen aparte. Los **5 min** hasta los 60 son **siempre reserva de extracción y comunicación** y **no** se usan para fases blandas. Si la referencia consume los 55, no hay fases blandas y se devuelve su resultado (factible o no concluyente). El servicio **no abandona antes de los 60 min**. El cálculo lento lo ejecuta **el usuario a mano**. | Usuario (QA B2/B5/N1) |
| D-3 | Resultado no factible | **Si con 36 h el modelo es NoFactible, se acepta y se muestra la causa** (categorías (1)–(5) o «no atribuible»). **No** se sube el tope automáticamente ni se cae al comportamiento blando. | Usuario |
| D-4 | Aislamiento | **Rama aislada `experimento/horas-duras-plazo-1h`**, creada desde `feat/Motor` en `455ba7f`. Nada de esto se fusiona en `feat/Motor` sin una decisión posterior. | Usuario |
| D-5 | Causa del NoFactible | **Se incluye en la 020 la propagación mínima de la causa** (el diagnóstico del motor que hoy se descarta): es una **excepción parcial y declarada a la 019**, solo el mínimo de H3. El resto de la 019 sigue fuera de alcance. | Usuario (QA B1) |

## Contexto y objetivo

El sistema garantiza hoy que un docente **preferentemente** no supere sus horas de aula, pero no lo
impone: si la demanda no se puede cubrir de otra forma, el modelo permite el exceso, lo penaliza en el
objetivo y lo minimiza. Esa decisión se tomó porque las horas totales que hay que repartir las fijan
las materias de los cursos, y declarar el horario imposible por horas no se consideró aceptable. El
resultado es que el tope no acota nada: es una inclinación, no una restricción.

Con el centro real (24 cursos, 33 docentes, 876 clases y un tope de 36 horas por docente) la generación
se acerca y a veces supera el plazo vigente de 300 s, y la interfaz termina informando de que el
cálculo no respondió. El usuario quiere saber si el problema es **de modelo o de tiempo**: si al
imponer el tope como regla dura el centro sigue teniendo solución, y si con una hora de cálculo el
motor la encuentra.

Hay además un defecto de integración que impide interpretar cualquier fallo: aunque el motor produce un
diagnóstico con la causa y sus cifras, el recorrido hasta la vista **lo sustituye por un mensaje
genérico** («no existe una solución factible»). Sin la causa, un NoFactible del experimento no enseña
nada. Por eso la 020 incluye **solo la propagación mínima** de ese diagnóstico, como excepción parcial
declarada a la 019.

**Objetivo.** Obtener una respuesta verificable, en un entorno aislado, a dos preguntas: (a) ¿existe un
horario que respete el tope de horas de todos los docentes con los datos actuales?, y (b) ¿lo encuentra
el motor dentro de una hora? Los desenlaces admisibles son: horario (óptimo o no, con la optimalidad
declarada por fase), NoFactible con causa, no concluyente por presupuesto agotado sin prueba, sin
respuesta por pérdida de comunicación y plazo agotado; los errores de entrada o de contrato se informan
aparte, con su causa.

**Qué no es.** No es una corrección del producto ni de la integración del frontend (salvo la
propagación mínima de la causa); no implementa el resto de la 019; no mejora el rendimiento; no cambia
el modelo de negocio de las materias; no altera el contrato, el esquema, los presets ni los datos; y no
pretende ser la versión definitiva de la regla de horas.

## Usuarios

- **Usuario que genera horarios (el experimentador):** quiere saber si el centro real tiene solución
  con el tope duro y dispone de hasta una hora para esperar una respuesta.
- **Dirección y coordinación del liceo:** necesitan saber si las 36 horas declaradas por docente
  alcanzan para cubrir la carga real y, si no, por qué.
- **Equipo de desarrollo y pruebas:** necesita un cambio **aislado, reversible y con criterio de
  aceptación ejecutable**, que no arrastre a la rama de trabajo ni obligue a reescribir specs
  aprobadas mientras dure el experimento.

## Historias de usuario

- HU-1. Como usuario que genera horarios, quiero que ningún docente reciba más clases de las horas que
  tiene declaradas, para que el horario respete de verdad el contrato del centro.
- HU-2. Como usuario que genera horarios, quiero que, si con el tope duro no hay solución, el sistema me
  muestre **la causa que calculó el motor** (o «no atribuible») y no un mensaje genérico, para saber qué
  ajustar.
- HU-3. Como usuario que genera horarios, quiero disponer de hasta una hora de cálculo, que la interfaz
  me lo diga y que un horario encontrado al límite **no se descarte por no estar probado como óptimo**.
- HU-4. Como usuario que genera horarios, quiero distinguir «no hay solución» de «no se pudo concluir» y
  de «se agotó el plazo», para no confundir un problema de los datos con una falta de tiempo.
- HU-5. Como equipo de desarrollo, quiero que el cambio viva en una rama aislada y sea reversible, para
  no comprometer la rama de trabajo ni las specs ya aprobadas.

## Definiciones

- **Horas de aula declaradas (`horas_aula`):** horas académicas semanales que el usuario declara para
  un docente en su configuración. En este experimento son el **tope duro** de ese docente.
- **Horas de aula ausentes:** si el dato no viene en la configuración, se interpreta como **cero** (el
  docente no puede dar clase) y **se informa** de qué docentes quedan sin capacidad por ese motivo, para
  que no sea un deshabilitado silencioso.
- **Horas requeridas (`horas_requeridas`):** dato de dominio opcional del docente. Sigue siendo una
  **cota de coherencia** del dato (las horas de aula no deben superarlas) y **no** es el tope que aplica
  el cálculo. El aviso P2 compara contra este dato, no contra las horas de aula.
- **Tope duro:** invariante que exige que la suma de horas académicas de clase de un docente no supere
  sus horas de aula declaradas. No admite excepción, ni compensación, ni penalización.
- **Exceso:** diferencia entre las clases asignadas a un docente y su tope. En este experimento **deja
  de existir como resultado admisible**; el término se conserva solo para el comportamiento anterior y
  para el aviso de coherencia P2.
- **Hora académica:** equivale a un bloque de clase. La comparación del tope es siempre en horas
  académicas y no depende de la duración real de la franja.
- **Fase de referencia:** la resolución que produce el horario base —la asignación de docente por
  materia-curso y la preferencia de titulares sobre suplentes—. Es el resultado que se devuelve si
  consume el presupuesto del solver.
- **Fases blandas:** resoluciones posteriores que solo mejoran aspectos no obligatorios (separación
  entre apariciones, huecos fuera del viernes y preferencia horaria) sin cambiar la asignación de
  docente ni el tope. Se ejecutan **solo con lo que quede de los 55 minutos del solver** después de la
  referencia; **no** tienen un margen aparte y **no** usan la reserva de 5 minutos.
- **Plazo del proceso:** **60 minutos de reloj** desde la petición hasta la respuesta. Es el límite del
  servicio y de la interfaz.
- **Presupuesto del solver:** **55 minutos de reloj en total** para el conjunto de fases (referencia y
  blandas). Es **tiempo de reloj**, no tiempo determinista del motor. La referencia puede consumirlo
  entero.
- **Reserva:** **5 minutos** entre el presupuesto del solver (55) y el plazo del proceso (60),
  **siempre reservados** a la extracción de la solución y a la comunicación. **No** se usan para fases
  blandas.
- **Horario presentado:** solución factible devuelta por el motor y presentada al usuario. La
  optimalidad se declara **por fase**: la marca de «no óptima» se refiere a la fase cuyo presupuesto se
  agotó sin probarla (la **fase de referencia** o una fase blanda concreta), **no** a las demás. La
  falta de prueba de optimalidad **no** descarta el horario.
- **NoFactible:** el motor ha **demostrado** que no existe ninguna solución que respete todas las
  invariantes duras, incluido el tope de horas. Es un resultado del experimento, no un error del
  sistema.
- **No concluyente:** el motor agotó su presupuesto **sin encontrar solución y sin demostrar que no
  exista** (presupuesto agotado sin prueba). **No** es NoFactible y **no** se presenta con un mensaje
  de infactibilidad.
- **Sin respuesta:** se perdió la comunicación con el proceso (conexión caída o proceso que no contesta)
  antes de agotarse el plazo. Es un desenlace **distinto de una respuesta inválida** y de NoFactible; el
  mensaje indica el motivo y no afirma que no exista solución.
- **Plazo agotado:** el proceso **no respondió dentro de los 60 minutos**. Es distinto de «sin
  respuesta» (pérdida de comunicación antes del plazo), de NoFactible y de no concluyente; el mensaje no
  afirma que no exista solución.
- **Error de entrada o de contrato:** el motor devuelve un modelo inválido o un error interno, o la
  respuesta no corresponde al contrato. Se informa con su causa y **no** es NoFactible ni no
  concluyente.
- **Causa:** la categoría del catálogo vigente —(1) entrada inválida, (2) conflicto de aula,
  (3) conflicto de persona, (4) conflicto de asignación de docente y (5) conflicto de bloques— con sus
  cifras, o la declaración de **«no atribuible»** cuando ninguna categoría explica el fallo. El
  diagnóstico que produce el motor es el que se presenta, sin sustituirlo por un texto genérico.
- **Reparto de horas:** resumen observable del resultado que, por cada docente con clases, indica las
  horas académicas asignadas y su tope de horas de aula declarado, y que totaliza las horas asignadas
  del centro frente a la suma de los topes.
- **Datos anteriores:** condición de un resultado cuyo cálculo se hizo sobre una instantánea de los
  datos que ya no coincide con la actual. El experimento **conserva** el marcado vigente; no implementa
  la confirmación de la 019.
- **Rama aislada:** rama de trabajo separada de la rama de producto, creada para este experimento, que
  se puede descartar sin afectar a lo ya integrado.

## Requisitos funcionales

### RF-1. El tope de horas de aula es duro para todos los docentes

- **EL SISTEMA tratará las horas de aula declaradas de cada docente como una invariante dura:** la suma
  de horas académicas de clase que se le asignen **no superará nunca** ese valor, tanto si actúa como
  titular como si actúa como suplente.
- **EL SISTEMA no admitirá ningún resultado de generación con exceso de horas:** no habrá una solución
  devuelta en la que un docente reciba más clases que sus horas de aula, ni siquiera cuando la demanda
  no pueda cubrirse de otra forma. Los horarios guardados con anterioridad no se revalidan ni se
  rechazan por esta spec.
- **SI las horas de aula declaradas de un docente son cero, ENTONCES EL SISTEMA no le asignará ninguna
  clase.**
- **SI las horas de aula de un docente faltan en la configuración, ENTONCES EL SISTEMA las tratará como
  cero, no le asignará clases e informará de qué docentes quedan sin capacidad por ese motivo**, sin
  deshabilitarlos en silencio.
- **SI una materia-curso solo puede cubrirse con un docente que ya alcanzó su tope, o con un docente de
  tope cero, ENTONCES EL SISTEMA no lo usará por encima del tope** y no relajará la regla.
- **EL SISTEMA no relajará el tope de horas de forma automática:** ni por reintento, ni por
  configuración, ni como último recurso, ni por ningún otro mecanismo.
- **EL SISTEMA comparará horas académicas con horas académicas** (una hora = un bloque) y **no** con
  horas de reloj; la duración real de la franja no altera el tope.
- **MIENTRAS exista el dato de horas requeridas, EL SISTEMA mantendrá la coherencia vigente** (horas de
  aula no por encima de las horas requeridas); el tope que aplica el cálculo es el de horas de aula.
- **Criterio de aceptación:** en todo horario devuelto, la cuenta de clases por docente no supera sus
  horas de aula declaradas; un docente con tope cero (declarado o ausente) no aparece con ninguna clase
  y el resultado lo informa; el resultado de una generación nunca presenta un docente por encima de su
  tope; un horario guardado antes del experimento con exceso no se rechaza por esta regla.

### RF-2. Sin solución demostrada, el resultado es NoFactible con la causa del motor

- **SI el motor demuestra que no existe solución que respete el tope duro junto con las demás
  invariantes, ENTONCES EL SISTEMA devolverá NoFactible.**
- **CUANDO devuelva NoFactible, EL SISTEMA conservará y presentará el diagnóstico que produjo el
  motor** —la causa del catálogo (1)–(5) con sus cifras o la declaración de «no atribuible»—, **en lugar
  de sustituirlo por un mensaje genérico**. Si el diagnóstico llegara vacío, informará de que no hay una
  causa atribuible.
- **EL SISTEMA no declarará NoFactible cuando el motor no haya demostrado la infactibilidad:** un
  presupuesto agotado sin prueba es **no concluyente** (RF-3), no NoFactible.
- **EL SISTEMA no declarará NoFactible por una respuesta inválida, un modelo inválido o un error
  interno:** esos desenlaces se informan como **error de entrada o de contrato** (RF-3), con su causa.
- **EL SISTEMA no subirá el tope, no cambiará al comportamiento blando y no reintentará con una regla
  relajada** para obtener un horario.
- **EL SISTEMA no alterará la configuración, los datos ni el contrato** para volver factible la
  instancia.
- **SI la única razón del fallo son las horas** —la demanda supera la capacidad conjunta de los topes
  duros—, **ENTONCES EL SISTEMA informará «no atribuible»** sin inventar una categoría nueva y añadirá,
  como dato de diagnóstico, **la demanda total de horas del centro y la suma de los topes duros**; si
  además aplica otra categoría del catálogo, se informará esa, con la precedencia vigente.
- **CUANDO devuelva NoFactible, EL SISTEMA no presentará horario ni ofrecerá guardarlo.**
- **Criterio de aceptación:** un caso con demanda de horas superior a la suma de los topes duros devuelve
  NoFactible, el mensaje **contiene el texto del motor** (la causa o «no atribuible») y las dos cifras
  (demanda y suma de topes), y **no** el mensaje genérico «no existe una solución factible»; ningún
  camino devuelve un horario con exceso.

### RF-3. El cálculo dispone de hasta 60 minutos de reloj y los desenlaces se distinguen

- **EL SISTEMA dispondrá de hasta 60 minutos de reloj** desde la petición hasta la respuesta; **el plazo
  de 300 s dejará de aplicarse** en este experimento. **El plazo se cuenta en tiempo de reloj, no en
  tiempo determinista del motor.**
- **EL SISTEMA dispondrá de 55 minutos de reloj de solver en total.** La **fase de referencia** puede
  consumir hasta esos 55 minutos; las **fases blandas** usan **lo que quede de esos 55**, no un margen
  aparte. Los **5 minutos** restantes hasta los 60 son **siempre reserva de extracción y comunicación** y
  **no** se usan para fases blandas.
- **MIENTRAS la fase de referencia termine antes de los 55 minutos, EL SISTEMA podrá ejecutar las fases
  blandas con lo que quede de esos 55**; **SI la fase de referencia consume los 55 minutos, ENTONCES
  devolverá su resultado** sin ejecutar las fases blandas.
- **SI el motor devuelve una solución factible al agotar el presupuesto de la fase de referencia,
  ENTONCES EL SISTEMA presentará la mejor solución encontrada** e indicará que **la asignación de
  docentes (fase de referencia) no está probada como óptima**. **No se descarta un horario factible por
  falta de prueba de optimalidad.**
- **SI la fase de referencia se prueba óptima y una fase blanda agota su parte del presupuesto sin
  probarse óptima, ENTONCES EL SISTEMA presentará el horario y la indicación de no optimalidad se
  referirá solo a esa fase blanda** (separación y huecos, o preferencia), **sin calificar de no óptima
  la asignación de docentes**.
- **SI el motor devuelve un resultado no concluyente por presupuesto agotado sin prueba de
  factibilidad ni de infactibilidad, ENTONCES EL SISTEMA informará de un desenlace «no concluyente»**,
  con el texto del motor, **distinto de NoFactible**, y no presentará horario.
- **SI se pierde la comunicación con el proceso (conexión caída o proceso que no contesta) antes de los
  60 minutos, ENTONCES EL SISTEMA informará de un desenlace «sin respuesta»**, con el motivo, **distinto
  de una respuesta inválida** y distinto de NoFactible, y no presentará horario. **EL SISTEMA no
  presentará una pérdida de comunicación como un contrato incumplido.**
- **SI el proceso no responde dentro de los 60 minutos, ENTONCES EL SISTEMA informará de «plazo
  agotado»** y no presentará horario; su mensaje **no afirmará que no exista solución** y será distinto
  del de «sin respuesta» y del de NoFactible.
- **SI la respuesta del motor o del proceso es inválida, o el motor devuelve un modelo inválido o un
  error interno, ENTONCES EL SISTEMA informará de un error de entrada o de contrato con su causa**, y
  **no** lo presentará como NoFactible ni como no concluyente.
- **SI el cálculo termina antes de la hora, EL SISTEMA devolverá el resultado sin esperar al límite.**
- **EL SISTEMA no abandonará la petición antes de los 60 minutos.**
- **EL SISTEMA informará en la interfaz de que el cálculo puede tardar hasta una hora** y dejará de
  anunciar que tarda «unos segundos».
- **EL SISTEMA no interrumpirá, reiniciará ni dará por caído el proceso de cálculo** por tardar más que
  el plazo anterior.
- **Criterio de aceptación:** con un plazo corto inyectado para la prueba automática, el sistema informa
  de plazo agotado y no de NoFactible; con una solución factible al límite de la referencia, el horario
  se presenta con la indicación de que la **asignación de docentes** no está probada como óptima; si la
  referencia es óptima y una fase blanda no, la indicación se refiere solo a esa fase; con un resultado
  no concluyente, el mensaje es distinto del de NoFactible; con una pérdida de comunicación, el mensaje
  es «sin respuesta» y no de contrato incumplido; con una respuesta inválida o un error interno, el
  mensaje es de entrada/contrato y no de NoFactible; con una infactibilidad demostrada, el mensaje es
  NoFactible con causa; la comprobación con la hora real es evidencia manual del usuario.

### RF-4. Las demás invariantes duras y el resto del contrato no cambian

- **EL SISTEMA conservará intactas todas las invariantes duras vigentes:** un docente por
  materia-curso; las horas exactas de cada materia-curso; ningún solapamiento de curso, docente o aula
  por encima del aforo del aula; elegibilidad por turno, plan, año y disponibilidad; tipo de aula exacto
  y aula de referencia; bloques de dos horas con su alineación, su misma aula y sus huecos al final; y
  la cobertura de titulares y suplentes elegibles.
- **EL SISTEMA aplicará el tope duro de horas además de esas invariantes**, no en lugar de ellas.
- **EL SISTEMA no cambiará el contrato de configuración, los presets, el esquema de la base, la huella
  de datos ni los datos del centro.**
- **EL SISTEMA seguirá produciendo un resultado válido con las reglas vigentes** y no prometerá el
  mismo horario entre ejecuciones.
- **Criterio de aceptación:** un preset vigente se acepta sin cambios; el horario obtenido no presenta
  incumplimientos de las invariantes vigentes; un resultado NoFactible, no concluyente, sin respuesta, de
  plazo agotado o de error de entrada o de contrato no se presenta como horario.

### RF-5. El resultado informa del reparto y comprueba el tope contra las horas de aula

- **CUANDO el sistema obtenga horario, el resultado incluirá un reparto de horas observable** con **una
  línea por cada docente con clases** —horas académicas asignadas y tope de horas de aula declarado— y
  **una línea de totales** —horas asignadas del centro frente a la suma de los topes—.
- **EL SISTEMA presentará ese reparto en la vista de generación o en el resumen del resultado** con los
  medios existentes, sin pantallas nuevas: la misma información por docente y los totales.
- **EL SISTEMA hará una comprobación posterior contra las horas de aula:** **SI alguna asignación de un
  docente supera su tope, ENTONCES el resultado no será presentable** y se informará como fallo de
  validación, sin presentarlo como horario.
- **EL SISTEMA mantendrá el aviso de exceso P2, que compara contra las horas requeridas.** Bajo el tope
  duro, ese aviso **no comprueba el tope de horas de aula** y no sustituye la comprobación anterior; si
  apareciera, se informa como dato de coherencia, sin convertirlo en la verificación del tope.
- **CUANDO devuelva NoFactible, no concluyente, sin respuesta, plazo agotado o un error de entrada o de
  contrato, EL SISTEMA no mostrará un reparto de horas.**
- **EL SISTEMA no cambiará el significado del dato de horas requeridas** ni lo usará como tope del
  cálculo.
- **Criterio de aceptación:** en un horario presentado, el reparto coincide con la cuenta de clases por
  docente y ninguna supera su tope de horas de aula; un resultado construido con una asignación por
  encima del tope **no es presentable**; el aviso P2 se mantiene separado y no se usa como prueba del
  tope; con NoFactible, no concluyente o plazo agotado, no hay reparto.

### RF-6. La validación previa no cambia: la falta de capacidad de horas no invalida la entrada

- **EL SISTEMA mantendrá la validación previa vigente sin cambios:** cada materia-curso seguirá
  necesitando al menos un candidato elegible por turno, plan, año y disponibilidad, y **la capacidad de
  horas no formará parte de esa validación**.
- **SI un docente no tiene capacidad de horas, ENTONCES seguirá siendo elegible** y la entrada **no** se
  rechazará por ese motivo.
- **SI la falta de capacidad de horas hace imposible el horario, ENTONCES el desenlace se producirá al
  resolver** (NoFactible o no concluyente), **no** como un rechazo previo de la entrada.
- **SI la entrada es inválida por otra causa, ENTONCES EL SISTEMA la rechazará como hoy**, con su
  mensaje.
- **Criterio de aceptación:** con un docente sin capacidad de horas como único candidato de una
  materia-curso, la validación previa **pasa**, el cálculo se ejecuta y el desenlace es NoFactible o no
  concluyente; en ningún caso el motivo es «entrada inválida por horas».

## Requisitos no funcionales

- **RNF-1. Tests primero y línea base acotada.** Cada cambio se acompaña de su comprobación escrita
  antes del código. Ninguna tarea cierra con el árbol sin compilar ni con pruebas en rojo. **«Línea
  base» son las pruebas no ligadas al comportamiento superseded:** todo lo que hoy pasa y no afirma el
  comportamiento anterior sigue pasando. **Se espera reescribir** las pruebas que afirman el exceso
  blando, la penalización del exceso en el objetivo, el ancla del exceso, los presupuestos 60/20/10, el
  plazo de 300 s y el uso del aviso P2 como control del tope. **Ajustarlas no es una regresión**: son
  pruebas de la regla que esta spec sustituye y se actualizan en la misma tarea.
- **RNF-2. El cálculo lento no lo lanza el agente.** La comprobación con el centro real y el plazo de
  una hora **la ejecuta el usuario a mano**; ningún agente lanza el cálculo lento. **La prueba
  automática del plazo usa un plazo corto inyectado**, que el servicio ya admite; la hora real es
  evidencia manual. La evidencia se registra en el diario de la spec.
- **RNF-3. Cambio aislado y reversible.** Todo el cambio vive en la rama del experimento; **no** se toca
  la rama de producto, **no** se fusiona y se puede descartar revirtiendo la rama. No deja estado
  persistente ni datos convertidos. **La spec se commitea antes de implementar** y el diff se limita a
  los archivos del plan.
- **RNF-4. El cálculo largo no se interpreta como caída.** El cliente de comprobaciones **no tiene
  plazo propio**: mientras el proceso está ocupado, las solicitudes se **encolan** y se responden
  después; no se consumen intentos ni se declara caída, y el proceso **no se mata** por salud. **EL
  SISTEMA no debe matar el proceso ni presentarlo como colapsado** durante el cálculo largo. **SI se
  decide suspender la verificación mientras hay una generación en curso, ENTONCES se reanudará al
  terminar la generación**, sin emitir ningún aviso de colapso y dejando la verificación en su estado
  normal.
- **RNF-5. Determinismo de lo que se informa.** Son **deterministas** para los mismos datos: la regla
  del tope, el formato del reparto, la comprobación contra las horas de aula y el diagnóstico de un
  fallo demostrado. Son **variables** entre ejecuciones: el horario concreto, el reparto de horas (la
  asignación de docente por materia-curso puede cambiar con 8 workers) y el desenlace temporal (si se
  encuentra solución, si es óptima o si el presupuesto se agota), que depende del reloj y de la carga de
  la máquina.
- **RNF-6. Trazabilidad y tamaño.** Cada RF tiene al menos una comprobación verificable en las tareas y
  la spec cabe en **10 tareas como máximo** (criterio de proceso, no de aceptación funcional).
- **RNF-7. Sin cambios de esquema, contrato, presets ni datos.** No se alteran la estructura de la
  base, el contrato de configuración, los presets vigentes, la huella de datos ni el conjunto del
  centro. Tampoco se añaden dependencias.
- **RNF-8. La regla vive una sola vez.** El tope duro se resuelve en la lógica y se comprueba sin
  interfaz; la pantalla solo presenta el plazo, el reparto y la causa, y no recalcula ni duplica la
  regla.
- **RNF-9. Sin pantallas nuevas.** Solo se admiten los textos y los indicios que exigen los RF (el
  plazo de una hora, el reparto de horas, la no optimalidad por fase y la causa); no se rediseñan pantallas.
- **RNF-10. Reversión declarada.** La rama documenta cómo volver al comportamiento anterior (tope
  blando y plazo de 300 s) sin tocar nada más.

## Casos límite

- **Docente con cero horas de aula que es el único candidato de una materia-curso:** no recibe clases;
  el resultado es NoFactible o no concluyente, nunca un exceso (RF-1, RF-2, RF-6).
- **Horas de aula ausentes en la configuración:** se tratan como cero; el docente no recibe clases y el
  resultado identifica a los docentes sin capacidad por ese motivo (RF-1).
- **Demanda total de horas superior a la suma de los topes duros:** NoFactible; se informa «no
  atribuible» con la demanda total y la suma de topes; no se sube ningún tope (RF-2).
- **Docente exactamente en su tope:** es válido; no hay exceso (RF-1, RF-5).
- **Suplente con el tope lleno:** no recibe clases; si no queda ningún candidato con capacidad, el
  resultado es NoFactible o no concluyente, nunca un exceso silencioso (RF-1, RF-2).
- **Docente con tope cero que no imparte ninguna materia:** no aparece en el reparto y no genera error
  (RF-5).
- **Horas requeridas ausentes:** el tope aplicable es el de horas de aula y la comprobación de
  coherencia se omite, como hoy (RF-1).
- **Horas de aula por encima de las horas requeridas:** se mantiene el rechazo previo por dato
  incoherente, igual que hoy; no es una regla nueva (RF-1).
- **Horario guardado antes del experimento con exceso:** no se revalida ni se rechaza por esta spec; la
  regla se aplica a los resultados de generación nuevos (RF-1).
- **Cálculo que dura 20 minutos y termina:** se devuelve el resultado y no se menciona el plazo (RF-3).
- **Cálculo que dura 50 minutos y termina:** se devuelve el resultado dentro de los 60 minutos (RF-3).
- **La fase de referencia agota los 55 minutos del solver y encuentra solución:** se presenta la mejor
  solución encontrada, con la indicación de que la **asignación de docentes** no está probada como
  óptima, y con los 5 minutos de reserva para extracción y respuesta; no hay fases blandas (RF-3).
- **La fase de referencia agota los 55 minutos sin solución ni prueba de infactibilidad:** desenlace «no
  concluyente», distinto de NoFactible, con el texto del motor (RF-3, RF-2).
- **Infactibilidad demostrada por el motor:** NoFactible con la causa del catálogo o «no atribuible»
  (RF-2).
- **La fase de referencia termina antes de los 55 minutos:** las fases blandas usan **lo que quede de esos
  55**; no hay un margen aparte y no se tocan los 5 minutos de reserva (RF-3).
- **La fase de referencia se prueba óptima y una fase blanda agota su parte del presupuesto:** el horario
  se presenta y la indicación de no optimalidad se refiere **solo a esa fase**, no a la asignación de
  docentes (RF-3).
- **NoFactible encontrado a los pocos segundos:** se informa la causa sin esperar a la hora; el plazo es
  un máximo (RF-2, RF-3).
- **El proceso no responde dentro de los 60 minutos:** plazo agotado, sin horario y sin afirmar que no
  exista solución (RF-3).
- **Se pierde la comunicación con el proceso antes de los 60 minutos:** desenlace «sin respuesta», con el
  motivo, distinto de una respuesta inválida y de NoFactible; no se presenta como contrato incumplido
  (RF-3).
- **El proceso devuelve una respuesta que no corresponde al contrato:** error de entrada o de contrato
  con su causa; no es NoFactible ni no concluyente (RF-3, RF-2).
- **El motor devuelve un modelo inválido o un error interno:** error de entrada o de contrato con su
  causa; no es NoFactible ni no concluyente (RF-3, RF-2).
- **Prueba automática del plazo:** con un plazo corto inyectado, el desenlace es plazo agotado; no se
  espera la hora real (RNF-2, RF-3).
- **Comprobaciones de salud durante un cálculo largo:** se encolan y se responden al terminar; no se
  consumen intentos, no se declara caída y no se mata el proceso; la verificación sigue o se reanuda en
  estado normal (RNF-4).
- **Los datos cambian durante un cálculo largo:** el resultado se marca como de datos anteriores, como
  hoy; el experimento no añade confirmación ni guardado nuevo (RF-4).
- **Asignación por encima del tope en un resultado:** no es presentable y se informa como fallo de
  validación; el aviso P2, que mide contra horas requeridas, no se usa como prueba del tope (RF-5).

## Fuera de alcance

- **La 019 completa, con una única excepción parcial y declarada:** se implementa **solo el mínimo de H3**
  —que la causa del motor llegue al mensaje de NoFactible en lugar de un texto genérico—. **No** se
  implementan H1, H2, H4–H16 ni el resto de H3 (presentación del diagnóstico en otras pantallas,
  tratamiento de «datos anteriores» con confirmación, aforo en el documento de visualización, rejilla de
  preferencias, esquema v7).
- **El modelo de materias genéricas** y cualquier rediseño de la carga académica.
- **El rendimiento y la descomposición del cálculo** (spec propia): no se optimiza la búsqueda, no se
  descompone la instancia y no se mide el rendimiento como objetivo. El plazo de una hora es una
  concesión, no una mejora.
- **Añadir una categoría nueva de causa ni una validación previa específica de horas.** Si la
  infactibilidad solo se explica por las horas, se informa «no atribuible» con la demanda y la suma de
  topes.
- **Cualquier cambio de esquema, contrato 2.0, presets, huella de datos o datos del centro.**
- **Cambiar el aforo, las reglas de bloques, el ancla estructural o las preferencias.**
- **Revalidar, convertir o rechazar horarios guardados** de la etapa blanda.
- **La relajación selectiva, por docente o automática del tope**, y cualquier escalón de vuelta al
  comportamiento blando.
- **La 017, el seed y el paquete de demostración.**
- **Editar `docs/constitution.md` o las specs 002/005/013/014/018/019.** Solo se propone la
  supersesión al final; aplicarla exige aprobación explícita.
- **Pantallas nuevas o rediseño de las existentes**, salvo los textos e indicios que piden los RF.

## Criterios de finalización

1. En todo horario devuelto, ningún docente supera sus horas de aula; un docente con tope cero
   (declarado o ausente) no recibe clases y el resultado lo informa (RF-1).
2. No existe relajación automática, reintento blando ni configuración que permita superar el tope; el
   resultado de una generación nunca presenta exceso; los horarios guardados no se revalidan (RF-1).
3. Un NoFactible solo se declara cuando el motor demuestra la infactibilidad, y su mensaje conserva el
   diagnóstico del motor (causa o «no atribuible»), no un texto genérico (RF-2).
4. Un NoFactible solo por horas informa «no atribuible» con la demanda total y la suma de los topes, sin
   inventar categoría (RF-2).
5. El plazo es de **60 minutos de reloj**, el presupuesto del solver de **55 minutos de reloj en total**
   (la referencia puede consumirlos; las blandas usan lo que quede) y **5 minutos de reserva** para
   extracción y comunicación que no se usan para fases blandas; el plazo de 300 s deja de aplicarse y el
   sistema anuncia el nuevo plazo (RF-3).
6. Una solución factible al límite se presenta; la no optimalidad se declara **por fase** (la asignación
   de docentes si la referencia no se probó óptima, o la fase blanda correspondiente, sin calificar de no
   óptima la asignación cuando la referencia sí es óptima); un resultado no concluyente se informa como
   tal y distinto de NoFactible; la pérdida de comunicación es «sin respuesta», distinta de una respuesta
   inválida y de NoFactible; la falta de respuesta dentro de los 60 minutos es plazo agotado sin afirmar
   que no exista solución (RF-3).
7. Una respuesta inválida, un modelo inválido o un error interno se informan como error de entrada o de
   contrato con su causa, y no como NoFactible ni como no concluyente (RF-2, RF-3).
8. La prueba automática del plazo usa un plazo corto inyectado y la comprobación con la hora real queda
   como evidencia manual del usuario (RF-3, RNF-2).
9. Las invariantes duras vigentes, el contrato 2.0, los presets, el esquema, la huella y los datos del
   centro quedan intactos (RF-4).
10. El reparto de horas es observable (una línea por docente y una de totales) y coincide con la cuenta
    por docente; una asignación por encima del tope hace el resultado no presentable; el aviso P2 se
    mantiene separado y no se confunde con el tope (RF-5).
11. La validación previa no cambia: la falta de capacidad de horas no rechaza la entrada y la
    infactibilidad por horas aparece al resolver (RF-6).
12. «Línea base» se limita a las pruebas no ligadas al comportamiento sustituido; las pruebas del exceso
    blando, del objetivo, del ancla, de los presupuestos 60/20/10, del plazo de 300 s y del uso de P2 se
    reescriben y eso no es regresión (RNF-1).
13. El cálculo largo no mata el proceso ni se presenta como colapso; si se suspende la verificación, se
    reanuda al terminar la generación (RNF-4).
14. El cambio es reversible, vive en la rama aislada, la spec está commiteada antes de implementar y el
    diff se limita a los archivos del plan (RNF-3, RNF-10).

## Dudas abiertas

**Ninguna.** Las decisiones D-1…D-5 cierran el alcance. La **unidad del plazo queda fijada en tiempo de
reloj** (60 min de proceso, 55 min de solver **en total**, 5 min de reserva fija): ya no se remite al plan
la equivalencia con el tiempo determinista del motor; el plan solo implementa el corte real. La **premisa
del health-check queda corregida**: el cliente no tiene plazo y las comprobaciones se encolan; el
requisito es no matar ni declarar colapso, y reanudar la verificación si se suspende. No quedan
contradicciones de **margen** (una sola regla: 55 min del solver en total y 5 min de reserva), de
**desenlaces** (horario, NoFactible, no concluyente, sin respuesta, plazo agotado y error de
entrada/contrato, todos distintos) ni de **optimalidad** (se declara por fase).

## Supersesiones (aprobadas como propuestas; **no aplicadas**)

El usuario aprobó estas supersesiones **como propuestas** el **2026-11-12**. **Ninguna se ha escrito** en
los documentos citados: la rama es un **experimento aislado** y aplicarlas sería **otra decisión**. Mientras
dure la rama, se leen como **excepciones temporales** del experimento.

| Documento | Lo que dice hoy | Propuesta (no aplicada) |
|---|---|---|
| **005** (RF-5, S1, definiciones de tope y exceso, casos límite) | El exceso es el último recurso, se penaliza y se minimiza; un docente a tope o con tope cero sigue siendo elegible y puede recibir clases. | El tope es **duro**: no hay exceso admisible y un docente con tope cero **no puede** dar clase. **V13 se mantiene intacta** (la falta de capacidad de horas no rechaza la entrada; la infactibilidad por horas aparece al resolver), ahora con carácter normativo en **RF-6**. |
| **018** (RF-4) | «EL SISTEMA no alterará el tope de horas de aula del docente **ni su tratamiento (blando)**: el exceso se paga y se informa, **no se convierte en infactibilidad**.» | **Conflicto declarado:** el experimento lo convierte en invariante dura y **sí** puede producir NoFactible. La aritmética de capacidad y el aforo **no cambian**; la referencia a la penalización del exceso queda histórica. |
| **002** (plazo del proceso de cálculo) y **014b B1b** | El plazo del proceso es de **300 s**. | En el experimento el plazo es de **60 min de reloj** de proceso y **55 min de reloj** de solver **en total**; se propone una nota de supersesión **temporal**, con la vuelta a 300 s al descartar la rama. |
| **014** (presupuestos de fases y 8 workers) | La fase de referencia usa un presupuesto de **60 s**, las blandas de 20 y 10, y **S1** forma parte del objetivo; el motor busca con 8 workers. | **S1 se retira** del objetivo (no hay exceso) y la referencia sube a **55 min de reloj**; 8 workers y el resto de reglas se mantienen. Las fases blandas pasan a **usar solo lo que quede de los 55 min** del solver (la reserva de 5 min no se usa para ellas). |
| **013** (fases 1/2a/2b, ancla estructural y presupuestos) | El **ancla estructural** fija el reparto de docentes (`z*`) **y el exceso por docente (`e*`)**; las fases usan presupuestos 60/20/10. | **El ancla pierde `e*`** porque el exceso deja de existir; conserva solo el reparto de docentes. Las fases 2a/2b **pueden no ejecutarse** si la referencia consume sus 55 minutos. |
| **`docs/constitution.md` §7.2** (y §7.1, §7.5) | El tope de horas es la **prioridad blanda 1** (`PESO_HORAS`); §7.1 no lo incluye entre las invariantes; §7.5 dice que el único aviso vigente es el exceso. | Propuesta: añadir el tope como **invariante dura**, retirar S1 del orden blando y rebajar el aviso de exceso a **dato de coherencia**. §7.4 (V13–V19) **no cambia**. No se edita sin aprobación explícita. |
| **019** (RF-4 y P-5, y H3) | Horas **flexibles**, sin tope fijo, con las 36 h como referencia sembrada y no como máximo. H3 (la causa del fallo llega a la interfaz) es un hallazgo de la 019. | **Conflicto declarado:** la 020 contradice RF-4/P-5. **Excepción parcial acordada:** la 020 incluye **solo el mínimo de H3** (propagar el diagnóstico del motor al mensaje de NoFactible). El resto de la 019 sigue fuera; **no se fusiona la 020** sin reconciliar ambas. |
