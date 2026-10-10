# Spec 001 - Base de datos local: arranque, persistencia de dominios e instancia única

Estado: implementada

## Contexto y objetivo

Hasta ahora la aplicación no tiene una conexión operativa entre la interfaz y la base de datos: los datos que ve el usuario no se persistieron ni se recuperaron de forma fiable. Esta spec establece que el lado cliente (el programa salvo el proceso de cálculo) es el dueño de la base de datos: la abre, la migra, la lee y la escribe.

El objetivo es que la interfaz funcione sobre datos reales y persistidos, que el usuario pueda operar incluso ante fallos de la base con una salida clara, y que exista una sola instancia. La base solo persiste dominios de entidad; la configuración del solver, los presets y los horarios se gestionan como archivos JSON (spec 002).

El consumidor de la lógica y del contrato de datos que define esta spec es la interfaz de usuario, que implementa otro equipo (ver «Fuera de alcance»): nuestro lado entrega la lógica y el contrato de datos que esa interfaz consume.

## Usuarios

- **Usuario académico:** gestiona docentes, aulas, asignaturas, cursos, planes de estudio, turnos y recesos.
- **Usuario que arranca la aplicación con un problema en su base de datos** (archivo corrupto, sin permisos, esquema incompatible): necesita entender qué falló y decidir qué hacer.
- **Usuario que intenta abrir una segunda instancia** de la aplicación.

## Historias de usuario

- HU-1. Como usuario académico, quiero que al abrir la aplicación mis datos estén disponibles desde la base de datos, para trabajar con información persistida y no con datos temporales.
- HU-2. Como usuario académico, quiero que los cambios que hago en docentes, aulas, asignaturas y demás dominios se guarden, para que no se pierdan al cerrar la aplicación.
- HU-4. Como usuario con una base de datos dañada, quiero que la aplicación me explique el problema y me ofrezca reintentar, restaurar un respaldo, crear una base nueva o salir, para decidir yo qué hago con mis datos.
- HU-5. Como usuario, quiero que solo exista una instancia de la aplicación, para que no haya escrituras en conflicto sobre mi base de datos.
- HU-6. Como usuario, quiero que si una operación de guardado falla se me avise y mis datos sigan en pantalla, para poder reintentar sin perder lo que estaba haciendo.

## Definiciones

- **Lado cliente:** todo el programa salvo el proceso de cálculo; incluye la interfaz y la gestión de la base de datos, con independencia de cómo se organice internamente. Está separado del proceso de cálculo.
- **Base de datos del usuario:** almacén local de los dominios en alcance (docentes, aulas, asignaturas/materias, cursos, planes de estudio, turnos y recesos).
- **Instancia:** una ejecución de la aplicación.
- **Cambio pendiente:** operación de escritura que el usuario confirmó y cuya persistencia falló (RF-6); puede ser un alta o modificación (pendiente de guardar) o una baja (pendiente de eliminar). En operación normal no existe: cada operación se persiste al confirmarse (RF-2).
- **Estado conocido (de la base):** la versión de esquema previa a la migración queda intacta, sin cambios parciales aplicados.

## Requisitos funcionales

### RF-1. Levantamiento de la base de datos por el lado cliente

- CUANDO la aplicación inicie, EL SISTEMA abrirá la base de datos de usuario y, si su versión de esquema es anterior a la que espera esta versión de la aplicación, aplicará las migraciones necesarias antes de permitir operar con datos.
- SI el archivo de la base de datos existe pero su versión de esquema está ausente, no es interpretable, o es posterior a la esperada, ENTONCES EL SISTEMA no migrará y tratará la apertura como fallo (RF-5).
- SI la base de datos no existe, ENTONCES EL SISTEMA la creará con su esquema inicial sin intervención del usuario.
- ANTES de aplicar una migración, EL SISTEMA creará un respaldo de la base actual; SI el respaldo no es posible, ENTONCES EL SISTEMA no migrará, lo informará al usuario y tratará la apertura como fallo (RF-5).
- SI una migración falla o se interrumpe, ENTONCES EL SISTEMA dejará la base en un estado conocido y tratará la apertura como fallo (RF-5), desde donde se ofrece restaurar el respaldo creado antes de migrar.
- MIENTRAS una migración esté en curso, EL SISTEMA indicará que la migración está en marcha y no permitirá operar con datos hasta que termine.
- MIENTRAS la aplicación esté en uso, EL SISTEMA mantendrá la base de datos disponible para todas las operaciones de lectura y escritura.

### RF-2. Operación con los dominios de datos

- CUANDO el usuario confirme la creación, modificación o eliminación de datos de los dominios en alcance, EL SISTEMA persistirá el cambio antes de dar la operación por completada.
- SI una eliminación afecta a datos referenciados por otros, ENTONCES EL SISTEMA mostrará los registros dependientes que se eliminarán en cascada y pedirá confirmación explícita antes de continuar.
- MIENTRAS se aplica una creación, modificación o eliminación en cascada, EL SISTEMA lo hará de forma atómica: si falla, no quedará aplicada ninguna parte.
- CUANDO la aplicación arranque o el usuario abra una vista que muestre datos, EL SISTEMA mostrará lo que hay almacenado en la base de datos.
- EL SISTEMA dispondrá de los dominios necesarios para construir el JSON de entrada del solver: docentes, aulas, asignaturas/materias, cursos, planes de estudio, turnos y recesos.

### RF-3. Fallos durante la operación

- SI una operación de lectura falla, ENTONCES EL SISTEMA informará al usuario y mostrará que no hay datos disponibles, sin presentar información anterior como si fuera actual.
- SI una operación de escritura sobre la base de datos falla (disco lleno, permisos, corrupción), ENTONCES EL SISTEMA informará al usuario, marcará esa operación como cambio pendiente y permitirá reintentarla.
- MIENTRAS una operación de escritura no haya confirmado su éxito, EL SISTEMA no dará por aplicado el cambio: un alta o modificación se mantendrá en pantalla como pendiente de guardar, y una baja se mantendrá en pantalla como pendiente de eliminar.

### RF-4. Fallo de apertura o migración de la base de datos

- SI la base de datos no se puede abrir o las migraciones fallan (corrupción, falta de permisos, esquema incompatible —incluida una base creada por una versión más nueva de la aplicación—), ENTONCES EL SISTEMA mostrará un diálogo con las opciones «Reintentar», «Restaurar respaldo», «Crear base de datos nueva (pierde datos)» y «Salir».
- CUANDO el usuario elija «Reintentar», EL SISTEMA volverá a intentar la apertura y, si vuelve a fallar, mostrará de nuevo el diálogo.
- CUANDO el usuario elija «Restaurar respaldo», EL SISTEMA le permitirá elegir un respaldo existente, lo validará (integridad y esquema compatibles) y restaurará; SI la restauración falla, ENTONCES informará al usuario y mostrará de nuevo el diálogo; si tiene éxito, el arranque continuará.
- CUANDO el usuario elija «Crear base de datos nueva», EL SISTEMA pedirá confirmación explícita y creará un respaldo de la base anterior antes de descartarla; SI el respaldo no es posible (base ilegible, sin espacio, sin permisos), ENTONCES EL SISTEMA lo informará y mantendrá deshabilitada esa opción, dejando operativas «Reintentar», «Restaurar respaldo» y «Salir».
- SI la creación de la base nueva falla después de descartar la anterior, ENTONCES EL SISTEMA lo informará, mostrará de nuevo el diálogo y ofrecerá restaurar el respaldo creado.
- CUANDO el usuario elija «Salir», EL SISTEMA cerrará sin escribir en la base de datos.
- MIENTRAS el diálogo de fallo esté activo, EL SISTEMA no permitirá operar con datos.

### RF-5. Una sola instancia

- CUANDO la aplicación detecte otra instancia en ejecución, EL SISTEMA intentará enfocar esa instancia existente y no arrancará una segunda sesión.
- SI la instancia existente no se puede enfocar en un plazo de 10 segundos (ventana no localizable), ENTONCES EL SISTEMA avisará al usuario y cerrará el arranque.
- EL SISTEMA mantendrá la detección de instancias activa en todo momento del arranque, incluido mientras una instancia anterior esté en el diálogo de fallo de RF-4 o en una migración.
- SI no hay ninguna otra instancia en ejecución, ENTONCES EL SISTEMA arrancará con normalidad.

### RF-6. Protección de los datos del usuario

- EL SISTEMA no eliminará ni sobrescribirá los datos del usuario sin confirmación explícita.
- CUANDO el usuario confirme la creación de una base de datos nueva, EL SISTEMA creará un respaldo de la base anterior antes de descartarla; si el respaldo no es posible, no se descartará la base (RF-4).
- MIENTRAS haya cambios pendientes (RF-3), EL SISTEMA avisará al usuario al intentar cerrar la aplicación y permitirá cancelar el cierre.

## Requisitos no funcionales

- RNF-1. Todos los mensajes de error mostrados al usuario identificarán la causa y las acciones disponibles, en español.
- RNF-2. La apertura normal de la base de datos no requerirá intervención del usuario salvo que falle.
- RNF-3. Ante una escritura fallida, ningún dato introducido por el usuario desaparecerá de la pantalla sin un aviso explícito.
- RNF-4. El usuario distinguirá en todo momento, por registro, si un dato está guardado o pendiente de guardar o eliminar.
- RNF-5. Ante un cierre no solicitado del sistema (fallo o corte), las escrituras ya confirmadas se conservarán y la base de datos no quedará parcialmente escrita.

## Casos límite

- Base de datos inexistente al primer arranque → se crea con el esquema inicial.
- Base de datos existente pero vacía → la aplicación opera con los dominios vacíos.
- Archivo de base de datos corrupto o sin permisos de lectura → diálogo de fallo (RF-4).
- Migración incompatible con el esquema existente o base creada por una versión más nueva → diálogo de fallo (RF-4).
- Versión de esquema ausente o ilegible → se trata como apertura fallida (RF-1, RF-4).
- Migración que tarda → estado visible de proceso; no se puede operar hasta que termine (RF-1).
- Migración interrumpida a mitad → base en estado conocido (versión previa intacta); se ofrece restaurar el respaldo previo (RF-1).
- Respaldo previo a migración o descarte no posible (disco lleno, sin permisos) → no se migra y la apertura se trata como fallo (RF-1); en el descarte, al no poder respaldar no se permite descartar (RF-4).
- Base de datos en ubicación sin permiso de escritura para el respaldo pero legible → no se permite descartar; quedan Reintentar, Restaurar y Salir (RF-4).
- Disco lleno durante la migración → la base queda en estado conocido y se ofrece restaurar el respaldo (RF-1).
- Restaurar respaldo cuando no existe ninguno disponible → se informa y se mantiene el diálogo de fallo (RF-4).
- Restauración interrumpida o fallida → se informa y se muestra de nuevo el diálogo; la base queda en estado conocido (RF-4).
- Fallo al crear la base nueva tras descartar la anterior → aviso, de nuevo el diálogo y opción de restaurar el respaldo (RF-4).
- Fallo de escritura sobre la base a mitad de sesión (disco lleno, permisos) → aviso y la operación queda como cambio pendiente con reintento (RF-3).
- Eliminación en cascada que falla a mitad → no se aplica ninguna parte (operación atómica) y se informa (RF-2).
- Cierre no solicitado del sistema con escrituras confirmadas → la base conserva lo escrito y no queda parcialmente escrita (RNF-5).
- Cierre de la aplicación con cambios pendientes → aviso cancelable (RF-6).
- Cierre de la aplicación durante una migración o una restauración → no se permite hasta que termine o falle (RF-1, RF-4).
- Segunda instancia → se enfoca la existente; si no es posible en un plazo, aviso y no arranque (RF-5).
- Segunda instancia cuando la primera está en el diálogo de fallo de RF-4 o migrando → la detección sigue activa (RF-5).

## Fuera de alcance

- **Interfaz de usuario (Qt) de `src/frontend`** (diálogos, vistas e indicadores): la implementa otro equipo. Esta spec define el comportamiento del producto y nuestro lado entrega la **lógica y el contrato de datos** que esa interfaz consume; el contrato de consumo se documenta en `docs/interfaz-frontend.md`. No se implementan aquí los widgets, las vistas ni los diálogos.
- Inicio de sesión y gestión de usuarios: el inicio de sesión no valida contra la base de datos en esta versión.
- Varias instancias simultáneas y cualquier forma de concurrencia o desincronización sobre la misma base de datos.
- Bloqueo de la base de datos por otro proceso externo, o base borrada o sustituida externamente mientras la aplicación está en uso.
- Sincronización entre máquinas, almacenamiento remoto o multiusuario.
- Acumulación y límite de reintentos de múltiples operaciones pendientes; orden de reintento.
- Acumulación y limpieza de respaldos de migraciones sucesivas; caso de dos respaldos creados en el mismo instante (misma marca de tiempo).
- Distinción de un esquema más nuevo pero compatible: toda versión posterior a la esperada se trata como fallo de apertura.
- Importación/exportación masiva de datos, copias automáticas programadas (solo los respaldos puntuales exigidos en RF-1 y RF-4).
- Rendimiento a gran escala y optimización de consultas fuera de los volúmenes habituales de uso académico.

## Criterios de finalización

1. Al arrancar, la aplicación abre la base de datos y la deja operativa sin intervención del usuario; si el esquema es anterior, migra tras crear el respaldo correspondiente; si es posterior o ilegible, falla a RF-4 (RF-1).
2. Los datos de los dominios en alcance se persisten y se recuperan de la base de datos tras reiniciar; las eliminaciones en cascada son atómicas y requieren confirmación explícita (RF-2).
3. Con una base de datos corrupta/inaccesible aparece el diálogo con Reintentar / Restaurar respaldo / Crear base nueva / Salir; crear base nueva exige respaldo previo o no descarta, y ninguna ruta pierde el control de la aplicación (RF-4, RF-6).
4. Un fallo de escritura deja el registro en pantalla marcado como cambio pendiente y permite reintentar (RF-3).
5. Un segundo arranque detecta la instancia previa, la enfoca (o avisa y no arranca si no puede en plazo) (RF-5).
6. Existe comprobación verificable en tests para cada RF antes de dar la spec por implementada (política de tests del proyecto).

## Dudas abiertas

Ninguna.
