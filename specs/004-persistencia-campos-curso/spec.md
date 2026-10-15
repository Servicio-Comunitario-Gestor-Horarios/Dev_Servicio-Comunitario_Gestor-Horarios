# Spec 004 - Persistencia y servicios de los cursos

Estado: implementada

> **Cambio de requisito (modelo de dominio, 2026-10-15).** Tras aprobar las specs 004 y 005, el
> modelo del curso cambió: se **retiran de la base** el nombre, el número de estudiantes, el código
> del plan y el indicador de aula fija por curso; se **elimina la relación persistida curso–materia**
> (las materias del curso se derivan del plan, en la configuración); la **migración se colapsa a un
> único esquema** (sin pasos versionados); y el **modo de aula fija pasa a ser global** de la
> configuración (spec 005). Esta spec se rehízo para reflejarlo y volvió a **borrador**; fue
> **re-revisada y re-aprobada el 2026-10-15** (`Estado: aprobada`) y su `plan.md`/`tasks.md` se
> regeneraron en consecuencia.
>
> **Reencuadre de alcance (2026-10-15).** Esta spec queda como la **spec de la base de datos y sus
> servicios** (persistencia de cursos, esquema único, cascadas, huella, contrato de consumo y consola);
> la spec **005** queda con el **solver y todo lo suyo**, incluida la **construcción de la entrada del
> solver** desde los datos persistidos más el preset. Se cierran además las dudas abiertas (turno
> opcional, baja de aula referenciada).
>
> **Ajuste de alcance (2026-10-15).** Se **mueve la construcción de la entrada del solver** (leer los
> datos persistidos y el preset y producir el JSON del solver) de esta spec (004) a la spec **005**,
> para que 004 quede como **persistencia y servicios** puros y la entrada viaje **con el solver**. Se
> **retira de esta spec el RF de construcción de la entrada** (antes RF-7) y sus referencias; los RF
> quedan en **RF-1..RF-6**. La frontera se mantiene: **el solver nunca abre la base**; el **algoritmo,
> la forma del contrato y su serialización son de 005**.
>
> **Nota de supersesión del versionado de esquema (2026-10-15).** Esta spec **supersede** el
> **versionado de esquema y la migración por versión** de las specs **001** y **003**, adoptando un
> **esquema único** sin pasos versionados. Quedan **anulados**: en **001**, la parte de **RF-1** que
> migra una base «de versión anterior» (aplicar migraciones, respaldo previo a la migración, estado
> «migrando») y toda noción de «versión de esquema anterior»; en **003**, la referencia de **RF-1** a
> dejar la «versión de esquema actual» al crear la base y **RF-3** (aviso de migración en marcha),
> que ya no aplica. **Se mantienen vigentes** en 001: el arranque, la creación de una base
> inexistente, el **respaldo obligatorio** para operaciones destructivas, el **diálogo de fallo de
> apertura** (RF-4), la **instancia única** (RF-5), la **cascada con confirmación** (RF-2) y los
> **cambios pendientes** (RF-3, RF-6); y en 003: **crear base nueva con respaldo** (RF-1), **distinguir
> error de «sin datos»** (RF-2), **enfoque de la instancia** (RF-4) y **mensajes en español** (RF-5).
> El «esquema esperado» se describe de forma comprobable en RF-6 de esta spec.

## Contexto y objetivo

Cada curso se identifica por su **año** (`anio`, entero) y su **sección** (`seccion`, un carácter);
el nombre se **deriva** de ellos para mostrarlo. Para que el contrato del solver (spec 005) funcione
con datos reales, los datos propios del curso —identidad, turno y aula— deben **persistirse** en la
base del cliente y **exponerse** en el dominio.

El modelo nuevo separa con claridad qué es **catálogo estable** (lo que se persiste) de qué es
**parámetro de cálculo** (lo que vive en la configuración del solver). Permanecen en la base sólo los
datos que identifican y ubican al curso (año, sección, turno, aula); el resto —plan, número de
estudiantes, materias y el modo de aula— pasa a la configuración del solver (spec 005).

Esta spec es la **spec de la base de datos y sus servicios**: cubre la **persistencia** de los datos
del curso, los **servicios** que los registran, consultan, listan y actualizan, el **colapso de la
migración a un único esquema**, la **cascada** y la **huella de datos**, el **contrato observable**
que se expone a quien consume los datos y la **consola de datos**. **No** cubre el **modelo del
solver**, ni la **construcción de la entrada del solver a partir de los datos persistidos más el
preset**, ni el **contrato de configuración del solver** (la forma del modelo de datos y su
serialización), ni la **gestión de los presets** como archivos: todo eso pertenece a la **spec 005**.

**Frontera con la spec 005.** La **construcción de la entrada** —leer los datos persistidos y el
preset y producir la configuración del solver— y la **forma de ese contrato** (el modelo de datos del
solver y su serialización) viven en la **spec 005**. Esta spec (004) solo **persiste y expone** los
datos que esa construcción consume. El **solver nunca abre la base** (constitución §7); la entrada se
arma con datos ya persistidos y el preset.

Referencias: `docs/constitution.md` §7 (el solver y su contrato) y §5 (protección de los datos del
usuario).

## Usuarios

- **Usuario académico:** mantiene cursos, aulas, planes de estudio y turnos, y espera que sus datos se
  guarden y se devuelvan de forma coherente.
- **Usuario que registra y edita cursos:** indica año, sección, turno y aula, y no escribe el nombre
  a mano; espera que sus cursos se guarden y se devuelvan con esos datos.
- **Usuario que consulta los cursos:** ve y cambia los datos del curso y espera un orden predecible.
- **Equipo de desarrollo:** mantiene los servicios y el dominio; necesita que los datos se persistan
  y se expongan de forma coherente, y que el esquema se cree de una vez sin pasos versionados.

## Historias de usuario

- HU-1. Como usuario que registra cursos, quiero indicar el año y la sección y que el nombre se
  genere solo, para no depender de nombres escritos a mano ni de duplicados.
- HU-2. Como usuario que consulta o edita un curso, quiero ver y cambiar su año, su sección, su turno
  y su aula, para mantener los cursos ordenados y ubicados.
- HU-3. Como usuario que consulta los cursos, quiero que las representaciones de dominio expongan los
  datos del curso, para que otras piezas los consuman sin volver a capturarlos.
- HU-4. Como usuario que mantiene la aplicación, quiero que la base se cree con un único esquema, para
  no arrastrar versiones ni pasos de migración que ya no existen.

## Definiciones

- **Año:** nivel del curso; entero mayor o igual que 1.
- **Sección:** carácter que distingue cursos del mismo año (se detalla su normalización y validación
  en RF-3).
- **Nombre derivado del curso:** texto formado por el **ordinal del año** (`1→"1ro"`, `2→"2do"`,
  `3→"3ro"`, el resto `→"to"`), un espacio y la sección (por ejemplo `"1ro A"`, `"4to B"`). Es
  **presentación**: lo calcula quien muestra (la interfaz de usuario o cualquier consulta de datos) a
  partir del año y la sección; **no** es un dato almacenado, ni de identidad, ni viaja en el dominio
  del curso.
- **Aula del curso:** referencia al aula concreta del registro de aulas, **opcional**; se usa cuando
  el **modo de aula fija** (global, de la configuración del solver) está activo. **Se persiste con
  independencia del modo** (el modo decide sólo su uso en el cálculo). No se persiste ningún índice de
  aula. Al registrar o actualizar el curso, el aula referenciada debe **existir** en el registro de
  aulas. **Si el aula referenciada se elimina, el usuario ve primero los cursos dependientes y
  confirma; si confirma, esos cursos quedan sin aula (referencia nula).**
- **Modo de aula fija (global):** parámetro **de la configuración del solver** (spec 005), más
  concretamente del **preset**, no de la base: activo → cada curso usa su aula; inactivo → los cursos
  son móviles y el solver les asigna aula. Esta spec sólo persiste la **referencia de aula** del curso.
- **Materias del curso:** materias y horas semanales que el curso cursa; se **derivan de su plan** en
  la configuración del solver (spec 005). **No** existe una relación persistida curso–materia.
- **Relación persistida curso–materia:** vínculo que antes enlazaba cada curso con sus materias y
  horas; **desaparece** en el modelo nuevo.
- **Huella de datos:** resumen determinista (una función *hash*) de los datos persistidos que
  intervienen en una generación —los cursos con su identidad, su turno y su aula, los docentes, las
  aulas, las materias, los planes de estudio y los turnos/recesos—. Se calcula antes de generar y se
  vuelve a calcular al recibir el resultado; si difieren, el horario se marca «datos anteriores» y
  exige confirmación explícita del usuario antes de guardarlo, porque los datos cambiaron mientras se
  calculaba. **No** incluye las materias del curso (no se persisten; se derivan del plan, spec 005).
- **Esquema esperado:** el esquema único del modelo nuevo, reconocido de forma comprobable por la
  presencia de la totalidad del catálogo de tablas y columnas del modelo y por la coincidencia de la
  marca de versión de esquema esperada (RF-6).
- **Años de un docente:** lista de años a los que puede dictar; vacía significa cualquiera. Es
  **dato de la configuración/preset** del solver (spec 005); **no** se persiste en la base.

## Requisitos funcionales

### RF-1. Representación persistida del curso: identidad, unicidad y aula

- **EL SISTEMA persiste cada curso con su año, su sección, su turno (opcional) y la referencia a su
  aula (opcional).** El formato de año y sección se valida en RF-3.
- **EL SISTEMA deja de almacenar el nombre del curso:** el nombre se deriva de año y sección cuando
  deba mostrarse.
- **EL SISTEMA deja de almacenar el número de estudiantes, el código del plan de estudios y un
  indicador de aula fija por curso:** el plan y el número de estudiantes son datos de la
  configuración del solver (spec 005) y el modo de aula es global (spec 005).
- **EL SISTEMA conserva el identificador propio de cada curso como clave principal (subrogada);** la
  unicidad año+sección es una **restricción adicional**, no la clave principal.
- **EL SISTEMA garantiza la unicidad de año+sección en todo el sistema:** no puede haber dos cursos
  con el mismo año y la misma sección.
- **SI se intenta registrar o actualizar un curso con un año+sección ya existentes, ENTONCES EL
  SISTEMA rechazará la operación** indicando la causa.
- **MIENTRAS exista un curso, EL SISTEMA garantiza que no hay otro con su mismo año+sección:** en
  ningún momento existe un estado con dos cursos del mismo año y sección, aunque la operación soslaye
  la validación de la aplicación.
- **EL SISTEMA persiste la referencia de aula del curso con independencia del modo de aula fija (spec
  005):** el modo decide sólo si esa aula se usa en el cálculo; no persiste el índice de aula.
- **SI se intenta registrar o actualizar un curso con una referencia de aula, ENTONCES EL SISTEMA
  exigirá que el aula referenciada exista en el registro de aulas; SI la referencia no existe,
  ENTONCES rechazará la operación indicando la causa.** Un curso sin aula (referencia ausente) es
  siempre válido.
- **SI se elimina un aula referenciada por uno o más cursos, ENTONCES EL SISTEMA mostrará primero los
  cursos dependientes y pedirá confirmación explícita; SI el usuario confirma, ENTONCES dejará esos
  cursos sin aula (referencia nula); SI cancela, ENTONCES no eliminará el aula.**

### RF-2. Servicios de cursos

- **EL SISTEMA conserva el año, la sección, el turno y el aula al registrar, consultar, listar y
  actualizar un curso.**
- **CUANDO se consulte o se liste un curso, EL SISTEMA devolverá su año, su sección, su turno y su
  aula.**
- **SI un curso no declara turno, ENTONCES EL SISTEMA lo conservará sin turno; SI no declara aula,
  ENTONCES EL SISTEMA conservará la referencia de aula ausente.**
- **EL SISTEMA deja de ofrecer operaciones para asignar, quitar o consultar las materias de un
  curso:** esas operaciones no existen; las materias se derivan del plan en la configuración (spec
  005).
- **EL SISTEMA ordena la lista de cursos por año ascendente y, dentro del mismo año, por sección** en
  orden alfabético insensible a mayúsculas y coherente con el idioma del usuario.
- **EL SISTEMA ofrece la consulta de cursos con año, sección, turno y aula de cada curso, y no ofrece
  operaciones curso–materia ni entrega un nombre almacenado;** si quien muestra decide mostrar el
  nombre, lo deriva de año y sección (RF-4).

### RF-3. Validación y normalización de la identidad del curso

- **SI el año no es un entero mayor o igual que 1, ENTONCES EL SISTEMA rechazará la operación** con un
  mensaje que identifique el campo y el valor rechazado. (La spec no fija cota superior.)
- **EL SISTEMA recorta los espacios de la sección y la normaliza a mayúsculas antes de compararla o
  guardarla**, de modo que `" a "` y `"A"` son la misma sección.
- **SI la sección, tras recortar espacios y normalizar a mayúsculas, no es exactamente un carácter,
  ENTONCES EL SISTEMA rechazará la operación** con un mensaje que identifique el campo y el valor
  rechazado. Esto incluye el caso en que la normalización produzca **más de un carácter** (por
  ejemplo, un carácter que al pasar a mayúsculas se expanda a dos, como `ß`→`SS`): se rechaza. Un
  único carácter no ASCII que siga siendo único tras la normalización (por ejemplo `Ñ` o `Á`) es
  válido.
- El rechazo por duplicado año+sección **no** se repite aquí: es RF-1.

### RF-4. Exposición de los datos del curso en el dominio

- **EL SISTEMA expone en el dominio de cada curso su año, su sección, su turno y su aula** (la
  referencia a su aula, o su ausencia).
- **EL SISTEMA NO expone en el dominio del curso** el número de estudiantes, el código del plan, un
  indicador de aula fija ni una lista de materias: esos datos no se persisten (el plan y el número de
  estudiantes son de la configuración del solver, spec 005).
- **CUANDO se consulte un curso, EL SISTEMA devolverá esos campos en el dominio.**
- **EL SISTEMA NO expone el nombre del curso en el dominio ni en los servicios:** el nombre es
  **presentación** y lo deriva quien muestra (la interfaz de usuario o cualquier consulta de datos) a
  partir del año y la sección; no es un campo almacenado ni viaja en el dominio. El dominio del curso
  expone `{identificador, año, sección, turno, aula}`.

### RF-5. Retirada de la relación persistida curso–materia y cascadas resultantes

- **EL SISTEMA deja de persistir la relación entre cursos y materias.**
- **CUANDO se elimine un curso, una materia o un plan de estudios, EL SISTEMA no arrastrará relaciones
  curso–materia:** esa relación ya no existe.
- **La dependencia que antes ligaba un plan con sus cursos desaparece:** como el curso ya no almacena
  el código del plan, **eliminar un plan no afecta a ningún curso.**
- **La dependencia de un curso con su aula sobrevive:** eliminar un aula referenciada muestra los
  cursos dependientes y, si el usuario confirma, los deja sin aula (RF-1).
- **La dependencia de un curso con su turno sobrevive:** eliminar un turno referenciado por cursos
  muestra los cursos dependientes y, si el usuario confirma, los deja sin turno. La baja del aula
  sigue el mismo patrón y deja el curso sin aula (RF-1); el mecanismo genérico de **diálogo de
  dependientes con confirmación** es el de la spec **001, RF-2**.
- **EL SISTEMA deja de incluir las materias del curso en la huella de datos** que detecta «datos
  anteriores» en una generación: la huella pasa a basarse en los datos que sí se persisten (los del
  curso, no sus materias).

### RF-6. Esquema único y colapso de la migración

- **EL SISTEMA mantiene un único esquema de base de datos:** no existen versiones de esquema previas
  ni pasos versionados de migración.
- **CUANDO la base de datos no exista, EL SISTEMA la creará con la totalidad del esquema de una sola
  vez.**
- **El esquema esperado es único y se reconoce de forma comprobable** por (a) la **presencia de la
  totalidad del catálogo de tablas y columnas del modelo nuevo** (entre ellas, el curso con año,
  sección, turno y referencia de aula, y la **ausencia** de la relación curso–materia) y (b) la
  **coincidencia de la marca de versión de esquema esperada**.
- **SI el esquema de una base existente no corresponde al esquema esperado** —falta alguna tabla o
  columna del catálogo, sobra una que el modelo retiró (como la relación curso–materia) o la marca de
  versión no coincide—, **ENTONCES EL SISTEMA tratará la apertura como fallo** (coherente con la spec
  001, RF-4), sin intentar migrar.
- **EL SISTEMA no migra datos ni conserva versiones previas: no existen datos de usuario anteriores al
  modelo nuevo.** Eventuales bases de desarrollo con otra estructura **también** se tratan como fallo
  de apertura; no se migran ni se corrigen.
- **EL SISTEMA conserva los sellos de auditoría de los registros (fecha de creación y de
  modificación);** el colapso del esquema no los elimina.
- **EL SISTEMA conserva la protección de los datos del usuario:** toda operación destructiva sobre una
  base existente exige respaldo previo o no se realiza (constitución §5). La creación del esquema
  nuevo no es destructiva porque no hay datos de usuario que preservar.

## Impacto en el contrato de consumo (dominio del curso)

El contrato que nuestro lado expone a quien consume los datos (la interfaz de usuario) cambia, en lo
observable:

- **Tipo de curso expuesto:** pasa a `{ identificador, año, sección, turno, aula }`. Desaparecen
  número de estudiantes, código de plan, indicador de aula fija y la lista de materias.
- **Operaciones de curso:** desaparecen las de asignar/quitar/consultar materias de un curso.
- **Nombre:** no viaja en el dominio ni en los servicios; cualquier consumidor que lo muestre lo
  deriva de año y sección.
- **Cascada:** la baja de un curso no arrastra relaciones curso–materia; la baja de un plan no afecta
  a los cursos; la baja de un aula referenciada muestra los cursos dependientes y, confirmada, los
  deja sin aula; la baja de un turno referenciado muestra los cursos dependientes y, confirmada, los
  deja sin turno.
- **Huella de datos:** deja de incluir las materias del curso (RF-5).
- **Consulta de datos:** refleja el mismo contrato (año, sección, turno y aula; sin nombre almacenado
  ni operaciones curso–materia; el nombre, si se muestra, se deriva).

La actualización concreta del documento de contrato es una tarea de implementación; esta spec solo
describe el cambio observable.

## Requisitos no funcionales

- **RNF-1. Protección de datos:** la creación del esquema nuevo no es destructiva; cualquier operación
  destructiva sobre una base existente mantiene el respaldo obligatorio (constitución §5). Fallar la
  apertura de una base con esquema no esperado **no borra ni sobrescribe sus datos**: el usuario decide
  (reintentar, restaurar respaldo, crear base nueva o salir).
- **RNF-2. Mensajes en español:** todo rechazo identifica el campo, el valor rechazado y, cuando
  exista, la acción disponible para el usuario (por ejemplo, «elija otro año o sección»), sin errores
  técnicos crudos.
- **RNF-3. Trazabilidad:** toda regla de esta spec tendrá al menos una comprobación verificable en las
  tareas; sin cobertura, la tarea no está hecha.

## Casos límite

- **Curso con año+sección ya existentes:** se rechaza el registro o la edición, aunque el año sea
  válido (RF-1).
- **Actualizar un curso sin cambiar su año ni su sección:** se acepta; la unicidad permite el mismo
  registro y la operación no se auto-rechaza (RF-1).
- **Escritura que evite la validación de la aplicación e intente un duplicado:** no llega a existir un
  estado con el año+sección repetido (RF-1).
- **Año cero, negativo o no entero:** rechazo con mensaje que identifica el campo y el valor (RF-3).
- **Sección vacía, formada solo por espacios o de más de un carácter:** rechazo con mensaje (RF-3).
- **Sección con espacios alrededor (`" a "`) o en minúscula (`"a"`):** se normaliza a `"A"`; `" a "` y
  `"A"` son el mismo curso (RF-3).
- **Sección no ASCII de un único carácter tras normalizar (`Ñ`, `Á`):** se acepta (RF-3).
- **Sección que al normalizar a mayúsculas se expande a más de un carácter (`ß`→`SS`):** rechazo con
  mensaje (RF-3).
- **Curso sin aula:** la referencia de aula queda ausente; el modo de aula (global, spec 005) decide
  su uso (RF-1, RF-2).
- **Registrar o actualizar un curso con una referencia de aula inexistente:** rechazo con mensaje que
  identifica la causa (RF-1).
- **Aula referenciada eliminada:** se muestran los cursos dependientes y se pide confirmación; si el
  usuario confirma, los cursos quedan sin aula (referencia nula) (RF-1).
- **Curso sin turno:** se **conserva sin turno** (el turno persistido es **opcional**, RF-1/RF-2). Al
  **generar**, la spec 005 exige que todo curso **declare** turno: es **obligatorio** en la entrada del
  solver y, si falta, la entrada se **rechaza** (V7). Por tanto, un curso sin turno persistido no puede
  entrar en una generación hasta que se le asigne uno; lo único **diferido a la spec 006** es la
  **asignación automática** (que el solver elija el turno). La persistencia admite el curso sin turno;
  su uso en la generación lo decide la 005 (RF-2).
- **Turno referenciado eliminado:** se muestran los cursos dependientes y se pide confirmación; si el
  usuario confirma, los cursos quedan sin turno (RF-5).
- **Intento de asignar, quitar o consultar las materias de un curso:** la operación ya no existe; las
  materias se derivan del plan (spec 005) (RF-2, RF-5).
- **Base de datos inexistente:** se crea con la totalidad del esquema de una sola vez (RF-6).
- **Base de datos existente con un esquema que no es el esperado** (falta alguna tabla o columna del
  catálogo, sobra la relación curso–materia, o la marca de versión no coincide, incluidas las bases de
  desarrollo con otro esquema): la apertura se trata como fallo, sin migración y sin borrar datos
  (RF-6).
- **Eliminación de un plan referenciado por cursos:** ya no afecta a los cursos (la dependencia
  desapareció); no hay cascada sobre cursos (RF-5).
- **Eliminación de una materia:** ya no arrastra relaciones curso–materia (RF-5).
- **Sellos de auditoría (`fecha_creacion`/`fecha_modificacion`) tras el colapso del esquema:** se
  conservan en los registros (RF-6).
- **Años del docente vacíos:** apto para cualquier año; es dato del preset, no de la base (spec 005).

## Fuera de alcance

- **El modelo del solver, las invariantes duras, la función objetivo, la construcción de la entrada del
  solver desde los datos persistidos más el preset, el contrato de configuración (forma del modelo de
  datos y su serialización), la gestión de los presets y la validación previa V1–V15** (spec 005), que
  se apoya en esta spec: 004 **persiste y expone** los datos; 005 los **lee y arma** la entrada.
- **La persistencia del plan y del número de estudiantes por curso** y **la persistencia de las
  materias del curso:** son datos de la configuración del solver (spec 005); no se persisten.
- **La persistencia de los años de un docente:** viven en el preset/configuración del solver (spec
  005).
- **La migración de datos entre versiones de esquema y la compatibilidad con bases anteriores:** no
  existen versiones (el esquema es único).
- **La traducción de índice de aula a nombre**, la **interfaz de usuario** y la presentación de los
  cursos; las aporta otro equipo.
- **La asignación automática de turno y el toggle de `turno`** (spec 006): decidir si el solver
  **elige** el turno de un curso que no lo declara. Aquí solo se persiste el `turno` como dato
  **opcional** del curso (RF-1, RF-2); en cambio, **al generar** el turno debe **declararse** (la spec
  005 lo exige y rechaza su ausencia, V7), de modo que lo diferido es la **asignación automática**, no
  el uso del turno declarado.
- **Generación parcial** y horarios previos (spec 007).

## Criterios de finalización

1. Los cursos se persisten y se devuelven con año, sección, turno y aula, sin nombre almacenado y con
   la unicidad año+sección garantizada en el almacenamiento; la referencia de aula debe existir y la
   baja de un aula referenciada pide confirmación mostrando los dependientes (RF-1 a RF-3).
2. El dominio expone año, sección, turno y aula por curso, **sin el nombre** (que es presentación) y
   sin número de estudiantes, código de plan, indicador de aula fija ni materias (RF-4).
3. No existe relación persistida curso–materia y ninguna operación la ofrece o la arrastra; la cascada
   de plan→cursos desaparece, la de aula→cursos y turno→cursos sobrevive con confirmación, y la huella
   de datos ya no incluye las materias del curso (RF-2, RF-5).
4. El esquema es único: la base inexistente se crea entera de una vez y no hay pasos versionados ni
   migración de datos; un esquema que no corresponda al **esquema esperado** (catálogo de tablas y
   columnas y marca de versión) se trata como fallo de apertura sin borrar datos (RF-6).
5. Existe comprobación verificable en pruebas para cada RF antes de dar la spec por implementada
   (RNF-3).

## Dudas abiertas

Ninguna. (El `turno` del curso queda **opcional**; la baja de un aula referenciada **pide confirmación
mostrando los dependientes** y, confirmada, deja el curso sin aula; el turno referenciado sigue el
mismo flujo; y el toggle de turno automático se difiere a la spec 006.)
