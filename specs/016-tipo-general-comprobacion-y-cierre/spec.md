# Spec 016 - Comprobación y cierre del tipo general de aula

Estado: implementada (2026-11-05)

> **015 y 016 son las dos mitades de la antigua 015 (decisión del usuario, 2026-11-05, al cerrar el
> re-QA).** Esta spec es la **mitad 2: comprobación y cierre**, y **depende de la 015**
> (`specs/015-tipo-general-en-el-esquema/`), que es la **mitad 1: la garantía estructural**. **La 015
> precede a la 016**: aquí se consume lo que la 015 deja en el esquema (la fila del tipo general, su
> protección y el predicado de **validez de la base**), y **no se redefine nada de eso**.
>
> **Qué entra aquí, y por qué.** Lo que hace que la garantía **se use**: **cuándo se comprueba** la
> validez (al abrir, antes de generar y antes de guardar la configuración) y **qué ocurre** en cada
> momento; el **frontend sin respaldo silencioso**; el **mensaje** de la validación del aula de
> referencia; el **tipo general fuera de la huella de datos**; la **documentación de la API** del
> catálogo; y las **supersesiones restantes**, incluida la coherencia con la **013**.
>
> **Decisiones cerradas.** Las diez dudas (D-1 a D-10) y las dos decisiones nuevas del **2026-11-05**
> —validez por presencia por nombre + la fila, y comprobación al abrir, antes de generar y antes de
> guardar, sin respaldo silencioso— se heredan de la 015 y **no se reinterpretan**. La spec **no tiene
> requisitos condicionales**: se **aprobó el 2026-11-05** y se **cerró como implementada** el mismo día
> (T1–T9, veredicto **APROBADO**).
>
> **Origen.** Con la 015, la base garantiza el tipo general y lo protege. Quedaban cuatro huecos que
> rompen esa garantía en el uso diario: (1) nada obligaba al producto a **negarse a seguir** con una base
> inválida fuera del arranque; (2) la interfaz, cuando no encontraba el tipo general, **usaba el primer
> tipo del catálogo** como si fuera él; (3) el rechazo de un aula de referencia no general **culpaba al
> aula** cuando el problema podía estar en el catálogo; y (4) la documentación de la API seguía
> describiendo un catálogo sin protección y una versión de esquema anterior.
>
> **Orden de trabajo (decisión del usuario, 2026-11-05).** La 015 y esta 016 **se implementan antes** que
> la **013** (preferencias del docente, aprobada y sin implementar) y antes que la **014** (materias en
> bloques de dos horas, planificada).
>
> **Número.** La **014** sigue reservada para los bloques de dos horas y no se ha escrito.

## Contexto y objetivo

El cálculo exige que el aula de referencia de cada curso sea del **tipo general** del centro, y la 015
deja ese tipo garantizado y protegido en el esquema. Pero una garantía que solo se comprueba al arrancar
se pierde en cuanto la base se manipula, se restaura un respaldo o se llega por otro camino: el producto
puede **generar** o **guardar configuración** con una base que ya no cumple, y el error aparecerá más
tarde, atribuido a otra cosa.

Además, la interfaz tiene su propio atajo: cuando una alta rápida necesita el tipo general y no lo
encuentra en el catálogo, **toma el primero de la lista**. Es un respaldo silencioso: la operación se
guarda con un tipo que el cálculo nunca reservará, y el problema se descubre al generar.

El objetivo de esta mitad es que **la validez de la base se compruebe donde importa** —al abrir, antes de
generar y antes de guardar la configuración— y que, cuando falle, **el producto no siga**: ni migre, ni
repare, ni escriba, ni calcule, ni guarde; que la interfaz **falle con un mensaje** en lugar de sustituir
el tipo; que el mensaje del aula de referencia **explique la convención**; y que la documentación y las
specs afectadas **dejen de contradecir** lo decidido.

**Qué no es.** No es la garantía estructural (015), ni un cambio del modelo del solver, del contrato o
de la API del catálogo, ni una migración de datos, ni una validación nueva, ni una pantalla de
administración.

## Usuarios

- **Usuario académico:** espera que si su base tiene un problema, el sistema se lo diga **antes** de
  calcular o de guardar, y no después, cuando el horario ya no cuadra.
- **Usuario que mantiene aulas y materias:** espera que sus altas no se guarden con un tipo que el
  cálculo no reconoce.
- **Equipo de desarrollo:** necesita que la comprobación sea una sola, barata y ejecutada en los tres
  momentos, y que la huella de datos siga sin cambiar por el tipo general.
- **Equipo de frontend:** necesita saber que sus formularios no cambian de forma y que el atajo del tipo
  general ya no puede sustituirlo.
- **Equipo de pruebas y QA:** necesita poder afirmar cuándo una base inválida impide abrir, generar y
  guardar, y que eso está cubierto por pruebas.

## Historias de usuario

- HU-1. Como usuario, quiero que el sistema me impida generar o guardar con una base inválida y me diga
  qué hacer, para no perder tiempo con un resultado que no sirve.
- HU-2. Como usuario que da de alta aulas y materias, quiero que la operación falle si el tipo general no
  está, en vez de guardarse con otro tipo.
- HU-3. Como miembro del equipo de desarrollo, quiero que la comprobación sea una sola y barata, para
  poder llamarla al abrir, al generar y al guardar sin coste.
- HU-4. Como miembro del equipo de frontend, quiero un mensaje claro y ningún cambio de firma, para
  adaptar mis vistas sin rehacer los formularios.
- HU-5. Como miembro del equipo de desarrollo, quiero que el mensaje del aula de referencia explique la
  convención, para no culpar al aula cuando el problema está en el catálogo.
- HU-6. Como responsable de la planificación, quiero que la 013 quede correctamente referenciada a la
  versión 6, para no planificarla sobre una base que ya no existe.

## Definiciones

- **Validez de la base:** el predicado de la **015 (RF-6)**: presencia por nombre de tablas, columnas,
  índice y los dos disparadores; ausencia de las tablas retiradas; y la fila del tipo general, que se
  enuncia **siempre con esta formulación**:
  > «La base es válida si existe **exactamente una** fila cuyo **nombre visible normalizado** sea
  > `aula de estudio` **y esa fila (la única) tiene la clave almacenada** `aula de estudio`.»

  El cuerpo de los disparadores no forma parte de la identidad y los objetos de más se toleran (salvo las
  tablas retiradas). **Esta spec no lo redefine.**
- **Comprobación de validez:** la consulta, **de solo lectura y barata**, que aplica ese predicado sobre
  una base abierta. No recorre datos de dominio ni escribe.
- **Respaldo silencioso:** el comportamiento de la interfaz que, al no encontrar el tipo general en el
  catálogo, **usa otro tipo** para completar un alta, de modo que la operación parece correcta y el
  cálculo no reconocerá después el tipo.
- **Aula de referencia:** la que el curso declara para las clases que no se desplazan (spec 005/010);
  debe ser del tipo general.
- **Huella de datos:** el resumen determinista de lo que interviene en una generación (specs 004, 009,
  011 y 012). El tipo general **no** forma parte de ella.
- **Deuda de la 009 sobre la ayuda del tipo general:** la guía de integración (RF-11 y §2.4.3) documentó
  que resolver el tipo general era una ayuda de la interfaz; esa ayuda **se cierra en esta spec** con un
  comportamiento estricto.

## Requisitos funcionales

### RF-1. Cuándo se comprueba la validez y qué ocurre si falla

- **CUANDO se abra una base, EL SISTEMA comprobará su validez. SI no es válida, ENTONCES la apertura
  fallará** con la causa concreta y **no se migrará, no se reparará y no se escribirá nada** en la base.
- **CUANDO se vaya a generar un horario, EL SISTEMA comprobará la validez de la base antes de calcular.
  SI no es válida, ENTONCES no invocará al cálculo** y devolverá un mensaje en español con la causa y la
  acción.
- **CUANDO se vaya a guardar la configuración de generación, EL SISTEMA comprobará la validez de la base.
  SI no es válida, ENTONCES rechazará el guardado** con un mensaje en español y no escribirá la
  configuración.
- **Las tres comprobaciones serán la misma y de solo lectura**, aptas para ejecutarse en cada apertura,
  generación y guardado, sin recorrer datos de dominio.
- **CUANDO la causa del fallo sea la versión, el sistema ofrecerá crear una base nueva** y advertirá de
  que restaurar un respaldo de la versión anterior volverá a fallar.
- **CUANDO la causa sea la falta de un objeto esperado, de la fila o la presencia de una tabla retirada,
  el sistema ofrecerá restaurar un respaldo de la versión vigente o crear una base nueva** con respaldo
  obligatorio (specs 001 y 004).
- **SI una base de la versión 5 (o anterior) se abre con esta compilación, ENTONCES el resultado es un
  fallo de apertura**: su contenido no se migra ni se copia automáticamente.
- **EL SISTEMA no dará por válida una base a la que se le haya quitado el tipo o la protección por una
  escritura ajena al producto**, tampoco en los caminos de generación y guardado.

> **Anotado por la 019 (2026-11-10).** La **versión de esquema vigente pasa a 7** por la 019 (materias
> suplentes). Se mantiene todo lo de esta spec: la comprobación única de solo lectura, los **tres
> momentos** (abrir, generar y guardar), el **mensaje de versión** que ofrece crear una base nueva y
> advierte de que restaurar un respaldo anterior volverá a fallar, y la política **sin migración**. La 019
> fija ese mismo comportamiento para una **base v6**.

### RF-2. La interfaz no sustituye el tipo general

- **CUANDO la interfaz prepare un alta que necesite el tipo general y no lo encuentre en el catálogo,
  ENTONCES la operación fallará con un mensaje en español y no se guardará nada.**
- **EL SISTEMA no usará otro tipo en su lugar**: la ayuda que resuelve el tipo general **dejará de
  devolver el primero del catálogo** y solo devolverá el tipo general o un fallo.
- **El mensaje indicará qué falta y qué hacer** (que la base no tiene el tipo general y que hay que
  restaurar un respaldo o crear una base nueva), sin errores técnicos crudos.
- **La conducta valdrá para toda alta que use la ayuda**, no solo para una vista: si una operación no
  puede determinar el tipo general, no elige otro.
- **EL SISTEMA conservará la forma de leer el catálogo**: mismas firmas y mismos campos; el cambio es de
  comportamiento, no de contrato.
- **La deuda de la 009 sobre esta ayuda queda atendida aquí**, y su documentación se actualiza (RF-5).

### RF-3. El mensaje del aula de referencia

- **EL SISTEMA seguirá exigiendo que el aula de referencia de cada curso sea del tipo general**, con la
  misma regla y el mismo momento de validación que hoy.
- **EL SISTEMA no añadirá validaciones nuevas al contrato** por este cambio: la cadena de validaciones
  conserva sus identificadores y el siguiente hueco queda reservado para la 013.
- **CUANDO se rechace un aula de referencia que no es del tipo general, EL SISTEMA explicará la
  convención y la acción**: que el tipo general del centro es el de nombre `aula de estudio`, que el aula
  recibida es de otro tipo y qué hacer (asignar al curso un aula del tipo general y comprobar que la base
  es válida).
- **EL SISTEMA no cambiará qué aulas puede reservar el cálculo**: el tipo general sigue siendo el de las
  aulas de referencia y los tipos especializados siguen sin reservarse.

### RF-4. El tipo general queda fuera de la huella de datos

- **EL SISTEMA mantendrá el tipo general fuera de la huella de datos de la generación**: es una constante
  del esquema, igual en toda base y sin cambios entre generaciones.
- **EL SISTEMA no marcará «datos anteriores» a un horario por la existencia del tipo general** ni por su
  nombre visible (que no puede cambiar).
- **EL SISTEMA seguirá reflejando en la huella cualquier cambio que altere las entradas de la
  generación**, incluidas las asignaciones de tipo de aulas y materias, como hoy.

### RF-5. Documentación de la API y de la deuda de la 009

- **EL SISTEMA actualizará la guía de integración del frontend** —la sección del catálogo de tipos— con
  la conducta nueva: qué operaciones se rechazan, con qué mensaje, que la base nueva ya trae el tipo
  general, que la ayuda del tipo general **falla** en vez de sustituirlo y que no hay campos ni firmas
  nuevas.
- **EL SISTEMA anotará en esa guía que la deuda de la 009 sobre la ayuda del tipo general queda
  atendida** por esta spec, y retirará la descripción anterior.
- **EL SISTEMA documentará la convención del tipo general y la nueva lectura de la validación del aula
  de referencia** en la documentación viva del contrato, sin cambiar el contrato ni su versión.
- **EL SISTEMA dejará constancia de que la comprobación de validez se aplica al abrir, al generar y al
  guardar**, para que el consumidor sepa cuándo puede recibir el rechazo.

### RF-6. Coherencia con la spec 013

- **EL SISTEMA dejará la 013 planificada sobre la versión de esquema 6**, no sobre la 5, y anotará
  expresamente los tres puntos afectados: su **`Base` `bb1767a`**, su **ancla de no regresión** (el
  escenario de datos de prueba y su preset vigentes en `bb1767a`) y la restricción **«Esquema (v5)»** de
  su plan.
- **EL SISTEMA no cambiará los requisitos de comportamiento de la 013**: qué hace con las preferencias,
  sus RF y sus criterios quedan intactos.
- **EL SISTEMA sí re-basará su `Base` y el ancla de verificación de su RNF-4 y de su criterio 11 al
  escenario posterior a la 015** (base en la versión 6, tipo general garantizado por el esquema y perfil
  de datos de prueba sin «Aula de estudio»). No es un cambio de comportamiento, pero **sí es un cambio de
  la referencia contra la que se verifica**, y se anota como supersesión de ese ancla, no como «solo
  referencias de planificación».
- **EL SISTEMA no tocará la huella de esquema de la 013** porque **no tiene una propia**: la única
  congelada es la del test de versión del esquema, que actualiza la 015.

## Requisitos no funcionales

- **RNF-1. Mensajes en español:** todo rechazo identifica la entidad, el valor recibido, la causa y, si
  existe, la acción disponible.
- **RNF-2. Comprobación barata:** la comprobación es de solo lectura, no recorre datos de dominio y puede
  ejecutarse en cada apertura, generación y guardado.
- **RNF-3. No escribir al comprobar:** ni la apertura ni la generación ni el guardado modifican la base
  cuando la validez falla; la apertura tampoco repara.
- **RNF-4. Alcance mínimo:** no cambian el modelo de generación, el contrato, la API del catálogo, la
  huella de datos ni los datos del usuario.
- **RNF-5. Determinismo:** la comprobación y los mensajes no dependen del reloj, del azar ni del orden de
  las operaciones.
- **RNF-6. Trazabilidad:** cada RF tendrá al menos una comprobación verificable en las tareas; sin
  cobertura, la tarea no está hecha.

## Mecanismo declarado (no es el requisito)

- **Comprobación única y reutilizada:** una sola función de validez (la de la 015), llamada en los tres
  momentos. El observable son los tres rechazos de RF-1, no la técnica.
- **Interfaz estricta:** la ayuda que resuelve el tipo general devuelve el tipo o un fallo; nunca otro
  tipo. El observable es RF-2.
- **Mensaje del aula de referencia:** se mejora el texto de la validación existente; no se añade una
  validación nueva (decisión del usuario del 2026-11-05, que reserva el identificador siguiente para la
  013).

## Riesgos declarados

- **Un camino sin comprobar.** La validez debe comprobarse en **tres** puntos y no solo en el arranque;
  si el guardado de configuración es un archivo y no la base, el plan debe fijar dónde se comprueba para
  no dejarlo fuera.
- **La interfaz no puede distinguir nombre y clave.** El catálogo publica id y nombre visible; la ayuda
  compara el nombre. Como el renombrado está prohibido (015), coincide en la práctica; si una escritura
  directa dejase la fila con clave válida y otro nombre, la guardia invalidaría la base (015, RF-6) pero
  la interfaz no encontraría el tipo y, con esta spec, fallaría en vez de sustituirlo.
- **Mensaje del almacenamiento.** El rechazo que produce la protección de la base debe llegar traducido;
  si algún camino del frontend escribe directamente, verá el texto del disparador (en español) o un error
  genérico. La 015 fija la traducción; aquí solo se verifica en los tres momentos.
- **La 013 condicionada.** Hasta que la 015 esté implementada, la 013 no puede replanificarse; el plan de
  la 016 debe fijar el momento de actualizar sus referencias.

## Casos límite

- **Apertura de una base sin el tipo general, sin un disparador, sin el índice o con una tabla
  retirada:** fallo de apertura con causa y acción; no se repara ni se escribe (RF-1).
- **Apertura de una base de la versión 5 o anterior:** fallo de apertura; la acción es crear base nueva,
  porque un respaldo de la versión anterior volverá a fallar (RF-1).
- **Generación con una base inválida:** no se invoca al cálculo; mensaje con causa y acción (RF-1).
- **Guardado de la configuración con una base inválida:** se rechaza sin escribir (RF-1).
- **Alta rápida de un aula, una materia o una asignación sin tipo general en el catálogo:** falla con
  mensaje; no se guarda nada y **no** se usa otro tipo (RF-2).
- **Catálogo sin el tipo general pero con otros tipos:** la ayuda no toma el primero; falla (RF-2).
- **Fallo de lectura del catálogo al preparar un alta:** falla con mensaje; no se sustituye el tipo
  (RF-2).
- **Generación con un aula de referencia de un tipo especializado:** rechazo con el mensaje mejorado, que
  explica la convención y la acción (RF-3).
- **Generación con todas las aulas de referencia del tipo general:** funciona como hoy (RF-3).
- **Preset 2.0 anterior a este cambio:** sigue siendo válido (RF-3).
- **Existencia del tipo general antes y después de una generación:** la huella no cambia por ello (RF-4).
- **Cambio del tipo asignado a un aula o a una materia entre el envío y la respuesta:** la huella cambia
  y el horario se marca «datos anteriores» (RF-4).
- **Consumidor que esperaba poder renombrar o eliminar cualquier tipo:** recibe el rechazo con el mensaje;
  su forma de leer el catálogo no cambia (RF-1, RF-2, RF-5).
- **Planificación de la 013 antes de cerrar la 015:** no es válida; sus referencias se actualizan después
  de la 015 (RF-6).

## Fuera de alcance

- **La garantía estructural** —fila, protección, siembra, validez y guardia v6—: es de la **015**.
- **La migración de bases existentes:** no hay migración (RF-1).
- **Una validación nueva del contrato:** el identificador siguiente queda reservado para la 013 (RF-3).
- **El modelo del solver, la regla de reserva y el contrato:** no cambian; solo el mensaje (RF-3).
- **La pantalla de administración del catálogo:** sigue siendo deuda del equipo de frontend; esta spec
  solo cierra la ayuda del tipo general y documenta.
- **Soportar centros cuyo tipo ordinario tenga otro nombre, la generación parcial, los turnos automáticos
  y los bloques de dos horas:** ajenos.

## Criterios de finalización

1. La validez de la base se comprueba al abrir, antes de generar y antes de guardar configuración; si
   falla, no se migra, no se repara, no se escribe, no se calcula y no se guarda, con causa y acción
   separadas para versión y para objetos/fila/tabla retirada (RF-1, RNF-2, RNF-3).
2. La interfaz que necesita el tipo general **falla con mensaje** cuando no lo encuentra y **nunca usa
   otro tipo**; la conducta vale para todas las altas que la usan (RF-2).
3. El mensaje del aula de referencia explica la convención y la acción, sin validaciones nuevas ni cambio
   de contrato (RF-3, RNF-4).
4. El tipo general queda fuera de la huella de datos y su existencia no marca «datos anteriores» (RF-4).
5. La guía de la API del catálogo documenta la protección, el fallo de la ayuda y la ausencia de campos
   nuevos, y anota que la deuda de la 009 queda atendida (RF-5).
6. La 013 queda **re-basada** al escenario posterior a la 015 en sus tres puntos (`Base`, ancla de
   verificación de su RNF-4/criterio 11 y restricción «Esquema (v5)»), **sin cambiar sus requisitos de
   comportamiento**, y se declara que no tiene huella propia (RF-6).
7. Cada RF tiene al menos una comprobación verificable en las tareas (RNF-6).

## Dudas abiertas

**Ninguna.** Las decisiones del 2026-11-05 están heredadas de la 015 y el reparto entre las dos mitades
está decidido. Queda **una cuestión diferida ajena a esta spec**: soportar centros cuyo tipo ordinario
tenga otro nombre.

## Supersesiones y notas documentales que esta spec declara

Normativas de esta spec; su anotación es **requisito y tarea del plan**.

- **013-preferencias-docente-solver:** **no se cambian sus requisitos de comportamiento** (qué hace con
  las preferencias, sus RF y sus criterios). **Sí se re-basa** su **`Base` `bb1767a`**, su ancla de
  verificación de **RNF-4 y criterio 11** y la restricción **«Esquema (v5)»** de su plan al escenario
  posterior a la 015 (versión 6, tipo general garantizado por el esquema y perfil sin «Aula de estudio»).
  **Nota de supersesión del ancla:** el ancla de verificación fijada en `bb1767a` queda **supersedida** por
  el escenario posterior a la 015; la 013 conserva su RNF-4 y su criterio 11, pero se verifican contra el
  ancla nueva. No se toca ninguna huella «de la 013»: no tiene una propia.
- **009-tipos-de-aula-y-eventos, RF-11 y §2.4.3 de la guía:** la deuda sobre la ayuda del tipo general
  queda **atendida**; la ayuda ya no sustituye el tipo.
- **010-solver-tipos-y-eventos y 005 (mensaje de la validación del aula de referencia):** la regla y su
  identificador **no cambian**; solo el texto del mensaje y su documentación en el contrato. No hay
  supersesión de requisito.
- **Documentación viva:** `docs/interfaz-frontend.md` §2.4.3 (catálogo) y `docs/contrato-solver-2.0.md`
  (convención del tipo general y lectura del mensaje). La **versión de esquema y la guardia** las
  documenta la **015**.

## Referencias de la decisión (no normativas)

- Validez de la base y guardia: **015, RF-6**; la fila, la protección y la siembra, **015, RF-1 a RF-5**.
- Aula de referencia y tipo general por convención: decisiones del 2026-10-26 (spec 010) y validación
  vigente.
- Respaldo silencioso: la ayuda de tipos de la interfaz devuelve hoy el primer tipo del catálogo cuando no
  encuentra el general, y tres vistas la usan para altas rápidas.
- Huella de datos: resume docentes, aulas (con su identidad de tipo), materias, planes, cursos, turnos,
  eventos y la duración; no incluye el catálogo.
- Deuda de la 009: su RF-11 y la guía de integración documentaron la ayuda del tipo general como deuda de
  la interfaz.
