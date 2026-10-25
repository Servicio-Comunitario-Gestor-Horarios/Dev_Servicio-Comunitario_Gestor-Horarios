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

## T4 - Modelo: tipo, referencia, desplazamiento, bloqueo y salida real (2026-10-25)

**Requisito:** RF-3 y RF-4. **Plan:** §4.1 (variables), §4.2 (restricciones), §4.3 (salida) y
decisiones 5-9. **Base:** `4142939` (T1-T3 commiteadas; suite 49/49).

### Rojo (tests primero)

Se migraron los fixtures de `test_solver_horarios` y `test_variables_cp_sat` a **tipos coherentes**
(materias y aulas del mismo tipo, `version` a `"2.0"`) y se añadieron los casos de tipo, referencia,
desplazamiento, bloqueo, exclusividad de desplazadas, modo móvil y `y` de todos los cursos. Con el
modelo de T3 (sin tipos, sin `y` en aula fija, salida con la ficha del curso):

```text
$ ./build-backend/test/test_solver_horarios
FAIL!  : solver_materiaDeOtroTipoSeDesplazaYVuelve() Actual (asig.aula): 0  Expected (1): 1
FAIL!  : solver_referenciaBloqueadaMientrasHayClaseDesplazada()
          Actual (usosDeLaReferenciaBloqueada): 2  Expected (0): 0
FAIL!  : solver_exclusividadEntreClasesDesplazadas() Actual (enLaboratorio): 0  Expected (4): 4
FAIL!  : solver_modoMovilEligeAulaDelTipoRequerido() Actual (asig.aula): 0  Expected (2): 2
Totals: 17 passed, 4 failed, 0 skipped, 0 blacklisted, 63ms

$ ./build-backend/test/test_backend_variables_cp_sat
[  FAILED  ] VariablesTest.AulaFija_CreaAulaAssignmentParaTodasLasFranjasDelCurso
[  FAILED  ] VariablesTest.AulaAssignment_SoloAulasDeLosTiposQueElCursoRequiere
[  FAILED  ] VariablesTest.MultipleCourses_DifferentTurnos
[  PASSED  ] 18 tests.
 3 FAILED TESTS
```

(`solver_aulaFijaUsaLaReferenciaSiElTipoCoincide` pasaba ya en rojo porque el atajo antiguo
«aula fija ⇒ aula del curso» devolvía la referencia sin comprobar nada; por eso la prueba decisiva de
la referencia es la de dos cursos y dos aulas del mismo tipo, ver controles negativos.)

### Verde

```text
$ cmake --build --preset backend --parallel
[6/6] Linking CXX executable test/test_ipc_aislamiento_bd     # exit 0

$ ctest --test-dir build-backend --output-on-failure -R 'test_solver_horarios|test_variables_cp_sat'
1/1 Test #47: test_solver_horarios .............   Passed    0.15 sec
100% tests passed out of 1

$ ctest --test-dir build-backend --output-on-failure -R 'variables_cp_sat'
1/1 Test #46: test_backend_variables_cp_sat ....   Passed    0.06 sec
```

(`-R 'test_variables_cp_sat'` no casa con el nombre real del test en CTest,
`test_backend_variables_cp_sat`; se ejecutó aparte. Sin cambios en `test/CMakeLists.txt`.)

Casos (23/23 en `test_solver_horarios`, ejecutando el binario):

```text
PASS   : solver_aulaFijaUsaLaReferenciaSiElTipoCoincide()
PASS   : solver_aulaFijaFuerzaLaReferenciaFrenteAOtraAulaDelMismoTipo()
PASS   : solver_materiaDeOtroTipoSeDesplazaYVuelve()
PASS   : solver_referenciaBloqueadaMientrasHayClaseDesplazada()
PASS   : solver_referenciaBloqueadaNoCedeSuFranja()
PASS   : solver_exclusividadEntreClasesDesplazadas()
PASS   : solver_modoMovilEligeAulaDelTipoRequerido()
Totals: 23 passed, 0 failed, 0 skipped, 0 blacklisted, 85ms
```

`test_backend_variables_cp_sat`: **21/21**, con `AulaFija_CreaAulaAssignmentParaTodasLasFranjasDelCurso`,
`AulaAssignment_SoloAulasDeLosTiposQueElCursoRequiere` y `MultipleCourses_DifferentTurnos` (la `y` de
los dos cursos, cada una solo en las franjas de su turno).

### Controles negativos (las pruebas no son vacuas)

Se desactivó cada regla por separado, se recompiló y se ejecutó el binario; después se restauró el
archivo (el árbol final no contiene ningún control):

```text
sin bloqueo de la referencia      -> FAIL solver_referenciaBloqueadaNoCedeSuFranja (el modelo pasa a ser factible)
sin tipo exacto                   -> FAIL solver_exclusividadEntreClasesDesplazadas + solver_modoMovilEligeAulaDelTipoRequerido
sin implicación de la referencia  -> FAIL solver_aulaFijaFuerzaLaReferenciaFrenteAOtraAulaDelMismoTipo
```

La primera versión de los casos de bloqueo/referencia era **vácua**: sin la regla seguían en verde
(el solver elegía la referencia por orden de variables). Se reforzaron: el bloqueo se prueba con un
caso que sin la reserva es **factible** (`solver_referenciaBloqueadaNoCedeSuFranja`) y la referencia
con dos cursos cuyas referencias están cruzadas respecto del orden de variables.

### Suite completa

```text
$ ctest --test-dir build-backend
98% tests passed, 1 tests failed out of 49
48 - test_solver_benchmark (Failed)
```

**Rojo único: `test_solver_benchmark`.** Su fixture declara `tipo` vacío en las 15 materias
(`{"Matematica"}, ...`) y tipos en las aulas (`regular`, `laboratorio`, `auditorio`). Con la regla dura
ninguna clave de materia coincide con ningún aula: no se crea ninguna `y` y D9 deja las clases a 0 →
INFEASIBLE. **No se tocó el fixture**: su estrategia (6 aulas: 5 comunes + 1 laboratorio, solo
`Tecnología` en laboratorio, referencias repartidas por turno) es de **T10** (§1.3 del plan). Con esa
estrategia la `y` sería de **615** variables (7 cursos × 5 días × 3 franjas × 5 aulas comunes + 1 curso
× 5 × 3 × 6 aulas), frente a las 0 de hoy; el resto de la suite (48/49) está en verde y
`test_solver_horarios` tarda 85 ms.

### Cambios

- `src/backend/src/solver/variables.cpp`: la `y[c][d][s][a]` se crea para **todos los cursos** (fijos y
  móviles), en las franjas del turno de cada curso, y **solo para las aulas cuyo tipo requiere alguna
  materia del curso** (las demás serían siempre 0 por el tipo exacto). Guarda `min(dim.num_aulas,
  aulas.size())` para no indexar fuera de rango con configuraciones incoherentes.
- `src/backend/src/solver/restricciones/restricciones_aulas.cpp`:
  - **D8**: exclusividad solo con `y` (fuera el caso de las `x` de los cursos fijos), agrupada por
    `(d, s, a)`; cubre las desplazadas.
  - **D9 ampliada**: `Σ_a y == Σ_{p,m} x` en cada `(c,d,s)`, en los dos modos; si no hay `y` en la
    franja, la igualdad fija las clases a 0 (la regla de tipo es dura).
  - **Tipo exacto**: `Σ_p x[p][m][c][d][s] + y[c][d][s][a] <= 1` para toda aula y materia con clave
    distinta.
  - **Aula de referencia** (solo modo fijo): coincidencia ⇒ `Σ_p x <= y[c][d][s][r]`; desplazamiento ⇒
    variable auxiliar implícita (la suma de las `x` de materias de otro tipo) y, por cada `y` **ya
    creada** de otro curso en `(d, s, r)`, `Σ x_desplazadas + y[c'][d][s][r] <= 1`. Se recorre el mapa
    de `y` existentes, nunca un índice ciego `c'`.
- `src/backend/src/solver/solver.cpp`: `extraerSolucion` informa la **`y` activa** de cada clase; fuera
  el atajo `config.aula_fija ? config.cursos[c].aula : y`.
- Cabeceras `variables.hpp` y `restricciones_aulas.hpp`: comentarios y firmas de las dos restricciones
  nuevas; sin cambios de ABI para el resto.
- `test/backend/test_solver_horarios.cpp`: tipos coherentes y `version` 2.0 en los fixtures; 7 casos
  nuevos (referencia, referencia forzada, desplazamiento y vuelta con aula real, bloqueo por
  invariante, bloqueo decisivo, exclusividad de desplazadas, móvil con tipo exacto); helper
  `dispCompleta(dias, franjasPorDia)`.
- `test/backend/test_variables_cp_sat.cpp`: materia con tipo; `FixedAula_NoAulaAssignmentCreated` →
  `AulaFija_CreaAulaAssignmentParaTodasLasFranjasDelCurso`; poda por tipos
  (`AulaAssignment_SoloAulasDeLosTiposQueElCursoRequiere`); `y` de los dos cursos en
  `MultipleCourses_DifferentTurnos`; `MobileAula_MultipleAulas` pasa a dos materias (regular y lab).
- Sin tocar el validador de salida (T5), las causas de infactibilidad (T6), el benchmark (T10), la
  documentación ni el frontend. `git diff --stat`: 7 archivos (5 de `src/backend`, 2 de `test/`).

### Hallazgos

- **El fixture del benchmark se vuelve infactible** por no declarar tipos (ver arriba): se deja en
  rojo a propósito y es de T10. No se relajó ninguna regla ni se inventaron tipos.
- **Poda de la `y` por tipos requeridos** (el plan §4.1 la dibuja para todas las aulas y §10 la
  propone como alternativa si el modelo crece): una `y` de un tipo que ninguna materia del curso
  requiere es siempre 0 por el tipo exacto, así que no se crea. Efecto en los conteos de
  `test_variables_cp_sat` (60 en vez de 90 con un aula de laboratorio sin materia de laboratorio).
- **`slots` como nombre de parámetro rompe moc** (`Missing access specifier for slots`): `slots` es
  palabra clave de Qt; el helper usa `franjasPorDia`.
- **El bloqueo no se puede probar solo con una invariante**: sin la regla, la solución también la
  cumplía por casualidad. Hizo falta un caso que sin bloqueo sea factible (y con bloqueo infactible) y
  un caso de referencias cruzadas para detectar la implicación.
- **Causa (2) del benchmark**: el mensaje actual es «no se pudo identificar una categoría concreta»
  (la saturación por tipo es de T6); no se tocó.

## T5 - Validador de salida sobre el aula real (2026-10-25)

**Requisito:** RF-4. **Plan:** §1.2 (sin cambios de producción), §1.3 (`test_validador_salida`,
solo pruebas) y §8-T5. **Base:** `0edc6f8` (T1-T4 commiteadas; suite 48/49, rojo único el
benchmark).

### Por qué no cambia la producción

`ValidadorSalidaSolver` ya cumple lo que la 010 necesita: P3 compara la capacidad con
`config.aulas[asig.aula].capacidad` y P4 nombra el aula con `asig.aula`, es decir, con el **aula
real** que emite el solver desde T4, no con `config.cursos[c].aula` (la referencia). Ningún caso
nuevo hizo falta en verde por un cambio de código: los 3 casos se escribieron primero y pasaron en
la primera ejecución. `git diff` no toca `src/`:

```text
 test/backend/test_validador_salida.cpp | 129 ++++++++++++++++++++++++++++++++-
 1 file changed, 126 insertions(+), 3 deletions(-)
```

### Rojo (tests primero)

Los casos nuevos se escribieron antes de tocar nada y **pasaron ya en la primera ejecución** (no
había nada que implementar):

```text
$ ./build-backend/test/test_validador_salida
PASS   : TestValidadorSalida::aulaFijaConClasesEnAulasDistintas_usaElAulaRealDeCadaClase()
PASS   : TestValidadorSalida::dosClasesDesplazadasEnElMismoAulaYfranja_noPresentable()
PASS   : TestValidadorSalida::aulaInexistente_noIndexaFueraDeRangoYseRechazaAlColisionar()
Totals: 11 passed, 0 failed, 0 skipped, 0 blacklisted, 1ms
```

Por eso el valor de la tarea está en los **controles negativos** (abajo): demuestran que los casos
no son vacuos.

### Controles negativos (mutaciones temporales, restauradas)

Cada mutación se aplicó, se recompiló, se ejecutó el binario y se restauró el archivo (el árbol
final solo tiene el test modificado):

```text
A) producción: P3 usa el aula de referencia del curso en vez de asig.aula
   -> FAIL aulaFijaConClasesEnAulasDistintas_usaElAulaRealDeCadaClase
      Actual (a.avisos.size()): 0  Expected (1): 1
   (sin aviso de capacidad del laboratorio: el caso detecta el atajo «todo en la referencia»)

B) datos del caso: la clase desplazada se informa en la referencia (aula 0 en vez de 1)
   -> FAIL aulaFijaConClasesEnAulasDistintas_usaElAulaRealDeCadaClase
      Actual (a.avisos.size()): 0  Expected (1): 1

C) producción: se desactiva la comprobación P4 de aula (ocupacionAula)
   -> FAIL solapamientoDeAula_noPresentable            (no dice «aula «A1»»)
   -> FAIL dosClasesDesplazadasEnElMismoAulaYfranja... (no dice «aula «Laboratorio»»)
   -> FAIL aulaInexistente_noIndexaFueraDeRango...     (no dice «aula «(desconocida)»»)
   Totals: 8 passed, 3 failed
```

El control C destapó una aserción **débil** en la primera versión: los mensajes se buscaban en
`conflictos + avisos`, y el aviso P3 de capacidad («…aula «Laboratorio»…») hacía pasar la
comprobación aunque P4 no dijera nada. Ahora se busca solo en `a.conflictos` (helper
`conflictos()`); con eso, C falla en los 3 casos como debe.

### Verde

```text
$ cmake --build --preset backend --parallel        # exit 0
$ ctest --test-dir build-backend --output-on-failure -R '^test_validador_salida$'
1/1 Test #24: test_validador_salida ............   Passed    0.01 sec
100% tests passed out of 1

$ ctest --test-dir build-backend
98% tests passed, 1 tests failed out of 49
	 48 - test_solver_benchmark (Failed)
```

**Rojo único: `test_solver_benchmark`**, el fixture sin tipos que migra T10 (ya documentado en T4).
El resto, 48/49.

### Cambios

- `test/backend/test_validador_salida.cpp` (único archivo):
  - `configBase()` migrado a **tipos coherentes** con la regla dura: `Matemática`→«Aula de
    estudio», `Tecnología`→«Laboratorio»; aulas `A1` («Aula de estudio», 30) y `Laboratorio`
    («Laboratorio», 30). Fuera el tipo `"regular"` y la materia sin tipo.
  - `configConMateriaDesplazada()` y `horarioConMateriaDesplazada()`: curso 1A en modo de aula
    fija con 1 h de Matemática en su referencia y 1 h de Tecnología **desplazada** al laboratorio;
    el laboratorio (capacidad 10) no aloja los 20 estudiantes y la referencia (30) sí.
  - Caso nuevo `aulaFijaConClasesEnAulasDistintas_usaElAulaRealDeCadaClase`: presentable, cero
    conflictos y **exactamente un aviso**, el de la capacidad del **laboratorio** (con el nombre
    del aula real y sin mencionar A1). Si el validador usara la referencia para todas las clases,
    no habría ningún aviso.
  - Caso nuevo `dosClasesDesplazadasEnElMismoAulaYfranja_noPresentable` y caso
    `solapamientoDeAula_noPresentable` reforzado: el conflicto de P4 nombra el aula real («A1»,
    «Laboratorio»). `aulaInexistente_noIndexaFueraDeRangoYseRechazaAlColisionar`: un aula fuera
    de rango no se indexa en P3 y el conflicto la nombra «(desconocida)».
  - Helpers `conflictos()` (solo P4) y `texto()` (diagnóstico de fallo).
- Sin tocar `src/backend` (ni el validador de salida ni el modelo), el validador de entrada, la
  documentación ni el frontend. Sin commit.

### Hallazgos (fuera del alcance de T5, requieren decisión)

- **P4 no distingue aulas ni profesores.** `ocupacionAula` y `ocupacionProfesor` se indexan por
  `(día, slot)` **sin** el índice de aula o de profesor, así que **dos clases cualesquiera en la
  misma franja** se denuncian como conflicto. Sonda temporal (añadida, ejecutada y retirada) con
  1A en A1 y 1B en el laboratorio, mismo día/slot:

  ```text
  FAIL!  : sondaDosCursosEnAulasDistintasMismaFranja() 'a.presentable()' returned FALSE.
    El docente «(desconocido)» está en dos clases a la vez (día 0, slot 0).
    El aula «Laboratorio» está ocupada por dos clases a la vez (día 0, slot 0).
  ```

  Es un defecto **preexistente** (el archivo no ha cambiado en la 009/010) y no lo cubre ningún
  test; con él, cualquier horario real con clases en paralelo haría que `ServicioGeneracion`
  (`analisis.presentable()` ⇒ `CalculoFallido`) rechace el resultado. La clave debería incluir el
  índice de aula y el de profesor. **No se toca**: el plan de T5 prohíbe cambios de producción y
  el arreglo es un cambio de comportamiento de P4 (spec 002/005) que necesita su propia decisión.
- **Un aula inexistente, sola, no se rechaza.** Sin colisión, `asig.aula` fuera de rango se salta
  en P3 (guarda de rango) y no genera conflicto en P4; la salida queda «presentable». El caso de
  T5 lo cubre solo en la colisión, que es donde hoy se detecta y se nombra «(desconocida)». Un
  rechazo por rango sería una validación nueva (no está en la 010).

### Cierre de T5 (2026-10-25)

**Estado final:** T5 queda cerrada con los tres casos de RF-4 y el arreglo de P4 autorizado fuera
del plan (ver `## P4 · claves por aula y docente (2026-10-25)`). El párrafo «Por qué no cambia la
producción» de arriba describe T5 **antes** del arreglo: con P4 indexando solo por día/slot, el
caso de dos cursos en paralelo fallaba y algunos casos de T5 pasaban por el motivo equivocado.
Cambios respecto a lo escrito arriba:

- `aulaInexistente_noIndexaFueraDeRangoYseRechazaAlColisionar` pasa a llamarse
  `aulaInexistenteRepetidaEnLaFranja_noPresentable`: con P4 arreglado, un aula 7 en 1A **no**
  colisiona con el laboratorio (aula 1) de 1B; la colisión tiene que ser real, así que ahora 1A y
  1B informan **el mismo** aula 7 en la misma franja y el conflicto la nombra «(desconocida)». El
  caso conserva la comprobación de que un aula inexistente **sola** no es conflicto (P4 solo
  detecta colisiones; P3 no indexa fuera de rango).
- `dosClasesDesplazadasEnElMismoAulaYfranja_noPresentable` sigue fallando **por aula repetida**
  (las dos clases comparten el laboratorio en el mismo slot y el docente de 1B no está asignado:
  no hay conflicto de docente que enmascare la aserción). Ya no lo hace por el defecto de P4.
- `solapamientoDeProfesor_noPresentable` usa aula distinta (1) y `solapamientoDeAula_noPresentable`
  deja el docente de 1B sin asignar (-1), con aserciones que excluyen el conflicto del otro
  recurso: cada caso falla por una sola razón.
- El único cambio en `ValidadorSalidaSolver` es el de P4; P3 sigue comparando la capacidad con
  `asig.aula` y el caso del aula real no se ha tocado.

Verificación final (mismos comandos del plan):

```text
$ cmake --build --preset backend --parallel        # exit 0
$ ctest --test-dir build-backend --output-on-failure -R 'test_validador_salida|test_servicio_generacion'
2/2 tests passed, 0 tests failed out of 2
$ ./build-backend/test/test_validador_salida
Totals: 14 passed, 0 failed, 0 skipped, 0 blacklisted
$ ctest --test-dir build-backend
98% tests passed, 1 tests failed out of 49
	 48 - test_solver_benchmark (Failed)      # fixture sin tipos, T10
```

## P4 · claves por aula y docente (2026-10-25)

**Requisito:** P4 de la salida (RF-3/RF-4) y desbloqueo de la generación real con más de un curso.
**Autorizado por el usuario fuera del plan de la 010.** **Base:** `0edc6f8` (T1-T4 commiteadas).

### Defecto

`ValidadorSalidaSolver.cpp` indexaba la ocupación **solo por `(día, slot)`**: cualquier segunda
clase en la misma franja se denunciaba como «docente en dos clases» y «aula ocupada por dos
clases», aunque el docente y el aula fueran distintos. `ServicioGeneracion.cpp:191` convierte un
conflicto de P4 en `CalculoFallido`, así que con más de un curso (el seed tiene 13) toda
generación real se marcaría como fallida. Los tests no lo veían porque el fixture tenía un solo
curso.

### Rojo (tests primero, producción sin tocar)

Casos nuevos en `test_validador_salida.cpp`:

- `dosCursosEnParalelo_conAulasYdocentesDistintos_esPresentable` (el caso que hoy falla y debe
  pasar): 1A en A1 con Ana y 1B en el laboratorio con Beto, mismas dos franjas; cada curso cubre
  sus 2 horas.
- `docenteYaulaSinAsignar_noSonConflictoEntreSi`: dos cursos en la misma franja sin docente ni
  aula (`-1`) no colisionan.
- `mismoCursoDosVecesEnLaFranja_noPresentable`: P4 de curso se conserva.
- `solapamientoDeProfesor_noPresentable` / `solapamientoDeAula_noPresentable` aislados por
  recurso.

Con la producción anterior (HEAD) y los tests nuevos:

```text
$ cmake --build --preset backend --parallel        # exit 0
$ ./build-backend/test/test_validador_salida
FAIL!  : dosCursosEnParalelo_conAulasYdocentesDistintos_esPresentable()
          'a.conflictos.isEmpty()' returned FALSE. (El docente «Beto» está en dos clases a la vez
          (día 0, slot 0).
          El aula «Laboratorio» está ocupada por dos clases a la vez (día 0, slot 0).
          El docente «Beto» está en dos clases a la vez (día 0, slot 1).
          El aula «Laboratorio» está ocupada por dos clases a la vez (día 0, slot 1).)
FAIL!  : mismoCursoDosVecesEnLaFranja_noPresentable()   (ve «docente «Beto»» además del curso)
FAIL!  : solapamientoDeAula_noPresentable()             (ve «docente «(desconocido)»» además del aula)
FAIL!  : solapamientoDeProfesor_noPresentable()         (ve «aula «…»» aunque las aulas son distintas)
FAIL!  : docenteYaulaSinAsignar_noSonConflictoEntreSi()
Totals: 9 passed, 5 failed, 0 skipped, 0 blacklisted
```

(El detalle completo quedó en `/tmp/opencode/p4/rojo.txt`.) Los cinco fallos son la firma del
defecto: el validador veía colisión en el recurso que **no** se compartía.

### Arreglo

- Clave `ClaveOcupacion{recurso, día, slot}` (con `operator<`) y dos mapas:
  `ocupacionProfesor` indexado por **(profesor, día, slot)** y `ocupacionAula` por **(aula, día,
  slot)**. Dos mapas separados ⇒ un aula nunca choca con un docente.
- **Índices inválidos:** un recurso negativo (`profesor < 0`, `aula < 0` = sin asignar) **no ocupa
  franja**, así que dos clases sin docente/aula no chocan entre sí. Un índice **positivo fuera de
  rango** se indexa por su valor bruto: no colisiona con las aulas/docentes del catálogo, pero dos
  clases que informan la **misma** aula inexistente sí colisionan y el mensaje la nombra
  «(desconocida)» (P3 conserva su guarda de rango, no indexa fuera de rango).
- Se conserva intacto el resto de P4 (mismo curso dos veces en la franja), P1, P2 y P3.

### Verde

```text
$ cmake --build --preset backend --parallel        # exit 0
$ ./build-backend/test/test_validador_salida
PASS   : dosCursosEnParalelo_conAulasYdocentesDistintos_esPresentable()
PASS   : docenteYaulaSinAsignar_noSonConflictoEntreSi()
PASS   : mismoCursoDosVecesEnLaFranja_noPresentable()
PASS   : aulaInexistenteRepetidaEnLaFranja_noPresentable()
PASS   : dosClasesDesplazadasEnElMismoAulaYfranja_noPresentable()
Totals: 14 passed, 0 failed, 0 skipped, 0 blacklisted

$ ctest --test-dir build-backend --output-on-failure -R 'test_validador_salida|test_servicio_generacion'
2/2 tests passed, 0 tests failed out of 2

$ ctest --test-dir build-backend
98% tests passed, 1 tests failed out of 49
	 48 - test_solver_benchmark (Failed)
```

### Control negativo (mutación reversible)

Se copió el archivo arreglado a `/tmp/opencode/p4/`, se restauró la versión de HEAD
(`git checkout -- src/backend/src/services/ValidadorSalidaSolver.cpp`), se recompiló y se ejecutó
el mismo binario; después se restauró el arreglo y se volvió a verificar:

```text
# producción anterior
$ ./build-backend/test/test_validador_salida
Totals: 9 passed, 5 failed
# (incluye el FAIL! de dosCursosEnParalelo_conAulasYdocentesDistintos_esPresentable)

# arreglo restaurado
$ cmake --build --preset backend --parallel        # exit 0
$ ./build-backend/test/test_validador_salida
Totals: 14 passed, 0 failed
```

### Efecto sobre otros tests

- Único consumidor de `analizarSalidaSolver`: `ServicioGeneracion` (`test_servicio_generacion`
  sigue verde sin cambios; su `respuestaConSolapamiento` comparte aula y docente, así que sigue
  siendo un conflicto real).
- Suite completa igual que antes del arreglo: **48/49**, único rojo `test_solver_benchmark`
  (fixture sin tipos que migra T10). Ningún otro test cambia de estado.
- `git diff --stat`: `test/backend/test_validador_salida.cpp` y
  `src/backend/src/services/ValidadorSalidaSolver.cpp` (más `tasks.md` y este journal). Sin tocar
  frontend, `spec.md`, `plan.md` ni `estado.md`.

