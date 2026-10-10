# Spec 003 - Cierre de la spec 001: huecos detectados en la validación

Estado: implementada

## Contexto y objetivo

La validación de la spec 001 (`gestor-horarios-spec-002-y-huecos-spec-001.md`) detectó cinco
incumplimientos en la implementación de la base de datos local. Esta spec los cierra. No introduce
requisitos nuevos: concreta y hace verificable lo que la spec 001 ya exigía (RF-1, RF-3, RF-4, RF-5
y RNF-1), y solo la implementa el lado cliente (`src/backend`, `src/app`). La interfaz Qt de
`src/frontend` la implementa otro equipo; el contrato se documenta en `docs/interfaz-frontend.md`.

## Usuarios

- **Usuario con una base dañada o incompatible:** necesita poder descartarla y crear una nueva sin
  perder sus datos si no hay respaldo.
- **Usuario que ve una lista de datos:** necesita distinguir «no hay datos» de «la lectura falló».
- **Usuario que arranca la aplicación:** necesita saber si la base está migrando y, si hay otra
  instancia, que se enfoque la existente.
- **Usuario que lee un error:** necesita la causa y qué puede hacer, en español.

## Historias de usuario

- HU-1. Como usuario con una base dañada, quiero crear una base nueva y que se respalde la anterior
  antes de descartarla, para no perder mis datos por accidente.
- HU-2. Como usuario, quiero que si la lectura de una lista falla se me avise, para no confundir un
  error con «no hay datos».
- HU-3. Como usuario, quiero ver que la base está migrando, para saber por qué aún no puedo operar.
- HU-4. Como usuario que abre una segunda instancia, quiero que se enfoque la que ya está abierta.
- HU-5. Como usuario, quiero mensajes de error claros (causa y acciones) en español.

## Definiciones

- **Descartar la base:** dejar sin efecto el archivo de la base actual para crear una nueva con el
  esquema inicial. Nunca ocurre sin respaldo previo.
- **Lectura fallida:** la operación de lectura no pudo ejecutarse (tabla inexistente, conexión
  cerrada, error de acceso); se distingue de «sin datos» (lectura correcta sin filas).
- **Migración en marcha:** estado observable mientras se aplican los pasos de migración de esquema.

## Requisitos funcionales

### RF-1. Crear base nueva con respaldo obligatorio

- CUANDO el usuario confirme «Crear base de datos nueva», EL SISTEMA creará un respaldo de la base
  anterior antes de descartarla.
- SI el respaldo no es posible (base ilegible, sin espacio, sin permisos), ENTONCES EL SISTEMA no
  descartará la base e informará el motivo.
- SI la creación de la base nueva falla después de descartar la anterior, ENTONCES EL SISTEMA
  informará el motivo y ofrecerá restaurar el respaldo creado.
- SI el descarte o la creación tienen éxito, ENTONCES EL SISTEMA dejará una base nueva con el
  esquema inicial y la versión de esquema actual.

### RF-2. Distinguir error de lectura de "sin datos"

- CUANDO una lectura de un dominio tenga éxito, EL SISTEMA devolverá los datos (aunque estén vacíos),
  sin confundir el vacío con un error.
- SI una lectura de un dominio falla, ENTONCES EL SISTEMA devolverá un resultado de error con un
  mensaje en español e identificará la causa, sin presentar el vacío como resultado válido.

### RF-3. Indicar la migración en marcha

- MIENTRAS una migración esté en curso, EL SISTEMA avisará (de forma observable) del progreso de la
  migración.
- SI no hay migración, ENTONCES no se producirá ningún aviso de migración.

### RF-4. Enfocar la instancia existente

- CUANDO se detecte otra instancia en ejecución, EL SISTEMA solicitará el enfoque de la existente.
- MIENTRAS la instancia principal esté en una migración o en el diálogo de fallo, EL SISTEMA
  mantendrá activa la detección y atenderá la solicitud de enfoque.

### RF-5. Mensajes en español con causa y acciones

- Los mensajes de error mostrados al usuario identificarán la causa y las acciones disponibles, en
  español, sin texto crudo de la herramienta subyacente.

## Requisitos no funcionales

- RNF-1. Las operaciones destructivas siguen exigiendo confirmación explícita y respaldo previo.
- RNF-2. El comportamiento ya cubierto en verde por la spec 001 no se rompe.

## Casos límite

- Respaldo imposible antes de descartar → no se descarta; la opción «Crear base nueva» queda
  deshabilitada.
- Creación de la base nueva fallida tras descartar → aviso y respaldo disponible para restaurar.
- Lectura de un dominio vacío → resultado correcto con lista vacía (no error).
- Lectura de un dominio con la conexión cerrada o tabla ausente → resultado de error con mensaje.
- Migración de varios pasos → una notificación de progreso por paso.
- Segunda instancia durante migración o diálogo de fallo → se enfoca la principal.
- Error con texto crudo de Qt → se sustituye por causa + acciones en español.

## Fuera de alcance

- La interfaz Qt de `src/frontend` (diálogos, listas, indicadores). Otro equipo la implementa sobre
  el contrato de `docs/interfaz-frontend.md`.
- El proceso de cálculo (`--backend`): sigue sin acceder a la base (RF-2 de la spec 001).
- Cambiar el QUÉ de la spec 001: esta spec solo cierra sus huecos.

## Criterios de finalización

1. Crear base nueva nunca descarta sin respaldo; si el respaldo falla, no descarta; si la creación
   falla tras descartar, se ofrece el respaldo (RF-1).
2. Los `listar*` distinguen error de vacío con resultado verificable (RF-2).
3. Existe un aviso observable por paso de migración (RF-3).
4. El enfoque de la instancia existente está cableado y se dispara (RF-4).
5. Los mensajes de error del cierre cumplen RNF-1 (RF-5).
6. Existe comprobación verificable en tests para cada RF; sin rojos nuevos sobre la línea base de
   la spec 001.

## Dudas abiertas

Ninguna.
