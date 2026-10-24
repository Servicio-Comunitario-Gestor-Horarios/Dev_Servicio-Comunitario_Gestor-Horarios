# Journal 009 - Persistencia y servicios de tipos de aula y eventos

## T1 - Build sin frontend (`BUILD_FRONTEND=OFF` + preset `backend`) (2026-10-24)

RNF-6. Base: `471311c`.

### Cambios

- `CMakeLists.txt`: opción `BUILD_FRONTEND` (`ON` por defecto); `BUILD_FRONTEND=OFF`
  omite la interfaz y el ejecutable `gestor-horarios`, pero `src/app` se sigue
  configurando.
- `src/CMakeLists.txt`: `add_subdirectory(frontend)` solo con `BUILD_FRONTEND`;
  `add_subdirectory(app)` se mantiene siempre (`app_core` no depende de la UI).
- `src/app/CMakeLists.txt`: el ejecutable `gestor-horarios` (main + fuentes de UI)
  queda dentro de `if(BUILD_FRONTEND)`; `app_core` sigue fuera del guard.
- `CMakePresets.json`: preset `backend` (configure/build/test) con
  `binaryDir=${sourceDir}/build-backend`, `BUILD_BACKEND=ON`, `BUILD_TESTING=ON`,
  `BUILD_FRONTEND=OFF` y `CMAKE_PREFIX_PATH=$env{HOME}/.local/ortools`.
- `.gitignore`: `build-backend/`.
- `test/CMakeLists.txt`: **sin cambios**; el guard existente
  `if(TARGET frontend AND TARGET backend)` deja fuera `test_frontend_conversion` y
  `test_generacion_seed`.

### Verificación

```bash
cmake --preset backend -Dortools_DIR="$HOME/.local/ortools/lib/cmake/ortools"
# EXIT=0
-- Found ZLIB: /home/luis/.local/ortools/lib/libz.so (found version "1.3.1")
-- Found SCIP: /home/luis/.local/ortools/lib/cmake/scip/scip-config.cmake (found version "10.0.0")
-- OR-Tools found - backend and solver enabled
-- Google Test found — GTest tests enabled
-- Configuring done (2.0s)
-- Generating done (0.3s)
-- Build files have been written to: /home/luis/Repos/Gestor-Horarios/build-backend

cmake --build --preset backend --parallel
# EXIT=0, 243/243 targets; 0 líneas con "error:"/"Error"
[243/243] Linking CXX executable test/test_solver_benchmark

ctest --test-dir build-backend -N
# EXIT=0, Total Tests: 46
Test  #3: test_instancia_unica
Test #11: test_backend_version_esquema
Test #15: test_backend_nucleo_datos
Test #42: test_seed_aplicacion
# sin test_frontend_conversion ni test_generacion_seed
```

Comprobaciones adicionales:

```bash
ls build-backend/src/app/            # libapp_core.a — sin ejecutable gestor-horarios
find build-backend/src -maxdepth 2 -type f -perm -u+x
# build-backend/src/consola/gestor-horarios-consola
# build-backend/src/seed/gestor-horarios-seed
ls build-backend/src/frontend        # no existe: el módulo no se configura

# valor por defecto de la opción (configure limpio fuera del repo, BUILD_BACKEND=OFF)
cmake -S . -B /tmp/opencode/t1-default-frontend -G Ninja -DBUILD_BACKEND=OFF -DBUILD_TESTING=OFF
# EXIT=0; CMakeCache.txt -> BUILD_FRONTEND:BOOL=ON
```

`git status --short` / `git diff --stat`: solo `.gitignore`, `CMakeLists.txt`,
`CMakePresets.json`, `src/CMakeLists.txt` y `src/app/CMakeLists.txt` (más
`specs/009-tipos-de-aula-y-eventos/{estado,plan}.md`, que ya estaban modificados
antes de empezar esta tarea). `build-backend/` no aparece: queda ignorado.

### Hallazgos / desviaciones

- **SCIP obliga a un prefijo extra.** El paquete de OR-Tools de `~/.local/ortools`
  no se resuelve solo con `-Dortools_DIR`: `ortoolsConfig.cmake` hace
  `find_dependency(SCIP REQUIRED)` y `FindSCIP` termina en `SCIP_ROOT: not found`
  (aunque exista `lib/cmake/scip/scip-config.cmake`), porque CMake no busca en el
  directorio hermano del paquete. Se añade `CMAKE_PREFIX_PATH=$env{HOME}/.local/ortools`
  al preset `backend` (en vez de una ruta de usuario fija), con lo que el comando
  tal cual del `Hecho cuando` funciona. En otra máquina sin esa ruta, el prefijo
  inexistente es inocuo y sigue valiendo `-Dortools_DIR`/`CMakeUserPresets.json`.
- La suite completa sigue roja por el esquema WIP de la base, como anticipa el
  plan; T1 solo garantiza el runner sin frontend y que **todo compila**.

## T2 - Esquema v3: identidad/orden de eventos, clave normalizada almacenada, catálogo esperado y huella (2026-10-24)

RF-2, RF-5, RF-6, RF-7, RF-9, RNF-2, RNF-4. Base: `8cfa211`.

### Cambios

- `src/backend/include/backend/database/version_esquema.hpp`: `VERSION_ESQUEMA_ACTUAL = 3`
  con el comentario de qué trae la v3 (catálogo con clave, tipo obligatorio en
  aulas/materias, eventos con identidad y orden, `Recesos` retirada).
- `src/backend/src/database/migracion.cpp`:
  - `TiposAula`: se sustituye `nombre UNIQUE` por `clave TEXT NOT NULL CHECK(length(clave) > 0)`
    y `nombre` con `CHECK(trim(nombre) = nombre AND length(nombre) > 0)`;
    `CREATE UNIQUE INDEX TiposAula_clave ON TiposAula(clave)`.
  - `Eventos`: `id INTEGER PRIMARY KEY AUTOINCREMENT` (identidad subrogada), `orden INTEGER
    NOT NULL CHECK(orden >= 1)`, `despues_de_slot >= 1`, `CHECK(length(trim(nombre)) > 0)`,
    `CHECK((inicio IS NULL) = (fin IS NULL))`, sellos de fecha y
    `UNIQUE(turno, despues_de_slot, orden)` (sustituye la PK compuesta `(turno, despues_de_slot)`).
  - `Aulas`/`Materias` ya llevaban `id_tipo_aula NOT NULL REFERENCES TiposAula(id)`; sin cambios.
- `src/backend/src/database/version_esquema.cpp`: `TABLAS_ESPERADAS` con `TiposAula` y `Eventos`
  (se retira `Recesos`), `TABLAS_RETIRADAS = {Curso_Materia, Recesos}`, y listas
  `COLUMNAS_AULAS`/`COLUMNAS_MATERIAS`/`COLUMNAS_EVENTOS`/`COLUMNAS_TIPOS`. El motivo del fallo
  agrupa las columnas que faltan por tabla («faltan las columnas «…» de la tabla «…»»).
- `test/backend/test_version_esquema.cpp`: versión 3, catálogo/tablas/columnas, rechazo de
  `Recesos` y de `Eventos` sin `orden` / `TiposAula` sin `clave`, unicidad de `clave`
  («Laboratorio»/«laboratorio» y «Física»/«FÍSICA» con clave `física`), unicidad
  `(turno, posición, orden)` con identidad propia, `CHECK` de posición/orden/duración y del par
  de horas, y **huella de la v3 registrada**:
  `{ 3, "ba386397216ac9fb8d427ab4603c09ce3d8342cc5605dda16eecc78a02f1701b" }`.
- `test/backend/test_apertura_base_datos.cpp` y `test/backend/test_esquema_cursos.cpp`: **sin
  cambios**; usan `VERSION_ESQUEMA_ACTUAL` (no hay literales de versión) y siguen verdes.

### Rojo inicial

Con los tests nuevos y el código de `8cfa211` (v2):

```bash
cmake --build build-backend --parallel --target test_backend_version_esquema \
    test_backend_apertura_base_datos test_esquema_cursos   # exit 0
ctest --test-dir build-backend --output-on-failure \
    -R 'test_backend_version_esquema|test_backend_apertura_base_datos|test_esquema_cursos'
# 33% tests passed, 2 tests failed out of 3
#  11 - test_backend_version_esquema (Failed)       Totals: 11 passed, 13 failed
#  14 - test_backend_apertura_base_datos (Failed)   Totals: 12 passed, 1 failed
#  12 - test_esquema_cursos ........................ Passed
# Fallos representativos:
#   versionActual_esTres            -> Actual 2, Expected 3
#   crearEsquema_*                  -> devuelve 2 y no hay `TiposAula.clave` ni `Eventos.orden`
#   tipoAula_claveUnica_*           -> el INSERT de `clave` falla (columna inexistente)
#   eventos_unicoTurnoPosicionOrden -> el INSERT de `orden` falla (columna inexistente)
#   esquemaEsperado_*               -> sigue exigiendo `Recesos`
#   huellaEsquema_...               -> real e887d78… ≠ registrada b768433…
#   abrir_versionYCatalogoCorrectos -> FalloEsquema (el catálogo v2 de 059168f ya no casa
#                                      con TABLAS_ESPERADAS, que aún pedía `Recesos`)
```

Tras el DDL nuevo, la huella aún vacía hizo fallar el guardián mostrando la real
(`ba386397…`), que se copió a `HUELLAS_CONOCIDAS` siguiendo el procedimiento del propio test.

### Verde final

```bash
cmake --build --preset backend --parallel      # EXIT=0, 63/63 targets, 0 "error:"
ctest --test-dir build-backend --output-on-failure \
    -R 'test_backend_version_esquema|test_backend_apertura_base_datos|test_esquema_cursos'
# 100% tests passed out of 3
#  11 test_backend_version_esquema ..... Passed (24 passed, 0 failed)
#  12 test_esquema_cursos .............. Passed
#  14 test_backend_apertura_base_datos . Passed
```

### Comprobaciones manuales (sqlite3 + consola)

```bash
./build-backend/src/consola/gestor-horarios-consola --db /tmp/opencode/t2-v3.db < /dev/null
# Apertura: OkCreada (base nueva)
sqlite3 t2-v3.db 'PRAGMA user_version;'          # 3
# tablas: Aulas Cursos Disponibilidad_Profesor Eventos Materias PlanEstudio
#         PlanEstudio_Materia Profesor_Materia Profesores TiposAula Turnos
#         (sin Curso_Materia ni Recesos)

# índice único de la clave, escritura directa
INSERT INTO TiposAula (nombre,clave) VALUES ('Laboratorio','laboratorio');  -- ok
INSERT INTO TiposAula (nombre,clave) VALUES ('laboratorio','laboratorio');  -- UNIQUE constraint failed
INSERT INTO TiposAula (nombre,clave) VALUES ('Física','física');            -- ok
INSERT INTO TiposAula (nombre,clave) VALUES ('FÍSICA','física');            -- UNIQUE constraint failed
# filas finales: 1|Laboratorio|laboratorio  2|Física|física
# sqlite_master: CREATE UNIQUE INDEX TiposAula_clave ON TiposAula(clave)

# único (turno, posición, orden)
INSERT Eventos ('Receso','Mañana',2,1,10)  -- ok
INSERT Eventos ('Acto','Mañana',2,1,10)    -- UNIQUE constraint failed: Eventos.turno, Eventos.despues_de_slot, Eventos.orden
INSERT Eventos ('Acto','Mañana',2,2,10)    -- ok; ids 1 y 2 distintos

# base v2: falla al abrir, sin migrar
sqlite3 t2-v2-marca.db 'PRAGMA user_version = 2;'
... gestor-horarios-consola --db t2-v2-marca.db   -> EXIT=2
# Detalle: «… declara la versión 2 y esta aplicación espera la versión 3. No se migra …»
# PRAGMA user_version sigue siendo 2 tras el fallo.

# catálogo no esperado con la marca correcta
sqlite3 t2-catalogo-viejo.db 'CREATE TABLE Recesos (...);'
... gestor-horarios-consola --db t2-catalogo-viejo.db -> EXIT=2
# Detalle: «… declara la versión 3, que coincide, pero su contenido no es el esperado
#            (existe la tabla retirada «Recesos»). No se migra …»
```

### Resto de la suite

`ctest --test-dir build-backend` → 76 % (35/46), 11 rojos **esperados** por el modelo viejo
que arreglan T3-T10: `test_backend_nucleo_datos` (exige `Recesos`),
`test_backend_cascada`, `test_backend_gestor_pendientes`, `test_servicio_aula`/`materias`/
`planes_estudio`/`carga_horaria`/`profesor` y `test_composicion_plan`
(`NOT NULL constraint failed: <tabla>.id_tipo_aula`), `test_datos_contexto_base_datos` (mismo
`NOT NULL` al crear materia) y `test_seed_aplicacion` (seed antiguo). Ningún fallo nuevo ajeno a
esos motivos.

### Hallazgos / desviaciones

- **El árbol ya traía T2 a medias.** Al empezar, `git status` mostraba modificados
  `version_esquema.{hpp,cpp}`, `migracion.cpp` y `test_version_esquema.cpp` (un intento previo
  interrumpido, sin commitear y sin huella registrada). Se verificaron contra el plan, se
  respaldaron en `/tmp/opencode/t2-wip`, se generó el rojo restaurando las tres fuentes a
  `8cfa211` (conservando el test nuevo) y después se restauraron y se completó lo que faltaba
  (la huella). No hay cambios fuera de T2.
- La clave normalizada **no se calcula en SQL**: la almacena el servicio (T5) y el índice
  frena la escritura directa; por eso el test inserta la clave a mano.
- `test_esquema_cursos` no necesitó ajustes (no fija la versión con literales).

## T3 - Modelo y servicio de eventos, con compatibilidad (2026-10-24)

RF-1, RF-2, RF-8. Base: `357102d` (T1 y T2 commiteadas; esquema v3 aplicado).

### Cambios

- `src/backend/include/backend/services/ServicioTurnosRecesos.hpp`:
  - `EventoDTO { id, nombre, despuesDeSlot, orden, duracion, inicio, fin }` con `id = -1`
    al crear y `orden = 0` como «automático» solo en el alta.
  - `TurnoDTO.eventos` (ordenados por posición, orden e id) junto a `TurnoDTO.recesos`
    (vista derivada, una entrada por evento —compatibilidad transitoria—).
  - CRUD de eventos por identidad: `agregarEvento`, `actualizarEvento` (turno inmutable,
    `orden` obligatorio), `eliminarEvento`.
  - `agregarReceso`/`eliminarReceso` y `RecesoDTO` se conservan como capa derivada; la
    clase **sigue llamándose `ServicioTurnosRecesos`** hasta T9.
- `src/backend/src/services/ServicioTurnosRecesos.cpp`:
  - `cargarEventos` sustituye a `cargarRecesos`: lee `Eventos` con
    `ORDER BY despues_de_slot, orden, id` y deriva `recesos` en el mismo recorrido.
  - Alta: valida turno existente, nombre, posición `1..numSlots`, duración `> 0`, orden
    (`0` → `máximo + 1` de la posición) y ausencia de `(turno, posición, orden)`
    duplicado; inserta con las horas ausentes como `NULL`.
  - Edición: lee el turno del evento y **no lo cambia** (la firma no lo admite), exige
    `orden >= 1`, rechaza duplicado excluyendo el propio id y conserva el `id`.
  - Borrado por `id`; no renumera. `eliminarTurno` borra ahora sus `Eventos` (antes
    intentaba borrar de la tabla retirada `Recesos` y siempre fallaba).
  - `agregarReceso` delega en `agregarEvento("Receso", …)` y `eliminarReceso` borra los
    eventos de la posición.
- `src/consola/src/ConsolaDatos.cpp`: `turnos eventos "<turno>"`,
  `turnos evento-agregar "<turno>" "<nombre>" <pos> <duracion> [inicio] [fin] [orden]`,
  `turnos evento-actualizar <id> "<nombre>" <pos> <duracion> <orden> [inicio] [fin]` y
  `turnos evento-eliminar <id>`. `turnos listar` añade `eventos=N` y conserva
  `recesos=N`; `receso-agregar`/`receso-eliminar` siguen operativos y la **ayuda no se
  toca** (T9/T10).
- `test/backend/test_servicio_turnos_eventos.cpp` (nuevo) y su `add_qtest` en
  `test/CMakeLists.txt` (sección de backend, junto a `test_servicio_generacion`).

### Rojo inicial

El test nuevo usa la API que T3 debía crear, así que el rojo es de compilación:

```bash
cmake --build build-backend --parallel --target test_servicio_turnos_eventos
# EXIT != 0; 79 líneas "error:":
#   27 × ‘class ServicioTurnosRecesos’ has no member named ‘agregarEvento’
#    6 × ‘…’ has no member named ‘actualizarEvento’; did you mean ‘actualizarTurno’?
#    4 × ‘…’ has no member named ‘eliminarEvento’; did you mean ‘eliminarTurno’?
#   42 × ‘struct TurnoDTO’ has no member named ‘eventos’
```

### Verde final

```bash
cmake --build --preset backend --parallel                     # EXIT=0 ("ninja: no work to do." en la repetición)
ctest --test-dir build-backend --output-on-failure -R 'test_servicio_turnos_eventos'
# 1/1 Test #26: test_servicio_turnos_eventos ..... Passed
# 100% tests passed; Totals: 25 passed, 0 failed (23 casos + init/cleanup)
```

Casos cubiertos: nombre vacío, turno inexistente, posición 0, posición fuera de rango,
duración 0 (alta y `agregarReceso`); orden automático `máximo + 1` (1 y 2 en la misma
posición) y orden explícito duplicado; lista por posición/orden; borrado sin renumerar
(el alta siguiente toma `4`, no `3`); edición y borrado por identidad; edición sin
cambio de turno; `orden = 0` en edición; horas ausentes guardadas como `NULL`;
`eliminarTurno` borra sus eventos; `agregarReceso`/`eliminarReceso` derivan de eventos.

### Compatibilidad (no empeora respecto a la base)

```bash
ctest --test-dir build-backend --output-on-failure \
    -R 'test_backend_nucleo_datos|test_backend_cascada|test_constructor_entrada'
```

| test | base `357102d` | con T3 |
|------|----------------|--------|
| `test_backend_nucleo_datos` | 6 pasan / 7 fallan | **7 pasan / 6 fallan** |
| `test_backend_cascada` | 6 pasan / 12 fallan | 6 pasan / 12 fallan |
| `test_constructor_entrada` | pasa | pasa |

`turnosRecesos_altaModificacionBaja` de `test_nucleo_datos` pasa ahora (el servicio ya
no consulta `Recesos`). Los rojos restantes son los ya conocidos: `Recesos` esperada en
el test y `NOT NULL id_tipo_aula` (T5/T6), y la cascada de turno que sigue apuntando a
la tabla retirada `Recesos` (T6).

Suite completa: `ctest --test-dir build-backend` → **36/47 (77 %)**, los **mismos 11
rojos** que dejó T2 (`nucleo_datos`, `cascada`, `gestor_pendientes`, `servicio_aula`,
`servicio_materias`, `servicio_planes_estudio`, `servicio_carga_horaria`,
`composicion_plan`, `servicio_profesor`, `datos_contexto_base_datos`,
`seed_aplicacion`); ninguno nuevo.

### Comprobaciones manuales (consola, base nueva v3)

```bash
./build-backend/src/consola/gestor-horarios-consola --db /tmp/opencode/t3-consola.db \
    --cmd 'turnos crear "Mañana" 07:00 12:30 8'                 # ok
… --cmd 'turnos evento-agregar "Mañana" "Receso" 2 10 08:20 08:30'   # ok
… --cmd 'turnos evento-agregar "Mañana" "Aviso" 2 5'                 # orden 2
… --cmd 'turnos evento-agregar "Mañana" "Acto cívico" 3 10'          # ok
… --cmd 'turnos listar'         # Mañana | 07:00-12:30 | slots=8 | eventos=3 | recesos=3
… --cmd 'turnos eventos "Mañana"'
#   #1 | pos=2 | orden=1 | 10 min | 08:20-08:30 | Receso
#   #2 | pos=2 | orden=2 |  5 min | - | Aviso
#   #3 | pos=3 | orden=1 | 10 min | - | Acto cívico
… --cmd 'turnos evento-agregar "Mañana" "Mal" 0 10'
#   error: La posición del evento debe estar entre 1 y 8.
… --cmd 'turnos evento-eliminar 2'   # ok; quedan #1 y #3 (sin renumerar)
… --cmd 'turnos receso-agregar "Mañana" 5 10'   # ok (compatibilidad; eventos=3)
… --cmd 'turnos evento-actualizar 1 "Receso largo" 2 15 1 08:20 08:35'  # ok
sqlite3 … "SELECT … quote(inicio), quote(fin) FROM Eventos"
# 1|Mañana|Receso largo|2|1|15|'08:20'|'08:35'
# 3|Mañana|Acto cívico|3|1|10|NULL|NULL
# 4|Mañana|Receso|5|1|10|NULL|NULL
--ayuda   # sigue listando receso-agregar/receso-eliminar (se ajusta en T9/T10)
```

### Hallazgos / desviaciones

- **`ordenSiguienteEn` es un ayudante privado del `.cpp`**, no `calculo_eventos`
  (`ordenSiguiente`). T3 necesita el `máximo + 1` pero `calculo_eventos` y su test son de
  T4; se deja aquí para no crear una función pública sin prueba. T4 la traslada.
- **Guarda del par de horas en T3.** El `CHECK((inicio IS NULL) = (fin IS NULL))` de la
  v3 convertiría un par incompleto en un error opaco de SQLite; el alta y la edición lo
  rechazan con mensaje propio. El resto de la coherencia temporal (diferencia, ventana,
  solape) queda en T4.
- **`agregarReceso` cambia dos bordes respecto a la tabla antigua:** la posición 0 ahora
  se rechaza (antes solo los negativos) y dos altas en la misma posición crean órdenes 1
  y 2 en vez de reemplazar (`INSERT OR REPLACE` por `(turno, posición)`). Ningún
  consumidor actual hace eso (seed y tests insertan un receso por turno); es la
  consecuencia directa de derivar del evento y de N4.
- **`TurnoDTO.recesos` deriva de *todos* los eventos** (una entrada por evento, decisión
  3 del plan), no solo de los llamados «Receso»; un «Acto» aparece también en la vista.
  No hay conflicto con el puente 1.0: es exactamente lo que pide §4.6 y lo que el
  constructor ya consume.
- **La consola usa `ServicioTurnosRecesos` directamente** (`m_contexto.conexion()`)
  porque la fachada `NucleoDatos` no expone eventos hasta T6; allí podrá moverse a la
  fachada. `turnos listar` añade `eventos=N` (conserva `recesos=N`) para que el manual de
  T10 ya refleje 2 → 3; la ayuda sigue con `receso-*` a propósito.
- **No se añadieron** `eventosFueraDeRango` ni `actualizarTurno(..., bool)`: son de T4.
  `NucleoDatos`, `cascada`, la huella, el puente del constructor, el seed y el frontend
  no se tocaron.

## T4 - Coherencia temporal, ventanas y reducción de franjas (2026-10-24)

RF-3, RF-4, RNF-1. Base: `08067ce` (T1-T3 commiteadas).

### Cambios

- `src/backend/include/backend/services/calculo_eventos.hpp` y
  `src/backend/src/services/calculo_eventos.cpp` (nuevos): funciones puras
  `duracionFranjaEstimada`, `ventanaDePosicion`, `eventoDentroDeVentana`,
  `haySolapeCronologico`, `ordenSiguiente` y `eventosFueraDeRango`.
  - `duracionFranjaEstimada` = `(minutos de ventana − Σ duraciones) / numSlots`, con guarda:
    `numSlots <= 0`, ventana no positiva o reparto entero `< 1` → `0` (nunca divide por cero).
  - `ventanaDePosicion(k)` = `inicio + k × franja + Σ duraciones de posiciones < k` hasta
    `inicio + Σ duraciones de la posición`; la última posición (k = `numSlots`) termina en
    `turno.fin` (N5: «la última hasta el fin del turno», incluido el resto entero del reparto).
  - `haySolapeCronologico` ordena por `(posición, orden, id)` y compara con el **fin máximo**
    acumulado (no solo con el evento anterior: A 08:00–10:00, B 09:00–09:10, C 09:20–09:30
    es solape y una comparación por pares contiguos no lo vería).
  - `ordenSiguiente` sustituye al ayudante privado `ordenSiguienteEn` del `.cpp` (T3).
- `src/backend/src/services/ServicioTurnosRecesos.cpp`:
  - `validarCoherenciaTemporal(turnoConCandidato, candidato)`: par de horas completo o ausente,
    `fin > inicio`, `fin − inicio == duracion`, encaje en la rejilla
    (`duracionFranjaEstimada > 0`), intervalo contenido en la ventana de su posición y ausencia
    de solapes/cronología invertida. Se aplica en el alta y en la edición (la edición valida el
    turno con el candidato sustituyendo al evento por su id).
  - `actualizarTurno(..., bool eliminarEventosFueraDeRango = false)` (firma del plan): si se
    reducen franjas y hay afectados, sin confirmación devuelve el error con `#id`, nombre y
    posición; con confirmación los borra. Cambiar la ventana que deja fuera un evento con horas
    se rechaza con el evento, su intervalo y la ventana nueva. El borrado y el `UPDATE` van en
    una sola transacción (rollback ante cualquier fallo).
  - `eventosFueraDeRango(turno, numSlots)` como método (la función pura homónima se invoca con
    `::` para evitar que el método se oculte a sí mismo).
- `src/backend/CMakeLists.txt`: `calculo_eventos.cpp` en la librería `backend`.
- `test/CMakeLists.txt`: `add_qtest(test_calculo_eventos ... backend)`.
- `test/backend/test_calculo_eventos.cpp` (nuevo, 26 casos) y
  `test/backend/test_servicio_turnos_eventos.cpp` (+15 casos de coherencia y turnos).

### Rojo inicial

```bash
cmake --build build-backend --parallel     # EXIT != 0
# test_calculo_eventos.cpp:7:10: fatal error: backend/services/calculo_eventos.hpp: No existe…
# test_servicio_turnos_eventos.cpp:454/460/464: ‘ServicioTurnosRecesos’ has no member
#   named ‘eventosFueraDeRango’
# test_servicio_turnos_eventos.cpp:492: no matching function for call to
#   ‘ServicioTurnosRecesos::actualizarTurno(const char[8], QTime, QTime, int, bool)’
```

Un primer verde parcial dejó 2 rojos de test, ya corregidos: el caso
`eventoDentroDeVentana_dentro_verdadero` usaba horas de la rejilla con dos eventos (franja 55)
cuando el turno de la prueba tenía uno (franja 58), y el caso de reducción sin confirmar pedía
«Receso» con `numSlots = 3` cuando ese evento (posición 2) deja de estar afectado.

### Verde final

```bash
cmake --build --preset backend --parallel      # EXIT=0
ctest --test-dir build-backend --output-on-failure \
    -R 'test_calculo_eventos|test_servicio_turnos_eventos'
# 100% tests passed out of 2
#  test_servicio_turnos_eventos ... Passed (38 casos + init/cleanup)
#  test_calculo_eventos .......... Passed (26 casos + init/cleanup)
```

Casos nuevos: guarda de `numSlots` 0/negativo, ventana invertida y reparto no positivo;
reparto `(ventana − Σ)/franjas` y ventana de cada posición (incluida la última con resto);
contención antes/dentro/después y evento sin horas; solape en la misma posición, entre
posiciones invertidas, contiguo sin solape, entrada desordenada y solape con el primero a
través de un tercero; `ordenSiguiente` y `eventosFueraDeRango` puros. En el servicio: par
incompleto, `fin <= inicio`, diferencia distinta de la duración, fuera de ventana, solape en
la misma posición, cronología invertida respecto a la posición, eventos que no caben en el
turno, solape en la edición, listado de afectados con y sin confirmación, borrado confirmado,
ventana nueva que deja fuera un evento y ventana sin reparto para los eventos.

### Compatibilidad (no empeora respecto a la base)

```bash
ctest --test-dir build-backend         # 37/48 (77 %); los MISMOS 11 binarios rojos de T3
```

`test_constructor_entrada` sigue verde; `test_backend_cascada` sigue en 6/12 y
`test_gestor_pendientes` en 11/2 por las razones conocidas (cascada de turno con la tabla
retirada `Recesos` y `NOT NULL id_tipo_aula`, T5/T6). `test_seed_aplicacion` sigue rojo por
`contarFilasDeDominio` consultando la tabla retirada `Recesos` (T8), no por T4.

Comprobación manual (consola, base nueva v3):

```bash
… --cmd 'turnos crear "Mañana" 07:00 12:30 8'
… --cmd 'turnos evento-agregar "Mañana" "Receso" 2 10 08:20 08:30'   # ok (ventana 08:20–08:30)
… --cmd 'turnos evento-agregar "Mañana" "Mal" 2 10 08:20 08:35'
# error: La duración del evento «Mal» (10 min) no coincide con la diferencia entre sus horas (15 min).
… --cmd 'turnos actualizar "Mañana" 07:00 12:30 2'
# error: El turno «Mañana» quedaría con 2 franjas y tiene 1 evento después de esa posición:
#        #2 «Acto» (pos. 3). Confirma la eliminación para continuar.
… --cmd 'turnos listar'   # sigue con slots=8 y eventos=2: sin confirmación no se toca nada
```

### Hallazgos / desviaciones

- **Conflicto con una fixture de `test_nucleo_datos` (T6).**
  `test_nucleo_datos.cpp:253` crea un turno 13:00–18:00 de 5 franjas y un receso de 15 min a las
  15:00–15:15. Con la fórmula aprobada (N5) la franja es `(300 − 15) / 5 = 57` y la ventana de
  la posición 2 es **14:54–15:09**, así que ese intervalo no cabe (y el
  `actualizarTurno(14:00, 19:00, 6)` posterior, franja 47, dejaría la ventana en 15:34–15:49 y
  también se rechazaría). El caso pasa de verde a rojo **dentro de un binario ya rojo**
  (`test_backend_nucleo_datos`: 7/6 → 6/7); el conjunto de tests rojos no cambia. **No se
  relajó la regla**: los datos sembrados sí encajan (Mañana 07:00–12:30, franja 40, posición 2 =
  08:20–08:30 exacta; Tarde 12:40–18:10, franja 40, posición 2 = 14:00–14:10 exacta), lo que
  confirma la fórmula; la fixture es la que quedó con la rejilla antigua de 60 min. T6 debe
  ajustar las horas de ese caso al convertirlo a `eventos` (o mover el evento a su ventana).
- **Solape entre posiciones.** Con la contención en ventanas disjuntas no es posible un cruce
  entre posiciones: un evento que empieza antes que el de una posición anterior queda fuera de
  su propia ventana y se rechaza ahí. `haySolapeCronologico` cubre el caso (probado con eventos
  fuera de ventana) y el mensaje del servicio nombra el solape cuando el fallo es de la misma
  posición. En el servicio, la cronología invertida se manifiesta como error de «ventana».
- **Guarda extra en `actualizarTurno`.** Además de la contención de los eventos con horas
  (§4.3), se rechaza la ventana nueva si `duracionFranjaEstimada` queda en 0, porque un turno
  cuya ventana es menor que la suma de los eventos deja la rejilla sin franja aunque los
  eventos no tengan horas. Es la misma regla de encaje de §4.2 aplicada al turno; no cambia
  firmas ni alcance.
- La última posición de la ventana llega a `turno.fin` (N5) y absorbe el resto entero del
  reparto (`(ventana − Σ) % numSlots`), de modo que el fin del turno nunca queda fuera de la
  rejilla. La semántica de las demás posiciones no cambia.
- `NucleoDatos` sigue delegando con 4 argumentos (el `bool` tiene valor por defecto): la
  fachada y la cascada son de T6. Consola, seed, huella, puente y frontend sin tocar. El
  ejemplo manual de T3 `evento-actualizar … 08:20 08:35` ya no es válido (con 15 min la franja
  baja a 39 y la ventana de la posición 2 pasa a 08:18–08:33); es consecuencia de la fórmula,
  no un cambio de T4.

## T5 - Catálogo, tipos en aulas/materias y huella (2026-10-24)

RF-5, RF-6, RF-7, RF-8 (parte de huella). Base: `693fa18` (T1-T4 commiteadas).

### Cambios

- `src/backend/include/backend/services/ServicioTiposAula.hpp` y
  `src/backend/src/services/ServicioTiposAula.cpp` (nuevos):
  - `TipoAulaDTO { id, nombre }` y `ServicioTiposAula::claveTipo(nombre)` =
    `trimmed().toCaseFolded()` (pública y pura: la usan los tests).
  - `crearTipoAula` guarda `nombre` recortado y `clave` normalizada; rechaza nombre vacío y
    duplica dos veces: comparación **en memoria** con `claveTipo` sobre todas las filas
    (frena claves escritas sin normalizar) y el índice único de la tabla (`UNIQUE` → mismo
    mensaje).
  - `renombrarTipoAula` valida existencia, excluye la propia fila del chequeo de clave y
    actualiza solo `TiposAula` (nombre + clave). Las aulas/materias lo ven por el `JOIN`: no
    se reescribe ninguna fila dependiente.
  - `eliminarTipoAula` devuelve `Resultado<bool>`: error con la lista de aulas y materias
    dependientes («…está en uso por las aulas «A», «B» y las materias «C».»); nunca confirma.
  - `namespace tipos_aula { catalogoVacio(db), existe(db, id) }`: consultas compartidas que
    usan `ServicioAula`/`ServicioMaterias` para exigir un tipo existente.
- `src/backend/include/backend/services/ServicioAula.hpp` / `src/.../ServicioAula.cpp`:
  `AulaDTO.idTipoAula`/`tipoAula`; `crearAula`/`actualizarAula` con `int idTipoAula = -1`
  **al final** (centinela transitorio, T9 retira el default); `validarTipoAula` rechaza
  `-1` en ejecución (mensaje «No hay tipos de aula registrados…» si el catálogo está vacío,
  «El aula debe tener un tipo de aula.» si hay catálogo) y los ids inexistentes; `SELECT` con
  `LEFT JOIN TiposAula` en `obtenerAula`/`listarAulas`; el `INSERT`/`UPDATE` incluye
  `id_tipo_aula`.
- `src/backend/include/backend/services/ServicioMaterias.hpp` / `.../ServicioMaterias.cpp`:
  mismo patrón para `MateriaDTO.idTipoAula`/`tipoAula`, `crearMateria(nombre, requisitos,
  idTipoAula = -1)` y `actualizarMateria(id, nombre, requisitos, idTipoAula = -1)`, con
  `LEFT JOIN` y validación del tipo.
- `src/backend/src/services/ServicioGeneracion.cpp` (`huellaDatos`): la huella canónica pasa
  a `A:<id>|<nombre>|<capacidad>|<idTipoAula>;`, `M:<id>|<nombre>|<idTipoAula>;` y
  `e:<id>|<nombre>|<pos>|<orden>|<duracion>|<inicio>|<fin>;` por evento (horas ausentes como
  cadena vacía). Se retira el bucle `r:` de recesos: esa vista deriva de los eventos, así que
  no se perdía información y se evitaba contar dos veces.
- `src/backend/CMakeLists.txt`: `ServicioTiposAula.cpp` en la librería `backend`.
- `test/CMakeLists.txt`: `add_qtest(test_servicio_tipos_aula … backend Qt6::Sql)`.
- `test/backend/test_servicio_tipos_aula.cpp` (nuevo, 28 casos; 30 totales con init/cleanup): clave de `claveTipo`
  (recorte y plegado), clave guardada en la tabla («laboratorio», «física»), nombre visible
  conservado, duplicados exacto/mayúsculas/acento/espacios y clave directa sin normalizar,
  listado ordenado, obtención, renombrado (nombre, clave, solo mayúsculas, colisión con otro
  tipo, propagación a aula y materia sin reescribir sus filas), borrado libre, en uso por aula,
  por materia y por ambos, borrado tras retirar dependientes e ids inválidos.
- `test/backend/test_servicio_aula.cpp` y `test_servicio_materias.cpp`: `setUp` crea un tipo
  («Aula de estudio») y se pasa como **último** argumento en todas las llamadas. Casos nuevos:
  crear sin tipo, tipo inexistente, catálogo vacío (base nueva), DTO expone `idTipoAula`/
  `tipoAula` en alta/obtención/listado, actualizar sin tipo (rechazado sin conservar el
  anterior) y actualizar cambiando de tipo.
- `test/backend/test_servicio_profesor.cpp`, `test_servicio_carga_horaria.cpp`,
  `test_composicion_plan.cpp`: crean el tipo en su `init` y lo pasan en cada `crearMateria`.
  No cambia lo que prueban.
- `test/backend/test_servicio_planes_estudio.cpp`: igual, y el `INSERT` directo
  `INSERT INTO Materias (nombre) …` de `eliminarPlan_conMaterias_falla` incluye `id_tipo_aula`.
- `test/backend/test_servicio_generacion.cpp`: `dominio()` fija `idTipoAula` en aula y materia;
  casos nuevos `huellaDatos_cambiaConElTipoDeAula` (aula y materia) y
  `huellaDatos_cambiaConLosEventos` (alta de evento y cambio de `orden`).

### Rojo inicial

Los tests usan la API que T5 debía crear; el rojo es de compilación:

```bash
cmake --build --preset backend --parallel     # EXIT != 0
# 5 × backend/services/ServicioTiposAula.hpp: No existe el fichero o el directorio
# 2 × ‘struct MateriaDTO’ has no member named ‘idTipoAula’
# 2 × ‘struct AulaDTO’ has no member named ‘idTipoAula’
```

### Verde final

```bash
cmake --build --preset backend --parallel       # EXIT=0 (repetido: "ninja: no work to do.")
ctest --test-dir build-backend --output-on-failure \
    -R 'test_servicio_tipos_aula|test_servicio_aula|test_servicio_materias|test_servicio_generacion'
# 100% tests passed out of 4
#  test_servicio_generacion (25) ... Passed
#  test_servicio_aula       (28) ... Passed
#  test_servicio_materias   (29) ... Passed
#  test_servicio_tipos_aula (30) ... Passed   (binario nuevo)
```

### Suite: antes y después

| | Antes (base `693fa18`) | Después (T5) |
|---|---|---|
| binarios | 48 | 49 (nuevo `test_servicio_tipos_aula`) |
| verdes | **37/48 (77 %)** | **44/49 (90 %)** |
| rojos | **11** | **5** |

Rojos que desaparecen: `test_servicio_aula`, `test_servicio_materias`,
`test_servicio_planes_estudio`, `test_servicio_carga_horaria`, `test_composicion_plan` y
`test_servicio_profesor` (los seis por `NOT NULL constraint failed: <tabla>.id_tipo_aula`).

Rojos que quedan, todos dependientes de T6-T8 (mismos totales por binario que en `693fa18`):

| binario | Totales | motivo |
|---|---|---|
| `test_backend_nucleo_datos` | 6/7 | `NucleoDatos` delega aún sin tipo (`-1` rechazado en ejecución), `Recesos` esperada y la fixture de 60 min de `:253` (T6) |
| `test_backend_cascada` | 6/12 | `crearAula`/`crearMateria` por la fachada sin tipo y cascada de turno sobre `Recesos` (T6) |
| `test_backend_gestor_pendientes` | 11/2 | idem fachada (T6) |
| `test_datos_contexto_base_datos` | 8/3 | `nucleo().crearMateria` sin tipo (T6) |
| `test_seed_aplicacion` | 5/9 | seed antiguo y `contarFilasDeDominio` sobre `Recesos` (T8) |

`test_constructor_entrada`, `test_servicio_turnos_eventos`, `test_calculo_eventos` y
`test_backend_version_esquema` siguen verdes.

### Comprobaciones manuales (sqlite3)

```bash
./build-backend/src/consola/gestor-horarios-consola --db /tmp/opencode/t5.db < /dev/null
sqlite3 /tmp/opencode/t5.db "INSERT INTO TiposAula (nombre, clave) VALUES ('Física','física');"
sqlite3 /tmp/opencode/t5.db "INSERT INTO TiposAula (nombre, clave) VALUES ('FÍSICA','física');"
# Error in 2nd command line argument: UNIQUE constraint failed: TiposAula.clave (exit 1)
# filas finales: 1|Física|física  (el índice sigue frenando la escritura directa)
```

### Hallazgos / desviaciones

- **`test_servicio_planes_estudio` se toca aquí, no en T6.** El plan (§1.4/T6) asigna a T6 el
  `INSERT` directo de materia de ese test, pero su único caso rojo era justo ese `NOT NULL`
  y la verificación de T5 exige dejar solo rojos de T6-T8. Se crea el tipo en su `init` y se
  usa su id en el `INSERT`; no cambia lo que el caso prueba (no borrar un plan con materias).
- **`test_datos_contexto_base_datos` queda rojo a propósito.** Sus altas van por
  `NucleoDatos`, cuya firma no recibe tipo hasta T6; hacerlo verde aquí obligaría a tocar la
  fachada, fuera del alcance. Se une a T6 como preveía el plan.
- **El centinela `-1` sigue vigente** en `ServicioAula`/`ServicioMaterias` (y por tanto
  compilan consola, seed, `NucleoDatos` y frontend); en ejecución se rechaza con mensaje
  propio. T9 retira los valores por defecto manteniendo el orden.
- **El `JOIN` es `LEFT`** en aulas y materias: una fila huérfana (si se desactivan las claves
  foráneas) se sigue listando con `idTipoAula = -1` y `tipoAula` vacío en vez de desaparecer.
  Con las FK activas y `id_tipo_aula NOT NULL` no hay diferencia observable.
- **`claveTipo` no se calcula en SQL** (T2 ya lo decidió): el servicio la guarda y compara en
  memoria; el índice único de la tabla queda como red para escrituras directas.
- La huella deja de recorrer `TurnoDTO.recesos`; al derivar de `t.eventos`, el contenido sigue
  cubierto y se evita duplicarlo. Ningún hash almacenado dependía del formato anterior.

## T6 - Fachada, cascada, puente y consumidores (2026-10-24)

RF-6, RF-7, RF-8, RF-12. Base: `83d7e7f` (T1-T5 commiteadas).

### Cambios

- `src/backend/include/backend/services/NucleoDatos.hpp` / `src/.../NucleoDatos.cpp`:
  - Aulas y materias reciben `int idTipoAula = -1` **al final** (centinela transitorio; T9 quita el
    defecto) y lo delegan al servicio, que lo exige en ejecución.
  - CRUD del catálogo: `listarTiposAula`, `obtenerTipoAula`, `crearTipoAula`, `renombrarTipoAula`,
    `eliminarTipoAula`. El borrado en uso **no** se registra como pendiente de reintento: devuelve el
    error con los dependientes (RF-5).
  - Turnos: `obtenerTurno`, `actualizarTurno(..., bool eliminarEventosFueraDeRango = false)`,
    `eventosFueraDeRango`, y el CRUD de eventos por identidad (`agregarEvento`, `actualizarEvento`,
    `eliminarEvento`) con dominio de pendiente `evento`. `agregarReceso`/`eliminarReceso` siguen como
    compatibilidad derivada hasta T9.
  - La fachada no consulta la tabla retirada `Recesos`: todo va por `ServicioTurnosRecesos` sobre
    `Eventos`.
- `src/backend/src/services/cascada.cpp`: la relación del dominio `turno` apunta a `Eventos` con
  `columnaId = id`, etiqueta `nombre` («Evento del turno: <nombre>») y descripción «Evento del turno».
  El nombre lógico `receso` se conserva hasta T9.
- `src/backend/include/backend/services/ConstructorEntradaSolver.hpp` /
  `src/backend/src/services/ConstructorEntradaSolver.cpp`: función pura
  `recesosDesdeEventos(const TurnoDTO&)` y el constructor la usa para emitir la sección `recesos`
  del contrato 1.0: **una entrada por evento** (turno, posición, duración, inicio y fin), sin añadir
  tipos ni eventos al contrato.
- `src/consola/include/consola/ConsolaDatos.hpp` / `src/consola/src/ConsolaDatos.cpp`:
  - Comando `tipos listar|crear|renombrar|eliminar`.
  - `aulas crear "<nombre>" <capacidad> <idTipoAula> [edificio] [piso]` y su `actualizar`;
    `materias crear "<nombre>" <idTipoAula> [requisitos]` y su `actualizar`. Los listados muestran
    `tipo=...`.
  - Los comandos `evento-*` y `turnos eventos` pasan por la fachada (`NucleoDatos`); se retira el uso
    directo de `ServicioTurnosRecesos` y su include. La ayuda sigue ofreciendo `receso-*` (T9/T10) y
    solo se amplía con `tipos` y el tipo de aulas/materias.
- Tests (primero en rojo):
  - `test/backend/test_constructor_entrada.cpp`: `dominioCompleto()` construye `turno.eventos` (no
    `recesos`) y nuevo caso `dosEventosEnLaMismaPosicion_emitenDosRecesos` (2 entradas).
  - `test/backend/test_cascada.cpp`: tipos en el `init`; altas por la fachada con tipo;
    `dependientesDe_turno_listaEventos` comprueba etiqueta con el nombre; nuevo
    `eliminarConCascada_turno_borraSusEventos` (2 eventos → 0).
  - `test/backend/test_gestor_pendientes.cpp` y `test/datos/test_contexto_base_datos.cpp`: crean un
    tipo y lo pasan en las altas por la fachada.
  - `test/backend/test_nucleo_datos.cpp`: `esquemaActual_contieneTablasDeDominio` exige `Eventos` y
    `TiposAula` y la ausencia de `Recesos`; casos nuevos de tipos por la fachada, tipo obligatorio en
    aula/materia (`aulas_sinTipo_seRechazan`, `materias_sinTipo_seRechazan`) y
    `eventos_altaEdicionBajaPorIdentidad` (dos en la misma posición, orden 1/2, sin renumerar).

### Rojo inicial

Punto de partida con la base `83d7e7f`:

```bash
ctest --test-dir build-backend --output-on-failure \
    -R 'test_backend_nucleo_datos|test_backend_cascada|test_backend_gestor_pendientes|...'
# 4 tests failed out of 9: nucleo_datos, cascada, gestor_pendientes, datos_contexto_base_datos
#   nucleo_datos: «Recesos» esperada, fachada sin tipo (NOT NULL id_tipo_aula) y fixture 15:00–15:15
#   cascada: crearMateria/crearAula por la fachada sin tipo y turno → tabla retirada «Recesos»
#   gestor_pendientes: crearAula sin tipo
#   datos_contexto_base_datos: crearMateria sin tipo
```

Con los tests nuevos, el rojo es de compilación porque usan la API que T6 debía crear:

```bash
cmake --build --preset backend --parallel     # EXIT != 0
# ‘class NucleoDatos’ has no member named ‘listarTiposAula’ / ‘agregarEvento’ /
#   ‘actualizarEvento’ / ‘eliminarEvento’
# no matching function for call to ‘NucleoDatos::crearAula(..., int&)’
# no matching function for call to ‘NucleoDatos::crearMateria(..., int&)’
```

### Verde final

```bash
cmake --build --preset backend --parallel      # EXIT=0, 65/65 targets, 0 "error:"
ctest --test-dir build-backend --output-on-failure \
    -R 'test_backend_nucleo_datos|test_backend_cascada|test_backend_gestor_pendientes|test_servicio_profesor|test_composicion_plan|test_servicio_carga_horaria|test_servicio_planes_estudio|test_datos_contexto_base_datos|test_servicio_generacion|test_constructor_entrada'
# 100% tests passed out of 10
#   test_backend_nucleo_datos (17 casos) · test_backend_cascada (14) · test_backend_gestor_pendientes (14)
#   test_constructor_entrada (9) · test_datos_contexto_base_datos (12)
#   test_servicio_generacion · test_servicio_planes_estudio · test_servicio_carga_horaria
#   test_composicion_plan · test_servicio_profesor ... Passed
```

El puente queda probado con `dosEventosEnLaMismaPosicion_emitenDosRecesos`: dos eventos en la
posición 1 producen **dos** entradas en `recesos` (duraciones 10 y 5), no una.

### Fixture de `test_nucleo_datos` (regla de T4, sin excepciones)

- Franja con 13:00–18:00 y 5 franjas y evento de 15 min: `(300 − 15) / 5 = 57`; la ventana de la
  posición 2 es **14:54–15:09**, no 15:00–15:15.
- Primer intento: evento 14:54–15:09 y actualización a 13:00–18:15/6. El servicio **rechazó** la
  actualización («su intervalo 14:54–15:09 no cabe en la ventana 14:40–14:55»), lo que confirma que
  la validación sigue activa y que la ventana termina en `inicio + Σ duraciones de la posición`.
- Fixture final: el turno se actualiza **antes** de crear el evento (14:00–19:00, 6 franjas →
  franja 47, ventana de la posición 2 = 15:34–15:49) y el evento se declara en 15:34–15:49. Pasa por
  la regla nueva; no se relajó la validación ni se añadieron excepciones. El caso cubre el CRUD de
  turnos/eventos por la fachada y la vista derivada `recesos`.

### Suite completa

```bash
ctest --test-dir build-backend                  # 48/49 (98 %)
# 45 - test_seed_aplicacion (Failed)  -> Totals: 5 passed, 9 failed
#   «No se pudo comprobar si la base ya tiene datos.» (contarFilasDeDominio sobre Recesos) y
#   seed antiguo sin tipos/eventos: es el rojo previsto para T7/T8.
# test_seed_perfil sigue verde.
```

### Comprobaciones manuales (consola, base nueva v3)

```bash
C=./build-backend/src/consola/gestor-horarios-consola
$C --db /tmp/opencode/t6-consola.db --cmd 'tipos crear "Aula de estudio"'          # ok
$C … 'tipos crear "Laboratorio"'                                                    # ok
$C … 'tipos listar'                       # 1 | Aula de estudio ; 2 | Laboratorio
$C … 'aulas crear "Aula 101" 30 1 "Edificio A"'                                     # ok
$C … 'aulas listar'                       # 1 | Aula 101 | capacidad=30 | tipo=Aula de estudio
$C … 'materias crear "Física" 2 "Matemática"'                                       # ok
$C … 'materias listar'                    # 1 | Física | tipo=Laboratorio | requisitos=Matemática
$C … 'turnos crear "Mañana" 07:00 12:30 8'
$C … 'turnos evento-agregar "Mañana" "Receso" 2 10 08:20 08:30'                     # ok
$C … 'turnos evento-agregar "Mañana" "Acto cívico" 2 5'                             # ok (orden 2)
$C … 'turnos listar'                      # slots=8 | eventos=2 | recesos=2
$C … 'dependientes turno "Mañana"'
#   - [receso] Evento del turno: Receso (id=1)
#   - [receso] Evento del turno: Acto cívico (id=2)
$C … 'cascada turno "Mañana" --si'        # borra los 2 eventos y el turno; 'turnos listar' -> 0
$C … 'aulas crear "Aula sin tipo" 20'     # Uso: … <idTipoAula> … (el tipo no es opcional en consola)
$C … 'tipos eliminar 1'                   # error: en uso por el aula «Aula 101»
```

### Hallazgos / desviaciones

- **Sintaxis de la consola.** El tipo entra como argumento obligatorio **antes** de los opcionales
  (`aulas crear … <idTipoAula> [edificio] [piso]`, `materias crear … <idTipoAula> [requisitos]`), para
  que no haya que pasar cadenas vacías; la alternativa natural (tipo al final) obligaba a rellenar
  `edificio`/`piso` con `""`. Los `evento-*` ya no usan el servicio directamente sino la fachada.
- **`eliminarTipoAula` no pasa por `conPendienteBool`.** Un borrado en uso es un rechazo de negocio,
  no una escritura fallida reintentable; registrarlo como pendiente haría que el reintento volviera a
  fallar para siempre.
- **Nombre lógico `receso` en la cascada.** Se mantiene hasta T9 (como fijó el plan para el dominio),
  pero la tabla es `Eventos`: `dependientesDe("turno")` lista los eventos con su nombre y
  `eliminarConCascada("turno")` los borra antes del turno.
- La consola `turnos actualizar` no pide confirmación para reducir franjas: usa el valor por defecto
  `false` y muestra el error con los afectados. La confirmación explícita de RF-4 ya está probada en
  el servicio (`test_servicio_turnos_eventos`) y su comando `--si` no forma parte de T6.
- El frontend sigue sin compilar (fuera de la entrega); esta verificación es la del preset `backend`.
