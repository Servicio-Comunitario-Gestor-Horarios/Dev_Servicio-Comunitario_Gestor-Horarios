# Spec 010 - Solver: tipos de aula y eventos del turno

Estado: aprobada (2026-10-23)

> **Alcance y reparto con la spec 009.** Esta spec cubre la parte del **solver**: el **contrato de
> configuración 2.0**, la **regla de tipo de aula** en el modelo, las **validaciones nuevas**, la
> **supersesión de la 005**, la **enmienda de la constitución**, la **documentación del contrato** y el
> **diagrama entidad-relación**. La **persistencia, los servicios, el esquema y su versión, los datos de
> prueba y la documentación de la API** viven en `specs/009-tipos-de-aula-y-eventos/spec.md`. La 009
> **precede** a la 010: esta spec **consume** lo que la 009 persiste y expone (catálogo de tipos, tipos
> de aulas y materias, eventos con identidad y orden, servicios, huella y esquema), y el solver **nunca
> abre la base**.
>
> **Regla de tipo (decisión del 2026-10-23).** La compatibilidad es **dura siempre**, en los dos
> modos: toda clase se imparte en un aula del **tipo exacto** requerido por su materia. El **aula fija
> es el aula de referencia** del curso: se usa cuando su tipo coincide, y cuando no, la clase se
> desplaza a un aula del tipo requerido y el curso **vuelve**. Decisión nueva del usuario: **mientras
> el curso está desplazado, el aula de referencia queda vacía y bloqueada** —ningún otro curso puede
> usarla en esa franja—. La **exclusividad de aula** se valida también en las clases desplazadas. La
> duda sobre compartir la cancha **sigue aparcada y fuera de alcance**.
>
> **Contrato (decisión del 2026-10-23).** El contrato declara y acepta **solo la versión 2.0**. Una
> configuración 1.0, sin versión o con versión no reconocida se **rechaza como entrada inválida**, sin
> conversión automática ni tipos por defecto. Los presets 1.0 guardados en disco no se convierten: el
> rechazo identifica la versión declarada y, cuando la configuración sea un preset guardado en disco,
> el archivo, y dice la acción disponible (generar una 2.0 desde los datos).
>
> **Enmienda de la constitución.** `docs/constitution.md` §7 se enmienda **dentro de esta spec** como
> requisito y **tarea propia** del plan: §7.1.5 para el aula de referencia con desplazamiento, vuelta y
> aula vacía; las validaciones nuevas numeradas como **V16 y V17**; **V1** hablando de eventos y no de
> recesos; y la **versión 2.0** del contrato. La 009 no modifica la constitución.
>
> **Interfaz de usuario fuera de alcance (decisión del 2026-10-23).** La interfaz la implementa otro
> equipo (spec 001); esta spec no toca vistas ni widgets.
>
> **Estado de entrega y verificación.** En esta tanda quedan dentro **backend, datos, seed, consola y
> sus pruebas**, que deben **compilar y pasar**; quedan fuera la **interfaz de usuario** y los tests
> ligados a ella. La construcción debe poder **excluir la interfaz** (requisito de la 009, RNF-6) para
> poder verificar el solver y el backend sin ella; el detalle técnico es del plan.

## Contexto y objetivo

El solver no puede distinguir qué tipo de espacio necesita cada materia: el contrato actual no lleva
el tipo requerido por la materia y toda aula viaja como si fuera del mismo tipo. Además, el modo de
aula fija obliga a que un curso use **siempre** su aula, de modo que una materia que necesita un
laboratorio o una cancha no puede ubicarse donde corresponde. La consecuencia es un horario que puede
ser formalmente válido pero físicamente imposible.

Esta spec define cómo el solver recibe los tipos (de las aulas y de las materias), cómo los usa en el
modelo y cómo cambia el significado del aula fija: pasa a ser el **aula de referencia**, que se usa
para las clases cuyo tipo coincide, se **desplaza** cuando la materia requiere otro tipo y se recupera
al volver. Mientras el curso está desplazado, su aula de referencia queda **vacía y bloqueada**. La
**exclusividad de aula** se mantiene y se extiende a las clases desplazadas.

También fija el **contrato 2.0** (solo esa versión), las **validaciones previas nuevas** y la
**supersesión** de las partes de la 005 que describen el aula fija y la versión 1.0, y exige la
**enmienda de la constitución** y el **diagrama entidad-relación** completo. La persistencia de los
tipos y los eventos es de la 009.

**Qué no es.** No es una migración de datos ni de presets, ni un cambio de las invariantes ajenas al
tipo de aula, ni una reforma del frontend, ni una decisión sobre compartir la cancha.

## Usuarios

- **Usuario académico que genera horarios:** espera que las clases se impartan en el espacio que su
  materia requiere, aunque el curso tenga aula de referencia.
- **Equipo del solver:** necesita un contrato sin ambigüedad, con el tipo de cada aula y el tipo
  requerido por cada materia, y reglas verificables.
- **Equipo de frontend:** necesita que el contrato 2.0 y su rechazo de la versión anterior estén
  documentados en la **documentación del contrato de esta spec (RF-10)**, no en la guía de servicios
  de la 009.
- **Equipo de desarrollo:** necesita que la 005 y la constitución no contradigan el comportamiento
  nuevo y que el modelo de datos completo esté dibujado.

## Historias de usuario

- HU-1. Como usuario académico, quiero que una clase de Educación Física vaya a la cancha y que el
  curso vuelva a su aula después, para que el horario sea físicamente posible.
- HU-2. Como usuario académico, quiero que mi aula no sea ocupada por otro curso mientras el mío está
  en otro espacio, para conservar la referencia de mi grupo.
- HU-3. Como usuario académico, quiero que dos clases nunca compartan un aula a la misma hora,
  incluidas las desplazadas, para no tener conflictos de espacio.
- HU-4. Como miembro del equipo del solver, quiero que el contrato lleve los tipos y declare una
  versión única, para validar la entrada sin suposiciones.
- HU-5. Como miembro del equipo de desarrollo, quiero que la 005 y la constitución reflejen la regla
  nueva, para no tener dos verdades.

## Definiciones

- **Clave de tipo:** forma normalizada del nombre de un tipo de aula (sin espacios extremos y sin
  distinguir mayúsculas). Es el valor sobre el que se evalúa la **igualdad de tipos** **solo para
  comparar**; el nombre visible se conserva tal cual se declaró.
- **Aula de referencia:** el aula que el curso declara y que se usa en el modo de aula fija para toda
  clase cuya materia requiera su mismo tipo. **No** es una restricción que anule el tipo ni significa
  «el curso usa siempre su aula» (esto supersede la definición y la invariante D10 de la 005).
- **Clase desplazada:** clase de un curso con aula de referencia que se imparte en un aula distinta
  porque el tipo requerido por su materia no coincide con el de su aula de referencia.
- **Aula de referencia reservada y vacía:** mientras un curso tiene una clase desplazada en una
  franja, su aula de referencia **no aloja ninguna clase** y **ningún otro curso puede usarla** en esa
  franja. La reserva no se extiende a franjas en las que el curso no tiene clase.
- **Modo de aula fija y curso móvil:** los de la 005; el modo global decide si el curso tiene aula de
  referencia, pero **no** exime del tipo requerido.
- **Contrato 2.0:** la versión del formato de configuración que esta spec produce y acepta; incluye el
  tipo de cada aula, el tipo requerido por cada materia y los eventos con nombre y orden.
- **Causa de un resultado no factible:** las categorías de la 005, en su orden; las validaciones
  nuevas pertenecen a **entrada inválida** y la saturación de aulas a **conflicto de aula**.
- **Eventos del turno:** metadatos del turno que el solver **ignora** para programar; viajan en el
  contrato para que la entrada sea la instantánea completa de los datos.

## Requisitos funcionales

### RF-1. Contrato de configuración 2.0

- **EL SISTEMA declarará y aceptará la versión `2.0` del contrato de configuración del solver.**
- **CUANDO se construya la configuración, EL SISTEMA incluirá el tipo de cada aula y el tipo de aula
  requerido por cada materia**, tomados de los datos persistidos, **sin valores por defecto**; si un
  aula o una materia carece de tipo, la construcción fallará en lugar de suponerlo.
- **EL SISTEMA identificará cada tipo por el nombre del catálogo y evaluará la igualdad sobre la clave
  de tipo:** dos referencias que solo difieren en mayúsculas o espacios extremos son el mismo tipo. La
  normalización es **solo para comparar**: el nombre del tipo se conserva tal cual se declaró y no se
  reescribe al cargar.
- **EL SISTEMA representará en la configuración los eventos de cada turno con su nombre, posición,
  orden, duración y horario opcional**, en lugar de una sección de recesos. **La identidad del evento
  no viaja en el contrato ni forma parte de la equivalencia:** los eventos se distinguen por nombre,
  posición y orden.
- **EL SISTEMA conservará la equivalencia de ida y vuelta:** guardar y volver a cargar una
  configuración reproduce los tipos de aulas y materias y los eventos sin pérdida; la comparación de
  tipos se hace sobre la clave y el nombre visible se conserva tal cual. Los campos desconocidos se
  siguen tolerando (spec 005).
- **EL SISTEMA construirá la configuración sin consultar la base de datos** desde el solver (spec
  005): los tipos y los eventos viajan en la entrada.

### RF-2. Rechazo de configuraciones anteriores

- **SI la configuración declara una versión ausente, `1.0` o no reconocida, ENTONCES EL SISTEMA la
  rechazará como entrada inválida**, sin reinterpretarla, sin convertirla y sin completar tipos
  ausentes.
- **CUANDO rechace una configuración por versión, EL SISTEMA identificará la versión declarada y, si
  la configuración proviene de un preset guardado en disco, el archivo**; la acción disponible será
  **generar una configuración 2.0 desde los datos** con la versión actual. No se ofrece «abrir con una
  versión compatible»: la única versión aceptada es la 2.0.
- **EL SISTEMA no modificará ni convertirá los presets 1.0 guardados en disco:** quedan como están y
  se rechazan al intentar usarlos.
- **EL SISTEMA no aceptará la versión `1.0` con valores por defecto** ni tratará los tipos ausentes
  como un tipo comodín.

### RF-3. Regla de tipo de aula en el modelo

- **EL SISTEMA exigirá que toda clase se imparta en un aula cuya clave de tipo coincida exactamente
  con la clave del tipo requerido por su materia**, tanto en el modo de aula fija como en el de aulas
  móviles.
- **MIENTRAS el modo de aula fija esté activo y el tipo del aula de referencia coincida con el
  requerido por la materia, EL SISTEMA impartirá esa clase en el aula de referencia** y no la
  desplazará a otra aula del mismo tipo.
- **SI el tipo del aula de referencia no coincide con el requerido, ENTONCES EL SISTEMA impartirá esa
  clase en un aula del tipo requerido elegida por el solver** y usará de nuevo el aula de referencia
  en las clases cuyo tipo coincida: el curso **vuelve** y puede tener clases en aulas distintas dentro
  del mismo día.
- **MIENTRAS un curso tenga una clase desplazada en una franja, EL SISTEMA mantendrá su aula de
  referencia reservada y vacía en esa franja:** no alojará ninguna clase en ella, ni del propio curso
  ni de otro, y la reserva no se extiende a franjas sin clase del curso.
- **MIENTRAS el modo de aula fija esté inactivo, EL SISTEMA elegirá para cada clase un aula del tipo
  requerido por su materia.**
- **EL SISTEMA garantizará la exclusividad de aula:** una aula no aloja dos clases a la vez en la
  misma franja, y la regla cubre también las **clases desplazadas**.
- **SI dos cursos necesitan el mismo tipo de aula en la misma franja y no hay aulas de ese tipo
  suficientes, ENTONCES EL SISTEMA no rechazará la entrada:** el solver buscará ubicar las clases en
  franjas distintas respetando la exclusividad y, si no existe solución, el resultado será **no
  factible con causa conflicto de aula** (RF-6).
- **EL SISTEMA no aplicará jerarquías ni tipos comodín:** la compatibilidad es igualdad exacta sobre
  la clave de tipo.
- **EL SISTEMA mantendrá la validación de aula de referencia** de la 005 (V8): con el modo activo, el
  curso debe declarar un aula válida, aunque no aloje todas sus clases.

### RF-4. Aula de referencia en la entrada y en la salida

- **CUANDO se arme la entrada con el modo de aula fija activo, EL SISTEMA traducirá el aula de
  referencia del curso a su índice de aula**, como en la 005.
- **CUANDO se extraiga el horario o se exporte, EL SISTEMA informará el aula real de cada clase como
  su índice de aula**, también en el modo de aula fija; una clase desplazada informa el aula donde se
  imparte, no el aula de referencia.
- **EL SISTEMA no informará el aula de referencia como el aula de todas las clases de un curso con
  modo de aula fija activo:** el curso puede tener clases en aulas distintas.
- **EL SISTEMA seguirá identificando cada curso en la salida por su identidad (año y sección)** y
  agrupando las clases bajo esa clave (spec 005).
- **La traducción de índice de aula a nombre no es del solver:** la resuelve quien muestra (fuera de
  alcance).

### RF-5. Validación previa (V16 y V17)

- **MIENTRAS se prepare una generación, EL SISTEMA ejecutará las validaciones de tipo junto con las
  V1–V15 de la spec 005, antes de invocar al solver; si alguna falla, no invocará al solver.** Las
  nuevas se comprueban a continuación de V15 y se informan en ese orden.
- **V1 (actualizada):** el JSON es parseable y contiene las secciones requeridas, **incluida la de
  eventos**; la sección de recesos ya no es la esperada.
- **V2 (ampliada y alineada con la 009):** cada sección y campo tiene el tipo esperado; los eventos
  declaran **nombre no vacío**, **turno existente**, **posición entre 1 y el número de franjas**,
  **orden mayor o igual que 1** y **duración mayor que 0**; el par inicio/fin está completo o ausente,
  el fin es posterior al inicio, la diferencia coincide con la duración, el intervalo cabe en la
  ventana del turno y en la de su posición, el orden es cronológico y no hay solapes dentro de la
  posición ni entre posiciones.
- **V16 → SI alguna aula o alguna materia de la configuración no declara un tipo no vacío, ENTONCES EL
  SISTEMA rechazará la entrada como entrada inválida**, indicando la entidad.
- **V17 → SI alguna materia de un curso requiere un tipo para el que no hay ninguna aula en el
  centro, ENTONCES EL SISTEMA rechazará la entrada como entrada inválida**, indicando el tipo, la
  materia y el curso; la regla vale en los dos modos de aula.
- **EL SISTEMA no rechazará la entrada por un tipo declarado que ninguna materia requiera**, siempre
  que exista como tipo de aula válido.

### RF-6. Causas de un resultado no factible

- **EL SISTEMA mantendrá las categorías y el orden de causas de la 005:** (1) entrada inválida,
  (2) conflicto de aula, (3) conflicto de persona, (4) conflicto de asignación de docente.
- **SI la falta de un tipo o de un aula de ese tipo impide construir o validar la entrada, ENTONCES la
  causa será entrada inválida** (V16, V17).
- **SI no existe una solución factible y el bloqueo es de aulas —exclusividad o falta de
  disponibilidad de aulas del tipo requerido—, ENTONCES EL SISTEMA informará el resultado como no
  factible con causa conflicto de aula**, incluida la saturación de aulas especializadas por varios
  cursos desplazados.
- **EL SISTEMA no completará ni corregirá la entrada:** no inventará tipos, no reasignará aulas fuera
  del tipo requerido y no liberará la reserva del aula de referencia para hacer factible el horario.

### RF-7. Supersesión de la spec 005

Esta spec **supersede** los siguientes puntos de la 005; el resto de la 005 sigue vigente:

- **Definición «Curso con aula fija»** («usa siempre su aula»): sustituida por **aula de referencia**
  con desplazamiento y vuelta (RF-3).
- **Invariante D10** («un curso con aula fija usa siempre su aula»): sustituida por la regla de tipo
  (RF-3).
- **Criterio de finalización 2 de la 005** (005:745-748, «el modelo respeta D1–D11 y D13»): se mantiene
  **salvo D10**, que queda supersedida; el resto de las invariantes sigue vigente.
- **Salida del horario por curso** («con aula fija, su aula fija»): sustituida por el **aula real de
  cada clase** (RF-4).
- **RF-7 de la 005** (modo de aula fija activo ⇒ cada curso usa su aula): sustituido por RF-3 y RF-4.
- **Versión del contrato `1.0`** (definición, RF-10 y criterio de finalización): sustituida por la
  versión **2.0** y su rechazo (RF-1, RF-2).
- **V1 de la 005** (sección de recesos): sustituida por la sección de **eventos** (RF-5).
- **Actualización del texto de la 005:** su V1 pasa a exigir la sección de **eventos** y su catálogo de
  validaciones pasa a **V1–V17**; esta actualización acompaña a la supersesión y es la referencia que
  cita RF-8. La anotación la hace el planner en la fase de plan.
- **D9 se amplía:** todo curso móvil ocupa exactamente un aula por clase, ahora del tipo requerido.
- **D8 se extiende:** la exclusividad de aula cubre también las clases desplazadas.
- **V8 se mantiene:** el aula de referencia sigue siendo obligatoria y válida con el modo activo.

### RF-8. Enmienda de la constitución

- **EL SISTEMA se mantendrá coherente con `docs/constitution.md`:** la sección §7 y sus dominios
  protegidos se enmendarán en esta entrega para que el contrato normativo no contradiga la regla
  nueva.
- **§7.1.5 (línea 64, invariante del aula):** se reescribe para describir el **aula de referencia**, el
  **desplazamiento** a un aula del tipo requerido, la **vuelta** al aula de referencia y el **aula
  vacía y bloqueada** mientras el curso está desplazado, junto con la exclusividad de aula.
- **§7.3 (línea 78, contrato de configuración):** el modo de aula fija deja de significar «cada curso
  usa su aula fija» y pasa a describir el aula de referencia con desplazamiento; la versión del
  contrato pasa a **2.0** y el tipo de aulas y materias forma parte de la entrada.
- **§7.4 (línea 87, validación previa):** el rango **V1–V15** pasa a **V1–V17**. La constitución **no
  define el contenido de V1**; solo lo menciona dentro del rango. La enmienda actualiza el rango y
  deja constancia de que **V1 se refiere a la sección de eventos**; el texto detallado de V1 y de las
  validaciones nuevas se actualiza en la **005** (RF-7), sin duplicarlo en la constitución.
- **§7.5 (línea 91, fuera de alcance):** «el receso es metadata que el solver ignora» pasa a «los
  **eventos** son metadata que el solver ignora», coherente con el modelo nuevo.
- **§5 (línea 34, protección de los datos del usuario):** los dominios protegidos sustituyen
  «recesos» por **eventos** e incorporan el **catálogo de tipos de aula**.
- **Las validaciones nuevas se numeran V16 y V17**, y la versión del contrato queda fijada en **2.0**.
- **La enmienda es una tarea propia de esta spec**, no un efecto colateral: sin ella, la 005 y la
  constitución seguirían describiendo el comportamiento anterior.

### RF-9. Diagrama entidad-relación completo

- **EL SISTEMA actualizará el diagrama entidad-relación para que refleje el modelo completo:** las
  aulas, los profesores, las materias, la disponibilidad, la relación profesor-materia, los planes de
  estudio, la relación plan-materia, los **turnos**, los **cursos**, el **catálogo de tipos de aula** y
  los **eventos** con nombre, posición y orden.
- **El diagrama mostrará las referencias de tipo de aulas y materias y las claves foráneas de cursos y
  eventos**, de modo que se pueda leer la estructura persistida sin consultar el código.
- **El requisito no es «quitar recesos»:** el diagrama actual **no muestra** turnos, cursos ni
  recesos; debe incorporarlos todos, con el catálogo y los eventos del modelo nuevo.

### RF-10. Documentación del contrato 2.0 y de la semántica del aula de referencia

- **EL SISTEMA actualizará la documentación viva del contrato del solver** (distinta de la guía de
  integración de servicios de la 009) con la **forma del contrato 2.0**, la versión única aceptada, la
  representación de los tipos (nombre del catálogo y clave de comparación), la sección de **eventos**,
  la **salida con el aula real de cada clase** y el **rechazo de 1.0**.
- **La documentación describirá la semántica nueva del modo de aula fija:** el aula de referencia se
  usa para las clases de su tipo, las demás se desplazan y el curso vuelve, y mientras está desplazado
  el aula queda vacía y bloqueada. Advertirá de que **ya no significa «el curso siempre en su aula»**.
- **La documentación será la referencia para leer y escribir presets 2.0** y no duplicará la guía de
  servicios de la 009, que cubre la API del frontend.

## Requisitos no funcionales

- **RNF-1. Reproducibilidad:** bajo el mismo contrato 2.0 y el mismo solver (versión y parámetros,
  incluida la semilla), el resultado es siempre el mismo; no se exige desempatar entre candidatos
  equivalentes (RNF-1 de la 005).
- **RNF-2. Desempeño:** el escenario de gran tamaño del banco de pruebas sigue resolviéndose en menos
  de 60 segundos en el entorno de referencia del proyecto, con la regla de tipo incorporada (RNF-2 de
  la 005).
- **RNF-3. Mensajes en español:** todo rechazo o fallo de factibilidad identifica la causa (categoría y
  entidad cuando sea posible) y, cuando exista, la acción disponible; el rechazo por versión identifica
  la versión declarada y, cuando la configuración provenga de un preset en disco, el archivo.
- **RNF-4. Sin regresión fuera de la regla de tipo:** las invariantes, la función objetivo y las
  validaciones V1–V15 de la 005 se mantienen **salvo** la redefinición del aula fija, la ampliación de
  D9/D8, la versión 2.0 y V1; el solver sigue ignorando los eventos.
- **RNF-5. Verificación sin la interfaz:** la construcción debe poder excluir la interfaz de usuario y
  sus pruebas (requisito de la 009, RNF-6); con esa configuración, **backend, datos, seed, consola y
  sus pruebas compilan y pasan**, y la política de pruebas del proyecto se cumple con esas piezas.
- **RNF-6. Trazabilidad:** cada RF de esta spec tendrá al menos una comprobación verificable en las
  tareas; sin cobertura, la tarea no está hecha.

## Casos límite

- **Curso con aula de referencia del mismo tipo que requiere la materia:** la clase se imparte en el
  aula de referencia; el curso no se mueve (RF-3).
- **Curso con aula de referencia de tipo distinto al requerido por una de sus materias:** la clase se
  imparte en un aula del tipo requerido y el curso vuelve a su aula para las demás (RF-3).
- **Curso desplazado a la cancha:** mientras está fuera, su aula de referencia queda **vacía y
  bloqueada**; otro curso no puede usarla en esa franja (RF-3).
- **Otro curso que necesita un aula del tipo del aula de referencia bloqueada:** el solver lo ubica en
  otra aula o en otra franja; no usa la bloqueada (RF-3).
- **Curso sin clase en una franja:** su aula de referencia no está reservada por esa ausencia (RF-3).
- **Dos cursos desplazados al mismo tipo en la misma franja:** no se rechaza la entrada; el solver los
  ubica en aulas distintas o en franjas distintas; si no existe solución que respete la exclusividad,
  el resultado es no factible con causa conflicto de aula (RF-3, RF-6).
- **Saturación de un laboratorio único por varios cursos:** el solver busca franjas distintas; si no
  hay solución, conflicto de aula, nunca reasignación a un aula de otro tipo (RF-3, RF-6).
- **Curso móvil con materias de varios tipos y aulas de cada tipo:** el solver ubica cada clase en un
  aula del tipo exacto (RF-3).
- **Contrato 1.0, sin versión o con versión desconocida:** rechazo como entrada inválida,
  identificando la versión y, si es un preset en disco, el archivo (RF-2).
- **Preset 1.0 guardado en disco:** no se convierte ni se modifica; se rechaza al usarlo (RF-2).
- **Contrato 2.0 con un evento sin turno existente, con posición 0 o mayor que las franjas, con orden
  menor que 1 o con un par inicio/fin incoherente:** rechazo por V2 (RF-5).
- **Contrato 2.0 con dos eventos solapados en la misma posición o en posiciones distintas:** rechazo
  por V2, alineado con la coherencia temporal de la 009 (RF-5).
- **Contrato 2.0 con un aula o una materia sin tipo:** rechazo como entrada inválida (RF-5, V16).
- **Contrato 2.0 con una materia que requiere un tipo sin aulas en el centro:** rechazo como entrada
  inválida, en los dos modos (RF-5, V17).
- **Contrato 2.0 con dos tipos que solo difieren en mayúsculas:** se consideran el mismo tipo por su
  clave; no se rechaza por esa diferencia (RF-1).
- **Contrato 2.0 con un tipo declarado en un aula que ninguna materia requiere:** se acepta si el tipo
  es válido (RF-5).
- **Eventos con varios por franja:** viajan en el contrato con su orden y el solver los ignora para
  programar (RF-1, RNF-4).
- **Salida con un curso que tuvo clases desplazadas:** cada clase informa el aula real donde se
  impartió, no el aula de referencia (RF-4).
- **Modo de aula fija activo y curso sin aula de referencia válida:** la entrada se rechaza (V8, spec
  005) (RF-3).
- **Cancha compartida por dos cursos a la vez:** no se admite; la cancha sigue la exclusividad como
  cualquier aula (duda diferida, fuera de alcance) (RF-3).

## Fuera de alcance

- **La persistencia, los servicios, el esquema y su versión, los datos de prueba y la documentación de
  la API:** son de la spec 009.
- **El uso compartido de la cancha:** queda **diferido**; se rige por la exclusividad como cualquier
  aula. Si se decide compartirla, se abre un cambio de requisitos.
- **La interfaz de usuario y el frontend:** los implementa otro equipo (spec 001); esta spec no toca
  vistas ni widgets.
- **La migración de bases de datos y la conversión de presets 1.0:** no existen; los presets antiguos
  se rechazan.
- **La capacidad de aula y el aforo:** siguen fuera de las reglas del solver (spec 005).
- **Que el solver programe los eventos, los reste de la rejilla o los use para calcular franjas
  útiles:** siguen siendo metadatos que el solver ignora.
- **El cambio del modelo de turnos, la asignación automática de turno y la generación parcial** (specs
  006 y 007).
- **Las jerarquías, los tipos comodín o las equivalencias entre tipos:** la compatibilidad es igualdad
  exacta sobre la clave de tipo.

## Criterios de finalización

1. El contrato 2.0 lleva el tipo de cada aula y el tipo requerido por cada materia, identifica los
   tipos por su nombre y compara sobre la clave de tipo, transporta los eventos con nombre, posición y
   orden (sin su identidad) y conserva la ida y vuelta (RF-1).
2. Una configuración 1.0, sin versión o desconocida se rechaza identificando la versión y, si es un
   preset en disco, el archivo; la acción es generar una 2.0; los presets antiguos no se convierten
   (RF-2).
3. Toda clase se imparte en un aula del tipo exacto requerido en ambos modos; con aula de referencia,
   las clases de otro tipo se desplazan y el curso vuelve; el aula de referencia queda vacía y
   bloqueada durante el desplazamiento; la exclusividad cubre las clases desplazadas (RF-3).
4. La salida informa el aula real de cada clase, también en modo de aula fija, y no el aula de
   referencia como aula de todas las clases (RF-4).
5. La validación previa incluye V16 y V17, V1 habla de eventos y V2 valida su forma y su coherencia
   temporal; la competición por el mismo tipo no se rechaza antes de resolver (RF-5).
6. Las causas de no factibilidad mantienen las categorías: tipo ausente o sin aulas es entrada
   inválida; la saturación por exclusividad es conflicto de aula; el sistema no corrige la entrada
   (RF-6).
7. La 005 queda supersedida en la definición de aula fija, D10, el criterio 2, la salida por curso,
   RF-7, la versión 1.0 y V1; D9 se amplía, D8 se extiende y V8 se mantiene (RF-7).
8. La constitución queda enmendada en §7.1.5, §7.3, §7.4 (V1–V17 y aclaración de V1), §7.5 y §5, con
   V16 y V17 y la versión 2.0, como tarea propia (RF-8).
9. El diagrama entidad-relación refleja turnos, cursos, catálogo de tipos y eventos, además del resto
   del modelo (RF-9).
10. La documentación viva del contrato describe la forma 2.0, los tipos, los eventos, la salida con
    aula real y la semántica del aula de referencia, sin duplicar la guía de servicios de la 009
    (RF-10).
11. La construcción puede excluir la interfaz y, con esa configuración, backend, datos, seed, consola y
    sus pruebas compilan y pasan (RNF-5).
12. Cada RF tiene al menos una comprobación verificable en las tareas (RNF-6).

## Dudas abiertas

Ninguna. Queda una **duda diferida fuera de alcance**: si la cancha debería ser un espacio compartido.
El usuario pidió dejarlo como está (exclusividad) y abrir un cambio de requisitos si más adelante
decide compartirla.
