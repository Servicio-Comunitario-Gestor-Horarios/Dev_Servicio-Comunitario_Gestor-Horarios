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
