# Spec 015 - El tipo general de aula como parte del esquema

Estado: implementada (2026-11-05)

> **015 y 016 son las dos mitades de la antigua 015 (decisión del usuario, 2026-11-05, al cerrar el
> re-QA).** Esta spec es la **mitad 1: la garantía estructural**. La **016** es la **mitad 2:
> comprobación y cierre**. **La 015 precede a la 016**: la 016 consume lo que esta deja en el esquema.
> Lo que aquí no aparece —los momentos de comprobación, el respaldo silencioso de la interfaz, el
> mensaje del aula de referencia, la huella de datos, la documentación de la API y la coherencia con la
> 013— es de la **016**.
>
> **Decisión de fondo (2026-11-05).** El **tipo general de aula** se garantiza **por el esquema** (la
> creación de la base), **no** por el conjunto de datos de prueba. Palabras literales: «hagamos la 015
> ahora, y la manejaremos mediante el **esquema**, no el seed de datos».
>
> **Decisiones cerradas.** Las diez dudas (D-1 a D-10) se resolvieron el **2026-11-05** y se añadieron
> dos decisiones nuevas el mismo día: **validez por presencia por nombre + la fila** (el cuerpo de los
> disparadores no es identidad) y **comprobación al abrir, antes de generar y antes de guardar, sin
> respaldo silencioso**. Ambas se reparten entre las dos mitades. **La spec no tiene requisitos
> condicionales** y sigue en **borrador** hasta la aprobación del usuario.
>
> **Qué entra aquí, y por qué.** Todo lo sin lo cual el producto se rompe en cuanto se crea la base: la
> **fila en la creación**, la **identidad y unicidad**, el **borrado prohibido**, el **renombrado
> prohibido**, la **convivencia con los datos de prueba** y la **validez de la base con su guardia de
> versión 6**, más la **documentación de la versión y de la guardia** y las **supersesiones** que la
> propia garantía deja obsoletas.
>
> **Origen.** La pregunta fue si todo tipo de aula debe existir siempre al crear una base y no poder
> borrarse, «para que el solver tenga el tipo de aula que va a reservar en los cambios de aula». Se
> verificó que **no estaba decidido ni implementado**: el catálogo admitía renombrar y eliminar, la
> creación de la base solo creaba estructura (la base nacía con el catálogo vacío) y el único camino que
> introducía el tipo general era la siembra de datos de prueba, que es opcional.
>
> **Orden de trabajo (decisión del usuario, 2026-11-05).** La 015 (y después la 016) **se implementan
> antes** que la **013** (preferencias del docente, aprobada y sin implementar) y antes que la **014**
> (materias en bloques de dos horas, planificada).
>
> **Número.** La **014** sigue reservada para los bloques de dos horas y no se ha escrito; por eso las
> dos mitades llevan 015 y 016 y queda el hueco de la 014.

## Contexto y objetivo

El solver reserva el aula de referencia de cada curso en las franjas en que el curso se desplaza, y
exige que esa aula sea del **tipo general/ordinario** del centro. Ese tipo no se guarda en ninguna
columna ni viaja en la configuración: se reconoce por **convención de nombre** (nombre normalizado
`aula de estudio`). La convención funcionaba, pero **nada la garantizaba**: la base se creaba con el
catálogo vacío, el tipo solo aparecía si alguien sembraba los datos de prueba o lo creaba a mano, y una
vez creado se podía renombrar a cualquier otro nombre o eliminar si no tenía dependientes. Un centro
podía tener la base llena de aulas, materias y cursos y, aun así, no tener el tipo que el solver
necesita.

Además, la guardia que decide si una base sirve solo miraba **tablas y columnas**. Aunque la fila
estuviera, una base sin su protección —o con la fila quitada— era indistinguible de una base correcta, y
la aplicación la aceptaba.

El objetivo de esta mitad es que **toda base nueva tenga el tipo general, lo conserve y lo proteja**, sin
depender de los datos de prueba ni de la memoria del usuario, y que la **validez de la base** incluya
expresamente ese tipo y su protección, de modo que una base sin ellos **no sea el esquema esperado**. La
mitad 2 (016) fija **cuándo** se comprueba y **qué ocurre** en cada momento.

**Qué no es.** No es una columna ni un rol nuevo del tipo (la convención de nombre se mantiene), ni un
cambio del modelo del solver, del contrato o de la huella de datos, ni una migración, ni una pantalla
nueva de administración, ni la sustitución de los datos de prueba por los reales.

## Usuarios

- **Usuario académico:** espera que una base recién creada pueda usarse para dar de alta aulas, materias,
  planes, cursos y turnos, sin saber qué nombre debe tener un tipo de aula.
- **Usuario que mantiene el catálogo:** espera que el sistema le impida las operaciones que dejarían el
  centro sin el tipo que el cálculo necesita, y que le diga por qué.
- **Equipo de desarrollo:** necesita una única lectura de «tipo general», un solo literal en el código de
  producción y una guardia que distinga una base con la garantía de otra sin ella.
- **Equipo de pruebas y QA:** necesita poder afirmar, sobre una base recién creada o sembrada, qué tipos
  existen, cuáles no se pueden tocar y cuándo una base no corresponde al esquema esperado.

## Historias de usuario

- HU-1. Como usuario académico, quiero que una base nueva ya tenga el tipo de aula que el cálculo
  necesita, para poder empezar sin configurarlo.
- HU-2. Como usuario que administra tipos, quiero que el sistema me impida renombrar o eliminar ese
  tipo, para no descubrir el problema cuando la generación falle.
- HU-3. Como usuario, quiero un mensaje claro cuando una operación no está permitida, para saber qué
  hacer en su lugar.
- HU-4. Como miembro del equipo de pruebas, quiero que la siembra y el reemplazo total sigan funcionando
  sobre una base nueva y dejen el tipo general presente.
- HU-5. Como miembro del equipo de desarrollo, quiero que el nombre del tipo general esté definido en un
  solo sitio y que la estructura de la base, los servicios y el cálculo no puedan desincronizarse.
- HU-6. Como miembro del equipo de desarrollo, quiero que una base a la que le falta el tipo o su
  protección no corresponda al esquema esperado, para no arrastrar un estado inválido.

## Definiciones

- **Tipo general (ordinario) de aula:** el tipo cuya **convención de nombre** es `aula de estudio`, es
  decir, aquel cuyo **nombre visible normalizado** (sin espacios extremos y sin distinguir mayúsculas en
  Unicode) es `aula de estudio`. Es el tipo al que deben pertenecer las aulas de referencia de los
  cursos.
  > Decisión vigente desde el 2026-10-26 (spec 010): no hay columna, campo del contrato ni consulta
  > adicional. El **contrato y la validación del aula de referencia comparan el nombre visible
  > normalizado**; la guardia de esta spec aplica el predicado de «validez de la base», que añade la
  > **clave almacenada** de esa fila. Son dos lecturas distintas y ese predicado es lo que las alinea.
- **Clave almacenada:** el valor de la columna de clave del catálogo, que guarda la forma normalizada
  calculada por el servicio. Para el tipo general, la clave almacenada exigida es `aula de estudio`.
- **Definición única del tipo general:** el nombre convenido y su clave se declaran **una sola vez en el
  código de producción** —creación de la base, guardia, protección, cálculo, mensajes, datos de prueba e
  interfaz— y el resto de puntos lo consume. Quedan **fuera** de esta exigencia la documentación, las
  pruebas y los ejemplos.
  > La exigencia cubre el **literal**, no el algoritmo de normalización: que servicio y cálculo tengan
  > cada uno su función de normalización equivalente es una duplicación conocida y aceptada, anotada
  > como riesgo, no como requisito.
- **Tipo especializado:** cualquier otro tipo del catálogo (laboratorio, taller, cancha…). Nunca es
  aula de referencia.
- **Protección del tipo general:** el rechazo de eliminarlo y de renombrarlo en el almacenamiento —para
  toda escritura, venga o no del producto— más el rechazo con mensaje en español en los servicios.
- **Base nueva:** base que la aplicación crea porque el archivo no existía.
- **Validez de la base:** una base es válida cuando se cumplen **estas condiciones**:
  1. están, **por nombre**, todas las tablas y columnas del catálogo esperado;
  2. están, **por nombre**, el **índice único** del catálogo de tipos y **los dos disparadores** (el que
     impide el borrado y el que impide el renombrado);
  3. la fila del tipo general cumple el predicado, que se enuncia **siempre con esta formulación**:
     > «La base es válida si existe **exactamente una** fila cuyo **nombre visible normalizado** sea
     > `aula de estudio` **y esa fila (la única) tiene la clave almacenada** `aula de estudio`.»
  4. **no hay tablas retiradas** presentes.

  La condición de **fila única** y la de **clave** cubren casos distintos: con la fila canónica presente,
  una segunda fila con nombre canónico y clave `AULA DE ESTUDIO` **falla la condición de fila única** (no
  la de clave); la condición de clave cubre el caso de **una sola** fila con nombre canónico y una clave
  almacenada distinta. La tolerancia de objetos de más **no alcanza a las tablas retiradas**: su presencia
  sigue invalidando la base (specs 004 y 009).
- **Base aceptada:** base que la aplicación abre porque **es válida** (definición anterior) y su marca de
  versión es la esperada. Los momentos y las consecuencias de la comprobación son de la **016**.
- **Guardia de esquema:** el conjunto de comprobaciones que decide si una base corresponde al esquema
  esperado: la marca de versión, la **presencia por nombre** de tablas, columnas, índice y disparadores,
  **la fila del tipo general** según el predicado de «validez de la base» y la **ausencia de las tablas
  retiradas**. El **cuerpo** de los
  disparadores **no** forma parte de la identidad: un reformateo no invalida la base. Los **objetos de
  más se toleran** (salvo las tablas retiradas) y los **índices automáticos** que crea el propio
  almacenamiento para claves primarias y restricciones **no se exigen por nombre**.
- **Base con datos / base vacía:** las define la spec 008, **con el ajuste de esta spec**: la fila del
  tipo general **no** hace que una base esté «con datos». Cualquier otro registro de dominio sigue
  contando.
- **Datos de prueba (siembra):** el conjunto determinista de la spec 008, que crea los tipos
  «Laboratorio» y «Taller» **reutilizando** el tipo general de la base y ya **no declara** «Aula de
  estudio» como dato propio.
- **Esquema esperado:** la estructura que la aplicación reconoce —tablas, columnas, índice y
  disparadores nombrados, **sin las tablas retiradas**— **más la fila del tipo general** y la marca de
  versión. Un esquema distinto es un fallo de apertura, no una conversión.

## Requisitos funcionales

### RF-1. El tipo general existe en toda base nueva

- **CUANDO la aplicación cree una base nueva, EL SISTEMA la dejará con exactamente un tipo general de
  aula registrado**, con el nombre visible convenido y su clave almacenada, y con su protección activa.
- **La garantía será inseparable de la creación:** o queda la base completa —estructura, tipo general y
  protección— o no queda esquema utilizable. **SI la creación no puede dejar los tres, ENTONCES no
  quedará esquema parcial**: lo escrito se revierte y el mensaje lo explica; el archivo puede quedar con
  la versión ausente o en cero y **no se reintenta sobre él**. La **única salida es descartar ese archivo
  y crear una base nueva con respaldo obligatorio** (specs 001 y 004).
- **EL SISTEMA no exigirá al usuario crear, nombrar ni configurar ese tipo** para poder usar la base, ni
  dependerá de que se apliquen los datos de prueba.
- **MIENTRAS no se apliquen datos de prueba, EL SISTEMA considerará la base recién creada utilizable
  para dar de alta aulas, materias, planes, cursos y turnos**, porque el catálogo nunca está vacío.
- **EL SISTEMA creará el tipo general de forma idempotente:** repetir el paso que lo garantiza no deja
  dos filas ni altera el nombre de la existente.

### RF-2. Identidad, unicidad y definición única del tipo general

- **EL SISTEMA identificará el tipo general por la convención de nombre** (nombre visible normalizado
  `aula de estudio`); la comprobación de validez aplica el predicado completo de RF-6, que incluye la
  **clave almacenada** de esa fila.
- **EL SISTEMA no añadirá ninguna marca, columna, campo del contrato ni sección nueva** para señalar qué
  tipo es el general: la convención de nombre sigue siendo la única fuente.
- **EL SISTEMA garantizará que no haya dos filas con la misma clave almacenada**, apoyándose en el índice
  único del catálogo. La garantía alcanza a la fila del tipo general en toda escritura.
  > **Límite declarado:** no se promete unicidad **Unicode** frente a escrituras directas. La
  > normalización la calcula el servicio; una escritura ajena puede guardar una clave sin normalizar y
  > convivir con la correcta. La validez de la base (RF-6) exige la clave **almacenada** correcta, así
  > que esa escritura deja la base **inválida** aunque la aplicación la hubiera aceptado antes; la
  > limitación de fondo es de la 009 y esta spec no añade protección en el alta.
- **CUANDO se intente crear un tipo con el nombre del general, la operación fallará por duplicado**, con
  el mensaje de duplicado que ya existe (spec 009, RF-5); esta spec no añade un mensaje nuevo para ese
  caso.
- **EL SISTEMA tendrá una única definición del literal del tipo general en el código de producción**
  —creación de la base, guardia, protección, cálculo, mensajes, datos de prueba e interfaz—; ningún otro
  punto de producción repetirá el literal y todos lo consumirán de esa definición. **La comprobación es
  ejecutable:** el literal no aparece más de una vez en el código de producción, y los puntos que lo
  necesitan lo obtienen de la definición (el criterio concreto y la lista de sitios los fija el plan).
- **CUANDO se consulte el catálogo, EL SISTEMA devolverá el tipo general como un tipo más**, con su
  identidad y su nombre visible, y la consulta no fallará por su presencia.

### RF-3. El tipo general no se puede eliminar

- **EL SISTEMA no permitirá eliminar el tipo general, aunque no lo use ninguna aula ni materia.**
- **El rechazo alcanzará a cualquier vía de escritura**, sea el servicio del producto o una escritura
  directa en la base; no habrá ningún camino del producto que desactive la protección.
- **CUANDO el rechazo lo produzca el servicio, EL SISTEMA devolverá un mensaje en español** que indique
  que es el tipo general del centro y que no admite borrado.
- **SI una escritura que no pasó por la comprobación del servicio es rechazada por la protección
  —por ejemplo, una carrera o otra conexión—, ENTONCES el sistema devolverá el mensaje del catálogo en
  español**, no el error técnico crudo del almacenamiento (RF-7).
- **SI el tipo es a la vez el general y está en uso por aulas o materias, ENTONCES el motivo que se
  muestra es «es el tipo general»**, no la lista de dependientes. La prioridad del mensaje es del
  requisito general sobre el de «en uso».
- **EL SISTEMA mantendrá sin cambios el rechazo del borrado de cualquier tipo especializado que esté en
  uso** por aulas o materias, con la lista de dependientes que ya devuelve.

### RF-4. El tipo general no se puede renombrar

- **EL SISTEMA no permitirá cambiar el nombre visible ni la clave del tipo general por ninguna vía**, ni
  siquiera cambiando solo mayúsculas, minúsculas o espacios extremos.
- **El rechazo alcanzará a cualquier escritura que cambie el nombre o la clave de esa fila**, venga del
  servicio o de una escritura directa. **Una actualización que no cambie ninguno de los dos** —por
  ejemplo, la que solo toca la fecha de modificación— **no se considera un renombrado** y no la bloquea
  la protección del almacenamiento.
- **CUANDO se intente renombrar el tipo general por el servicio, EL SISTEMA rechazará la operación con
  un mensaje en español**, incluso si el nombre recibido es idéntico al actual: el servicio no admite
  ninguna petición de renombrado del tipo general, y el mensaje indica el valor recibido, el nombre
  admitido y la acción disponible.
- **SI el rechazo proviene del almacenamiento, ENTONCES el sistema devolverá el mensaje del catálogo en
  español** (RF-7).
- **EL SISTEMA no reescribirá los datos de las aulas ni de las materias**: la referencia es al tipo, no
  al texto (regla ya vigente).
- **EL SISTEMA mantendrá sin cambios la regla general de unicidad del catálogo** al renombrar cualquier
  tipo especializado.

### RF-5. Convivencia con los datos de prueba

- **CUANDO se apliquen los datos de prueba, EL SISTEMA reutilizará el tipo general que ya trae la base y
  no lo creará ni lo duplicará**; el conjunto seguirá creando los demás tipos de su perfil (hoy
  «Laboratorio» y «Taller») y **dejará de declarar «Aula de estudio» como dato propio**.
- **CUANDO se compruebe si la base ya tiene datos, EL SISTEMA excluirá exactamente la fila del tipo
  general**: una base recién creada sigue siendo una base vacía a efectos de la aplicación inicial, y
  cualquier otro registro de dominio sigue contando.
- **CUANDO se ejecute el reemplazo total, EL SISTEMA borrará los datos de dominio excepto el tipo
  general, que conservará con su identidad**, sin desactivar la protección en ningún momento.
- **CUANDO termine cualquier aplicación o reemplazo de los datos de prueba, EL SISTEMA dejará la base con
  el tipo general presente.**
- **EL SISTEMA mantendrá deterministas los recuentos que informa la siembra y los leerá de la base tras
  escribir.** El recuento de tipos de aula es **3** (el general, «Laboratorio» y «Taller») y cuenta las
  filas del catálogo, incluido el general.
- **CUANDO se aplique el mismo perfil dos veces, EL SISTEMA dejará los mismos datos y los mismos
  recuentos.** La comparación se hace **sobre dos bases vacías o con reemplazo total**; sin reemplazo,
  una base con datos se rechaza, como ya fija la spec 008, y esta spec no lo cambia.
- **EL SISTEMA no promete, por este cambio, que el conjunto sembrado sea generable** con la regla dura de
  tipo de aula: eso lo dejó dicho la 009 y no se reabre.

### RF-6. Validez de la base y guardia de esquema

- **EL SISTEMA considerará válida una base solo si se cumplen estas condiciones:** tablas y columnas por
  nombre; el índice único y **los dos** disparadores por nombre; la ausencia de las tablas retiradas; y el
  predicado de la fila, que se enuncia **siempre con esta formulación**:
  > «La base es válida si existe **exactamente una** fila cuyo **nombre visible normalizado** sea
  > `aula de estudio` **y esa fila (la única) tiene la clave almacenada** `aula de estudio`.»
- **EL SISTEMA distinguirá dos bases que difieran en la presencia de los objetos exigidos o de la fila**,
  y lo hará **en la guardia**, no solo en la huella congelada de las pruebas: una base sin un objeto
  nombrado, sin la fila o con la fila en la forma incorrecta **no corresponde al esquema esperado**.
  > Si la huella congelada de las pruebas incluye el **cuerpo** de un disparador, es una **guardia de
  > release** (anti-regresión del DDL) y **no** forma parte de la validez: un reformateo no invalida la
  > base.
- **EL SISTEMA no exigirá el cuerpo de los disparadores** y **tolerará objetos de más**, **salvo las
  tablas retiradas**, cuya presencia sigue invalidando la base (specs 004 y 009). Los **índices
  automáticos** del almacenamiento no se exigen por nombre.
- **CUANDO se introduzca el tipo general y su protección, EL SISTEMA incrementará la marca de versión de
  esquema a 6** y mantendrá **inseparables** la marca, el catálogo de estructura esperada y la huella que
  lo vigila: no puede quedar una base con la marca nueva y una descripción antigua del catálogo.
- **Los momentos y las consecuencias de aplicar esta validez son de la spec 016.**

> **Anotado por la 019 (2026-11-10).** La **versión de esquema vigente pasa a 7** por la 019 (materias
> suplentes en el esquema). La **política sin migración**, el **catálogo de estructura esperada** y la
> **guardia** de esta spec **no cambian**: se sigue exigiendo lo mismo (tablas, columnas, índice, los dos
> disparadores y exactamente una fila del tipo general) y una base de la versión anterior se rechaza al
> abrir.

### RF-7. Servicios, mensajes y coherencia documental

- **EL SISTEMA conservará las operaciones actuales del catálogo** (crear, listar, renombrar, eliminar)
  con sus firmas y **sin añadir campos al tipo que expone**: el consumidor no cambia su forma de leer el
  catálogo.
- **CUANDO rechace una operación sobre el tipo general, EL SISTEMA devolverá un mensaje en español** que
  nombre el tipo, la operación no permitida, el valor recibido y la acción disponible, sin errores
  técnicos crudos, **también cuando el rechazo venga del almacenamiento** (RF-3, RF-4). El texto del
  rechazo del almacenamiento estará en español.
- **EL SISTEMA dejará documentadas la versión de esquema vigente y la nueva guardia** —que ya no se
  limita a tablas y columnas— en la documentación viva que hoy las describe, como requisito y tarea del
  plan.
- **EL SISTEMA anotará las supersesiones que esta garantía deja obsoletas** (lista al final), como
  requisito y tarea del plan: la regla del catálogo de la 009 que permitía renombrar y eliminar el tipo
  general, la definición del esquema esperado como tablas y columnas de las specs 001, 004, 009, 011 y
  012, la nota de la 010 sobre el nombre que debía usar la siembra y sobre «sin cambio de esquema», y las
  reglas de la 008 sobre base vacía, aplicación inicial, reemplazo y recuentos.

## Requisitos no funcionales

- **RNF-1. Mensajes en español:** todo rechazo identifica la entidad, el valor recibido, la causa y, si
  existe, la acción disponible.
- **RNF-2. Protección de los datos del usuario:** ninguna operación de esta spec borra ni sobrescribe
  datos sin confirmación; toda operación destructiva conserva el respaldo obligatorio (constitución §5).
- **RNF-3. Determinismo e idempotencia:** la garantía del tipo general no depende del reloj, del azar ni
  del orden de las operaciones; repetirla no duplica filas ni cambia nombres, y dos creaciones de base
  dejan el mismo catálogo.
- **RNF-4. Alcance mínimo:** el cambio no altera el modelo de generación, el contrato, la API del
  catálogo, la huella de datos ni los datos del usuario; no introduce dependencias nuevas.
- **RNF-5. Una sola definición del literal:** el código de producción no puede tener dos definiciones del
  literal del tipo general que puedan discrepar (alcance y criterio en la definición correspondiente).
- **RNF-6. Protección no desactivable desde el producto:** ningún camino del sistema —incluida la siembra
  y el reemplazo total— desactiva la protección del tipo general.
- **RNF-7. Trazabilidad:** cada RF tendrá al menos una comprobación verificable en las tareas; sin
  cobertura, la tarea no está hecha.

## Mecanismo declarado (no es el requisito)

Las decisiones de mecanismo del usuario se declaran aquí para que el plan no las reinvente; **el
requisito es el observable** y no la técnica:

- **Alta + protección (D-1):** la fila se inserta al crear la base; dos **disparadores** cubren el
  borrado y el renombrado (incluidas las escrituras directas); los servicios traducen el rechazo. El
  observable es RF-1 (existe e inseparable), RF-3 (no se borra por ninguna vía) y RF-4 (no se renombra
  por ninguna vía).
- **Guardia (D-2 y decisión nueva):** la validez se decide por **presencia por nombre** más la fila,
  según el predicado único de «validez de la base»; el cuerpo de los disparadores queda fuera; los
  objetos de más se toleran salvo las tablas retiradas. El observable es RF-6.
- **Renombrado (H9):** la protección del almacenamiento rechaza todo cambio de nombre o de clave; una
  actualización que no cambie ninguno de los dos no la dispara, pero el **servicio** rechaza cualquier
  petición de renombrado del tipo general.

## Riesgos residuales aceptados

- **Reescribir un disparador conservando su nombre no se detecta.** La identidad es la presencia por
  nombre y el cuerpo queda fuera: una base manipulada con un disparador del mismo nombre y otro cuerpo
  pasaría la guardia. Se acepta y se documenta (decisión del 2026-11-05).
- **El contrato y la guardia miran cosas distintas.** El contrato y la validación del aula de referencia
  comparan el **nombre visible normalizado**; la guardia aplica el predicado de «validez de la base», que
  añade la **clave almacenada** de la única fila. Ese predicado es lo que las alinea; si en el futuro el
  contrato pasara a comparar la clave, esta decisión habría que revisarla.
- **Normalización duplicada.** Servicio y cálculo mantienen cada uno su función de normalización
  equivalente; RNF-5 cubre el **literal**, no el algoritmo. Si se separan en el futuro, nada lo detecta.

## Casos límite

- **Base nueva sin aplicar datos de prueba:** contiene el tipo general y su protección; se puede reabrir
  y se pueden crear aulas y materias con él (RF-1).
- **Fallo durante la creación de la base:** no queda esquema parcial; el archivo puede quedar con la
  versión en cero y el mensaje lo dice (RF-1).
- **Intento de crear un tipo «AULA DE ESTUDIO» o « aula de estudio » por el servicio:** falla por
  duplicado, con el mensaje de duplicado existente (RF-2).
- **Alta directa de una segunda fila con nombre visible canónico («Aula de estudio») y clave almacenada
  `AULA DE ESTUDIO`, con la fila canónica ya presente:** la base queda **inválida** porque hay **dos**
  filas cuyo nombre normalizado es `aula de estudio`: falla la condición de **fila única**, no la de
  clave. El índice único no la frena, porque la clave almacenada difiere (RF-2, RF-6).
- **Alta directa que deja **una sola** fila con nombre visible canónico y una clave almacenada distinta
  (sin la fila correcta):** la base queda **inválida** porque esa fila única no tiene la clave
  `aula de estudio`: falla la condición de **clave** (RF-6).
- **Intento de eliminar el tipo general sin dependientes:** rechazo con el motivo «es el tipo general»
  (RF-3).
- **Intento de eliminar el tipo general con aulas y materias que lo usan:** rechazo con el motivo «es el
  tipo general», no con la lista de dependientes (RF-3).
- **Escritura directa que intenta borrar el tipo general:** rechazo en español, y el servicio lo traduce
  si llega por su camino (RF-3, RF-7).
- **Intento de renombrar el tipo general a «Laboratorio»:** rechazo y conservación del nombre anterior
  (RF-4).
- **Intento de renombrar el tipo general a «AULA DE ESTUDIO» o « Aula de estudio »:** rechazo, porque la
  prohibición es total (RF-4).
- **Petición de renombrar el tipo general con el nombre que ya tiene:** el servicio la rechaza también,
  con su mensaje (RF-4).
- **Actualización de la fila general que solo toca la fecha:** no es un renombrado y no la bloquea la
  protección (RF-4).
- **Escritura directa que intenta renombrar el tipo general:** rechazo en español (RF-4, RF-7).
- **Intento de renombrar o eliminar un tipo especializado:** sin cambios respecto a la regla vigente
  (RF-3, RF-4).
- **Aplicar los datos de prueba sobre una base recién creada:** reutiliza el tipo general, no declara
  «Aula de estudio» y se aplica como sobre una base vacía (RF-5).
- **Reemplazo total de los datos de prueba:** borra todo menos el tipo general, que conserva su identidad
  (RF-5).
- **Aplicar el mismo perfil dos veces sobre bases vacías, o con reemplazo total:** mismos datos y
  recuentos (RF-5).
- **Aplicar el perfil sin reemplazo sobre una base con datos:** se rechaza por la regla de la 008
  (RF-5).
- **Base de la versión 6 sin el tipo general, sin uno de los dos disparadores, sin el índice o con la
  fila en forma incorrecta:** no corresponde al esquema esperado (RF-6; los momentos, en la 016).
- **Base de la versión 6 con un disparador reformateado o con objetos de más:** sigue siendo válida
  (RF-6).
- **Base con una tabla retirada presente** (`Curso_Materia`, `Recesos`): **inválida**; la tolerancia no
  alcanza a las tablas retiradas (RF-6).
- **Base creada con el esquema nuevo:** se abre de nuevo sin intervención y sigue siendo válida (RF-6).
- **Centro cuyo tipo ordinario se llama de otra forma** (por ejemplo, «Aula común»): no está soportado;
  su tipo no es el general y el renombrado del general a ese nombre se rechaza (RF-4). La limitación se
  documenta (RF-7).
- **Consumidor del catálogo que espera poder renombrar o eliminar cualquier tipo:** recibe el rechazo con
  el mensaje; su forma de leer el catálogo no cambia (RF-7, RNF-4).

## Fuera de alcance

- **Los momentos de comprobación y sus consecuencias** (al abrir, antes de generar, antes de guardar
  configuración): son de la **016**.
- **El respaldo silencioso de la interfaz y el mensaje de la validación del aula de referencia:** de la
  **016**.
- **La huella de datos de la generación y la documentación de la API del catálogo:** de la **016**.
- **La conversión de bases existentes:** no hay migración; la base del usuario en la versión 5 se rechaza
  y se recrea con respaldo.
- **Una columna, marca o rol que señale qué tipo es el general:** la identidad sigue siendo la
  convención de nombre (decisión del 2026-10-26, spec 010).
- **Vigilar el cuerpo de los disparadores:** fuera de la identidad por decisión del 2026-11-05.
- **La unicidad Unicode frente a escrituras directas:** limitación de la 009, no se corrige aquí.
- **La pantalla de administración del catálogo:** deuda del equipo de frontend.
- **Soportar centros cuyo tipo ordinario tenga otro nombre, la generación parcial, los turnos
  automáticos y los bloques de dos horas:** ajenos.

## Criterios de finalización

1. Una base nueva queda con exactamente un tipo general y su protección, de forma inseparable; si la
   creación falla **no queda esquema parcial**, el mensaje lo explica y **no se reintenta sobre el archivo
   residual** (versión en cero): se descarta y se crea una base nueva con respaldo (RF-1).
2. El tipo general se reconoce por la convención de nombre, no admite un duplicado por el servicio, la
   unicidad prometida es la de la clave almacenada —no la Unicode— y el literal está definido una sola
   vez en el código de producción (RF-2, RNF-5).
3. El tipo general no se puede eliminar por ninguna vía, el mensaje prioriza «es el tipo general» y el
   rechazo del almacenamiento se traduce al mensaje del catálogo en español (RF-3, RF-7).
4. El tipo general no se puede renombrar por ninguna vía; el servicio rechaza toda petición —incluso con
   el mismo nombre— y una actualización que no cambie nombre ni clave no se bloquea (RF-4).
5. La siembra reutiliza el tipo general, la base recién creada cuenta como vacía, el reemplazo total lo
   conserva, el recuento de tipos es 3 leído de la base y dos aplicaciones sobre bases vacías o con
   reemplazo total dan los mismos datos (RF-5).
6. La validez exige, por nombre, tablas, columnas, el índice único y los dos disparadores; exige la
   **ausencia de las tablas retiradas**; y exige el predicado de la fila, que se enuncia siempre igual:
   > «La base es válida si existe **exactamente una** fila cuyo **nombre visible normalizado** sea
   > `aula de estudio` **y esa fila (la única) tiene la clave almacenada** `aula de estudio`.»

   Con la fila canónica presente, una segunda fila con nombre canónico y clave `AULA DE ESTUDIO` falla la
   condición de **fila única**; la condición de **clave** cubre el caso de una fila única con nombre
   canónico y clave distinta. **Dos bases que difieran en la presencia de los objetos exigidos o de la
   fila no corresponden igual al esquema esperado.** El cuerpo de los disparadores y los objetos de más no
   cuentan (salvo las tablas retiradas), y si la huella del test incluye el cuerpo, es una guardia de
   release (RF-6).
7. La marca de versión sube a 6 y viaja inseparable del catálogo esperado y de la huella que lo vigila
   (RF-6).
8. La documentación de la versión y de la guardia queda al día, y las supersesiones de esta mitad están
   anotadas en las specs que correspondan (RF-7).
9. Cada RF tiene al menos una comprobación verificable en las tareas (RNF-7).

## Dudas abiertas

**Ninguna.** Las diez dudas (D-1 a D-10) y las dos decisiones nuevas del **2026-11-05** están
incorporadas; el reparto entre las dos mitades también está decidido. Queda **una cuestión diferida ajena
a esta spec**: soportar centros cuyo tipo ordinario tenga otro nombre.

## Supersesiones que esta spec declara

Normativas de esta spec; su anotación en los documentos afectados es **requisito y tarea del plan**.

- **Supersesión global:** toda descripción del **esquema esperado** como «catálogo de tablas y
  columnas» —o «tablas y columnas más la marca de versión»— queda supersedida por el predicado de
  **validez de la base** de RF-6: presencia por nombre de tablas, columnas, índice y los dos
  disparadores, ausencia de las tablas retiradas, más la marca de versión, y la fila, que se enuncia
  siempre con esta formulación:
  > «La base es válida si existe **exactamente una** fila cuyo **nombre visible normalizado** sea
  > `aula de estudio` **y esa fila (la única) tiene la clave almacenada** `aula de estudio`.»
- **001-base-datos-local:** «esquema esperado (catálogo de tablas/columnas y marca de versión)».
- **004-persistencia-campos-curso, RF-6 y su criterio de cierre:** presencia de tablas y columnas →
  tablas, columnas, índice, dos disparadores **y la fila**. Se **mantiene** la exigencia de **ausencia**
  de las tablas retiradas, que esta spec no toca.
- **009-tipos-de-aula-y-eventos, RF-5:** «permitirá crear, listar, renombrar y eliminar tipos de aula»
  deja de ser incondicional **para el tipo general**; se mantiene para los especializados, y el alta con
  el nombre del general pasa a fallar por duplicado. **Definición «Esquema esperado» y RF-9:** se ajustan
  al predicado de RF-6 (se mantiene la exigencia de ausencia del registro de recesos). **Definición
  «Catálogo de tipos de aula»:** «no puede quedar vacío mientras existan aulas o materias» pasa a «nunca
  está vacío: toda base nace con el tipo general». **RF-10:** la siembra reutiliza el tipo general, ya no
  lo declara y lo conserva en el reemplazo.
- **010-solver-tipos-y-eventos:** su nota «tipo general fijo por nombre, **sin cambio de esquema** ni de
  contrato (2026-10-26)» y su definición de «tipo general» («el nombre que **debe usar la semilla** del
  catálogo») quedan supersedidas en la parte del esquema y de la siembra: la existencia y el nombre pasan
  a estar garantizados por la base y la siembra los reutiliza. Se mantienen la convención de nombre, la
  regla del aula de referencia y el contrato 2.0; su mensaje se trata en la 016.
- **011-horas-academicas y 012-preferencias-docente:** su marca de esquema (v4 y v5) y su descripción del
  catálogo esperado pasan a v6 y al predicado de RF-6; se mantiene su política sin migración.
- **008-datos-de-prueba (por dependencia de RF-5, no por simetría):** la definición «Base vacía» —la fila
  del tipo general no convierte la base en «con datos»— y **RF-10/RF-12** (aplicación inicial, reemplazo
  y recuentos: 3 tipos leídos de la base). Sin esta anotación, la 008 contradiría lo que esta spec
  implementa.
- **013-preferencias-docente-solver:** sus referencias a la versión de esquema se ajustan en la **016**
  (mitad 2), que es donde vive la coherencia con esa spec.

## Referencias de la decisión (no normativas)

- Convención del tipo general: decisión del **2026-10-26** (spec 010) y validación del aula de referencia
  vigente.
- Catálogo anterior: alta, renombrado libre y borrado solo si estaba en uso (spec 009, RF-5).
- Creación de la base: antes solo estructura; el catálogo nacía vacío (spec 009, RF-9).
- Siembra anterior: creaba «Aula de estudio», «Laboratorio» y «Taller», contaba la tabla de tipos como
  dominio y su reemplazo borraba el catálogo (specs 008 y 009, RF-10).
- Guardia anterior: solo tablas y columnas; la huella congelada de las pruebas no vigilaba índices ni
  disparadores (incidente del 2026-10-17).
- Precedente de fila creada con la base: la configuración global de la hora académica (spec 011).
