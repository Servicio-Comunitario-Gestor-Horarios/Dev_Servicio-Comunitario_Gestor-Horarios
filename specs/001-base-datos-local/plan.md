# Plan 001 — Base de datos local: arranque, persistencia de dominios e instancia única

Estado: propuesto (a la espera de aprobación explícita del usuario para pasar a `tasks.md`).
Spec de referencia: `specs/001-base-datos-local/spec.md` (aprobada, sin dudas abiertas).
Constitución: `docs/constitution.md` (stack único, separación lógica/UI, tests primero, protección de datos).
Interfaz de consumo: `docs/interfaz-frontend.md` (documentación viva del contrato que expone nuestro lado; la crea y mantiene la tarea de documentación de la interfaz, T8).

> Este documento describe el CÓMO. No introduce dependencias nuevas: todo se resuelve con
> C++17 · Qt6 (Widgets/Sql) · SQLite · CMake/Ninja · CTest. El proceso de cálculo (solver) se
> trata en la spec 002.

---

## 0. Arquitectura objetivo

Un único binario `gestor-horarios` con dos modos (se mantiene el diseño dual actual):

- **Lado cliente (proceso por defecto):** `QApplication` + Qt Widgets; dueño de la base de datos
  (abre, migra, respalda, lee y escribe); CRUD de dominios en proceso; instancia única.
- **Proceso de cálculo (`--backend`):** `QCoreApplication` + motor OR-Tools; no abre la base de
  datos. Detallado en el plan 002.

Cambio central respecto al código actual: la base de datos deja de intervenir en el proceso
`--backend`. Los `Servicio*` y `DatabaseManager` pasan a usarse en el proceso cliente; los widgets
dejan de mandar `OP_CREAR/ACTUALIZAR/ELIMINAR_*` por IPC y llaman a los servicios en proceso.

Reparto por capa (constitución §3). **Entregables de esta spec:** `src/backend`, `src/app` y
`src/middleware`. La interfaz Qt de `src/frontend` la implementa **otro equipo**: consumirá en
proceso la lógica y el contrato de datos que expone `src/backend`, documentados en
`docs/interfaz-frontend.md` (sección «Interfaz expuesta para el equipo de frontend»).

- `src/backend`: dominios, `DatabaseManager`, migraciones, respaldo, servicios CRUD. Sin Qt Widgets.
- `src/middleware`: transporte del protocolo IPC (detallado en el plan 002).
- `src/frontend` (otro equipo): interfaz Qt; consume `backend` en proceso según el contrato expuesto.
- `src/app`: arranque, instancia única, apertura de base y ciclo de vida.

---

## 1. Archivos y responsabilidades

### 1.1 Backend — base de datos y arranque (RF-1, RF-4, RF-6)

| Archivo (nuevo/modificado) | Responsabilidad | RF |
|---|---|---|
| `src/backend/include/backend/database/version_esquema.hpp` + `src/database/version_esquema.cpp` **(nuevo)** | Constante `VERSION_ESQUEMA_ACTUAL`; lista ordenada de migraciones; `aplicarHasta(db, versionDestino)`; lectura/escritura de `PRAGMA user_version`. Reemplaza el actual `Migracion::runAll` (baseline v1 = esquema actual). | RF-1 |
| `src/backend/include/backend/database/respaldo.hpp` + `src/database/respaldo.cpp` **(nuevo)** | `crearRespaldo(db, ruta)` vía `VACUUM INTO`; `validarRespaldo(ruta)` (`integrity_check` + versión compatible); `restaurarRespaldo(rutaOrigen, rutaDestino)`; `descartarBaseYcrearNueva(...)`. Nombres con marca de tiempo. | RF-1, RF-4, RF-6 |
| `src/backend/include/backend/database/apertura_base_datos.hpp` + `src/database/apertura_base_datos.cpp` **(nuevo)** | Orquesta el arranque: decidir → respaldar → migrar → verificar; devuelve un `ResultadoApertura` (estado + detalle + estado conocido) que la UI traduce al diálogo de fallo. | RF-1, RF-4 |
| `src/backend/src/database/DatabaseManager.cpp` / `.hpp` **(modificado)** | Baja a "abrir/cerrar conexión" y delega migración/versión. Se mantiene como API de conexión SQLite. | RF-1 |
| `src/backend/src/database/migracion.cpp` **(reescrito)** | Migraciones versionadas y transaccionales (el `CREATE TABLE IF NOT EXISTS` actual queda como migración v1). Sigue en `namespace Migracion`. | RF-1 |

Nuevas tablas de dominio que exige RF-2 (migración v2):
- `Cursos(id, nombre UNIQUE, turno, aula_fija, num_estudiantes, codigo_plan FK)`
- `Curso_Materia(id_Curso FK, id_Materia FK, horas_semanales)`
- `Turnos(nombre PK, inicio, fin, slots)` y `Recesos(turno FK, despues_de_slot, duracion, inicio, fin)`

> La configuración del solver NO es tabla de base de datos: se gestiona como archivo JSON
> (plan 002). La base solo persiste dominios de entidad.

### 1.2 Backend — núcleo de datos del cliente (RF-2, RF-3)

| Archivo | Responsabilidad | RF |
|---|---|---|
| `src/backend/include/backend/services/NucleoDatos.hpp` + `src/services/NucleoDatos.cpp` **(nuevo)** | Fachada única sobre `DatabaseManager` + `ServicioAula/Profesor/Materias/PlanesEstudio/Cursos/TurnosRecesos`. Expone listar/crear/actualizar/eliminar con `Resultado<T>`, cálculo de dependencias en cascada, ejecución atómica (transacción) y registro de operaciones pendientes de reintento. Es lo que consumen los widgets. | RF-2, RF-3 |
| `src/backend/include/backend/services/GestorPendientes.hpp` + `src/services/GestorPendientes.cpp` **(nuevo)** | Estado en memoria de cambios pendientes: marca alta/modificación/eliminación fallida, permite reintentar y exponer el estado por registro para la UI. | RF-3, RNF-4 |
| `src/backend/include/backend/services/ServicioCursos.hpp`, `ServicioTurnosRecesos.hpp` (+ `.cpp`) **(nuevos)** | CRUD de los dominios nuevos siguiendo el patrón de `ServicioAula` (DTO + `Resultado<T>` + extracción para solver). | RF-2 |
| `ServicioMaterias`, `ServicioProfesor`, `ServicioPlanesEstudio`, `ServicioAula` **(modificados)** | Añadir consulta de dependientes (para la cascada) y garantizar transaccionalidad dentro de las operaciones compuestas. | RF-2 |

### 1.3 App — arranque e instancia única (RF-1, RF-4, RF-5, RF-6)

| Archivo | Responsabilidad | RF |
|---|---|---|
| `src/app/src/instancia_unica.cpp` + `include/app/instancia_unica.hpp` **(nuevo)** | `QLocalServer` de instancia única; primario atiende "activar" y emite señal para enfocar; secundario conecta, pide activar y espera acuse con plazo de 10 s. Activo desde antes de abrir la base y durante migración/diálogo de fallo. | RF-5 |
| `src/app/src/aplicacion_frontend.cpp` **(reescrito)** | Orquesta el arranque: instancia única → apertura de base (RF-1) → si falla, diálogo (RF-4) → crea `NucleoDatos` → `MainWindow` con las dependencias → guardia de cierre (RF-6). | RF-1, RF-4, RF-5, RF-6 |
| `src/app/main.cpp` **(modificado)** | Parseo de `--backend`/`--version`/`--help`; en modo cliente, instancia única antes de construir la UI. | RF-5 |

### 1.4 Interfaz expuesta para el equipo de frontend (contrato de consumo)

La interfaz Qt de `src/frontend` (vistas, listas, diálogos e indicadores) la implementa **otro
equipo**; **no es un entregable de esta spec**. Nuestro lado expone la lógica y el contrato de
datos que esa interfaz consume, con las firmas, estados y errores detallados en
**`docs/interfaz-frontend.md`** (documentación viva). Aquí solo se resume el reparto.

| Elemento que exponemos | Lo que el frontend hace con él | RF |
|---|---|---|
| `AperturaBaseDatos::Resultado` (`estado`, `detalle`, `rutaRespaldo`, `ok()`) | Diálogo de fallo (RF-4) con Reintentar / Restaurar respaldo / Crear base nueva (pierde datos) / Salir; deshabilita «Crear base nueva» si no hubo respaldo posible. | RF-4, RF-6 |
| Progreso de migración (estado de la apertura) | Indicador «Migrando…» y bloqueo de la operación con datos hasta terminar. | RF-1 |
| `NucleoDatos` (listar/crear/actualizar/eliminar con `Resultado<T>`) | Listas (docentes/aulas/asignaturas/cursos/turnos) pobladas desde la base y CRUD en proceso; confirmación de cascada. | RF-2 |
| Dependencias de cascada (`dependenciasDe`) | Diálogo que lista los dependientes que se borrarán y pide confirmación explícita. | RF-2 |
| Estado por registro (`estadoPendienteDe`, `GestorPendientes`) | Fila normal / **pendiente de guardar** (resaltada) / **pendiente de eliminar** (atenuada) + acción «Reintentar». | RF-3, RNF-3, RNF-4 |
| Consulta de cambios pendientes (RF-6) | Guardia de cierre cancelable al intentar cerrar la aplicación. | RF-6 |

> El contrato completo (firmas, estados, errores y tratamiento esperado) se documentará en
> `docs/interfaz-frontend.md`, sección «Base de datos local». Esa documentación la crea la tarea
> de documentación de la interfaz (T8).

---

## 2. Funciones puras (lógica determinista)

Toda la lógica dependiente del tiempo recibe el instante como parámetro (`ahora`, el "hoy"),
para poder testearla sin reloj real. Sin E/S, sin UI.

| Función | Firma (conceptual) | Qué decide | RF |
|---|---|---|---|
| Decidir apertura | `EstadoApertura decidirApertura(bool existe, int versionArchivo, int versionEsperada)` | Crear / migrar / abrir / fallo por versión ausente\|posterior | RF-1, RF-4 |
| Nombre de respaldo | `QString construirNombreRespaldo(const QString& rutaDb, const QDateTime& ahora)` | Ruta de respaldo con marca temporal determinista | RF-1, RF-4, RF-6 |
| Dependencias de cascada | `QVector<Dependencia> dependenciasDe(const QString& dominio, qint64 id, const InstantaneaDominios& datos)` | Registros que se borrarán en cascada | RF-2 |
| Estado pendiente | `EstadoPendiente estadoPendienteDe(const OperacionPendiente&)` | Si un registro está guardado / pendiente-guardar / pendiente-eliminar | RF-3, RNF-4 |

`InstantaneaDominios`/`DatosDominio` son estructuras en memoria (DTOs ya existentes,
reutilizados). No requieren nuevas dependencias.

---

## 3. Algoritmos en pseudocódigo

### 3.1 Apertura, respaldo y migración (RF-1, RF-4)

```
abrirBaseLocal(ruta):
    existe = archivoExiste(ruta)
    si no existe:
        crearEsquemaInicial(db); fijar user_version = ACTUAL; return OK(creada)   # RF-1
    abrir conexión (si falla → return FALLO(corrupción/permisos))                # RF-4
    version = leer user_version(db)               # ausente/0 = no interpretable
    decision = decidirApertura(existe=true, version, ACTUAL)
    segun decision:
        FALLO_AUSENTE | FALLO_POSTERIOR:
            cerrar db; return FALLO(decision)                                    # RF-1, RF-4
        OK:
            return OK(abierta)
        MIGRAR:
            emitir estado "migración en curso" (bloquea operar con datos)         # RF-1
            rutaRespaldo = construirNombreRespaldo(ruta, ahora)
            si NOT crearRespaldo(db, rutaRespaldo):                              # respaldo obligatorio
                cerrar db; return FALLO(respaldo_imposible)                      # RF-1
            para cada migracion m desde version+1 hasta ACTUAL:
                BEGIN
                  ejecutar m
                  fijar user_version = destino(m)
                COMMIT   # si falla: ROLLBACK → estado conocido = versión previa
                si falla:
                    si no hay transacción rota: cerrar db
                    return FALLO(migracion, respaldo=rutaRespaldo)               # RF-1, RF-4
            return OK(migrada, respaldo=rutaRespaldo)

# Recuperación (diálogo RF-4)
reintentar():              volver a abrirBaseLocal (bucle del diálogo)
restaurarResp(rutaResp):   validarRespaldo(rutaResp) y reemplazar archivo → abrirBaseLocal
crearBaseNueva(ruta):
    si NOT crearRespaldo(baseActual): deshabilitar opción                     # RF-4, RF-6
    pedir confirmación explícita
    descartar actual; crear esquema inicial
    si falla tras descartar: informar y ofrecer restaurar el respaldo
salir():                   cerrar sin escribir (RF-4)
```

### 3.2 CRUD atómico con cascada (RF-2, RF-3)

```
operacionCritica(accion):
    BEGIN
      resultado = accion()
      si not resultado.ok: ROLLBACK → informar (sin parte aplicada)            # RF-2
      si exito: COMMIT; marcar guardado; refrescar vista                        # RF-2, RNF-4
    EXCEPTION E/S: ROLLBACK; informar; registrar cambio pendiente               # RF-3

eliminarConCascada(dominio, id):
    dependientes = dependenciasDe(dominio, id, instantanea(dominio))            # pura
    si dependientes no vacío:
        mostrar dependientes + pedir confirmación explícita                     # RF-2
        si cancela: abortar (sin tocar datos)
    operacionCritica(borrar dominio + dependientes)                             # atómico

reintentarPendiente(op):
    resultado = op.accion()
    si ok: commit; quitar de pendientes; refrescar                               # RF-3
    si no: informar; mantener pendiente (no se pierde de pantalla)              # RF-3, RNF-3
```

### 3.3 Instancia única (RF-5)

```
arranque:
    servidor = QLocalServer(nombre "GestorHorarios_InstanciaUnica")
    si servidor.listen():
        # primario: registrar handler de "activar" durante toda la sesión
        conectar activarSolicitada → levantar/enfocar ventana (o el diálogo activo)
        continuar arranque (base, UI)
    si no:
        # secundario
        socket.connectToServer(nombre); enviar "activar"; esperar acuse ≤ 10 s
        si acuse ok: salir(0)                                   # enfoca existente
        si no: avisar "no se pudo enfocar la instancia existente"; salir(≠0)
```

### 3.4 Guardia de cierre (RF-6)

```
closeEvent():
    si hay cambios pendientes (RF-3):
        preguntar "hay cambios sin guardar, ¿cerrar igualmente?" (Cancelar / Cerrar)
        si cancelar: ignorar evento; no se cierra
    si hay migración/restauración en curso: no permitir cerrar hasta que termine o falle
```

---

## 4. Interfaz (cómo se pinta) — comportamiento esperado del frontend

> Estos elementos los implementa el equipo de frontend. Se describen aquí como el
> comportamiento que debe ofrecer la interfaz que consume nuestro contrato (ver §1.4 y
> `docs/interfaz-frontend.md`); no son entregables de esta spec.

| Elemento | Comportamiento visual | RF |
|---|---|---|
| Diálogo de fallo de base | Ventana modal con la causa (corrupción/permisos/versión) y 4 acciones. "Crear base nueva" deshabilitada y con explicación si no se puede respaldar; textos en español (RNF-1). Se reabre tras cada reintento/restauración fallida. Mientras está activo no se opera con datos. | RF-4, RF-6, RNF-1 |
| Estado de migración | Aviso "Migrando la base de datos…" con la UI de datos bloqueada hasta terminar. | RF-1 |
| Listas (docentes/aulas/asignaturas) | Tabla poblada desde la base al arrancar/abrir vista. Fila normal = guardado. Fila **pendiente de guardar** = resaltada + sufijo "⚠︎ Pendiente de guardar" y acción "Reintentar". **Pendiente de eliminar** = atenuada + sufijo "⚠︎ Pendiente de eliminar". | RF-2, RF-3, RNF-3, RNF-4 |
| Confirmación de cascada | Diálogo que lista los registros dependientes que se borrarán y exige confirmación explícita. | RF-2 |
| Guardia de cierre | Diálogo cancelable que enumera los cambios pendientes. | RF-6 |

---

## 5. Decisiones técnicas

Cada decisión incluye la alternativa descartada y el motivo (constitución §1 y §3).

| # | Decisión | Alternativa descartada | Motivo |
|---|---|---|---|
| D-1 | La base de datos la posee el proceso cliente. | Mantener el CRUD por IPC hacia el proceso `--backend` (estado actual). | Es el núcleo de la spec 001 y de RF-4: la base es del cliente. |
| D-2 | Un único binario dual-mode; separación por comportamiento y rutas. | Dos ejecutables separados (cliente y solver). | Mantiene la simplicidad del stack (constitución §1) y el empaquetado actual. |
| D-3 | Versión de esquema con `PRAGMA user_version`; 0/ausente = fallo de apertura. | Tabla propia `schema_version`. | Nativo en SQLite, menos esquema y menos código; satisface "ausente → fallo". |
| D-4 | Respaldo con `VACUUM INTO`. | Copia de archivo (`QFile::copy`). | `VACUUM INTO` produce un snapshot consistente; la copia puede capturar estado intermedio. |
| D-5 | Migraciones por pasos, cada paso en su transacción, con `user_version` por paso. | Una sola transacción para todas las migraciones. | "Estado conocido" = versión previa intacta y avance incremental (RF-1). |
| D-6 | CRUD en proceso mediante servicios + fachada `NucleoDatos`. | Rutas CRUD por IPC. | La base es del cliente y la lógica queda testeable sin UI (constitución §3). |
| D-7 | Instancia única con `QLocalServer` + mensaje "activar". | `QLockFile` o mutex del sistema. | El lock no permite pedir a la instancia existente que se enfoque (RF-5). |
| D-8 | Pendientes en memoria, por operación, con reintento manual. | Cola persistente con política de reintentos/orden. | El fuera de alcance excluye acumulación, límite y orden de reintentos; se mantiene simple (RF-3). |
| D-9 | La configuración del solver no es tabla de la base (es JSON). | Guardarla en la base. | Corrección del usuario: la base solo persiste entidades de dominio. |
| D-10 | Fecha de modificación estampada con un `ahora` inyectado. | Leer el reloj dentro de las funciones. | Determinismo y testabilidad (sección "funciones puras"). |

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
  lugar verificamos el contrato que exponemos (estados, `Resultado<T>` y estado por registro).

| Capa | Archivo de test (nuevo/ext.) | Qué comprueba | RF |
|---|---|---|---|
| Backend puro | `test/backend/test_version_esquema.cpp` (QTest) | `decidirApertura` en todas las ramas; migración aplica y fija `user_version`; fallo deja versión previa. | RF-1 |
| Backend puro | `test/backend/test_respaldo.cpp` (QTest) | `construirNombreRespaldo` determinista con `ahora`; crear/validar/restaurar; respaldo imposible → no migrar/descartar. | RF-1, RF-4, RF-6 |
| Backend | `test/backend/test_apertura_base_datos.cpp` (QTest) | BD inexistente → crea; versión ausente/posterior → fallo; migración con respaldo; disco/permiso simulado. | RF-1, RF-4 |
| Backend | `test/backend/test_nucleo_datos.cpp` (QTest) | Alta/modificación/baja persisten y se recuperan tras reapertura; cascada atómica (fallo → nada aplicado); dependientes. | RF-2 |
| Backend puro | `test/backend/test_gestor_pendientes.cpp` (GTest) | Estado guardado/pendiente-guardar/pendiente-eliminar; reintento con éxito/sin éxito. | RF-3, RNF-4 |
| App | `test/test_instancia_unica.cpp` (QTest) | Segundo arranque enfoca al primero; sin acuse en 10 s → avisa y no arranca; detección activa durante migración. | RF-5 |
| Backend (contrato) | `test/backend/test_contrato_interfaz.cpp` (QTest) | La semántica que consume el frontend: `AperturaBaseDatos::Resultado::ok()`/`estado`/`rutaRespaldo`, `Resultado<T>::exito/error` con código, y `estadoPendienteDe` (guardado / pendiente-guardar / pendiente-eliminar). | RF-3, RF-4, RNF-4 |

Cobertura por criterio de finalización de la spec: cada RF tiene al menos un test verificable
(la traza se formaliza en `tasks.md` con "Hecho cuando:").

---

## 7. Trazabilidad RF → partes del plan

| RF | Partes del plan |
|---|---|
| RF-1 | §1.1 (`version_esquema`, `respaldo`, `apertura_base_datos`, `DatabaseManager`, `migracion`), §3.1, §4 (estado de migración), tests versión/respaldo/apertura |
| RF-2 | §1.1 (migración v2: dominios nuevos), §1.2 (`NucleoDatos`, servicios), §3.2, §4 (listas/cascada), test `nucleo_datos` |
| RF-3 | §1.2 (`GestorPendientes`), §1.4 (`estadoPendienteDe`/pendientes expuestos al frontend), §3.2, test contrato |
| RF-4 | §1.1 (`apertura`, `respaldo`), §1.4 (`AperturaBaseDatos::Resultado` para el diálogo), §3.1, §4 (comportamiento del frontend), tests respaldo/apertura/contrato |
| RF-5 | §1.3 (`instancia_unica`, `main`, `aplicacion_frontend`), §3.3, test instancia única |
| RF-6 | §1.1 (`respaldo`), §1.3 (guardia de cierre en el arranque), §1.4 (consulta de pendientes expuesta), §3.4, §4, test contrato |

RNF cubiertos de forma transversal: RNF-1 (textos en español en todos los diálogos/avisos),
RNF-2 (apertura normal sin intervención), RNF-3/RNF-4 (estado pendiente sin perder datos),
RNF-5 (transacciones; escrituras confirmadas persistentes).

---

## 8. Riesgos y supuestos

- **Bases de datos antiguas:** no existen bases creadas con anterioridad; las primeras se crearán
  tras esta implementación. Una base sin `user_version` se trata como versión ausente → fallo,
  que es el comportamiento aceptado.
- **Tamaño:** el alcance se acota a la base de datos, el CRUD y la instancia única; la generación
  y el IPC viven en la spec 002.
