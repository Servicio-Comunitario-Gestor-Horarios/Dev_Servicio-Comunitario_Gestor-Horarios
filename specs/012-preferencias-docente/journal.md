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