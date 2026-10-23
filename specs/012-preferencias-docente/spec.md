# Spec 012 - Preferencias horarias del docente

Estado: aprobada (2026-10-23)

> **Alcance.** Esta spec cubre **persistencia, servicios y exposición** de las preferencias horarias de
> los docentes. **No** toca el solver, ni el contrato de configuración, ni la generación: las
> preferencias se guardan y se exponen para que una spec futura decida cómo usarlas. La interfaz de
> usuario queda **fuera de alcance** (spec 001); se entregan los servicios y la guía de API.
>
> **Decisión del usuario (2026-10-23).** Se **replica el modelo de disponibilidad del docente** para
> almacenar también sus **preferencias de hora**. Cada franja de preferencia se marca como
> **preferida** o **evitada**.
>
> **Dependencia y esquema.** Depende de la **009** (esquema, marca de versión, catálogo, servicios,
> huella y guía). El esquema cambia (registro nuevo), así que se incrementa la marca de versión y se
> aplica la política **sin migración** de la 009: una base con esquema no esperado se rechaza al abrir.
>
> **Supersesión de la 008.** Esta spec supersede **RF-9 de la 008** y su definición de **reemplazo de
> docentes** en lo que toca a las preferencias: la conmutación de variante de docentes ya no puede
> borrar solo disponibilidad y materias; debe borrar también las preferencias antes que los docentes,
> porque las claves foráneas están activas.
>
> **Estado de entrega y verificación.** Dentro: **backend, datos, seed, consola y sus pruebas**, que
> compilan y pasan **sin la interfaz**. Fuera: la interfaz y los tests ligados a ella. La construcción
> debe poder excluir la interfaz (requisito de la 009, RNF-6).

## Contexto y objetivo

El centro ya guarda cuándo **puede** dar clase cada docente (su disponibilidad), pero no **cuándo
prefiere** darla ni qué franjas quiere **evitar**. La disponibilidad es una restricción de factibilidad;
la preferencia es un criterio de calidad que el solver podrá usar más adelante. Sin un sitio donde
guardarla, el usuario no puede expresarla y el equipo del solver no tiene de dónde leerla cuando llegue
el momento.

El objetivo es **replicar el modelo de disponibilidad** —docente, día y rango horario— y añadirle un
**sentido**: preferida o evitada. La spec se limita a persistir y exponer: no decide pesos, ni
prioridades, ni cómo se combinan con la disponibilidad; eso será de una spec del solver.

**Qué no es.** No es una regla de generación, ni un cambio del contrato, ni una pantalla, ni un
calendario de excepciones.

## Usuarios

- **Usuario que mantiene docentes:** registra las preferencias de cada docente junto con su
  disponibilidad y espera que se conserven y se devuelvan ordenadas.
- **Equipo del solver (futuro):** necesitará leer las preferencias; esta spec deja el dato disponible,
  no la regla.
- **Equipo de frontend:** consume los servicios; necesita la API documentada.
- **Equipo de pruebas y QA:** necesita una base coherente y determinista, con el recuento de
  preferencias.
- **Equipo de desarrollo:** necesita que el borrado del docente y la conmutación de variante arrastren
  las preferencias, y que el esquema y la huella sigan la política vigente.

## Historias de usuario

- HU-1. Como usuario que mantiene docentes, quiero marcar franjas como preferidas o evitadas, para
  expresar la preferencia horaria de cada docente.
- HU-2. Como usuario que mantiene docentes, quiero que una misma franja no sea a la vez preferida y
  evitada, para no dejar una contradicción.
- HU-3. Como usuario que consulta docentes, quiero ver sus preferencias ordenadas por día y hora, para
  revisarlas con facilidad.
- HU-4. Como responsable de los datos, quiero que al borrar un docente o cambiar la variante se borren
  también sus preferencias, para no dejar huérfanas ni romper las claves foráneas.
- HU-5. Como miembro del equipo de pruebas, quiero conocer el recuento de preferencias del conjunto
  sembrado, para afirmar sobre los datos.

## Definiciones

- **Preferencia horaria:** registro que indica que un docente **prefiere** o **evita** un rango de
  horas de un día. Tiene docente, día, hora de inicio, hora de fin y **sentido**.
- **Sentido:** **preferida** o **evitada**. No hay otros valores.
- **Día:** entero de **0 a 6**, con **0 = domingo, 1 = lunes, …, 6 = sábado**. Es el mismo rango de la
  disponibilidad. La generación actual usa de lunes a viernes; los días fuera de esa semana se admiten y
  se guardan, pero no intervienen en la generación.
- **Franja de preferencia:** el rango de horas de una preferencia. Expresa una inclinación, no
  factibilidad.
- **Disponibilidad:** los rangos en los que el docente **puede** dar clase (modelo existente). Es un
  dato distinto y no se mezcla con la preferencia.
- **Solape de sentidos opuestos:** dos preferencias del mismo docente y día cuyos rangos se cruzan y una
  es preferida y la otra evitada. Es una contradicción y se rechaza.
- **Duplicado exacto:** dos preferencias del mismo docente, día, hora de inicio, hora de fin y sentido.
  Se rechaza. No implica exclusividad sobre el resto de rangos: los solapes del mismo sentido y los
  rangos contiguos se admiten.
- **Huella de datos:** resumen determinista de los datos que intervienen en una generación (spec 004).

## Requisitos funcionales

### RF-1. Modelo de la preferencia

- **EL SISTEMA persistirá cada preferencia con su docente, su día, su hora de inicio, su hora de fin y
  su sentido.**
- **EL SISTEMA identificará cada preferencia por su identidad propia**, independiente del docente y del
  rango, y las operaciones la usarán para actualizar y eliminar.
- **EL SISTEMA admitirá varias preferencias por docente y día**, en rangos distintos o del mismo
  sentido.
- **CUANDO se consulte un docente, EL SISTEMA devolverá sus preferencias ordenadas por día y, dentro
  del día, por hora de inicio; a igual hora de inicio, por hora de fin ascendente y, si persiste el
  empate, por su identidad.**

### RF-2. Validación

- **SI el docente no existe, ENTONCES EL SISTEMA rechazará el registro o la actualización** indicando la
  causa.
- **SI el día no es un entero entre 0 y 6 (domingo a sábado), ENTONCES EL SISTEMA rechazará la
  operación** con un mensaje que identifique el día y el rango.
- **EL SISTEMA admitirá días fuera de la semana de generación (lunes a viernes):** se guardan y se
  exponen, pero no intervienen en la generación actual; la API lo documenta.
- **SI la hora de inicio no es válida o no es anterior a la hora de fin, ENTONCES EL SISTEMA rechazará
  la operación** indicando el rango.
- **SI el sentido no es «preferida» ni «evitada», ENTONCES EL SISTEMA rechazará la operación.**
- **SI dos preferencias del mismo docente y día se solapan con sentidos opuestos, ENTONCES EL SISTEMA
  rechazará el registro o la actualización** indicando las dos franjas y el solape.
- **SI se registra una preferencia idéntica a otra del mismo docente, día, rango y sentido, ENTONCES EL
  SISTEMA rechazará el duplicado exacto.**
- **EL SISTEMA admitirá solapes entre preferencias del mismo sentido y rangos contiguos:** no se
  contradicen y el consumidor puede unificarlos al presentarlos; el sistema no los fusiona.
- **CUANDO se cambie el sentido de una preferencia, EL SISTEMA validará de nuevo los solapes de sentidos
  opuestos** y rechazará el cambio si aparece una contradicción.

### RF-3. Servicios de preferencias

- **EL SISTEMA ofrecerá operaciones para agregar, listar, actualizar y eliminar preferencias** de un
  docente, identificando la preferencia por su identidad.
- **CUANDO se consulte un docente, EL SISTEMA devolverá sus preferencias junto con su disponibilidad**
  como datos distintos, sin mezclarlas.
- **EL SISTEMA permitirá limpiar todas las preferencias de un docente** como operación propia, sin
  tocar su disponibilidad.
- **SI una operación falla a mitad, ENTONCES EL SISTEMA no dejará cambios parciales.**

### RF-4. Cascada y baja del docente

- **CUANDO se elimine un docente, EL SISTEMA eliminará sus preferencias y su disponibilidad en la misma
  operación atómica**, sin dejar registros huérfanos.
- **SI la baja del docente falla a mitad, ENTONCES EL SISTEMA no borrará ni el docente ni sus datos
  dependientes.**
- **EL SISTEMA incluirá las preferencias en las comprobaciones de dependencias del docente** que
  muestran los datos que se van a borrar antes de confirmar.

### RF-5. Huella de datos

- **MIENTRAS el solver no consuma las preferencias, EL SISTEMA no las incluirá en la huella de datos**:
  un cambio de preferencias no marca un horario como «datos anteriores», porque no intervienen en la
  generación actual.
- **CUANDO una spec del solver incorpore las preferencias a la generación, deberán incluirse en la
  huella**; esta spec deja anotada esa condición y no la implementa.

### RF-6. Esquema, versión y apertura

- **EL SISTEMA incluirá el registro de preferencias en el esquema esperado**, con la referencia al
  docente obligatoria y las restricciones de forma del modelo.
- **CUANDO cambie el esquema esperado, EL SISTEMA incrementará la marca de versión de esquema** y
  actualizará en el mismo cambio el catálogo de tablas y columnas esperadas y la huella de esquema que
  lo vigila, según la política de la 009.
- **SI una base existente no tiene el registro de preferencias, ENTONCES EL SISTEMA tratará la apertura
  como fallo**, sin migrar, sin borrar y con la opción de crear una base nueva con respaldo.
- **EL SISTEMA no creará el registro por migración ni supondrá preferencias en bases anteriores.**

### RF-7. Datos de prueba y conmutación de variante

- **EL SISTEMA no sembrará preferencias por defecto:** el conjunto de prueba queda con **cero**
  preferencias, porque no hay una política decidida y el usuario sustituirá el conjunto por los datos
  reales del liceo.
- **EL SISTEMA informará el recuento de preferencias (0)** en el resultado de la siembra.
- **CUANDO se vacíe o reemplace el conjunto, EL SISTEMA borrará las preferencias antes que los
  docentes**, respetando las claves foráneas activas.
- **CUANDO se conmute la variante de docentes, EL SISTEMA borrará primero las preferencias, la
  disponibilidad y las materias, y después los docentes**, y cargará la variante pedida; esto
  **supersede RF-9 de la 008** en lo que toca a las preferencias.
- **SI la conmutación falla a mitad, ENTONCES EL SISTEMA dejará la variante anterior completa**, sin
  preferencias huérfanas ni docentes a medio borrar.
- **EL SISTEMA mantendrá el determinismo:** dos siembras del mismo perfil dejan el mismo recuento y los
  mismos datos.

### RF-8. Documentación de la API para el frontend

- **EL SISTEMA actualizará la guía de integración del frontend** con las operaciones de preferencias, el
  modelo (docente, día, rango, sentido), el rango de día 0–6 con su significado, las validaciones y los
  errores.
- **La documentación distinguirá preferencia de disponibilidad**, advertirá de que la preferencia no
  altera todavía la generación y de que los días fuera de lunes–viernes se guardan sin efecto.
- **La documentación enumerará los cambios de firma que rompen la compilación del frontend**, si los
  hay.

## Requisitos no funcionales

- **RNF-1. Mensajes en español:** todo rechazo identifica el campo, el valor y la causa, y la acción
  disponible.
- **RNF-2. Protección de datos:** no hay migración automática; una base con esquema no esperado no se
  borra ni se sobrescribe, y las operaciones destructivas conservan el respaldo (constitución §5).
- **RNF-3. Determinismo:** la siembra y el orden de presentación no dependen del reloj, del azar ni del
  orden de inserción.
- **RNF-4. Integridad:** no quedan preferencias huérfanas tras borrar un docente, conmutar la variante o
  vaciar el conjunto; las operaciones son atómicas.
- **RNF-5. Verificación sin la interfaz:** la construcción puede excluir la interfaz y sus pruebas (009,
  RNF-6); con esa configuración, backend, datos, seed, consola y sus pruebas compilan y pasan.
- **RNF-6. Trazabilidad:** cada RF de esta spec tendrá al menos una comprobación verificable en las
  tareas; sin cobertura, la tarea no está hecha.

## Casos límite

- **Día fuera del rango 0–6:** rechazo con el día y el rango (RF-2).
- **Hora de inicio igual o posterior a la de fin:** rechazo (RF-2).
- **Hora inválida:** rechazo (RF-2).
- **Sentido distinto de preferida/evitada:** rechazo (RF-2).
- **Docente inexistente:** rechazo con la causa (RF-2).
- **Dos preferencias idénticas del mismo docente, día, rango y sentido:** rechazo por duplicado exacto
  (RF-2).
- **Dos preferencias del mismo sentido con solape:** se aceptan; no se fusionan (RF-2).
- **Dos preferencias del mismo sentido contiguas:** se aceptan (RF-2).
- **Dos preferencias de sentidos opuestos con solape:** rechazo con las dos franjas (RF-2).
- **Cambio de sentido que crea un solape opuesto:** rechazo; el sentido anterior se conserva (RF-2).
- **Cambio de sentido sin conflicto:** se acepta (RF-2).
- **Preferencia en sábado o domingo (fuera de la semana de generación):** se acepta, se guarda y se
  expone, pero no interviene en la generación (RF-2, RF-8).
- **Docente sin preferencias:** la consulta devuelve una lista vacía, sin error (RF-3).
- **Actualizar o eliminar una preferencia inexistente:** se rechaza indicando que no existe (RF-3).
- **Misma hora de inicio en dos preferencias del mismo día:** se ordenan por hora de fin y, si persiste,
  por identidad (RF-1).
- **Borrado del docente con preferencias y disponibilidad:** se borran todas en una operación atómica
  (RF-4).
- **Conmutación de variante con preferencias:** se borran antes que los docentes, junto con
  disponibilidad y materias; no falla por claves foráneas (RF-7).
- **Fallo a mitad de la baja o de la conmutación:** no se borra nada (RF-4, RF-7).
- **Base anterior sin el registro de preferencias:** fallo de apertura, sin migración (RF-6).
- **Siembra del conjunto:** cero preferencias y recuento 0 (RF-7).
- **Cambio de preferencias durante una generación:** no cambia la huella ni marca el horario como «datos
  anteriores», porque el solver no las consume (RF-5).
- **Preferencia que coincide con una franja de disponibilidad:** se aceptan; son datos distintos
  (RF-3).

## Fuera de alcance

- **El uso de las preferencias por el solver, su peso y su prioridad:** será una spec del solver; esta
  spec solo las persiste y expone.
- **El contrato de configuración y la generación:** no cambian.
- **La interfaz de usuario:** la implementa otro equipo (spec 001); se entregan servicios y guía.
- **La fusión de solapes del mismo sentido:** la hará quien presente los datos, si lo necesita.
- **Preferencias recurrentes, excepciones por fecha o calendarios:** no existen en este modelo.
- **La siembra de preferencias de ejemplo:** el conjunto queda vacío hasta que se decida una política o
  lleguen los datos reales.
- **La migración de bases existentes:** no hay migración (RF-6).

## Criterios de finalización

1. Cada preferencia se persiste con docente, día, rango y sentido, tiene identidad propia y se expone
   ordenada por día, hora de inicio, hora de fin e identidad (RF-1).
2. La validación cubre docente, día 0–6, rango, sentido, solapes de sentidos opuestos y duplicados
   exactos; los solapes y rangos contiguos del mismo sentido se aceptan; el cambio de sentido se
   revalida (RF-2).
3. Existen servicios de alta, consulta, actualización, borrado y limpieza, con la preferencia separada
   de la disponibilidad y sin cambios parciales (RF-3).
4. La baja del docente borra preferencias y disponibilidad de forma atómica y las muestra como
   dependencias (RF-4).
5. Las preferencias no entran en la huella mientras el solver no las consuma, con la condición futura
   anotada (RF-5).
6. El esquema incluye el registro, la marca de versión se incrementó con su catálogo y su huella, y una
   base anterior falla sin migrar (RF-6).
7. El seed deja cero preferencias y lo informa, el vaciado las borra antes que los docentes y la
   conmutación de variante también, sin fallar por claves foráneas (RF-7).
8. La guía de API documenta el modelo, el rango de día, las validaciones, la diferencia con la
   disponibilidad y el efecto nulo en la generación (RF-8).
9. Cada RF tiene al menos una comprobación verificable en las tareas (RNF-6).

## Dudas abiertas

Ninguna. El modelo, el sentido, el rango de día, la política de solapes, la siembra vacía, la
conmutación de variante y la exclusión del solver y del frontend están decididos en esta spec.
