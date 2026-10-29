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

## T2 - Esquema v4: `ConfiguracionHoraria`, catálogo y huella (2026-10-29)

**Requisito:** RF-1, RF-2, RF-7 (y RNF-2 en el rechazo sin migración). **Plan:** §1.3, §2.2 (implícito),
§3 y §9 (T2). **Base:** `fc86368`.

### Rojo (tests primero)

`test_version_esquema` y `test_apertura_base_datos` se escribieron antes del DDL:

```text
$ ctest --test-dir build-backend --output-on-failure -R '^(test_backend_version_esquema|test_backend_apertura_base_datos)$'
[...]
Totals: 21 passed, 6 failed, 0 skipped, 0 blacklisted   # version_esquema
FAIL!  : TestAperturaBaseDatos::abrir_baseV3_fallaEsquemaSinMigrarNiBorrar()
   Actual   (resultado.estado)    : 1      # OkAbierta con VERSION_ESQUEMA_ACTUAL = 3
   Expected (Estado::FalloEsquema): 2
```

Casos: versión 4; tabla/columnas en el catálogo; fila `(1, 35)` única e idempotente; `INSERT id = 2`
rechazado por `CHECK(id = 1)`; `UPDATE` a 29 y 91 rechazados por el `CHECK` del rango; una base con
`user_version = 3` se rechaza al abrir y conserva su marca.

### Verde

Primera pasada con la huella como marcador: el test imprimió la huella real.

```text
FAIL!  : TestVersionEsquema::huellaEsquema_noCambiaSinSubirLaMarcaDeVersion()
   Actual   (QString::fromLatin1(huella))    : "f630cc2547e21454b8273fa255124142138e254bfa8b2a580c52bf8b051f7101"
   Expected (QString::fromLatin1(registrada)): "pendiente-de-copiar"
```

Copiada a `HUELLAS_CONOCIDAS`:

```text
$ ctest --test-dir build-backend --output-on-failure -R '^(test_backend_version_esquema|test_backend_apertura_base_datos|test_esquema_cursos)$'
1/3 Test #11: test_backend_version_esquema .......   Passed    0.05 sec
2/3 Test #12: test_esquema_cursos ................   Passed    0.02 sec
3/3 Test #14: test_backend_apertura_base_datos ...   Passed    0.04 sec
100% tests passed out of 3

$ ctest --preset backend
51/51 Test #51: test_ipc_aislamiento_bd ............   Passed    0.08 sec
100% tests passed out of 51
```

La suite sigue en **51/51**: T2 añade casos a binarios existentes, no un binario nuevo (eso es T3).

### Cambios

- `src/backend/src/database/migracion.cpp`: DDL de `ConfiguracionHoraria` (antes de `Cursos`) con
  `CHECK(id = 1)` y `CHECK(duracion_minutos BETWEEN 30 AND 90)`, más
  `INSERT OR IGNORE ... VALUES (1, 35)`.
- `src/backend/include/backend/database/version_esquema.hpp`: `VERSION_ESQUEMA_ACTUAL = 4` y comentario
  de la v4.
- `src/backend/src/database/version_esquema.cpp`: `ConfiguracionHoraria` en `TABLAS_ESPERADAS` y
  `COLUMNAS_CONFIGURACION = {id, duracion_minutos}` en `TABLAS_CON_COLUMNAS`.
- `test/backend/test_version_esquema.cpp`: versión 4, catálogo, fila por defecto, `CHECK`s y huella v4
  copiada del fallo real (no inventada).
- `test/backend/test_apertura_base_datos.cpp`: caso explícito de base con marca 3 → `FalloEsquema` sin
  migrar ni reescribir la marca.

### Notas

- **Huella v4:** `f630cc2547e21454b8273fa255124142138e254bfa8b2a580c52bf8b051f7101`, copiada del
  `Actual` que imprimió el test con el DDL ya aplicado.
- **`INSERT OR IGNORE` y no `INSERT`:** `crearEsquema` es idempotente y se vuelve a llamar en tests y en
  el flujo de catálogo (`esquemaEsperado_explicaElMotivo`), así que la fila por defecto no debe duplicar
  ni fallar.
- **Sin migración:** una base con `user_version = 3` se rechaza al abrir (`FalloEsquema`) y conserva su
  marca; es el comportamiento esperado de la 009 y queda documentado en el test.
- No se tocaron servicios, `NucleoDatos`, `calculo_eventos`, el constructor, el seed, el contrato,
  `duracion_horaria` ni `docs/`.
