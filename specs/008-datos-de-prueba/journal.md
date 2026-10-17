# Journal 008 — Datos de prueba del centro (Plan 31059)

Registro de la implementación de las tareas del plan 008. Una entrada por tarea ejecutada.

---

## T1 — Módulo `seed` y plan General

**Fecha:** 2026-10-17

### Qué se hizo

- Se creó el módulo `src/seed` (biblioteca estática, sin OR-Tools): `CMakeLists.txt`,
  `include/seed/perfil.hpp` y `src/perfil.cpp`.
- Se añadió `option(BUILD_SEED ...)` en el `CMakeLists.txt` raíz y
  `add_subdirectory(seed)` en `src/CMakeLists.txt`, dentro de `if(BUILD_BACKEND)` y bajo
  `if(BUILD_SEED)`, tras `datos`. La biblioteca `seed` enlaza `datos` + `Qt6::Core` + `Qt6::Sql`.
- Se definieron los tipos del perfil (`MateriaAnio`, `CargaPlan`, `PlanDef`, `AulaDef`,
  `TurnoDef`, `CursoDef`, `Perfil`) y las funciones puras `bloques()` (identidad),
  `horasDeAnio()`, `bloquesDeAnio()` y `seccionesDeAnio()`.
- Se implementó el **catálogo de 51 materias-año** del 31059 (las 14 áreas en su orden oficial,
  nombre `<Área> <año>º`) y el **plan General** con la tabla adaptada a **40/40/40/40/40 horas
  académicas** (9/9/10/11/12 materias por año). `perfilPorDefecto()` construye una vez el perfil
  (`static const`); turnos/aulas/cursos quedan vacíos en T1 (se rellenan en T3).
- Se creó `test/backend/test_seed_perfil.cpp` (Qt Test) y se registró en `test/CMakeLists.txt`
  con `add_qtest(test_seed_perfil backend/test_seed_perfil.cpp seed)` dentro de `if(TARGET seed)`.

### Archivos tocados

- `CMakeLists.txt` (raíz) — `option(BUILD_SEED "Build the test-data seeder (requires BUILD_BACKEND)" ON)`.
- `src/CMakeLists.txt` — `add_subdirectory(seed)` bajo `if(BUILD_BACKEND)` y `if(BUILD_SEED)`.
- `src/seed/CMakeLists.txt` — biblioteca `seed`.
- `src/seed/include/seed/perfil.hpp` — tipos y funciones puras de T1.
- `src/seed/src/perfil.cpp` — catálogo de 51 materias-año y plan General.
- `test/backend/test_seed_perfil.cpp` — casos del plan General y `bloques()`.
- `test/CMakeLists.txt` — bloque `if(TARGET seed)` con `add_qtest(test_seed_perfil ...)`.

### Desviación del plan (necesaria)

El plan §3 llama al campo `int slots;` en `TurnoDef`. **`slots` es una macro vacía de Qt**
(`qobjectdefs.h`), así que `int slots;` no compila («declaration does not declare anything»).
Se renombró a **`int numSlots;`** (semántica idéntica: franjas del turno, 8). El resto de la
estructura del plan se mantiene sin cambios. Se deja constancia para T3 (turnos) y T5a (inserción).

### Verificación (rojo → verde)

1. **Rojo** (tests escritos y registrados antes del código; `perfil.cpp` en stub):
   ```
   docker exec gestor-dev bash -lc "cd /workspace && cmake --preset full \
     && cmake --build build --target test_seed_perfil \
     && ctest --test-dir build -R test_seed_perfil --output-on-failure"
   ```
   Resultado: `2 passed, 6 failed` — `bloquesEsLaIdentidad`, `catalogo31059Tiene51Materias`,
   `planGeneralReproduceTablaAdaptada`, `horasYBloquesDeAnioCoherentes`,
   `horasDeAnioSinAnioDevuelveMenosUno` y `seccionesDeAnioValores` en rojo (el stub devolvía
   0/-1/vacío).

2. **Verde** (tras rellenar `perfil.cpp`):
   ```
   1/1 Test #41: test_seed_perfil ..... Passed
   100% tests passed, 0 tests failed out of 1
   ```
   El plan General suma **40/40/40/40/40 horas académicas**, **9/9/10/11/12 materias** por año,
   **51 materias-año** y `bloques(h) == h` para varios `h`.

3. **Suite completa** (para asegurar que no se rompe nada): `cmake --build build &&
   ctest --preset full --output-on-failure` → **45/45 verde** (44 previos + `test_seed_perfil`).

4. **dev-frontend** (sin OR-Tools y sin el módulo seed):
   `docker exec gestor-dev bash -lc "cd /workspace && cmake --preset dev-frontend"` configura
   correctamente con `BUILD_BACKEND=OFF - backend and OR-Tools disabled` (el seed queda fuera al
   vivir dentro de `if(BUILD_BACKEND)`). Nota: hubo que limpiar `build-fe/CMakeCache.txt` +
   `build-fe/CMakeFiles` (cache generado en el host con otra ruta fuente); `build-fe` está en
   `.gitignore` y es descartable.

### Hallazgos

- `bloquesDeAnio(plan, anio)` se define como `bloques(horasDeAnio(plan, anio))`; como `bloques`
  es la identidad, `bloques(-1) = -1` conserva el caso «año sin materias».
- El orden del catálogo (año 1..5 y, dentro del año, orden de áreas del 31059) deja
  `materias[22] == "Física 3º"`, coherente con el plan §4.2 (índice estable que T2/T4 usarán).

---

## T2 — Planes derivados y validación del perfil

**Fecha:** 2026-10-17

### Qué se hizo

- **Catálogo a 57 materias**: se añadieron las 6 materias nuevas de los derivados al final de su
  año, en el orden fijo del plan §4.2 (determinismo, RF-11):
  - 4.º (índices 39/40/41): `Matemática Aplicada`, `Química Aplicada`, `Geografía Económica`.
  - 5.º (índices 54/55/56): `Física Aplicada`, `Ciencias de la Tierra Aplicadas`,
    `Historia Contemporánea`.
- **Tres planes derivados** EXA/NAT/SOC (código, nombre y área de desarrollo según §4.2): copian el
  General y aplican los recortes de 1 h en tres áreas base del mismo año más su materia nueva de
  3 h, de modo que **cada año suma exactamente 40 horas académicas** (9/9/10/12/13 materias,
  53 cargas por derivado). Los tres difieren también en qué áreas recortan.
- **`esMateriaNueva()`** (pública, plan §3) y, en el anónimo, `materiaNueva()`: identifica las 6
  materias nuevas.
- **`validarPerfil()`** (pública, plan §5, alcance T2): catálogo sin duplicados ni huérfanas y año
  1–5; progresión del 31059 (Arte/CN 1–2; Física/Química/Biología 3–5; CT 5; Soberanía 4–5) en
  todos los planes; por plan, sin materias repetidas ni años vacíos ni horas negativas y
  `Σ horas ≤ 40` por año; derivados con ≥2 materias nuevas de su eje en 4.º–5.º usadas solo por su
  plan y ≥2 áreas base con horas distintas de GEN; y **aula repetida entre cursos** (RF-5). Los
  mensajes son en español e identifican plan/año/materia.

### Archivos tocados

- `src/seed/include/seed/perfil.hpp` — declara `esMateriaNueva()` y `validarPerfil()`.
- `src/seed/src/perfil.cpp` — 6 materias nuevas, 3 planes derivados, `validarPerfil()`.
- `test/backend/test_seed_perfil.cpp` — casos nuevos y `catalogo31059Tiene51Materias` renombrado a
  `catalogoTiene57MateriasY4Planes` (el catálogo pasa de 51 a 57).

### Verificación (rojo → verde)

1. **Rojo** (tests escritos antes del código): los casos nuevos y el catálogo a 57 se
   redactaron primero; contra el código de T1 no podían compilar/enlazar (faltaban
   `esMateriaNueva` y `validarPerfil`, y el catálogo tenía 51 materias), y `validarPerfil` /
   los derivados no existían.
2. **Verde** (tras rellenar `perfil.cpp` y el header):
   ```
   docker exec gestor-dev bash -lc "cd /workspace && cmake --build build --target test_seed_perfil \
     && ctest --test-dir build -R test_seed_perfil --output-on-failure"
   ```
   `test_seed_perfil ..... Passed` (14 funciones: initTestCase, 12 casos y cleanupTestCase).
3. **Suite completa**: `cmake --build build && ctest --preset full --output-on-failure` →
   **45/45 verde** (sin rojos; `test_seed_perfil` sigue siendo el test #41).

### Hallazgos / decisiones

- Las materias nuevas se insertan **al final de su año** en el catálogo (no al final del vector),
  para conservar los índices estables 39–41 y 54–56 que exige el plan §4.2 y que usarán los
  titulares de la variante de suplentes (mapeo `i, i+19, i+38`) en T4.
- En las cargas de los planes derivados las materias nuevas quedan al final del vector (tras las de
  5.º); el plan no exige orden dentro de las cargas y los recuentos por año (9/9/10/12/13) son
  correctos.
- `validarPerfil()` ignora las materias nuevas en la comprobación de progresión (`esMateriaNueva`
  primero), para que «Física Aplicada» no se confunda con el área «Física» ni «Ciencias de la
  Tierra Aplicadas» con «Ciencias de la Tierra».
- La comprobación de «aula repetida» es **entre cursos** (dos cursos con la misma aula), coherente
  con el caso límite de RF-5; se ejercita con un perfil sintético porque las aulas/cursos reales se
  pueblan en T3. La validación de turnos/16 aulas/cursos completos queda para T3.

---

## T3 — Turnos, recesos, aulas y cursos

**Fecha:** 2026-10-17

### Qué se hizo

- **2 turnos** en `perfilPorDefecto()` (RF-4, plan §4.1): `Mañana` 07:00–12:30 y `Tarde` 12:40–18:10,
  **8 franjas** (`numSlots = 8`) cada uno y **receso de 10 min tras la franja 2**
  (`recesoDespuesDeSlot = 2`): mañana 08:20–08:30 y tarde 14:00–14:10. El `fin` es la campana (incluye
  el receso); la rejilla aritmética termina en 12:20/18:00 y el desfase de 10 min se asume (spec 008).
- **16 aulas** con nombre único, capacidad (30–40), edificio (A/B/C) y piso (`Planta baja`/`Piso 1`)
  variados: `Aula 01`…`Aula 13` para los cursos, `Aula 14` de reserva, `Laboratorio de Ciencias` y
  `Taller de Informática` (RF-5, plan §4.3).
- **13 cursos** (3 secciones en 1.º–3.º y 2 en 4.º–5.º: A/B/C y A/B), **sin turno** (el tipo `CursoDef`
  no declara campo `turno`; el turno lo aporta el preset, RF-4), cada uno con un **aula distinta**
  (`Aula 01`…`Aula 13` en orden).
- **`validarPerfil()` extendido** (plan §5, alcance T3): exactamente 2 turnos con nombre/ventana/franjas
  y receso tras la franja 2 exactos; 16 aulas con nombre único; 13 cursos con año 1–5, sección de un
  carácter en mayúscula, pareja año+sección única y aula existente y **distinta**. Mensajes en español
  que identifican la entidad.
- **Tests nuevos** en `test_seed_perfil.cpp`: `cursosAulasYTurnos` (inicio/fin/franjas, `recesoDespuesDeSlot
  = 2`, 08:20–08:30 y 14:00–14:10, 16 aulas únicas con laboratorio/taller, 13 cursos sin turno con aula
  única) y `validarPerfilRechazaTurnosAulasYCursos` (receso mal colocado, aula duplicada, aula
  inexistente y pareja año+sección repetida).

### Archivos tocados

- `src/seed/src/perfil.cpp` — turnos, aulas y cursos en `perfilPorDefecto()`; turnos/aulas/cursos en
  `validarPerfil()`.
- `test/backend/test_seed_perfil.cpp` — casos `cursosAulasYTurnos` y
  `validarPerfilRechazaTurnosAulasYCursos`.

### Verificación (rojo → verde)

1. **Rojo** (tests escritos antes del código; con `turnos` vacío —población deshabilitada
   temporalmente—):
   ```
   Totals: 12 passed, 4 failed
   FAIL! : cursosAulasYTurnos()  Actual (turnos.size()): 0, Expected: 2
   FAIL! : validarPerfilRechazaTurnosAulasYCursos()  Actual (turnos.size()): 0, Expected: 2
   FAIL! : validarPerfilAceptaPorDefecto()  «El perfil debe tener 2 turnos (tiene 0).»
   ```
2. **Verde** (tras rellenar `perfil.cpp`):
   ```
   docker exec gestor-dev bash -lc "cd /workspace && cmake --build build --target test_seed_perfil \
     && ctest --test-dir build -R test_seed_perfil --output-on-failure"
   ```
   `test_seed_perfil ..... Passed` (16 funciones: initTestCase, 14 casos y cleanupTestCase).
3. **Suite completa**: `cmake --build build && ctest --preset full --output-on-failure` → **45/45
   verde** (sin rojos; `test_seed_perfil` sigue siendo el test #41).

### Hallazgos / decisiones

- **`despues_de_slot`** se modela como `recesoDespuesDeSlot = 2` (1-based, plan §4.1): la franja 2 de la
  mañana termina a 08:20 y la de la tarde a 14:00, que coinciden con el `recesoInicio`.
- **«Sin turno» es estructural**: `CursoDef` no tiene campo `turno`, así que no hay nada que validar a
  nivel de perfil; el turno se materializa como `NULL` al insertar en T5a
  (`crearCurso(anio, seccion, QString(), idAula)`). Los tests lo documentan en un comentario.
- **Orden de comprobación de cursos**: la repetición de aula se comprueba **antes** que la pareja
  año+sección y que el recuento de 13, para que el caso heredado de T2
  (`validarPerfilRechazaCorrupciones`, que añade dos cursos con `Aula 01` al perfil por defecto ya
  poblado) siga produciendo el mensaje con «Aula 01».
- El recuento de 16 aulas y de 13 cursos se comprueba **después** de recorrer (y de detectar
  duplicados/aulas inexistentes), para que los mensajes identifiquen la causa concreta y no solo el
  total.

---

## T4 — Variantes de docentes, carga y cobertura

**Fecha:** 2026-10-17

### Qué se hizo

- **`perfil.hpp`/`perfil.cpp`**: se añadieron las funciones puras de carga del plan §3/§4.4:
  - `horasMaximas(perfil, materia)`: horas máximas de una materia entre los planes.
  - `cargaMateria(perfil, materia)`: `horasMaximas × seccionesDeAnio(anio)`; **0** para las 6
    materias nuevas (sustituyen horas base, RF-3). Resultados: `Castellano 1º = 12`,
    `Matemáticas 3º = 12`, `Ciencias de la Tierra 5º = 4`, `Matemática Aplicada = 0`.
  - `cargaAsignada(perfil, materias)`: Σ `cargaMateria`.
  - `demandaTotal(perfil)`: Σ `cargaMateria` del catálogo = **520** (13 cursos × 40).
- **`docentes.hpp`/`docentes.cpp`** (nuevos, añadidos al `CMakeLists.txt` de `seed`):
  - `VarianteDocentes`, `FranjaDef`, `DocenteDef` (id/nombre/email/teléfono/materias/
    disponibilidad/horasAula).
  - `docentesDeVariante(perfil, variante)`: genera las **4 variantes** deterministas:
    - **Holgada (65)**: P001–P057 con una materia cada uno (catálogo en orden) y tope 30; P058–P065
      parciales (índices 0/7/14/21/28/35/42/49) con disponibilidad de un solo turno (mañana
      `07:00–12:20` los pares, tarde `12:40–18:00` los impares). 5 filas por docente → 325.
    - **Suplentes (43)**: P001–P019 titulares con tres materias (índices `i`, `i+19`, `i+38`) y
      tope `ceil(carga / 1,15)`; P020–P043 suplentes sin materias con tope 8. Cargas de los 19
      titulares **Σ 520**, topes **Σ 461** (88,65 % de 520).
    - **Sin profesorado (64)**: la holgada **sin P023** (`Física 3º` queda sin docente).
    - **Sin disponibilidad (65)**: la holgada con la disponibilidad de P023 sustituida por
      `19:00–21:00` (solape nulo en ambos turnos).
  - `coberturaDeVariante(perfil, variante, error)`: por cada materia-año, ≥1 docente asignado y,
    por **cada** turno, ≥1 docente con un rango que contiene un bloque completo (fronteras
    `inicio + k×40`; el receso no desplaza bloques).
  - `docentesCumplenCarga(perfil, variante, error)`: holgada `≤ 0,6×tope`; suplentes
    `≥ 0,8×tope`, ratio `[1,00, 1,15]` y `Σ topes ∈ [0,85, 0,90] × demanda`; las inválidas
    heredan la holgada.
  - `nombreVariante()` y `varianteDesdeTexto()` (acepta con guiones).
- **Tests** en `test_seed_perfil.cpp` (8 casos nuevos): `cargaMateriaYDemanda`,
  `variantesValidasCubrenAmbosTurnos`, `varianteSinProfesoradoNoCubre`,
  `varianteSinDisponibilidadNoCubre`, `recuentosEsperados`, `holgadaCumpleTope`,
  `suplentesCumplenTopeYCapacidad` y `varianteNombresYTextos`.

### Archivos tocados

- `src/seed/include/seed/perfil.hpp` — declaraciones `horasMaximas`/`cargaMateria`/`cargaAsignada`/
  `demandaTotal` (+ `#include <QStringList>`).
- `src/seed/src/perfil.cpp` — implementaciones de las 4 funciones.
- `src/seed/include/seed/docentes.hpp` — nuevo (variantes, `DocenteDef`, cobertura y carga).
- `src/seed/src/docentes.cpp` — nuevo (generación de las 4 variantes y comprobaciones).
- `src/seed/CMakeLists.txt` — `docentes.cpp`/`docentes.hpp` en `target_sources`.
- `test/backend/test_seed_perfil.cpp` — 8 casos nuevos + helpers `asignacionesDe`/`disponibilidadesDe`.

### Desviaciones del plan (menores)

1. **`docentesDeVariante(perfil, variante)`** lleva el `perfil` como primer parámetro (el plan §4.4
   la escribió `docentesDeVariante(VarianteDocentes)`), porque el tope de los titulares de suplentes
   se calcula como `ceil(cargaAsignada / 1,15)` y `cargaAsignada` necesita el perfil. La decisión del
   plan §9 («docentes por regla», no tabla literal) exige esa dependencia.
2. **`cargaAsignada` acepta `const QStringList&`** (no `const QVector<QString>&` como el plan §3),
   para casar con `DocenteDef::materias` (`QStringList`, plan §4.4) y poder llamar
   `cargaAsignada(perfil, docente.materias)` sin conversión. El plan §3 y §4.4 eran incoherentes
   entre sí; se resolvió a favor del tipo de `DocenteDef`.
3. **`validarPerfil` no se extendió** con el paso 7 de `docentesCumplenCarga` del plan §5: el alcance
   de T4 (tasks.md y las instrucciones) solo pide `cargaMateria`/`cargaAsignada`/`demandaTotal` en
   `perfil` y las funciones de `docentes.hpp`; ningún «Hecho cuando» de T4 lo exige y los tests
   llaman a `docentesCumplenCarga` directamente. Se deja constancia para quien cierre T5b/T6 si se
   quiere enlazar la validación.

### Verificación (rojo → verde)

1. **Rojo** (tests escritos antes del código): el build falla al no existir el header:
   ```
   test/backend/test_seed_perfil.cpp:3:10: fatal error: seed/docentes.hpp: No such file or directory
   ```
2. **Verde** (tras implementar):
   ```
   docker exec gestor-dev bash -lc "cd /workspace && cmake --build build --target test_seed_perfil \
     && ctest --test-dir build -R test_seed_perfil --output-on-failure"
   ```
   `test_seed_perfil ..... Passed`. El binario reporta **24 passed** (initTestCase + 22 casos +
   cleanupTestCase), con los 8 casos nuevos en verde.
3. **Suite completa**: `cmake --build build && ctest --preset full --output-on-failure` →
   **45/45 verde** (sin rojos; `test_seed_perfil` sigue siendo el test #41).

### Hallazgos / decisiones

- **Números del plan confirmados por código y tests**: `demandaTotal = 520`; cargas por materia de
  **0 a 18** (máximo 18 = «Geografía, Historia y Ciudadanía 1º/2º», 6 h × 3 secciones, exactamente el
  60 % del tope 30); suplentes con **19 titulares + 24 suplentes**, `Σ topes = 461` (88,65 % de 520)
  y ratios carga/tope en `[1,09, 1,148]` (dentro de `[1,00, 1,15]`); recuentos
  **65/65/325 · 43/57/215 · 64/64/320 · 65/65/325**.
- **Índices estables**: `P023` = índice 22 = `Física 3º` (ya fijado en T1); es el docente que la
  variante sin profesorado elimina y al que la sin disponibilidad cambia la disponibilidad a
  `19:00–21:00`. La cobertura de ambas inválidas falla en `Física 3º` (sin docente / sin solape).
- **Aritmética entera sin coma flotante**: `tope = (carga * 100 + 114) / 115` = `ceil(carga/1,15)`,
  y las comparaciones de porcentajes se hacen con multiplicación cruzada (`carga*10 <= 6*tope`, etc.)
  para evitar redondeos.
- **Bloques de 40 min**: la comprobación de solape usa `turno.inicio.addSecs(k * 40 * 60)` con el
  fin en `+40 min`, exactamente la rejilla aritmética de la spec (el receso no desplaza bloques).

---

## T5a — Aplicador: apertura, detección y catálogo

**Fecha:** 2026-10-17

### Qué se hizo

- **`src/seed/include/seed/sembrador.hpp` + `src/seed/src/sembrador.cpp`** (nuevos, añadidos al
  `CMakeLists.txt` de `seed`): `OpcionesAplicacion`, `ResultadoAplicacion` (con el enum completo
  `EstadoAplicacion` del plan §6: `Ok`, `UsoInvalido`, `BaseConDatos`, `VarianteDesconocida`,
  `ConfirmacionRequerida`, `RespaldoFallido`, `EsquemaNoEsperado`, `FalloApertura`,
  `FalloEscritura`, `PerfilIncoherente`) y `aplicar()`.
- **`Recuentos`** (struct de 10 contadores del plan §3) añadido a `perfil.hpp`, con
  inicializadores a 0; `ResultadoAplicacion::recuentos` lo usa. Se rellena **contando en la base
  tras el commit** (`leerRecuentos`), no desde el perfil (plan §8).
- **`aplicar()`**:
  1. `UsoInvalido` si `rutaDb` vacía (RF-10).
  2. `PerfilIncoherente` si `validarPerfil()` falla.
  3. `VarianteDesconocida` si la variante no es una de las cuatro (RF-9).
  4. `UsoInvalido` si `--reemplazar` y `--solo-docentes` juntos.
  5. Apertura/creación con `ContextoBaseDatos`; `FalloEsquema`/`FalloApertura` mapean a
     `EsquemaNoEsperado`/`FalloApertura`.
  6. Detección de datos: suma de `COUNT(*)` sobre las 10 tablas de dominio; `> 0` sin
     `reemplazarTodo` → `BaseConDatos` **sin escribir**.
  7. **Transacción única** (`db.transaction()`), y dentro: turnos+recesos (`crearTurno` +
     `agregarReceso`), aulas (guardando `nombre→id`), materias (guardando `nombre→id`), planes y
     las **210 cargas por SQL directo** (`INSERT INTO PlanEstudio_Materia …` preparado una vez y
     `bindValue` por fila; **nunca** `asignarCarga`), cursos con `turno` NULL y aula existente.
     Cualquier `Resultado::ok == false` → `rollback()` → `FalloEscritura`; si no, `commit()`.
- **No inserta docentes** (T5b): `Profesores`/`Profesor_Materia`/`Disponibilidad_Profesor` quedan
  a 0 filas; la variante se valida pero se usa en T5b.
- **`test/backend/test_seed_aplicacion.cpp`** (nuevo, `add_qtest(test_seed_aplicacion
  backend/test_seed_aplicacion.cpp seed Qt6::Sql)` en `test/CMakeLists.txt`), con `QTemporaryDir`:
  - `creaBaseNuevaYPuebla`: `Ok` y recuentos 2/2/16/57/4/210/13/0/0/0; recesos exactos
    `(Mañana, 2, 08:20, 08:30)` y `(Tarde, 2, 14:00, 14:10)`; 13 cursos con `turno` NULL y aula
    distinta.
  - `baseConDatosFallaSinEscribir`: segunda aplicación → `BaseConDatos`, recuentos intactos.

### Archivos tocados

- `src/seed/include/seed/perfil.hpp` — struct `Recuentos`.
- `src/seed/include/seed/sembrador.hpp` — nuevo (estados, opciones, `aplicar()`).
- `src/seed/src/sembrador.cpp` — nuevo (apertura, detección, transacción, inserciones, recuentos).
- `src/seed/CMakeLists.txt` — `sembrador.cpp`/`sembrador.hpp` en `target_sources`.
- `test/backend/test_seed_aplicacion.cpp` — nuevo (2 casos).
- `test/CMakeLists.txt` — `add_qtest(test_seed_aplicacion … seed Qt6::Sql)` en `if(TARGET seed)`.

### Desviaciones del plan (menores)

1. **`Recuentos` en `perfil.hpp`** con inicializadores `= 0` (el plan §3 no los pone): evita leer
   campos no inicializados en los caminos de error de `ResultadoAplicacion`. `recuentosEsperados()`
   queda para T5b/T8 (no se declara aún).
2. **`reemplazarTodo`/`soloDocentes`/`confirmado`/`ahora`/`simularFalloTrasDocentes`** se declaran
   en `OpcionesAplicacion` (API del plan §6) y se implementa la guarda de operaciones
   contradictorias; el reemplazo total, el respaldo y la conmutación de docentes llegan en T6 y no
   se ejecutan en T5a (los casos de T5a usan las opciones por defecto).
3. **`EstadoAplicacion` no se usa aún** en `ConfirmacionRequerida`/`RespaldoFallido` (T6); quedan
   declarados para que la API sea estable entre T5a y T6.

### Verificación (rojo → verde)

1. **Rojo**: el test se redactó y registró antes de `sembrador.cpp`; el primer intento de compilar
   fallaba por no existir el header `seed/sembrador.hpp` (y luego, una corrección del test: `QCOMPARE`
   no admite la braced-init-list `QStringList{…}` con comas —se extrajo a variables nombradas—).
2. **Verde** (test del aplicador):
   ```
   docker exec gestor-dev bash -lc "cd /workspace && cmake --preset full \
     && cmake --build build --target test_seed_aplicacion \
     && ctest --test-dir build -R test_seed_aplicacion --output-on-failure"
   ```
   `1/1 Test #42: test_seed_aplicacion … Passed` (2 casos + init/cleanup).
3. **Suite completa**: `cmake --build build && ctest --preset full --output-on-failure` →
   **46/46 verde** (45 previos + `test_seed_aplicacion` como test #42).

### Hallazgos / decisiones

- Los servicios de alta (`crearTurno`, `agregarReceso`, `crearAula`, `crearMateria`, `crearPlan`,
  `crearCurso`) **no abren transacción propia** (verificado en sus `.cpp`); las cargas de
  `PlanEstudio_Materia` se insertan por SQL directo para no anidar `asignarCarga`. Así la
  transacción única del seed envuelve todo el catálogo (RF-10, RNF-6).
- La columna de enlace se llama **`curso`** (no `anio`): `CargaPlan.anio` se mapea a `curso` en el
  `INSERT INTO PlanEstudio_Materia`, y `horas` a `horas`.
- `crearCurso(anio, seccion, QString(), idAula)` deja `turno` NULL (el servicio une `QVariant()`
  cuando el turno está vacío), y `crearAula`/`crearMateria` devuelven el `id` generado
  (`lastInsertId`), que se usa para el mapeo `nombre→id`.

---

## T5b — Aplicador: docentes, enlaces y recuentos

**Fecha:** 2026-10-17

### Qué se hizo

- **`recuentosEsperados(perfil, variante)`** (nuevo): calcula los recuentos completos del conjunto
  (2 turnos, 2 recesos, 16 aulas, 57 materias, 4 planes, 210 materias de plan, 13 cursos y, según la
  variante, docentes/asignaciones/disponibilidades) leyendo el perfil y `docentesDeVariante`.
- **`aplicar()`** inserta ahora, **dentro de la misma transacción**, los docentes de la variante
  pedida (por defecto holgada), sus `Profesor_Materia` y sus `Disponibilidad_Profesor` (5 filas por
  docente), **por SQL directo** (tres `QSqlQuery` preparados una vez y `bindValue` por fila), mapeando
  nombre de materia → `id` con el `QHash` ya construido en T5a; el `id` de profesor es el literal
  estable `P001`…`P065` (RF-11). La columna `dia` de `Disponibilidad_Profesor` es TEXT y se escribe
  como `QString::number(franja.dia)` (1=lunes…5=viernes), igual que `ServicioProfesor`.
- **Comprobación de recuentos completos tras el commit** (RF-12): antes de `commit()` se contrasta
  `leerRecuentos(db)` contra `recuentosEsperados()`; si difieren → `rollback()` → `FalloEscritura`.
- **`simularFalloTrasDocentes`** (seam de atomicidad, RF-10/RNF-6): fuerza el rollback justo tras los
  docentes; una base nueva queda con **0 filas en las 10 tablas**.
- **Tests** en `test_seed_aplicacion.cpp`: `creaBaseNuevaYPuebla` y `baseConDatosFallaSinEscribir`
  actualizados a 65/65/325, y dos casos nuevos `determinismo` (dos bases idénticas salvo
  `fecha_creacion`/`fecha_modificacion`, comparando las 10 tablas por columnas de identidad) y
  `rollbackAnteFalloTrasDocentes` (0 filas en las 10 tablas).

### Archivos tocados

- `src/seed/include/seed/docentes.hpp` — declara `recuentosEsperados()`.
- `src/seed/src/docentes.cpp` — implementa `recuentosEsperados()`.
- `src/seed/src/sembrador.cpp` — inserción de docentes/enlaces/disponibilidad, comprobación de
  recuentos y fallo simulado.
- `test/backend/test_seed_aplicacion.cpp` — recuentos a 65/65/325, `determinismo`,
  `rollbackAnteFalloTrasDocentes` y helper `filasDe`.

### Desviaciones del plan (menores)

1. **`recuentosEsperados()` vive en `docentes.hpp`/`docentes.cpp`** (el plan §3 lo lista en
   `perfil.hpp`): depende de `docentesDeVariante` y de `VarianteDocentes`, ambos de `docentes.hpp`; ahí
   queda sin forward-declaration del enum ni nueva dependencia de `perfil.cpp`.
2. **Inserción de docentes por SQL directo** (el plan §6.4, punto 6, sugiere `ServicioProfesor`):
   la instrucción de la tarea pide «por SQL directo»; además evita las comprobaciones intermedias de
   `crearProfesor`/`agregarDisponibilidad` y mantiene el alta determinista, coherente con el patrón ya
   usado para `PlanEstudio_Materia`. Ningún servicio usado abre transacción propia (regla del plan).

### Verificación (rojo → verde)

1. **Rojo**: los casos nuevos y los recuentos a 65/65/325 se redactaron antes de `sembrador.cpp`; el
   primer intento no compilaba por `QSqlError` incompleto (faltaba `#include <QSqlError>` en el test).
2. **Verde** (test del aplicador):
   ```
   docker exec gestor-dev bash -lc "cd /workspace && cmake --build build --target test_seed_aplicacion \
     && ctest --test-dir build -R test_seed_aplicacion --output-on-failure"
   ```
   `test_seed_aplicacion … Passed` (6 funciones: initTestCase, `creaBaseNuevaYPuebla`,
   `baseConDatosFallaSinEscribir`, `determinismo`, `rollbackAnteFalloTrasDocentes`, cleanupTestCase).
3. **Suite completa**: `cmake --build build && ctest --preset full --output-on-failure` → **46/46
   verde** (sin rojos).

### Hallazgos / decisiones

- La columna `dia` de `Disponibilidad_Profesor` es **TEXT** (no entero), como confirma `migracion.cpp`
  y `ServicioProfesor::agregarDisponibilidad`; se escribe el día como texto `1..5`.
- Los `id` de `Materias`/`Aulas`/`Cursos` son `AUTOINCREMENT` y estables por orden de inserción
  determinista; el determinismo se comprueba **sin** las marcas de auditoría ni los `id`
  auto-incrementados de `Disponibilidad_Profesor` (RF-11, RNF-5).
- `Profesor_Materia` **no** tiene columnas de auditoría; `Profesores` y `Disponibilidad_Profesor` sí
  (`fecha_creacion`/`fecha_modificacion` con `DEFAULT CURRENT_TIMESTAMP`), coherente con RNF-5.

---

## T6 — Reemplazo, respaldo, conmutación y atomicidad

**Fecha:** 2026-10-17

### Qué se hizo

- **`rutaRespaldoLibre(rutaDb, ahora)`** (pública, en `sembrador.hpp`/`sembrador.cpp`, plan §6.2):
  parte de `Respaldo::construirNombreRespaldo(rutaDb, ahora)` y, si el archivo ya existe, prueba
  `<ruta>-2`, `<ruta>-3`… hasta 99; devuelve `QString()` si los 99 están ocupados. Es determinista
  (no escribe nada, solo consulta `QFile::exists`), de modo que la prueba puede fijar el `ahora` y
  predecir la ruta del respaldo.
- **`aplicar()` reestructurado** para cubrir las operaciones destructivas (RF-9, RF-10, RNF-6):
  - **Detección y confirmación:** si la base tiene datos y no se pide operación destructiva →
    `BaseConDatos` (sin escribir). Si se pide `--reemplazar` o `--solo-docentes` **sin** `--si` →
    `ConfirmacionRequerida` (antes de respaldar, así no queda un respaldo huérfano).
  - **Respaldo obligatorio** antes de borrar, solo si hay datos que preservar: `rutaRespaldoLibre`
    + `Respaldo::crearRespaldo` (VACUUM INTO, fuera de la transacción). Si falla → `RespaldoFallido`
    **sin borrar nada** (RF-10, constitución §5). La ruta queda en `ResultadoAplicacion::rutaRespaldo`.
  - **Reemplazo total:** dentro de la misma transacción, borrado por SQL directo de las 10 tablas en
    orden FK-seguro (plan §6.3: `Disponibilidad_Profesor` → `Profesor_Materia` → `Cursos` →
    `PlanEstudio_Materia` → `PlanEstudio` → `Materias` → `Recesos` → `Turnos` → `Aulas` →
    `Profesores`) y repoblación completa; `commit()` único.
  - **Solo docentes:** borrado de `Disponibilidad_Profesor`, `Profesor_Materia` y `Profesores`, y
    carga de la variante pedida; el resto de tablas (incluidas las 210 cargas) no se toca. El mapa
    `materia→id` se reconstruye **leyendo la base** (`SELECT id, nombre FROM Materias`), porque en la
    conmutación el catálogo ya está sembrado y no se recorre el perfil.
  - **Atomicidad:** `simularFalloTrasDocentes` sigue forzando el rollback tras insertar los docentes;
    con reemplazo, la base conserva **exactamente** los datos previos (incluidas las 210 cargas).
- **Refactor de las lambdas**: el alta de docentes se extrajo a una lambda `insertarDocentes` (bool +
  `QString* error`) y los borrados a `borrarTabla`, reutilizados por la conmutación y el reemplazo.
  Ninguna llama a métodos con transacción propia (`asignarCarga`, `eliminarProfesor`, `eliminarTurno`,
  `eliminarConCascada`), que siguen prohibidos dentro de la transacción del seed (plan §6.4).
- **Tests** en `test_seed_aplicacion.cpp` (rojo primero), 5 casos nuevos:
  - `rutaRespaldoLibre_eligeRutaDisponible`: base libre, y sufijo `-2`/`-3` cuando hay colisiones.
  - `reemplazoTotalConRespaldo`: `Ok`, respaldo existe y `Respaldo::validarRespaldo` = true, recuentos
    correctos (65/65/325, 210 cargas).
  - `reemplazoTotalSinConfirmacion`: `ConfirmacionRequerida`, sin respaldo ni cambios (65 docentes,
    210 cargas intactas).
  - `soloDocentesConservaElResto`: conmutación holgada→suplentes → docentes 43 / asignaciones 57 /
    disponibilidades 215, y las 7 tablas no-docentes idénticas antes/después (incluidas las 210
    cargas), comparadas por columnas de identidad.
  - `atomicidadAnteFalloEnReemplazo`: reemplazo total a suplentes con fallo simulado → `FalloEscritura`
    y base previa intacta (65 docentes y 210 cargas idénticas).

### Archivos tocados

- `src/seed/include/seed/sembrador.hpp` — declara `rutaRespaldoLibre()`.
- `src/seed/src/sembrador.cpp` — `rutaRespaldoLibre()`, confirmación, respaldo, borrados por SQL en
  orden FK y reestructura de `aplicar()` para `--reemplazar` y `--solo-docentes`.
- `test/backend/test_seed_aplicacion.cpp` — 5 casos nuevos + includes `QDateTime`, `QFile` y
  `backend/database/respaldo.hpp`.

### Desviaciones del plan (menores)

1. **`resumenTexto()` no se declara aún** en `sembrador.hpp` (el plan §2/§6 la lista junto a
   `aplicar()`/`rutaRespaldoLibre()`): el informe llega en T8 y T6 no la usa. Se deja constancia.
2. **`rutaRespaldoLibre` consulta `QFile::exists`** (el plan la llama «pura»): decide el nombre sin
   escribir nada, pero para detectar colisiones tiene que mirar el sistema de archivos; es
   determinista dado el estado del directorio. La prueba fija `ahora` y controla los archivos
   preexistentes.
3. **Confirmación/respaldo condicionados a «hay datos»**: la confirmación se exige siempre en una
   operación destructiva, y el respaldo solo se crea si hay filas que preservar (una base vacía no
   tiene qué respaldar). Coherente con «el respaldo es previo a destruir» (RF-10).

### Verificación (rojo → verde)

1. **Rojo**: los 5 casos se redactaron antes de `sembrador.cpp`/`sembrador.hpp`; el build fallaba por
   `rutaRespaldoLibre` no declarado (y el aplicador aún no hacía reemplazo/conmutación/respaldo).
2. **Verde** (test del aplicador):
   ```
   docker exec gestor-dev bash -lc "cd /workspace && cmake --build build --target test_seed_aplicacion \
     && ctest --test-dir build -R test_seed_aplicacion --output-on-failure"
   ```
   `test_seed_aplicacion … Passed` — 11 funciones (initTestCase + 9 casos + cleanupTestCase), con los
   5 casos de T6 en verde.
3. **Suite completa**: `cmake --build build && ctest --preset full --output-on-failure` →
   **46/46 verde** (sin rojos).

### Hallazgos / decisiones

- Los `DELETE FROM <tabla>` directos se ejecutan con `foreign_keys = ON` (lo habilita
  `ContextoBaseDatos::abrir` en su conexión persistente, verificado); el orden de borrado del plan
  §6.3 es correcto y no dispara violaciones de clave foránea.
- `VACUUM INTO` se ejecuta **antes** de `db.transaction()` (no puede ir dentro); por eso el respaldo
  captura el estado previo al borrado, que es lo que exige RF-10/RNF-6.
- El `id` de `Materias` es `AUTOINCREMENT`; al reinsertar tras un reemplazo, los ids pueden avanzar,
  pero el mapa `materia→id` se reconstruye desde lo insertado y los enlaces (`PlanEstudio_Materia`,
  `Profesor_Materia`) usan esos ids reales, así que el resultado es internamente coherente y las
  pruebas comparan por identidad (nombres/códigos/horas), no por id numérico.

---

## T7 — Ejecutable `gestor-horarios-seed`

**Fecha:** 2026-10-17

### Qué se hizo

- **`src/seed/src/main.cpp`** (nuevo): CLI del aplicador con `QCoreApplication` y `QTextStream`, con
  las opciones exactas del plan §7:
  - `--db <ruta>` (obligatoria), `--variante <v>` (por defecto `holgada`; se resuelve con
    `varianteDesdeTexto`, que acepta con guiones), `--reemplazar`, `--solo-docentes`, `--si`,
    `--ayuda`/`--help`/`-h` y `--version`.
  - La variante desconocida se detecta **en el parseo** y devuelve directamente el **código 6**
    (antes de abrir la base), coherente con la verificación de §11.
  - `--ayuda` y `--version` terminan con **código 0** sin exigir `--db`.
  - Sin `--db` (o `--db` sin valor), o con un argumento desconocido → mensaje en español + uso y
    **código 1**.
  - El desenlace de `aplicar()` se mapea a código con `codigoSalida(EstadoAplicacion)` según la tabla
    de §7: `Ok→0, UsoInvalido→1, BaseConDatos→2, ConfirmacionRequerida→3, RespaldoFallido→4,
    EsquemaNoEsperado/FalloApertura→5, VarianteDesconocida→6, FalloEscritura→7, PerfilIncoherente→8`.
- **Resumen mínimo en éxito** (RF-12; el informe completo con materias por año y totales por plan es
  **T8**): imprime `Base`, `Variante`, la línea de recuentos por entidad (`resultado.recuentos`,
  leídos de la base por `aplicar()`), `Cursos (sin turno)`, `Respaldo` (ruta o «no aplica») y `ok`.
  No se toca `resumenTexto` (sigue sin declararse; es T8).
- **`src/seed/CMakeLists.txt`**: target `gestor-horarios-seed` que enlaza `seed` (PRIVATE; hereda
  `datos` + `Qt6::Core`/`Qt6::Sql` como PUBLIC) y define `PROJECT_VERSION="${PROJECT_VERSION}"` para
  `--version`.

### Archivos tocados

- `src/seed/src/main.cpp` — nuevo (CLI, salida en español y códigos de salida).
- `src/seed/CMakeLists.txt` — target `gestor-horarios-seed` y `PROJECT_VERSION`.

### Verificación (CLI y suite)

1. **Build del target**:
   ```
   docker exec gestor-dev bash -lc "cd /workspace && cmake --preset full \
     && cmake --build build --target gestor-horarios-seed"
   ```
   `[2/2] Linking CXX executable src/seed/gestor-horarios-seed` (éxito).

2. **Primera aplicación** sobre base nueva:
   ```
   ./build/src/seed/gestor-horarios-seed --db /tmp/opencode/seed-t7.db
   ```
   Imprime `Variante: holgada`, recuentos `2/2/16/57/4/210/13/65/65/325`, `Respaldo: no aplica` y
   `ok`; **código 0**.

3. **Segunda aplicación** sobre la misma base:
   ```
   ./build/src/seed/gestor-horarios-seed --db /tmp/opencode/seed-t7.db; echo $?
   ```
   → `La base ya tiene datos; use «--reemplazar» para reemplazarlos.` y **código 2**.

4. **Variante inexistente**:
   ```
   ./build/src/seed/gestor-horarios-seed --db /tmp/opencode/seed-t7-var.db --variante inexistente; echo $?
   ```
   → `Variante de docentes desconocida: «inexistente»` y **código 6**.

5. **Ayuda y versión** (código 0 ambos): `--ayuda` lista las 7 opciones y su uso; `--version` imprime
   `gestor-horarios-seed v0.1.0`.

6. **Conmutación destructiva (extra, para cubrir las opciones del CLI)**:
   ```
   ./build/src/seed/gestor-horarios-seed --db /tmp/opencode/seed-t7.db \
     --variante suplentes --solo-docentes --si
   ```
   → `Variante: suplentes`, `Docentes: 43 / Materias asignadas: 57 / Disponibilidades: 215`, con
   `Respaldo: /tmp/opencode/seed-t7.db.respaldo-…` y `ok`; **código 0**.

7. **Suite completa**:
   ```
   docker exec gestor-dev bash -lc "cd /workspace && cmake --build build \
     && ctest --preset full --output-on-failure"
   ```
   `100% tests passed, 0 tests failed out of 46` (46/46 verde, sin rojos).

### Desviación del plan (menor, ya prevista en las instrucciones de T7)

- El plan §8 describe la salida de éxito completa (materias por año y totales por plan), que es
  **T8**. T7 imprime solo el resumen mínimo (recuentos por entidad + variante + respaldo + `ok`),
  usando `resultado.recuentos`; no se implementa `resumenTexto` ni las líneas de materias/totales.
- El binario queda en `build/src/seed/gestor-horarios-seed` (layout Ninja, igual que
  `gestor-horarios-consola`), no en `build/gestor-horarios-seed` como decía literalmente la
  instrucción de la tarea; es el camino del plan §11 y el que se usó en la verificación.

### Hallazgos / decisiones

- Los códigos de salida se concentran en una única función `codigoSalida()` con el mapeo 1:1 de la
  tabla §7; `EsquemaNoEsperado` y `FalloApertura` comparten el código 5 (apertura/esquema), tal como
  fija el plan.
- El parseo resuelve `--variante` con `varianteDesdeTexto` (acepta `sin-profesorado` y
  `sin profesorado`); una variante inexistente no llega a `aplicar()` y el proceso sale con 6 sin
  tocar la base.
- La salida usa `QTextStream(stdout)`/`(stderr)` con `flush()` explícito antes de `return`, para no
  perder texto al cerrar `QCoreApplication`.

---

## T8 — Informe de recuentos, materias por año y totales

**Fecha:** 2026-10-17

### Qué se hizo

- **`resumenTexto(perfil, variante, recuentos)`** (pública, `sembrador.hpp`/`sembrador.cpp`,
  nombre exacto del plan §6): devuelve el informe determinista de §8 en un `QString`:
  - línea `Variante` (parámetro, no forma parte de `Recuentos`);
  - los **recuentos por entidad** (turnos, recesos, aulas, materias, planes, materias de plan,
    cursos, docentes, asignaciones y disponibilidades) tal como `aplicar()` los leyó de la base tras
    el commit;
  - **`Materias por año`** calculada del perfil (validado): `GEN 9/9/10/11/12 · EXA 9/9/10/12/13 ·
    NAT 9/9/10/12/13 · SOC 9/9/10/12/13`;
  - las **cuatro líneas de plan** con sus totales semanales por año `40/40/40/40/40 h académicas`,
    con el nombre alineado a la izquierda (ancho del nombre más largo) para una lectura estable.
- **CLI (`main.cpp`)**: `imprimirResumen` sustituye el resumen mínimo por el informe completo:
  `Base` + `resumenTexto(...)` + `Respaldo` (ruta o «no aplica») + `ok`. `resumenTexto` no recibe la
  ruta de la base ni la del respaldo (no están en su firma del plan §6), así que esas dos líneas las
  imprime la CLI, que dispone de `opciones.rutaDb` y `resultado.rutaRespaldo`; el orden resultante
  coincide con §8.
- **Test `tiempoDeAplicacion`** en `test_seed_aplicacion.cpp` (RNF-1): aplica sobre una base nueva y
  comprueba con `QElapsedTimer` que tarda **< 15 000 ms**.

### Archivos tocados

- `src/seed/include/seed/sembrador.hpp` — declara `resumenTexto()`.
- `src/seed/src/sembrador.cpp` — implementa `resumenTexto()`.
- `src/seed/src/main.cpp` — `imprimirResumen` usa `resumenTexto` (informe completo).
- `test/backend/test_seed_aplicacion.cpp` — `#include <QElapsedTimer>` + caso `tiempoDeAplicacion`.

### Verificación

1. **Test del aplicador**:
   ```
   docker exec gestor-dev bash -lc "cd /workspace && cmake --build build --target test_seed_aplicacion \
     && ctest --test-dir build -R test_seed_aplicacion --output-on-failure"
   ```
   `1/1 Test #42: test_seed_aplicacion … Passed` (2,44 s), con `tiempoDeAplicacion` en verde.

2. **CLI** (informe completo):
   ```
   docker exec gestor-dev bash -lc "cd /workspace && cmake --build build --target gestor-horarios-seed \
     && rm -f /tmp/opencode/seed-t8.db && build/src/seed/gestor-horarios-seed --db /tmp/opencode/seed-t8.db"
   ```
   ```
   Base: /tmp/opencode/seed-t8.db
   Variante: holgada
   Turnos: 2   Recesos: 2   Aulas: 16   Materias: 57   Planes: 4   Materias de plan: 210
   Cursos: 13 (sin turno)   Docentes: 65   Materias asignadas: 65   Disponibilidades: 325
   Materias por año: GEN 9/9/10/11/12 · EXA 9/9/10/12/13 · NAT 9/9/10/12/13 · SOC 9/9/10/12/13
   GEN  General            40/40/40/40/40 h académicas
   EXA  Ciencias Exactas   40/40/40/40/40 h académicas
   NAT  Ciencias Naturales 40/40/40/40/40 h académicas
   SOC  Ciencias Sociales  40/40/40/40/40 h académicas
   Respaldo: no aplica
   ok
   ```
   Código de salida **0** y `ok`; materias por año y `40/40/40/40/40` correctos.

3. **Suite completa**:
   ```
   docker exec gestor-dev bash -lc "cd /workspace && cmake --build build \
     && ctest --preset full --output-on-failure"
   ```
   `100% tests passed, 0 tests failed out of 46` (46/46 verde, sin rojos).

### Decisiones / hallazgos

- **Reparto de responsabilidades de las líneas de ruta**: `resumenTexto` solo recibe
  `(perfil, variante, recuentos)` (firma del plan §6), así que las líneas `Base:` y `Respaldo:` las
  compone la CLI; el bloque central que devuelve `resumenTexto` es determinista (sin fechas ni rutas
  temporales, como exige §8).
- **Materias por año y totales salen del perfil** (no de la base): son estructura validada por
  `validarPerfil()` antes de aplicar; los `Recuentos` (conteo de filas) sí se leen de la base tras el
  commit, tal como fija §8.
- El separador de planes en `Materias por año` es el punto medio `·` y los totales usan `/` entre
  años, siguiendo el formato literal de §8.

---

## T9 — Documentación y cierre

**Fecha:** 2026-10-17

### Qué se hizo

- Se creó **`docs/guias/datos-prueba.md`** (guía de uso en español, RF-13): qué contiene el conjunto
  (Plan 31059 + 4 planes, 2 turnos, 16 aulas, 57 materias, 210 cargas, 13 cursos), requisitos y
  compilación (`cmake --preset full` / `cmake --build build`), uso completo de
  `gestor-horarios-seed` con todas las opciones y códigos de salida, las **4 variantes de docentes**
  con sus recuentos y propósito, seguridad (`--reemplazar`/`--solo-docentes` con `--si` y respaldo,
  atomicidad), determinismo, y los límites (datos ficticios, **no genera presets**, la duración de la
  franja no cambia la carga: 1 hora académica = 1 bloque) con el ejemplo de salida real.

### Archivos tocados

- `docs/guias/datos-prueba.md` — nuevo (guía de uso del conjunto de datos de prueba).

### Verificación

1. **CLI contra la salida real** (los comandos que documenta la guía son los reales):
   ```
   docker exec gestor-dev bash -lc "cd /workspace && cmake --preset full && cmake --build build \
     && rm -rf /tmp/seed-t9 && mkdir -p /tmp/seed-t9 \
     && build/src/seed/gestor-horarios-seed --db /tmp/seed-t9/centro.db"
   ```
   → `Variante: holgada`, recuentos `2/2/16/57/4/210/13/65/65/325`, `Materias por año: GEN
   9/9/10/11/12 · EXA 9/9/10/12/13 · NAT 9/9/10/12/13 · SOC 9/9/10/12/13`, las cuatro líneas de plan
   con `40/40/40/40/40 h académicas`, `Respaldo: no aplica` y `ok` (código **0**).
   ```
   docker exec gestor-dev bash -lc "cd /workspace \
     && build/src/seed/gestor-horarios-seed --db /tmp/seed-t9/centro.db \
     --solo-docentes --variante suplentes --si"
   ```
   → `Variante: suplentes`, `Docentes: 43 / Materias asignadas: 57 / Disponibilidades: 215`, resto
   intacto, `Respaldo: /tmp/seed-t9/centro.db.respaldo-…` y `ok` (código **0**).
   - `--ayuda` lista las 7 opciones y `--version` imprime `gestor-horarios-seed v0.1.0` (código 0).
   - Base con datos sin operación destructiva → `La base ya tiene datos; use «--reemplazar» para
     reemplazarlos.` (código **2**); variante inexistente → `Variante de docentes desconocida:
     «inexistente»` (código **6**).

2. **Suite completa**:
   ```
   docker exec gestor-dev bash -lc "cd /workspace && ctest --preset full --output-on-failure"
   ```
   `100% tests passed, 0 tests failed out of 46` (46/46 verde, sin rojos; `test_seed_perfil` #41 y
   `test_seed_aplicacion` #42 incluidos).

### Decisiones / hallazgos

- La guía replica el formato literal de salida de §8 (separador `·` en materias por año, `/` en los
  totales, alineado de nombres de plan) tomado de la salida real del ejecutable, no del plan.
- No se detectó ninguna errata imprescindible en la ayuda de la CLI: los textos, opciones y códigos de
  `main.cpp` coinciden con la salida verificada.

---

## Corrección post-validación (fase 6)

**Fecha:** 2026-10-17

### Hallazgo (reviewer, RF-2 · plan §5 paso 3)

`validarPerfil` solo rechazaba un año con `> 40` horas académicas. Un plan General corrompido que
sumara 39 h (o con una materia con horas distintas a la tabla adaptada) pasaba la validación, porque
ningún año superaba 40. Faltaba la comprobación exacta del plan General contra la tabla canónica de
§4.2 y el cierre de recuentos del plan §5.6.

### Cambio

- **`src/seed/src/perfil.cpp`**
  - Añadida la **tabla canónica del plan General** como constante independiente del `Perfil` que se
    valida: `CargaCanonica` + `cargaCanonica31059(nombre)` derivan de `kAreas31059` (la tabla del
    31059) el año y las horas académicas esperadas de cada materia-año. `validarPerfil` compara el
    plan General **contra esta constante, no contra sí mismo**.
  - Nuevo bloque «3bis» en `validarPerfil`: el plan General debe coincidir exactamente — por año
    `9/9/10/11/12` materias y **Σ == 40 h académicas** (mensaje con **año y diferencia**, p. ej.
    «Plan General, año 1: suma 39 h académicas; se esperaban 40 (diferencia -1)»), **51 cargas**, y
    cada materia con **horas y año idénticos** a la tabla canónica (rechaza una materia de GEN con
    horas distintas aunque el año siga sumando 40, y también una materia ajena al 31059).
  - Nuevo bloque «6» de **recuentos**: 57 materias, 4 planes y 210 plan-materias (2 turnos/2 recesos,
    16 aulas y 13 cursos ya se contaban).
- **`src/seed/src/sembrador.cpp`** — tras `validarPerfil` y `varianteValida`, `aplicar()` comprueba
  `docentesCumplenCarga(perfil, variante)` (plan §5 paso 7) y falla con `PerfilIncoherente` si la
  variante pedida incumple la carga. Es una guarda pura y defensiva (siempre cierta para el perfil
  canónico).
- **`test/backend/test_seed_perfil.cpp`** — 3 casos nuevos: `validarPerfilRechazaGeneralConHorasDesviadas`
  (año 1 → 39 con «diferencia -1», y horas compensadas que rechaza la tabla canónica),
  `validarPerfilRechazaGeneralConMateriasDesviadas` (recuento 9/9/10/11/12) y
  `validarPerfilRechazaRecuentos` (209 plan-materias y 56 materias).
- **`test/backend/test_seed_aplicacion.cpp`** — 2 casos nuevos: `rutaVaciaEsUsoInvalido` y
  `varianteDesconocidaEsRechazada` (guarda de `aplicar()`, enum fuera de rango).

### Verificación (rojo → verde)

1. **Rojo** (conceptual): los 3 casos nuevos de `test_seed_perfil` apuntan a un comportamiento que el
   código previo **no rechazaba** (solo comprobaba `> 40 h`); contra la implementación anterior,
   `validarPerfilRechazaGeneralConHorasDesviadas` habría fallado en su primer bloque (el perfil
   corrompido habría validado como `true`).
2. **Verde**:
   ```
   docker exec gestor-dev bash -lc "cd /workspace && cmake --build build --target test_seed_perfil test_seed_aplicacion \
     && ctest --test-dir build -R test_seed --output-on-failure"
   ```
   `test_seed_perfil … Passed` y `test_seed_aplicacion … Passed` (2/2, 0 fallos).
3. **Suite completa**:
   ```
   docker exec gestor-dev bash -lc "cd /workspace && cmake --build build && ctest --preset full --output-on-failure"
   ```
   `100% tests passed, 0 tests failed out of 46` (46/46 verde, sin rojos).

### Decisiones / desviaciones (menores)

- **`coberturaDeVariante` NO se añade a `aplicar()`**: devuelve `false` a propósito en las variantes
  `SinProfesorado` y `SinDisponibilidad` (RF-8, diseñadas para ejercitar huecos de cobertura), así que
  comprobarla en la aplicación rompería el sembrado de esas dos variantes. En cambio se añade
  `docentesCumplenCarga`, que sí es cierta para las cuatro variantes (las inválidas heredan la
  relación de la holgada). No hay dependencia circular (docentes no depende de sembrador).
- **`varianteDesconocida`/`rutaVacia`**: se añadieron como tests directos de `aplicar()`
  (`static_cast<VarianteDocentes>(999)` y `rutaDb` vacía), en lugar de anotar solo su cobertura por la
  CLI/guards.
- **`.clangd`/`.gitignore`**: no se toca `.gitignore` en esta corrección (`.clangd` es preexistente y
  ajeno a la spec).
- La comprobación de año de la materia canónica (`carga.anio != canonica.anio`) es una defensa extra
  del «horas **y años**» del plan §5.3; con el perfil canónico siempre coincide.




