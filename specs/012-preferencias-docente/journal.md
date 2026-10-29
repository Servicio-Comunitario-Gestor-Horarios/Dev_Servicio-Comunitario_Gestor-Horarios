# Journal 012 - Preferencias horarias del docente

## T1 - Modelo puro y validación compartida de día/rango (2026-10-29)

**Requisito:** RF-1, RF-2, RNF-1. **Plan:** §1.1, §2.1 y §2.2. **Base:** `6d58f8e` (52/52).

### Rojo (tests primero)

`test/backend/test_preferencias.cpp` (10 casos) registrado con `add_gtest` y CMake regenerado; sin los
módulos el árbol no compila:

```text
$ cmake --build --preset backend --parallel
test_preferencias.cpp:7:10: fatal error: backend/services/franja_horaria_validacion.hpp:
No existe el fichero o el directorio
```

### Verde

```text
$ ctest --test-dir build-backend --output-on-failure -R '^(test_preferencias|test_servicio_profesor)$'
1/2 Test #40: test_servicio_profesor ...........   Passed    0.12 sec
2/2 Test #46: test_preferencias ................   Passed    0.01 sec
100% tests passed out of 2

$ ./build-backend/test/test_preferencias
10 tests from TestPreferencias
[  PASSED  ] 10 tests.

$ ctest --preset backend
100% tests passed out of 53
```

`test_servicio_profesor` sigue verde **sin cambios**: solo comprueba los fragmentos «día» y
«anterior», que los mensajes nuevos conservan (además de citar el valor y la acción).

### Cambios

- **Nuevo** `src/backend/include/backend/services/franja_horaria_validacion.hpp` y
  `src/backend/src/services/franja_horaria_validacion.cpp`: `franja_horaria::DIA_MIN/DIA_MAX` y
  `errorDeFranja` (día 0–6 con el valor y «Corrija el día»; horas válidas con «HH:mm» y «Corrija el
  rango»; inicio ≥ fin con «anterior» y «Corrija el rango»).
- **Nuevo** `src/backend/include/backend/services/preferencias.hpp` y
  `src/backend/src/services/preferencias.cpp`: `enum class SentidoPreferencia`,
  `textoDeSentido`, `sentidoDesdeTexto`, `sentidoValido` y `haySolape`
  (`a.inicio < b.fin && b.inicio < a.fin`).
- `src/backend/src/services/ServicioProfesor.cpp`: se retira `validarFranja` y los `DIA_MIN/DIA_MAX`
  locales; `agregarDisponibilidad` usa `franja_horaria::errorDeFranja`.
- `src/backend/CMakeLists.txt` (los dos `.cpp` en `backend`) y `test/CMakeLists.txt`
  (`test_preferencias`).
- `test/backend/test_preferencias.cpp`: 10 casos (extremos 0/6, −1/7 con valor y acción, horas
  inválidas, inicio ≥ fin, solape y contigüidad, ida y vuelta del sentido, valor casteado).

### Notas

- **Dos módulos, como el plan:** §1.1 separa la validación de franja
  (`franja_horaria_validacion.hpp`) del modelo de preferencias (`preferencias.hpp`), y `haySolape`
  queda en `preferencias` (allí lo sitúa §2.2). `errorDeFranja` vive en el namespace `franja_horaria`,
  como pide la tarea.
- **Registro como `add_gtest`:** el plan §1.9 dice `add_qtest(... backend)` para `test_preferencias`;
  la tarea de la fase pide `add_gtest` y el módulo es lógica pura (mismo criterio que
  `test_duracion_horaria`). El binario y su verificación no cambian.
- **`haySolape` no mira el día:** es la fórmula del plan; el servicio (T3) consulta por docente y día,
  así que el filtro de día es del llamador. Queda documentado en el header y en un caso del test.
- No se tocaron la base, el esquema, el seed, `docs/` ni nada de T2–T9.

## T2 - Esquema v5: `Preferencia_Profesor`, catálogo y huella (2026-10-29)

**Requisito:** RF-1, RF-6 (y RNF-2 en el rechazo sin migración). **Plan:** §1.2, §2.2, §3 y §10 (T2).
**Base:** `4993be0`.

### Rojo (tests primero)

```text
$ ctest --test-dir build-backend --output-on-failure -R '^(test_backend_version_esquema|test_backend_apertura_base_datos|test_esquema_cursos)$'
FAIL!  : …versionActual_esCinco() Actual: 4  Expected: 5
FAIL!  : …crearEsquema_creaCatalogoYFijaVersion() 'tablaExiste(… "Preferencia_Profesor")' returned FALSE
FAIL!  : …preferencia_check_rechazaDiaSentidoYHorasInvalidas() … returned FALSE
FAIL!  : …abrir_baseV4_fallaEsquemaSinMigrarNiBorrar() Actual: 1  Expected: Estado::FalloEsquema
```

### Verde

Primera pasada de la huella con marcador: imprimió la real.

```text
FAIL!  : …huellaEsquema_noCambiaSinSubirLaMarcaDeVersion()
   Actual   (huella)    : "eebe660c385f9802dc7b0b63abb6e7821db3ad3f2ec35982eeae55e49e9deaa0"
   Expected (registrada): "pendiente-de-copiar"
```

```text
$ ctest --test-dir build-backend --output-on-failure -R '^(test_backend_version_esquema|test_backend_apertura_base_datos|test_esquema_cursos)$'
1/3 Test #11: test_backend_version_esquema .......   Passed    0.06 sec
2/3 Test #12: test_esquema_cursos ................   Passed    0.02 sec
3/3 Test #14: test_backend_apertura_base_datos ...   Passed    0.04 sec
100% tests passed out of 3

$ ctest --preset backend
100% tests passed out of 53
```

### Cambios

- `src/backend/src/database/migracion.cpp`: DDL de `Preferencia_Profesor` después de
  `Disponibilidad_Profesor` (día TEXT 0–6, horas TEXT, `sentido` preferida/evitada,
  `CHECK(hora_inicio < hora_fin)` y `UNIQUE(id_Profesor, dia, hora_inicio, hora_fin, sentido)`).
- `version_esquema.hpp`: `VERSION_ESQUEMA_ACTUAL = 5` y comentario de la v5.
- `version_esquema.cpp`: `Preferencia_Profesor` en `TABLAS_ESPERADAS` y
  `COLUMNAS_PREFERENCIAS = {id, id_Profesor, dia, hora_inicio, hora_fin, sentido}`.
- Tests: versión 5 (renombrado), tabla/columnas, `CHECK` y `UNIQUE`, catálogo (tabla y columna
  faltantes), huella v5 copiada del fallo real, base v4 rechazada al abrir y comentario v4→v5 de
  `test_servicio_configuracion_horaria.cpp`.

### Notas

- **Huella v5:** `eebe660c385f9802dc7b0b63abb6e7821db3ad3f2ec35982eeae55e49e9deaa0`, copiada del
  `Actual` que imprimió el test con el DDL aplicado.
- **`CHECK` de texto:** funciona porque las horas se escriben `HH:mm` a dos dígitos y en 24 h
  (`QTime::toString("HH:mm")`), así que el orden lexicográfico coincide con el temporal. Una escritura
  manual con `'8:00'` compararía mal; el contrato de la columna es `HH:mm` y la validación de negocio
  (T1/T3) lo garantiza. El test usa los valores canónicos.
- La política sigue siendo **sin migración**: una base con `user_version = 4` se rechaza al abrir y
  conserva su marca.
- No se tocaron los servicios de preferencias, el DTO, la cascada, el seed, `docs/` ni el contrato.

## T3 - Servicio de preferencias: CRUD, validación y orden (2026-10-29)

**Requisito:** RF-1, RF-2, RF-3, RNF-1. **Plan:** §1.3, §2.3, §3 y §10 (T3). **Base:** `3871488`.

### Rojo (tests primero)

`test/backend/test_servicio_preferencias.cpp` (30 casos) registrado con `add_qtest(... Qt6::Sql)`; sin
el servicio el árbol no compila y el primer rojo real fue la regla de solape:

```text
$ ctest --test-dir build-backend --output-on-failure -R '^test_servicio_preferencias$'
FAIL!  : …listar_ordenaPorDiaInicioFinEId() 'agregar(… día 1, 8–10, preferida).ok' returned FALSE
Totals: 29 passed, 1 failed
```

### Verde

```text
$ ctest --test-dir build-backend --output-on-failure -R '^test_servicio_preferencias$'
1/1 Test #47: test_servicio_preferencias .......   Passed    0.07 sec
100% tests passed out of 1

$ ./build-backend/test/test_servicio_preferencias
Totals: 30 passed, 0 failed, 0 skipped, 0 blacklisted

$ ctest --preset backend
100% tests passed out of 54
```

### Cambios

- **Nuevo** `src/backend/include/backend/services/ServicioPreferencias.hpp`: `PreferenciaDTO` y los
  cinco métodos (`agregar`, `listar`, `actualizar`, `eliminar`, `limpiar`).
- **Nuevo** `src/backend/src/services/ServicioPreferencias.cpp`: validación con
  `franja_horaria::errorDeFranja` + `sentidoValido` + `SELECT` directo de `Profesores`; duplicado
  exacto y solape opuesto con `haySolape`; `listar` ordena por día, inicio, fin e id; `actualizar`
  compara contra el día nuevo excluyendo la propia fila; `eliminar` distingue «no existe»; `limpiar`
  borra solo `Preferencia_Profesor`.
- `src/backend/CMakeLists.txt` y `test/CMakeLists.txt`.

### Casos delicados del test (30 en total)

- `agregar_solapeDeSentidosOpuestos_fallaNombrandoLasDosFranjas`: el mensaje cita `08:00–10:00
  (preferida)`, `09:00–11:00 (evitada)`, el docente y la acción.
- `agregar_solapesDelMismoSentido_seAceptan` y `agregar_rangosContiguosDeSentidosOpuestos_seAceptan`.
- `actualizar_solapeOpuesto_rechazaYConservaElAnterior`: tras el rechazo, la fila conserva 14:00–16:00
  y su sentido.
- `actualizar_cambioDeDiaQueCreaSolapeOpuesto_rechaza`: la comprobación usa el **día nuevo**; la fila
  conserva el día 2.
- `limpiar_borraSoloPreferenciasYNoLaDisponibilidad`: borra 2 de P1, deja 1 de P2 y la disponibilidad
  intacta.
- `agregar_preferenciaQueCoincideConDisponibilidad_seAcepta`; días 0 y 6 aceptados; cada rechazo con
  valor y acción.

### Notas

- **Ajuste del test de orden:** la primera versión insertaba dos filas del mismo día y rango con
  sentidos opuestos para probar el desempate por id; eso **es un solape opuesto** y el servicio lo
  rechaza (correcto). El orden por id queda en el SQL como desempate estable, pero es inalcanzable con
  filas válidas: el duplicado exacto se rechaza y el único par con mismo día/inicio/fin no puede tener
  sentidos distintos por la regla de solape. El test ordena por día, inicio y fin.
- **Mensajes:** se usan los de §2.3; en `actualizar` el docente inexistente conserva su mensaje
  («…de la preferencia…»). Ningún rechazo queda sin valor y acción.
- No se tocaron el esquema, `ProfesorDTO`/`NucleoDatos`, la cascada, el seed, `ServicioGeneracion` ni
  `docs/`.

## T4 - `ProfesorDTO` con preferencias y fachada `NucleoDatos` (2026-10-29)

**Requisito:** RF-3. **Plan:** §1.4, §2.3 (fachada) y §10 (T4). **Base:** `7405351`.

### Rojo (tests primero)

Se añadieron los casos en `test_servicio_profesor` y `test_nucleo_datos`; sin los campos ni los métodos
el árbol no compila:

```text
$ cmake --build --preset backend --parallel
test_servicio_profesor.cpp: error: ‘struct ProfesorDTO’ has no member named ‘preferencias’
test_nucleo_datos.cpp: error: ‘class NucleoDatos’ has no member named ‘agregarPreferencia’
[…]
```

### Verde

```text
$ ctest --test-dir build-backend --output-on-failure -R '^(test_servicio_profesor|test_backend_nucleo_datos|test_servicio_preferencias)$'
100% tests passed out of 3

$ ./build-backend/test/test_servicio_profesor     → Totals: 56 passed
$ ./build-backend/test/test_backend_nucleo_datos  → Totals: 20 passed

$ ctest --preset backend
100% tests passed out of 54
```

### Cambios

- `ServicioProfesor.hpp`: `ProfesorDTO` gana `QVector<PreferenciaDTO> preferencias` (lista propia,
  junto a `disponibilidad`, **sin mezclarlas**) y `cargarPreferencias` queda en la sección privada,
  junto a `cargarDisponibilidad`/`cargarMaterias`.
- `ServicioProfesor.cpp`: `cargarPreferencias` delega en `ServicioPreferencias(m_db).listar(id)` (una
  sola consulta y un solo orden); `obtenerProfesor` y `listarProfesores` la llaman.
  `ProfesorDTO::toProfesor()` **no** la tiene en cuenta (el modelo del solver no cambia).
- `NucleoDatos.hpp` / `.cpp`: miembro `m_preferencias` y las cinco operaciones con `conPendiente`.
- Tests: preferencias separadas y ordenadas en `obtenerProfesor`/`listarProfesores`, `toProfesor` sin
  preferencias (mismo `toJson` antes y después), fachada completa y escritura fallida reintentable.

### Notas

- **Registro y reintento:** `agregarPreferencia`, `actualizarPreferencia`, `eliminarPreferencia` y
  `limpiarPreferencias` pasan por `conPendiente`; una escritura fallida se registra con dominio
  `preferencia` y clave (alta: `idProfesor|dia|inicio|fin|sentido`; edición/baja: `id`; limpieza:
  `idProfesor`) y la acción que repite la llamada. El test falla el alta con un docente inexistente,
  crea el docente y `reintentarPendiente` aplica el cambio y retira el pendiente.
- **Listas distintas:** una preferencia que coincide con la disponibilidad coexiste y no se mezcla
  (`disponibilidad` sigue con 1 y `preferencias` con 2 en el test).
- No se tocaron el esquema, la cascada, el seed, `ServicioGeneracion` ni `docs/`.

## T5 - Cascada y baja del docente (2026-10-29)

**Requisito:** RF-4, RNF-4. **Plan:** §1.5 y §10 (T5). **Base:** `d2f2147`.

### Rojo (tests primero, con la producción vieja)

Se añadieron los casos y, antes de la producción, el build con el código anterior dejó:

```text
$ ctest --test-dir build-backend --output-on-failure -R '^(test_backend_cascada|test_servicio_profesor|test_backend_nucleo_datos)$'
FAIL!  : TestNucleoDatos::docentes_altaModificacionBaja() 'eliminarDocente("P1")' returned FALSE
FAIL!  : TestCascada::dependenciasDe_describeRelacionesPorDominio() Compared values are not the same
FAIL!  : TestCascada::dependientesDe_profesor_listaRelaciones() Compared values are not the same
FAIL!  : TestCascada::eliminarConCascada_profesor_borraDisponibilidadYPreferencias() 'eliminarConCascada(…)'
FAIL!  : TestCascada::eliminarConCascada_profesor_atomico_falloDejaTodoIntacto() 'eliminarConCascada(…)'
FAIL!  : TestServicioProfesor::eliminarProfesor_borraRelaciones() 'eliminarProfesor("P-900")' returned FALSE
0% tests passed, 3 tests failed out of 3
```

Las bajas fallaban por **clave foránea**: las preferencias no se borraban (los dos caminos).

### Verde

```text
$ ctest --test-dir build-backend --output-on-failure -R '^(test_backend_cascada|test_servicio_profesor|test_backend_nucleo_datos|test_servicio_preferencias)$'
100% tests passed out of 4
$ ctest --preset backend
100% tests passed out of 54
```

### Cambios

- `cascada.cpp`: nueva relación del dominio `profesor` → tabla `Preferencia_Profesor`, columna
  `id_Profesor`, id `id`, etiqueta
  `sentido || ' ' || dia || ' ' || hora_inicio || '-' || hora_fin`, descripción «Preferencia horaria».
  `dependenciasDe("profesor").size()` pasa de 2 a **3**.
- `ServicioProfesor.cpp`: `eliminarProfesor` borra `Preferencia_Profesor` dentro de su transacción
  (antes de la disponibilidad y de las materias y del docente).
- Tests: `test_cascada` (tamaño 3, dependencia con etiqueta legible, borrado por `eliminarConCascada`
  y rollback con trigger), `test_servicio_profesor` (la baja borra también preferencias) y
  `test_nucleo_datos` (`eliminarDocente` arrastra preferencias).

### Notas

- **Dos caminos cubiertos:** `ServicioProfesor::eliminarProfesor` (lo usa `NucleoDatos::eliminarDocente`)
  y `NucleoDatos::eliminarConCascada`; ambos dejan 0 disponibilidades y 0 preferencias.
- **Etiqueta:** el listado de dependientes muestra «Preferencia horaria: evitada 2 14:00-16:00»
  (sentido, día y rango), así que el usuario ve qué confirma.
- **Rollback:** un trigger `BEFORE DELETE ON Profesores` aborta tras borrar las hijas; el test exige que
  docente, disponibilidad y preferencia sigan existiendo y que, retirado el trigger, la cascada
  funcione. (Con la producción vieja ese test también fallaba, por la FK al intentar borrar el docente.)
- No se tocaron el esquema, el seed, `ServicioGeneracion` ni `docs/`.

## T6 - Seed: recuento 0 y tabla de dominio (2026-10-29)

**Requisito:** RF-7. **Plan:** §1.6 y §10 (T6). **Base:** `8acce25`.

### Rojo (tests primero)

Los tests usaban `Recuentos::preferencias` antes de existir el campo; con la producción vieja el árbol
no compila:

```text
$ cmake --build --preset backend --parallel
test_seed_aplicacion.cpp:114:38: error: ‘const struct seed::Recuentos’ has no member named ‘preferencias’
test_seed_aplicacion.cpp:640:38: error: idem
test_seed_perfil.cpp:395:28: error: idem
test_seed_perfil.cpp:401:30: error: idem
```

### Verde

```text
$ ctest --test-dir build-backend --output-on-failure -R '^(test_seed_perfil|test_seed_aplicacion)$'
1/2 Test #49: test_seed_perfil .................   Passed    0.06 sec
2/2 Test #50: test_seed_aplicacion .............   Passed    0.54 sec
100% tests passed out of 2

$ ctest --preset backend
100% tests passed out of 54
```

Comprobación real de la CLI:

```text
$ gestor-horarios-seed --db …/centro.db
Hora académica: 35 min (configuraciones: 1)
Turnos: 2   Eventos: 2   Aulas: 16   Materias: 57   Planes: 4   Materias de plan: 210
Cursos: 13 (sin turno)   Docentes: 65   Materias asignadas: 65   Disponibilidades: 325   Tipos de aula: 3
Preferencias: 0
$ sqlite3 … "SELECT COUNT(*) FROM Preferencia_Profesor;" → 0
```

### Cambios

- `seed/perfil.hpp`: `Recuentos.preferencias` (0: el conjunto no las siembra).
- `seed/src/docentes.cpp`: `recuentosEsperados` deja `r.preferencias = 0` explícito.
- `seed/src/sembrador.cpp`: `Preferencia_Profesor` en `kTablasDominio` y comentario a **12 tablas /
  esquema v5**; `leerRecuentos` la cuenta; `recuentosCoinciden` la compara; `resumenTexto` imprime
  `Preferencias: 0`.
- Tests: recuento 0 en las cuatro variantes, tabla vacía tras sembrar, comparación de determinismo,
  lista de «12 tablas» del rollback y línea del resumen.

### Notas

- **Tabla de dominio, conjunto vacío:** `Preferencia_Profesor` cuenta para la política de base vacía,
  pero como el conjunto la deja a 0, una base recién creada (solo con la configuración) se sigue
  sembrando sin `--reemplazar`.
- **T7 pendiente:** el borrado/atomicidad de la preferencia en `--solo-docentes` y `--reemplazar` no se
  toca aquí; T6 es solo recuento y tabla de dominio.
- De paso se corrigió el formato de `leerRecuentos` (la declaración de `Recuentos r;` quedaba en la
  misma línea que la llave).

## T7 - Seed: borrado, atomicidad y determinismo (2026-10-29)

**Requisito:** RF-7, RNF-3, RNF-4. **Plan:** §1.6 y §10 (T7). **Base:** `3b8ba90`.

### Rojo (tests primero)

Con una preferencia preinsertada y la producción anterior:

```text
$ ctest --test-dir build-backend --output-on-failure -R '^test_seed_aplicacion$'
FAIL!  : …soloDocentes_conPreferenciaPreinsertada_borraSinFallarPorFK()
         'resultado.ok()' returned FALSE. (No se pudieron borrar los docentes para la conmutación de variante.)
FAIL!  : …reemplazoTotal_conPreferenciaPreinsertada_borraSinFallarPorFK()
         'resultado.ok()' returned FALSE. (No se pudieron borrar los datos previos para el reemplazo total.)
Totals: 18 passed, 2 failed
```

Eran claves foráneas: el docente se borraba antes que su preferencia.

### Verde

```text
$ ctest --test-dir build-backend --output-on-failure -R '^test_seed_aplicacion$'
1/1 Test #50: test_seed_aplicacion .............   Passed    0.62 sec
100% tests passed out of 1
$ ./build-backend/test/test_seed_aplicacion
Totals: 20 passed, 0 failed

$ ctest --preset backend
100% tests passed out of 54
```

### Cambios

- `seed/src/sembrador.cpp`:
  - **`--solo-docentes`:** borra `Preferencia_Profesor` → `Disponibilidad_Profesor` →
    `Profesor_Materia` → `Profesores` (preferencias y disponibilidad referencian al docente; el
    orden cambia solo para añadir la nueva tabla al principio).
  - **Reemplazo total:** `Eventos` → `Preferencia_Profesor` → `Disponibilidad_Profesor` →
    `Profesor_Materia` → `Cursos` → … → `Profesores` (la nueva tabla entra antes de sus padres).
- `test/backend/test_seed_aplicacion.cpp`: helper `preinsertarPreferencia` y tres casos nuevos; el
  comentario y la lista de «12 tablas» ya venían de T6 y siguen con `Preferencia_Profesor` incluida.

### Notas

- **Rollback real:** `falloSimulado_conPreferenciaPreinsertada_conservaLaVarianteYLaPreferencia`
  siembra la variante holgada, inserta un docente + preferencia, conmuta a suplentes con
  `simularFalloTrasDocentes` y exige que el resultado sea `FalloEscritura` y que la base siga con
  66 docentes (65 holgada + el preinsertado), 65 asignaciones, 325 disponibilidades y **1**
  preferencia. Sin el borrado ordenado, el fallo llegaría antes (FK) y sin rollback la preferencia
  habría desaparecido.
- **Determinismo:** la comparación de dos siembras ya incluye `Preferencia_Profesor` (T6); ambas
  quedan con 0 filas.
- No se tocaron el esquema, los servicios, el DTO, la cascada, `ServicioGeneracion` ni `docs/`.