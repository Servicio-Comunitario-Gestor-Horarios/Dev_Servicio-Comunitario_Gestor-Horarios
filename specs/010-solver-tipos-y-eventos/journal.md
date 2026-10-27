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


## T6 - Causas de un resultado no factible: saturación por tipo y por referencia (2026-10-25)

**Requisito:** RF-6 (causas de un resultado no factible), en `src/backend/src/solver/solver.cpp`
(`causaEntradaInvalida`, `causaConflictoDeAula` y la nueva `causaSaturacionPorTipo`).
**Base:** `06fc70e` (T1-T5 commiteadas). Sin cambios en el modelo (T4), el validador de salida (T5)
ni la documentación. **No se commitea** (lo verifica el usuario).

### Tests primero (rojo)

Cinco casos nuevos en `test/backend/test_solver_horarios.cpp`:

- `solver_falloCausaConflictoDeAulaPorSaturacionDeTipo`: dos cursos con referencia propia que
  desplazan 5 bloques cada uno al **único** laboratorio (10 bloques frente a 8 franjas).
- `solver_falloCausaConflictoDeAulaPorSaturacionDeTipoEnModoMovil`: mismo escenario con
  `aula_fija = false`.
- `solver_falloCausaConflictoDeAulaPorReferenciaSaturada`: dos cursos comparten la referencia
  Aula 1 y dejan en ella 10 bloques de su tipo frente a 6 franjas.
- `solver_causaDeAulaNoCuentaLasClasesDesplazadas`: los bloques totales compartidos (10) superan
  las 6 franjas de la referencia, pero solo se quedan 6; la causa real es (4).
- `solver_falloTipoSinAulas_esEntradaInvalida`: materia «Laboratorio» sin ningún aula de ese tipo.

Con la producción anterior (HEAD) y los tests nuevos:

```text
$ cmake --build --preset backend --parallel        # exit 0
$ ./build-backend/test/test_solver_horarios
FAIL!  : solver_falloCausaConflictoDeAulaPorSaturacionDeTipo()
          'texto.contains("Causa (2)")' returned FALSE. (… Causa: no se pudo identificar …)
FAIL!  : solver_falloCausaConflictoDeAulaPorSaturacionDeTipoEnModoMovil()   (mismo síntoma)
FAIL!  : solver_falloCausaConflictoDeAulaPorReferenciaSaturada()
          'texto.contains("10 bloques")' returned FALSE. (ve «12 bloques en 6 franjas»: sumaba
          también la clase desplazada)
FAIL!  : solver_causaDeAulaNoCuentaLasClasesDesplazadas()
          '!texto.contains("Causa (2)")' returned FALSE. (reportaba aula «Aula 1» por los 10
          bloques totales)
FAIL!  : solver_falloTipoSinAulas_esEntradaInvalida()
          'texto.contains("Causa (1)")' returned FALSE. (… no se pudo identificar …)
Totals: 23 passed, 5 failed, 0 skipped, 0 blacklisted
```

(Salida completa en `/tmp/opencode/t6/rojo.txt`.)

### Cambios

- **`causaEntradaInvalida` (categoría 1).** Tras las guardas de dimensiones, turno y plan, recorre
  las materias de cada curso y, si un tipo requerido no tiene ninguna aula (comparación por
  `claveTipo`), devuelve entrada inválida nombrando materia, curso y tipo. Es V17 en la llamada
  directa al solver; el tipo sin aulas no se reporta como conflicto de aula.
- **`causaSaturacionPorTipo` (categoría 2, nueva).** Demanda por clave de tipo = bloques de las
  materias de todos los cursos que lo requieren; capacidad = nº de aulas de ese tipo × días ×
  franjas que aportan los turnos de los cursos que lo piden (unión de slots). Si la demanda supera
  la capacidad, mensaje con **tipo visible, demanda y capacidad**. Tipos sin aulas y tipos sin
  franjas se saltan (los juzgan las categorías 1 y 3).
- **`causaConflictoDeAula` (categoría 2).**
  - Modo móvil: se conserva la guarda global y después se aplica la saturación por tipo.
  - Modo aula fija: la comprobación de aulas de referencia compartidas pasa a contar **solo las
    clases que se quedan** en la referencia (materias del tipo del aula); las desplazadas ya no
    inflan el aula de referencia. Se conserva la guarda «si un curso no cabe en su turno, la causa
    es de persona» y, tras el bucle, se aplica la saturación por tipo.
- **Orden de categorías intacto:** entrada inválida → conflicto de aula → persona → asignación.
  La saturación por tipo cede ante la categoría (3) cuando algún curso no cabe en su turno, para
  no ocultar una causa existente.

### Hallazgo durante la implementación (regresión detectada y corregida)

La primera versión de `causaSaturacionPorTipo` hacía que
`solver_falloCausaConflictoDePersona` (4 bloques en un turno de 2 franjas) pasara de categoría (3)
a categoría (2). Se corrigió con la guarda «algún curso no cabe en su turno ⇒ no se informa
saturación por tipo», que devuelve la categoría (3) a ese caso (y también a la guarda `todosCaben`
que ya tenía D8).

### Verde

```text
$ cmake --build --preset backend --parallel        # exit 0
$ ctest --test-dir build-backend --output-on-failure -R '^test_solver_horarios$'
1/1 Test #47: test_solver_horarios .............   Passed    0.19 sec
100% tests passed out of 1

$ ./build-backend/test/test_solver_horarios
Totals: 28 passed, 0 failed, 0 skipped, 0 blacklisted

$ ctest --test-dir build-backend
98% tests passed, 1 tests failed out of 49
	 48 - test_solver_benchmark (Failed)
```

Único rojo: `test_solver_benchmark`, el fixture sin tipos que migra T10. Ningún otro test cambia de
estado (49 configurados, 48 en verde).

### Alcance

`git diff --stat`: `src/backend/src/solver/solver.cpp` (+101/-10) y
`test/backend/test_solver_horarios.cpp` (+159), más `tasks.md` y este journal. Sin tocar el modelo
(T4), `ValidadorSalidaSolver` (T5), frontend, `spec.md`, `plan.md` ni `estado.md`.

## T7 - Enmienda constitucional y supersesión de la 005 (2026-10-25)

**Requisito:** RF-7 y RF-8. **Tarea:** T7 de `tasks.md`. **Commit:** `f054b2e`
(`docs(normativa): enmienda de §5 y §7 por tipos de aula y eventos`), del planner con un parche
aplicado por el coordinator; el implementer solo verifica.

```text
$ git show --stat --oneline f054b2e
f054b2e docs(normativa): enmienda de §5 y §7 por tipos de aula y eventos
 docs/constitution.md                       | 14 ++++----
 specs/005-nucleo-solver/spec.md            | 55 ++++++++++++++++++++++++++++++
 specs/010-solver-tipos-y-eventos/estado.md |  9 +++++
 3 files changed, 72 insertions(+), 6 deletions(-)
```

### Verificación (los dos `grep` del «Hecho cuando»)

```text
$ grep -n "aula de referencia\|V1–V17\|tipos de aula\|evento.*metadata" docs/constitution.md
34:- Los datos del usuario en SQLite (planes de estudio, docentes, aulas, tipos de aula, asignaturas,
    cursos, turnos, eventos y configuración del solver) nunca se borran ni se sobrescriben sin
    confirmación explícita.
64:5. **Aula.** … cada curso declara un **aula de referencia**: la usa cuando su tipo coincide, y
    las clases cuya materia requiere otro tipo se imparten en un aula de ese tipo y el curso
    **vuelve** a la referencia en las demás. Mientras un curso tiene una clase desplazada en una
    franja, su aula de referencia queda **vacía y bloqueada** en esa franja. …
78:- **Cursos:** … el modo de aula fija es **global** … activo → el aula del curso es su **aula de
    referencia** (§7.1.5: desplazamiento y vuelta) …
89:Antes de construir el modelo se ejecutan las validaciones V1–V17; … **V1** espera la sección de
    **eventos** (no la de recesos); el contenido de V1, V2, V16 y V17 vive en la spec 005 y en la 010.
93:… El **evento** de turno es metadata que el solver ignora; el horario del profesor se deriva por
    post-proceso.

$ grep -n "eventos" specs/005-nucleo-solver/spec.md
495:> **tipo de aula** de aulas y materias y la sección de **eventos**.
573:> un aula del centro, en los dos modos) y cambia **V1** (sección de eventos y versión 2.0). …
582:    > secciones y exige la sección de **eventos** del turno; ya **no** exige la sección `recesos`.
584:    > `eventos`, `planes`, `cursos`, `profesores`, `materias` y `aulas`.
589:    > temporalidad de los eventos** (`nombre`, turno existente, posición, orden, duración y par
```

Los cinco puntos de la enmienda (§5 dominios protegidos con tipos de aula y eventos, §7.1.5
invariante del aula, §7.3 contrato de configuración con turno/plan obligatorios, §7.4 rango
`V1–V17` con V1 hablando de eventos, §7.5 «el evento es metadata») y la anotación de supersesión
de la 005 están en el commit. **T7 no ha requerido cambios del implementer**:
`git status --short` en ese momento no mostraba `docs/constitution.md` ni `specs/005-*/spec.md`
modificados, así que la tarea se marca por enmienda ya aplicada.

## T8 - Diagrama entidad-relación completo (2026-10-25)

**Requisito:** RF-9. **Plan:** §1.4, §8-T8 y decisión 11. **Base:** `f054b2e` (T1-T7 commiteadas;
suite 48/49, rojo único `test_solver_benchmark`). **No se commitea.**

Tarea solo documental: no hay runner ni lógica que pueda romperse, así que la verificación es la del
«Hecho cuando» (grep) más una validación real de sintaxis Mermaid.

### Modelo tomado de la fuente

No se inventó nada: el esquema se copió de `src/backend/src/database/migracion.cpp` (T1 de la 009),
que es el estado real tras la 009.

- `TiposAula` (líneas 26-42): `id`, `nombre` (`CHECK(trim(nombre)=nombre AND length>0)`), `clave`
  (`CHECK(length>0)`) y **`CREATE UNIQUE INDEX TiposAula_clave ON TiposAula(clave)`**. La clave es la
  forma normalizada que calcula `ServicioTiposAula` (recorte + plegado), tal como comenta el esquema;
  se anota en el diagrama (`text clave UK "…normalizado…"`).
- `Aulas.id_tipo_aula` (línea 52) y `Materias.id_tipo_aula` (línea 76): `INTEGER NOT NULL
  REFERENCES TiposAula(id)`, **obligatorias**; se dibujan como `TiposAula ||--o{ Aulas` y
  `TiposAula ||--o{ Materias` (un tipo, cero o muchas aulas/materias; cada aula/materia exige
  exactamente un tipo).
- `Turnos` (líneas 128-138): `nombre TEXT PRIMARY KEY`, `inicio`, `fin`, `slots`
  (`CHECK(slots>0)`).
- `Eventos` (líneas 140-159): `id PK`, `nombre`, `turno REFERENCES Turnos(nombre)`, `despues_de_slot`
  (`>=1`), `orden` (`>=1`), `duracion` (`>0`), `inicio`/`fin` con `CHECK((inicio IS NULL) = (fin IS
  NULL))` y **`UNIQUE(turno, despues_de_slot, orden)`**. El diagrama lleva el único en el comentario
  de `despues_de_slot` («UK junto con turno y orden») y la relación `Turnos ||--o{ Eventos`.
  **Cascada:** el esquema **no** declara `ON DELETE CASCADE`; la cascada es de servicio
  (`ServicioTurnosEventos.cpp:402`: «Los eventos referencian el turno sin ON DELETE CASCADE: se borran
  primero»). Se dibuja la relación con la etiqueta «aloja (los eventos se borran con el turno)» para
  no afirmar una cascada de FK que no existe.
- `Cursos` (líneas 161-176): `id`, `anio` (`>=1`), `seccion` (1 carácter), `turno REFERENCES
  Turnos(nombre)` **nullable**, `aula REFERENCES Aulas(id)` **nullable**, `UNIQUE(anio, seccion)`.
  Se dibujan `Turnos |o--o{ Cursos` y `Aulas |o--o{ Cursos` (el aula es la **referencia** del curso).
- **Relación con `PlanEstudio`:** no hay FK persistida. En el esquema, `PlanEstudio_Materia.curso` es
  el año del plan y el plan **del curso** viaja en el preset, no en la base (005: «el plan del curso
  **no** se persiste»). Por eso la relación se dibuja **no identificante** (`Cursos }o..o{
  PlanEstudio: "plan del curso (no persistido)"`, línea discontinua) en lugar de inventar una FK.

Se conservan todas las entidades y relaciones previas (`Profesores`, `Disponibilidad_Profesor`,
`Profesor-Materia`, `PlanEstudio`, `PlanEstudio-Materia`, `Aulas`, `Materias`) y el estilo del
documento: un único bloque de código mermaid con `erDiagram`, sin prosa alrededor; solo se
añadieron comentarios `%%` de cabecera y comentarios de atributo.

### Verificación

```text
$ grep -nE "TiposAula|Eventos|Turnos|Cursos|id_tipo_aula|clave" docs/Diagrama-MER.md
5:    %% reflejan REFERENCES reales y las UK los índices/claves únicos.
7:    TiposAula {
10:        text clave UK "nombre normalizado (recorte + plegado Unicode); índice único TiposAula_clave"
21:        int id_tipo_aula FK "obligatoria -> TiposAula.id"
39:        int id_tipo_aula FK "obligatoria -> TiposAula.id"
76:    Turnos {
85:    Eventos {
88:        text turno FK "-> Turnos.nombre"
98:    Cursos {
102:        text turno FK "opcional -> Turnos.nombre"
108:    TiposAula ||--o{ Aulas: clasifica
109:    TiposAula ||--o{ Materias: "es requerido por"
115:    Turnos ||--o{ Eventos: "aloja (los eventos se borran con el turno)"
116:    Turnos |o--o{ Cursos: agrupa
117:    Aulas |o--o{ Cursos: "es aula de referencia de"
118:    Cursos }o..o{ PlanEstudio: "plan del curso (no persistido)"
```

Sintaxis Mermaid comprobada con la herramienta real (no a ojo):

```text
$ npx -y @mermaid-js/mermaid-cli -i docs/Diagrama-MER.md -o /tmp/opencode/mer/MER.svg
Found 1 mermaid charts in Markdown input
 ✅ ./MER-1.svg

$ npx -y @mermaid-js/mermaid-cli -i docs/Diagrama-MER.md -o /tmp/opencode/mer/MER.png --size 2400
 ✅ ./MER-1.png
```

Comprobaciones sobre el SVG/PNG generados (para descartar que Mermaid se comiera entidades o
relaciones en silencio):

```text
entidades renderizadas: 11   (TiposAula, Aulas, Profesores, Materias, Disponibilidad_Profesor,
                              Profesor-Materia, PlanEstudio, PlanEstudio-Materia, Turnos, Eventos,
                              Cursos)
relaciones sólidas:     10
relaciones discontinuas: 1   (Cursos..PlanEstudio)
comentarios de atributo: 68  (incluye los dos «obligatoria -> TiposAula.id»)
cardinalidades:          markers onlyOne / zeroOrOne / oneOrMore / zeroOrMore presentes
```

El PNG se revisó visualmente: los 11 recuadros, sus columnas PK/FK/UK y las etiquetas de relación se
leen sin solapes.

Suite sin cambios (solo documentación):

```text
$ ctest --test-dir build-backend
98% tests passed, 1 tests failed out of 49
	 48 - test_solver_benchmark (Failed)      # fixture sin tipos, T10
```

`test_solver_benchmark` ya estaba rojo en la base `f054b2e` y es de T10; ningún otro test cambia de
estado.

### Cambios

- `docs/Diagrama-MER.md`: añadidas `TiposAula` (con `clave` UK y su índice único), `id_tipo_aula` FK
  obligatoria en `Aulas` y `Materias`, `Turnos`, `Eventos` (posición/orden/duración y el único
  `(turno, despues_de_slot, orden)`) y `Cursos` (FK de turno y aula, único `(anio, seccion)`); las
  relaciones con `TiposAula`, `Turnos`, `Aulas` y la relación no persistida con `PlanEstudio`.
  Entidades y relaciones previas intactas.
- `specs/010-solver-tipos-y-eventos/tasks.md`: T7 y T8 marcadas con su nota.
- Sin tocar código, `spec.md`, `plan.md` ni `estado.md`. `git diff --stat`: `docs/Diagrama-MER.md` y
  `specs/010-solver-tipos-y-eventos/tasks.md`, más este journal.

### Hallazgos

- **El MER no puede mostrar una cascada de FK en `Eventos`.** El esquema de la 009 no declara `ON
  DELETE CASCADE`; la cascada (dominio `turno`) es de `ServicioTurnosEventos` y del registro de
  pendientes. Se refleja como etiqueta de la relación, no como comportamiento declarativo de SQLite.
- **`Cursos` no tiene FK a `PlanEstudio`.** El plan del curso vive en el preset (005, I1); la única
  huella persistida es `PlanEstudio_Materia.curso` (año). La relación se dibuja discontinua y
  etiquetada «no persistido» para que el diagrama no sugiera una columna que no existe.
- **`id_tipo_aula` es `NOT NULL`** en ambas tablas: se dibuja `||` en el lado de `TiposAula`
  (exactamente uno), no `|o`.

## T9 - Documentación viva del contrato 2.0 (2026-10-25)

**Requisito:** RF-10. **Plan:** §1.4, §2, §8-T9 y decisión 11. **Base:** `4134209` (T1-T8
commiteadas). **No se commitea.**

Tarea solo documental: no hay runner ni lógica que pueda romperse, así que la verificación es la del
«Hecho cuando» (grep) más una **carga real** del ejemplo con `CargadorConfiguracionSolver`.

### Documento creado

`docs/contrato-solver-2.0.md` (raíz de `docs/`, **fuera de `docs/solver/`**, que
`docs/constitution.md` §7 declara histórico y no obligatorio). Estructura:

1. **Versión 2.0 única** — tabla declaración → resultado → mensaje, comprobación de la versión
   **antes** que las secciones, mensaje literal del cargador con **archivo + versión + acción**,
   sellado de `2.0` al escribir, sin conversión y sin tipos por defecto, mención de
   `datos-prueba/preset-generacion-31059.json` como preset 1.0 que hay que regenerar.
2. **Forma del contrato** — tabla de las 10 secciones obligatorias + `aula_fija` opcional con sus
   validaciones, y un apartado por sección (dimensiones, franja, turnos con `inicio`/`fin`,
   planes, cursos con `aula` como índice y referencia, profesores, materias/aulas con `tipo`).
3. **Tipos por nombre y clave** — `tipo` obligatorio (V16), V17 en los dos modos, `claveTipo` =
   `trimmed()` + `toCaseFolded()`, nombre visible conservado, sin sinónimos/jerarquías/acentos.
4. **Eventos** — campos y reglas V2 (posición `1..slots`, orden `>=1`, duración `>0`, par
   inicio/fin completo o ausente, `fin-inicio = duracion`, ventana de turno y de posición, solapes y
   cronología), identidad fuera, `recesos` ignorado como campo desconocido.
5. **Semántica del aula de referencia** — referencia, coincidencia de tipo, desplazamiento y
   vuelta, referencia **vacía y bloqueada** solo en franjas con clase desplazada, exclusividad de las
   desplazadas, no factible → conflicto de aula, modo móvil, y tabla de lecturas erróneas.
6. **Salida con aula real** — `asignaciones[].aula` es el índice real (fragmento con dos aulas en el
   mismo día de un curso fijo) y P3/P4 de `ValidadorSalidaSolver` operan sobre él.
7. **Ejemplo mínimo válido 2.0** — 1 curso, 1 docente, 2 materias de tipos distintos (referencia
   «Aula de estudio» + «Laboratorio»), 1 evento.
8. **Contraejemplo 1.0** rechazado, con el mensaje del cargador y remisión al fixture versionado
   `test/data/preset-1.0.json`.
9. **Referencias** — 005, 010 (y su `plan.md` §2), 009, `docs/interfaz-frontend.md` §3.2 y **§5**,
   MER y constitución §7.

No se duplica la guía de servicios: la API del frontend se remite a `docs/interfaz-frontend.md`.
No se escribió nada en `docs/solver/` (verificado con `git status --porcelain docs/solver/`, vacío).

### Verificación

```text
$ grep -nE "2\.0|eventos|aula de referencia|tipo|1\.0" docs/contrato-solver-2.0.md
1:# Contrato 2.0 del solver — referencia viva
20:## 1. La versión es 2.0 y solo 2.0
22:El campo `version` es obligatorio y su valor debe ser exactamente `"2.0"`...
27:| `"version": "2.0"` | se acepta y se valida el resto (V1–V17) | — |
28:| `"version": "1.0"` | **entrada inválida** | «V1: versión de configuración no reconocida ('1.0')…
34:- **No hay conversión.** Un preset 1.0 no se reinterpreta, no se completa con tipos por defecto…
38:  > La configuración de «/ruta/al/preset.json» declara la versión «1.0»; solo se acepta la versión
65:| `cursos` | arreglo | año, sección, turno, plan, aula de referencia | …
164:### 2.7 `materias` y `aulas`: los tipos
177:- **`tipo` es obligatorio en toda aula y en toda materia** (**V16**)…
212:Los eventos (recesos, actos, intervalos) son **metadata**: el solver **los ignora**…
244:## 5. Semántica del aula de referencia (modo `aula_fija: true`)
258:4. **Referencia vacía y bloqueada.** Mientras el curso tiene una clase **desplazada**…
289:verdad**, también en modo de aula fija (fragmento; la raíz del archivo lleva además `metadata`):
```
(`…` = línea recortada para el journal; el `grep` completo devuelve 91 líneas.)

Conteo por término (cubre los cinco puntos del «Hecho cuando»): `2.0` ×19, `eventos` ×20,
`aula de referencia` ×10, `aula real` ×4, `1.0` ×13. `wc -l`: 423 líneas.

**El ejemplo mínimo es válido de verdad.** Se extrajo el bloque ```json de §7 a
`/tmp/ejemplo-2.0.json` y se cargó con un verificador temporal (`/tmp/valida_preset.cpp`,
compilado fuera del repo contra `build-backend/src/backend/libbackend.a` y `CargadorConfiguracionSolver`,
sin tocar el árbol):

```text
$ python3 -m json.tool /tmp/ejemplo-2.0.json > /dev/null && echo OK
OK
$ /tmp/valida_preset /tmp/ejemplo-2.0.json
ACEPTADA: version=2.0 aulas=2 materias=2 cursos=1 eventos=1 toJson.version=2.0
exit=0
```

El contraejemplo de §8, por el mismo cargador:

```text
$ /tmp/valida_preset /tmp/contraejemplo-1.0.json
RECHAZADA: La configuración de «/tmp/contraejemplo-1.0.json» declara la versión «1.0»; solo se
acepta la versión 2.0. Genere una configuración 2.0 desde los datos con la versión actual.
exit=1
```

El texto pegado en el documento (§1 y §8) es **el literal** de
`CargadorConfiguracionSolver.cpp:59-68`.

```text
$ git status --short
?? docs/contrato-solver-2.0.md
$ git diff --stat
(ninguna línea: el archivo es nuevo y aún no está rastreado)
$ git status --porcelain docs/solver/
(vacío: no se tocó ningún documento histórico)
```

Superficie acotada a `docs/contrato-solver-2.0.md` (nuevo), `tasks.md` y este journal.

### Hallazgos

- **`slots_por_turno` no se valida ni se usa.** Solo se parsea y se serializa; la rejilla efectiva de
  un turno es su lista `slot` y la de un evento se acota por `slot.size()`. El documento lo dice así
  para que nadie lo tome por una validación.
- **`claveTipo` no normaliza acentos, solo caja.** `trimmed()` + `toCaseFolded()`
  (`contrato_solver.cpp:29`): `"Física"` = `"FÍSICA"` pero `"Fisica"` ≠ `"Física"`. T3 lo describe
  como «normaliza acentos»; el comportamiento real y el documentado es **plegado de caja**, y así se
  escribió (§3). No es un defecto de código, pero conviene no prometer equivalencia `Fisica/Física`.
- **`capacidad <= 0` no genera aviso.** P3 solo compara cuando `capacidad > 0`
  (`ValidadorSalidaSolver.cpp:150`); documentado en §2.7 y §6.
- **El ejemplo se validó por el camino de producción completo** (cargador → `fromJson` → V1–V17), no
  solo por `json.tool`; no hizo falta CLI nueva ni tocar tests, porque se compiló un `main` efímero en
  `/tmp` enlazado contra la librería ya construida.

## T10 - Regresión global y benchmark con tipos (2026-10-25)

**Requisito:** RNF-2 (benchmark < 60 s), RNF-5 (suite backend sin frontend), RNF-6 (trazabilidad).
**Plan:** §1.3 (estrategia fija del benchmark), §8-T10 y §10 (riesgo de degradación con `y` para todos
los cursos). **Base:** `9c5c301` (T1-T9 commiteadas; único rojo `test_solver_benchmark`). **No se
commitea.**

### Rojo de partida (el que había que cerrar)

```text
$ ctest --test-dir build-backend --output-on-failure -R '^test_solver_benchmark$'
1/1 Test #48: test_solver_benchmark ............***Failed    0.84 sec
QDEBUG : ...benchmark_solverGrandeResuelve() Tiempo total: 55 ms
QDEBUG : ...benchmark_solverGrandeResuelve() Status: FALLO
QDEBUG : ...benchmark_solverGrandeResuelve() Errores: QList("El modelo no tiene solución factible
  (INFEASIBLE).", "Causa: no se pudo identificar una categoría concreta; ...", ...)
FAIL!  : ... 'resultado.exito' returned FALSE.
0% tests passed, 1 tests failed out of 1
```

### Cambio 1 - fixture del benchmark a la estrategia de `plan.md` §1.3

`test/backend/test_solver_benchmark.cpp` (`crearConfigGrande`), solo fixture y rótulos; **ni una línea
de modelo, contrato, validador ni spec**:

- `version` de `"1.0"` a `"2.0"` (migración obligatoria del contrato).
- `dimensiones.num_aulas` 5 → **6**.
- **6 aulas**: índices 0-4 con tipo `"Aula común"` (0 `Aula 101`/30, 1 `Aula 102`/25, 2 `Aula
  202`/35, 3 `Aula 201`/30, 4 `Aula 203`/100) y el índice **5 `Laboratorio`/20**. El antiguo
  «Auditorio» era el índice 4 y pasa a `Aula 203`, como fija el plan.
- **15 materias tipadas**: todas `"Aula común"` salvo `Tecnologia` (índice **14**), que exige
  `"Laboratorio"`. Es la única materia de laboratorio del fixture.
- **Referencias por turno sin repetir aula** (sin cambios de índices respecto del fixture anterior):
  mañana 1A→0, 1B→1, 2A→2, 3A→3, 4A→4; tarde 2B→0, 3B→1, 4B→2. Ninguna aula se comparte dentro de
  un turno y solo 4B (plan `Ciencias`) desplaza sus 3 bloques de Tecnología.
- El `qDebug` de cabecera dice ahora «6 aulas».

Las dos aserciones que pedía el plan ya existían y se mantienen: `QVERIFY(resultado.exito)` y
`QVERIFY(elapsed < 60000)`.

**Decisión no cubierta por el plan (solo nombres del fixture):** el plan fija índices y tipos pero no
qué nombre recibe el aula del índice 2 (antes `Lab Ciencias`, ahora debe ser común). Se renombra a
`Aula 202` y se deja `Aula 201` en su índice 3 de siempre, para no duplicar nombres; capacidades
conservadas del fixture anterior (y 20 para el laboratorio). Ningún nombre interviene en el modelo.

### Evidencia de que el fixture ejercita de verdad la regla de tipo

Instrumentación **temporal** (`qDebug` de cada asignación, retirada antes de la evidencia final; el
diff entregado no la contiene) sobre la solución del benchmark:

```text
TEMP curso indice 7 turno "tarde"
TEMP  dia 0 slot 3 materia 3 aula 2
TEMP  dia 0 slot 4 materia 0 aula 2
TEMP  dia 0 slot 5 materia 3 aula 2
TEMP  dia 1 slot 4 materia 14 aula 5     <- Tecnologia en el laboratorio
TEMP  dia 2 slot 4 materia 3 aula 2
TEMP  dia 2 slot 5 materia 0 aula 2
TEMP  dia 3 slot 3 materia 14 aula 5     <- Tecnologia en el laboratorio
TEMP  dia 3 slot 5 materia 14 aula 5     <- Tecnologia en el laboratorio
TEMP  dia 4 slot 4 materia 0 aula 2
```

Curso índice 7 = 4B (tarde, `Ciencias`): sus **3** clases de Tecnología van al aula **5** y las demás
vuelven a la referencia **2**. Los otros siete cursos usan su referencia en **todas** sus clases
(índices 0,1,2,3,4, y 0/1 en la tarde). El antiguo `Lab Ciencias` estaba en el índice 2 y habría hecho
que 2A y 4B compartieran «laboratorio»; con el laboratorio en el índice 5 la referencia 2 queda libre
en las franjas desplazadas de 4B (su única otra usuaria, 2A, es de mañana).

### Cambio 2 - nota de no-generabilidad del seed

`docs/guias/datos-prueba.md` §6 (límites), viñeta nueva tras «No genera presets del solver»: con la
regla de tipo de la 010 el conjunto sembrado **no es generable** (≈104 bloques de ciencias frente a
1 laboratorio × 40 franjas), la 009 ya retiró la promesa, no se intenta generar y `test_generacion_seed`
queda fuera del preset `backend`; la generabilidad se recuperará con los **datos reales del liceo**.

### Verificación

```text
$ cmake --build --preset backend --parallel
[1/4] Automatic MOC for target test_solver_benchmark
[2/3] Building CXX object test/CMakeFiles/test_solver_benchmark.dir/backend/test_solver_benchmark.cpp.o
[3/3] Linking CXX executable test/test_solver_benchmark
exit=0
```

```text
$ ctest --test-dir build-backend --output-on-failure -R '^test_solver_benchmark$'
1/1 Test #48: test_solver_benchmark ............   Passed    0.32 sec
100% tests passed out of 1

$ ./build-backend/test/test_solver_benchmark
QDEBUG : ...benchmark_solverGrandeResuelve() Dimensiones: 10 prof, 15 materias, 6 aulas, 8 cursos, 5 dias, 6 slots
QDEBUG : ...benchmark_solverGrandeResuelve() Tiempo total: 135 ms
QDEBUG : ...benchmark_solverGrandeResuelve() Status: EXITO
QDEBUG : ...benchmark_solverGrandeResuelve() Total asignaciones: 80
Totals: 4 passed, 0 failed, 0 skipped, 0 blacklisted, 268ms
```

**Tiempo del solver: 135 ms** (tope 60 000 ms; 0,8 s de reloj CTest incluye arranque de Qt). La
primera corrida tras el build dio **144 ms**; el orden de magnitud se mantiene muy por debajo del
límite, así que el riesgo de §10 (más variables por la `y` de todos los cursos) no se materializa.

```text
$ ctest --preset backend
...
48/49 Test #48: test_solver_benchmark ..............   Passed    0.32 sec
49/49 Test #49: test_ipc_aislamiento_bd ............   Passed    0.08 sec

100% tests passed out of 49
Total Test time (real) =   5.65 sec

$ ctest --test-dir build-backend -N
Total Tests: 49
$ ctest --test-dir build-backend -N | grep -E "frontend|generacion_seed"
(sin test_frontend_conversion ni test_generacion_seed)
```

```text
$ grep -n "generab" docs/guias/datos-prueba.md
141:- **El conjunto sembrado no es generable con la regla de tipo (spec 010).** Desde la 010 toda clase se
146:  resoluble del solver. La **009 ya retiró la promesa de generabilidad** del seed y la 010 no la
148:  del preset `backend`. La generabilidad se recuperará con los **datos reales del liceo** (decisión del

$ git diff --stat
 docs/guias/datos-prueba.md             |  9 ++++++++
 test/backend/test_solver_benchmark.cpp | 38 ++++++++++++++++++++--------------
 2 files changed, 32 insertions(+), 15 deletions(-)
```

### Hallazgos

- **El benchmark es el único punto de la suite que ejercitaba la escala grande; ahora también la regla
  de tipo.** Sin el aula de laboratorio en un índice propio, el fixture anterior no probaba la
  exclusividad entre clases desplazadas; con 4B desplazando 3 bloques al aula 5 sí lo hace.
- **80 asignaciones** en la solución (10 bloques por curso × 8 cursos): el fixture está saturado al
  100 % de las franjas útiles (5 días × 3 franjas por turno), así que es un caso exigente y no un
  escenario holgado.
- **`test_backend_variables_cp_sat`** es el nombre real del test de variables (el plan y `tasks.md`
  lo llaman `test_variables_cp_sat`); no se toca el nombre, solo se anota para futuras búsquedas.

## Aviso · las garantías de las correcciones 1-5 están superadas (2026-10-25)

> **Leer antes de las cinco secciones de corrección que siguen (correcciones 1 a 5).** Las
> afirmaciones de garantía que aparecen en ellas —«nunca inventa un conflicto de aula»,
> «estructuralmente ≤ lo contado», «el descuento no excede las franjas sumadas»— están **superadas**:
> el cálculo que describen sigue mezclando unidades (el palomar cuenta pares día×franja y el descuento
> compara con valores de franja), así que con una semana de 2 o más días puede descontar de más y
> declarar causa (2) cuando la causa real es (4). El defecto, con el contraejemplo reproducible, está
> en `specs/010-solver-tipos-y-eventos/estado.md` → «Validación 6 · defecto abierto».
>
> El historial **no se borra**: cada sección queda como registro de lo que se probó y de por qué se
> cambió. Pero ninguna de esas frases de garantía debe citarse como vigente, y las afirmaciones
> optimistas que ya se corrigieron dentro de cada sección no son la última palabra sobre la materia.
> El usuario decidió **rediseñar el diagnóstico de causas** en una spec nueva (pendiente de abrir) en
> vez de seguir parchando la heurística; esa spec es la que sustituirá a estas cinco correcciones.

## Corrección de validación · causa de referencia bloqueada (2026-10-25)

**Hallazgo 1 de la validación de la 010 (RF-6).** `causaConflictoDeAula` (D8) contaba solo los
bloques que se quedan en el aula de referencia y no descontaba las franjas que las clases
desplazadas del grupo dejan bloqueadas; `causaSaturacionPorTipo` tampoco. Una entrada válida podía
ser INFEASIBLE y quedarse con «no se pudo identificar una categoría concreta».

### Cambio (solo `src/backend/src/solver/solver.cpp`)

- **D8 (aula de referencia compartida):** capacidad disponible = `días × franjas del grupo` menos
  `min(bloques desplazados del grupo, capacidad)`. La causa (2) se informa si los bloques que se
  quedan superan esa capacidad disponible. El bloqueo es **1 franja por bloque desplazado** (RF-3).
  Si no hay bloques que se queden (`total <= 0`), la referencia no compite y no se informa causa: el
  descuento nunca convierte en conflicto un aula que nadie usa.
- **Saturación por tipo:** se restan de la capacidad del tipo las franjas bloqueadas en las
  referencias **de ese tipo**, limitadas a las franjas que el tipo puede usar (`días × slots de la
  referencia ∩ franjas del tipo`), para no penalizar franjas fuera del turno.
- Los mensajes de causa (2) conservan su forma cuando no hay bloqueo y añaden «N quedan bloqueadas
  por clases desplazadas» cuando lo hay; el orden de categorías (1 → 2 → 3 → 4) no cambia.

### Tests (primero en rojo)

```text
$ ctest --test-dir build-backend --output-on-failure -R '^test_solver_horarios$'   # ANTES del cambio
FAIL!  : TestSolverHorarios::solver_causaDeAulaCuentaLasFranjasBloqueadasPorDesplazamientos()
  'texto.contains("Causa (2)")' returned FALSE.
  El modelo no tiene solución factible (INFEASIBLE).
  Causa: no se pudo identificar una categoría concreta; revise horas, aulas y disponibilidad de docentes.
  Entorno: 2 docentes, 2 cursos, 2 materias, 2 aulas.
Totals: 29 passed, 1 failed, 0 skipped, 0 blacklisted, 141ms
```

Reproduce el escenario del reviewer (A1 compartida; 1|A 2 comunes + 4 de laboratorio; 2|B 3
comunes; 2 días × 3 franjas; A1 + laboratorio). Añadidos a `test/backend/test_solver_horarios.cpp`:

1. `solver_causaDeAulaCuentaLasFranjasBloqueadasPorDesplazamientos` — exige causa (2) y el aula
   «A1», y que **no** aparezca «no se pudo identificar» (el caso rojo de arriba).
2. `solver_causaDeAulaConCapacidadSuficienteNoInventaElConflicto` — control negativo: mismo dato con
   una segunda aula común como referencia de 2|B (contraprueba del reviewer) → `exito` y sin errores.
3. `solver_causaSaturacionPorTipoDescuentaLasFranjasBloqueadas` — el descuento por tipo: el conflicto
   no se ve referencia a referencia (A1 sin bloques que se queden; A2 al 100 %), solo al restar del
   tipo las 6 franjas que 1|A bloquea; sin el descuento el caso se quedaba sin causa.

**Test existente ajustado (misma causa):** `solver_causaDeAulaNoCuentaLasClasesDesplazadas` cambia
de 3 a 6 días (12 franjas) y su comentario. Su fixture era **realmente infactible por aula** con la
regla corregida (1|A llenaba las 6 franjas y bloqueaba A1 en 4; 2|B solo cabía en 2), así que la
causa (2) que aparecía era legítima; con la semana más ancha el aula vuelve a tener capacidad tras
el descuento y el test sigue exigiendo la causa (4) que le da nombre, sin cambiar su intención de
fondo (los bloques desplazados no se cuentan como bloques que se quedan).
**Corrección 2 (2026-10-25):** con ese fixture el test ya **no discrimina** la regla «las clases
desplazadas no se cuentan como ocupantes» (los 10 bloques caben en 12 franjas aun contándolas como
ocupantes, así que el veredicto no cambia). Se renombró a
`solver_causaDeAulaConSemanaAnchaNoInventaElConflicto` y su comentario declara qué comprueba y qué
test discrimina la regla; ver «Corrección 2 · unión de franjas bloqueadas».

```text
$ cmake --build --preset backend --parallel
ninja: no work to do.        # build previo de esta misma corrección: exit 0

$ ctest --test-dir build-backend --output-on-failure -R '^test_solver_horarios$'
1/1 Test #47: test_solver_horarios .............   Passed    0.21 sec
100% tests passed out of 1
(PASS de los tres casos nuevos y del ajustado; Totals: 31 passed, 0 failed)

$ ctest --preset backend
49/49 Test #49: test_ipc_aislamiento_bd ............   Passed    0.08 sec
100% tests passed out of 49
Total Test time (real) =   5.93 sec
```

```text
$ git diff --stat
 src/backend/src/solver/solver.cpp     |  63 +++++++++++++++---
 test/backend/test_solver_horarios.cpp | 119 ++++++++++++++++++++++++++++++++--
 2 files changed, 167 insertions(+), 15 deletions(-)
```

### Hallazgos

- **Cota pedida, no exacta.** Se cuenta 1 franja bloqueada por bloque desplazado, como indica el
  hallazgo, y se limita al turno del curso que desplaza. Es una **cota conservadora**: si dos cursos
  del mismo grupo desplazan en la misma franja (posible con ≥2 aulas del tipo desplazado), la franja
  se descuenta dos veces y el cálculo puede declarar un conflicto que la colocación real evitaría.
  No se implementa la cota exacta (requeriría un matching de colocación) porque el hallazgo fija la
  regla; queda anotado para el reviewer. El guardia `total <= 0` evita que el descuento convierta en
  conflicto una referencia que nadie usa.
- **`causaSaturacionPorTipo` ya estaba en el camino** de los dos modos; el cálculo nuevo solo actúa
  con `aula_fija` y cursos con referencia.
- **V8** obliga a que todo curso declare aula en modo fijo, así que no hay referencias ausentes que
  bloqueen por tipo.

## Corrección 2 · unión de franjas bloqueadas (2026-10-25)

**Hallazgos 1 y 2 de la segunda validación (RF-6).** Alcance estricto:
`src/backend/src/solver/solver.cpp`, `test/backend/test_solver_horarios.cpp` y este journal. Base
`d7728e6` (árbol limpio, `ctest --preset backend` 49/49). **No se commitea.**

### Hallazgo 2 · el bloqueo de la referencia es una unión de franjas, no una suma

`causaConflictoDeAula` (D8) descontaba **un bloqueo por cada bloque desplazado** de cada curso del
grupo y `causaSaturacionPorTipo` hacía lo mismo respecto de las referencias del tipo. Si dos cursos
que comparten aula de referencia se desplazan **en la misma franja** (posible cuando hay ≥ 2 aulas
del tipo desplazado), esa franja se descontaba dos veces y se declaraba causa (2) cuando el bloqueo
real no era de aulas.

Caso del reviewer: 2 cursos comparten `A1`, cada uno 2 bloques comunes + 2 de laboratorio, 2
laboratorios, turno 2 días × 3 franjas. Los 4 bloques que se quedan en `A1` caben en `6 − 2 = 4`
franjas porque ambos pueden desplazarse a las mismas 2 franjas (cada uno a un laboratorio distinto).
Con el docente de laboratorio para ambos el dato es **factible**; si el docente de `2|B` no cubre el
laboratorio, la causa real es **(4)**.

#### Cambio (solo producción)

- **Nueva `franjasBloqueadasPorDesplazamientos`** (anónima, junto a los helpers de `solver.cpp`):
  recibe los bloqueos del grupo (`BloqueoDesplazado`: bloques desplazados y slots de su turno) y
  devuelve el número de franjas `(día, slot)` **distintas** de la unión. Procesa primero el curso que
  más franjas necesita y **reutiliza** las ya bloqueadas por otro curso del grupo. Cada curso queda
  limitado a `min(bloques desplazados, días × franjas de su turno)` porque un curso no tiene dos
  clases en la misma franja. **Corregido en la corrección 4:** la unión **no** era una cota
  inferior —es la cuenta de una colocación voraz concreta— y la frase «nunca declara más bloqueo
  del que el modelo puede evitar» era falsa; el término se eliminó y hoy el helper es solo el
  palomar por tipo.
- **D8 (`causaConflictoDeAula`)**: `bloqueadas = franjasBloqueadasPorDesplazamientos(grupo)` en vez
  de la suma de bloques desplazados. El resto (capacidad = `días × franjas del grupo`, guardas
  `todosCaben` y `total <= 0`, mensajes) no cambia.
- **Saturación por tipo**: los bloqueos se **agrupan por aula de referencia**; dentro de una
  referencia se aplica la unión y entre referencias distintas del mismo tipo las franjas se siguen
  acumulando. El filtro por `d.franjas` (slots que el tipo puede usar) se aplica a las franjas del
  bloqueo, como antes.

#### Tests primero (rojo)

Fixture compartido `configDosCursosDesplazadosALaMismaFranja()` (caso exacto del reviewer) y dos
casos nuevos en `test/backend/test_solver_horarios.cpp`:

1. `solver_causaDeAulaNoDescuentaDosVecesLaMismaFranja` — positivo: mismo dato con docente de
   laboratorio para ambos cursos → `exito`.
2. `solver_falloDocenteNoElegibleNoSeInformaComoConflictoDeAula` — el docente de `2|B` solo opta por
   la materia común (no por el laboratorio) → `Causa (4)` y **ninguna** `Causa (2)`.

Con la producción anterior (suma de bloques desplazados) el caso 2 estaba en rojo:

```text
$ cmake --build --preset backend --parallel          # exit 0
$ ctest --test-dir build-backend --output-on-failure -R '^test_solver_horarios$'
FAIL!  : TestSolverHorarios::solver_falloDocenteNoElegibleNoSeInformaComoConflictoDeAula()
  'texto.contains("Causa (4)")' returned FALSE. (El modelo no tiene solución factible (INFEASIBLE).
  Causa (2) conflicto de aula: el aula «A1» no da abasto para las clases que se quedan en ella
  (4 bloques en 6 franjas; 4 quedan bloqueadas por clases desplazadas).
  Entorno: 2 docentes, 2 cursos, 2 materias, 3 aulas.)
Totals: 32 passed, 1 failed, 0 skipped, 0 blacklisted, 148ms
```

El positivo `solver_causaDeAulaNoDescuentaDosVecesLaMismaFranja` pasa en rojo y en verde: el modelo
es factible y las causas no se evalúan; su papel es fijar que la colocación en la misma franja existe
(hay 2 laboratorios), que es lo que hace legítima la unión.

#### Verde

```text
$ cmake --build --preset backend --parallel          # exit 0 (ninja: no work to do)
$ ctest --test-dir build-backend -R '^test_solver_horarios$'
1/1 Test #47: test_solver_horarios .............   Passed    0.20 sec
100% tests passed out of 1

$ ./build-backend/test/test_solver_horarios
Totals: 33 passed, 0 failed, 0 skipped, 0 blacklisted, 144ms

$ ctest --preset backend
49/49 Test #49: test_ipc_aislamiento_bd ............   Passed    0.08 sec
100% tests passed out of 49
Total Test time (real) =   5.65 sec
```

#### Regresiones temporales (todas restauradas)

1. **Volver a contar por suma** (mutación del helper: devolver la suma de bloques desplazados por
   curso, que es el comportamiento anterior). Falla exactamente el caso nuevo, con la causa (2)
   falsa, y **sigue pasando** el caso de saturación real del reviewer
   (`solver_causaDeAulaCuentaLasFranjasBloqueadasPorDesplazamientos`): la unión no pierde ese caso.

   ```text
   FAIL!  : solver_falloDocenteNoElegibleNoSeInformaComoConflictoDeAula()
     'texto.contains("Causa (4)")' returned FALSE. (… Causa (2) … el aula «A1» … 4 bloques en 6
     franjas; 4 quedan bloqueadas por clases desplazadas …)
   PASS   : solver_causaDeAulaCuentaLasFranjasBloqueadasPorDesplazamientos()
   Totals: 32 passed, 1 failed
   ```

2. **Contar las clases desplazadas como ocupantes del aula de referencia en lugar de como
   bloqueo** (la rama `else` suma al `total` y deja de sumar a `bloqueo.bloques`), la regresión
   alternativa del hallazgo 1: fallan **tres** tests, uno de ellos S1 (reproducido sobre `232bd19`):

   ```text
   FAIL!  : solver_falloCausaConflictoDeAulaPorReferenciaSaturada()
     'texto.contains("10 bloques")' returned FALSE. (Causa (2) … el aula «Aula 1» … 12 bloques en
     6 franjas)                                            ← «12» = 10 que se quedan + 2 desplazadas
   FAIL!  : solver_causaDeAulaCuentaLasFranjasBloqueadasPorDesplazamientos()
     'texto.contains("5 bloques en 6 franjas; 4 quedan bloqueadas")' returned FALSE.
     (Causa (2) … el aula «A1» … 9 bloques en 6 franjas)   ← «9» = 6 de 1|A (2 + 4) + 3 de 2|B
   FAIL!  : solver_falloDocenteNoElegibleNoSeInformaComoConflictoDeAula()
     'texto.contains("Causa (4)")' returned FALSE. (… Causa (2) …)
   Totals: 30 passed, 3 failed
   ```

   (Corrección de la anotación anterior, que decía «fallan dos tests … 31 passed, 2 failed»: era
   falsa. Si además de contarlas como ocupantes se mantiene el bloqueo, el reparto es 27/6; la
   mutación con la que se midió —y la que da 30/3— sustituye el bloqueo por la ocupación.)

3. Tras restaurar ambas mutaciones: build exit 0, `test_solver_horarios` 33/33 y suite 49/49 (arriba).

El caso S1 del reviewer (`solver_causaDeAulaCuentaLasFranjasBloqueadasPorDesplazamientos`: 1|A con 2
comunes + 4 de laboratorio, 2|B con 3 comunes, `A1` + 1 laboratorio, 2 días × 3 franjas) **sigue
dando causa (2)**: 5 bloques que se quedan frente a `6 − 4 = 2` disponibles. Se le añadió la
aserción del mensaje `«5 bloques en 6 franjas; 4 quedan bloqueadas»`, que fija el número de la unión
(solo desplaza 1|A; sus 4 franjas se cuentan una vez). **Corrección:** la frase anterior —«habría
fallado con cualquier suma»— era falsa: con la mutación de suma del punto 1 este caso sigue pasando
(4 bloques desplazados = 4 franjas con un solo curso desplazándose), así que no discrimina suma vs
unión; quien lo hace es `solver_falloDocenteNoElegibleNoSeInformaComoConflictoDeAula`.

### Hallazgo 1 · test que ya no discriminaba

`solver_causaDeAulaNoCuentaLasClasesDesplazadas` (ampliado a 6 días en `aa7ee74`) pasaba también con
la regresión que cuenta las clases desplazadas como ocupantes del aula de referencia: 10 bloques
(6 que se quedan + 4 desplazados) caben en las 12 franjas, así que el veredicto no cambiaba y el
nombre del test y el journal afirmaban más de lo que probaba. Se eligió la opción (b):

- Renombrado a **`solver_causaDeAulaConSemanaAnchaNoInventaElConflicto`**, con un comentario que dice
  lo que comprueba (con capacidad de sobra el aula no inventa la causa (2) y se informa la (4) de
  2|B) y lo que **no** comprueba.
- **Qué test discrimina «no se cuentan las clases desplazadas»**: hoy
  `solver_falloCausaConflictoDeAulaPorReferenciaSaturada`, porque exige el mensaje `«10 bloques»`
  (solo las clases que se quedan) y la regresión de ocupantes lo convierte en `«12 bloques»` (ver
  regresión temporal 2), y el nuevo
  `solver_falloDocenteNoElegibleNoSeInformaComoConflictoDeAula`, donde la causa (2) taparía la (4).
- Párrafo del journal de `aa7ee74` corregido (remite a esta sección).

### Hallazgos

- **La unión no era una cota: era una colocación voraz.** El helper reutilizaba franjas ya
  bloqueadas del grupo (con el mismo turno devolvía `max(bloques desplazados por curso)`) sin buscar
  la colocación óptima. **Corregido en la corrección 4:** eso **no es una cota inferior** —es la
  cuenta de una colocación concreta— y con turnos distintos **sobreestima** (reparte los bloques de
  cada curso en franjas diferentes y descuenta más de las que el modelo necesita). La frase «prefiere
  no inventar bloqueo antes que declarar causa (2) falsa» era una intuición, no una propiedad. Hoy el
  helper es **solo el palomar por tipo** (ver corrección 4).
- **Grupos por referencia en la saturación por tipo.** Antes se sumaba por curso; ahora se agrupa por
  `c.aula`, así que dos cursos que comparten referencia comparten franjas bloqueadas, pero dos
  referencias distintas del mismo tipo siguen sumando (cada una pierde su propia capacidad).
- **`solver_causaDeAulaConSemanaAnchaNoInventaElConflicto` se conserva** como control de que el
  descuento por bloqueo no inventa conflictos; solo cambia el nombre y el comentario.
- **Sin cambios** en el modelo (T4), `ValidadorSalidaSolver` (T5), frontend, `spec.md`, `plan.md` ni
  `estado.md`.

## Corrección 3 · cota inferior por tipo (2026-10-25)

**Hallazgo pendiente de la tercera validación (RF-6 / criterio 6).** Alcance estricto:
`src/backend/src/solver/solver.cpp`, `test/backend/test_solver_horarios.cpp` y este journal. Base
`232bd19` (árbol limpio, `ctest --preset backend` 49/49). **No se commitea.**

### Hallazgo · la unión pura era optimista con pocas aulas del tipo desplazado

`franjasBloqueadasPorDesplazamientos` devolvía solo la **unión** de franjas: suponía que todas las
clases desplazadas del grupo caben en las mismas franjas. Eso exige que haya aulas suficientes del
tipo desplazado. Si hay **menos aulas que clases que las necesitan**, no caben: el aula de
referencia pierde más franjas y una entrada válida puede ser INFEASIBLE quedándose en «no se pudo
identificar una categoría concreta».

Caso R2 del reviewer: 5 días × 6 franjas = 30; dos cursos comparten `A1`, cada uno 14 bloques
comunes + 2 de laboratorio; **1 solo laboratorio**. Los 4 bloques de laboratorio necesitan 4
franjas distintas (una clase por franja y aula), así que `A1` pierde 4 y solo le quedan 26 para los
28 bloques comunes: infactible **por aula**. Con 2 laboratorios el mismo dato es factible.

### Cambio (solo producción)

La estimación del bloqueo pasa a incluir el **palomar por tipo**:

```cpp
// franjasBloqueadasPorDesplazamientos(config, cursos, numDias)
//   total = unión de (día, slot) alcanzables por las clases desplazadas del grupo
//   para cada tipo t:  total = max(total, ⌈desplazados_que_requieren_t / aulas_de_t⌉)
```

- **Por qué el palomar es seguro:** el palomar da el mínimo de franjas que necesitan k clases con A
  aulas del tipo (una por franja y aula), así que el bloqueo real no puede ser menor. **Corregido en
  la corrección 4:** el otro término, la unión, **no era cota inferior** (es una colocación voraz
  concreta y puede sobreestimar), así que el `max` de los dos **no** era una cota inferior y la
  frase «nunca inventa un conflicto de aula» era falsa para el conjunto. Hoy solo queda el palomar,
  que sí es cota inferior; puede quedarse corta, y eso es lo que hay que decir.
- **`BloqueoDesplazado` gana `bloquesPorTipo`**: los bloques desplazados de cada curso agrupados
  por la clave del tipo de aula que requieren. El helper suma el mapa del grupo y divide por el
  **nº de aulas del centro de ese tipo** (no del grupo). Si el tipo no tiene aulas (0) no se divide:
  V17 ya lo rechaza como entrada inválida.
- **D8 (`causaConflictoDeAula`)** usa la cota inferior en lugar de la unión pura; **la saturación
  por tipo** (`causaSaturacionPorTipo`) la aplica también, agrupando por aula de referencia y
  sumando entre referencias distintas del mismo tipo, igual que antes.
- **Sin cambios** en el modelo (T4), en el resto de causas, en el frontend, en `spec.md`,
  `plan.md`, `estado.md` ni `tasks.md`.

### Tests primero (rojo)

Fixture compartido `configDosCursosCasiLlenosConLaboratorios(int laboratorios)` (caso R2: 5 días ×
6 franjas, `A1` compartida, 14 + 2 bloques por curso) y dos casos nuevos:

1. `solver_causaDeAulaCuentaLasFranjasQueNoCabenEnUnSoloLaboratorio` — 1 laboratorio → causa (2)
   con el aula y las cifras (`«28 bloques en 30 franjas; 4 quedan bloqueadas»`).
2. `solver_causaDeAulaConLaboratoriosSuficientesNoInventaElConflicto` — 2 laboratorios, mismo dato
   → `exito` (control de que la cota no inventa el conflicto).

Con la producción anterior el caso 1 estaba en rojo (y el 2 ya pasaba):

```text
$ cmake --build --preset backend --parallel          # exit 0
$ ./build-backend/test/test_solver_horarios
FAIL!  : solver_causaDeAulaCuentaLasFranjasQueNoCabenEnUnSoloLaboratorio()
  'texto.contains("Causa (2)")' returned FALSE. (El modelo no tiene solución factible (INFEASIBLE).
  Causa: no se pudo identificar una categoría concreta; revise horas, aulas y disponibilidad de
  docentes.  Entorno: 2 docentes, 2 cursos, 2 materias, 2 aulas.)
PASS   : solver_causaDeAulaConLaboratoriosSuficientesNoInventaElConflicto()
Totals: 34 passed, 1 failed, 0 skipped, 0 blacklisted, 201ms
```

### Verde

```text
$ cmake --build --preset backend --parallel          # exit 0 (ninja: no work to do)
$ ctest --test-dir build-backend -R '^test_solver_horarios$'
1/1 Test #47: test_solver_horarios .............   Passed    0.26 sec
100% tests passed out of 1

$ ./build-backend/test/test_solver_horarios
Totals: 35 passed, 0 failed, 0 skipped, 0 blacklisted, 197ms

$ ctest --preset backend
100% tests passed out of 49

$ git diff --stat -- src/backend/src/solver/solver.cpp test/backend/test_solver_horarios.cpp
 src/backend/src/solver/solver.cpp     | 87 +++++++++++++++++++++++++----------
 test/backend/test_solver_horarios.cpp | 59 ++++++++++++++++++++++++
 2 files changed, 122 insertions(+), 24 deletions(-)
```

### Números de los tres escenarios (antes → después)

| Escenario | Antes (unión pura) | Después (max unión/palomar) |
|---|---|---|
| **R2**, 1 laboratorio: 28 bloques en `A1`, 4 de laboratorio, 30 franjas | unión 2 → 28 disponibles ≥ 28 → INFEASIBLE **sin categoría** | palomar ⌈4/1⌉ = 4 → 26 disponibles < 28 → **causa (2)**: «28 bloques en 30 franjas; 4 quedan bloqueadas» |
| **R2**, 2 laboratorios (contraprueba) | 2 franjas → factible | palomar ⌈4/2⌉ = 2 → 28 disponibles ≥ 28 → **exito** (sin cambio) |
| **S3b**, 2 laboratorios, docente de laboratorio no elegible para 2\|B | unión 2 → causa (4) | palomar ⌈4/2⌉ = 2 → causa (4), **sin** causa (2) |
| **S1**, 1 laboratorio: 5 bloques en `A1`, 4 desplazados | unión 4 → causa (2) «5 bloques en 6 franjas; 4 quedan bloqueadas» | unión 4, palomar ⌈4/1⌉ = 4 → la misma causa (2) con la misma cifra |

**Corrección 4:** el término de la unión ya no existe. Las cifras de esta tabla siguen valiendo
porque en R2 y S1 el palomar ya dominaba el `max` y en S3b la unión valía lo mismo que el palomar
(2); el caso que distinguía la unión del palomar era otro (turnos distintos), y ahí la unión
sobreestimaba.

### Regresiones temporales (todas restauradas)

1. **Volver a la unión pura** (anular el término del palomar). Falla exactamente el caso R2 nuevo,
   con el mensaje «no se pudo identificar»; S1 y S3b siguen pasando (su bloqueo real ya lo daba la
   unión). `Totals: 34 passed, 1 failed`.
2. **Volver a la suma** (que el helper devuelva la suma de bloques desplazados, comportamiento
   previo a la corrección 2). Falla exactamente S3b
   (`solver_falloDocenteNoElegibleNoSeInformaComoConflictoDeAula`): causa (2) falsa
   «… (4 bloques en 6 franjas; 4 quedan bloqueadas …)» en lugar de la (4). El caso R2 sigue en
   verde con la suma (también declara causa (2): 4 franjas bloqueadas dejan 26 para 28 bloques),
   así que R2 no discrimina suma vs cota; S3b sí. `Totals: 34 passed, 1 failed`.
3. Tras restaurar ambas mutaciones: build exit 0, `test_solver_horarios` 35/35 y suite 49/49
   (arriba).

### Hallazgos

- **Cota inferior, no colocación exacta.** El término del palomar ignora que otras aulas del mismo
  tipo pueden estar ocupadas por otros grupos o por clases que se quedan; el modelo real puede
  necesitar aún más franjas. Se prefiere quedarse corto antes que declarar causa (2) falsa, que es
  el defecto que se corrige. Un caso que siga INFEASIBLE y sin categoría con la cota es posible,
  pero ya no es el R2: haría falta una interacción que la cota no modela (p. ej. competencia entre
  tipos por las mismas franjas).
- **El nº de aulas es el del centro.** Para el término del palomar se cuentan todas las aulas del
  tipo en el centro, incluidas las que son referencia de otros cursos: es la opción optimista
  (cota inferior) y evita sobreestimar. Un tipo sin aulas (0) no se divide: V17 lo rechaza antes.
- **Sin cambios en la unión** (histórico, superado por la corrección 4). El helper conservó la
  reutilización de franjas de la corrección 2; solo se le añadió el `max` con el palomar.
  `solver_causaDeAulaCuentaLasFranjasBloqueadasPorDesplazamientos` (S1) fija que la unión no se
  pierde. Hoy la unión está eliminada y S1 sigue verde porque solo desplaza un curso (su palomar
  ⌈4/1⌉ = 4 coincide con las 4 franjas que contaba la unión).
- **La saturación por tipo** hereda el mismo `max`; el caso
  `solver_causaSaturacionPorTipoDescuentaLasFranjasBloqueadas` sigue en verde con sus cifras.
- **Correcciones a este journal (detectadas por el reviewer):** la regresión de «contar las
  desplazadas como ocupantes» da **30/3** (no 31/2) y entre las que fallan está S1 con «9 bloques en
  6 franjas»; la frase «habría fallado con cualquier suma» era falsa (S1 pasa con la mutación de
  suma). Ver la sección de la corrección 2, ya corregida.

## Corrección 4 · solo palomar (2026-10-25)

**Hallazgo de la cuarta validación (RF-6 / criterio 6).** Alcance estricto:
`src/backend/src/solver/solver.cpp`, `test/backend/test_solver_horarios.cpp` y este journal. Base
`45ac292` (árbol limpio, `ctest --preset backend` 49/49). Decisión del usuario: quedarse **solo**
con el palomar. **No se commitea.**

### Defecto · el término de la unión no era una cota inferior

`franjasBloqueadasPorDesplazamientos` devolvía `max(unión voraz, palomar por tipo)` y el comentario
afirmaba que ambos términos eran cotas inferiores. **La unión no lo era:** coloca las clases
desplazadas de cada curso con un método voraz (primero el que más franjas necesita, reutilizando las
ya bloqueadas) y cuenta las franjas de **esa colocación concreta**. Si los cursos del grupo tienen
**turnos distintos**, la colocación voraz no puede reutilizar franjas que el modelo sí puede
compartir, así que sobreestima el bloqueo, descuenta de más en el aula de referencia y declara una
causa (2) **falsa** que tapa la real.

Caso del reviewer (entrada válida para V8/V16/V17): `dimensiones={2,2,3,2,2,4}`, turnos
`T1={0,1,2,3}` y `T2={2,3}`, aulas `A1 (Aula común), Lab1, Lab2`, materias `Matematica (Aula común)`
y `Laboratorio (Laboratorio)`, planes `Plan A{{0,1,4},{1,1,2}}` y `Plan B{{0,2,2},{1,2,2}}`, cursos
`1|A` (T1) y `2|B` (T2) con la **misma referencia** `A1`, `Prof A` con `{0,1}` y `Prof B` con `{0}`
(sin laboratorio).

- **Antes:** `Causa (2) conflicto de aula: el aula «A1» no da abasto para las clases que se quedan en
  ella (6 bloques en 8 franjas; 4 quedan bloqueadas por clases desplazadas).` — la unión contaba 4
  franjas: 1|A tomaba dos de `T1` y 2|B no podía reutilizarlas porque solo alcanza `{2,3}`.
- **Después:** causa **(4)** (`la materia «Laboratorio» del curso 2|B no tiene ningún docente
  elegible`). El palomar ⌈4 bloques de laboratorio / 2 laboratorios⌉ = **2** deja 6 franjas de `A1`
  para los 6 bloques que se quedan, así que el aula no es la causa.
- **Control positivo:** con `Prof B` cubriendo el laboratorio (`{0,1}`) la misma entrada da `exito`
  (antes y después).

### Cambio (solo producción)

El helper se queda **solo con el palomar**:

```cpp
// Clases desplazadas de un curso, agrupadas por el tipo de aula que requieren.
using BloquesDesplazadosPorTipo = QMap<QString, int>;

// franjasBloqueadasPorDesplazamientos(config, cursos) =
//     max_t ⌈bloques desplazados del grupo que requieren t / nº de aulas del CENTRO de tipo t⌉
int franjasBloqueadasPorDesplazamientos(const SolverConfig& config,
                                        const QVector<BloquesDesplazadosPorTipo>& cursos) {
    QMap<QString, int> desplazadosPorTipo;
    for (const BloquesDesplazadosPorTipo& c : cursos)
        for (auto it = c.constBegin(); it != c.constEnd(); ++it)
            desplazadosPorTipo[it.key()] += it.value();

    int total = 0;
    for (auto it = desplazadosPorTipo.constBegin(); it != desplazadosPorTipo.constEnd(); ++it) {
        int aulas = 0;
        for (const AulaSolverConfig& a : config.aulas)
            if (claveTipo(a.tipo) == it.key()) ++aulas;
        if (aulas <= 0) continue;   // V17: entrada inválida, no conflicto de aula
        total = qMax(total, (it.value() + aulas - 1) / aulas);   // ⌈k / A⌉
    }
    return total;
}
```

- **Garantía:** con A aulas del tipo t, k clases desplazadas que lo requieren no caben en menos de
  ⌈k/A⌉ franjas distintas (una clase por franja y aula): el palomar es una cota inferior **por
  capacidad del tipo**. Formulación corregida en la corrección 5 (antes decía «el descuento nunca
  inventa un conflicto de aula» sin limitar de qué franjas se descuenta): **el descuento se limita
  a las franjas contadas en la capacidad; el palomar es cota inferior**. Sin extrapolaciones.
- **Límite, dicho en el comentario:** la cota **puede quedarse corta** —la colocación real puede
  necesitar más franjas por competencia con otros grupos, con las clases que se quedan o por turnos
  que no se solapan—, de modo que una entrada infactible puede quedarse **sin** causa (2). Antes se
  anunciaba lo contrario («nunca inventa», pero sin admitir que podía no detectar); ahora la
  propiedad que se declara es la del palomar.
- **Código muerto eliminado sin restos:** fuera la colocación voraz (`stable_sort`, `QSet<QPair<int,int>>
  bloqueadas`), los campos `bloques` y `franjas` de `BloqueoDesplazado` (el struct pasa a
  `using BloquesDesplazadosPorTipo = QMap<QString, int>`), el parámetro `numDias` del helper y los
  `#include <QPair>` / `#include <algorithm>`, que solo servían a la unión. Los dos call sites
  (`causaConflictoDeAula` D8 y `causaSaturacionPorTipo`) construyen el mapa por curso; un curso sin
  bloques desplazados se detecta con `isEmpty()` en vez de con `bloques > 0`.
- **Comentarios corregidos:** el del helper, el de D8 y el de la saturación por tipo ya no hablan de
  unión ni de «cota optimista»; dicen que la garantía es el palomar y que puede quedarse corta.
- **Sin cambios** en el modelo (T4), en `ValidadorSalidaSolver`, en el orden de categorías, en los
  mensajes, en el frontend, en `spec.md`, `plan.md`, `estado.md` ni `tasks.md`.

### Tests primero (rojo)

Fixture `configDosCursosConTurnosDistintos()` (caso exacto del reviewer) y dos casos nuevos:

1. `solver_causaDeAulaNoSobreestimaConTurnosDistintos` — `Prof B` con `{0}` → causa (4), **sin**
   causa (2), con `2|B` y «Laboratorio» en el mensaje.
2. `solver_causaDeAulaConTurnosDistintosNoInventaElConflicto` — control positivo con `Prof B` con
   `{0,1}` → `exito` sin errores.

Con la producción de `45ac292` el caso 1 estaba en rojo (y el 2 ya pasaba):

```text
$ cmake --build --preset backend --parallel          # exit 0
$ ./build-backend/test/test_solver_horarios
FAIL!  : TestSolverHorarios::solver_causaDeAulaNoSobreestimaConTurnosDistintos()
  'texto.contains("Causa (4)")' returned FALSE. (El modelo no tiene solución factible (INFEASIBLE).
  Causa (2) conflicto de aula: el aula «A1» no da abasto para las clases que se quedan en ella
  (6 bloques en 8 franjas; 4 quedan bloqueadas por clases desplazadas).
  Entorno: 2 docentes, 2 cursos, 2 materias, 3 aulas.)
PASS   : TestSolverHorarios::solver_causaDeAulaConTurnosDistintosNoInventaElConflicto()
Totals: 36 passed, 1 failed, 0 skipped, 0 blacklisted, 209ms
```

### Verde

```text
$ cmake --build --preset backend --parallel          # exit 0
$ ctest --test-dir build-backend -R '^test_solver_horarios$'
1/1 Test #47: test_solver_horarios .............   Passed    0.27 sec
100% tests passed out of 1

$ ./build-backend/test/test_solver_horarios
Totals: 37 passed, 0 failed, 0 skipped, 0 blacklisted, 209ms

$ ctest --preset backend
100% tests passed out of 49
Total Test time (real) =   5.73 sec

$ git diff --stat
 specs/010-solver-tipos-y-eventos/journal.md |  47 +++++++----
 src/backend/src/solver/solver.cpp           | 118 +++++++++-------------------
 test/backend/test_solver_horarios.cpp       |  59 ++++++++++++++
 3 files changed, 128 insertions(+), 96 deletions(-)
```

### Escenarios antes → después (mensajes medidos)

| Escenario | Antes (unión + palomar) | Después (solo palomar) |
|---|---|---|
| **Turnos distintos** (nuevo): 6 bloques en `A1`, 4 de laboratorio, 2 laboratorios, `Prof B` sin laboratorio | causa (2) «el aula «A1» … (6 bloques en 8 franjas; 4 quedan bloqueadas)» | **causa (4)** «la materia «Laboratorio» del curso 2\|B no tiene ningún docente elegible» |
| **Turnos distintos** (control): `Prof B` con laboratorio | `exito` | `exito` |
| **R2**, 1 laboratorio: 28 bloques en `A1`, 4 de laboratorio, 30 franjas | causa (2) «28 bloques en 30 franjas; 4 quedan bloqueadas» (palomar ⌈4/1⌉ = 4) | **idéntico** |
| **R2**, 2 laboratorios (contraprueba) | `exito` (palomar ⌈4/2⌉ = 2) | **idéntico** |
| **S3b**, 2 laboratorios, `Prof B` sin laboratorio | causa (4) «… curso 2\|B no tiene ningún docente elegible» | **idéntico** |
| **S1**, 1 laboratorio: 5 bloques en `A1`, 4 desplazados | causa (2) «5 bloques en 6 franjas; 4 quedan bloqueadas» (unión 4 = palomar 4) | **idéntico** |

Ninguna aserción de mensaje cambió: R2, S1, S3b y la saturación por tipo conservan sus cifras.

### Regresión temporal (restaurada)

Copiar `git show 45ac292:src/backend/src/solver/solver.cpp` sobre el fichero actual (reintroduce el
término de la unión con el palomar) deja el árbol de tests como está y falla **exactamente** el caso
nuevo, con el defecto del reviewer:

```text
$ ./build-backend/test/test_solver_horarios
FAIL!  : TestSolverHorarios::solver_causaDeAulaNoSobreestimaConTurnosDistintos()
  'texto.contains("Causa (4)")' returned FALSE. (… Causa (2) conflicto de aula: el aula «A1» no da
  abasto para las clases que se quedan en ella (6 bloques en 8 franjas; 4 quedan bloqueadas …).)
Totals: 36 passed, 1 failed, 0 skipped, 0 blacklisted, 206ms
```

Restaurado el `solver.cpp` del palomar: build exit 0, `test_solver_horarios` 37/37 y suite 49/49
(arriba). El resto de casos de aula no cambian con la regresión, así que el nuevo test es el único
que discrimina «unión + palomar» de «solo palomar»; R2/S1/S3b no distinguían ambos términos.

### Hallazgos

- **El palomar sí es cota inferior; la unión no lo era.** El error de la corrección 3 fue agrupar dos
  términos bajo la etiqueta «cota inferior» sin comprobar el primero: la unión es una **colocación
  concreta** (voraz), no un mínimo. Se elimina en lugar de «acotarla», porque cualquier cota basada
  en turnos exige resolver una colocación (matching) que el modelo ya resuelve.
- **Puede quedarse corta y hay que decirlo.** Con el palomar solo, una entrada infactible puede
  quedarse sin causa (2) cuando el bloqueo real excede ⌈k/A⌉ (p. ej. competencia de las mismas
  franjas con otros grupos del centro, o el propio turno del curso que limita la reutilización). Es
  el precio de no volver a declarar causas (2) falsas; los escenarios R2/S1/S3b y el caso del
  reviewer quedan cubiertos.
- **`BloquesDesplazadosPorTipo` no necesita el recuento total de bloques.** El palomar solo mira los
  bloques por tipo; el total (`bloques`/`desplazadosPorCurso`) sigue existiendo donde se usa
  (`causaSaturacionPorTipo` para decidir si un curso desplaza algo) pero ya no viaja al helper.
- **Correcciones a este journal:** (i) corrección 2, «nunca declara más bloqueo del que el modelo
  puede evitar» → **falso**, la unión podía sobreestimar; (ii) corrección 2, hallazgo «la unión es
  una cota optimista» → **falso**, es una colocación voraz sin garantía; (iii) corrección 3, «el
  `max` de dos cotas inferiores es cota inferior … nunca inventa un conflicto de aula» → **falso**
  para el conjunto, solo el palomar lo garantiza; (iv) corrección 3, «sin cambios en la unión» →
  superado: la unión ya no existe.

## Corrección 5 · descuento dentro de las franjas contadas (2026-10-25)

**Hallazgo de la quinta validación (RF-6).** Alcance estricto:
`src/backend/src/solver/solver.cpp`, `test/backend/test_solver_horarios.cpp` y este journal. Base
`5fc40e0` (árbol limpio, `ctest --preset backend` 49/49). **No se commitea.**

### Defecto · se restaban franjas que nunca se sumaron

En `causaSaturacionPorTipo` la **capacidad** de un tipo se contaba con las franjas de los turnos de
los cursos que **requieren** ese tipo (`d.franjas`), pero las **franjas bloqueadas** se contaban
sobre los turnos de los cursos que tienen ese tipo como **aula de referencia**, que pueden ser otros
turnos. El descuento restaba franjas que no estaban en la suma: podía declarar una causa (2) falsa y
tapar la causa real.

Caso del reviewer (entrada **válida** para V7/V10/V13/V14/V17):
`dimensiones={3,2,3,3,1,6}`, `franja={60,6}`, turnos `TB={0,1,2,3}` y `TL={4,5}`; materias
`Ciencias («Aula base»)` y `Laboratorio («Laboratorio»)`; aulas `A1`, `A2 («Aula base»)` y
`L1 («Laboratorio»)`; planes `Plan C1{{1,1,2}}`, `Plan C2{{0,2,4}}` y `Plan C3{{0,3,4}}`; cursos
`1|A` (turno `TL`, referencia `A1`, **solo** laboratorio), `2|B` (turno `TB`, referencia `A2`) y
`3|C` (turno `TB`, referencia `A1`); docentes `Prof Lab` (materia 1, disponibilidad solo en el slot
4), `Prof B` y `Prof C` (materia 0, 4 horas).

- **Capacidad contada** de «Aula base»: 2 aulas × 1 día × `{0,1,2,3}` = **8** franjas.
- **Bloqueo descontado:** solo lo produce `1|A` (2 bloques de laboratorio) en su referencia `A1`,
  y sus clases caen en `TL = {4,5}`, **fuera** de las 4 franjas contadas. El descuento de 2 era
  ficticio: 8 − 2 = 6 < 8 → causa (2) falsa.
- **Antes:** `Causa (2) conflicto de aula: el tipo «Aula base» necesita 8 bloques y sus 2 aulas
  ofrecen 8 franjas (2 las bloquean clases desplazadas).`
- **Después:** causa **(4)** `la materia «Laboratorio» del curso 1|A necesita 2 bloques y ningún
  docente elegible dispone de tantas franjas.` (`Prof Lab` solo tiene el slot 4).
- **Control positivo:** con `Prof Lab` disponible en todo el turno el modelo da `exito` (los 8
  bloques caben en las 8 franjas de «Aula base» y los 2 de laboratorio en `L1`).

### Cambio (solo producción)

El descuento se calcula por grupo de referencia y por tipo, **limitado a las franjas contadas**:

```cpp
struct DesplazamientosDeGrupo {
    QMap<QString, int> bloques;                   // tipo requerido → bloques desplazados
    QMap<QString, QSet<int>> franjasAlcanzables;  // tipo requerido → slots donde pueden caer
};

// cota_t = max(0, ⌈k_t / A_t⌉ − |alcanzables_t \ franjasContadas|), tope en
// |alcanzables_t ∩ franjasContadas|
int franjasBloqueadasEnFranjasContadas(const SolverConfig& config,
                                       const QMap<QString, int>& bloques,
                                       const QMap<QString, QSet<int>>& franjasAlcanzables,
                                       const QSet<int>& franjasContadas) {
    int total = 0;
    for (auto it = bloques.constBegin(); it != bloques.constEnd(); ++it) {
        const int aulas = aulasDelTipo(config, it.key());
        if (aulas <= 0) continue;   // V17: entrada inválida, no conflicto de aula
        const QSet<int> alcanzables = franjasAlcanzables.value(it.key());
        int fuera = 0;
        for (int s : alcanzables)
            if (!franjasContadas.contains(s)) ++fuera;
        const int dentro = alcanzables.size() - fuera;
        const int palomar = (it.value() + aulas - 1) / aulas;              // ⌈k / A⌉
        total = qMax(total, qMin(qMax(palomar - fuera, 0), dentro));
    }
    return total;
}
```

- **Por qué esa fórmula y no un simple tope:** el palomar dice que las clases desplazadas necesitan
  al menos ⌈k/A⌉ franjas distintas; de ellas, como mucho `|alcanzables_t \ franjasContadas|` pueden
  caer fuera de las contadas, así que al menos la diferencia cae dentro. El tope en
  `alcanzables_t ∩ franjasContadas` cubre el caso en que el palomar exceda las franjas alcanzables
  (entrada ya infactible por otra vía). Invariante: **el descuento no excede las franjas sumadas en
  la capacidad**; el palomar sigue siendo cota inferior y puede quedarse corta.
- **`franjasAlcanzables`** es la unión de los turnos de los cursos que aportan los bloques de cada
  tipo en el grupo; se rellena en el mismo recorrido que los bloques.
- **`palomarPorTipo`** queda como helper aparte (sin restricción) para D8, y `aulasDelTipo` evita
  duplicar el recuento de aulas del centro.
- **D8 no necesita la restricción** (comprobado, no supuesto): su capacidad se mide con las franjas
  de los turnos de **todos** los cursos del grupo, también los desplazados, así que
  `alcanzables_t ⊆ franjas` y `fuera = 0`; el tope frente a la capacidad en **franjas-día** lo pone
  el `qMin(bloqueadas, capacidad)` que ya existía. Pasarle el conjunto de slots como
  `franjasContadas` sería un error de unidades: en S1 el bloqueo son 4 clases = 4 franjas-día de
  las 6 del aula, no 4 de los 3 slots del día; se probó y S1 bajó a 3 («5 bloques en 6 franjas; 3
  quedan bloqueadas»), así que ese camino se descartó.
- **Comentarios:** el del helper y el de D8 ya no afirman «nunca inventa un conflicto de aula»;
  dicen que el descuento se limita a las franjas contadas y que el palomar es cota inferior (y que
  puede quedarse corta). Igual el bullet «Garantía» de la corrección 4, reformulado.
- **Sin cambios** en el modelo (T4), `ValidadorSalidaSolver`, el orden de categorías, el frontend,
  `spec.md`, `plan.md`, `estado.md` ni `tasks.md`.

### Tests primero (rojo)

Fixture `configLaboratorioEnTurnoDistinto()` (caso exacto del reviewer) y tres casos nuevos:

1. `solver_causaSaturacionPorTipoNoDescuentaFranjasDeOtroTurno` — variante **alcanzable en
   producción** (V13 solo exige solape, no capacidad): `Prof Lab` con `{1}` y disponibilidad
   `{4}` → causa (4), **sin** causa (2).
2. `solver_causaSaturacionPorTipoNoDescuentaFranjasConDocenteNoElegible` — misma entrada con
   `materias_asignadas` **vacío** (V10/V13 la rechazan en el flujo real; el solver directo debe
   informar causa (4) «no tiene ningún docente elegible», no causa (2)).
3. `solver_causaSaturacionPorTipoConDocenteSuficienteNoInventaElConflicto` — control positivo con
   `Prof Lab` disponible en todo el turno → `exito`.

Con la producción de `5fc40e0` fallaban exactamente los dos negativos (el control ya pasaba):

```text
$ cmake --build --preset backend --parallel          # exit 0
$ ./build-backend/test/test_solver_horarios
FAIL!  : solver_causaSaturacionPorTipoNoDescuentaFranjasDeOtroTurno()
  '!texto.contains("Causa (2)")' returned FALSE. (… Causa (2) conflicto de aula: el tipo «Aula base»
  necesita 8 bloques y sus 2 aulas ofrecen 8 franjas (2 las bloquean clases desplazadas). …)
FAIL!  : solver_causaSaturacionPorTipoNoDescuentaFranjasConDocenteNoElegible()
  '!texto.contains("Causa (2)")' returned FALSE. (… la misma causa (2) …)
PASS   : solver_causaSaturacionPorTipoConDocenteSuficienteNoInventaElConflicto()
Totals: 38 passed, 2 failed, 0 skipped, 0 blacklisted, 253ms
```

### Verde

```text
$ cmake --build --preset backend --parallel          # exit 0
$ ctest --test-dir build-backend -R '^test_solver_horarios$'
1/1 Test #47: test_solver_horarios .............   Passed    0.27 sec
100% tests passed out of 1

$ ./build-backend/test/test_solver_horarios
Totals: 40 passed, 0 failed, 0 skipped, 0 blacklisted, 205ms

$ ctest --preset backend
100% tests passed out of 49
Total Test time (real) =   5.67 sec

$ git diff --stat
 src/backend/src/solver/solver.cpp     | 118 ++++++++++++++++++++++++--------
 test/backend/test_solver_horarios.cpp |  81 ++++++++++++++++++++++++
 2 files changed, 157 insertions(+), 42 deletions(-)
```

### Escenarios antes → después (mensajes medidos)

| Escenario | Antes (`5fc40e0`) | Después |
|---|---|---|
| **Reviewer**, `Prof Lab` con 1 franja (alcanzable en producción) | causa (2) «el tipo «Aula base» necesita 8 bloques y sus 2 aulas ofrecen 8 franjas (2 las bloquean clases desplazadas)» | **causa (4)** «la materia «Laboratorio» del curso 1\|A necesita 2 bloques y ningún docente elegible dispone de tantas franjas» |
| **Reviewer**, `materias_asignadas` vacío | causa (2) (la misma) | **causa (4)** «la materia «Laboratorio» del curso 1\|A no tiene ningún docente elegible» |
| **Reviewer**, control: `Prof Lab` disponible en todo el turno | `exito` | `exito` |
| **R2**, 1 laboratorio: 28 bloques en `A1`, 4 de laboratorio, 30 franjas | causa (2) «A1 … (28 bloques en 30 franjas; 4 quedan bloqueadas por clases desplazadas)» | **idéntico** |
| **R2**, 2 laboratorios (contraprueba) | `exito` | **idéntico** |
| **S3b**, 2 laboratorios, `Prof B` sin laboratorio | causa (4) «… curso 2\|B no tiene ningún docente elegible» | **idéntico** |
| **S1**, 1 laboratorio: 5 bloques en `A1`, 4 desplazados | causa (2) «A1 … (5 bloques en 6 franjas; 4 quedan bloqueadas por clases desplazadas)» | **idéntico** |
| **Saturación por tipo** (T6): «Aula base» 7 bloques, 12 franjas, 6 bloqueadas | causa (2) «… necesita 7 bloques y sus 2 aulas ofrecen 12 franjas (6 las bloquean clases desplazadas)» | **idéntico** |
| **Corrección 4**: turnos distintos, `Prof B` sin laboratorio | causa (4) «… curso 2\|B no tiene ningún docente elegible» | **idéntico** |

### Regresión temporal (restaurada)

Copiar `git show 5fc40e0:src/backend/src/solver/solver.cpp` sobre el fichero actual (descuento sin
restringir) deja los tests como están y falla **exactamente** los dos casos nuevos:

```text
$ ./build-backend/test/test_solver_horarios
FAIL!  : solver_causaSaturacionPorTipoNoDescuentaFranjasDeOtroTurno()
  '!texto.contains("Causa (2)")' returned FALSE. (… «Aula base» necesita 8 bloques y sus 2 aulas
  ofrecen 8 franjas (2 las bloquean clases desplazadas). …)
FAIL!  : solver_causaSaturacionPorTipoNoDescuentaFranjasConDocenteNoElegible()
  '!texto.contains("Causa (2)")' returned FALSE. (… la misma causa (2) …)
Totals: 38 passed, 2 failed, 0 skipped, 0 blacklisted, 203ms
```

Restaurado el `solver.cpp` corregido: build exit 0, `test_solver_horarios` 40/40 y suite 49/49
(arriba). R2, S1, S3b, la saturación por tipo y el caso de la corrección 4 no distinguen ambos
comportamientos; los dos negativos nuevos son los únicos que discriminan.

### Hallazgos

- **La capacidad y el bloqueo deben medirse sobre el mismo conjunto de franjas.** El error de la
  corrección 3 (y de la 4) no era el palomar, sino aplicarlo a un conjunto de franjas distinto del
  que se sumó: capacidad por los turnos de los cursos que **piden** el tipo, bloqueo por los turnos
  de los cursos que lo tienen como **referencia**. Con la restricción, el descuento es
  estructuralmente ≤ lo contado (cada grupo aporta como mucho las franjas de su intersección y hay
  a lo sumo tantos grupos como aulas del tipo).
- **La fórmula conserva el palomar como cota inferior.** Restar las franjas alcanzables que quedan
  fuera (`palomar − fuera`) es más fino que quedarse con la intersección, y sigue sin poder
  descontar de más; el tope `∩` solo actúa cuando el palomar ya excede lo alcanzable (entrada
  infactible por otra vía).
- **D8 no requería el cambio, pero ahora está documentado por qué.** Allí la capacidad y el bloqueo
  salen del mismo grupo de cursos; el tope ya estaba en el `qMin` con la capacidad en franjas-día.
  La lección de unidades (slots vs franjas-día) está anotada arriba para no repetir el intento.
- **Cobertura de las dos variantes de causa (4).** La de disponibilidad insuficiente es la que V13
  deja pasar (solo exige solape), así que era la que podía producir la causa (2) falsa en
  producción; la de `materias_asignadas` vacío se cubre también porque es la que usaban las
  correcciones 2 y 4 para forzar la (4).

## T11 - `FranjaSemana` y funciones puras de capacidad y demanda (2026-10-27)

**Requisito:** RF-6. **Plan:** §3 (M5), §0.7 (nota del 2026-10-27) y §8/T11. **Base:** `295b59e`.

### Rojo (tests primero)

Se creó `test/backend/test_capacidad_tipo.cpp` (7 casos) y su registro
`add_gtest(test_capacidad_tipo backend/test_capacidad_tipo.cpp backend)` en `test/CMakeLists.txt`
(junto a `test_contrato_solver`), con la producción todavía sin las firmas nuevas:

```text
$ cmake --preset backend && cmake --build --preset backend --parallel
test_capacidad_tipo.cpp:64:11: error: ‘FranjaSemana’ no nombra a un tipo
test_capacidad_tipo.cpp:69:17: error: ‘lunesCuarta’ no se declaró en este ámbito
...
ninja: build stopped: subcommand failed.        # exit 1
```

### Verde

```text
$ cmake --build --preset backend --parallel
[69/69] Linking CXX executable test/test_solver_horarios        # exit 0

$ ctest --test-dir build-backend --output-on-failure -R 'test_capacidad_tipo|test_contrato_solver|test_backend_variables_cp_sat'
1/3 Test #42: test_contrato_solver .............   Passed    0.11 sec
2/3 Test #43: test_capacidad_tipo ..............   Passed    0.01 sec
3/3 Test #47: test_backend_variables_cp_sat ....   Passed    0.63 sec

100% tests passed out of 3

$ grep -n "int aulasDelTipo\|QVector<int> slotsTurno" src/backend/src/solver/solver.cpp
(ninguna salida; exit 1)

$ ctest --preset backend
50/50 Test #50: test_ipc_aislamiento_bd ............   Passed    0.07 sec

100% tests passed out of 50
Total Test time (real) =   6.31 sec
```

`test_capacidad_tipo` (7/7, ejecutando el binario):

```text
[       OK ] TestCapacidadTipo.franjaSemana_ordenIgualdadYqHash
[       OK ] TestCapacidadTipo.slotsTurno_devuelveLosSlotsDeclarados
[       OK ] TestCapacidadTipo.franjasDelCurso_cuentaParesDiaFranjaYnoSlotsSueltos
[       OK ] TestCapacidadTipo.franjasDelTipo_uneLosTurnosDeLosCursosQueLoTienen
[       OK ] TestCapacidadTipo.demandaDelTipo_sumaLosBloquesDeTodosLosCursosYNoLosDeOtroTipo
[       OK ] TestCapacidadTipo.aulasYcapacidadDelTipo_seCuentanPorClave
[       OK ] TestCapacidadTipo.tipoGeneralAula_esAulaDeEstudioPorClave
[  PASSED  ] 7 tests.
```

La suite pasa de **49 a 50** tests por el `test_capacidad_tipo` nuevo; los 49 anteriores siguen verdes
(`test_solver_horarios`, benchmark factible y resto incluidos).

### Cambios

- `src/backend/include/backend/solver/config/contrato_solver.hpp` (ámbito global, junto a
  `claveTipo`; el `namespace ContratoSolver` **no** se amplía): `struct FranjaSemana` con `==`, `<`
  y `qHash` (`qHashMulti`); `slotsTurno`, `franjasDelCurso`, `franjasSemanales`, `franjasDelTipo`,
  `demandaDelTipo`, `aulasDelTipo`, `capacidadDelTipo` y `tipoGeneralAula()`. Incluye `<QSet>` y
  `<QHashFunctions>`.
- `src/backend/src/solver/config/contrato_solver.cpp`: se implementan esas funciones puras y se
  retira el `slotsDelTurno` privado (`54-60`); `slotsDisponibles` pasa a `slotsTurno`.
  `tipoGeneralAula()` devuelve `"Aula de estudio"` (una sola constante para V18).
- `src/backend/src/solver/solver.cpp`: se borran **solo** las dos copias anónimas duplicadas
  (`aulasDelTipo`, `33-38`, y `slotsTurno`, `92-98`), como manda T11/§0.7. **Ninguna llamada cambia**:
  los 8 usos resuelven a las funciones públicas (mismas firmas y cuerpos idénticos). El resto del
  inventario de causas (palomar, `franjasBloqueadasEnFranjasContadas`, `causaConflictoDeAula`,
  `nombreAula`, bloque por referencia) sigue siendo de T13. Diff: **17 líneas eliminadas, 0 añadidas**.
- `test/backend/test_capacidad_tipo.cpp` (nuevo) y `test/CMakeLists.txt`.
- `specs/010-solver-tipos-y-eventos/tasks.md`: T11 marcada como hecha.

### Hallazgos

- **La ambigüedad es por *using-directive* + ADL, no por ocultamiento.** El namespace anónimo se
  inyecta en el ámbito global y, con argumentos de tipo global (`SolverConfig`, `QString`), la copia
  anónima y la pública coinciden como candidatas: **8 errores** si se dejan ambas. Borrar solo las
  definiciones (sin tocar llamadas) es suficiente y no altera el comportamiento.
- **`slots` es macro de Qt** (`#define slots`): como nombre de variable local no compila
  (`error: expected unqualified-id before ‘=’`). En `franjasDelCurso` la lista local se llama
  `slotsDelCurso`, como fija el plan.
- **La unidad queda fijada por el tipo, no por un comentario.** `franjasDelCurso` produce pares
  `(día, slot)` distintos (2 días × 2 franjas = 4): el producto manual `num_dias × slots.size()` que
  T13 sustituye por `franjasSemanales` da ahora el mismo número por construcción.
- **`aulasDelTipo` y `franjasDelTipo` comparan por clave** (`claveTipo`), no por cadena cruda: el
  test usa a propósito `"AULA DE ESTUDIO"` en un aula y cuenta 2 aulas del tipo general.

## T12 - V18: aula de referencia del tipo general (2026-10-27)

**Requisito:** RF-3, RF-5. **Plan:** §3.2, §5.18 y §8/T12. **Base:** `227eacd`.

### Rojo (tests primero)

Se añadieron 6 casos en `test/backend/test_validador_entrada.cpp` y el helper
`exigeRechazoV18` (aula, curso, tipo y la mención «tipo general»). Con la producción sin V18, los
dos casos de rechazo fallaban y el resto ya pasaba:

```text
FAIL!  : TestValidadorEntrada::v18_referenciaSobreLaboratorio_falla() '!resultado.ok' returned FALSE.
FAIL!  : TestValidadorEntrada::v18_referenciaSobreTallerConMateriasEspecializadas_falla() '!resultado.ok' returned FALSE.
Totals: 54 passed, 2 failed, 0 skipped, 0 blacklisted, 23ms
0% tests passed, 1 tests failed out of 1     # test_validador_entrada
```

### Verde

```text
$ cmake --build --preset backend --parallel
build exit=0

$ ctest --test-dir build-backend --output-on-failure -R '^test_validador_entrada$'
1/1 Test #23: test_validador_entrada ...........   Passed    0.03 sec
100% tests passed out of 1

$ ctest --preset backend
50/50 Test #50: test_ipc_aislamiento_bd ............   Passed    0.07 sec
100% tests passed out of 50
Total Test time (real) =   5.61 sec
```

Casos V18 (56/56 en el binario):

```text
PASS   : v18_referenciaSobreLaboratorio_falla
PASS   : v18_referenciaSobreTallerConMateriasEspecializadas_falla
PASS   : v18_referenciaGeneralPorClave_pasa
PASS   : v18_aulasMoviles_noAplica
PASS   : v18_sinAulaGeneralEnModoFijo_llegaPorV17
PASS   : v18_sinAulaGeneralEnModoMovil_llegaPorV17
Totals: 56 passed, 0 failed, 0 skipped, 0 blacklisted, 25ms
```

### Dónde se encadena V18 y por qué

`validarReferenciaGeneral` se define **después de `validarTiposConAulas`** en
`solver_config.cpp` y se añade **como última entrada** de la `QStringList errores` de
`SolverConfig::fromJson` (tras `validarTiposConAulas`). Así el orden es **V16 → V17 → V18**: un
centro sin aula del tipo general que además tenga materias que lo requieran se informa por **V17**
(el tipo requerido no tiene aulas) y solo cuando V17 está satisfecho y la referencia es de otro tipo
se informa **V18**. Con `aula_fija == false` la función devuelve vacío (V8 ya ignora la referencia);
un índice de aula fuera de rango se salta porque V6/V8 lo rechazan antes.

Mensaje (una sola fuente, `tipoGeneralAula()`, para que el texto no derive de la convención):

```text
V18: el aula «L1», referencia del curso 1A, es del tipo «Laboratorio»; el aula de referencia debe
ser del tipo general «Aula de estudio». Asigne al curso un aula del tipo general.
```

### Cambios

- `src/backend/src/solver/config/solver_config.cpp`: `validarReferenciaGeneral` (compara
  `claveTipo(aula.tipo)` con `claveTipo(tipoGeneralAula())`) y su registro en la lista de errores.
  Sin columna, sección de contrato ni consulta a la base.
- `test/backend/test_validador_entrada.cpp`: helper `exigeRechazoV18` y los 6 casos (laboratorio,
  taller con materias especializadas, clave con mayúsculas/espacios, móvil, y los dos V17 sin tipo
  general).
- `test/backend/test_contrato_solver.cpp`: **fixture acoplado**. `tipoSeConservaTalCualSeDeclaro`
  convertía el aula de referencia en ` LABORATORIO `, que ahora es entrada inválida por V18
  (mensaje real del fallo: «V18: el aula «A1», referencia del curso 1A, es del tipo «LABORATORIO»…»).
  Se conserva la aserción del nombre visible del aula (sigue siendo `aulas[0]`) y se añade un aula
  general `A2` como referencia del curso (`aula = 1`, `num_aulas = 2`). No se relaja la regla.
- `specs/010-solver-tipos-y-eventos/tasks.md`: T12 marcada como hecha.

### Hallazgos

- **El orden V16 → V17 → V18 exige encadenar V18 al final.** Si se ejecutara antes de V17, un
  centro sin aula general pero con materias que la requieren se informaría por V18 cuando la causa
  concreta (y la acción) es V17.
- **La regla nueva invalida fixtures que usaban una referencia especializada**, no solo presets: el
  caso de conservación de nombre de `test_contrato_solver` era uno. Se ajusta el fixture (aula
  general como referencia) en vez de tocar la validación; es la misma consecuencia que T15 documenta
  para los presets 2.0 anteriores.
- **El tipo general no se duplica en el motor** (§5.18): `causaEntradaInvalida` no comprueba V18 y
  `solver.cpp` no se toca.

## T13 - Causa (2) por capacidad: producción y tests acoplados (2026-10-27)

**Requisito:** RF-6, RNF-3. **Plan:** §0.7, §3, §4.4, §5.14 y §8/T13. **Base:** `736bbcd`.

### Rojo (test de regresión antes de tocar producción)

Se añadió `solver_causaDeAulaContraejemploDelReviewer_esCausaDeDocente` (fixture M6:
`dimensiones = {2,2,2,2,2,7}`, `TB={5,6}`, `TX={4,5}`, `R1` compartida, `Prof Lab` disponible solo en
`(0,4)`) y, con la heurística intacta, **falla con (2)**:

```text
$ ./build-backend/test/test_solver_horarios solver_causaDeAulaContraejemploDelReviewer_esCausaDeDocente
FAIL!  : TestSolverHorarios::solver_causaDeAulaContraejemploDelReviewer_esCausaDeDocente()
  '!texto.contains("Causa (2)")' returned FALSE. (El modelo no tiene solución factible (INFEASIBLE).
Causa (2) conflicto de aula: el tipo «Aula base» necesita 4 bloques y sus 1 aulas ofrecen 4 franjas
(1 las bloquean clases desplazadas).
Entorno: 2 docentes, 2 cursos, 2 materias, 2 aulas.)
Totals: 2 passed, 1 failed, 0 skipped, 0 blacklisted, 2ms
```

### Verde

```text
$ cmake --build --preset backend --parallel
build exit=0

$ ctest --test-dir build-backend --output-on-failure -R '^test_solver_horarios$'
1/1 Test #48: test_solver_horarios .............   Passed    0.26 sec
100% tests passed out of 1

$ ./build-backend/test/test_solver_horarios
Totals: 41 passed, 0 failed, 0 skipped, 0 blacklisted, 205ms

$ grep -n "palomarPorTipo\|franjasBloqueadasEnFranjasContadas\|causaConflictoDeAula\|DesplazamientosDeGrupo\|nombreAula\|claveAula" src/backend/src/solver/solver.cpp
(ninguna salida; exit 1)

$ ctest --preset backend
50/50 Test #50: test_ipc_aislamiento_bd ............   Passed    0.07 sec
100% tests passed out of 50
Total Test time (real) =   5.65 sec
```

El test de regresión ya pasa y devuelve `Causa (4)` sobre `1|X` y «Laboratorio» (sin `(2)`).

### Producción

- **Se eliminan** `DesplazamientosDeGrupo`, `palomarPorTipo`,
  `franjasBloqueadasEnFranjasContadas`, `franjasCurso`, `nombreAula`, `claveAula` y
  `causaConflictoDeAula` (rama móvil + bloque de referencia compartida).
- **`causaSaturacionPorTipo` reescrita**: guard de la excepción de precedencia de RF-6
  (`bloquesCurso > franjasSemanales` → vacío, para que la causa sea (3)); nombre visible del tipo
  desde la primera materia que lo requiere; por cada clave en orden determinista de `QMap`:
  tipo sin aulas → se salta (V17 es (1)), tipo sin franjas → se salta (lo juzga (3)), y
  `demandaDelTipo(t) > capacidadDelTipo(t)` → `Causa (2)` con nombre visible, demanda y capacidad.
  **Sin descuento de bloqueos.**
- `franjasCurso` → `franjasSemanales` (guard de (2) y causa (3)); `primeraCausaNoFactible` llama a
  (2) directamente; el comentario de `causaEntradaInvalida` pasa a **V1–V18**.
- `analizarFallo`: el fallback dice **«Causa: no atribuible. Ninguna categoría de (1) a (4) explica
  el fallo; revise la capacidad de los tipos de aula, las franjas de los turnos y la disponibilidad
  de los docentes.»**

### Los 13 tests de causas: destino real

| Test | Cambio real en T13 |
|---|---|
| `solver_causaDeAulaConSemanaAnchaNoInventaElConflicto` | **sin cambios**; sigue exigiendo (4) |
| `solver_causaDeAulaCuentaLasFranjasBloqueadasPorDesplazamientos` | **reescrito** a «no atribuible» (fixture intacto; nombre para T14) |
| `solver_causaDeAulaConCapacidadSuficienteNoInventaElConflicto` | **sin cambios**; factible |
| `solver_causaDeAulaNoDescuentaDosVecesLaMismaFranja` | **sin cambios**; factible (renombrado en T14) |
| `solver_causaDeAulaCuentaLasFranjasQueNoCabenEnUnSoloLaboratorio` | **reescrito** a «no atribuible» (fixture intacto) |
| `solver_causaDeAulaConLaboratoriosSuficientesNoInventaElConflicto` | **sin cambios**; factible |
| `solver_causaSaturacionPorTipoDescuentaLasFranjasBloqueadas` | **reescrito** a «no atribuible» (fixture intacto, referencia `Cancha`) |
| `solver_causaDeAulaNoSobreestimaConTurnosDistintos` | **sin cambios**; sigue exigiendo (4) |
| `solver_causaDeAulaConTurnosDistintosNoInventaElConflicto` | **sin cambios**; factible |
| `solver_causaSaturacionPorTipoNoDescuentaFranjasDeOtroTurno` | **sin cambios**; sigue exigiendo (4) (renombrado en T14) |
| `solver_causaSaturacionPorTipoNoDescuentaFranjasConDocenteNoElegible` | **sin cambios**; sigue exigiendo (4) (renombrado en T14) |
| `solver_causaSaturacionPorTipoConDocenteSuficienteNoInventaElConflicto` | **sin cambios**; factible |
| `solver_falloDocenteNoElegibleNoSeInformaComoConflictoDeAula` | **sin cambios**; sigue exigiendo (4) |

Fuera de los 13, se ajustan **dos aserciones** (sin tocar fixtures): `solver_falloCausaConflictoDeAula`
asevera el **tipo** `«regular»` en vez del nombre del aula (8 > 4 sigue siendo (2)) y
`solver_falloCausaConflictoDeAulaPorReferenciaSaturada` asevera `«Aula de estudio»` en vez de
«Aula 1» (10 > 6). `...PorSaturacionDeTipo`, `...EnModoMovil`, `solver_falloTipoSinAulas` y
`solver_falloCausaConflictoDePersona` no cambian (la (3) sigue siendo la prueba de la excepción).

### Hallazgos

- **El guard de la excepción es imprescindible en el orden nuevo**: `solver_falloCausaConflictoDePersona`
  tiene demanda 4 > capacidad 2 y, sin el guard, la (2) taparía la (3).
- **«No atribuible» no nombra entidades**: los tres tests reescritos comprueban además que el texto
  no contiene `Causa (` ni el nombre del aula/curso, para que el fallback no derive a un mensaje
  con entidad.
- **El conteo es necesario pero no suficiente**: los dos fixtures de referencia compartida
  (`28 ≤ 30` y `5 ≤ 6`) siguen siendo INFEASIBLE y ahora se informan sin categoría, tal y como
  decide RF-6; no se inventa una (2) que el conteo no prueba.
- **El motor no valida V18** (§5.18): `solver_causaSaturacionPorTipoDescuentaLasFranjasBloqueadas`
  mantiene la referencia de `3|C` en la cancha sin cambios y sigue siendo infactible.
