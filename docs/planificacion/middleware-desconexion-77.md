# Plan de reparación — ciclo de vida de la conexión IPC (middleware #77)

Documento de plan, read-only. No aplica ninguna corrección al código: la única mutación de este
change es este archivo. El arreglo del código es un change posterior en la misma rama.

- **Change SDD:** `estado-middle-servidor`
- **Capacidad:** `ipc-connection-lifecycle-repair-plan`
- **Rama:** `fix/middleware/#77-desconexion-middleware` (base `9916c93`)
- **Pin de la evidencia de crash:** `develop e3a7a73` — declarado en
  `docs/auditoria/evidencia/ui-probe-verdicts.md:1`. `9916c93` es la base de la rama de este
  change, no un pin declarado por la evidencia de crash.
- **Etiquetas de evidencia usadas en todo el documento:** `verificado`, `inferido`, `no verificado`.
  Cuando la lectura del código y una traza capturada discrepan, **la traza prevalece**.

## 1. Resumen ejecutivo

La tormenta de reconexiones y el desenlace con `SIGSEGV` del teardown **no son un defecto de un
solo archivo**. Son la manifestación de tres defectos distribuidos en dos unidades de traducción, y
reparar solo una de ellas no elimina ninguno de los dos desenlaces observados.

| # | Dimensión | Ubicación | Desenlace que explica | Estado |
|---|-----------|-----------|-----------------------|--------|
| D1 | `connectToServer` incondicional; el socket nunca vuelve a `UnconnectedState` | `src/middleware/src/client/internalclient.cpp:27`, `:44-56`, `:12-14` | Tormenta (`setServerName`, exit 124) | `verificado` |
| D2 | Cadena de reintento de health-check **sin cota** + `connect` acumulado que nunca se desconecta | `src/app/src/gestor_proceso_backend.cpp:122-126` (con `:124-125`) y `:116-117` | Tormenta (se auto-renueva) | `verificado` |
| D3 | `detener()` detiene el timer periódico pero **no cancela** el `QTimer::singleShot` pendiente | `src/app/src/gestor_proceso_backend.cpp:63-77` (con `:65`) | Desenlace post-login, exit 139 | `inferido` (la cadena causal) / `no verificado` (que D3 produzca el SIGSEGV) |

**Por qué la premisa original es incorrecta.** La premisa «el cliente nunca desconecta» describe D1
y sugiere un arreglo de un solo archivo. Es incompleta: aunque se cerrara D1 por completo, la cadena
de reintento de D2 seguiría sin cota y seguiría disparando `sendHealthCheck` indefinidamente, y el
`singleShot` pendiente de D3 seguiría ejecutándose durante el teardown. Un arreglo parcial es una
**violación de contrato** de este plan, no una variante aceptable.

**Decisión que este plan NO toma.** El valor del límite de reintentos del health-check es una
decisión abierta del mantenedor (§9). El plan registra el hueco; no elige un número.

## 2. Alcance

**Dentro:** el ciclo de vida de la conexión IPC del health-check, la cadena de reintento, la
cancelación del temporizador pendiente en el teardown, y los criterios verificables correspondientes.

**Fuera:** todo lo enumerado en §10, que incluye R1, R2, R3 y R4 de la exploración de contexto.

## 3. Cadena de evidencia: una fuente canónica por código de salida

Es el punto de mayor riesgo de este change. Tres correcciones sucesivas del artefacto de
planificación fallaron por atribuir evidencia al archivo equivocado. La regla operativa es:
**cada código de salida tiene exactamente una fuente canónica, y los logs no la contienen.**

| Código de salida | Fuente canónica (única) | Dónde **no** está | Verificado por |
|---|---|---|---|
| `139` (SIGSEGV post-login) | `docs/auditoria/evidencia/ui-probe-verdicts.md:14-19` y `:26` | `ui-xvfb-postlogin-run2.log`, `ui-xvfb-postlogin-run3.log` → **cero** coincidencias | `rg "139\|SIGSEGV\|segv\|Segmentation"` sobre ambos logs → sin resultados |
| `124` (timeout kill, offscreen) | `docs/auditoria/evidencia/crash-diff.txt:2-3` | `crash-run1.log`, `crash-run2.log` → `grep -c "124"` = 0 en ambos | `grep -c` y `rg -n "124" crash-diff.txt` |
| `0 / 0` (`fatal\|abort`) | `docs/auditoria/evidencia/crash-diff.txt:6` — **exclusivamente** el escenario offscreen | No aplica a ningún otro escenario | Lectura de `crash-diff.txt:6` |

Las dos últimas filas son la razón por la que el `0 / 0` de `fatal|abort` **nunca** puede usarse
para denying el desenlace de exit 139: pertenecen a otro escenario de reproducción, con otro
comando, otra plataforma gráfica y otro código de salida. Los logs post-login no registran el
código de salida en absoluto; capturan solo la firma de teardown y terminan exactamente en
`QObject::startTimer`.

**Pin de evidencia.** `docs/auditoria/evidencia/ui-probe-verdicts.md:1` declara `@ develop e3a7a73`;
ese es el pin de la evidencia de crash. `9916c93` es la base de la rama de este change, no un pin
declarado por la evidencia de crash, y no debe presentarse como tal. La comprobación que lo sostiene
va **acotada al subárbol auditado**: `rg "9916c93" docs/auditoria/` → **0 coincidencias**. Sobre
`docs/` sin calificar la búsqueda no puede dar cero, porque este mismo documento cita el token; por
eso la forma sin calificar no es una comprobación válida. `crash-diff.txt:1` solo declara el
timestamp `2026-08-24T15:45:56+00:00`.

## 4. Secuencia de fallo, paso a paso

Escenario offscreen (tormenta). Cada paso cita su origen.

| # | Paso | Ubicación |
|---|------|-----------|
| 1 | `QProcess::started` → se conecta el slot | `gestor_proceso_backend.cpp:21-22` |
| 2 | `lanzarProceso()` emite y arranca el hijo `--backend` | `gestor_proceso_backend.cpp:99-100` |
| 3 | `alIniciarProceso()` programa `QTimer::singleShot(1500 ms)` | `gestor_proceso_backend.cpp:113-115` |
| 4 | La lambda conecta `healthCheckResponseReceived` **dentro** del propio lambda | `gestor_proceso_backend.cpp:116-117` |
| 5 | `sendHealthCheck()` → `enviarSolicitud()` | `internalclient.cpp:17-22` |
| 6 | `connectToServer(SERVER_NAME)` **sin verificar el estado previo** | `internalclient.cpp:27` |
| 7 | Primer health-check: el servidor no ha abierto el socket todavía → `onErrorOccurred` emite `healthCheckResponseReceived(false)` | `internalclient.cpp:58`, `:69` |
| 8 | Rama `else`: `qWarning` y **nuevo** `QTimer::singleShot(1500 ms, …, &sendHealthCheck)` **sin contador** | `gestor_proceso_backend.cpp:122-126` (con `:124-125`) |
| 9 | El ciclo vuelve al paso 5 y se repite indefinidamente | `gestor_proceso_backend.cpp:124-125` |

Secuencia del escenario post-login (teardown). Parte del health-check **exitoso** y de ahí en adelante
el estado del socket ya no se recupera:

| # | Paso | Ubicación |
|---|------|-----------|
| 1 | Health-check responde `ÉXITO`; el socket queda en `ConnectedState` porque nadie lo cierra | `internalclient.cpp:27`, `:44-56` |
| 2 | `onReadyRead` emite la respuesta y **no** desconexa | `internalclient.cpp:44-56` |
| 3 | Rama `if (exito)`: arranca el timer periódico de 5 s | `gestor_proceso_backend.cpp:118-120` |
| 4 | Tick del timer → el slot `alExpiracionVerificacion()`, invocado por la señal `timeout`, → `sendHealthCheck()` → `connectToServer` sobre un socket ya conectado; el timer se armó **una sola vez** en `:120` y no se rearma | `gestor_proceso_backend.cpp:27-28` (invocación del slot) y `:120` (única llamada a `start()`) → `internalclient.cpp:27` |
| 5 | Qt advierte y emite error; `onErrorOccurred` → `healthCheckResponseReceived(false)` | `internalclient.cpp:58-69` |
| 6 | Rama `else` → la cadena de reintento de 1.5 s toma el relevo | `gestor_proceso_backend.cpp:122-126` |
| 7 | `detener()` detiene el timer periódico, envía `terminate()` (SIGTERM) y no cancela el `singleShot` pendiente | `gestor_proceso_backend.cpp:63-77` (con `:65`, `:70`) |
| 8 | El `singleShot` pendiente dispara `sendHealthCheck` durante el teardown | `gestor_proceso_backend.cpp:124-125` |

**Precisión de cadencia (relevante para el arreglo).** El primer fallo lo siembra el timer
periódico de 5 s (`INTERVALO_VERIFICACION_MS`, `gestor_proceso_backend.hpp:106`, armado en
`gestor_proceso_backend.cpp:120`). Todos los ciclos posteriores de la traza observada los produce la
cadena de reintento de 1.5 s (`ESPERA_INICIAL_MS`, `gestor_proceso_backend.hpp:107`): la llamada a
`m_timerVerificacion->start()` aparece **una sola vez** en todo el archivo (`:120`), dentro de la rama
de éxito, y esa rama no vuelve a ejecutarse tras el primer fallo. Consecuencia: acotar solo el timer
periódico no detiene la tormenta; el ciclo observado corre a la cadencia de D2.

La dirección de la cita que `plan-hotfix.md:35` usa para respaldar esa reentrada está **invertida**:
`gestor_proceso_backend.cpp:179-182` es el cuerpo de `alExpiracionVerificacion()`, es decir el slot que
el timer **invoca** (conectado en `:27-28`), y no una llamada que programe el temporizador. El arranque
ocurre en `:120`, una sola vez. La cita de `plan-hotfix.md:35` se marca aquí como falsa respecto de
este punto; el disparador del primer fallo que ese claim describe sí es correcto (§11).

## 5. Las tres dimensiones en detalle

### D1 — Estado del socket nunca verificado ni restaurado

`enviarSolicitud()` llama `connectToServer` de forma incondicional
(`src/middleware/src/client/internalclient.cpp:27`) sin inspeccionar `state()`. `onReadyRead`
(`:44-56`) lee la respuesta, emite las señales y no cierra el socket. El constructor (`:12-14`)
conecta únicamente `connected`, `readyRead` y `errorOccurred`: **no conecta `disconnected`**, por lo
que no existe manejador de desconexión en el cliente. Verificado por inspección: cero ocurrencias de
`disconnectFromServer`, `abort` o `close` en todo el archivo.

Contraste que delimita el defecto: el servidor sí gestiona la desconexión —
`src/middleware/src/server/internalserver.cpp:90-91` conecta `disconnected` y
`onClientDisconnected` (`:141-147`) libera el socket con `deleteLater`. La ausencia es exclusivamente
del lado cliente. `verificado`.

### D2 — Una sola dimensión, dos causas inseparables

D2 **no** es un defecto único. Son dos causas que solo se corrigen juntas:

1. **Cadena de reintento sin cota.** `gestor_proceso_backend.cpp:122-126`; el reintento se
   programa en `:124-125` con `QTimer::singleShot(ESPERA_INICIAL_MS, m_clienteVerificacion,
   &InternalClient::sendHealthCheck)` y no incrementa ningún contador.
2. **`connect` acumulado y nunca desconectado.** El `connect` se hace **dentro** de la lambda de
   `:115-129` (`:116-117`), y esa lambda se re-evalúa en cada `alIniciarProceso()`. Como nunca se
   desconecta, **cada reinicio del proceso suma una conexión más a la misma señal**, multiplicando
   el número de invocaciones por fallo.

Por qué son inseparables: cerrar solo la causa 1 deja activa la 2, de modo que un reinicio del
proceso seguiría multiplicando los disparos; cerrar solo la causa 2 deja activa la 1, de modo que un
único proceso seguiría reintentando para siempre. Un arreglo que toque una de las dos es una
**violación de contrato** de este plan. `verificado` (ambas causas, por lectura de código); el efecto
de multiplicación es `inferido` salvo por la observación directa de la acumulación de ciclos en
`crash-run1.log`/`crash-run2.log`.

### D3 — Temporizador pendiente no cancelado en el teardown

`detener()` (`gestor_proceso_backend.cpp:63-77`) ejecuta `m_timerVerificacion->stop()` en `:65` y
`m_proceso->terminate()` en `:70`. **No cancela el `QTimer::singleShot` pendiente** que D2 pudo haber
programado, ni el lambda de `:115`. Consecuencia: el callback sigue siendo invocado mientras el
proceso se termina, y esa es la vía por la que un `QTimer` puede ser arrancado desde un hilo que no
es el que lo creó. `verificado` para el hueco en el código; `inferido` para la cadena causal
completa (§7).

## 6. Firmas observadas (literales)

### 6.1 Firma de teardown post-login

Los dos runs reproducen la misma secuencia; `ui-xvfb-postlogin-run2.log:10-16` y
`ui-xvfb-postlogin-run3.log:10-16`:

```
GestorProcesoBackend: deteniendo backend...
GestorProcesoBackend: error en QProcess: QProcess::Crashed
Frontend: backend no disponible
GestorProcesoBackend: backend COLAPSÓ con código 15
Frontend: backend no disponible
GestorProcesoBackend: reintento 1 de 3 en 2000 ms
QObject::startTimer: Timers can only be used with threads started with QThread
```

El código 15 es el `SIGTERM` emitido por `terminate()` en `gestor_proceso_backend.cpp:70`. El
`Frontend: backend no disponible` intermedio se explica por `alOcurrirErrorProceso`
(`:168-172`), que emite `backendColapsado` antes de que `alFinalizarProceso` (`:140-161`) emita el
`qWarning` con el código.

La violación de hilo de `QObject::startTimer` **es un defecto observado por derecho propio**, no
ruido de log: está presente en ambos runs post-login. Se registra aquí de forma explícita para que
no se descarte junto con el resto de la firma.

### 6.2 Firma de la tormenta

`docs/auditoria/evidencia/crash-run1.log` y `crash-run2.log`, 32 ocurrencias de cada línea en cada
archivo, que cubren las líneas 10 a 137 (128 de 137):

```
Cliente: Conectando al servidor IPC para operación: "health_check"
QLocalSocket::setServerName() called while not in unconnected state
Cliente: Error de conexión: "QLocalSocket::connectToserver: Operation not permitted when socket is in this state"
GestorProcesoBackend: verificación falló, reintentando...
```

El ciclo es: el primer health-check tiene éxito, el socket queda conectado, el timer periódico
vuelve a dispararlo sobre un socket ya conectado → advertencia → `Error de conexión` →
`verificación falló, reintentando...` → repetición. La grafía `connectToserver` con **s** minúscula
es la de Qt 6.11 y es la que aparece en las trazas; se cita literalmente.

`cmp` sobre `crash-run1.log` y `crash-run2.log` no reporta diferencias, y `wc -l` devuelve 137 en
ambos: las trazas son byte-idénticas. Es la afirmación que sostiene `TRACES_IDENTICAL=yes` en
`crash-diff.txt:5`.

## 7. AC-5 en tres niveles de evidencia

El criterio de «salida sin SIGSEGV» no puede sostenerse con una sola afirmación. Se descompone:

| Nivel | Afirmación | Etiqueta | Fuente |
|-------|-----------|----------|--------|
| 1 | **Desenlace**: login aceptado y luego `SIGSEGV` con exit 139 en ambos runs, firma de teardown idéntica, determinista ×2 | `verificado` | `ui-probe-verdicts.md:14-19` (login aceptado con `Login exitoso` en la línea 9 de ambos logs, ambos runs `SEGFAULTED exit 139`, `Deterministic-reproduced ×2`) y `ui-probe-verdicts.md:26` (fila `MainWindow`: «UNREACHABLE for users: integrated exits(139) after login», veredicto `BROKEN`) |
| 2 | **Mecanismo causal**: algún defecto del ciclo de vida de la conexión produce el access violation | `inferido` | No existe backtrace nativo en `docs/auditoria/evidencia/`. Lo confirmaría un backtrace de `gdb`, una línea de `dmesg` con la señal, o un build con símbolos de depuración que reproduzca el exit 139. Lo refutaría un exit 139 idéntico con las tres dimensiones ya cerradas |
| 3 | **Cadena `D3 → SIGSEGV`**: el `QTimer::singleShot` no cancelado produce el access violation | `no verificado` | `docs/auditoria/plan-hotfix.md:35` afirma que el teardown post-login encadena reintento hacia `SIGSEGV 139`. La evidencia sostiene el **desenlace** y la **firma**; no establece esta cadena causal concreta |

El nivel 1 es el único que puede darse por probado hoy. El nivel 2 orienta dónde mirar; el nivel 3
es una hipótesis de trabajo, no un diagnóstico.

## 8. Criterios de aceptación observables

Todos contra símbolos y señales que ya existen. Ninguno requiere inventar un caso de prueba.

| # | Criterio | Señal observable | Etiqueta inicial |
|---|----------|------------------|------------------|
| AC-1 | Sin `connect` redundante | Cero ocurrencias de `QLocalSocket::setServerName() called while not in unconnected state` en el escenario offscreen (base medida: **32** por corrida, ×2 corridas byte-idénticas) | `verificado` reproducido hoy; a confirmar tras el arreglo |
| AC-2 | Cadena de health-check acotada | Constante propia de límite junto a `MAX_INTENTOS_REINICIO` (`gestor_proceso_backend.hpp:105`) y señal crítica al agotar. **Contador propio**; nunca `m_intentosReinicio` | `verificado` (el hueco) / valor del límite `no verificado` (§9) |
| AC-3 | Temporizador pendiente cancelado | `detener()` lo cancela; desaparece la violación `QObject::startTimer: Timers can only be used with threads started with QThread` y el `qWarning` de verificación fallida posterior al cierre | `verificado` (firma actual) / a confirmar |
| AC-4 | Socket en `UnconnectedState` tras la respuesta | La segunda `enviarSolicitud` sí llega al servidor. Hoy es imposible | `verificado` que hoy falla |
| AC-5 | Salida distinta de 139 | Código de salida ≠ 139 en el escenario post-login | Desenlace `verificado`; ausencia del defecto `inferido` |
| AC-6 | Una invocación por fallo en `healthCheckResponseReceived` | Base medida: **32** invocaciones por corrida (`gestor_proceso_backend.cpp:116-117` conectado en cada `alIniciarProceso`) | `verificado` |

### AC-1 reinstated: por qué el corpus correcto son las trazas

Una revisión previa de la especificación descartó este criterio alegando que el símbolo no aparece
en `src/`. Ese argumento es **incorrecto** y el criterio queda **restablecido como confirmado**.

`setServerName` no es un símbolo del proyecto: es un aviso de la biblioteca Qt en tiempo de
ejecución, y por definición no puede aparecer en el código fuente propio. El corpus correcto para
un aviso de runtime es la traza que lo emitió. Medido: **32 ocurrencias en `crash-run1.log` y 32 en
`crash-run2.log`**, con `cmp` sin diferencias y 137 líneas en cada uno, lo que sustenta
`TRACES_IDENTICAL=yes` (`crash-diff.txt:5`). El criterio es empírico y reproducible.

Existe además una string anterior que **no** debe usarse como criterio: se compuso por analogía con
el nombre del método, pero tiene cero ocurrencias tanto en `src/` como en
`docs/auditoria/evidencia/`. No es el mensaje que el runtime emite y no se reproduce aquí para evitar
que vuelva a circular. La string real está en §6.2.

## 9. Decisión abierta: el límite de reintentos

Este plan **no elige** el valor del límite de reintentos del health-check. Se registra como decisión
explícita del mantenedor, porque acotar la cadena cambia un comportamiento del que un mantenedor
podría depender, y el valor correcto depende de criterios operativos que no están en el repositorio
(tolerancia a arranque lento, tiempo máximo de degradación aceptable antes de declarar el backend
caído).

Restricción dura, para que la decisión no se contamination: el límite debe ser un **contador
propio**, con su propia constante junto a `MAX_INTENTOS_REINICIO = 3`
(`src/app/include/app/gestor_proceso_backend.hpp:105`).

`m_intentosReinicio` **no** es candidato. Guarda únicamente el camino de **reinicio del proceso**
(`gestor_proceso_backend.cpp:148` como guarda, `:149` incremento, `:51` reinicio a cero); no protege
en absoluto la cadena de health-check. Reutilizarlo acoplaría dos límites con shedding distinto: un
backend que colapse tres veces dejaría sin margen de verificación. `INTERVALO_VERIFICACION_MS = 5000`
está en `gestor_proceso_backend.hpp:106`; los otros dos valores en uso son
`ESPERA_INICIAL_MS = 1500` (`:107`) y `BASE_ESPERA_REINTENTO_MS = 2000` (`:108`).

## 10. Fuera de alcance

Se enumeran explícitamente para que el arreglo no los absorba por accidente.

| # | Elemento fuera de alcance | Ubicación de referencia |
|---|---------------------------|-------------------------|
| R1 | El servidor nunca devuelve el campo `op`; los widgets descartan toda respuesta en silencio | `internalserver.cpp:188-191` frente a los lectores de `respuesta["op"]` |
| R2 | `correo` en el servidor frente a `email` en el frontend; el alta de docentes siempre falla | `internalserver.cpp:220` |
| R3 | El cliente solo envía la primera petición de su vida, por falta de reset de socket | `internalclient.cpp:27`, `:40` |
| R4 | Las tres operaciones de lectura nunca se piden en producción | `messages.h:22, 30, 38` |
| Framing | Sin delimitador de mensajes: `readAll()` asume que un `read` equivale a un mensaje, dos peticiones rápidas coalescen y la segunda se pierde | `internalserver.cpp:105` |
| `m_responded` | Bandera única compartida por todos los sockets, usada como estado por petición; con dos clientes la respuesta de uno cancela el timeout del otro | Usos en `internalserver.cpp:123`, `:126`, `:187`; declaración en `internalserver.h:30` |
| `removeServer()` | Llamada incondicional: una segunda instancia borra el socket de la primera | `internalserver.cpp:25` |
| `MainWindow` | Bootstrap: el flujo integrado retorna tras aceptar el login y la ventana principal es inalcanzable | `ui-probe-verdicts.md:26`; §4 de la exploración de contexto |
| Puente | Conectar el middleware con el backend de dominio: es cambio de tamaño mayor, no absorbed aquí | Change propio |
| Refactors | Cualquier rediseño, reordenamiento o limpieza no requerida por D1, D2 o D3 | — |

**Solapamiento que debe arbitrar el mantenedor.** R3 y D1 recaen sobre el mismo código
(`internalclient.cpp:27`). R3 describe el contrato roto —«solo la primera petición llega al
servidor»— y D1 describe la causa que la tormenta comparte. Cerrar D1 restaura mecánicamente el
síntoma de R3; ese es un efecto consecuencia, no una unidad de trabajo adicional. R3 se mantiene
abierto como ruptura de contrato independiente, y la delimitación de su alcance es una decisión
del mantenedor, no una omisión de este plan.

## 11. Reconciliación con `docs/auditoria/plan-hotfix.md`

`plan-hotfix.md:35-37` (bloque TOP-1, `F-CRASH1`) es el documento preexistente más cercano. Se
reconcilia claim por claim; no se ignora ni se duplica.

| Claim de `plan-hotfix.md` | Veredicto | Evidencia |
|---------------------------|-----------|-----------|
| `:35` — `enviarSolicitud()` siempre llama `connectToServer` y nunca desconecta; cero llamadas a `disconnectFromServer`/`abort`/`close` | **Confirma** | `internalclient.cpp:27`; cero ocurrencias de los tres símbolos en el archivo |
| `:35` — el servidor retiene sockets abiertos | **Refuta** | `internalserver.cpp:90-91` conecta `disconnected` con `onClientDisconnected`, y `:141-147` libera el socket con `deleteLater()`. El servidor **no** retiene sockets: los libera en la desconexión, tal como registra §5 (D1). `internalserver.cpp:83-92` por sí solo es evidencia insuficiente del claim, porque ese rango no incluye el manejador que lo refuta |
| `:35` — el timer de 5 s re-dispara sobre el socket en `ConnectedState` | **Confirma y extiende** | Correcto como disparador del primer fallo. Se extiende con §4: los ciclos posteriores de la traza los produce la cadena de 1.5 s de D2, porque `m_timerVerificacion->start()` aparece una sola vez (`:120`), en la rama de éxito que ya no se re-evalúa. Acotar solo el timer periódico no detiene la tormenta |
| `:35` — warning `setServerName` y retry-storm infinito | **Confirma** | §6.2; 32 ocurrencias por corrida, trazas byte-idénticas |
| `:35` — el teardown post-login encadena reintento hacia `SIGSEGV 139 ×2` con firma idéntica | **Confirma el desenlace, no la cadena** | El desenlace es `verificado` (§7, nivel 1). La cadena reintento → SIGSEGV es `no verificado` (§7, nivel 3): no hay backtrace en `docs/auditoria/evidencia/` |
| `:35` — Fix sketch: validar `state()` y desconectar tras la respuesta, todo en `enviarSolicitud()` | **Extiende en alcance, confirma en dirección** | La dirección es correcta. El alcance declarado es insuficiente: no cubre D2 (cadena sin cota y `connect` acumulado en `gestor_proceso_backend.cpp:116-117`, `:122-126`) ni D3 (`singleShot` no cancelado en `detener()`). Effort «S — un archivo» es **impreciso** |
| `:36` — «SIGSEGV: `evidencia/ui-xvfb-postlogin-run{2,3}.log`» | **Imprecisa. Atribución incorrecta** | Esos dos logs no contienen `139` ni `SIGSEGV` ni `segv` ni `Segmentation`: cero coincidencias. Registran la firma de teardown y terminan en la línea 16. La fuente canónica del exit 139 es `ui-probe-verdicts.md:14-19` y `:26` |
| `:36` — `evidencia/crash-run1.log` ≡ `evidencia/crash-run2.log`, 137 líneas byte-idénticas | **Confirma** | `cmp` sin diferencias; `wc -l` = 137 en ambos |
| `:37` — re-ejecutar el crash repro offscreen ×2 y el run Xvfb post-login para verificar desaparición de storm y segfault | **Confirma como método, se mantiene diferido** | §12. Los dos escenarios son los correctos, y cada uno conserva su propio código de salida |

**Brecha de trazabilidad que se registra, no se disimula.** `crash-diff.txt:4` referencia
`crash-runA.log` y `crash-runB.log` en la línea `diff crash-runA.log crash-runB.log:`, pero los
archivos comprometidos en el repositorio se llaman `crash-run1.log` y `crash-run2.log`. La
correspondencia de contenido es evidente —mismo escenario, mismo conteo de 137 líneas, misma
afirmación de identidad byte a byte—, pero **el nombre no cierra**. Ninguna corrección de este plan
debe dar por resuelta esta discrepancia. Queda registrada como hueco abierto.

## 12. Verificación diferida en contenedor

Nada de lo anterior es verificable **en este entorno**. Se registra el estado real, medido, en lugar
de asumirlo.

| Comprobación | Estado observado |
|--------------|------------------|
| `pkg-config --exists Qt6Core` | `TRUE`, versión `6.11.2` |
| Cabeceras de desarrollo de Qt6 (`/usr/include/x86_64-linux-gnu/qt6/`) | **No existen.** Esta es la razón real por la que no compila nada: el paquete está registrado, pero no hay cabeceras que incluir |
| `pkg-config --exists gtest` | `TRUE` |
| OR-Tools | Ausente |
| `build/` | Vacío |
| Daemon de Docker | **Caído** |
| `docker/Dockerfile.dev` | Existe |

Vía de verificación futura, la del proyecto: `docker/Dockerfile.dev`, después
`cmake -S . -B build -G Ninja`, y `ctest --test-dir build/test`. A eso se añade la re-ejecución de
los dos escenarios de §11: el repro offscreen para AC-1 y AC-6, y el run post-login para AC-3 y
AC-5.

La evidencia ya capturada bajo `docs/auditoria/evidencia/` **sigue siendo evidencia válida del
defecto tal como estaba en su pin**, `develop e3a7a73`. Que no sea reproducible en esta máquina no
la invalida: la obliga a leer con su pin, que es como debe citarse.

## 13. Siguiente paso

El arreglo del código es un **change posterior en la misma rama**
(`fix/middleware/#77-desconexion-middleware`), dividido en work units cada una por debajo del
presupuesto de 400 líneas:

1. **WU-1 — Cadena acotada y temporizador cancelado.** D2 (ambas causas) y D3, con la constante de
   límite de §9 ya resuelta por el mantenedor. Bloqueante de todo lo demás.
2. **WU-2 — Estado del socket y cierre tras la respuesta.** D1, con test QTest que hoy fallaría por
   el estado del socket.
3. **WU-3 — este documento de plan.**

WU-1 y WU-2 no son intercambiables: cerrar solo una de las dos dimensiones deja el defecto
observable, según §1 y §5.

## Checklist de trazabilidad

Las casillas se dejan deliberadamente sin marcar: comprobarlas y firmarlas corresponde al
mantenedor y al verificador, no a quien redacta el plan. Lo que este apartado declara es dónde
reside cada afirmación, para que la comprobación sea local.

### Trazabilidad por requisito de la especificación

Cada ítem nombra el requisito de `ipc-connection-lifecycle-repair-plan` con su título literal y la
sección de este documento que lo sostiene.

- [ ] R1 «Entregable único» → §2, que delimita el alcance, y §13, que sitúa el arreglo del código
  en un change posterior y asigna a este documento la work unit WU-3. El change entrega
  exactamente un archivo bajo `docs/planificacion/`.
- [ ] R2 «Base de evidencia empírica» → §3, donde cada código de salida tiene una única fuente
  canónica citada con archivo y líneas, y §8, subsección «AC-1 reinstated», que registra la base
  medida de 32/32 por log, `cmp` sin diferencias, 137 líneas y `TRACES_IDENTICAL=yes`.
- [ ] R3 «Dimensiones D1, D2 y D3» → §5, que documenta las tres y presenta D2 como una sola
  dimensión con dos causas inseparables, cuyo cierre parcial es violación de contrato.
- [ ] R4 «El fix de un solo archivo es insuficiente» → §5, que declara la violación de contrato, y
  §11, donde el fix sketch de `plan-hotfix.md:35` queda marcado como insuficiente en alcance y con
  el effort «S — un archivo» declarado impreciso.
- [ ] R5 «Criterios de aceptación» → §8, tabla AC-1 a AC-6, con señal observable y etiqueta
  inicial por criterio.
- [ ] R6 «AC de setServerName empírico y reproducible» → §8, subsección «AC-1 reinstated»: el
  corpus correcto son los logs y nunca `src/`, y el criterio queda restablecido como confirmado.
- [ ] R7 «La cota de reintentos es decisión del mantenedor» → §9, que declara la decisión abierta,
  no fija ningún valor para la cadena de health-check y descarta `m_intentosReinicio` como
  candidato.
- [ ] R8 «AC-5 separa desenlace verificado de mecanismo inferido» → §7, con los tres niveles
  etiquetados por separado: desenlace `verificado`, mecanismo causal `inferido` y cadena
  `D3 → SIGSEGV` `no verificado`.
- [ ] R9 «Verificación diferida y entorno corregido» → §12, con los hechos de entorno medidos y la
  vía diferida a `docker/Dockerfile.dev`, `cmake -S . -B build -G Ninja` y
  `ctest --test-dir build/test`.
- [ ] R10 «Alcance excluido registrado» → §10, con R1, R2, R3 y R4, la bandera compartida
  `m_responded`, `removeServer()` incondicional, el bootstrap de `MainWindow`, la conexión del
  backend y los refactors, más el solapamiento R3/D1 registrado como arbitraje explícito del
  mantenedor.
- [ ] R11 «Reconciliación con plan-hotfix.md» → §11, reconciliada claim por claim: la fila de
  `plan-hotfix.md:35` sobre retención de sockets del servidor lee **Refuta**, y la de `:36` queda
  marcada como imprecisa en su atribución.

### Ítems transversales

Los ítems siguientes no corresponden a un requisito único: atraviesan varios a la vez. Se conservan
sin cambios respecto de la versión anterior de esta lista.

- [ ] AC-1 declara el corpus correcto (las trazas) y la base medida de 32/32
- [ ] Ninguna afirmación de §3 se apoya en el log equivocado
- [ ] Los tres niveles de AC-5 están etiquetados por separado
- [ ] D2 se presenta como una dimensión con dos causas inseparables
- [ ] El límite de reintentos aparece como decisión abierta, sin número
- [ ] `m_intentosReinicio` no se presenta como candidato
- [ ] La atribución de `plan-hotfix.md:36` está marcada como imprecisa
- [ ] La brecha `crash-runA/B` está registrada
- [ ] El pin de evidencia es `e3a7a73`; `9916c93` es la base de la rama y no un pin declarado por la evidencia (`rg "9916c93" docs/auditoria/` → 0)
- [ ] Fuera de alcance completo, con R1-R4 y las ubicaciones
