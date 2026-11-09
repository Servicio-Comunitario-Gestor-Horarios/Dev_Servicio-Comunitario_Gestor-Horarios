# Journal 017 - Datos reales del centro

Registro de la implementación de las tareas del plan 017. Una entrada por tarea ejecutada; las
entradas son **aditivas** (no se reescriben las anteriores).

- **Base:** `b5844b580d495397903e5d3bfd8756aa41d98f05` (sha real de `HEAD`), sin commits.
- **Fecha de apertura:** 2026-11-08.
- **Estado:** T1–T5 y T8 hechas; T6, T7, T9 y T10 pendientes (T10 la cierra el implementer).
- **Suites de partida medidas (`ctest -N`, 2026-11-08):** **61** (`backend`) y **66**
  (`full-release`). El QA del plan anotaba 62/67; el criterio que manda es «todas verdes» y se deja
  aquí el número **medido**, como pedía T1.

```text
Comandos de referencia
  backend:      cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend
  full-release: cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release
  semilla:      build-backend/src/seed/gestor-horarios-seed --db /tmp/c17.db
```

---

## T1 - Sustitución del conjunto: perfil, docentes, sembrador y tests del seed

**Fecha:** 2026-11-08
**Requisito:** RF-1, RF-2, RF-3, RF-4, RF-5, RF-6, RF-7, RF-8 · RNF-4, RNF-6, RNF-7, RNF-8.
**Plan:** §2, §3, §4 completo, §5, §8 y §13.

### Rojo (tests primero)

Se reescribieron `test/backend/test_seed_perfil.cpp` y `test/backend/test_seed_aplicacion.cpp`
antes de tocar el perfil. El rojo fue de **compilación**, porque el contrato nuevo aún no existía:

```text
error: ‘DocenteDef’ no se declaró en este ámbito              (sembrador.cpp)
error: ‘validarDocentes’ no se declaró en este ámbito          (sembrador.cpp)
error: no matching function for call to ‘TestSeedPerfil::areaConCursosEnTurno(...)’
       (el slot del test ocultaba la función pura: se renombró a areasConCursosEnTurno)
```

### Cambios

- **`perfil.hpp` / `perfil.cpp`**: `MateriaAnio.area/sinCarga`, `CursoDef.plan/turno/matricula`,
  `Perfil.duracionMinutos`, `Recuentos.duracionMinutos/configuraciones`. Tablas reales: 2 planes
  (31059 **34/34/34/36/34** y 8/8/9/10/10; 31060 **40×5** y 10×5), catálogo de **85** (79 con carga
  + 6 sin carga, 16 compartidas), **95** cargas, **24** cursos con turno y matrícula, **13** aulas
  (12 de 44 + cancha de 88, tipo `Cancha`), **2 turnos de 40 min** con **3 eventos**
  (07:00–12:40 y 12:40–18:10) y `duracionMinutos = 40`. Salen `seccionesDeAnio`, `esMateriaNueva`,
  `cargaMateria`, `cargaAsignada`, `horasMaximas` y los planes derivados; entran `horasDeCurso`,
  `demandaTotal`, `demandaPorTurno`, `areaConCursosEnTurno`. `validarPerfil` reescrita (duración,
  turnos/eventos con la duración del perfil, aulas/cancha, tipos, catálogo, tablas canónicas por
  plan, progresión del 31059, cursos con turno/aula general/matrícula/12 por turno).
- **`docentes.hpp` / `docentes.cpp`**: fuera `VarianteDocentes`, `nombreVariante`,
  `varianteDesdeTexto`, `coberturaDeVariante`, `docentesCumplenCarga`; **32 docentes** (30 del
  centro + P031/P032 añadidos), regla §5.4 (comparación **exacta** de área, compartidas una vez,
  sin filtrar por año, solo si el área tiene cursos en el turno), tope **36**, disponibilidad de
  **5 filas** con la ventana del turno en reloj. Entran `docentesDelCentro`, `validarDocentes`
  (32, únicos, tope, 16/16, 5 filas, **272**) y `coberturaDelCentro` (materia × curso × turno).
- **`sembrador.hpp` / `sembrador.cpp` / `main.cpp`**: `OpcionesAplicacion` sin
  `variante`/`soloDocentes`, `EstadoAplicacion` sin `VarianteDesconocida`, configuración **40**
  antes de crear turnos (el servicio de turnos valida el encaje con ella), `crearCurso(...,
  curso.turno, ...)`, sin conmutación, recuentos comparados **con duración y configuraciones**, y
  `resumenTexto(perfil, recuentos)`.
- **Tests**: `test_seed_perfil` reescrito completo (planes, catálogo, cargas, cursos/matrícula,
  aulas/cancha, turnos/eventos, duración, demanda 876/414/462, áreas con cursos, 32 docentes, 272
  asignaciones con la tabla por área, **18** de 5.º de 31060, cobertura y rechazos);
  `test_seed_aplicacion` con los recuentos nuevos y sin variantes/suplentes/`--solo-docentes`;
  `test_selector_generacion` con `"NAT"` → **`"31059"`**; baja de `test_generacion_seed` de
  `test/CMakeLists.txt` (hueco declarado; lo cierra `test_entrada_seed` en T4).

### Verde

```text
$ ctest --preset backend                 → 100% tests passed, 61/61 (17,84 s)
$ ctest --preset full-release            → 100% tests passed, 65/65 (85,61 s)
    (66 de partida − test_generacion_seed = 65; test_selector_generacion incluido y verde)
$ grep -n 'kRecortes\|kMateriasNuevas\|"EXA"\|"SOC"\|"GEN"\|esMateriaNueva' \
      src/seed/src/perfil.cpp src/seed/include/seed/perfil.hpp   → sin resultados (exit 1)
```

Salida real de la semilla sobre base nueva:

```text
$ build-backend/src/seed/gestor-horarios-seed --db /tmp/c17.db
Base: /tmp/c17.db
Hora académica: 40 min (configuraciones: 1)
Turnos: 2   Eventos: 3   Aulas: 13   Materias: 85   Planes: 2   Materias de plan: 95
Cursos: 24 (12 Mañana · 12 Tarde)   Docentes: 32   Materias asignadas: 272   Disponibilidades: 160   Tipos de aula: 2
Preferencias: 0
Materias por año: 31059 8/8/9/10/10 · 31060 10/10/10/10/10
31059  Media General         34/34/34/36/34 h académicas
31060  Ciencias y Tecnología 40/40/40/40/40 h académicas
Respaldo: no aplica
ok                                                        (código 0)
```

Comprobación directa en la base:

```text
sqlite3 /tmp/c17.db "SELECT (SELECT COUNT(*) FROM Materias), (SELECT COUNT(*) FROM Cursos),
  (SELECT COUNT(*) FROM Profesores), (SELECT COUNT(*) FROM Profesor_Materia),
  (SELECT COUNT(*) FROM Disponibilidad_Profesor), (SELECT COUNT(*) FROM Aulas),
  (SELECT COUNT(*) FROM Eventos), (SELECT COUNT(*) FROM PlanEstudio_Materia),
  (SELECT COUNT(*) FROM Turnos), (SELECT COUNT(*) FROM TiposAula),
  (SELECT duracion_minutos FROM ConfiguracionHoraria WHERE id=1);"
→ 85|24|32|272|160|13|3|95|2|2|40
Cursos por turno → Mañana 12 · Tarde 12 · sin turno 0
```

### Decisiones y desviaciones registradas

1. **Línea base medida 61/66** (no 62/67 del QA). Tras la baja de `test_generacion_seed`, la suite
   completa queda en 65. Ambas verdes.
2. **CLI**: T1 retira el parseo de `--variante`/`--solo-docentes`, el estado
   `VarianteDesconocida` y su código 6 (eran necesarios para compilar) y adapta
   `imprimirResumen` a la firma nueva. La **ayuda** y la descripción del centro siguen siendo de
   **T3**, que es su tarea; la ayuda aún nombra los flags retirados hasta entonces.
3. **`ResultadoAplicacion`** ya no lleva `duracionMinutos`/`configuraciones` duplicados: viven en
   `Recuentos` (plan §3), que el sembrador rellena desde la base y compara en `recuentosCoinciden`.
4. **«18 de 5.º de 31060»**: se interpreta como las asignaciones de las 10 materias del 5.º de
   31060 hechas a docentes de la **tarde** (las 4 compartidas con 31059 5.º también van a la mañana,
   que sí tiene cursos; por eso el total de esas materias es 26, no 18). Coincide con la derivación
   por área del plan §4.7 (Matemáticas 2, EF 2, Física 2, Química 1 en la tarde). Queda comentado en
   `asignacionesDeQuintoDe31060`.
5. **Rechazos no alcanzables con el conjunto real**: «materia que exige un tipo sin aulas» (la única
   exigencia especial es la cancha, siempre sembrada) y los recuentos defensivos de 85 materias / 95
   cargas (las tablas canónicas fallan antes). La validación los conserva; los tests cubren el resto
   por la vía alcanzable (catálogo vacío/duplicado y `tipoFinalDeMateria` con y sin cancha).
6. **Reglas de tipo de materia** se reducen a Educación Física → `Cancha` y el resto → general: el
   centro no tiene laboratorios ni talleres (RF-3).

---

## T2 - Informe «no generable» y motivo

**Fecha:** 2026-11-08
**Requisito:** RF-8, RF-9. **Plan:** §5.5 y §5.7.

### Cambios

- **`perfil.hpp`/`perfil.cpp`**: nuevas funciones puras `demandaDeAreaPorTurno(perfil, área,
  turno)` (48 clases de Educación Física por turno) y `motivoNoGenerable(perfil)`, que devuelve el
  primer turno que no cabe —o `""` si el conjunto cabría— y, sin cancha, la causa. No es
  validación: `aplicar` no falla por esto.
- **`sembrador.cpp` (`resumenTexto`)**: el informe añade `Centro: Liceo Nacional Robert Serra
  (2026/2027)`, `Clases: 876 (EF 96; 48 por turno)`, `Matrícula: 889 (579 + 310)`,
  `Capacidad docente: 576 por turno (16 × 36)   Demanda: 414 Mañana · 462 Tarde` y la línea
  `Generable: no — <motivo>`.
- **Tests**: `test_seed_perfil::motivoNoGenerableExplicaLaCancha` (48/40, cancha, aforo
  simultáneo, sin cancha y sin cursos) y, en `test_seed_aplicacion`,
  `resumenTexto_informaClasesMatriculaCapacidadYNoGenerable` e
  `informeTrasAplicarNoFallaYMuestraElMotivo`.

**Texto del motivo (centro real):**

```text
«Mañana»: 48 clases de Educación Física frente a 40 franjas de cancha (aforo 1). Pendiente el
aforo simultáneo (dos cursos), que lo resolverá su spec.
```

**Nota de proceso:** las funciones puras y sus pruebas se escribieron en el mismo paso (la tarea no
admite estados intermedios); no se capturó un rojo separado. La evidencia es la suite acotada en
verde.

### Verde

```text
$ ctest --preset backend -R "test_seed_perfil|test_seed_aplicacion" --output-on-failure
1/2 Test #54: test_seed_perfil .......   Passed    0.11 sec
2/2 Test #55: test_seed_aplicacion ...   Passed    1.54 sec
100% tests passed out of 2
```

---

## T3 - CLI: ayuda, códigos y binario

**Fecha:** 2026-11-08
**Requisito:** RF-1. **Plan:** §3 y §4 (menores de T3).

### Cambios

- `imprimirUso`: ayuda sin `--variante` ni `--solo-docentes`, descripción del centro real
  (2 planes, 24 cursos, 12 aulas y cancha, 32 docentes, 40 min), conjunto único sin variantes y
  **aviso de que no genera** (48 clases de EF frente a 40 franjas). `--reemplazar` y `--si` siguen.
- Comentario de `src/seed/CMakeLists.txt` actualizado (ya no habla de «variantes de docentes»).
- Sin arnés de CLI en el proyecto: se verifica por ejecución (abajo) y esta entrada.

### Verde

```text
$ build-backend/src/seed/gestor-horarios-seed --ayuda            → código 0; no menciona
                                                     --variante ni --solo-docentes
$ build-backend/src/seed/gestor-horarios-seed --db /tmp/x.db --variante holgada
  Argumento desconocido: --variante                              → código 1
$ build-backend/src/seed/gestor-horarios-seed --db /tmp/x.db --solo-docentes
  Argumento desconocido: --solo-docentes                         → código 1
$ grep -rn -- "--variante\|--solo-docentes\|VarianteDocentes\|soloDocentes" src/seed
  → sin resultados (exit 1)
```

---

## T4 - `test_entrada_seed` (sustituye a `test_generacion_seed`)

**Fecha:** 2026-11-08
**Requisito:** RF-9, RNF-2. **Plan:** §8.

### Cambios

- Nuevo `test/frontend/test_entrada_seed.cpp` (`QTEST_GUILESS_MAIN`): siembra, compone con
  `GeneracionSolver::construirEntrada` los **24 cursos** (plan y turno del perfil,
  `numEstudiantes = curso.matricula`), **exige sin avisos**, valida con
  `validarEntradaSolver` (**V1–V19**: 24 cursos, 32 docentes, 2 planes, 85 materias, 13 aulas,
  2 turnos × 8 franjas, 3 eventos, 40 min), comprueba que cada curso conserva plan/turno/matrícula
  y aula general, hace **ida y vuelta del preset** con `CargadorConfiguracionSolver` (24 cursos,
  32 docentes, 40 min) y comprueba el «no generable» del informe. **No resuelve**.
- Alta en `test/CMakeLists.txt`: `add_qtest(test_entrada_seed frontend/test_entrada_seed.cpp
  frontend seed Qt6::Sql)` + `target_include_directories(... src/frontend/src)`, dentro del bloque
  `frontend AND backend`. Por eso **no** entra en el preset `backend` (sin interfaz) y **sí** en
  `full-release`.

### Verde

```text
$ ctest --preset full-release -R test_entrada_seed --output-on-failure
1/1 Test #67: test_entrada_seed ...   Passed    1.82 sec
$ ./build-rel/test/test_entrada_seed
Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted
$ grep -n "resolverEntradaSolver" test/frontend/test_entrada_seed.cpp
  → sin resultados (exit 1)
```

---

## T5 - `test_seed_aplicacion`: seguridad, determinismo y 15 s

**Fecha:** 2026-11-08
**Requisito:** RF-8, RNF-1, RNF-4. **Plan:** §8.

### Estado

Los casos se consolidaron en T1 (la unidad atómica) y T2 añadió el informe. Se revisan y quedan
cubiertos, sin variantes:

- **Seguridad:** `baseConDatosFallaSinEscribir`, `reemplazoTotalSinConfirmacion`,
  `reemplazoTotalConRespaldo` (respaldo válido), `atomicidadAnteFalloEnReemplazo`,
  `rollbackAnteFalloTrasDocentes`, `reemplazoTotalConCursosAsignados`,
  `reemplazoTotal_conPreferenciaPreinsertada_borraSinFallarPorFK`.
- **Reutilización del tipo general + cancha:** `aplicar_reutilizaElTipoGeneral`,
  `reemplazar_conservaElTipoGeneral`, `aplicar_recuentoTiposEsDos`.
- **Determinismo:** `determinismo` y `aplicar_dosVeces_mismosDatosYRecuentos` (dos bases y
  reemplazo con `filasDe` idénticas, incluida `ConfiguracionHoraria`).
- **Informe:** `resumenTexto_incluyeLaHoraAcademicaYSuRecuento`,
  `resumenTexto_informaClasesMatriculaCapacidadYNoGenerable`,
  `informeTrasAplicarNoFallaYMuestraElMotivo`.
- **Tiempo:** `tiempoDeAplicacion` exige **< 15000 ms** y cita el medido si falla.

### Verde

```text
$ ctest --preset backend -R test_seed_aplicacion --output-on-failure
1/1 Test #55: test_seed_aplicacion ...   Passed    1.54 sec   (Totals: 23 passed)
$ time build-backend/src/seed/gestor-horarios-seed --db /tmp/c17.db
→ 0,089 s total (aplicación sobre base nueva)
```

---

## Suites tras T2–T5

```text
$ cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend
100% tests passed out of 61                                    (27,00 s)
$ cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release
100% tests passed out of 66                                    (19,97 s)
```

**Justificación del cambio de número:** `backend` sigue en **61** porque `test_entrada_seed` enlaza
`frontend` y no se registra en el preset sin interfaz. `full-release` pasa de **65** a **66** por el
alta de `test_entrada_seed` (T4); `test_generacion_seed` sigue de baja desde T1.

---

## T8 - Supersesión de la 008 (planner)

**Fecha:** 2026-11-08
**Requisito:** RF-10. **Plan:** §9 y §3.

### Cambios

- **`specs/008-datos-de-prueba/spec.md`**: **nota aditiva** al principio (línea 5) — «Sustituida en sus
  datos por la 017 (2026-11-08)»: centro real (2 planes, 85 materias, 24 cursos con turno, 13 aulas +
  cancha, 32 docentes, 40 min), retirada de las **4 variantes** y de los planes derivados, superación de
  «cada curso un aula propia y distinta» (12 aulas compartidas entre turnos), **cursos con turno**, ventana
  de mañana **12:30 → 12:40** por el acto cívico, y **no generable** hasta el aforo (48 clases de EF por
  turno frente a 40 franjas). Se declara vigente la seguridad, el respaldo, el reemplazo total y la
  transacción única. No se reescribe nada de lo histórico.
- **`specs/008-datos-de-prueba/estado.md`**: `Fase:` pasa a «cerrada · **superada en datos por la 017
  (2026-11-08)**»; «Siguiente paso» apunta a la 017; se añaden **«Superación»** (tabla 008 → 017 y lo que
  sigue vigente) e **«Historial»** con la entrada del 2026-11-08.
- **`specs/017-datos-reales-del-centro/estado.md`**: **T8 marcada**; tabla de implementación (T1–T5 y T8
  hechas; T6, T7 y T9 pendientes; T10 sin marcar) y entrada de historial.
- **`specs/017-datos-reales-del-centro/tasks.md`**: `T8` → `[x]`.
- **`MEMORY.md`**: **no editable** por el planner (permiso denegado); el parche se entrega al coordinator
  con las cifras, el cambio `test_generacion_seed` → `test_entrada_seed` y el siguiente paso (aforo
  simultáneo y rendimiento).

### Verde

```text
$ grep -n "017" specs/008-datos-de-prueba/spec.md
5:> **Sustituida en sus datos por la 017 (2026-11-08).** El conjunto de datos de prueba pasa a ser el **centro
20:> `specs/017-datos-reales-del-centro/`.
44:> del 24-08-2017; …        ← falso positivo: «2017» contiene «017»

$ grep -n "por la 017" specs/008-datos-de-prueba/spec.md
5:> **Sustituida en sus datos por la 017 (2026-11-08).**

$ git diff --stat -- specs/
 specs/008-datos-de-prueba/estado.md | 35 +++++++++++++++++++++++++++++++++--
 specs/008-datos-de-prueba/spec.md   | 17 +++++++++++++++++
 2 files changed, 50 insertions(+), 2 deletions(-)
$ git status --short -- specs/
 M specs/008-datos-de-prueba/estado.md
 M specs/008-datos-de-prueba/spec.md
?? specs/017-datos-reales-del-centro/        ← el directorio de la 017 (spec, estado, plan, tareas, journal)
```

Sin cambios de código en esta tarea (`git status` no muestra `src/` ni `test/` por T8).

---

## T6 - Guía `datos-prueba.md`

**Fecha:** 2026-11-08
**Requisito:** RF-10. **Plan:** §9.

Reescritura completa de `docs/guias/datos-prueba.md` para el **centro real**:

- §1 recuentos: 2 planes (**31059** 34/34/34/36/34 y **31060** 40×5), **85 materias** (79 con
  carga + 6 sin carga; 16 compartidas), **95 cargas**, **24 cursos** con turno y letras libres,
  **13 aulas** (12 de 44 compartidas + cancha de 88), **32 docentes** (272 asignaciones, 160
  disponibilidades, tope 36), **40 min**, 3 eventos, **876 clases** (96 de EF, 48/turno) y
  **matrícula 889** (579 + 310); tablas de planes, catálogo, cursos/turnos y eventos.
- §2 compilación (`cmake --preset backend`, binario en `build-backend/…`) y §3 uso **sin variantes**
  (solo `--db`, `--reemplazar`, `--si`, `--ayuda`, `--version`; códigos sin el 6), con la salida real
  del seed.
- §4 profesorado del centro, regla de asignación y tope 36; §5 seguridad (falla con datos, `--si`,
  respaldo y atomicidad); §6 límites (no generable por **48 > 40** por turno, EF/cancha, aforo
  pendiente, paquete demo fuera de alcance, duración y determinismo); §7 verificación.

### Verde

```text
$ grep -n "57 materias\|210 cargas\|65 docentes\|16 aulas\|13 cursos\|holgada" \
       docs/guias/datos-prueba.md
  → sin resultados (exit 1)
$ grep -n "85 materias\|95 cargas\|24 cursos\|32 docentes\|272\|889\|40 min" \
       docs/guias/datos-prueba.md
  → todas presentes (2, 1, 3, 5, 4, 3 y 8 coincidencias)
```

Sin apariciones de los códigos de plan retirados ni de las antiguas opciones de variante.

---

## T7 - Guía `datos-prueba-frontend.md`

**Fecha:** 2026-11-08
**Requisito:** RF-10. **Plan:** §9.

Reescritura completa de `docs/guias/datos-prueba-frontend.md`:

- §1 la hora académica es un bloque y ahora dura **40 min** (34/36/40 bloques por curso según plan y
  año; tiempo real 1360/1440/1600 min).
- §2 recuentos exactos: **2 planes** (**31059**/**31060**), **85 materias**, **95 cargas**,
  **24 cursos**, **13 aulas**, **32 docentes**, 272/160, 2 turnos × 40 min, 3 eventos, matrícula
  **889** y 876 clases.
- §3 abrir/usar la base y §4 **«los cursos ya traen turno»**: `turno` y `aula` de la base, `plan` del
  preset, matrícula sugerida; la configuración **no** sobrescribe el turno (D-8); desplegable de
  plan con **31059/31060**; reparto plan/turno del centro.
- §5 un solo profesorado y «no generable» (48 > 40; se informa como `NoFactible`, no como error de
  validación); §6 checklist y §7 resumen de cambios.

### Verde

```text
$ grep -n "57 materias\|210 cargas\|65 docentes\|16 aulas\|13 cursos\|holgada" \
       docs/guias/datos-prueba-frontend.md
  → sin resultados (exit 1)
$ grep -n "31059\|31060\|24 cursos\|32 docentes\|889" docs/guias/datos-prueba-frontend.md
  → todas presentes (10, 11, 5, 3 y 4 coincidencias)
$ grep -n "NAT\|65" docs/guias/datos-prueba-frontend.md
  → sin resultados (exit 1)
```

---

## T9 - La interfaz con el centro real

**Fecha:** 2026-11-08
**Requisito:** RF-4, RF-6, RF-10. **Plan:** §8 y §12 (riesgo 3).

- `docs/interfaz-frontend.md`: el builder **ya no** describe el turno de la base como vacío (lo traen
  los 24 cursos y el preset lo replica); la duración derivada de la vista es **40 min**
  (`(340 − 20) / 8` y `(330 − 10) / 8`); la columna **Turno** muestra el valor sembrado y la
  configuración no lo sobrescribe (D-8); el párrafo de integración apunta a **`test_entrada_seed`**
  (compone y valida **sin resolver**) y se añade la **actualización 2026-11-08 (017)** con los
  recuentos, la retirada de `test_generacion_seed` y las suites (61/61 y 66/66).
- `docs/guias/migracion-frontend.md`: datos del seed vigente actualizados (07:00–12:40 / 12:40–18:10,
  40 min, 3 eventos) y la deuda de `test_generacion_seed` marcada como resuelta por
  `test_entrada_seed`.
- `docs/guias/desarrollo/CMAKE_GUIDE.md`: nota de la 017 en el paquete de demostración (`datos-demo/`
  **no se regenera** con el centro real; sigue sintético). Es la única mención restante de los
  recuentos antiguos y describe el paquete demo, fuera de alcance por H7.
- Revisión de `src/frontend/src` (solo lectura): **ningún** literal `65`, `13 cursos`, `NAT`, `EXA`,
  `SOC`, `GEN` ni variantes en el código de la interfaz.
- `test_selector_generacion` ya usa **`31059`** y queda verde con 24 cursos y 32 docentes.

### Verde

```text
$ ctest --preset full-release -R test_selector_generacion --output-on-failure
1/1 Test #66: test_selector_generacion ...   Passed    9.75 sec
$ grep -n "65\|13 cursos\|NAT" test/frontend/test_selector_generacion.cpp
  → sin resultados (exit 1)
```

---

## T10 - Cierre

**Fecha:** 2026-11-08
**Requisito:** RNF-3, RNF-7. **Plan:** §10 y §11.

### Suites completas

```text
$ cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend
100% tests passed out of 61                                    (17,78 s)
$ cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release
100% tests passed out of 66                                    (17,51 s)
```

### Alcance

```text
$ git diff --stat -- src/backend/src/database src/backend/src/solver src/frontend/src
  → vacío (0 líneas)
$ grep -rn "resolverEntradaSolver" test/backend/test_seed_aplicacion.cpp \
       test/frontend/test_entrada_seed.cpp
  → sin resultados (exit 1)
```

**Matiz del `grep` amplio.** `grep -rn "resolverEntradaSolver" test/ | grep -v "/build"` sigue
encontrando dos usos **ajenos al conjunto**:

- `test/test_ipc_aislamiento_bd.cpp`: prueba legítima del camino `solver_resolve` del proceso de
  cálculo (spec 002/010); no compone ni valida el centro real.
- `test/frontend/test_generacion_seed.cpp`: archivo **retirado del build** en T1 (hueco declarado),
  sustituido por `test_entrada_seed` en T4; no se compila ni se ejecuta.

Los tests del conjunto (`test_seed_aplicacion` y `test_entrada_seed`, los que nombra el plan §10) **no
resuelven**. Retirar el archivo muerto o tocar el test de IPC quedaría fuera del alcance de la 017.
*(Actualizado el 2026-11-09: el archivo muerto **se borró** en el «Cierre técnico» de esa misma fecha; el
test de IPC sigue intacto y es ajeno al conjunto.)*

---

## Cierre técnico (2026-11-09)

**Retirado el test muerto:** se borró `test/frontend/test_generacion_seed.cpp` (ya no estaba
registrado en `test/CMakeLists.txt` desde T1 y lo sustituye `test_entrada_seed` desde T4); el
comentario del sustituto se reformuló para no citar el nombre del archivo retirado.
`grep -rn "test_generacion_seed" test/ src/ CMakeLists.txt` → **sin resultados**.
**No cambia ningún recuento:** `ctest --preset backend` **61/61** y `ctest --preset full-release`
**66/66**, ambas verdes.

---

## Correcciones de la validación en `specs/` (2026-11-09)

Planner. **No** se toca código en esta entrada. **Después**, el implementer cierra **V3** (informe con
clases por plan y año, con tests) y **V6** con **deuda declarada** (la guía del frontend aún menciona el
`NoFactible` «en la vista»); **T2** y **T7** quedan marcadas y las suites siguen **61/61** y **66/66**.

- **V7 (añadido en la misma corrección)** — El 5.º de 31060 (sin cursos) **no se imprime como 0**: el
  informe da `31060 120/80/80/80` (cuatro valores). Alineado en RF-8, criterio 8 y `plan.md` §5.7.
- **T9 (ajuste)** — `docs/guias/desarrollo/CMAKE_GUIDE.md` queda **excluido** del grep de recuentos
  obsoletos: su apartado del **paquete demo** describe el conjunto sintético (13 cursos, 65 docentes, 57
  materias, `holgada`) y **no se regenera** con el centro real (H7). La verificación de T9 se limita a
  `docs/interfaz-frontend.md`, `docs/guias/migracion-frontend.md` y la **nota de la 017** del CMAKE_GUIDE.

- **V1 — estado desfasado, no el árbol.** El borrado de `test/frontend/test_generacion_seed.cpp` ya estaba
  hecho y registrado aquí (sección «Cierre técnico», 2026-11-09). Se corrige el **estado**: el archivo se
  **eliminó** y **no** es deuda; `plan.md` §3 recoge el borrado además de la baja de CMake.
- **V2 — nombres reales fuera de las specs.** Los dos docentes que aparecían con su nombre real pasan a
  **P029** («única de BAT en la tarde») y **P028** («única de Inglés+Idiomas en la tarde»), sin apellidos,
  en `spec.md` y `estado.md`. `grep -rn` de los dos nombres en `specs/` → **sin resultados**.
- **V3 — informe incompleto (RF-8 / criterio 8).** RF-8 y el criterio 8 exigen **clases por plan** (31059
  **516** · 31060 **360**) y **por año** (68/102/102/108/136 y 120/80/80/80), además de 414/462 por turno.
  `plan.md` §5.7 y §10 y la tarea **T2** (reabierta) llevan la verificación. **Lo implementa el
  implementer.**
- **V4 — «18» ambiguo.** Son **18 asignaciones a docentes de la tarde** de materias de 5.º de 31060; el
  **total de esas 10 materias es 26** (las 4 compartidas con 31059 5.º también se asignan a la mañana).
  Corregido en `spec.md` (datos, RF-6, casos) y `plan.md` §4.7.
- **V5 — traza del plan.** `plan.md` §3 añade `docs/interfaz-frontend.md`,
  `docs/guias/migracion-frontend.md`, `docs/guias/desarrollo/CMAKE_GUIDE.md`,
  `specs/008-datos-de-prueba/estado.md`, `src/seed/CMakeLists.txt` y el **borrado** del test; **T9** añade
  la verificación de las **tres** guías con un `grep` de los recuentos nuevos.
- **V6 — guía contra RF-9.** Informar el «no generable» **no es función de interfaz** en esta entrega: el
  artefacto es el informe del conjunto (y la CLI); la guía del frontend lo describe, no lo promete como
  pantalla/aviso/diálogo (RF-9 y `plan.md` §9). **T7 reabierta** para la reescritura.

**Aceptación del exceso:** el usuario **aprobó** las 10 tareas con el exceso de T1/T4/T5 el **2026-11-08**
(registrado en `estado.md` y en `plan.md` §13).

---

## Correcciones de validación V3 y V6 (2026-11-09)

**V3 (RF-8) — informe por plan y por año.** `resumenTexto` añade `Clases por plan` (Σ de los cursos
de cada plan) y `Clases por año` (Σ por plan y año, **siempre con las 5 columnas**: 0 en un año sin
cursos, el 5.º de 31060). Se deriva de los datos. Aserción en `test_seed_aplicacion`
(`resumenTexto_informaClasesMatriculaCapacidadYNoGenerable`) y «Ejemplo de salida en éxito»
actualizado en `docs/guias/datos-prueba.md`.

**V6 (RF-9) — guía de frontend como deuda.** `docs/guias/datos-prueba-frontend.md` deja de exigir a
la interfaz informar o bloquear el «no generable»: lo describe como **comportamiento futuro / deuda**
para cuando el aforo simultáneo habilite la generación. `src/frontend/src` no se toca y no es
criterio de cierre; queda anotado también en `tasks.md` (T7, deuda declarada).

### Verde

```text
$ cmake --build --preset backend --parallel && ctest --preset backend
100% tests passed out of 61
$ cmake --build build-rel --parallel && ctest --preset full-release
100% tests passed out of 66
$ build-backend/src/seed/gestor-horarios-seed --db /tmp/res017.db | grep "Clases por"
Clases por plan: 31059 516 · 31060 360
Clases por año: 31059 68/102/102/108/136 · 31060 120/80/80/80/0
```

No cambia ningún recuento (61/61 · 66/66). Base temporal `/tmp/res017.db` borrada.

---

## Unificación de «Clases por año» (2026-11-09)

La **spec y el plan** fijan que **un año sin cursos no se imprime**: 31060 lleva **cuatro** valores.
Se unifica el código con la spec (esto sustituye la nota de V3 que citaba cinco columnas con 0):

- `resumenTexto` vuelve a **omitir los años sin cursos**: `Clases por año: 31059
  68/102/102/108/136 · 31060 120/80/80/80`.
- `test_seed_aplicacion` vuelve a la aserción de **cuatro** valores.
- `docs/guias/datos-prueba.md` (ejemplo de salida) también con cuatro valores.

### Verde

```text
$ cmake --build --preset backend --parallel && ctest --preset backend
100% tests passed out of 61
$ cmake --build build-rel --parallel && ctest --preset full-release
100% tests passed out of 66
$ build-backend/src/seed/gestor-horarios-seed --db /tmp/u017.db | grep "Clases por"
Clases por plan: 31059 516 · 31060 360
Clases por año: 31059 68/102/102/108/136 · 31060 120/80/80/80
```

No cambia ningún recuento (61/61 · 66/66). Base temporal `/tmp/u017.db` borrada.
