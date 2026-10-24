# Spec 009 - Persistencia y servicios de tipos de aula y eventos

Estado: implementada (2026-10-24)

> **Decisión del usuario (2026-10-23): la clave normalizada se almacena.** El catálogo de tipos de
> aula guarda la **clave normalizada** (recorte de espacios extremos + plegado de mayúsculas/minúsculas
> en Unicode con `toCaseFolded()`), **calculada por el servicio**, y el **índice único va sobre esa
> clave**. Motivo: `lower()`/`trim()` de SQLite solo son ASCII y no frenan «Física»/«FÍSICA» en
> escrituras directas. Corrige la definición de «clave de tipo» y RF-5; la spec quedó **aprobada**
> con ese contenido.

> **Alcance y reparto con la spec 010.** Esta spec cubre la **persistencia, los servicios, la huella de
> datos, el esquema y su versión, los datos de prueba y la documentación de la API** del cambio. La
> parte del **solver** —contrato 2.0, regla de tipo de aula, validaciones nuevas, supersesión de la
> 005, enmienda de la constitución y diagrama entidad-relación— vive en
> `specs/010-solver-tipos-y-eventos/spec.md`. La frontera se mantiene: esta spec **persiste y expone**;
> la 010 **lee y arma** la entrada del solver, y el solver **nunca abre la base**.
>
> **Eventos (decisión del 2026-10-23).** El registro de recesos se generaliza a **eventos del turno**
> (recesos, actos cívicos y sucesos análogos) con **nombre**. Puede haber **varios eventos tras la
> misma franja**, cada uno con **identidad propia**; que además lleven un **orden persistente** es una
> **consecuencia derivada** de que la presentación debe ser estable, no una decisión literal del
> usuario. Esta parte se queda en la 009.
>
> **Interfaz de usuario fuera de alcance (decisión del 2026-10-23).** La spec **001** ya declara la
> interfaz de usuario fuera de alcance y entrega la lógica y el contrato de datos que consume; esta
> spec mantiene esa frontera. En su lugar, exige **documentar la API de servicios para el equipo de
> frontend**. Las **firmas cambian en limpio**, sin capas de compatibilidad ni sobrecargas
> transitorias; se acepta como **condición conocida de esta tanda** que la interfaz no compilará hasta
> que su equipo adapte las llamadas. **No es un defecto de la entrega.** La heurística de la interfaz
> que deduce el tipo de aula por el nombre o por el texto libre de requisitos pasa a ser **deuda
> documentada del equipo de frontend**.
>
> **La consola no es frontend.** La consola de datos forma parte de nuestro lado: sus comandos de alta
> y baja de recesos se actualizan aquí para operar sobre eventos con nombre e identidad.
>
> **Datos de prueba (decisión del 2026-10-23).** El sembrador debe **seguir aplicándose** de forma
> determinista con tipos y eventos, y sus recuentos deben ser coherentes; pero **no** se exige que el
> conjunto sembrado sea generable con la regla dura de tipo de aula. Eso **supersede la promesa de
> generabilidad de la spec 008** en lo que respecta al tipo; el usuario anunció que más adelante
> sustituirá el conjunto por los **datos reales del liceo**. Esta spec no inventa aulas nuevas.
>
> **Supersesión de la 008 y frontera con la 001.** Esta spec **supersede de la 008**: la noción de
> **receso** en los turnos, los **recuentos por entidad** de su RF-12 (008:429-431, donde «recesos» pasa
> a «eventos»), su **RF-9** (008:390, que deja intactos «turnos y recesos») y sus **criterios 1 y 3**
> (008, criterios con «2 turnos con su receso» y «receso de 10 minutos tras la franja 2»). Se **apoya
> en la frontera de la 001** (001:132), que declara la interfaz de usuario fuera de alcance y entrega
> solo el contrato de datos que consume.
>
> **Estado de entrega y verificación.** En esta tanda quedan dentro: **backend, datos, seed, consola y
> sus pruebas**, que deben **compilar y pasar**. Quedan fuera: la **interfaz de usuario** y los tests
> ligados a ella. La construcción del proyecto debe **poder excluir la interfaz** para que la política
> de pruebas del proyecto se pueda cumplir con backend/datos/seed/consola; el detalle técnico
> pertenece al plan.
>
> **Puente de la entrada del solver.** La construcción de la entrada es backend y hoy consume los
> recesos del turno para emitir la sección de recesos del contrato 1.0. Hasta que la 010 la sustituya,
> esta spec **mantiene esa sección derivada de los eventos**, para que el backend y sus pruebas sigan
> compilando y pasando; **no** añade tipos ni eventos al contrato. La 010 elimina el puente.
>
> **Sin migración automática (vigente).** El proyecto no migra esquemas: una base con un esquema que
> no es el esperado se **rechaza al abrir** y el usuario decide (crear una base nueva con respaldo,
> restaurar una copia o salir). Por eso esta spec **no** exige convertir datos de una base anterior.
>
> **La constitución no se toca aquí.** La enmienda de `docs/constitution.md` §7 es requisito y tarea
> de la spec **010**. La 009 **precede** a la 010: la 010 consume lo que esta spec persiste y expone.

## Contexto y objetivo

Hoy el centro solo puede declarar **recesos**, y el registro no tiene nombre: no puede representar más
que «el receso» de una posición. Un centro real tiene otros sucesos con la misma forma —un **acto
cívico**, una izada de bandera, una pausa activa— que ocupan tiempo dentro del turno y que el usuario
necesita registrar y ver. **Generalizar el registro a eventos con nombre**, con identidad propia y
varios por franja, permite describir el turno real sin inventar un registro nuevo para cada suceso.

En paralelo, el sistema **no sabe qué tipo de espacio necesita cada materia**. La interfaz lo adivina
por el **nombre del aula** y por un **texto libre de requisitos**, y el solver recibe todas las aulas
como si fueran del mismo tipo. El objetivo de esta spec es convertir esa intuición en **datos
explícitos y persistidos**: un **catálogo de tipos de aula**, un **tipo por aula** y un **tipo
requerido por materia**, junto con los **eventos con nombre**. Cómo los usa el solver es de la 010.

El cambio llega con el **esquema nuevo ya escrito pero no integrado**: la marca de versión no se ha
subido, los servicios siguen consultando el registro de recesos, los registros de aula y materia no
rellenan el tipo obligatorio y los datos de prueba fallan. Sin corregirlo, **una base nueva se crea y
no se vuelve a abrir**, y el sembrador no termina. Por eso esta spec exige dejar coherentes el
esquema, los servicios, la huella, los datos de prueba y la documentación de la API.

**Qué no es.** No es un cambio del modelo del solver (010), ni una migración de bases existentes, ni
una pantalla de administración del catálogo, ni una reforma del frontend, ni una sustitución de los
datos de prueba por los reales.

## Usuarios

- **Usuario académico:** registra los sucesos del turno con su nombre y su horario, y espera verlos
  tal cual.
- **Usuario que mantiene aulas y materias:** asigna a cada aula y a cada materia su tipo de espacio y
  espera que el sistema lo recuerde, sin adivinar por el nombre.
- **Equipo de frontend:** consume la API de servicios; necesita una documentación exacta de las
  firmas nuevas y de las que cambian.
- **Equipo de pruebas y QA:** necesita datos de prueba que incluyan tipos y eventos, con recuentos
  conocidos y una base que se pueda abrir y volver a abrir.
- **Equipo de desarrollo:** necesita que esquema, servicios, consola y documentación cuenten la misma
  historia, sin restos del registro de recesos.

## Historias de usuario

- HU-1. Como usuario académico, quiero registrar varios sucesos del turno con un nombre (receso,
  acto cívico…), para describir el turno real y no solo el receso.
- HU-2. Como usuario que mantiene aulas, quiero asignar un tipo a cada aula, para que el sistema sepa
  qué espacio es y no lo deduzca de su nombre.
- HU-3. Como usuario que mantiene materias, quiero declarar qué tipo de aula requiere cada materia,
  para que el sistema lo use al ubicar las clases.
- HU-4. Como miembro del equipo de frontend, quiero una API documentada con las firmas y los errores
  nuevos, para adaptar la interfaz sin adivinar.
- HU-5. Como miembro del equipo de pruebas, quiero una base nueva que se abra de nuevo y un conjunto
  de datos de prueba con tipos y eventos, para probar el sistema completo.
- HU-6. Como miembro del equipo de desarrollo, quiero que ningún servicio ni consulta siga hablando
  de recesos, para no arrastrar dos modelos a la vez.

## Definiciones

- **Evento del turno:** suceso dentro de un turno (receso, acto cívico, izada de bandera, pausa
  activa y análogos). Tiene **identidad propia**, **nombre**, **turno**, **posición**, **orden dentro
  de la posición**, **duración** y, opcionalmente, **hora de inicio y fin**. No es una clase: no ocupa
  aula ni docente.
- **Posición del evento:** número entero entre **1 y el número de franjas del turno**. La posición **k**
  significa **después de la k-ésima franja de clase** (convención 1-based de la 008); el máximo es el
  número de franjas (después de la última). Esta definición **cierra la ambigüedad 0-based/1-based**
  que dejó abierta la spec 008 y descarta la posición 0: no hay tiempo antes de la primera franja.
- **Orden del evento:** entero mayor o igual que 1, único dentro de la misma posición y turno;
  determina en qué orden se presentan los eventos que comparten franja.
- **Ventana de una posición:** intervalo de la línea de tiempo del turno comprendido entre el fin de
  la franja de clase k y el inicio de la franja de clase k+1; para la última posición, entre el fin de
  la última franja y el fin del turno. Esa línea de tiempo es la del turno **con los eventos
  insertados** (la misma de la estimación de duración), no la rejilla aritmética que usa el solver
  (spec 005), que ignora los eventos. Los eventos de una posición se sitúan en su ventana, de modo que
  las ventanas de posiciones distintas son **disjuntas**.
- **Duración estimada de la franja de clase:** resultado de restar de la ventana del turno la **suma
  de las duraciones de todos los eventos** y dividir entre el número de franjas; fija el ancho de las
  franjas de clase en la línea de tiempo que usan las ventanas de posición. Como no se admiten
  solapes, la suma coincide con la unión de los intervalos. Es una estimación de presentación y **no**
  altera la rejilla aritmética que usa el solver (spec 005).
- **Receso:** evento cuya finalidad es la pausa entre clases; un caso particular de evento. El solver
  lo ignora (spec 010).
- **Tipo de evento:** **no existe como dato**. El sistema no clasifica los eventos en un enumerado ni
  en un catálogo: el **nombre** describe el suceso. Ninguna regla distingue unos eventos de otros.
- **Tipo de aula:** categoría de espacio del centro («Aula de estudio», «Laboratorio», «Taller»,
  «Cancha»…), identificada por su nombre y mantenida en un **catálogo**.
- **Clave de tipo:** forma normalizada del nombre de un tipo (sin espacios extremos y sin distinguir
  mayúsculas en Unicode) que se usa para la **unicidad** y para comparar tipos. **El catálogo guarda
  la clave normalizada, calculada por el servicio**, y el **índice único va sobre ella**; el nombre
  visible se conserva tal como lo escribió el usuario.
- **Catálogo de tipos de aula:** conjunto de tipos disponibles. Es **administrable** (crear, listar,
  renombrar, eliminar) y **no** puede quedar vacío mientras existan aulas o materias.
- **Tipo de un aula:** **exactamente uno**, obligatorio. No hay aulas sin tipo.
- **Tipo requerido por una materia:** **exactamente uno**, obligatorio. No hay materias sin tipo
  requerido.
- **Aula del curso:** referencia opcional a un aula (spec 004). Su papel como **aula de referencia**
  en el cálculo es de la spec 010; aquí solo se persiste la referencia.
- **Esquema esperado:** el conjunto de tablas y columnas que la aplicación reconoce, más la marca de
  versión de esquema. Un esquema distinto es un **fallo de apertura**, no una migración.
- **Huella de datos:** resumen determinista de los datos persistidos que intervienen en una
  generación (spec 004); ahora incluye también los tipos de aula y los eventos.
- **Generabilidad del conjunto de prueba:** propiedad de que el conjunto sembrado permita una
  generación factible. Esta spec **no la exige** (supersede la 008 en ese punto): exige coherencia de
  datos y de recuentos.

## Requisitos funcionales

### RF-1. Eventos del turno

- **EL SISTEMA representará todo suceso del turno —receso, acto cívico o análogo— como un evento con
  nombre, turno, posición, duración e, opcionalmente, hora de inicio y fin.**
- **CUANDO se registre o actualice un evento, EL SISTEMA exigirá:** un nombre **no vacío** tras
  recortar espacios, un **turno existente**, una **posición entre 1 y el número de franjas del
  turno** y una **duración mayor que 0**; si falta o no se cumple alguno, rechazará la operación
  indicando el campo y el valor.
- **EL SISTEMA interpretará la posición con la convención definida:** la posición k significa después
  de la k-ésima franja, y el máximo después de la última; no existe la posición 0.
- **EL SISTEMA no mantendrá un catálogo ni un enumerado de tipos de evento:** el nombre describe el
  suceso y no está restringido a una lista.
- **EL SISTEMA tratará los eventos como metadatos del turno:** no alteran la rejilla aritmética de
  bloques, las franjas útiles ni la traducción de disponibilidad de la spec 005; su traslado al
  contrato del solver es de la spec 010.
- **SI se elimina un turno, ENTONCES EL SISTEMA eliminará sus eventos** (cascada del turno).

### RF-2. Identidad, orden y unicidad de los eventos

- **EL SISTEMA asignará a cada evento una identidad propia e independiente del turno y de la
  posición**, y las operaciones de consulta, actualización y eliminación lo identificarán por esa
  identidad.
- **EL SISTEMA admitirá varios eventos tras la misma franja del mismo turno** (por ejemplo, un receso
  seguido de un acto cívico).
- **EL SISTEMA mantendrá un orden explícito y persistente dentro de cada posición**, de modo que los
  eventos de una misma franja se presenten siempre en el mismo orden, con independencia del orden en
  que se registraron.
- **CUANDO se registre un evento sin orden explícito, EL SISTEMA le asignará el orden inmediatamente
  posterior al mayor de su posición** (máximo + 1), o **1** si la posición no tiene eventos.
- **SI se intenta registrar o actualizar un evento con un orden ya ocupado en la misma posición y
  turno, ENTONCES EL SISTEMA rechazará la operación** indicando el turno, la posición y el orden
  ocupado.
- **CUANDO se consulte un turno, EL SISTEMA devolverá sus eventos ordenados primero por posición y
  después por su orden dentro de la posición.**
- **SI se elimina un evento, ENTONCES los demás conservan su orden** y el sistema no los renumera.
- **SI dos eventos comparten nombre y posición, ENTONCES se aceptan:** el nombre describe el suceso y
  no identifica al evento.

### RF-3. Coherencia temporal de los eventos

- **EL SISTEMA tratará la hora de inicio y la de fin como un par opcional:** o se declaran **ambas**, o
  **ninguna**; declarar solo una se rechaza indicando el campo faltante.
- **CUANDO se declaren inicio y fin, EL SISTEMA exigirá que el fin sea posterior al inicio, que la
  diferencia coincida exactamente con la duración en minutos y que el intervalo esté contenido en la
  ventana del turno y en la ventana de su posición**; si no se cumple, rechazará la operación
  indicando la causa.
- **EL SISTEMA considerará la ventana de la posición k entre el fin de la franja de clase k y el
  inicio de la franja k+1** (para la última posición, entre el fin de la última franja y el fin del
  turno), de modo que las ventanas de posiciones distintas son **disjuntas**.
- **SI dos eventos de la misma posición declaran horario y sus intervalos se solapan, ENTONCES EL
  SISTEMA rechazará el registro o la actualización** indicando los dos eventos y el solape; unos
  eventos sin horario no se pueden comprobar y no se rechazan por esta regla.
- **SI dos eventos de posiciones distintas declaran horario y sus intervalos se solapan, o el de la
  posición posterior empieza antes de que termine el de la anterior, ENTONCES EL SISTEMA rechazará la
  operación** indicando los dos eventos y el cruce.
- **CUANDO dos eventos de la misma posición declaren horario, EL SISTEMA exigirá que su orden sea
  coherente con la cronología:** el de orden menor empieza antes.
- **EL SISTEMA estimará la duración de la franja de clase restando de la ventana del turno la suma de
  las duraciones de todos sus eventos y dividiendo entre el número de franjas**, sin admitir solapes
  internos: la suma coincide con la unión de intervalos y es la cifra que documenta RF-11. La
  estimación es de presentación y no altera la rejilla aritmética del solver (spec 005).

### RF-4. Actualización de turnos con eventos dependientes

- **SI se reduce el número de franjas de un turno y algún evento queda con su posición fuera del
  nuevo rango, ENTONCES EL SISTEMA mostrará los eventos afectados y pedirá confirmación explícita**;
  si se confirma, eliminará esos eventos y aplicará la reducción de forma atómica; si se cancela, no
  modificará el turno.
- **SI se actualiza la ventana del turno y el horario declarado de un evento deja de estar contenido
  en ella, ENTONCES EL SISTEMA rechazará la actualización** indicando los eventos afectados; no
  elimina ni recorta eventos por esta causa.
- **EL SISTEMA no cambiará las posiciones ni el orden de los eventos que sigan dentro del rango.**

### RF-5. Catálogo de tipos de aula

- **EL SISTEMA mantendrá un catálogo de tipos de aula identificados por su nombre.**
- **CUANDO se cree o renombre un tipo, EL SISTEMA exigirá un nombre no vacío tras recortar espacios y
  único según su clave de tipo** (sin distinguir mayúsculas ni espacios extremos); si no se cumple,
  rechazará la operación indicando el valor.
- **EL SISTEMA garantizará la unicidad normalizada en el almacenamiento**, no solo en la aplicación:
  guardará la **clave normalizada calculada por el servicio** (recorte + `toCaseFolded()`) y mantendrá
  un **índice único sobre esa clave**; ninguna escritura puede dejar dos tipos con la misma clave,
  incluidas las que solo cambian mayúsculas en Unicode (p. ej. «Física»/«FÍSICA»).
- **CUANDO se renombre un tipo cambiando solo mayúsculas o espacios extremos, EL SISTEMA lo tratará
  como el mismo tipo:** lo acepta y actualiza el nombre visible, sin considerarlo duplicado.
- **EL SISTEMA permitirá crear, listar, renombrar y eliminar tipos de aula.**
- **CUANDO se renombre un tipo, EL SISTEMA mostrará el nombre nuevo en todas las aulas y materias que
  lo referencian**, sin reescribir sus datos: la referencia es al tipo, no al texto.
- **SI se intenta eliminar un tipo referenciado por alguna aula o materia, ENTONCES EL SISTEMA
  rechazará siempre la operación mostrando las aulas y materias dependientes**; este catálogo **no**
  sigue el flujo genérico de «mostrar dependientes y pedir confirmación»: el borrado en uso nunca se
  confirma, porque dejaría aulas o materias sin tipo.
- **EL SISTEMA no permitirá crear aulas ni materias mientras el catálogo esté vacío**: ambos exigen
  un tipo existente.

### RF-6. Tipo de cada aula

- **EL SISTEMA exigirá que toda aula declare exactamente un tipo del catálogo al crearse o
  actualizarse.**
- **SI un aula se registra sin tipo o con un tipo que no existe en el catálogo, ENTONCES EL SISTEMA
  rechazará la operación** indicando el campo y el valor.
- **SI se actualiza un aula sin declarar el tipo, ENTONCES EL SISTEMA rechazará la operación:** la
  actualización declara el estado completo del aula y no conserva el tipo anterior de forma implícita.
- **CUANDO se consulte o se liste un aula, EL SISTEMA devolverá su tipo** (nombre e identidad del
  tipo).
- **EL SISTEMA no deducirá el tipo de un aula de su nombre, de su ubicación ni de ningún otro campo.**

### RF-7. Tipo requerido por cada materia

- **EL SISTEMA exigirá que toda materia declare exactamente un tipo de aula requerido del catálogo al
  crearse o actualizarse.**
- **SI una materia se registra sin tipo requerido o con un tipo que no existe en el catálogo,
  ENTONCES EL SISTEMA rechazará la operación** indicando el campo y el valor.
- **SI se actualiza una materia sin declarar el tipo requerido, ENTONCES EL SISTEMA rechazará la
  operación**, por la misma razón que en el aula.
- **CUANDO se consulte o se liste una materia, EL SISTEMA devolverá el tipo de aula que requiere.**
- **EL SISTEMA no deducirá el tipo requerido del texto libre de requisitos de la materia:** el tipo
  es un dato propio y el texto libre no participa en ninguna decisión del sistema.

### RF-8. Servicios, dominio, consola y huella de datos

- **EL SISTEMA ofrecerá operaciones para crear, consultar, listar, actualizar y eliminar eventos de un
  turno** con las reglas de RF-1 a RF-3, identificando el evento por su identidad; el turno conserva
  sus operaciones actuales.
- **EL SISTEMA ofrecerá las operaciones del catálogo de tipos de aula** descritas en RF-5.
- **EL SISTEMA exigirá y devolverá el tipo en las operaciones de aulas y materias** (RF-6, RF-7).
- **CUANDO se elimine una entidad referenciada, EL SISTEMA mostrará primero los dependientes y pedirá
  confirmación explícita**, según el patrón vigente de la spec 001; la excepción es el catálogo de
  tipos, cuyo borrado en uso siempre se rechaza (RF-5).
- **EL SISTEMA incluirá en la huella de datos el tipo de cada aula, el tipo requerido por cada
  materia y los eventos de cada turno**, para que un cambio de tipo o de evento durante una generación
  marque el horario como «datos anteriores» (spec 004).
- **EL SISTEMA actualizará los comandos de la consola de datos que hoy dan de alta y de baja
  recesos**, para que operen sobre eventos con nombre e identidad; la consola **no** es frontend y
  forma parte de esta entrega.
- **EL SISTEMA dejará de exponer recesos como concepto propio en los servicios, la consola y la
  documentación de la API:** quien consume esos datos ve **eventos con nombre** y **tipos de aula**.
  La construcción de la entrada del solver se rige por RF-12 y el contrato 2.0 es de la 010.

### RF-9. Esquema esperado, marca de versión y apertura

- **EL SISTEMA reconocerá como esquema esperado el que incluye el catálogo de tipos de aula y el
  registro de eventos**, con la referencia de tipo obligatoria en aulas y materias, y en los eventos
  la identidad propia, el nombre obligatorio, la posición y el orden.
- **EL SISTEMA considerará retirado el registro de recesos:** su presencia no forma parte del esquema
  esperado.
- **CUANDO cambie el esquema esperado, EL SISTEMA incrementará la marca de versión de esquema** y
  actualizará en el mismo cambio el catálogo de tablas y columnas esperadas y la huella de esquema que
  lo vigila.
- **SI una base existente tiene el esquema anterior —recesos sin eventos, sin catálogo de tipos o sin
  las referencias de tipo— ENTONCES EL SISTEMA tratará la apertura como fallo**, sin migrar, sin
  borrar y sin sobrescribir datos, y ofrecerá crear una base nueva con respaldo (specs 001 y 004).
- **EL SISTEMA no convertirá datos de recesos en eventos ni supondrá tipos para aulas o materias
  existentes.**
- **EL SISTEMA garantizará la reapertura:** una base creada con el esquema nuevo se abre
  correctamente en usos posteriores.

### RF-10. Datos de prueba

- **EL SISTEMA sembrará un catálogo inicial de tipos de aula suficiente para los espacios del
  centro**, con al menos un tipo genérico de aula y los tipos especializados que el conjunto use.
- **EL SISTEMA asignará a cada aula sembrada un tipo coherente con su papel:** las aulas genéricas, el
  tipo genérico; el laboratorio, su tipo; el taller, el suyo.
- **EL SISTEMA asignará a cada materia sembrada el tipo especializado que le corresponde según su
  área, y el tipo genérico cuando el conjunto no tenga ninguna aula de ese tipo especializado.** La
  correspondencia es: ciencias experimentales (Física, Química, Biología, Ciencias Naturales y
  Ciencias de la Tierra) → laboratorio; materias de informática o taller → taller; Educación Física →
  cancha; el resto → genérico. **La fila de informática/taller se declara inocua para el perfil
  actual**, que no tiene materias de esas áreas: el tipo taller queda como tipo de aula sin materia; si
  el perfil incorpora una materia de esa área, la regla se aplica.
- **EL SISTEMA garantizará que todo tipo requerido por una materia sembrada tiene al menos un aula
  sembrada de ese tipo**, lo que se cumple por construcción con la regla anterior.
- **EL SISTEMA sembrará los recesos de los dos turnos como eventos con nombre**, conservando posición,
  orden, duración y horario.
- **CUANDO termine la siembra, EL SISTEMA informará los recuentos de tipos de aula y de eventos**: el
  recuento de recesos **se sustituye** por el de eventos (no se suman ambos) y se añade el de tipos de
  aula.
- **EL SISTEMA mantendrá el determinismo de la siembra:** mismo perfil, mismos tipos, mismos eventos y
  mismos recuentos.
- **CUANDO se reemplace o vacíe el conjunto, EL SISTEMA borrará en orden de dependencia con las claves
  foráneas activas:** primero los **eventos**, después los **turnos**; primero las **aulas** y las
  **materias**, después el **catálogo de tipos de aula**.
- **EL SISTEMA actualizará sus comprobaciones de vaciado y de esquema** para nombrar las entidades
  nuevas y no el registro de recesos.
- **EL SISTEMA no promete que el conjunto sembrado sea generable con la regla dura de tipo de aula:**
  esta spec supersede la promesa de generabilidad de la 008 en ese punto; la coherencia exigida es la
  de los datos y los recuentos. El conjunto se sustituirá más adelante por los datos reales del liceo,
  fuera de esta spec.

### RF-11. Documentación de la API para el equipo de frontend

- **CUANDO cambie la API de servicios, EL SISTEMA actualizará la guía de integración del frontend
  (`docs/interfaz-frontend.md`)** con las firmas, los tipos expuestos, los errores y los estados
  nuevos.
- **La documentación describirá los eventos con nombre e identidad, las operaciones del catálogo de
  tipos y los campos de tipo de aulas y materias**, y **dejará de documentar** el concepto de receso
  como entidad propia, la clave de evento basada solo en turno y posición y las operaciones de alta y
  baja de recesos.
- **La documentación indicará la fórmula de estimación de la duración del bloque** a partir de la
  ventana del turno y de la **suma de las duraciones** de todos sus eventos, tal como la fija RF-3;
  con solo recesos sembrados, el resultado coincide con el actual.
- **La documentación enumerará los cambios de firma que rompen la compilación del frontend**, para que
  su equipo los adapte, y advertirá de que la interfaz de usuario está fuera del alcance de esta
  entrega.
- **La documentación registrará como deuda del equipo de frontend** las heurísticas que deducen el
  tipo de aula por el nombre o por el texto libre de requisitos, e indicará que su sustitución por los
  tipos persistidos corresponde a ese equipo.

### RF-12. Puente de la entrada del solver hasta la 010

- **MIENTRAS la 010 no sustituya el contrato, EL SISTEMA mantendrá la construcción de la entrada
  emitiendo la sección de recesos del contrato 1.0, derivada de los eventos** del turno (posición,
  duración y horario), **con una entrada por evento**, de modo que el backend y sus pruebas sigan
  compilando y pasando.
- **EL SISTEMA no añadirá en esta spec los tipos ni los eventos al contrato de configuración:** el
  contrato 2.0 y su sección de eventos son de la 010.
- **EL SISTEMA tratará este puente como temporal:** la 010 lo elimina al sustituir el contrato y no se
  ofrece como capa de compatibilidad del producto.

## Requisitos no funcionales

- **RNF-1. Mensajes en español:** todo rechazo identifica la entidad, el campo, el valor y, cuando
  exista, la acción disponible, sin errores técnicos crudos.
- **RNF-2. Protección de datos:** no hay migración automática; una base con esquema no esperado no se
  borra ni se sobrescribe, y toda operación destructiva conserva el respaldo obligatorio
  (constitución §5).
- **RNF-3. Determinismo:** el catálogo inicial, los tipos de aulas y materias, los eventos sembrados y
  la asignación automática del orden de un evento no dependen del reloj, del azar ni del orden de
  inserción; dos siembras del mismo perfil dejan los mismos datos.
- **RNF-4. Reapertura:** una base creada con el esquema nuevo se abre correctamente tantas veces como
  se use; una base con el esquema anterior falla al abrir con un mensaje que explica la causa y la
  acción.
- **RNF-5. Documentación completa:** la guía de integración permite al equipo de frontend compilar y
  adaptar sus llamadas sin consultar el código, y enumera todos los cambios incompatibles de esta
  tanda.
- **RNF-6. Verificación sin la interfaz:** la construcción del proyecto debe poder **excluir la
  interfaz de usuario y sus pruebas**; con esa configuración, **backend, datos, seed, consola y sus
  pruebas compilan y pasan**, y la política de pruebas del proyecto se cumple con esas piezas. La
  interfaz y los tests que dependen de ella quedan fuera de la entrega. El detalle técnico es del
  plan.
- **RNF-7. Trazabilidad:** cada RF de esta spec tendrá al menos una comprobación verificable en las
  tareas; sin cobertura, la tarea no está hecha.

## Casos límite

- **Evento con nombre vacío o solo espacios:** rechazo con el campo y el valor (RF-1).
- **Evento con turno inexistente:** rechazo (RF-1).
- **Evento con posición menor que 1 o mayor que el número de franjas del turno:** rechazo (RF-1).
- **Evento con posición 0:** rechazo: no existe tiempo antes de la primera franja (RF-1).
- **Evento con posición igual al número de franjas:** se acepta y significa después de la última
  (RF-1).
- **Evento con duración cero o negativa:** rechazo (RF-1).
- **Dos eventos tras la misma franja (receso y acto cívico):** se aceptan, cada uno con su identidad, y
  se presentan según su orden (RF-2).
- **Alta de un evento sin orden en una posición vacía:** recibe el orden 1 (RF-2).
- **Alta de un evento sin orden en una posición con eventos:** recibe máximo + 1 (RF-2).
- **Dos eventos con el mismo orden en la misma posición y turno:** rechazo indicando turno, posición y
  orden (RF-2).
- **Dos eventos con el mismo nombre y la misma posición:** se aceptan; el nombre no identifica al
  evento (RF-2).
- **Eliminar un evento intermedio:** los demás conservan su orden y no se renumeran (RF-2).
- **Actualizar o eliminar un evento por su identidad:** la operación no depende del turno ni de la
  posición (RF-2).
- **Evento con solo hora de inicio o solo de fin:** rechazo por par incompleto (RF-3).
- **Evento con fin anterior o igual al inicio:** rechazo (RF-3).
- **Evento cuya diferencia fin − inicio no coincide con su duración:** rechazo (RF-3).
- **Evento cuyo intervalo se sale de la ventana del turno o de la ventana de su posición:** rechazo
  (RF-3).
- **Dos eventos de la misma posición con horarios solapados:** rechazo con los dos eventos (RF-3).
- **Dos eventos de la misma posición sin horario:** se aceptan; no hay solape comprobable (RF-3).
- **Dos eventos de posiciones distintas con horarios solapados:** rechazo con los dos eventos (RF-3).
- **Dos eventos de posiciones distintas donde el de la posición posterior empieza antes de que termine
  el anterior:** rechazo por cruce de ventanas (RF-3).
- **Dos eventos de la misma posición con horario en orden inverso a su orden:** rechazo por
  incoherencia cronológica (RF-3).
- **Estimación de la duración del bloque:** se resta la suma de las duraciones de todos los eventos y
  se divide entre el número de franjas; no se admiten solapes internos (RF-3, RF-11).
- **Reducir las franjas de un turno y dejar eventos fuera de rango:** se muestran los afectados y se
  pide confirmación; si se confirma, se eliminan y se aplica la reducción; si no, no cambia nada
  (RF-4).
- **Actualizar la ventana de un turno y dejar un evento fuera:** rechazo indicando los eventos (RF-4).
- **Tipo de aula con nombre repetido salvo mayúsculas o con espacios extremos:** rechazo por
  duplicado según la clave de tipo (RF-5).
- **Renombrar un tipo cambiando solo mayúsculas o espacios:** se acepta como el mismo tipo, sin
  duplicado (RF-5).
- **Renombrar un tipo en uso:** se acepta y las aulas y materias muestran el nombre nuevo (RF-5).
- **Eliminar un tipo referenciado:** se muestran los dependientes y se rechaza siempre; no hay
  confirmación que permita el borrado (RF-5).
- **Catálogo vacío:** no se pueden crear aulas ni materias (RF-5).
- **Aula sin tipo, con tipo inexistente o actualizada sin tipo:** rechazo (RF-6).
- **Materia sin tipo requerido, con tipo inexistente o actualizada sin tipo:** rechazo (RF-7).
- **Materia con texto libre de requisitos que menciona «laboratorio» pero tipo requerido «Aula de
  estudio»:** manda el tipo requerido; el texto libre no interviene (RF-7, RF-11).
- **Aula existente en una base anterior sin tipo:** no se migra; la apertura falla y el usuario crea
  una base nueva (RF-9).
- **Base anterior con registro de recesos:** fallo de apertura, sin convertir recesos en eventos
  (RF-9).
- **Base nueva con el esquema esperado:** se crea y se vuelve a abrir (RF-9).
- **Cambio de tipo de un aula o de una materia durante una generación:** la huella de datos cambia y
  el horario se marca «datos anteriores» (RF-8).
- **Materia de ciencias experimentales en un conjunto con laboratorio:** recibe el tipo laboratorio
  (RF-10).
- **Materia de ciencias experimentales en un conjunto sin laboratorio:** recibe el tipo genérico, no se
  inventa un laboratorio (RF-10).
- **Educación Física en un conjunto sin aula de cancha:** recibe el tipo genérico; la spec no inventa
  una cancha (RF-10).
- **Perfil sin materias de informática o taller:** la fila de la regla de tipos es inocua; el tipo
  taller queda como tipo de aula sin materia (RF-10).
- **Vaciado o reemplazo del conjunto:** se borran primero los eventos y después los turnos; primero
  las aulas y las materias y después el catálogo de tipos, con las claves foráneas activas (RF-10).
- **Siembra con un tipo requerido sin aula de ese tipo:** se rechaza la aplicación del conjunto
  (RF-10).
- **Siembra aplicada dos veces sobre bases vacías:** mismos tipos, mismos eventos y mismos recuentos
  (RF-10).
- **Conjunto sembrado no generable por la regla de tipo:** no es un fallo de esta spec; se documenta y
  se sustituirá por los datos reales (RF-10).
- **Varios eventos en la misma posición al construir la sección de recesos del puente:** se emite una
  entrada por evento, no solo la primera (RF-12).

## Fuera de alcance

- **El contrato del solver, la regla de tipo, las validaciones nuevas, la supersesión de la 005, la
  enmienda de la constitución y el diagrama entidad-relación:** son de la spec 010.
- **La interfaz de usuario y su compilación:** la implementa otro equipo (spec 001); esta entrega deja
  la API documentada y las firmas en limpio, y **no** añade capas de compatibilidad para que el
  frontend siga compilando.
- **La adaptación de las vistas y widgets del frontend** y la sustitución de sus heurísticas por los
  tipos persistidos: quedan como deuda documentada de su equipo (RF-11).
- **La migración de bases existentes y la conversión de recesos en eventos:** no hay migración
  (RF-9).
- **Una pantalla dedicada a administrar el catálogo de tipos de aula:** el catálogo se administra por
  servicios (RF-5, RF-11).
- **Que el conjunto sembrado sea generable con la regla dura de tipo** y la sustitución del conjunto
  por los datos reales del liceo: llegan después (RF-10).
- **El uso compartido de la cancha:** queda diferido y se registra en la spec 010.
- **La capacidad de aula y el aforo:** siguen fuera de las reglas del solver (spec 005).

## Criterios de finalización

1. Se registran, consultan, actualizan y eliminan eventos con nombre e identidad propia; varios
   comparten franja y se presentan en su orden; la posición usa la convención 1..número de franjas y
   el turno los elimina en cascada (RF-1, RF-2).
2. La coherencia temporal de los eventos exige el par inicio/fin, la diferencia igual a la duración,
   el intervalo dentro del turno y de la ventana de su posición, y la ausencia de solapes dentro de la
   posición y entre posiciones (RF-3).
3. Reducir franjas muestra los eventos afectados y pide confirmación antes de eliminarlos; cambiar la
   ventana no recorta eventos (RF-4).
4. Existe un catálogo administrable de tipos de aula, con unicidad normalizada garantizada en el
   almacenamiento, renombrado propagado, renombres de solo mayúsculas aceptados, borrado en uso
   siempre rechazado y **catálogo vacío que impide crear aulas y materias** (RF-5).
5. Toda aula y toda materia declara exactamente un tipo del catálogo, se persiste con él, se expone en
   las consultas, no se infiere de nombres ni de texto libre y su omisión en una actualización se
   rechaza (RF-6, RF-7).
6. Los servicios, el dominio y la consola trabajan con eventos con nombre e identidad y con tipos de
   aula y materia; la huella de datos los incluye (RF-8).
7. El esquema esperado incluye el catálogo y los eventos con identidad y orden, la marca de versión se
   incrementó junto con su catálogo y su huella, una base nueva se abre de nuevo y una base anterior
   falla sin migrar (RF-9).
8. Los datos de prueba aplican la regla medible de tipos: cada materia recibe el tipo especializado de
   su área si existe un aula de ese tipo y, si no, el genérico; todo tipo requerido tiene al menos un
   aula; los recuentos informan eventos en lugar de recesos y añaden tipos; el vaciado respeta el
   orden de dependencia; todo es determinista y no se promete generabilidad (RF-10).
9. La guía de integración del frontend documenta las firmas nuevas, los cambios incompatibles, la
   fórmula de duración por suma de eventos y la deuda de las heurísticas de la interfaz (RF-11).
10. La construcción de la entrada sigue emitiendo la sección de recesos derivada de los eventos hasta
    que la 010 la sustituya, sin añadir tipos ni eventos al contrato (RF-12).
11. La construcción puede excluir la interfaz y, con esa configuración, backend, datos, seed, consola
    y sus pruebas compilan y pasan (RNF-6).
12. Cada RF tiene al menos una comprobación verificable en las tareas (RNF-7).

## Dudas abiertas

Ninguna. La duda diferida sobre el uso compartido de la cancha vive en la spec 010, porque afecta a la
regla del solver.
