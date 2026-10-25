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
