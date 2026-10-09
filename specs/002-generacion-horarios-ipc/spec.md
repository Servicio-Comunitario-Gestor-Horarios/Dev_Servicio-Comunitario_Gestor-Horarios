# Spec 002 - Generación de horarios y comunicación con el proceso de cálculo

Estado: aprobada

## Contexto y objetivo

El lado cliente prepara el JSON de entrada del contrato documentado del motor de solver (datos de dominio tomados de la base de datos, más la configuración del solver y los presets, que se manejan como archivos JSON), lo valida y lo envía al proceso de cálculo. El proceso de cálculo solo resuelve el problema y nunca accede a la base de datos. Al recibir la salida, el lado cliente la valida, la muestra y permite guardarla como archivo JSON.

El objetivo es que la generación funcione sobre datos reales y que quede delimitado qué lado posee los datos y qué lado calcula.

## Usuarios

- **Usuario académico:** genera horarios, los revisa y decide guardarlos como archivo.
- **Usuario cuyo proceso de cálculo no está disponible:** necesita un aviso claro y seguir operando con normalidad sobre los dominios de datos.

## Historias de usuario

- HU-1. Como usuario académico, quiero generar un horario, verlo en pantalla y decidir yo si lo guardo como archivo, para que solo conserve los resultados que yo apruebe.
- HU-2. Como usuario, quiero que si el proceso de cálculo no está disponible la aplicación me avise y siga funcionando, para no perder mi trabajo.
- HU-3. Como usuario, quiero que si no existe una solución factible se me informe y no se me ofrezca guardarla, para no conservar resultados no válidos.

## Definiciones

- **Lado cliente:** todo el programa salvo el proceso de cálculo.
- **JSON de entrada/salida del solver:** contrato de datos del motor de solver, con sus validaciones previas y posteriores. Esta spec no modifica ese contrato.
- **Configuración del solver y presets:** conjuntos de datos que se gestionan como archivos JSON y no se persisten en la base de datos.
- **Horario generado:** resultado devuelto por el proceso de cálculo, mostrado en pantalla pero aún no guardado por el usuario.
- **Horario guardado:** archivo JSON que el usuario decide crear a partir de un horario generado.
- **Proceso de cálculo:** el proceso que recibe el JSON de entrada y devuelve el horario resuelto; no accede a la base de datos.
- **Salud:** comprobación de que el proceso de cálculo está disponible y responde.
- **Apagado:** solicitud al proceso de cálculo de que termine de forma ordenada.

## Requisitos funcionales

### RF-1. Preparación, envío y resultado de la generación de horarios

- CUANDO el usuario solicite la generación de un horario, EL SISTEMA preparará el JSON de entrada según el contrato documentado (datos de dominio de la base de datos, más configuración del solver y presets en JSON), lo validará contra las validaciones previas y solo lo enviará al proceso de cálculo si las supera; si no las supera, mostrará el motivo y no enviará la petición.
- CUANDO el proceso de cálculo devuelva un resultado conforme a la estructura del contrato, EL SISTEMA aplicará las validaciones posteriores (RF-3) y, si el resultado es presentable, lo mostrará como horario generado; EL SISTEMA solo guardará el horario como archivo cuando el usuario lo solicite.
- CUANDO el proceso de cálculo indique que no existe solución factible, ENTONCES EL SISTEMA informará al usuario y no ofrecerá guardarlo.
- SI el resultado recibido no corresponde al contrato (estructura inválida o corrupta), ENTONCES EL SISTEMA informará al usuario y no lo presentará como resultado válido.
- SI el proceso de cálculo no está disponible, no responde en 60 segundos desde que se envió la petición, o pierde la comunicación después de enviarla, ENTONCES EL SISTEMA informará al usuario, dará la petición por abandonada y la aplicación seguirá operando con normalidad; si llegara un resultado posterior, EL SISTEMA lo ignorará.
- SI mientras el proceso de cálculo trabaja el usuario modifica datos de los dominios que formaron parte de la petición, ENTONCES al recibir el resultado EL SISTEMA informará que corresponde a datos anteriores y pedirá confirmación antes de guardarlo.

### RF-2. Delimitación entre lado cliente y proceso de cálculo

- EL SISTEMA garantiza que solo el lado cliente lee y escribe el archivo de la base de datos; el proceso de cálculo nunca lo accede.
- CUANDO la aplicación arranque, EL SISTEMA comprobará la salud del proceso de cálculo con un plazo de 5 segundos y, si no está disponible, informará al usuario sin impedir el uso de los dominios de datos.
- CUANDO la aplicación vaya a cerrarse, EL SISTEMA solicitará el apagado del proceso de cálculo con un plazo de 5 segundos; si no responde, EL SISTEMA continuará el cierre.
- MIENTRAS la aplicación esté en uso, EL SISTEMA usará la comunicación con el proceso de cálculo únicamente para las operaciones de salud, apagado y generación de horarios, no para leer ni escribir datos de usuario.

### RF-3. Validaciones posteriores del resultado

- CUANDO el proceso de cálculo devuelva un resultado estructuralmente válido (RF-1), EL SISTEMA aplicará las validaciones posteriores del contrato: si no hay conflictos de solapamiento, lo presentará como horario generado y mostrará junto a él los avisos de horas no cubiertas, exceso de horas de profesor y exceso de capacidad de aula, conforme a los umbrales del contrato.
- SI las validaciones posteriores detectan conflictos de solapamiento, ENTONCES EL SISTEMA informará que el cálculo falló y no presentará el resultado como horario válido.

### RF-4. Guardado de horarios y presets como archivos

- CUANDO el usuario solicite guardar un horario generado, EL SISTEMA lo escribirá como archivo JSON en el destino que el usuario elija.
- CUANDO el usuario gestione presets (crear, cargar, guardar), EL SISTEMA lo hará como archivos JSON.
- SI una operación de escritura de un archivo (un horario o un preset) falla (disco lleno, permisos), ENTONCES EL SISTEMA informará al usuario y permitirá reintentarla, sin retirar de pantalla el contenido que el usuario quería guardar.

### RF-5. Protección al cerrar con trabajo sin guardar

- MIENTRAS haya un horario generado sin guardar o el proceso de cálculo esté procesando una petición (entre el envío y la recepción del resultado o el aviso por timeout), EL SISTEMA avisará al usuario al intentar cerrar la aplicación y permitirá cancelar el cierre.

## Requisitos no funcionales

- RNF-1. Todos los mensajes de error mostrados al usuario identificarán la causa y las acciones disponibles, en español.

## Casos límite

- Dominios vacíos o insuficientes para construir el JSON de entrada (sin materias, sin profesores o curso sin estudiantes) → validación previa y no se envía la petición (RF-1).
- El proceso de cálculo devuelve que no hay solución → aviso al usuario y no se ofrece guardarlo (RF-1).
- El proceso de cálculo devuelve un JSON de salida inválido o corrupto → aviso y no se presenta como resultado válido (RF-1).
- El proceso de cálculo no responde en 60 segundos o pierde la comunicación tras la petición → aviso, la petición se da por abandonada y la aplicación sigue operando (RF-1).
- El usuario edita datos enviados mientras el proceso de cálculo trabaja → el resultado se marca como de datos anteriores y requiere confirmación para guardarlo (RF-1).
- El proceso de cálculo no está disponible al arrancar (no responde en 5 s) → aviso no bloqueante (RF-2).
- El apagado del proceso de cálculo no responde en su plazo → se avisa y el cierre continúa (RF-2).
- El resultado incluye avisos de validación posterior (horas, capacidad) → se muestran junto al horario generado (RF-3).
- Fallo al guardar un horario o un preset como archivo (disco lleno, permisos) → aviso y reintento, sin perder el contenido (RF-4).
- Cierre de la aplicación con un horario generado sin guardar o una generación en curso → aviso cancelable (RF-5).

## Fuera de alcance

- Que el proceso de cálculo acceda a la base de datos de cualquier forma.
- Modificación del contrato JSON de entrada/salida del solver.
- Segundas generaciones concurrentes: mientras una generación está en curso no se admite otra petición.
- Resultados que llegan después de que el usuario cierre la vista o cambie de contexto.
- Persistencia de la configuración del solver, los presets y los horarios guardados en la base de datos: se gestionan como archivos JSON.
- Reintento o recuperación automática del proceso de cálculo tras una caída; el apagado del proceso se limita a la solicitud ordenada al cerrar.

## Criterios de finalización

1. La generación de un horario envía el JSON validado y muestra la salida con sus avisos de validación posterior; el horario solo se guarda como archivo si el usuario lo decide; el proceso de cálculo nunca lee ni escribe la base de datos (RF-1, RF-2, RF-3).
2. El guardado de horarios y presets como archivos JSON informa y permite reintentar ante fallos (RF-4).
3. Un intento de cierre con trabajo sin guardar o con el proceso de cálculo en marcha muestra un aviso cancelable (RF-5).
4. Existe comprobación verificable en tests para cada RF antes de dar la spec por implementada (política de tests del proyecto).

## Dudas abiertas

Ninguna.
