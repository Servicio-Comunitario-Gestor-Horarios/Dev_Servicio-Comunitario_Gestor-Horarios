# Plan 002 — Generación de horarios y comunicación con el proceso de cálculo

Estado: propuesto (a la espera de aprobación explícita del usuario para pasar a `tasks.md`).
Spec de referencia: `specs/002-generacion-horarios-ipc/spec.md` (aprobada, sin dudas abiertas).
Constitución: `docs/constitution.md` (stack único, separación lógica/UI, tests primero).
Depende de: `specs/001-base-datos-local/spec.md` (base de datos y núcleo de datos del cliente).
Interfaz de consumo: `docs/interfaz-frontend.md` (documentación viva del contrato que expone nuestro lado; la crea/amplía la tarea de documentación de la interfaz, T9, sobre lo creado por la spec 001).

> Este documento describe el CÓMO. No introduce dependencias nuevas: C++17 · Qt6 (Widgets/Network)
> · OR-Tools · CMake/Ninja · CTest.

---

## 0. Arquitectura objetivo y frontera con el solver

```
                     ┌─────────────────────────────────────────────┐
                     │  LADO CLIENTE  (proceso por defecto)        │
    gestor-horarios ─┤  · dueño de la base de datos (spec 001)     │
      (default)      │  · construye JSON de entrada (dominios +    │
                     │    configuración del solver/presets JSON)   │
                     │  · analiza la salida y la muestra            │
                     │  · cliente IPC: salud, apagado, generación  │
                     └───────────────┬─────────────────────────────┘
                                     │  QLocalSocket (JSON NDJSON)
                                     │  solo: health_check · ready ·
                                     │        shutdown · solver_resolve
                     ┌───────────────▼─────────────────────────────┐
                     │  PROCESO DE CÁLCULO  (`--backend`)          │
    gestor-horarios ─┤  QCoreApplication + InternalServer          │
      --backend      │  · OR-Tools CP-SAT únicamente               │
                     │  · NO abre ni escribe la base de datos      │
                     └─────────────────────────────────────────────┘
```

La **configuración del solver y los presets** son archivos JSON gestionados por el lado cliente;
la base de datos no interviene. El proceso de cálculo recibe el JSON de entrada y devuelve el
horario resuelto.

Reparto por capa (constitución §3). **Entregables de esta spec:** `src/backend`, `src/app` y
`src/middleware`. La interfaz Qt de `src/frontend` la implementa **otro equipo**: consumirá la
generación y el contrato de datos que exponemos, documentados en `docs/interfaz-frontend.md`
(sección «Generación de horarios»).

- `src/backend`: construcción/validación del JSON del solver, análisis de salida y lectura/escritura de archivos (config, presets, horarios). Sin Qt Widgets.
- `src/middleware`: transporte y validación del protocolo IPC únicamente.
- `src/frontend` (otro equipo): interfaz Qt; consume `backend` en proceso y el cliente IPC de `middleware` según el contrato expuesto.
- `src/app`: modo `--backend` y ciclo de vida del proceso de cálculo.

---

## 1. Archivos y responsabilidades

### 1.1 Backend — preparación/validación de la generación (RF-1, RF-3, RF-4)

| Archivo | Responsabilidad | RF |
|---|---|---|
| `src/backend/include/backend/services/ConstructorEntradaSolver.hpp` + `.cpp` **(nuevo)** | Toma una instantánea de los dominios (`DatosDominio`) y de la configuración del solver/presets (JSON); construye el JSON de entrada de las 13 secciones del contrato. Deriva `dimensiones`. Marca `meta.fecha_modificacion` desde un `QDateTime` inyectado. | RF-1 |
| `src/backend/include/backend/services/CargadorConfiguracionSolver.hpp` + `.cpp` **(nuevo)** | Carga/guarda la configuración del solver y los presets como archivos JSON (no base de datos). Informa de fallos de lectura/escritura. | RF-1, RF-4 |
| `src/backend/include/backend/services/ValidadorSalidaSolver.hpp` + `.cpp` **(nuevo)** | Aplica las validaciones posteriores del contrato (P1–P4): conflictos de solapamiento, horas no cubiertas, exceso de horas de profesor, exceso de capacidad de aula. Devuelve `AnalisisSalida` presentable o no. | RF-3 |
| `src/backend/include/backend/services/ServicioGeneracion.hpp` + `.cpp` **(nuevo)** | Orquesta una generación: snapshot → construir entrada → validar pre (V1–V12 con `SolverConfig::fromJson`) → enviar por IPC → esperar con plazo de 60 s → validar salida → marcar "datos anteriores" si el snapshot cambió. Ignora resultados tardíos. | RF-1, RF-3 |
| `src/backend/.../ServicioHorarioSalida` **(existente, se consolida)** | Lectura/escritura de un horario generado como archivo JSON en el destino elegido (RF-4). El diálogo de elección de archivo es del frontend. | RF-4 |

> La escritura/lectura de **horarios y presets como archivo** es nuestra (backend); el diálogo de
> archivo corresponde al equipo de frontend. La parte de presets/configuración la cubre
> `CargadorConfiguracionSolver`; la del horario, `ServicioHorarioSalida`.

### 1.2 Middleware — transporte para el proceso de cálculo (RF-1, RF-2)

| Archivo | Responsabilidad | RF |
|---|---|---|
| `src/middleware/src/client/internalclient.cpp` **(modificado)** | Corregir la gestión de estado del socket (F-CRASH1: no llamar a `connectToServer` ya conectado/conectando; un solo vuelo por operación; limpiar operación en vuelo al agotar plazo) para que salud/apagado/generación sean fiables. | RF-1, RF-2 |
| `src/middleware/include/middleware/messages.h` **(modificado)** | Mantener `OP_HEALTH_CHECK`, `OP_LISTO`, `OP_APAGAR`, `OP_RESOLVER_HORARIO`; retirar/marcar como obsoletas las `OP_*_PROFESOR/AULA/MATERIA` de negocio (dejan de viajar por IPC). Añadir códigos de respuesta de timeout/no solución. | RF-1, RF-2 |
| `src/middleware/src/server/internalserver.cpp` **(modificado)** | El servidor del proceso de cálculo solo registra rutas de sistema; `solver_resolve` lo registra `src/app`. | RF-2 |
| `src/middleware/include/middleware/ipc_framing.hpp` | Sin cambios (framing NDJSON ya estable). | — |

### 1.3 App — modo cálculo y ciclo de vida (RF-2, RF-5)

| Archivo | Responsabilidad | RF |
|---|---|---|
| `src/app/src/aplicacion_backend.cpp` **(reescrito)** | Modo proceso de cálculo: `InternalServer`, registra `solver_resolve` (parsea JSON → `SolverConfig` → `Solver::resolver` → responde `HorarioSalida` JSON + metadata), salud y apagado. **No usa la base de datos.** | RF-1, RF-2 |
| `src/app/src/gestor_proceso_backend.cpp` / `.hpp` **(modificado)** | Lanzar el hijo una vez, comprobar salud (5 s) y solicitar apagado ordenado (5 s). Se elimina el reinicio automático con backoff (fuera de alcance). | RF-2 |
| `src/app/src/aplicacion_frontend.cpp` **(modificado)** | Lanza el proceso de cálculo y comprueba salud 5 s; conecta el guardia de cierre de trabajo sin guardar (RF-5). | RF-2, RF-5 |

### 1.4 Interfaz expuesta para el equipo de frontend (contrato de consumo)

La interfaz Qt de `src/frontend` (vista de generación, visualización y diálogos) la implementa
**otro equipo**; **no es un entregable de esta spec**. Nuestro lado expone la lógica y el
contrato de datos que esa interfaz consume, con las firmas, estados y errores detallados en
**`docs/interfaz-frontend.md`** (documentación viva). Aquí solo se resume el reparto.

| Elemento que exponemos | Lo que el frontend hace con él | RF |
|---|---|---|
| `ServicioGeneracion` (estados, señales, plazo de 60 s, huella «datos anteriores») | Vista de generación: botón «Generar» (bloqueado si hay una en curso), estados validando/enviando/calculando/no-factible/inválido/timeout/listo y confirmación «datos anteriores». | RF-1, RF-3 |
| `AnalisisSalida` (avisos P1–P3, conflicto de solapamiento P4) | Rejilla del horario + panel de avisos; nunca presenta como válido un resultado con conflictos. | RF-3 |
| `HorarioSalida` (`metadata`/`horarios`) + servicio de archivos de horario (`guardarHorario`/`cargarHorario`) | Visualización del horario y guardado/carga como archivo JSON (el diálogo de archivo es del frontend). | RF-1, RF-4 |
| `CargadorConfiguracionSolver` | Cargar/guardar la configuración del solver y los presets como JSON. | RF-1, RF-4 |
| Estado de generación en curso / horario sin guardar | Guardia de cierre cancelable al intentar cerrar la aplicación. | RF-5 |
| Salud del proceso de cálculo (5 s) | Aviso no bloqueante al arrancar y continuación del cierre al salir. | RF-2 |

> El contrato completo (firmas, estados, errores y tratamiento esperado) se documentará en
> `docs/interfaz-frontend.md`, sección «Generación de horarios». Esa documentación la crea/amplía
> la tarea de documentación de la interfaz (T9).

---

## 2. Funciones puras (lógica determinista)

Toda la lógica dependiente del tiempo recibe el instante como parámetro (`ahora`, el "hoy"),
para poder testearla sin reloj real. Sin E/S, sin UI.

| Función | Firma (conceptual) | Qué decide | RF |
|---|---|---|---|
| Construir entrada | `QJsonObject construirJsonEntrada(const DatosDominio& datos, const ConfiguracionSolver& config, const QDateTime& ahora)` | JSON de 13 secciones + `dimensiones` derivadas + `meta.fecha_modificacion` | RF-1 |
| Validar entrada | `Resultado<SolverConfig> validarEntradaSolver(const QJsonObject& json)` | V1–V12 del contrato | RF-1 |
| Huella de datos | `QString huellaDatos(const DatosDominio& datos)` | Detectar si los datos cambiaron durante el cálculo ("datos anteriores") | RF-1 |
| Analizar salida | `AnalisisSalida analizarSalidaSolver(const HorarioSalida&, const SolverConfig&)` | Conflictos (P4) + avisos de horas/capacidad (P1–P3) | RF-3 |
| Estampar fecha | `HorarioSalida estamparFechaGeneracion(const HorarioSalida&, const QDateTime& ahora)` | `metadata.fecha_generacion` determinista | RF-1 |

`ConfiguracionSolver` es una estructura en memoria (DTO); se carga desde JSON, no desde la base.

---

## 3. Algoritmos en pseudocódigo

### 3.1 Generación de horarios (RF-1, RF-2, RF-3)

```
generar():
    si ya hay generación en curso: rechazar ("segunda generación no admitida")
    datos  = instantanea de todos los dominios (de la base, vía NucleoDatos)      # spec 001
    config = cargarConfiguracionSolver(archivoJson)                               # JSON
    entrada = construirJsonEntrada(datos, config, ahora)                          # pura
    validacion = validarEntradaSolver(entrada)                                    # V1–V12
    si not validacion.ok:
        mostrar motivo; NO enviar                                                  # RF-1
        return
    huella = huellaDatos(datos)                                                   # pura
    if not procesoCalculoDisponible():
        informar; operar normal; return                                            # RF-1, RF-2
    enviar(OP_RESOLVER_HORARIO, entrada); iniciar temporizador 60 s
    esperar respuesta
    segun resultado:
        TIMEOUT o desconexión:
            informar; abandonar petición; operar normal; ignorar respuesta tardía    # RF-1
        no_factible:
            informar; no ofrecer guardar                                             # RF-1
        contrato_inválido:
            informar; no presentar como válido                                       # RF-1
        ok:
            salida = estamparFechaGeneracion(parsear(respuesta), ahora)              # pura
            analisis = analizarSalidaSolver(salida, config)                          # pura, P1–P4
            si analisis.hay_conflicto_solapamiento:
                informar "el cálculo falló"; no presentar                            # RF-3
            si no:
                mostrar horario generado + avisos de horas/capacidad                 # RF-3
                si huellaDatos(instantanea actual) != huella:
                    marcar "corresponde a datos anteriores"                          # RF-1
                    pedir confirmación antes de guardar
                guardar solo si el usuario lo pide (archivo JSON, diálogo del frontend) # RF-1, RF-4
```

Proceso de cálculo (`--backend`), ruta `solver_resolve` (RF-1, RF-2):

```
al recibir solver_resolve(payload):
    config = SolverConfig::fromJson(payload)    # nunca toca la base de datos
    si not config.ok: responder error de validación
    res = Solver::resolver(config.valor)
    responder: exito/no_factible + HorarioSalida JSON + metadata
# El proceso NO abre DatabaseManager ni ningún archivo de base de datos. (RF-2)
```

### 3.2 Guardia de cierre (RF-5)

```
closeEvent():
    motivos = []
    si hay horario generado sin guardar: añadir motivo
    si hay generación en curso: añadir motivo
    si motivos no vacío:
        preguntar "¿cerrar igualmente?" (Cancelar / Cerrar)
        si cancelar: ignorar evento; no se cierra
    solicitar apagado del proceso de cálculo con plazo 5 s; continuar el cierre pase lo que pase
```

---

## 4. Interfaz (cómo se pinta) — comportamiento esperado del frontend

> Estos elementos los implementa el equipo de frontend. Se describen aquí como el
> comportamiento que debe ofrecer la interfaz que consume nuestro contrato (ver §1.4 y
> `docs/interfaz-frontend.md`); no son entregables de esta spec.

| Elemento | Comportamiento visual | RF |
|---|---|---|
| Vista de generación | Botón "Generar"; bloqueado mientras hay una generación en curso. Estados: validando, enviando, "calculando…", no hay solución, resultado inválido, tiempo agotado, listo. | RF-1, RF-2 |
| Resultado "datos anteriores" | Aviso destacado + confirmación obligatoria antes de guardar. | RF-1 |
| Visualización de horario | Rejilla por curso (día × slot) con materia/profesor/aula; panel de **avisos** (horas no cubiertas, exceso de horas de profesor, exceso de capacidad de aula). Nunca se muestra un resultado con conflictos de solapamiento como válido. | RF-3 |
| Guardado como archivo | Diálogo de archivo para horario o preset; si falla la escritura, se avisa, se mantiene el contenido en pantalla y se permite reintentar. | RF-1, RF-4 |
| Guardia de cierre | Diálogo cancelable que enumera el horario sin guardar y/o la generación en curso. | RF-5 |
| Salud del proceso de cálculo | Al arrancar: si no responde en 5 s, aviso no bloqueante. Al cerrar: si no responde en 5 s, el cierre continúa. | RF-2 |

---

## 5. Decisiones técnicas

Cada decisión incluye la alternativa descartada y el motivo (constitución §1 y §3).

| # | Decisión | Alternativa descartada | Motivo |
|---|---|---|---|
| D-1 | El proceso de cálculo solo resuelve; nunca toca la base. | Mantener el CRUD por IPC hacia el proceso `--backend` (estado actual). | Es el núcleo de RF-2: el proceso de cálculo no accede a la base. |
| D-2 | Separar la parte de cálculo de la capa de persistencia en lo que consume. | Dejar el proceso de cálculo dependiendo de `backend` completo (DB incluida). | Reduce el acoplamiento del proceso de cálculo con la persistencia; sin dependencias nuevas. |
| D-3 | Configuración del solver y presets como archivos JSON. | Guardarlos en la base de datos. | Decisión del usuario: la base solo persiste entidades de dominio. |
| D-4 | Plazo de generación con `QTimer` en `ServicioGeneracion`; respuestas tardías descartadas. | Espera bloqueante al resultado. | No congela la UI y hace posible "dar por abandonada la petición" (RF-1). |
| D-5 | "Datos anteriores" por huella (hash) del snapshot de dominio. | Comparar marcas de tiempo de las filas. | Las marcas pueden no cambiar con precisión de segundos ni cubrir todos los dominios (RF-1). |
| D-6 | Validaciones posteriores (P1–P4) en el cliente y puras. | Confiar solo en los avisos del solver. | RF-3 exige que el sistema las aplique y decida qué es presentable. |
| D-7 | `metadata.fecha_generacion` y `meta.fecha_modificacion` se estampan con un `ahora` inyectado. | Leer el reloj dentro de las funciones. | Determinismo y testabilidad (sección "funciones puras"). |
| D-8 | Corregir la gestión del socket de `InternalClient`. | Dejar el comportamiento actual (F-CRASH1: tormenta de reintentos). | Salud/apagado/generación deben ser fiables (RF-2). |
| D-9 | Quitar el reinicio automático con backoff del proceso de cálculo. | Conservar los reintentos automáticos de `GestorProcesoBackend`. | El fuera de alcance excluye recuperación automática; el reinicio enmascararía fallos (RF-2). |

---

## 6. Estrategia de tests

Comando del proyecto (constitución §4 y AGENTS.md):

```bash
cmake --preset full && cmake --build build && ctest --preset full
```

- **Tests primero** en cada tarea: se escriben en rojo, luego el código, y la tarea solo se
  marca al pasar en verde (constitución §4).
- Frameworks ya integrados: **Google Test** (lógica/backend) y **Qt Test** (middleware). Se
  registran con `add_gtest` / `add_qtest` en `test/CMakeLists.txt`.
- Los tests de interfaz (`test/frontend/...`) son responsabilidad del equipo de frontend; en su
  lugar verificamos el contrato que exponemos (estados de generación y `Resultado<T>`).
- Los tests de solver (OR-Tools) solo corren en el preset `full` dentro de Docker; los de lógica,
  construcción/validación de JSON y middleware son independientes de OR-Tools.

| Capa | Archivo de test (nuevo/ext.) | Qué comprueba | RF |
|---|---|---|---|
| Backend puro | `test/backend/test_constructor_entrada.cpp` (GTest/QTest) | JSON de 13 secciones; `dimensiones` derivadas; V1–V12 (sin materias/profesores o curso con 0 estudiantes → error y no envía). | RF-1 |
| Backend puro | `test/backend/test_validador_salida.cpp` (GTest) | P1–P3 generan avisos; P4 (solapamiento) → no presentable. | RF-3 |
| Backend puro | `test/backend/test_cargador_configuracion.cpp` (QTest) | Config/presets se leen y escriben como JSON; fallo de archivo → aviso y reintento. | RF-1, RF-4 |
| Backend | `test/backend/test_servicio_generacion.cpp` (QTest) | Timeout 60 s → abandona e ignora resultado tardío; no-factible; contrato inválido; huella distinta → "datos anteriores". | RF-1 |
| Middleware | `test/test_middleware_transport.cpp` (extender) | Ruta `solver_resolve`; apagado ordenado; salud; socket reutilizable sin tormenta de reintentos. | RF-1, RF-2 |
| App/integración | `test/test_ipc_aislamiento_bd.cpp` (QTest) | El proceso de cálculo arranca y resuelve sin crear ni abrir archivo de base; rechaza ops de negocio. | RF-2 |
| Backend (contrato) | `test/backend/test_contrato_generacion.cpp` (QTest) | La semántica que consume el frontend: estados de `ServicioGeneracion` (calculando/no-factible/inválido/timeout/«datos anteriores») y `Resultado<T>` con código de error. | RF-1, RF-3, RF-5 |

Cobertura por criterio de finalización de la spec: cada RF tiene al menos un test verificable
(la traza se formaliza en `tasks.md` con "Hecho cuando:").

---

## 7. Trazabilidad RF → partes del plan

| RF | Partes del plan |
|---|---|
| RF-1 | §1.1 (`ConstructorEntradaSolver`, `CargadorConfiguracionSolver`, `ServicioGeneracion`), §1.3 (ruta `solver_resolve`), §1.4 (estados de generación expuestos al frontend), §3.1, tests constructor/generación/middleware/contrato |
| RF-2 | §0, §1.2 (middleware), §1.3 (`aplicacion_backend`, `GestorProcesoBackend`), §3.1, §4 (salud), tests middleware/aislamiento |
| RF-3 | §1.1 (`ValidadorSalidaSolver`), §1.4 (`AnalisisSalida` para visualización + avisos), §3.1, test validador de salida |
| RF-4 | §1.1 (`CargadorConfiguracionSolver`, `ServicioHorarioSalida`), test cargador de configuración y archivos de horario |
| RF-5 | §1.3 (cierre del proceso de cálculo), §1.4 (estado expuesto para la guardia de cierre), §3.2, test contrato |

RNF cubierto de forma transversal: RNF-1 (textos en español en todos los avisos y diálogos).

---

## 8. Riesgos y supuestos

- **Dependencia del contrato documentado** en `docs/solver/Motor-Solver-Plan-Completo.md`
  (entrada de 13 secciones, validaciones V1–V12 y P1–P4). Si el contrato cambia, cambia el plan
  y no la spec (fuera de alcance: modificar el contrato JSON).
- **OR-Tools solo en Docker** (ver `MEMORY.md`): los tests que ejercitan el solver deben correr
  con el preset `full` en el contenedor; el resto corre también en host.
- **Tamaño:** alcance acotado a generación, IPC, guardado de archivos y guardia de cierre;
  la base de datos y su CRUD viven en la spec 001.
