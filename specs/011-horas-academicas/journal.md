# Journal 011 - Duración de la hora académica

## T1 - Punto único de duración: rango 30–90 y horas académicas → minutos (2026-10-29)

**Requisito:** RF-1, RF-3, RNF-4 (y RNF-1/NFR-3 en los mensajes y el determinismo). **Plan:** §1.1,
§2.1, §9 (T1) y §10. **Base:** `3a840e6` (HEAD de `feat/Base-datos`, árbol limpio, 50/50).

### Rojo (tests primero)

`test/backend/test_duracion_horaria.cpp` con los casos del «Hecho cuando» y los bordes de §2.1, y
`add_gtest(test_duracion_horaria ... backend)` en `test/CMakeLists.txt`. Sin el módulo, el árbol no
compila:

```text
$ cmake --build --preset backend --parallel
/home/luis/Repos/Gestor-Horarios/test/backend/test_duracion_horaria.cpp:5:10: fatal error:
backend/services/duracion_horaria.hpp: No existe el fichero o el directorio
    5 | #include "backend/services/duracion_horaria.hpp"
      |          ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
compilation terminated.
ninja: build stopped: subcommand failed.
```

### Verde

```text
$ cmake --preset backend && cmake --build --preset backend --parallel
[54/54] Linking CXX executable test/test_ipc_aislamiento_bd        # exit 0

$ ctest --test-dir build-backend --output-on-failure -R '^test_duracion_horaria$'
1/1 Test #44: test_duracion_horaria ............   Passed    0.01 sec
100% tests passed out of 1

$ ./build-backend/test/test_duracion_horaria
[----------] 17 tests from TestDuracionHoraria (0 ms total)
[  PASSED  ] 17 tests.

$ ctest --preset backend
51/51 Test #51: test_ipc_aislamiento_bd ............   Passed    0.07 sec
100% tests passed out of 51
```

Casos cubiertos: 30/35/90 aceptan; 29/91/5 rechazan con el rango, el valor y la acción
«Ajuste la duración a un valor del rango»; `minutosDeHorasAcademicas(40, 35) == 1400` y
`(40, 30) == 1200` (más 0 horas, horas negativas y D inválida → 0); `franjasUtilizables(290, 10, 45) == 6`,
`(290, 10, 35) == 8` y `(150, 10, 35) == 4` (más sin franja completa, ventana ≤ 0, eventos > ventana y
D ≤ 0 → 0); `deficitDeEncaje(8, 290, 10, 45) == 80`, `(5, 150, 10, 35) == 35` y `(8, 370, 10, 35) == 0`.

### Cambios

- **Nuevo** `src/backend/include/backend/services/duracion_horaria.hpp` (namespace
  `duracion_horaria`: `POR_DEFECTO = 35`, `MINIMA = 30`, `MAXIMA = 90`, `errorDeRango`,
  `minutosDeHorasAcademicas`, `franjasUtilizables`, `deficitDeEncaje`).
- **Nuevo** `src/backend/src/services/duracion_horaria.cpp`: funciones puras según plan §2.1, sin
  `QSql*` ni reloj.
- `src/backend/CMakeLists.txt`: `src/services/duracion_horaria.cpp` en el target `backend`.
- **Nuevo** `test/backend/test_duracion_horaria.cpp`, registrado con `add_gtest` (lógica pura, sin
  `Qt6::Sql`).

### Notas

- **Registro como `add_gtest`, no `add_qtest`.** El plan §1.10 dice `add_qtest(... backend)`; la tarea
  aprobada (T1) no fija el macro y `AGENTS.md` reserva Google Test para la lógica pura. Se usa
  `add_gtest`, como `test_capacidad_tipo` (misma naturaleza): el binario y su verificación no cambian.
- **D fuera de rango en `franjasUtilizables`/`deficitDeEncaje`:** el plan §2.1 solo guarda `D <= 0`;
  el rango 30–90 es responsabilidad de `errorDeRango` (lo aplica el llamador, p. ej. `errorDeEncaje` en
  T4). No se duplica la validación aquí (RNF-4). Con `D = 5` las cuentas son aritméticamente correctas
  pero ningún consumidor debe llegar con ese valor sin haber validado.
- **`ventana ≤ eventos`:** `franjasUtilizables` devuelve 0 (no hay franja completa) y
  `deficitDeEncaje` devuelve el exceso positivo; con ventana ≤ 0 ambas devuelven 0.
- `minutosDeHorasAcademicas` valida D con `errorDeRango`: con D fuera de 30–90 devuelve 0.
- No se tocó `calculo_eventos.*`, `solver_config.cpp`, el constructor, el seed, el esquema ni `docs/`.
