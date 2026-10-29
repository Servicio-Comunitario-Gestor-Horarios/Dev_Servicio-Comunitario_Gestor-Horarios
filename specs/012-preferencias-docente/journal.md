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