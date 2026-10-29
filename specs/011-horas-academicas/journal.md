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

## T4 - Franja = D y encaje: funciones puras nuevas, con el legado temporal (2026-10-29)

**Requisito:** RF-4 (y RNF-1/RNF-4 en mensajes y punto único). **Plan:** §1.4, §2.2, §9 (T4),
decisiones 6 y 12. **Base:** `4262b49`.

### Rojo (tests primero)

Se añadieron los casos nuevos a `test_calculo_eventos` conservando los antiguos; el árbol no compila
hasta que existen las firmas nuevas:

```text
$ cmake --build --preset backend --parallel
test_calculo_eventos.cpp:117:18: error: ‘minutosDeVentana’ no se declaró en este ámbito
test_calculo_eventos.cpp:129:18: error: ‘franjasUtilizables’ no se declaró en este ámbito
test_calculo_eventos.cpp:151:18: error: ‘deficitDeEncaje’ no se declaró en este ámbito
[...]
```

### Verde

```text
$ ctest --test-dir build-backend --output-on-failure -R '^test_calculo_eventos$'
1/1 Test #27: test_calculo_eventos .............   Passed    0.01 sec
100% tests passed out of 1

$ ./build-backend/test/test_calculo_eventos
Totals: 42 passed, 0 failed, 0 skipped, 0 blacklisted

$ ctest --preset backend
52/52 Test #52: test_ipc_aislamiento_bd ...............   Passed    0.08 sec
100% tests passed out of 52
```

Casos nuevos: `minutosDeVentana`/`tiempoDeEventos`; `franjasUtilizables` 8/6 con 290/10 y 8 con 35,
4 con 150/5 y 0 si D ≤ 0 o sin ventana; `deficitDeEncaje` 80 (45), 0 (cabe justo y sobrante), 35;
`errorDeEncaje` con turno + «45» + «déficit 80», vacío al caber, «déficit 35», turno sin ventana con
«Declare sus horas» y D fuera de rango con el mensaje de `duracion_horaria`; `ventanaDePosicion` con
D = 35 → posición 2, 08:10–08:20, la última en el fin del turno e inválidas fuera de rango/sin D/sin
ventana; `eventoDentroDeVentana` con D y sin horas; y las dos pruebas de delegación del legado.

### Cambios

- `calculo_eventos.hpp` / `.cpp`:
  - **Nuevas:** `franjasUtilizables(turno, D)`, `deficitDeEncaje(turno, D)`, `errorDeEncaje(turno, D)`,
    `ventanaDePosicion(turno, posicion, D)`, `eventoDentroDeVentana(evento, turno, posicion, D)`.
  - Se hacen públicas `minutosDeVentana(turno)` y `tiempoDeEventos(turno)`.
  - `duracionFranjaEstimada` conserva su fórmula; las firmas antiguas de `ventanaDePosicion` (2 args) y
    `eventoDentroDeVentana` (3 args) son **envoltorios** que delegan en las nuevas con
    `D = duracionFranjaEstimada(turno)`, marcados como legado temporal que se retira en T9.
- `test/backend/test_calculo_eventos.cpp`: casos nuevos; los antiguos intactos.

### Notas

- **Sin duplicar aritmética:** `franjasUtilizables` y `deficitDeEncaje` delegan en `duracion_horaria`;
  `errorDeEncaje` obtiene el rango de `duracion_horaria::errorDeRango` (RNF-4).
- **Orden de `errorDeEncaje`:** primero rango de D, después ventana ausente, después el encaje (plan
  §2.2). Un turno sin ventana devuelve el mensaje con la acción aunque D sea válida.
- **Expectativa corregida en el test:** la ventana de la posición *k* empieza en `inicio + k×D`
  (N5 de la 009), así que la posición 1 de 07:00–11:50 con D = 35 empieza a las 07:35, no a las 07:00.
  El error era del test, no del código.
- No se tocaron `ServicioTurnosEventos.cpp`, `solver_config.cpp`, `perfil.cpp`, el constructor, el seed,
  `duracion_horaria`, el esquema ni `docs/`: son T5/T6/T7/T8/T10. El `grep` de `duracionFranjaEstimada`
  sigue con resultados a propósito (T9).

## T5 - Servicio de turnos: encaje con la D efectiva en turnos y eventos (2026-10-29)

**Requisito:** RF-4 (y RNF-1/RNF-4). **Plan:** §1.4, §2.2, §8 decisión 4 y §9 (T5). **Base:** `36c28b2`.

### Rojo (tests primero)

Se adaptaron los casos temporales de `test_servicio_turnos_eventos` a D = 35 (ventana de la posición
1: 07:35–07:45; posición 2: 08:10–08:20) y se añadieron los de encaje con la configuración. Contra el
servicio que aún usa la franja estimada:

```text
$ ctest --test-dir build-backend --output-on-failure -R '^(test_servicio_turnos_eventos|test_backend_nucleo_datos|test_backend_cascada)$'
FAIL!  : TestServicioTurnosEventos::agregarEvento_enLaVentanaDeSuPosicion_aceptado()
         'r.ok' returned FALSE. (El intervalo 07:35–07:45 del evento «Receso» no cabe en la
         ventana de la posición 1 (07:58–08:08).)
FAIL!  : TestServicioTurnosEventos::crearTurno_conDuracion45YVentana290_rechazaConDeficit70()
FAIL!  : TestNucleoDatos::turnosEventos_altaModificacionBaja() (ventana 15:34–15:49 del reparto)
[...]
Totals: 31 passed, 11 failed          # test_servicio_turnos_eventos
```

### Verde

```text
$ ctest --test-dir build-backend --output-on-failure -R '^(test_servicio_turnos_eventos|test_backend_nucleo_datos|test_backend_cascada)$'
1/3 Test #15: test_backend_nucleo_datos ........   Passed    0.07 sec
2/3 Test #16: test_backend_cascada .............   Passed    0.07 sec
3/3 Test #26: test_servicio_turnos_eventos .....   Passed    0.12 sec
100% tests passed out of 3
```

### Cambios

- `ServicioTurnosEventos.hpp` / `.cpp`:
  - `duracionEfectiva()` privada: consulta `ServicioConfiguracionHoraria(m_db).consultar()` en cada
    operación que valida (sin caché ni global); si falla, 35 y un `qWarning`.
  - `crearTurno` y `actualizarTurno` comprueban `errorDeEncaje(turno, D)` **antes de escribir**; en la
    actualización sustituye la antigua comprobación de «reparto en positivo».
  - `validarCoherenciaTemporal(..., D, ...)` valida el encaje del turno con el candidato dentro y usa
    `ventanaDePosicion`/`eventoDentroDeVentana` con D.
  - Las seis llamadas antiguas (`:114, :122, :124, :315, :330, :334`) quedan migradas; el servicio ya
    no llama a `duracionFranjaEstimada`.
- `test/backend/test_servicio_turnos_eventos.cpp`: casos temporales a D = 35 y seis casos nuevos de
  encaje (8 franjas/290 con 35, 5 franjas/5 h, D = 45 → déficit 70, evento con D = 45 → déficit 80,
  actualización de turno y de evento rechazadas sin escribir, y comparación literal con
  `errorDeEncaje` para probar que el mensaje es único).
- `test/backend/test_nucleo_datos.cpp`: el evento de la fachada pasa a la rejilla de 35
  (15:10–15:25) y el comentario deja de hablar del reparto.

### Notas y hallazgo

- **D se inyecta por consulta, no por constructor** (decisión 4 del plan): cada operación de escritura
  lee la D efectiva una vez y la pasa a los validadores; no hay estado que se desincronice entre la
  fachada, la consola y los tests.
- **No se valida al eliminar un evento:** borrar no puede romper la invariante, `eliminarEvento`
  devuelve `bool` sin mensaje y bloquearlo impediría corregir datos tras subir D. `tasks.md` y el plan
  solo piden alta/edición; se sigue esa lectura.
- **`test_seed_aplicacion` queda en rojo (9 casos) hasta T8.** El perfil del seed aún declara el
  inventario de 40 min (turno 07:00–12:30, receso 08:20–08:30); con D = 35 la ventana de la posición 2
  es 08:10–08:20 y el alta se rechaza. Es exactamente lo que T8 migra (07:00–11:50, receso
  08:10–08:20). El `-R` de T5 no lo incluye y `perfil.cpp` está fuera de su alcance; el árbol compila
  y las tres suites de la tarea están en verde. `ctest --preset backend` da **51/52** por ese motivo.

## T8 - Seed: turnos con D = 35, configuración asegurada y resultado (2026-10-29)

**Requisito:** RF-8 (y RF-2/RF-4 para configuración y encaje). **Plan:** §1.8, §6 y §9 (T8). **Base:**
`bc7122b`. **Tarea adelantada** por decisión del usuario tras T5: su servicio ya valida con D = 35 y el
perfil del seed seguía con el inventario de 40 min (`test_seed_aplicacion` en rojo, 51/52).

### Rojo (tests primero)

Expectativas nuevas de perfil y de siembra contra el seed antiguo:

```text
$ cmake --build --preset backend --parallel
test_seed_aplicacion.cpp:115:28: error: ‘const struct seed::ResultadoAplicacion’ has no member
named ‘duracionMinutos’
test_seed_aplicacion.cpp:630:45: error: demasiados argumentos para la función
‘QString seed::resumenTexto(const Perfil&, VarianteDocentes, const Recuentos&)’
```

Con las estructuras ya ampliadas, la siembra seguía fallando por el perfil de 40 min (ventana de la
posición 2 = 08:10–08:20 con D = 35).

### Verde

```text
$ ctest --test-dir build-backend --output-on-failure -R '^(test_seed_perfil|test_seed_aplicacion)$'
1/2 Test #47: test_seed_perfil .................   Passed    0.05 sec
2/2 Test #48: test_seed_aplicacion .............   Passed    0.44 sec
100% tests passed out of 2

$ ctest --preset backend
52/52 Test #52: test_ipc_aislamiento_bd ...............   Passed    0.08 sec
100% tests passed out of 52
```

Comprobación real de la CLI y la base:

```text
$ gestor-horarios-seed --db …/centro.db
Variante: holgada
Hora académica: 35 min (configuraciones: 1)
…
1|35
Mañana|07:00|11:50|8
Tarde|12:40|17:30|8
Mañana|08:10|08:20
Tarde|13:50|14:00
```

### Cambios

- `src/seed/src/perfil.cpp`:
  - Turnos: Mañana 07:00–11:50 y Tarde 12:40–17:30, 8 franjas, recesos 08:10–08:20 y 13:50–14:00;
    comentario con `8 × 35 = 280` de rejilla y `8 × 35 + 10 = 290` de ventana.
  - `validarPerfil`: ventanas canónicas nuevas, `eventoDentroDeVentana`/`ventanaDePosicion` con
    `duracion_horaria::POR_DEFECTO` y comprobación de encaje con `errorDeEncaje(turno, 35)` (tras los
    errores propios de cada evento, para no eclipsarlos). Un turno que no encaja se rechaza.
- `src/seed/include/seed/sembrador.hpp` / `src/seed/src/sembrador.cpp`:
  - `ResultadoAplicacion` gana `duracionMinutos` y `configuraciones`.
  - `resumenTexto` recibe ambos y emite `Hora académica: 35 min (configuraciones: 1)`.
  - `aplicar` asegura la configuración con `NucleoDatos::guardarConfiguracionHoraria(35)` **dentro
    de la transacción y antes de crear turnos**, en las dos ramas (`--reemplazar` y `--solo-docentes`);
    al final `leerConfiguracionSembrada` lee valor y recuento de la base.
  - `kTablasDominio` sigue **sin** `ConfiguracionHoraria`: no bloquea una base recién creada.
- `src/seed/src/main.cpp`: pasa los dos valores al informe.
- `test/backend/test_seed_perfil.cpp` y `test/backend/test_seed_aplicacion.cpp`: rejilla nueva, encaje y
  turnos, configuración única, siembra sobre base solo con configuración, determinismo con
  `ConfiguracionHoraria`, `--reemplazar`/`--solo-docentes`, rollback y resumen.

### Notas

- **Turnos/eventos:** Mañana 07:00–11:50 (rejilla 07:00–11:40, receso 08:10–08:20, `fin` = 11:40 + 10) y
  Tarde 12:40–17:30 (rejilla 12:40–17:20, receso 13:50–14:00, `fin` = 17:20 + 10).
  `8 × 35 + 10 = 290` en ambos: encaje exacto, sobrante 0.
- **Rollback:** la fila `(1, 35)` la crea el esquema **fuera** de la transacción de la siembra; tras un
  fallo queda 1 fila con 35 (no 0). El upsert de la siembra es idempotente y no duplica.
- **Encaje en `validarPerfil`:** se comprueba después de los errores de cada evento para que una
  posición/orden/duración incoherente se informe como tal; el caso nuevo de déficit usa un evento extra
  de 20 min (`8 × 35 + 10 + 20 = 310 > 290`, «déficit 20»).
- No se tocaron `calculo_eventos` (legado vivo), `solver_config.cpp`, el constructor, el esquema,
  `duracion_horaria` ni `docs/`.

## T6 - Contrato 2.0: rango 30–90, no entero y encaje (2026-10-29)

**Requisito:** RF-5 (y RF-1). **Plan:** §1.5, §2.2, §5, §9 (T6) y decisión 11. **Base:** `c913d87`.

### Rojo (tests primero)

Nuevos casos de V2 (rango 1/5/29/91, válidos 30/35/45/90), V1 (`35.5`), encaje del turno sin eventos,
y recálculo de las horas de evento a la rejilla de 45 min. Contra la regla `> 0` y la franja estimada:

```text
$ ctest --test-dir build-backend --output-on-failure -R '^(test_validador_entrada|test_cargador_configuracion|test_contrato_solver)$'
FAIL!  : …v2_eventoConHorarioValido_pasa() … (V2: el intervalo 08:30–08:40 … (08:36–08:46).)
FAIL!  : …v2_duracionMinutosFueraDeRango_falla() '!resultado.ok' returned FALSE. (1)
FAIL!  : …v1_duracionMinutosNoEntera_falla() … (V2: duracion_minutos debe ser mayor que 0.)
FAIL!  : …v2_turnoQueNoEncaja_falla() '!resultado.ok' returned FALSE. ()
Totals: 52 passed, 6 failed
```

El caso de `35.5` mostró el problema real: el parseo la convierte en **0** y V2 decía «mayor que 0».

### Verde

```text
$ ctest --test-dir build-backend --output-on-failure -R '^(test_validador_entrada|test_cargador_configuracion|test_contrato_solver)$'
100% tests passed out of 3
$ ctest --preset backend
52/52 Test #52: test_ipc_aislamiento_bd ...............   Passed    0.07 sec
100% tests passed out of 52
```

### Cambios

- `solver_config.cpp`:
  - **V1:** si `franja_horaria.duracion_minutos` existe y no es un número entero, se rechaza citando el
    valor (`V1: duracion_minutos debe ser un entero; se recibió «35.5».`).
  - **V2:** el rango sale de `duracion_horaria::errorDeRango` (punto único) y se prefija `V2: `.
  - **V2 (turnos):** todos los turnos con ventana se validan con `errorDeEncaje(turno, D)` del propio
    contrato, aunque no tengan eventos; se elimina la comprobación con `duracionFranjaEstimada` y las
    llamadas de ventana de posición pasan a las firmas con D. Comentario de la función actualizado.
- `test/backend/test_validador_entrada.cpp`: casos nuevos y recálculo de fixtures a D = 45.
- `test/backend/test_contrato_solver.cpp`: solo el comentario de la ventana (47 → 45 min); los datos
  ya encajaban (`6 × 45 + 15 = 285 ≤ 300`).
- `test/backend/test_constructor_entrada.cpp`: el evento del dominio pasa de 07:48–07:58 a
  **07:45–07:55** (rejilla de 45 min), porque V2 ya rechaza el reparto antiguo. T7 lo llevará a la
  rejilla de 35 (07:35–07:45) al cambiar la firma del constructor.

### Notas

- **Fixtures ajustados con aritmética:**
  - `conFranjasUtiles40`: 8 franjas × 45 = 360 → la ventana del turno pasa a 07:00–13:00 (antes 300).
  - Eventos del turno 07:00–12:00 con D = 45: la posición 2 pasa de 08:36–08:46 a **08:30–08:40**;
    solapes y cronología se recalculan sobre 08:30 (6+6 y 4+5).
  - Duración 90: 6 × 90 = 540 → ventana 07:00–16:00 para el caso «pasa».
- `configValida()` del cargador (45, 6 franjas, evento de 30 min en 08:30–09:00) sigue válida:
  `6 × 45 + 30 = 300` = ventana.
- No se relajó ninguna aserción: el antiguo `v2_duracionMinutosUno_pasa` se sustituye por el rechazo
  de 1, que es justo la regla nueva.
- No se tocaron `calculo_eventos` (legado vivo), el constructor, el seed, el esquema, `duracion_horaria`
  ni `docs/`.

## T7 - Constructor con D y huella de datos con D (2026-10-29)

**Requisito:** RF-5, RF-3 y RF-6 (y RNF-1 en los mensajes). **Plan:** §1.5, §1.6, §1.7, §2.4, §2.5 y
§9 (T7). **Base:** `fbb1f68`.

### Rojo (tests primero)

Las llamadas de los tests se pasaron a la firma con D y se añadieron los casos de sobrescritura, rango y
huella. Sin la firma nueva el árbol no compila:

```text
$ cmake --build --preset backend --parallel
test_validador_entrada.cpp:85:78: error: invalid initialization of reference of type
‘const QDateTime&’ from expression of type ‘int’
test_servicio_generacion.cpp:267:39: error: demasiados argumentos para la función
‘QString huellaDatos(const DatosDominio&)’
```

### Verde

```text
$ ctest --test-dir build-backend --output-on-failure -R '^(test_constructor_entrada|test_validador_entrada|test_servicio_generacion|test_ipc_aislamiento_bd)$'
100% tests passed out of 4

$ ctest --preset backend
52/52 Test #52: test_ipc_aislamiento_bd ...............   Passed    0.08 sec
100% tests passed out of 52
```

La huella **no** se movió a T9: cupo en la tarea.

### Cambios

- `ConstructorEntradaSolver.hpp` / `.cpp`:
  - Firma obligatoria `construirJsonEntrada(datos, parametros, duracionMinutos, ahora)` (y la fachada
    `ConstructorEntradaSolver::construir(..., duracionMinutos, ...)`), **sin** sobrecarga antigua.
  - `salida.franja_horaria = parametros.franja_horaria` y **sobrescritura incondicional** de
    `duracion_minutos` con la D recibida; la validación con `duracion_horaria::errorDeRango` va
    **después** de la sobrescritura y antes de emitir JSON.
  - `slotsDeFranja` usa la D efectiva de la salida, no la del preset.
- `ServicioGeneracion.hpp` / `.cpp`: `huellaDatos(datos, duracionMinutos)` incluye `dur:<D>;` en el
  resumen canónico; el resto del servicio no cambia (el llamador construye la huella con la D efectiva).
- Tests: `test_constructor_entrada` (todas las llamadas + fachada), `test_validador_entrada`,
  `test_servicio_generacion` (`entradaValida` y todas las `huellaDatos`) y
  `test/test_ipc_aislamiento_bd`.

### Notas

- **Sobrescritura + validación:** el orden es tipos → `salida.franja_horaria = preset` → `duracion = D`
  → `errorDeRango` → rejilla. Un preset con 0, 5 o 45 produce siempre la salida con 35 (probado con
  los tres); `construirJsonEntrada(..., 5, ...)` devuelve error con el rango y la acción y **no** emite
  JSON (`resultado.valor.isEmpty()`).
- **Disponibilidad:** 07:00–09:00 con D = 35 → slots 0, 1 y 2 (antes 2 con 45).
- **Fixture de eventos a 35:** el constructor conserva el evento del dominio (07:35–07:45) y V2 lo
  acepta con la franja de 35; los eventos de `test_validador_entrada` se recalculan de 08:30 a **08:10**
  (ventana de la posición 2: 08:10–08:20) y el déficit del turno sin encaje pasa a **180**
  (`6 × 35 − 30`). `conFranjasUtiles40` vuelve a 07:00–11:40 (`8 × 35 = 280`).
- **Frontend:** `src/frontend/src/services/entrada_solver_builder.cpp:165` sigue con la firma antigua;
  el preset `backend` no lo compila y queda como deuda documentada (T10, RF-9). No se tocó.
- No se tocaron `calculo_eventos` (legado vivo), `solver_config.cpp`, el seed, el esquema,
  `duracion_horaria` ni `docs/`.

## T9 - Limpieza del legado y cierre del punto único (2026-10-29)

**Requisito:** RF-4, RNF-4. **Plan:** §1.4 (T9), §2.2 y §9 (T9). **Base:** `15db0d2`.

### Cierre (el build es la prueba de aridad)

Se comprobó antes de borrar que ningún consumidor seguía usando las firmas antiguas
(`ServicioTurnosEventos`, `solver_config` y `perfil` ya llamaban con D desde T5/T6/T8).

```text
$ grep -rn "duracionFranjaEstimada" src test
(vacío; exit 1)

$ grep -rn "ventanaDePosicion\|eventoDentroDeVentana" src
… solo la definición nueva de cada una (calculo_eventos.hpp/.cpp) y las llamadas con D
de ServicioTurnosEventos, solver_config y perfil.

$ ctest --test-dir build-backend --output-on-failure -R '^test_calculo_eventos$'
1/1 Test #27: test_calculo_eventos .............   Passed    0.01 sec
Totals: 30 passed, 0 failed, 0 skipped, 0 blacklisted

$ ctest --preset backend
52/52 Test #52: test_ipc_aislamiento_bd ...............   Passed    0.10 sec
100% tests passed out of 52
```

### Cambios

- `calculo_eventos.hpp` / `.cpp`: se borran `duracionFranjaEstimada` y las sobrecargas antiguas de
  `ventanaDePosicion` (2 args) y `eventoDentroDeVentana` (3 args), con su bloque de comentario de
  legado. Quedan solo las firmas con D.
- `test/backend/test_calculo_eventos.cpp`: de 42 a **30 casos**.

### Casos retirados y migrados

Retirados por duplicar cobertura de T4 o por cubrir la función eliminada:
- Los **7** de `duracionFranjaEstimada` (reparto, sin eventos, `numSlots` 0/negativo, ventana inválida,
  eventos que no caben y reparto no positivo): el reparto ya no existe; su semántica la cubren
  `franjasUtilizables`/`deficitDeEncaje`.
- `ventanaDePosicion_posicionFueraDeRango_ventanaInvalida` (duplica el caso con D de T4, que prueba
  0/9/−1).
- `eventoDentroDeVentana_dentro_verdadero` y `eventoDentroDeVentana_sinHoras_verdadero` (duplican el
  caso con D de T4).
- Los dos casos de delegación del legado.

Migrados a la firma con D (aportaban algo que T4 no cubre):
- `ventanaDePosicion_delFinDeLaFranjaAlInicioDeLaSiguiente`: posición 2 = 08:10–08:20, la 3 empieza una
  franja después más el receso (08:55) y la última llega al fin (10:40–13:00), con D = 35.
- `ventanaDePosicion_ultimaLlegaAlFinDelTurnoConResto`: 8 × 35 + receso → última 11:50–12:31 (sobrante
  de 41 min sin repartir).
- `ventanaDePosicion_turnoSinFranjas_ventanaInvalida`: `numSlots = 0` con D = 35 → ventana inválida
  (guarda distinta de la de D ≤ 0).
- `eventoDentroDeVentana_antesDeLaVentana_falso` y `_despuesDeLaVentana_falso`: ventana 08:10–08:20 con
  D = 35, intervalo anterior y posterior.

### Notas

- No se añadió producción nueva; solo se retiró legado y se recalcularon sus casos.
- No quedan comentarios que anuncien el legado (`grep` de «legado» vacío en los archivos tocados).
- No se tocaron `solver_config.cpp`, el constructor, el seed, el esquema, `duracion_horaria` ni
  `docs/`.
