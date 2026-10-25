# Journal 010 - Solver: tipos de aula y eventos del turno

## T1 - Contrato 2.0, constructor con `Resultado` y consumidores (2026-10-25)

**Requisito:** RF-1. **Plan:** §1.1, §2.1, §2.2, §3.1 y decisión 12. **Base:** `0096b36`.

### Rojo (tests primero)

Se reescribieron `test_contrato_solver` (forma 2.0, tipos, eventos, ventana de turno, ida y
vuelta) y `test_constructor_entrada` (tipos, `eventos`, `inicio/fin`, `Resultado`, fallo por aula o
materia sin tipo, ida y vuelta, fachada), y se adaptaron los consumidores
(`test_validador_entrada`, `test_servicio_generacion`, `test_ipc_aislamiento_bd`,
`test_variables_cp_sat`) a `Resultado<QJsonObject>` y a los `DatosDominio` con `tipoAula` por
nombre. Con la producción todavía en 1.0, el árbol no compilaba:

```text
$ cmake --build --preset backend --parallel
test_constructor_entrada.cpp:104:50: error: 'const class QJsonObject' has no member named 'ok'
test_constructor_entrada.cpp:311:44: error: 'const struct MateriaSolverConfig' has no member named 'tipo'
test_constructor_entrada.cpp:313:49: error: 'const struct TurnoConfig' has no member named 'inicio'
test_constructor_entrada.cpp:315:32: error: 'const struct SolverConfig' has no member named 'eventos'
test_servicio_generacion.cpp:111:50: error: 'const class QJsonObject' has no member named 'ok'
test_ipc_aislamiento_bd.cpp:86:61: error: 'const class QJsonObject' has no member named 'mensajeError'
ninja: build stopped: subcommand failed.
```

### Verde

```text
$ cmake --build --preset backend --parallel
[88/88] Linking CXX executable test/test_solver_benchmark        # exit 0

$ ctest --test-dir build-backend --output-on-failure -R 'test_contrato_solver|test_constructor_entrada|test_servicio_generacion'
1/3 Test #22: test_constructor_entrada .........   Passed    0.02 sec
2/3 Test #25: test_servicio_generacion .........   Passed    0.09 sec
3/3 Test #42: test_contrato_solver .............   Passed    0.01 sec
100% tests passed out of 3
```

Casos nuevos de `test_constructor_entrada` (18/18 en verde, ejecutando el binario):

```text
PASS   : TestConstructorEntrada::turnosYeventosSeDerivanDelDominio()
PASS   : TestConstructorEntrada::tiposDeMateriasYAulasSeCopianDelDominio()
PASS   : TestConstructorEntrada::aulaSinTipo_fallaSinEmitirJson()
PASS   : TestConstructorEntrada::materiaSinTipo_fallaSinEmitirJson()
PASS   : TestConstructorEntrada::tipoAusenteEnAulaYmateria_informaElAula()
PASS   : TestConstructorEntrada::idaYVuelta2_0_reproduceTiposYeventos()
PASS   : TestConstructorEntrada::entradaSinVersionDeclarada_seSellaA_2_0()
PASS   : TestConstructorEntrada::fachadaDevuelveElMismoResultado()
Totals: 18 passed, 0 failed, 0 skipped, 0 blacklisted
```

Suite completa (`ctest --test-dir build-backend --output-on-failure`): **48/49**. El único rojo es
`test_cargador_configuracion` (8 casos de versión/loader con fixture `"version": "1.0"`); es de
**T2** según el reparto del plan §1.3/§8 y queda fuera del `-R` de esta tarea.

### Cambios

- `solver_config.hpp`: `MateriaSolverConfig{QString nombre; QString tipo;}`,
  `TurnoConfig{nombre, slot, inicio, fin}`, `EventoConfig` (sin identidad), `SolverConfig::version`
  por defecto `2.0` y `eventos`; fuera `RecesoConfig` y `recesos`.
- `solver_config.cpp`: parseo/serialización de `eventos`, `inicio/fin` de turno y `tipo` de
  materia; V1 comprueba **la versión antes que las secciones** (ausente o ≠ `2.0` → error), exige
  `eventos` y ya no `recesos`; `toJson` sella siempre `2.0`.
- `ConstructorEntradaSolver.{hpp,cpp}`: `construirJsonEntrada` y `ConstructorEntradaSolver::construir`
  devuelven `Resultado<QJsonObject>`; guarda de RF-1 (aulas y después materias, primer defecto) con
  mensaje en español que nombra la entidad y **sin emitir JSON**; copia `tipoAula` → `tipo`,
  rellena `inicio/fin` desde el `TurnoDTO` y emite una entrada por evento sin identidad; fuera
  `recesosDesdeEventos`; la versión de la entrada no se copia del preset (siempre 2.0). El instante
  sigue inyectado y la función es pura.
- Tests: `test_contrato_solver` y `test_constructor_entrada` reescritos; consumidores
  (`test_validador_entrada`, `test_servicio_generacion`, `test_ipc_aislamiento_bd`,
  `test_variables_cp_sat`) adaptados. `test_variables_cp_sat` solo pierde `cfg.recesos = {}` y pasa
  su `version` a `2.0`: sus tipos son de T4/T6.
- Sin tocar `src/frontend` (rotura conocida del builder), `spec.md`, `plan.md` ni `estado.md`.
  `git diff --stat`: 10 archivos, todos de `src/backend` y `test/`.

### Hallazgos

- **El helper `entradaValida()` del plan no puede usar `QVERIFY`:** la macro hace `return;` y no
  compila en una función que devuelve `QJsonObject`. Se usó `QTest::qVerify(...)` (informa el fallo
  sin retornar) y se devuelve `resultado.valor`; en los tests se sigue comparando `.valor`.
- **Un solo nombre de campo:** el contrato usa `tipo`; la guarda de RF-1 mira `tipoAula` del DTO.
  Fijar solo `idTipoAula` (la FK) no produce tipos en la entrada.
- **Sección `recesos`:** en un 2.0 se ignora como campo desconocido (no es requerida); lo comprueba
  `construyeJsonEstructuralmenteCompleto` (`!json.contains("recesos")`).
- **Pendiente de T3:** V2 (forma/temporalidad de eventos), V16 y V17 no se implementaron aquí; sus
  casos viven en `test_validador_entrada`, que hoy sigue verde sin ellos.

## T2 - Rechazo de configuraciones anteriores (2026-10-25)

**Requisito:** RF-1 (2.0 válido) y RF-2 (rechazo de 1.0/sin versión). **Plan:** §1.3, §2.4 y §8-T2.
**Base:** `3a3e724`.

### Rojo (tests primero)

Rojo de partida (el que dejó T1), sobre el test todavía en 1.0:

```text
$ ctest --test-dir build-backend --output-on-failure -R '^test_cargador_configuracion$'
Totals: 6 passed, 8 failed, 0 skipped, 0 blacklisted, 2ms
FAIL!  : guardarYcargar_roundtrip() 'config.ok' returned FALSE.
          (V1: versión de configuración no reconocida ('1.0'); se espera '2.0'.)
FAIL!  : camposDesconocidos_seToleran() 'config.ok' returned FALSE.
FAIL!  : version_1_0_pasa() 'config.ok' returned FALSE.
FAIL!  : versionNoReconocida_falla() 'config.mensajeError.contains("1.0")' returned FALSE.
FAIL!  : configSinVersionDeclarada_seSellaYRecarga() 'parseada.ok' returned FALSE.
FAIL!  : configPorDefectoYaDeclaraVersion_1_0() Actual "2.0" vs Expected "1.0"
FAIL!  : guardarCarpetaInexistente_error() 'config.ok' returned FALSE.
FAIL!  : listarPresets_devuelveJsonOrdenados() 'config.ok' returned FALSE.
```

Tras reescribir el test (2.0 válido, 1.0 rechazado, fixture versionado) y **antes** de tocar el
cargador, quedaban solo los dos casos del cargador en rojo:

```text
Totals: 17 passed, 2 failed, 0 skipped, 0 blacklisted, 7ms
FAIL!  : preset1_0_seRechazaConArchivoVersionYaccion() 'mensaje.contains(ruta)' returned FALSE.
          (V1: versión de configuración no reconocida ('1.0'); se espera '2.0'.)
FAIL!  : presetSinVersion_seRechazaConArchivoYaccion() 'resultado.mensajeError.contains(ruta)'
          returned FALSE. (V1: la configuración no declara versión; se espera '2.0'.)
```

El rechazo por versión de `fromJson` y el sellado 2.0 de `toJson` ya venían de T1: lo que faltaba era
el **envoltorio del cargador** con archivo y acción.

### Verde

```text
$ cmake --build --preset backend --parallel
[54/54] Linking CXX executable test/test_cargador_configuracion        # exit 0

$ ctest --test-dir build-backend --output-on-failure -R '^test_cargador_configuracion$'
1/1 Test #21: test_cargador_configuracion .......   Passed    0.02 sec
100% tests passed, 0 tests failed out of 1
```

Casos (19/19, ejecutando el binario):

```text
PASS   : cargarArchivoInexistente_error()
PASS   : cargarArchivoCorrupto_error()
PASS   : guardarYcargar_roundtrip()
PASS   : guardarYcargar_conservaTiposYeventos()
PASS   : camposDesconocidos_seToleran()
PASS   : version_1_0_seRechaza()
PASS   : versionAusente_seRechaza()
PASS   : versionNoReconocida_seRechaza()
PASS   : preset1_0_seRechazaConArchivoVersionYaccion()
PASS   : presetSinVersion_seRechazaConArchivoYaccion()
PASS   : config2_0ConErrorDeSeccion_noApuntaAVersion()
PASS   : configSinVersionDeclarada_seSellaA_2_0()
PASS   : configPorDefectoYaDeclaraVersion_2_0()
PASS   : toJson_noCompletaTiposPorDefecto()
PASS   : guardarCarpetaInexistente_error()
PASS   : listarPresets_devuelveJsonOrdenados()
PASS   : listarPresets_directorioInexistente_error()
Totals: 19 passed, 0 failed, 0 skipped, 0 blacklisted, 11ms
```

Mensaje real del preset 1.0 (capturado con un `qInfo` temporal, ya retirado; el binario se recompiló y
se volvió a verificar después):

```text
La configuración de «/home/luis/Repos/Gestor-Horarios/test/backend/../data/preset-1.0.json»
declara la versión «1.0»; solo se acepta la versión 2.0.
Genere una configuración 2.0 desde los datos con la versión actual.
```

(La ruta sale tal cual la recibe el cargador: `QFINDTESTDATA` devuelve
`test/backend/../data/preset-1.0.json`. No se canonicaliza en el cargador para no reescribir la ruta
que da el llamador.)

Suite completa:

```text
$ ctest --test-dir build-backend
100% tests passed, 0 tests failed out of 49
```

### Cambios

- `src/backend/src/services/CargadorConfiguracionSolver.cpp`: `cargar` deja de devolver el error de
  `fromJson` sin más. Si la versión del JSON **no** es `2.0` (ausente, `1.0` o desconocida), devuelve
  un error que nombra el archivo, la versión declarada («no declara versión» si falta) y la acción
  («Genere una configuración 2.0 desde los datos con la versión actual.»). Cualquier otro error de
  validación de un 2.0 se devuelve intacto (no se disfraza de rechazo de versión). No se convierte ni
  se reinterpreta nada.
- `src/backend/include/backend/services/CargadorConfiguracionSolver.hpp`: una nota en el comentario de
  cabecera sobre el rechazo de versiones anteriores (RF-2); sin cambios de firma.
- `test/data/preset-1.0.json` (**nuevo, versionado**): configuración 1.0 completa y válida para el
  contrato 1.0 (las 10 secciones con `recesos`, con un receso, sin `eventos` y sin `tipo` en materias
  ni aulas). sha256 `6e881f83cf459945a761c3a2570a76abfda65d9f5bd1f11b9a8350cf0453e79a`.
- `test/backend/test_cargador_configuracion.cpp`: `configValida()` pasa a 2.0 con `eventos`,
  `turnos[].inicio/fin` y `tipo` coherente en materia y aula; los casos de versión se reescriben
  (`version_1_0_pasa` → `version_1_0_seRechaza`, `configPorDefectoYaDeclaraVersion_1_0` →
  `..._2_0`, `configSinVersionDeclarada_seSellaYRecarga` → `..._seSellaA_2_0`) y se añaden el fixture
  con hash de bytes, el archivo sin versión, el 2.0 con error de sección (no se confunde con versión),
  la conservación de tipos/eventos y la ausencia de tipos por defecto al serializar.
- Sin tocar V1/V2/V16/V17 del validador (T3), el modelo del solver (T4+) ni el frontend.

### Hallazgos

- **El envoltorio se decide por la versión del JSON, no por el texto del error.** `fromJson` no
  devuelve un código de error de versión (solo un mensaje), así que el cargador relee
  `objeto["version"]` tras el fallo. Es seguro porque V1 comprueba la versión **antes** que las
  secciones: si la versión no es 2.0, el fallo es necesariamente de versión. Evita acoplar el cargador
  al texto de V1 (que puede cambiar de redacción).
- **El evento de `configValida()` tiene horario válido por construcción:** turno 07:00–12:00 (300 min),
  6 franjas, un evento de 30 min → `duracionFranjaEstimada` = 45; la ventana de la posición 2 es
  08:30–09:45 y el evento 09:15–09:45 cabe. Se elige así para que siga siendo válido cuando T3
  implemente V2 reutilizando las funciones puras de la 009.
- **El 2.0 sin tipos no se prueba como rechazo aquí** (es V16, T3). En T2 lo que se fija es que ni
  `toJson` ni la carga inventan un tipo: `toJson_noCompletaTiposPorDefecto` exige cadena vacía.
- **Consumidores que dependían de aceptar 1.0: ninguno en backend.** `CargadorConfiguracionSolver::cargar`
  solo se llama desde el frontend (`src/frontend/src/views/schedule_generation_widget.cpp:842`, que ya
  gestiona `Resultado` y está fuera de alcance y sin compilar). Quedan `version = "1.0"` en configs
  construidas en C++ de `test_solver_horarios`, `test_solver_benchmark` y en
  `src/frontend/src/services/generacion_tipos.hpp:46`: no llaman a `fromJson` con ese valor (serializan
  y `toJson` sella 2.0), así que no dependen del aceptar 1.0; su limpieza es de T4/T10 y del frontend.
  `test_servicio_generacion` y `test_ipc_aislamiento_bd` no cargan presets 1.0.

## T3 - Casos V1 (eventos), V2, V16 y V17 (2026-10-25)

**Requisito:** RF-5. **Plan:** §1.1 (`claveTipo`), §2.3 (validaciones) y §8-T3.
**Base:** `d2fbda1` (T1-T2 commiteadas; suite 49/49).

### Rojo (tests primero)

Se añadieron los casos de V1 (sección `eventos`, `recesos`, versión ausente y 1.0), V2
(nombre, turno, posición, orden, duración, par, `fin<=inicio`, diferencia con las horas, ventana de
turno y de posición, solapes, cronología invertida y `duracion_minutos`), V16 (aula/materia) y V17
(tipo sin aulas en ambos modos, tipos equivalentes por clave, tipo no requerido) a
`test_validador_entrada`, y `claveTipo` a `test_contrato_solver`. Con la producción de T2 (V2 solo
tipos/rangos, sin V16/V17):

```text
$ ./build-backend/test/test_validador_entrada
FAIL!  : TestValidadorEntrada::v2_eventoSinNombre_falla() '!resultado.ok' returned FALSE. ()
FAIL!  : TestValidadorEntrada::v2_eventoTurnoInexistente_falla() ...
FAIL!  : ... (posición 0/fuera, orden 0, duración 0, par incompleto, fin no posterior,
          duración que no coincide, fuera de ventana, turno sin ventana, solape, cronología)
FAIL!  : TestValidadorEntrada::v16_aulaSinTipo_falla() '!resultado.ok' returned FALSE. ()
FAIL!  : TestValidadorEntrada::v16_aulaSinCampoTipo_falla() ...
FAIL!  : TestValidadorEntrada::v16_materiaSinTipo_falla() ...
FAIL!  : TestValidadorEntrada::v17_tipoRequeridoSinAulas_falla() ...
FAIL!  : TestValidadorEntrada::v17_tipoRequeridoSinAulas_fallaConAulasMoviles() ...
Totals: 32 passed, 18 failed, 0 skipped, 0 blacklisted, 22ms
```

Los casos de V1 pasaron ya en el primer intento: el comportamiento se cambió en T1 (versión antes
que secciones, `eventos` en vez de `recesos`) y aquí solo se reescribieron sus pruebas. Rojo de
`claveTipo` (la función no existía; el plan la asignaba al contrato y T1 no la añadió):

```text
$ cmake --build --preset backend --parallel
test_contrato_solver.cpp:243:18: error: ‘claveTipo’ no se declaró en este ámbito
```

### Verde

```text
$ cmake --build --preset backend --parallel      # EXIT=0 (ninja: no work to do en la 2.ª pasada)

$ ctest --test-dir build-backend --output-on-failure -R 'test_validador_entrada|test_contrato_solver'
1/2 Test #23: test_validador_entrada ...........   Passed    0.03 sec
2/2 Test #42: test_contrato_solver .............   Passed    0.01 sec
100% tests passed out of 2

$ ctest --test-dir build-backend
100% tests passed, 0 tests failed out of 49
```

Casos nuevos/juzgados de `test_validador_entrada` (50/50 en verde, ejecutando el binario):

```text
PASS   : v1_seccionEventosAusente_falla()          PASS   : v2_eventoFueraDeLaVentanaDeSuPosicion_falla()
PASS   : v1_recesosEnVezDeEventos_falla()          PASS   : v2_eventoConHorarioYturnoSinVentana_falla()
PASS   : v1_versionAusente_falla()                 PASS   : v2_eventosSolapados_fallan()
PASS   : v1_version1_0_falla()                     PASS   : v2_eventosConCronologiaInvertida_fallan()
PASS   : v2_eventoConHorarioValido_pasa()          PASS   : v2_duracionMinutosUno_pasa()
PASS   : v2_eventoSinHorario_pasa()                PASS   : v2_duracionMinutosCero_falla()
PASS   : v2_eventoSinNombre_falla()                PASS   : v16_aulaSinTipo_falla()
PASS   : v2_eventoTurnoInexistente_falla()         PASS   : v16_aulaSinCampoTipo_falla()
PASS   : v2_eventoPosicionCero_falla()             PASS   : v16_materiaSinTipo_falla()
PASS   : v2_eventoPosicionFueraDeRango_falla()     PASS   : v17_tipoRequeridoSinAulas_falla()
PASS   : v2_eventoOrdenCero_falla()                PASS   : v17_..._fallaConAulasMoviles()
PASS   : v2_eventoDuracionCero_falla()             PASS   : v17_tipoIgualSalvoMayusculasYEspacios_pasa()
PASS   : v2_eventoParIncompleto_falla()            PASS   : v17_tipoDeclaradoQueNingunaMateriaRequiere_pasa()
PASS   : v2_eventoFinNoPosteriorAlInicio_falla()
PASS   : v2_eventoDuracionNoCoincideConLasHoras_falla()
Totals: 50 passed, 0 failed, 0 skipped, 0 blacklisted, 36ms

$ ./build-backend/test/test_contrato_solver
PASS   : TestContratoSolver::claveTipo_normalizaRecorteYPlegado()
Totals: 13 passed, 0 failed, 0 skipped, 0 blacklisted, 2ms
```

Mensajes reales (capturados con un programa desechable fuera del repo, ya borrado):

```text
V2: el evento «Receso» declara una duración de 9 min, que no coincide con sus horas (10 min).
V2: el intervalo 08:00–08:10 del evento «Receso» no cabe en la ventana de la posición 2 (08:36–08:46).
V2: el turno «manana» no declara una ventana inicio/fin válida (inicio «», fin «») y tiene eventos con horario.
V2: los eventos del turno «manana» se solapan o van en orden cronológico invertido; revise sus horas.
V2: el evento «Receso» declara la posición 7; debe estar entre 1 y 6.
V16: el aula «A1» no declara tipo de aula (campo 'tipo' vacío).
V16: la materia «Matemática» no declara tipo de aula requerido (campo 'tipo' vacío).
V17: el tipo de aula «Laboratorio» requerido por la materia «Matemática» del curso 1A no tiene ninguna aula en el centro.
duracion_minutos = 1 -> ACEPTA
tipos equivalentes por clave -> ACEPTA
```

### Cambios

- `src/backend/src/solver/config/solver_config.cpp`: **V2 de eventos** (`validarEventos`, tras
  `validarTiposYRangos`): nombre no vacío, turno existente, posición `1..turno.slot.size()`, orden
  `>= 1`, duración `> 0`, par inicio/fin completo o ausente, horas `HH:mm`, `fin > inicio`,
  `fin − inicio = duración`; después, por turno, `duracionFranjaEstimada`, `eventoDentroDeVentana` y
  `haySolapeCronologico` sobre `EventoDTO`/`TurnoDTO` convertidos. **V16** (`validarTiposDeclarados`,
  aulas y después materias) y **V17** (`validarTiposConAulas`, ambos modos, comparación por
  `claveTipo`), añadidas tras V15 en el orden del plan. `duracion_minutos` sigue exigiendo solo `> 0`
  (la 011 lo estrecha a 30–90).
- `src/backend/include/backend/solver/config/contrato_solver.hpp` y `.cpp`: `claveTipo(nombre)` pura
  (`trimmed().toCaseFolded()`), la misma noción que `ServicioTiposAula::claveTipo`, sin BD.
- `test/backend/test_validador_entrada.cpp`: helpers `conEventos`/`conAulas`/`conMaterias`/`evento` y
  `exigeRechazoV2|V16|V17`; 5 casos de V1, 17 de V2, 3 de V16 y 4 de V17.
- `test/backend/test_contrato_solver.cpp`: `claveTipo_normalizaRecorteYPlegado` (espacios, mayúsculas,
  acentos, vacío); fixture `config2_0` con el evento dentro de la ventana de su posición.
- Fixtures preexistentes que **V2 rechazaba** por aritmética de ventana (no por el contrato):
  `test_constructor_entrada` (evento 08:20–08:30 → 07:48–07:58, posición 1) y
  `test_cargador_configuracion` (09:15–09:45 → 08:30–09:00, posición 2), con sus assertions. La ventana
  de una posición va del **inicio** de la posición a **inicio + suma de duraciones** de sus eventos; el
  comentario de T2 suponía que llegaba al fin del turno.

### Hallazgos

- **No hizo falta parar por datos ausentes:** el número de franjas de un turno sale de
  `TurnoConfig::slot.size()` (el constructor emite ahí los slots absolutos del día), que es lo que
  `TurnoDTO::numSlots` necesita. Nada nuevo en el contrato.
- **Sin ventana en el turno, con eventos sin horario, se acepta.** La comprobación de rejilla
  (`duracionFranjaEstimada`) solo se aplica si el turno declara `inicio`/`fin`; si un evento declara
  horario y el turno no declara ventana, se rechaza con mensaje propio. Así una configuración antigua
  sin ventana sigue siendo válida mientras sus eventos no declaren horas, y el constructor no puede
  emitir una entrada con eventos con horario y turno sin ventana.
- **V17 obliga a que el fixture de «el nombre visible se conserva» declare el tipo también en el
  aula:** `tipoSeConservaTalCualSeDeclaro` ponía la materia como `"  Laboratorio  "` con el aula
  `"Aula de estudio"`, que con la regla dura es un tipo sin aulas; ahora el aula declara
  `" LABORATORIO "` y la igualdad se resuelve por clave (y se comprueba que el nombre no se reescribe).
- **`claveTipo` faltaba del cierre de T1.** El plan (§1.1) la asignaba al contrato y T3 es el primer
  consumidor (V17 y su prueba); se añade aquí como función pura, sin duplicar la del servicio de BD.
- **Fuera de alcance respetado:** sin cambios en el modelo del solver, la regla de asignación (T4), el
  validador de salida (T5) ni la documentación (T7-T9); sin commit.
