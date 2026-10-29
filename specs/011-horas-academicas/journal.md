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

## T3 - Servicio de configuración y fachada en `NucleoDatos` (2026-10-29)

**Requisito:** RF-1, RF-2 (y RNF-1 en los mensajes, RNF-4 en el punto único de rango). **Plan:** §1.2,
§2.3, §4 y §9 (T3). **Base:** `11077f8`.

### Rojo (tests primero)

```text
$ cmake --build --preset backend --parallel
test_servicio_configuracion_horaria.cpp:11:10: fatal error:
backend/services/ServicioConfiguracionHoraria.hpp: No existe el fichero o el directorio
test_nucleo_datos.cpp:324:40: error: 'class NucleoDatos' has no member named 'configuracionHoraria'
test_nucleo_datos.cpp:329:41: error: 'class NucleoDatos' has no member named 'guardarConfiguracionHoraria'
```

### Verde

```text
$ ctest --test-dir build-backend --output-on-failure -R '^(test_servicio_configuracion_horaria|test_backend_nucleo_datos)$'
1/2 Test #15: test_backend_nucleo_datos .............   Passed    0.06 sec
2/2 Test #45: test_servicio_configuracion_horaria ...   Passed    0.02 sec
100% tests passed out of 2

$ ./build-backend/test/test_servicio_configuracion_horaria
PASS   : TestServicioConfiguracionHoraria::consultar_sinFila_devuelve35PorDefectoSinError()
PASS   : TestServicioConfiguracionHoraria::consultar_conFila_devuelveElValorGuardadoYNoEsDefecto()
PASS   : TestServicioConfiguracionHoraria::guardar_valido_haceUpsertDeUnaSolaFila()
PASS   : TestServicioConfiguracionHoraria::guardar_invalido_fallaConRangoYConservaElAnterior()
PASS   : TestServicioConfiguracionHoraria::guardar_sinFila_creaLaFila()
PASS   : TestServicioConfiguracionHoraria::guardar_invalidoSinFila_noEscribeNiCrea()
Totals: 8 passed, 0 failed, 0 skipped, 0 blacklisted

$ grep -rn "ConfiguracionHoraria" src/backend | grep -vE "version_esquema|migracion|ServicioConfiguracionHoraria|NucleoDatos"
(vacío; solo quedan el catálogo, el DDL, el servicio y el tipo en la fachada)

$ ctest --preset backend
52/52 Test #52: test_ipc_aislamiento_bd ...............   Passed    0.07 sec
100% tests passed out of 52
```

### Cambios

- **Nuevo** `src/backend/include/backend/services/ServicioConfiguracionHoraria.hpp`: DTO
  `ConfiguracionHoraria { int duracionMinutos; bool porDefecto; }`, `consultar()` y `guardar(int)`.
- **Nuevo** `src/backend/src/services/ServicioConfiguracionHoraria.cpp`: `consultar()` devuelve el
  valor de la fila o `{POR_DEFECTO, true}` sin fila (sin error); `guardar()` valida con
  `duracion_horaria::errorDeRango`, no escribe si falla y hace upsert
  (`ON CONFLICT(id) DO UPDATE`) de la fila `id = 1`, sin abrir transacción propia.
- `src/backend/include/backend/services/NucleoDatos.hpp` / `.cpp`: fachada
  `configuracionHoraria()` / `guardarConfiguracionHoraria(int)` y miembro `m_configuracion`; **sin**
  `GestorPendientes` (la configuración es global, plan §1.2).
- `src/backend/CMakeLists.txt`: `ServicioConfiguracionHoraria.cpp` en el target `backend`.
- **Nuevo** `test/backend/test_servicio_configuracion_horaria.cpp` (6 casos) y caso de fachada en
  `test/backend/test_nucleo_datos.cpp`.

### Notas

- **Registro como `add_qtest`, no `add_gtest` (desviación de la instrucción de la fase, no del plan).**
  El plan §1.10 fija `add_qtest(... backend Qt6::Sql)` y todos los tests de servicios con base usan
  Qt Test. La primera versión como gtest **segfaulta** en `QSqlDatabase::open()`: Qt6 SQL necesita un
  `QCoreApplication` que gtest no crea. Prueba mínima:

  ```text
  $ ./sqltest            # QSqlDatabase::open() sin QCoreApplication
  sin app EXIT=139
  $ ./sqltest app
  app=1 open=1
  ```

  Con Qt Test (`QTEST_MAIN`) hay `QCoreApplication` y el binario pasa. El resto de la tarea no cambia.
- **Modelo de `consultar()`:** `Resultado<ConfiguracionHoraria>` con el DTO `{int duracionMinutos;
  bool porDefecto;}`. `Resultado<T>` (backend/resultado.hpp) no distingue «vacío» de «valor», así que la
  ausencia de fila se modela como éxito con `porDefecto = true`, no como error.
- **Upsert:** `INSERT ... ON CONFLICT(id) DO UPDATE SET duracion_minutos = excluded.duracion_minutos,
  fecha_modificacion = CURRENT_TIMESTAMP` (SQLite 3.54). Una sola sentencia, como pide §2.3, para
  participar de la transacción del llamador (el seed de T8).
- La fachada no registra pendientes: una configuración inválida no es un registro reintentable.
- No se tocaron `calculo_eventos`, el constructor, el seed, el contrato, `duracion_horaria`, el DDL ni
  `docs/`.
