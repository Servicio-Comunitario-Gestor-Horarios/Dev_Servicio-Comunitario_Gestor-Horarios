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
