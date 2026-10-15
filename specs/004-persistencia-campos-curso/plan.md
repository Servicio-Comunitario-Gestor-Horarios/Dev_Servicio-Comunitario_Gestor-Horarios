# Plan 004 — Base de datos y servicios de curso

Estado: propuesto
Spec de referencia: `specs/004-persistencia-campos-curso/spec.md` (aprobada, sin dudas abiertas).
Constitución: `docs/constitution.md` §1 (simplicidad), §2 (spec↔código), §3 (lógica/UI), §4 (tests), §5 (protección de datos), §6 (idioma), §7 (contrato del solver).
Specs relacionadas: `specs/001-base-datos-local` y `specs/003-cierre-001-base-datos` (implementadas; **esta spec anula su versionado de esquema y su migración por versión**); `specs/005-nucleo-solver` (**define** el modelo del solver, **construye la entrada** desde los datos persistidos y gestiona los presets).

> Este documento describe el CÓMO. No introduce dependencias nuevas: C++17 · Qt6 (Core/Sql) · CMake/Ninja · CTest + Qt Test. OR-Tools **no** se toca aquí.
> Alcance: esquema único, persistencia y servicios de curso, cascadas, huella de datos y adaptación de la consola. La **interfaz Qt** (`src/frontend`) queda fuera (otro equipo); solo se actualiza el **contrato** que consume (`docs/interfaz-frontend.md`).
> **Modo didáctico:** en la implementación, el **agente escribe los tests** (rojo) y el **usuario escribe el código de producción**; el agente revisa. Una tarea cada vez.

---

## 0. Frontera y capas (constitución §3, §5, §7)

- **`src/backend`** (Qt6 Core/Sql, sin OR-Tools): esquema único de la base, `AperturaBaseDatos`, `NucleoDatos`/`ServicioCursos`, la cascada y la huella. Todo testeable sin UI.
- **`src/consola`** (herramienta de desarrollo): ejercita el mismo contrato por línea de comandos; se ajusta al modelo nuevo.
- **`src/frontend`** (Qt, otro equipo): **no se modifica**. Solo cambia el contrato documentado en `docs/interfaz-frontend.md`.
- **`backend_solver`** (OR-Tools): **fuera de esta spec** (spec 005); aquí no se toca OR-Tools. La **construcción de la entrada** y el modelo del solver son de la spec 005.

```
  esquema único (una vez)        ServicioCursos (validar/crear/actualizar/listar)        CursoDTO
BD ───────────────────────► NucleoDatos ───────────────────────────────► { id, anio, seccion, turno, aula }
        (Migracion / VersionEsquema)          (backend, Qt6 Sql)
```

---

## 1. Archivos y responsabilidades

### 1.1 Esquema único y apertura (RF-6, RF-1)

| Archivo | Acción | Responsabilidad | RF |
|---|---|---|---|
| `src/backend/include/backend/database/migracion.hpp` + `src/backend/src/database/migracion.cpp` | **modificado** | `Migracion::aplicarPaso(QSqlDatabase&)` crea **de una sola vez** todo el catálogo del esquema único. `Cursos` pasa a `(id, anio CHECK(anio>=1), seccion CHECK(length(seccion)=1), turno REFERENCES Turnos(nombre), aula REFERENCES Aulas(id), sellos de auditoría, UNIQUE(anio,seccion))`; **se elimina** `Curso_Materia`. Sin pasos versionados ni `versionDestino`. | RF-1, RF-6 |
| `src/backend/include/backend/database/version_esquema.hpp` + `src/backend/src/database/version_esquema.cpp` | **modificado** | `VERSION_ESQUEMA_ACTUAL = 2` (marca única). `EstadoApertura { Crear, Abrir, FalloEsquema }`. `decidirApertura(...)` pura: sin archivo → `Crear`; versión == esperada → `Abrir`; cualquier otra (ausente, anterior o posterior) → `FalloEsquema`. Nuevas: `crearEsquema(db)` (una transacción: `aplicarPaso` + fijar `user_version`) y `esquemaEsperado(db)` (comprueba el catálogo). Se retiran `PasoMigracion` y las dos `aplicarHasta`. | RF-6 |
| `src/backend/include/backend/database/apertura_base_datos.hpp` + `src/backend/src/database/apertura_base_datos.cpp` | **modificado** | `Estado { OkCreada, OkAbierta, FalloEsquema, FalloApertura }`. `Resultado { estado, detalle }` (se retira `rutaRespaldo`: ya no hay respaldo de migración). `Opciones { versionEsperada, crear, verificar }` (se retiran `paso`, `respaldo`, `ahora`, `progreso` y `FuncionRespaldo`). `procesar`: si `!existe` → `crearEsquema`; si existe → versión **y** catálogo (`esquemaEsperado`) deben coincidir, si no → `FalloEsquema`. Sin migración ni respaldo previo. | RF-6; RNF-1 |
| `src/backend/src/database/DatabaseManager.cpp` | **modificado** | `runMigrations`: si no existía → `crearEsquema`; si existe → verificar versión + `esquemaEsperado`; si no coincide → `false` (no crea ni migra). Adapta la ruta de desarrollo/test al esquema único. | RF-6 |
| `src/backend/src/database/respaldo.cpp` | **sin cambios** | `crearRespaldo`/`validarRespaldo`/`restaurarRespaldo`/`descartarBaseYCrearNueva` siguen vigentes (RF-4/RF-6 de 001 y protección de datos). `descartarBaseYCrearNueva` crea la nueva vía `VersionEsquema::crearEsquema`. | RNF-1 |

### 1.2 Servicio de cursos, funciones puras y fachada (RF-1..RF-5)

| Archivo | Acción | Responsabilidad | RF |
|---|---|---|---|
| `src/backend/include/backend/services/ServicioCursos.hpp` | **modificado** (header + funciones puras inline) | `CursoDTO { int id; int anio; QString seccion; QString turno; int aula; }` (`aula` = id del aula, `-1` = ninguna). **Se eliminan** `CursoMateriaDTO` y `QVector<CursoMateriaDTO> materias`. Firmas `crearCurso(anio, seccion, turno, aula)` / `actualizarCurso(id, anio, seccion, turno, aula)`. **Incluye**, inline junto al servicio, las funciones puras `normalizarSeccion`, `validarIdentidadCurso`, `nombreCurso`, `cursoAntes` y el struct `IdentidadCurso` (§2). Sin header `backend/dominio/`. | RF-1, RF-2, RF-3, RF-4 |
| `src/backend/src/services/ServicioCursos.cpp` | **modificado** | `mapearARecord` lee `id/anio/seccion/turno/aula`; **se retiran** `cargarMaterias`, `asignarMateria`, `quitarMateria` y el borrado de `Curso_Materia` en `eliminarCurso`. `crear`/`actualizar` validan la identidad, pre-chequean duplicado año+sección y aula inexistente, y se defienden ante `UNIQUE`/`FOREIGN KEY`. `listar` ordena con `cursoAntes`. | RF-1, RF-2, RF-3, RF-4 |
| `src/backend/include/backend/services/NucleoDatos.hpp` + `src/backend/src/services/NucleoDatos.cpp` | **modificado** | Propaga las firmas nuevas; **se retiran** `asignarMateriaACurso` y `quitarMateriaDeCurso` (y su clave de pendiente `curso_materia`). La clave de la operación pendiente de **alta** de curso pasa de `nombre` a la identidad normalizada `"<anio>-<seccion>"`. | RF-2, RF-4, RF-5 |

### 1.3 Cascadas y huella (RF-5)

| Archivo | Acción | Responsabilidad | RF |
|---|---|---|---|
| `src/backend/include/backend/services/cascada.hpp` + `src/backend/src/services/cascada.cpp` | **modificado** | Se **retira** la relación `curso_materia` de `materia` y de `curso`, y la dependencia `plan → cursos`. Se **conservan** `aula → cursos` y `turno → cursos`, pero dejan de **borrar** el curso: pasan a **anular la referencia** (`aula = NULL` / `turno = NULL`) tras la confirmación. Se añade a `RelacionCascada` el modo de acción (`eliminar` vs `anularReferencia`). `dominioValido("curso")` sigue; `dependenciasDe("curso")` queda vacío. La etiqueta del curso dependiente es `anio || ' ' || seccion`. | RF-1, RF-5 |
| `src/backend/src/services/NucleoDatos.cpp` (`borrarRecursivo`) | **modificado** | Interpreta el modo de acción: para las relaciones de **anulación**, ejecuta `UPDATE <tabla> SET <columna> = NULL WHERE <columna> = :id` en vez de borrar las filas; para las de **eliminación**, borra como hoy. Todo dentro de la misma transacción atómica. | RF-1, RF-5 |
| `src/backend/src/services/ServicioGeneracion.cpp` | **modificado** | `huellaDatos` deja de depender de `nombre`, `numEstudiantes`, `codigoPlan` y de la lista de materias del curso: canoniza `id`, `anio`, `seccion`, `turno` y `aula`. | RF-5 |

### 1.4 Consola y contrato de consumo

| Archivo | Acción | Responsabilidad | RF |
|---|---|---|---|
| `src/consola/src/ConsolaDatos.cpp` | **modificado** | `cursos listar` muestra `id`, nombre **derivado**, `anio`, `seccion`, `turno`, `aula`; `cursos crear <anio> <seccion> [turno] [aula]` y `cursos actualizar <id> <anio> <seccion> [turno] [aula]`; se **retiran** las acciones `asignar`/`quitar` de materias de curso. Textos de ayuda en español (RNF-2). | RF-2, RF-4 |
| `docs/interfaz-frontend.md` | **modificado** | Actualiza solo la parte observable: `CursoDTO = { id, anio, seccion, turno, aula }` (sin `nombre`/`numEstudiantes`/`codigoPlan`/`aulaFija`/`materias`); firmas de `NucleoDatos::crearCurso/actualizarCurso`; sin operaciones curso–materia; clave de pendiente de alta de curso `"<anio>-<seccion>"`; estados de `AperturaBaseDatos` reducidos; cascada aula→cursos y turno→cursos con **anulación de referencia**. | RF-2, RF-4, RF-6 |

> `src/frontend/**` **no se toca** (spec, «Fuera de alcance»). Los `CursoOutput`/`HorarioSalida` (salida del solver) son de la spec 005.

---

## 2. Funciones puras y deterministas

Declaradas **inline junto a `ServicioCursos`** en `src/backend/include/backend/services/ServicioCursos.hpp`; sin estado, sin E/S, sin base de datos y sin Qt Widgets. Deterministas.

| Función | Firma | Comportamiento | RF | ¿Reloj? |
|---|---|---|---|---|
| `normalizarSeccion` | `QString normalizarSeccion(const QString& bruta)` | `bruta.trimmed().toUpper()` (mayúsculas Unicode). | RF-3 | No |
| `validarIdentidadCurso` | `Resultado<IdentidadCurso> validarIdentidadCurso(int anio, const QString& seccionBruta)` | Rechaza `anio < 1`; normaliza la sección y exige **exactamente un carácter** (rechaza si la normalización produce >1, p. ej. `ß`→`SS`); devuelve `IdentidadCurso{anio, seccionNormalizada}` o error en español con **campo y valor**. | RF-1, RF-3 | No |
| `nombreCurso` | `QString nombreCurso(int anio, const QString& seccion)` | Ordinal + sección: `1→"1ro"`, `2→"2do"`, `3→"3ro"`, resto `→"<n>to"`; `"1ro A"`, `"4to B"`. **Presentación** (consola/frontend); el solver **no** lo usa. | RF-2, RF-4 | No |
| `cursoAntes` | `bool cursoAntes(int anioA, const QString& secA, int anioB, const QString& secB)` | `anio` ascendente; a igual año, sección con el colador español del proyecto (`nombreAntes`, `backend/services/ordenacion.hpp`: insensible a mayúsculas y con acentos). | RF-2 | No |
| `decidirApertura` | `VersionEsquema::EstadoApertura decidirApertura(bool existe, int versionArchivo, int versionEsperada)` | `Crear` / `Abrir` / `FalloEsquema` (pura; ver §3.2). | RF-6 | No |

`struct IdentidadCurso { int anio = 0; QString seccion; };`

### Nota sobre «hoy» como parámetro

Ninguna función pura de esta spec consulta el reloj: derivar un nombre, normalizar una sección, ordenar cursos y decidir la apertura son deterministas por construcción. Los **sellos de auditoría** siguen fijándose en SQL (`DEFAULT`/`CURRENT_TIMESTAMP`), el «mismo comportamiento de hoy» que exige RF-6; por eso **no** se inyecta un parámetro «hoy» (decisión D-10).

---

## 3. Pseudocódigo

### 3.1 Validación de identidad (pura) — RF-1, RF-3

```
funcion validarIdentidadCurso(anio, seccionBruta):
    si anio < 1:
        error("El año del curso debe ser un entero mayor o igual que 1 (valor recibido: <anio>).")
    seccion = normalizarSeccion(seccionBruta)                 # trim + toUpper
    si longitud(seccion) != 1:                                # vacío, espacios o ≥2 (incluye ß→SS)
        error("La sección del curso debe ser exactamente un carácter (valor recibido: «<seccionBruta>»).")
    devolver exito(IdentidadCurso{anio, seccion})
```

### 3.2 Esquema único y esquema esperado — RF-6

```
funcion decidirApertura(existe, versionArchivo, versionEsperada):
    si !existe:                            devolver Crear
    si versionArchivo == versionEsperada:  devolver Abrir
    devolver FalloEsquema                  # ausente (<=0), anterior o posterior: todas fallo

funcion crearEsquema(db):                  # una sola transacción
    db.transaction()
    si !Migracion::aplicarPaso(db): rollback(); devolver false
    si !fijarVersionEsquema(db, VERSION_ESQUEMA_ACTUAL): rollback(); devolver false
    devolver db.commit()

funcion esquemaEsperado(db):
    para cada tabla en CATALOGO_ESPERADO:  si !tablaExiste(db, tabla): devolver false   # p. ej. Aulas, Profesores, …, Cursos
    para cada (tabla, columna) en COLUMNAS_ESPERADAS:
        si !columnaExiste(db, tabla, columna): devolver false                            # p. ej. Cursos(anio, seccion, aula)
    si tablaExiste(db, "Curso_Materia"):   devolver false                                # sobra una tabla retirada
    devolver true
```

`AperturaBaseDatos::procesar`:

```
si !existe:  ok = crearEsquema(db) → OkCreada | FalloApertura
si existe:   si leerVersionEsquema(db) == versionEsperada y esquemaEsperado(db) → OkAbierta
             si no → FalloEsquema   (no migra, no borra datos, RF-6/RNF-1)
```

### 3.3 Esquema (RF-1, RF-6)

```
CREATE TABLE IF NOT EXISTS Cursos (
    id                 INTEGER PRIMARY KEY AUTOINCREMENT,     -- clave subrogada (RF-1)
    anio               INTEGER NOT NULL CHECK(anio >= 1),
    seccion            TEXT    NOT NULL CHECK(length(seccion) = 1),
    turno              TEXT    REFERENCES Turnos(nombre),
    aula               INTEGER REFERENCES Aulas(id),          -- referencia (opcional); sin índice persistido
    fecha_creacion     TEXT    NOT NULL DEFAULT CURRENT_TIMESTAMP,
    fecha_modificacion TEXT    NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(anio, seccion)                                     -- unicidad en todo el sistema (RF-1)
)
-- NO existe `Curso_Materia` (RF-5): las materias se derivan del plan (spec 005).
-- El «modo de aula fija» es global del preset (spec 005): no hay columna por curso.
```

### 3.4 Crear / actualizar curso (RF-1, RF-2, RF-3; RNF-2)

```
funcion crearCurso(anio, seccionBruta, turno="", aula=-1):
    v = validarIdentidadCurso(anio, seccionBruta); si !v.ok: devolver error(v.mensaje)
    si aula > 0 y !existeFila(Aulas, id == aula):
        error("El aula indicada no existe; elija un aula registrada.")      # RF-1, RNF-2
    si existeFila(Cursos, anio == v.anio y seccion == v.seccion):
        error("Ya existe un curso con el año <v.anio> y la sección «<v.seccion>». Elija otro año o sección.")
    INSERTAR en Cursos (anio, seccion, turno, aula) valores (v.anio, v.seccion, turno.trimmed(), aula>0 ? aula : NULL)
    si falla con UNIQUE → error(duplicado)   # defensa en profundidad (RF-1)
    si falla con FOREIGN KEY → error("El aula indicada no existe…")
    devolver obtenerCurso(idInsertado)

funcion actualizarCurso(id, anio, seccionBruta, turno, aula):
    (validación + pre-chequeos análogos; el aula inexistente también se rechaza)
    ACTUALIZAR Cursos SET anio=:anio, seccion=:seccion, turno=:turno, aula=:aula,
        fecha_modificacion = CURRENT_TIMESTAMP WHERE id=:id
    si filasAfectadas == 0 → error("No se encontró un curso con ese ID.")
    devolver obtenerCurso(id)
```

### 3.5 Consultar y listar (RF-2, RF-4)

```
funcion mapearARecord(record):
    dto.id = record["id"]; dto.anio = record["anio"]; dto.seccion = record["seccion"]
    dto.turno = record["turno"] o ""
    dto.aula  = record["aula"] nulo ? -1 : record["aula"]
    (sin nombre almacenado, sin materias, sin plan/estudiantes/aulaFija)

funcion listarCursos():
    filas = SELECT id, anio, seccion, turno, aula FROM Cursos
    ordenar estable con cursoAntes(anio, seccion)          # año asc, luego sección (RF-2)
```

### 3.6 Cascada (RF-1, RF-5)

```
# dependenciasDe(dominio):
plan    → [plan_materia: eliminar]                       # la dependencia plan→cursos DESAPARECE
materia → [plan_materia: eliminar, profesor_materia: eliminar]   # curso_materia RETIRADA
aula    → [curso: anularReferencia]                      # deja el curso sin aula, no lo borra (RF-1)
turno   → [receso: eliminar, curso: anularReferencia]    # deja el curso sin turno (RF-5)
curso   → []                                             # ya no arrastra nada

# borrarRecursivo, dentro de una transacción:
para cada relacion:
    si relacion.modo == eliminar:          DELETE FROM <tabla> WHERE <columna> = :id
    si relacion.modo == anularReferencia:  UPDATE <tabla> SET <columna> = NULL WHERE <columna> = :id
# `dependientesDe` sigue devolviendo los cursos afectados con etiqueta «anio seccion» para el diálogo de confirmación.
```

### 3.7 Huella de datos (RF-5)

```
funcion huellaDatos(datos):
    canon = docentes(id,nombre,disponibilidad,materias) + aulas(id,nombre,capacidad)
          + materias(id,nombre) + planes(codigo,nombre) + turnos(nombre,inicio,fin,slots,recesos)
          + cursos(id, anio, seccion, turno, aula)         # SIN nombre, SIN num_estudiantes/codigoPlan, SIN materias
    devolver sha256(canon)
```

---

## 4. Interfaz y contrato (sin cambios en `src/frontend`)

- **Contrato para el frontend (`docs/interfaz-frontend.md`).** `CursoDTO = { id, anio, seccion, turno, aula }`; firmas `crearCurso(anio, seccion, turno, aula)`/`actualizarCurso(id, anio, seccion, turno, aula)`; sin `asignarMateriaACurso`/`quitarMateriaDeCurso`; `AperturaBaseDatos::Estado { OkCreada, OkAbierta, FalloEsquema, FalloApertura }` sin `rutaRespaldo`; cascada `aula`/`turno` → cursos con **anulación de referencia**; clave de pendiente de alta de curso `"<anio>-<seccion>"`. El nombre del curso es **presentación**: el frontend lo deriva con `nombreCurso(anio, seccion)`.
- **Consola de datos (`gestor-horarios-consola`).** Interfaz verificable de la base:
  - `cursos listar` → `id | <nombre derivado> | anio=<n> | seccion=<X> | turno=… | aula=…`.
  - `cursos crear <anio> <seccion> [turno] [aula]`.
  - `cursos actualizar <id> <anio> <seccion> [turno] [aula]`; sin acciones de materias de curso.
- **Diálogo de dependientes (consumido por el frontend).** La etiqueta de un curso dependiente es «año sección» (`anio || ' ' || seccion`); los dependientes de `aula`/`turno` se muestran para confirmar que quedarán **sin** esa referencia.

---

## 5. Decisiones técnicas (con alternativa descartada)

| # | Decisión | Alternativa descartada | Motivo |
|---|---|---|---|
| D-1 | Identidad `(anio, seccion)` con `id` subrogado; `UNIQUE(anio, seccion)` adicional. | Usar `(anio, seccion)` como clave principal y eliminar `id`. | RF-1 pide conservar el `id` subrogado; el contrato y las operaciones se apoyan en él. |
| D-2 | Sin columna `nombre`; el nombre se **deriva** en memoria (`nombreCurso`). | Columna `nombre` almacenada o generada por SQL. | RF-1/RF-4: el nombre es presentación; una columna duplicaría la lógica ordinal y se podría desincronizar. |
| D-3 | `aula` como única columna de referencia (`id` del aula, `NULL` = ninguna); **sin indicador por curso**. | Conservar `aula_fija` booleano por curso. | RF-1: el modo de aula fija es **global** (spec 005); el indicador por curso ya no existe. |
| D-4 | Esquema único: una sola `Migracion::aplicarPaso` sin parámetro; `VERSION_ESQUEMA_ACTUAL` se mantiene en `2`; sin `aplicarHasta` ni pasos versionados. | Conservar `aplicarHasta(paso)` y/o subir a `v3` con un paso. | RF-6: no hay versiones previas ni migración; un paso sería destructivo o inútil. La marca `2` sigue siendo la única esperada y el catálogo distingue las bases de desarrollo antiguas (que fallan, RF-6). |
| D-5 | `decidirApertura` se simplifica a `{ Crear, Abrir, FalloEsquema }` y se añade `esquemaEsperado(db)`. | Reusar los estados viejos (`Migrar`, `FalloAusente`, `FalloPosterior`). | RF-6 define **un** resultado de fallo para cualquier esquema no esperado; mantener estados de migración sería engañoso (parecería que el arranque por versión sigue vigente). |
| D-6 | `AperturaBaseDatos::Estado` se reduce y se retira `Resultado::rutaRespaldo`/`Opciones::paso/respaldo/ahora/progreso`. | Dejar campos y estados «muertos» por compatibilidad. | Sin migración no hay respaldo previo ni progreso; dejar campos vacíos invita a usarlos por error. El contrato del frontend se actualiza en la misma tarea. |
| D-7 | `UNIQUE(anio, seccion)` autoritativo en la tabla + pre-chequeo del servicio para el mensaje. | Confiar solo en la validación de la aplicación. | RF-1: no debe existir el duplicado «aunque la operación soslaye la validación». |
| D-8 | Normalizar la sección con `trimmed().toUpper()` (Unicode) y exigir **exactamente** un carácter tras normalizar. | Restringir a ASCII o validar con una expresión regular limitada. | RF-3 acepta no ASCII (`Ñ`, `Á`) y exige rechazar la expansión (`ß`→`SS`). |
| D-9 | Orden por `anio` numérico y, a igual año, por sección con `nombreAntes` (`QCollator`). | Ordenar por el nombre derivado como texto. | RF-2: en texto «10» quedaría antes que «2»; la sección debe seguir la colación del proyecto. |
| D-10 | Sellos de auditoría gestionados por SQL; sin parámetro «hoy». | Inyectar un reloj en los servicios. | RF-6 exige el «mismo comportamiento de hoy»; inyectar tiempo cambiaría lo observable sin pedirlo ningún RF. |
| D-11 | Cascada `aula`/`turno` → cursos como **anulación de referencia** (`SET NULL`), no borrado; se añade el modo a `RelacionCascada`. | Seguir borrando el curso dependiente (comportamiento actual). | RF-1/RF-5: eliminar un aula/turno deja el curso **sin** esa referencia; el curso no desaparece. |
| D-12 | La etiqueta de un curso dependiente usa la expresión SQL `anio || ' ' || seccion`. | Ampliar `RelacionCascada` para componer la etiqueta en C++. | Cambio mínimo; la derivación ordinal sigue en C++ como fuente única (D-2). |
| D-13 | `huellaDatos` canoniza solo los campos persistidos del curso (identidad, turno, aula). | Mantener `nombre`/`numEstudiantes`/`codigoPlan`/materias en la huella. | RF-5: la huella deja de incluir las materias del curso y no puede depender de datos que ya no se persisten. |

---

## 6. Estrategia de tests (tests primero, constitución §4)

**Comando del proyecto** (AGENTS.md / constitución §4), **en el contenedor** (OR-Tools solo vive en Docker):
```
docker exec gestor-dev bash -lc "cd /workspace && cmake --preset full && cmake --build build && ctest --preset full --output-on-failure"
```

En cada tarea: test en **rojo**, luego el código, y la tarea se marca al pasar en **verde**. **Modo didáctico:** el agente escribe los tests; el usuario escribe el código de producción. Registro en `test/CMakeLists.txt` con `add_qtest(<nombre> backend/test/<archivo>.cpp backend Qt6::Sql)` (o `add_gtest`).

| Test | Estado | Cobertura | RF |
|---|---|---|---|
| `test/backend/test_campos_curso.cpp` **(ya creado, rojo)** | Qt Test, `backend` + `Qt6::Sql`; incluye `backend/services/ServicioCursos.hpp` | `nombreCurso` (1→`1ro`…10→`10to`, `"4to B"`, `Ñ`); `normalizarSeccion` (`" a "`→`A`, `ñ`→`Ñ`); `validarIdentidadCurso` (año 0/-1, sección vacía/solo espacios/2 caracteres, `Ñ`/`Á`, mensaje con campo y valor); `cursoAntes` (año numérico, misma-identidad, colación española). | RF-1, RF-2, RF-3, RF-4 |
| `test/backend/test_esquema_cursos.cpp` **(nuevo)** | Qt Test, `backend` + `Qt6::Sql` | `Cursos` **no** tiene `nombre`/`num_estudiantes`/`codigo_plan`/`aula_fija`; tiene `anio`, `seccion`, `aula`, sellos; **no existe** `Curso_Materia`; `UNIQUE(anio,seccion)` rechaza un `INSERT` directo duplicado; `CHECK` rechaza año 0 y sección de 2 caracteres; FK rechaza `aula` inexistente. | RF-1, RF-5, RF-6 |
| `test/backend/test_version_esquema.cpp` **(reescrito)** | Qt Test, `backend` + `Qt6::Sql` | `decidirApertura` (`Crear`/`Abrir`/`FalloEsquema` para versión 0, anterior, posterior); `crearEsquema` crea el catálogo y fija la marca en **una** transacción (idempotente); `esquemaEsperado` acepta el esquema correcto y rechaza uno con `Curso_Materia` o sin `aula`. Se retiran los casos de `aplicarHasta`/pasos. | RF-6 |
| `test/backend/test_apertura_base_datos.cpp` **(reescrito)** | Qt Test, `backend` + `Qt6::Sql` | Base inexistente → `OkCreada` con catálogo; existente con versión y catálogo correctos → `OkAbierta`; versión ausente/anterior/posterior o catálogo no esperado → `FalloEsquema` sin migrar ni borrar; ruta vacía/imposible → `FalloApertura`; mensajes sin texto crudo de Qt. Se retiran los casos de migración/respaldo/progreso. | RF-6; RNF-1 |
| `test/backend/test_respaldo.cpp` **(ajustado)** | Qt Test, `backend` + `Qt6::Sql` | Sustituye `aplicarHasta(db, VERSION_ESQUEMA_ACTUAL)` por `crearEsquema(db)`; el resto (crear/validar/restaurar/descartar) sigue, incluida `validarRespaldo` que rechaza versión posterior. | RF-6; RNF-1 |
| `test/backend/test_contrato_interfaz.cpp` **(ajustado)** | Qt Test, `backend` + `Qt6::Sql` | Fija los estados nuevos de `AperturaBaseDatos` (`OkCreada`/`OkAbierta` con `ok()`, `FalloEsquema`/`FalloApertura` sin `ok()`); sin `OkMigrada`/`FalloRespaldo`/`FalloMigracion`. | RF-6 |
| `test/backend/test_nucleo_datos.cpp` **(modificado)** | Qt Test, `backend` + `Qt::Sql` | Esquema sin `Curso_Materia`; alta/modificación/baja con `anio`/`seccion`; `listarCursos` ordena por año y luego sección (con acentos/mayúsculas); duplicado año+sección rechazado al crear **y** al actualizar; actualizar sin cambiar identidad se acepta; año/sección inválidos rechazados con mensaje; **aula inexistente** rechazada; sin operaciones de materias. | RF-1, RF-2, RF-3, RF-4, RF-5 |
| `test/backend/test_cascada.cpp` **(modificado)** | Qt Test, `backend` + `Qt::Sql` | `dependenciasDe`: `plan` ya no arrastra cursos; `materia` y `curso` ya no tienen `curso_materia`; `aula`/`turno` listan cursos como **anulación**. `eliminarConCascada("aula")` deja los cursos **sin aula** (no los borra); `eliminarConCascada("turno")` los deja sin turno; `plan`/`materia` conservan sus cascadas; atomicidad intacta. | RF-1, RF-5 |
| `test/backend/test_servicio_generacion.cpp` **(modificado)** | Qt Test, `backend` | `huellaDatos` cambia de forma predecible al cambiar año/sección/turno/aula y es estable ante cambios no persistidos (p. ej., materias del curso); no usa `nombre`. | RF-5 |

---

## 7. Cobertura RF → partes del plan

| RF | Partes que lo cubren |
|---|---|
| **RF-1** (representación, unicidad, aula, baja de aula) | §1.1 esquema (`UNIQUE`, `CHECK`, FK `aula`); §1.2/§3.4 `validarIdentidadCurso` + pre-chequeos + defensa `UNIQUE`/FK; §1.3/§3.6 cascada `aula`→cursos (anulación); tests `test_esquema_cursos`, `test_nucleo_datos`, `test_cascada`, `test_campos_curso`. |
| **RF-2** (servicios, orden) | §1.2/§3.5 `ServicioCursos`/`NucleoDatos`, `cursoAntes`/`nombreCurso`; §1.4 consola; tests `test_nucleo_datos`, `test_campos_curso`. |
| **RF-3** (validación y normalización) | §2 `normalizarSeccion`/`validarIdentidadCurso`; §3.1/§3.4; tests `test_campos_curso`, `test_nucleo_datos`. |
| **RF-4** (exposición en el dominio) | §1.2 `CursoDTO { id, anio, seccion, turno, aula }` + `mapearARecord`; §1.4 consola y contrato; §3.5; tests `test_nucleo_datos`, `test_campos_curso`. |
| **RF-5** (retirada curso–materia, cascadas, huella) | §1.3 cascada (retirada `curso_materia`, `plan→cursos`); `borrarRecursivo` (anulación); §3.6; §3.7 `huellaDatos`; §1.3 `NucleoDatos` sin operaciones de materias; tests `test_cascada`, `test_servicio_generacion`, `test_nucleo_datos`. |
| **RF-6** (esquema único y colapso) | §1.1 `Migracion`/`VersionEsquema`/`AperturaBaseDatos`/`DatabaseManager`; §3.2/§3.3; §1.4 contrato de apertura; tests `test_esquema_cursos`, `test_version_esquema`, `test_apertura_base_datos`, `test_respaldo`, `test_contrato_interfaz`. |
| **RNF-1** (sin operación destructiva) | §1.1 sin migración ni `DROP`/`ALTER`; `FalloEsquema` no borra datos; §1.4 contrato. |
| **RNF-2** (mensajes en español) | §2/§3.4 mensajes con campo, valor y acción; §1.4 consola; tests de mensajes. |
| **RNF-3** (trazabilidad) | §6 y §7: cada RF con comprobación verificable en `tasks.md`. |

---

## 8. Secuencia con la spec 005

- **T2–T6 son un único refactor acoplado** del modelo `Cursos` (esquema → servicios → DTO → cascada → huella): al reescribir la tabla, los tests que tocan `Cursos` quedan rojos hasta que T3–T6 adaptan servicios, cascada y huella. La suite de dominio **vuelve a verde al cerrar T6**; a partir de T6 cada tarea es autónoma.
- La **construcción de la entrada del solver** y el **modelo del solver** pertenecen a la **spec 005**; esta spec (004) solo **persiste y expone** los datos que 005 consume. No hay dependencia de 004 hacia la forma del contrato de 005.

---

## 9. Fuera de alcance de este plan

- El **modelo del solver**, las invariantes, la función objetivo, la **construcción de la entrada** desde los datos persistidos más el preset, la serialización del contrato de configuración y la validación V1–V15 (**spec 005**).
- La **gestión de presets** como archivos y la **salida** del solver (`HorarioSalida`/`CursoOutput`/exportación) — spec 005.
- La **interfaz Qt** (`src/frontend`) y la traducción de índice de aula a nombre (otro equipo).
- La **persistencia** del plan, del número de estudiantes, de las materias del curso y de los años del docente: son configuración/preset (spec 005).
- La **migración entre versiones de esquema** y la compatibilidad con bases anteriores: no existen (esquema único).
- **Turnos automáticos** (spec 006) y **generación parcial** (spec 007).
