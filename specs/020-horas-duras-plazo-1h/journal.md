# Journal 020 - Tope duro de horas y plazo de una hora

Base de la rama: `455ba7f651d59e46f633753ccbe8368f0a37c0f9` (`feat/Motor`).
Commit de la spec/plan/tareas (base de implementación): `0801e56` («docs(sdd): spec 020 aprobada y
plan del experimento»).
Rama: `experimento/horas-duras-plazo-1h`. Ejecución del implementer: **2026-11-12**.

> **Aislamiento:** no se toca `feat/Motor`, no se hace merge y **no** se ha ejecutado
> `slow-release` ni el cálculo del centro real (lo lanza el usuario a mano).

---

## T1 - Tope duro: sin exceso, ancla sin `e*`, tope 0 y ausente; V13 intacta

RF-1, RF-4, RF-6, RNF-8.

### Cambios

- `src/backend/include/backend/solver/variables.hpp`: se retira el miembro `exceso` de
  `VariablesSolver` (y su línea de documentación). `z` (`asignacionDocente`) sigue creándose para
  todo candidato elegible: la capacidad de horas no filtra variables.
- `src/backend/src/solver/variables.cpp`: se retira la creación de `e[p]` y la cota de la variable
  (`cotaExceso`). Queda un comentario que remite a la restricción dura.
- `src/backend/src/solver/restricciones/restricciones_turnos.cpp`:
  `restriccionTopeHorasProfesor` pasa a **`AddLessOrEqual(Sum(x[p]), bloquesAula(horas_aula))`**,
  sin término de exceso ni holgura. Un `horas_aula = 0` (declarado o ausente, el parser ya usa
  defecto 0) deja al docente sin clases.
- `src/backend/include/backend/solver/restricciones/restricciones_turnos.hpp`: doc de la regla
  (tope duro).
- `src/backend/include/backend/solver/solver.hpp`: `AnclajeEstructural` se queda **solo** con
  `docentePorMateria` (`z*`); desaparece `excesoPorDocente` (`e*`). Comentario actualizado.
- `src/backend/src/solver/solver.cpp`: `construirObjetivo` ya **no suma S1** (solo S2 y, en su
  fase, la preferencia); `extraerAnclaje`/`fijarAnclaje` operan solo sobre `z*`. Comentarios de
  FASE 1/2a actualizados.
- `src/backend/include/backend/solver/config/contrato_solver.hpp`: `PESO_HORAS` y su
  `static_assert` se conservan **documentados como históricos** (S1 retirado del objetivo);
  `COSTE_SUPLENTE` y `PESO_PREFERENCIA` siguen activos.
- **V13 intacta**: no se añadió ningún filtro de capacidad de horas a la elegibilidad
  (`docenteCandidatoElegible`) ni a `crearVariables`.

### Tests (escritos antes del código; el rojo se comprobó)

| Test | Archivo | Estado |
|---|---|---|
| `solver_topeDuroSinCapacidadEsInfactible` (reescribe `solver_excesoDeHorasEsLaValvulaSinCapacidad`) | `test/backend/test_solver_horarios.cpp` | rojo antes del código (`!sinCapacidad.exito` fallaba: el exceso blando devolvía horario); verde después |
| `solver_topeCeroNoRecibeClases` (nuevo) | `test/backend/test_solver_horarios.cpp` | verde |
| `v13_noFiltraPorCapacidadDeHoras` (nuevo; `horas_aula = 0` y campo ausente ⇒ la entrada pasa) | `test/backend/test_validador_entrada.cpp` | verde |
| `TopeCero_NoFiltraLasVariables` (nuevo; `horas_aula = 0` conserva `x` y `z`) | `test/backend/test_variables_cp_sat.cpp` | verde |
| `horasAulaAusente_seLeeComoCero` (nuevo; `fromJson` sin el campo ⇒ 0) | `test/backend/test_contrato_solver.cpp` | verde |

Ajustes de comentarios en pruebas que describían el comportamiento sustituido
(`solver_titularAlTope_usaAlSuplente`, `solver_d3_bloquesSonHoras_s1_topeSinConversion`): su
aserción sigue siendo válida bajo el tope duro (el titular a tope cede la materia al suplente).
`test_variables_cp_sat.cpp` no contaba variables `e[p]`, así que no hubo recuentos que corregir.

### Verificación

| Suite | Comando | Resultado |
|---|---|---|
| C1 (backend) | `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend` | **62/62 verde**, 0 fallos (19,25 s) |
| C3 (frontend sin OR-Tools) | `cmake -S . -B /tmp/opencode/build-fe-t1b-frontend -G Ninja -DBUILD_BACKEND=OFF -DBUILD_TESTING=OFF && cmake --build /tmp/opencode/build-fe-t1b-frontend --target frontend --parallel` | **verde**: 29/29, `libfrontend.a` enlazada (ver T1b) |

Comprobaciones del «Hecho cuando»:

```
$ grep -R "exceso" src/backend/include/backend/solver/variables.hpp src/backend/src/solver/variables.cpp
(sin resultados)
$ grep -R "cotaExceso\|excesoPorDocente" src/
(sin resultados)
$ grep -R "vars\.exceso" src/
(sin resultados)
```

### Hallazgos

1. **C3 no era reproducible por un defecto previo del build, corregido como T1b.** `build-fe/` está
   generado dentro de Docker (`/workspace`) y su caché no se puede reutilizar en el host. Además,
   `src/frontend/CMakeLists.txt` no añadía `src/common/include` al target `frontend`:
   `src/frontend/src/services/tipos_aula_ui.hpp:19` incluye `common/tipo_general.hpp` y, con
   `BUILD_BACKEND=OFF`, no existe el target `backend` que arrastre esa ruta por herencia
   (`target_link_libraries(backend PUBLIC … common)`), así que fallaba al compilar
   `classroom_list_widget.cpp`. Se reprodujo idéntico en un worktree del commit base `0801e56`.
   Corregido en **T1b**; ver su sección y la evidencia de C3/C1.
2. **La retirada del exceso no rompió la línea base acotada** (RNF-1): las 62 pruebas de backend
   pasan, incluidos los fixtures con docente a tope exacto y los de bloques/aforo. Ninguna prueba
   rápida dependía del exceso para ser factible.
3. **El ancla pierde `e*` sin cambiar la escalera de fases**: la fase 2a/2b siguen fijando solo el
   reparto de docentes; con el tope duro el reparto de la referencia ya es admisible por
   construcción, así que no hace falta anclar holgura alguna. Presupuestos y `max_deterministic_time`
   quedan **sin tocar** (es T5).
4. `test_solver_centro_sin_ef` (etiqueta `slow`) **compila** con el nuevo modelo; **no se ha
   ejecutado**, igual que `test_solver_centro_real` y cualquier cálculo del centro real (RNF-2).

---

## T1b - `common/include` en el target `frontend` (defecto previo de C3)

Soporte de verificación de T1 (RF-4, RNF-7); no cambia comportamiento.

### Defecto previo (confirmado en la base `0801e56`)

`src/frontend/src/services/tipos_aula_ui.hpp` incluye `common/tipo_general.hpp`, pero
`src/frontend/CMakeLists.txt` solo declara `${CMAKE_SOURCE_DIR}/src/backend/include` y el árbol de
`src/frontend` en sus rutas de inclusión. Con `BUILD_BACKEND=ON` la ruta llega **por herencia**
(`frontend → backend → common`); con `BUILD_BACKEND=OFF` el target `backend` no existe y la
compilación de la interfaz fallaba con:

```
src/frontend/src/services/tipos_aula_ui.hpp:19:10: fatal error: common/tipo_general.hpp:
No existe el fichero o el directorio
```

Se reprodujo en un worktree limpio de `0801e56` antes de cualquier cambio de esta rama.

### Cambio

`src/frontend/CMakeLists.txt`: `${CMAKE_SOURCE_DIR}/src/common/include` se añade al bloque
**PUBLIC** de `target_include_directories(frontend …)` (junto a `src/backend/include`, sin duplicar
rutas). PUBLIC porque los consumidores compilan cabeceras privadas del frontend que incluyen tipos
compartidos (`test_tipos_aula_ui` incluye `services/tipos_aula_ui.hpp`), igual que ya ocurre con la
ruta del backend. No se enlaza el target `common` (misma ruta, sin dependencia nueva); se elige la
forma explícita que pedía la tarea.

### Verificación

| Suite | Comando | Resultado |
|---|---|---|
| C3 (target frontend, dir nuevo) | `cmake -S . -B /tmp/opencode/build-fe-t1b-frontend -G Ninja -DBUILD_BACKEND=OFF -DBUILD_TESTING=OFF && cmake --build /tmp/opencode/build-fe-t1b-frontend --target frontend --parallel` | **29/29 verde**, `Linking CXX static library src/frontend/libfrontend.a` (sin OR-Tools) |
| C3 (build completo del mismo dir) | `cmake --build /tmp/opencode/build-fe-t1b --parallel` | `libfrontend.a` se compila y enlaza; falla **solo** el enlazado de `src/app/gestor-horarios` por símbolos de `NucleoDatos` (defecto previo distinto, ver hallazgo) |
| C1 (backend) | `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend` | **62/62 verde**, 0 fallos (40,46 s) |

### Hallazgo (no corregido aquí)

Con `BUILD_BACKEND=OFF` y `BUILD_FRONTEND=ON` (valor por defecto), `src/app/CMakeLists.txt` sigue
creando el ejecutable `gestor-horarios`, que enlaza `frontend`; los widgets de la interfaz
referencian `NucleoDatos` y las funciones de `settings_dialog.cpp`, que viven en el backend
desactivado, así que el enlace falla aunque la librería `frontend` compile. Es un defecto previo
**independiente del include** (se reproduce igual en un worktree de `0801e56` con el fix aplicado:
`[33/40] Linking CXX static library src/frontend/libfrontend.a` seguido de
`FAILED: src/app/gestor-horarios` con referencias sin definir). Corregirlo exige decidir si el
ejecutable debe omitirse o compilarse en modo degradado: **fuera del alcance de T1/T1b**; la
verificación de C3 se hace sobre el target `frontend`, que es el objetivo declarado de la suite.

---

## T2 - Desenlaces del solver: enum limpio, mapeo y optimalidad por fase

RF-2, RF-3, RNF-5.

### Cambios

- **Nuevo** `src/backend/include/backend/solver/desenlace_solver.hpp`: **solo** el enum
  `DesenlaceSolver { Exito, Infactible, NoConcluyente, ErrorContrato, SinRespuesta }`, sin OR-Tools
  ni Qt (D5). Lo podrán incluir `ServicioGeneracion.hpp` y el frontend (T3).
- **Nuevo** `src/backend/include/backend/solver/desenlace_solver_backend.hpp` (solo backend):
  `inline DesenlaceSolver desenlaceDesdeEstado(CpSolverStatus)` con
  `OPTIMAL|FEASIBLE → Exito`, `INFEASIBLE → Infactible`, `UNKNOWN → NoConcluyente`,
  `MODEL_INVALID|otros → ErrorContrato`. Incluye además el predicado puro
  `inline bool pruebaOptimalidad(CpSolverStatus)` = «el estado es `OPTIMAL`», que es la regla de la
  marca por fase (§5.2: una fase se marca solo si su respuesta fue `FEASIBLE`, no `OPTIMAL`).
- `src/backend/include/backend/solver/solver.hpp`: `ResultadoSolver` gana
  `DesenlaceSolver desenlace = DesenlaceSolver::SinRespuesta` y
  `QStringList fasesNoOptimas`; **se conservan** `exito`, `resultado`, `errores` y `tiempo_ms`
  para no romper consumidores (hoy `servicio_solver` solo lee `exito`/`errores`/`resultado`).
- `src/backend/src/solver/solver.cpp`:
  - `desenlace` se rellena en **todos** los caminos: referencia no factible
    (`desenlaceDesdeEstado`), fallo de 2a/2b (`ErrorContrato` para `MODEL_INVALID`), éxito
    (`Exito`) y el camino de 0 s.
  - `fasesNoOptimas` acumula `"referencia"`, `"separacion"` y `"preferencias"` cuando la respuesta
    de la fase es `FEASIBLE` (no `OPTIMAL`). Un `UNKNOWN`/`INFEASIBLE` de una fase blanda sigue
    cayendo a la fase anterior **sin** marcarla: el horario presentado es el de la fase anterior
    (§5.2). El comportamiento de las fases no cambia.
  - **Guarda de 0 s (D15)**: si `presupuesto.referencia <= 0` **no se llama** a `ejecutar` (en
    CP-SAT 0 = sin límite) y se devuelve `NoConcluyente` con texto propio, sin horario. Es el
    mínimo que exige el «Hecho cuando» de T2; T5 sustituye el presupuesto por el deadline de reloj
    y el guard pasa a `restante <= 0`.
- `ServicioGeneracion`/IPC **sin tocar** (T3). `UNKNOWN` ya no se confunde con NoFactible en el
  nivel del solver; `MODEL_INVALID` sale como `ErrorContrato`.

### Tests (escritos antes del código; el rojo se comprobó)

Rojo inicial: `test_solver_horarios.cpp` no compilaba con
`'Solver::ResultadoSolver' has no member named 'desenlace'/'fasesNoOptimas'`.

| Test | Archivo | Cubre |
|---|---|---|
| `desenlaceDesdeEstado_mapeaEstadosDelMotor` | `test/backend/test_solver_horarios.cpp` | los cinco estados |
| `desenlaceDesdeEstado_unknownYModeloInvalidoNoSonInfactibles` | idem | RF-2: no hay NoFactible sin prueba |
| `desenlaceDesdeEstado_feasibleSePresentaYSuFaseNoEsOptima` | idem | `FEASIBLE → Exito`; solo `OPTIMAL` prueba optimalidad |
| `solver_falloDemostrado_esInfactible` | idem | `INFEASIBLE ⇒ desenlace Infactible`, sin horario ni fases marcadas |
| `solver_presupuestoCero_esNoConcluyente` | idem | no llama al motor; `NoConcluyente`, no `Infactible` |
| `solver_exitoDeclaraDesenlaceYFasesNoOptimas` | idem | `Exito` + lista de fases vacía en un caso probado óptimo |
| `desenlaceSolver_sinOrTools` | `test/backend/test_contrato_solver.cpp` | el enum se incluye y usa en un target **sin OR-Tools** (D5) |

No hubo tests del mapeo antiguo que ajustar; ninguno afirmaba `UNKNOWN` como NoFactible a nivel de
solver. Los tests existentes solo miran `exito`, que se conserva.

### Verificación

| Suite | Comando | Resultado |
|---|---|---|
| C1 (backend) | `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend` | **62/62 verde**, 0 fallos (47,58 s) |
| C3 (target `frontend`, dir nuevo) | `cmake -S . -B /tmp/opencode/build-fe-t2 -G Ninja -DBUILD_BACKEND=OFF -DBUILD_TESTING=OFF && cmake --build /tmp/opencode/build-fe-t2 --target frontend --parallel` | **29/29 verde**, `libfrontend.a` enlazada (sin OR-Tools) |

Detalle de los tests nuevos:

```
$ ./build-backend/test/test_solver_horarios | grep desenlace
PASS: desenlaceDesdeEstado_mapeaEstadosDelMotor, …_unknownYModeloInvalidoNoSonInfactibles,
      …_feasibleSePresentaYSuFaseNoEsOptima, solver_falloDemostrado_esInfactible,
      solver_presupuestoCero_esNoConcluyente, solver_exitoDeclaraDesenlaceYFasesNoOptimas
Totals: 55 passed, 0 failed
$ ./build-backend/test/test_contrato_solver | grep desenlaceSolver
PASS: desenlaceSolver_sinOrTools  · Totals: 24 passed, 0 failed
```

### Hallazgos

1. **La marca por fase es determinista y la prueba también.** El desenlace temporal (`FEASIBLE` vs
   `OPTIMAL` de una fase real) depende del reloj y de la máquina (RNF-5), así que la regla se prueba
   por su predicado puro (`desenlaceDesdeEstado` + `pruebaOptimalidad`) y no forzando una corrida al
   límite. El escenario real de solución factible al límite queda para la evidencia manual del
   usuario (RNF-2).
2. **`pruebaOptimalidad` es una adición mínima al header de backend** (no estaba nombrada en el
   plan): evita duplicar en `solver.cpp` la condición `status == OPTIMAL` en tres fases y da un
   punto único testeable de «qué significa probar optimalidad».
3. **La guarda de 0 s se implementó sobre `presupuesto.referencia`** porque T2 aún conserva
   `PresupuestoFases{referencia, separacionHuecos, preferencias}`. T5 la reescribirá sobre
   `restanteSegundos(deadline, …) <= 0`; el test se ajustará en esa tarea.

---

## T3 - IPC y servicio: códigos, puerto y estados distintos

RF-2, RF-3, RNF-4.

### Cambios

- `src/backend/include/backend/services/ServicioGeneracion.hpp`:
  - incluye `backend/solver/desenlace_solver.hpp` (header limpio, sin OR-Tools);
  - `RespuestaSolver` pasa a `DesenlaceSolver desenlace` + `salida` + `error`, con
    `ok`/`factible` **conservados como campos derivados de compatibilidad** (ver hallazgo 1);
  - `ResultadoGeneracion::Estado` gana **`NoConcluyente`** y **`SinRespuesta`** (documentados como
    distintos de NoFactible y de contrato); `TiempoAgotado` queda solo para el vencimiento del plazo.
- `src/backend/src/services/ServicioGeneracion.cpp` (`alResponder`), **switch exhaustivo** sobre
  `DesenlaceSolver`:
  `Exito` → análisis del horario (`Listo`/`DatosAnteriores`/`CalculoFallido`);
  `Infactible` → `NoFactible`; `NoConcluyente` → `NoConcluyente`; `ErrorContrato` →
  `ContratoInvalido`; `SinRespuesta` → `SinRespuesta`. Mensajes distintos por desenlace y **H3
  mínimo**: en NoFactible se conserva `respuesta.error` y, si llega vacío, se informa «no hay una
  causa atribuible». `ok`/`factible` ya no deciden nada.
- `src/backend/src/solver/servicio_solver.cpp` (y doc del `.hpp`): propaga
  `ResultadoSolver::desenlace` a `RespuestaSolver` y rellena `ok`/`factible` derivados; `error`
  conserva siempre el texto del motor (`errores.join`). Una entrada que no es del contrato sale
  como `ErrorContrato`.
- `src/middleware/include/middleware/messages.h`: **solo** se añade
  `RESP_NO_CONCLUYENTE = -8`. `RESP_SIN_SOLUCION` sigue para la infactibilidad y `RESP_INVALIDO`
  para el error de entrada/contrato. `InternalClient` **sin cambios**.
- `src/app/src/aplicacion_backend.cpp`: la ruta `solver_resolve` mapea por `desenlace`:
  `Exito→RESP_EXITO`, `Infactible→RESP_SIN_SOLUCION`, `NoConcluyente→RESP_NO_CONCLUYENTE`,
  `ErrorContrato→RESP_INVALIDO`; `SinRespuesta` (que este lado no produce) queda defensivo en
  `RESP_ERROR`.
- `src/frontend/src/services/puerto_solver_ipc.{hpp,cpp}`:
  - `DesenlaceSolver desenlaceParaCodigo(std::optional<int>)`, puro y documentado (D6): sin `code`
    → `ErrorContrato`; `RESP_ERROR`/`RESP_TIEMPO_AGOTADO` → `SinRespuesta`; `RESP_INVALIDO` →
    `ErrorContrato`; `RESP_NO_CONCLUYENTE` → `NoConcluyente`; `RESP_SIN_SOLUCION` → `Infactible`.
  - `resolver` sin cliente → `SinRespuesta` con su motivo (no «contrato incumplido»);
    `onRespuestaRecibida` construye `RespuestaSolver` desde el desenlace y conserva el texto
    (`data`/`message`) o pone uno específico. El resto de operaciones IPC no cambia.
- `schedule_generation_widget`: **sin cambios de código**; ver hallazgo 2.

### Tests (escritos antes del código; el rojo se comprobó)

Rojo inicial: `test_contrato_generacion` y `test_servicio_generacion` no compilaban
(`NoConcluyente`/`SinRespuesta` no existen; `RespuestaSolver` no tiene `desenlace`).

| Test | Archivo |
|---|---|
| `noFactible` (conserva el texto del motor, H3) y `noFactible_sinTexto_informaQueNoHayCausaAtribuible` | `test/backend/test_servicio_generacion.cpp` |
| `noConcluyente_esDistintoDeNoFactible`, `sinRespuesta_noEsContratoNiNoFactible`, `errorDeContrato_conservaSuCausa`, `contratoInvalido` | idem |
| `estadosDeGeneracion_estanExponidos` (incluye los estados nuevos) | `test/backend/test_contrato_generacion.cpp` |
| `resolverEntradaSolver_entradaInvalida_esErrorContrato` (propagación del desenlace) | `test/backend/test_solver_horarios.cpp` |
| `desenlaceParaCodigo_mapeaCodigosDelSolver` (los seis casos + sin `code`) y `puertoSolver_sinConexion_esSinRespuesta` | `test/frontend/test_conversion_entrada.cpp` |

Los dobles de `test_servicio_generacion` se ajustaron al nuevo contrato (`r.desenlace`), y
`respuestaFactible()` marca `Exito`. **`test_middleware_transport`, `test_middleware_health_check`
y `test_ipc_aislamiento_bd` pasan sin modificar** (62/62 en C1 y 67/67 en C2).

### Verificación

| Suite | Comando | Resultado |
|---|---|---|
| C1 (backend) | `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend` | **62/62 verde**, 0 fallos (23,12 s) |
| C2 (Release completa) | `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release` | **67/67 verde**, 0 fallos (66,74 s; el lento no se ejecuta) |
| C3 (target `frontend`) | `cmake --build /tmp/opencode/build-fe-t2 --target frontend --parallel` | **verde** (10/10; solo avisos previos de `schedule_model.hpp`) |

Detalle: `test_servicio_generacion` **39/39**, `test_contrato_generacion` **7/7**,
`test_solver_horarios` **56/56**, `test_frontend_conversion` **8/8** (incluye los dos nuevos).

### Hallazgos

1. **`ok`/`factible` se conservaron como campos derivados** de `RespuestaSolver` (el plan §4 solo
   listaba `desenlace`/`salida`/`error`). Motivo: `test/test_ipc_aislamiento_bd.cpp` los usa y T3
   exige que pase **sin modificar**; cumple. El servicio y la ruta real deciden por `desenlace`;
   los derivados son `ok = Exito|Infactible|NoConcluyente` y `factible = Exito`.
2. **La vista no necesitó cambios**: `onGeneracionFinalizada` ya ramifica por
   `resultado.presentable` y muestra `resultado.mensaje` en el diálogo de error
   (`schedule_generation_widget.cpp` ~1085-1098), así que los estados nuevos se presentan como
   error con su texto y sin emitir `horarioGenerado`. Los textos del plazo y el reparto son T5/T6.
3. **Prueba de «socket caído» en el puerto, no en el cliente global**: `desenlaceParaCodigo`
   cubre `RESP_ERROR`/`RESP_TIEMPO_AGOTADO` y `PuertoSolverIpc` sin cliente devuelve
   `SinRespuesta`. No se emitió la señal real de `InternalClient` en el test (evita conectar un
   `QLocalSocket` durante la suite); el cliente global no se toca y sus suites siguen verdes.
4. **H3 mínimo ya en T3** (lo pedía esta tarea): se conserva `respuesta.error` en NoFactible y se
   declara «no hay causa atribuible» si viene vacío. T4 añadirá demanda/topes y
   `docentesSinCapacidad`; no se solapa con esto.
5. `desenlaceParaCodigo` solo existe con `GESTOR_TIENE_BACKEND` (el `.cpp` del puerto solo se añade
   al frontend cuando hay target `backend`); en C3 el header queda vacío y el frontend compila sin
   OR-Tools. El orden numérico de `Estado` cambió al insertar dos valores, pero no se persiste ni
   serializa en ningún sitio (solo se compara por nombre).

---

## T4 - Causa mínima (H3), demanda vs topes y docentes sin capacidad

RF-1, RF-2, RNF-5.

### Cambios

- `src/backend/include/backend/solver/config/contrato_solver.hpp` / `.cpp`: funciones puras
  **`demandaTotalHoras`** (Σ bloques del plan de todos los cursos, vía `materiasDelCurso`) y
  **`sumaTopesHorasAula`** (Σ `bloquesAula(horas_aula)` de todos los docentes; un tope 0
  declarado o ausente suma 0 y uno negativo no resta). Sin E/S, sin OR-Tools.
- `src/backend/src/solver/solver.cpp` (`analizarFallo`): en `INFEASIBLE`, la precedencia (1)–(5)
  se mantiene. **Si ninguna categoría aplica**, sigue «no atribuible» y, cuando
  `demandaTotalHoras > sumaTopesHorasAula`, añade el diagnóstico:
  `«Horas: la demanda total es X h y la suma de los topes de horas de aula es Y h; Z h de demanda
  no caben en los topes duros de los docentes.»` **No** se inventa categoría (RF-2).
- `src/backend/include/backend/services/ValidadorSalidaSolver.hpp` / `.cpp`:
  `AnalisisSalida::docentesSinCapacidad` (nombres con `horas_aula <= 0`, declarado o ausente).
  Es informativo y **no** altera `presentable()` (el tope lo comprobará T6).
- `src/backend/src/services/ServicioGeneracion.cpp`: el mensaje de `Infactible` conserva el texto
  del motor (H3, T3) y, si hay docentes sin capacidad, **añade**
  `«Docentes sin capacidad (horas de aula 0 o ausentes): N1, N2.»` — se informa en cualquier
  NoFactible, no solo en el causado por horas, para no dejar un deshabilitado silencioso (RF-1).
- Sin cambios de contrato 2.0, esquema, presets ni datos.

### Tests (escritos antes del código)

Las pruebas se escribieron **antes** del código: al redactarlas, `demandaTotalHoras`,
`sumaTopesHorasAula` y `AnalisisSalida::docentesSinCapacidad` no existían en el árbol. **En esta
tarea no se capturó una compilación en rojo por separado** (el primer build tras implementar fue
verde); en T1–T3 sí quedó registrada la salida roja. La cobertura efectiva se comprobó ejecutando
los binarios (salida abajo).

| Test | Archivo |
|---|---|
| `demandaYTopesDeHoras_sonSumasEnHorasAcademicas`, `sumaTopesHorasAula_noCuentaDocentesSinCapacidad` | `test/backend/test_contrato_solver.cpp` |
| `noFactibleSoloPorHoras_informaDemandaYTopes` (solver: «no atribuible» + las dos cifras + sin «Causa (») | `test/backend/test_solver_horarios.cpp` |
| `noFactible_conservaElTextoDelMotor`, `noFactibleSinTexto_informaQueNoHayCausaAtribuible`, `docenteSinHorasAula_apareceEnElMensaje` (declarado 0 y campo **ausente**) | `test/backend/test_servicio_generacion.cpp` |
| `docentesSinCapacidad_incluyeTopeCeroDeclaradoYAusente` | `test/backend/test_validador_salida.cpp` |

Los dos tests del servicio de T3 se renombraron a los nombres del plan
(`noFactible` → `noFactible_conservaElTextoDelMotor`, `…sinTexto…` →
`noFactibleSinTexto_informaQueNoHayCausaAtribuible`). El único test que afirmaba el texto genérico
era el propio `noFactible` de T3; ya se reescribió allí y ahora además comprueba que el genérico
**no** aparece.

### Verificación

| Suite | Comando | Resultado |
|---|---|---|
| C1 (backend) | `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend` | **62/62 verde**, 0 fallos (24,43 s) |
| C2 (Release completa) | `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release` | **67/67 verde**, 0 fallos (61,13 s; lento no ejecutado) |
| C3 (target `frontend`, dir nuevo) | `cmake -S . -B /tmp/opencode/build-fe-t4 -G Ninja -DBUILD_BACKEND=OFF -DBUILD_TESTING=OFF && cmake --build /tmp/opencode/build-fe-t4 --target frontend --parallel` | **29/29 verde**, `libfrontend.a` enlazada |

Detalle: `test_contrato_solver` 26/26, `test_validador_salida` 22/22,
`test_servicio_generacion` 40/40, `test_solver_horarios` 57/57.

### Hallazgos / decisiones no cubiertas literalmente por el plan

1. **`noFactibleSoloPorHoras_informaDemandaYTopes` vive en `test_solver_horarios`**, no en
   `test_servicio_generacion` como decía el plan: el diagnóstico demanda/topes se produce en
   `analizarFallo` (como fija §2) y viaja en `respuesta.error`; en el test del servicio con un
   doble solo se podría fabricar el texto, que no probaría nada. El servicio sí se prueba con
   `noFactible_conservaElTextoDelMotor` (paso del texto del motor, H3) y
   `docenteSinHorasAula_apareceEnElMensaje`.
2. **Las cifras se añaden solo si ninguna categoría del catálogo aplica** y la demanda supera los
   topes. Si aplica (1)–(5), se informa esa con la precedencia vigente y sin cifras de horas
   (RF-2: «si además aplica otra categoría, se informará esa»).
3. **`docentesSinCapacidad` incluye a todo docente con `horas_aula <= 0`**, imparta o no clases
   (es información, no reparto). No cambia `presentable()`; T6 añadirá la comprobación del tope y
   el reparto.
4. **El mensaje añade los docentes sin capacidad en cualquier NoFactible** (no solo por horas):
   RF-1 exige informar de los docentes con horas ausentes/0 para que no queden deshabilitados en
   silencio; si la causa es otra categoría, el dato sigue siendo útil.
5. `demandaTotalHoras` se apoya en `materiasDelCurso` (plan embebido) y no reutiliza el
   `bloquesCurso` anónimo de `solver.cpp`: mismo cálculo por curso pero agregado a todo el centro;
   se ha preferido la función pura y testeable del contrato, sin tocar el código de la causa (3).
6. La variante «campo ausente» se prueba de extremo a extremo en el servicio (JSON sin
   `horas_aula`, que pasa la validación y llega a 0) y en el parser desde T1
   (`horasAulaAusente_seLeeComoCero`).

---

## T5 - Plazo 60/55/5 de reloj y textos

RF-3, RNF-2.

### Cambios

- `src/backend/include/backend/solver/solver.hpp`:
  - funciones puras **`restanteMs(deadlineMs, ahoraMs)`** y **`restanteSegundos(...)`** (nunca
    negativas) para el presupuesto de cada fase (§5.2);
  - `PresupuestoFases` pasa a `{ totalSegundos = 3300.0, workers = 8,
    ejecutarSeparacionHuecos = true, ejecutarPreferencias = true }`: **un solo presupuesto de
    reloj** (55 min) para todo el solver; los flags sustituyen a los presupuestos por fase y
    permiten probar la escalera sin reloj (D4).
- `src/backend/src/solver/solver.cpp`:
  - **deadline único** `deadlineMs = totalSegundos × 1000` al inicio; la referencia recibe
    `restanteSegundos(...)` y las blandas solo se ejecutan si `restante > 0` (si no, se devuelve
    la fase anterior);
  - camino de **0 s (D15)**: si el restante inicial es `<= 0` **no se llama al motor** y se
    devuelve `NoConcluyente` con texto propio;
  - `ejecutar` usa **`max_time_in_seconds`** (reloj) en lugar de `max_deterministic_time`, con
    `Q_ASSERT(segundos > 0.0)`; **nunca** recibe 0 s.
  - Los 5 min hasta los 60 son **reserva del servicio** (extracción/IPC), fuera del deadline.
- `src/backend/include/backend/services/ServicioGeneracion.hpp`:
  **`PLAZO_MS_POR_DEFECTO = 3'600'000`** (60 min) y usado como valor por defecto de `plazoMs`;
  documentación actualizada (60 min de proceso / 55 de solver / 5 de reserva).
- `src/frontend/src/views/schedule_generation_widget.cpp`: instancia el servicio con
  `PLAZO_MS_POR_DEFECTO` y anuncia
  **«Generando horarios… el cálculo puede tardar hasta una hora.»** (antes «unos segundos»).
  Sin pantallas nuevas.

### Tests

Pruebas ajustadas/escritas **antes** del código; no se capturó el build rojo por separado (el
primer build tras implementar fue verde). Detalle:

| Test | Archivo |
|---|---|
| `presupuestoCero_esNoConcluyenteSinEjecutar` (antes `solver_presupuestoCero_esNoConcluyente`) con `totalSegundos = 0` y sin fases marcadas | `test/backend/test_solver_horarios.cpp` |
| `presupuestoPorDefectoDeLa020` (3300 s, 8 workers, flags a `true`; antes `…014b` con 60/20/10) y `restanteDeReloj_nuncaEsNegativo` | `test/backend/test_solver_bloques.cpp` |
| `separacionInformada…`, `escalera2aDevuelveLaReferenciaSinError`, `escalera2bDevuelveLa2a`, `sinPreferenciasSeOmiteLa2b` reescritos para usar `ejecutarSeparacionHuecos`/`ejecutarPreferencias` en vez de presupuestos por fase | `test/backend/test_solver_bloques.cpp` |
| `fasesBlandasDesactivadas_devuelvenLaReferencia` (antes `fase2SinSolucion_devuelveLaReferencia`) | `test/backend/test_solver_preferencias.cpp` |
| `plazoCortoInyectado_informaPlazoAgotado` (antes `timeout_…`, con `plazoMs = 50`: estado `TiempoAgotado`, sin horario y sin afirmar que no exista solución) y `plazoPorDefecto_esUnaHora` (`PLAZO_MS_POR_DEFECTO == 3 600 000`) | `test/backend/test_servicio_generacion.cpp` |
| `test_solver_centro_sin_ef` (lento, **solo compila**): la referencia usa `ejecutarSeparacionHuecos = false` | `test/backend/test_solver_centro_sin_ef.cpp` |

No hay suite de UI del widget, así que no hay test del texto: se verificó con `grep` y con la
compilación del target `frontend` (C3).

### Verificación

| Suite | Comando | Resultado |
|---|---|---|
| C1 (backend) | `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend` | **62/62 verde**, 0 fallos (18,23 s) |
| C2 (Release completa) | `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release` | **67/67 verde**, 0 fallos (14,72 s; lento no ejecutado) |
| C3 (target `frontend`, dir nuevo) | `cmake -S . -B /tmp/opencode/build-fe-t5 -G Ninja -DBUILD_BACKEND=OFF -DBUILD_TESTING=OFF && cmake --build /tmp/opencode/build-fe-t5 --target frontend --parallel` | **29/29 verde**, `libfrontend.a` enlazada |

Detalle: `test_solver_bloques` 24/24, `test_solver_preferencias` 12/12,
`test_servicio_generacion` 41/41, `test_solver_horarios` 57/57.

### Hallazgos / decisiones

1. **Ya no existe «presupuesto de la fase 2a/2b»**: con el deadline único, una fase blanda solo
   puede quedarse sin tiempo si la anterior lo consume. Los tests de la escalera pasan a usar los
   flags (D4), que es el mecanismo previsto para probarla sin depender del reloj (RNF-5). Está
   declarado y **no es regresión**: son pruebas del comportamiento sustituido (RNF-1).
2. **`fase2SinSolucion_devuelveLaReferencia` se reescribió** como
   `fasesBlandasDesactivadas_devuelvenLaReferencia`: el caso «la 2b tiene 1 ns» ya no es
   expresable con un presupuesto compartido; el contrato que importa (no fallar y devolver la
   fase anterior) se conserva.
3. **Cambia la unidad de los tests lentos**: `test_solver_centro_sin_ef` y
   `test_solver_centro_real` (etiqueta `slow`, **no ejecutados**) pasan de 60 unidades
   deterministas (~172 s medidos) a un techo de **3300 s de reloj**, y sus techos internos de
   300 s pueden quedar obsoletos. Queda para T7/T8 y para la corrida manual del usuario.
4. **`Q_ASSERT(segundos > 0.0)`** se compila fuera en Release; la garantía efectiva la dan los
   guards `restante > 0` antes de cada fase y el test puro `restanteDeReloj_nuncaEsNegativo`.
5. El servicio no abandona antes de los 60 min por defecto y el solver no puede pasar de 55: los
   5 min restantes cubren extracción y comunicación (D2). No se ha añadido ningún mecanismo de
   parada por salud (`InternalClient` sigue sin cambios).
6. `PLAZO_MS_POR_DEFECTO` es `inline constexpr` en `ServicioGeneracion.hpp` (header que ya
   incluye el frontend: sin OR-Tools), y la vista lo usa explícitamente.

---

## T6 - Tope posterior contra `horas_aula`, reparto y separación P4/P5

RF-5, RF-1.

### Cambios

- `src/backend/include/backend/services/ValidadorSalidaSolver.hpp`:
  - **`LineaHorasDocente`** (`profesor`, `nombre`, `horasAsignadas`, `horasAula`) y en
    `AnalisisSalida`: `reparto` (una línea por docente **con clases**), `totalHorasAsignadas`,
    `totalHorasAula`, `excesosTopeHoras` (P5) y `repartoTexto()`.
  - **`presentable()` = `conflictos` vacío && `excesosTopeHoras` vacío.** `hayConflictoSolapamiento()`
    se conserva y sigue significando **solo** P4.
- `src/backend/src/services/ValidadorSalidaSolver.cpp`: calcula el reparto desde las asignaciones,
  los totales (Σ asignadas; Σ `horas_aula` declaradas del centro) y P5
  (`asignadas > horas_aula`, mensaje
  «El docente «X» tiene N h asignadas y su tope de horas de aula es M h.»). P2 sigue midiendo
  `horas_requeridas` y no se mezcla con P5.
- `src/backend/src/services/ServicioGeneracion.cpp`:
  - en `!presentable()` se ramifica: **P4** (`hayConflictoSolapamiento()`) con el mensaje de
    solapamiento; **si no**, P5 (`excesosTopeHoras`) con un mensaje propio que **no menciona
    solapamiento** y lista los docentes; fallback genérico;
  - la **optimalidad por fase** se lee de `salida.metadata.fases_sin_optimalidad` y se expone en
    `ResultadoGeneracion::fasesNoOptimas` (campo nuevo).
- `src/backend/include/backend/solver/servicio_solver.hpp` / `.cpp`: **función pura
  `salidaConFasesSinOptimalidad(salida, fases)`** (D8) que inyecta
  `metadata.fases_sin_optimalidad` **solo si hay fases**; no toca `MetadataSalida` ni el modelo de
  datos. `resolverEntradaSolver` la usa en el camino `Exito`.
- `src/frontend/src/views/schedule_generation_widget.cpp`: el diálogo de éxito existente añade
  `analisis.repartoTexto()` («Horas por docente») y el texto de no optimalidad **por fase**
  (`referencia`/`separacion`/`preferencias`); si la referencia fue óptima no aparece. Los estados
  sin horario siguen mostrando solo su causa (sin reparto). Sin pantallas nuevas.

### Tests (escritos antes del código; rojo capturado)

Rojo inicial: `AnalisisSalida` no tenía `excesosTopeHoras`/`reparto`/`totalHoras*`/`repartoTexto`
y `ResultadoGeneracion` no tenía `fasesNoOptimas` (errores de compilación en
`test_validador_salida` y `test_servicio_generacion`).

| Test | Archivo |
|---|---|
| `repartoPorDocenteYtotales`, `excesoDeTopeDeHorasAula_noEsPresentable`, `presentable_conservaHayConflictoSolapamiento`, `hayConflictoSolapamiento_seConserva`, `p2YElTopeDeHorasAulaSonComprobacionesDistintas`, `p2SigueComparandoHorasRequeridas`, `repartoTexto_incluyeSinCapacidad` | `test/backend/test_validador_salida.cpp` |
| `calculoFallidoPorSolapamiento_noSeConfundeConTope`, `calculoFallidoPorTopeDeHorasAula_mensajeDistinto`, `reparto_enElResultado`, `fasesNoOptimas_enElResultado` | `test/backend/test_servicio_generacion.cpp` |
| `salidaConFasesSinOptimalidad_inyectaLaMetadata` (no muta la entrada y no añade la clave sin fases) | `test/backend/test_solver_horarios.cpp` |

### Verificación

| Suite | Comando | Resultado |
|---|---|---|
| C1 (backend) | `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend` | **62/62 verde**, 0 fallos (18,97 s) |
| C2 (Release completa) | `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release` | **67/67 verde**, 0 fallos (15,78 s; lento no ejecutado) |
| C3 (target `frontend`, dir nuevo) | `cmake -S . -B /tmp/opencode/build-fe-t6 -G Ninja -DBUILD_BACKEND=OFF -DBUILD_TESTING=OFF && cmake --build /tmp/opencode/build-fe-t6 --target frontend --parallel` | **29/29 verde**, `libfrontend.a` enlazada |

Detalle: `test_validador_salida` 29/29, `test_servicio_generacion` 45/45,
`test_solver_horarios` 58/58.

### Hallazgos / decisiones

1. **`totalHorasAula` suma `horas_aula` declaradas** (con las líneas del reparto), no llama a
   `sumaTopesHorasAula`: esa función pura depende de `franja_horaria.duracion_minutos` y el
   validador de salida trabaja en horas académicas de forma pura; con la entrada validada ambos
   valores coinciden.
2. **Fixtures de P4 con `horas_aula = -1` («sin tope») tuvieron que declarar un tope real (8 h).**
   Bajo el tope duro, `-1`/`0` significan «sin capacidad» y un horario con clases de esos docentes
   deja de ser presentable (RF-1/RF-5). Es un ajuste del dato de prueba, no una regresión: esos
   tests seguían comprobando P4, no las horas. Además, el fixture `docentesSinCapacidad…` asigna
   ahora las clases a un docente con capacidad para seguir midiendo solo la lista.
3. **D8 sin tocar el modelo de datos**: `fases_sin_optimalidad` se inyecta en el JSON de la
   metadata desde `servicio_solver` (función pura, testeada); `MetadataSalida`/`fromJson` no
   cambian y la clave ausente significa «todas óptimas».
4. **P4 y P5 no comparten mensaje**: el servicio ramifica por `hayConflictoSolapamiento()` y, si no
   hay conflicto, por `excesosTopeHoras`; el texto de tope no contiene «solapamiento» (test).
5. **La vista no recalcula**: usa `analisis.repartoTexto()` y `resultado.fasesNoOptimas` ya
   calculados por el servicio. No hay suite de UI, así que la presentación se verifica por la
   compilación del target `frontend` (C3) y por los campos del resultado (C1).
6. `docentesSinCapacidad` (T4) sigue sin alterar `presentable()`; ahora convive con P5, que sí lo
   altera, y ambos aparecen en el reparto/mensaje con propósitos distintos.

---

## T7 - Barrido de tests superseded y suites completas

RNF-1, RF-3, RF-4.

### Barrido realizado (revisados)

`test_solver_horarios`, `test_solver_bloques`, `test_solver_aforo`, `test_solver_preferencias`,
`test_analisis_bloques`, `test_servicio_generacion`, `test_contrato_generacion`,
`test_contrato_solver`, `test_conversion_entrada`, `test_validador_salida`,
`test_validador_entrada`, `test_variables_cp_sat`, `test_solver_centro_sin_ef` (solo compila) y,
además, `test_solver_benchmark`.

Comprobaciones:

- **No queda ninguna prueba que afirme el exceso permitido** ni el objetivo con S1: los casos
  superseded se reescribieron en T1 (tope duro, tope 0), T2/T3 (desenlaces), T5 (presupuestos de
  reloj) y T6 (P5/reparto). `solver_topeDuroSinCapacidadEsInfactible` es el antiguo
  `solver_excesoDeHorasEsLaValvulaSinCapacidad` y ya exige `!exito`.
- **`PESO_HORAS` ya no se prueba como jerarquía activa**: `constantes_pesoHorasEsHistorico` solo
  fija el valor histórico y `constantes_jerarquiaConPreferencia` comprueba únicamente
  `PESO_PREFERENCIA < COSTE_SUPLENTE` (activa).
- **No queda ningún 60/20/10 ni 300 s como vigente**: `presupuestoPorDefectoDeLa020` fija 3300 s y
  8 workers; `plazoPorDefecto_esUnaHora` fija 3 600 000 ms. Los textos de 300 s del benchmark y de
  `centro_sin_ef` se corrigieron para no presentarlo como plazo vigente.
- `max_deterministic_time` solo aparece en los **arneses locales** de `test_solver_bloques` y
  `test_solver_aforo` (resuelven el modelo construido a mano, no usan `Solver::ejecutar`); no es
  comportamiento de producción y no se toca.
- `test_variables_cp_sat` no contaba variables `e[p]`; no había recuentos que corregir.
- `test_analisis_bloques` usa «tope» en el sentido de tamaño de bloque (014), no de horas.
- `test_validador_entrada` conserva V13/V14 y la coherencia `horas_aula ≤ horas_requeridas`.

### Cambios de esta tarea (solo tests)

- `test/backend/test_solver_horarios.cpp`: renombrado
  `solver_d3_bloquesSonHoras_s1_topeSinConversion` → `solver_d3_bloquesSonHoras_topeDuroSinConversion`
  (el comentario ya hablaba del tope sin conversión). El comentario de
  `solver_titularAlTope_usaAlSuplente` ya quedó coherente en T1 («con el tope duro no puede
  cubrirlo»); el test sigue siendo válido: el titular a tope cede la materia al suplente.
- `test/backend/test_contrato_solver.cpp`: `constantes_jerarquiaDelObjetivo` →
  `constantes_pesoHorasEsHistorico` (PESO_HORAS documentado, no activo);
  `constantes_jerarquiaConPreferencia` deja de comparar con `PESO_HORAS`; comentario de
  `bloquesAula_esIdentidad` actualizado al tope duro.
- `test/backend/test_solver_preferencias.cpp`: comentarios del fichero, del helper
  `excesoPorDocente` y de dos casos reescritos («S1+S2» → S2; «S1 intacto» → tope duro; la fase 2
  fija `z*`, no `e*`). Las aserciones no cambian: el exceso medido debe ser 0.
- `test/backend/test_solver_centro_sin_ef.cpp` (lento, solo compila): comentarios de presupuesto
  (55 min de reloj, 8 workers) y del techo de 300 s, que queda como control de rendimiento
  pendiente de revisar en la corrida manual.
- `test/backend/test_solver_benchmark.cpp`: comentarios «300 s» → «una hora» (el techo de 60 s del
  benchmark no es el plazo del servicio).

**Ningún cambio de código de producción** en esta tarea: el barrido solo requirió comentarios y un
renombrado de test.

### Verificación

| Suite | Comando | Resultado |
|---|---|---|
| C1 (backend) | `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend` | **62/62 verde**, 0 fallos (18,69 s) |
| C2 (Release completa) | `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release` | **67/67 verde**, 0 fallos (16,13 s) |
| C3 (target `frontend`, dir nuevo) | `cmake -S . -B /tmp/opencode/build-fe-t7 -G Ninja -DBUILD_BACKEND=OFF -DBUILD_TESTING=OFF && cmake --build /tmp/opencode/build-fe-t7 --target frontend --parallel` | **29/29 verde**, `libfrontend.a` enlazada |

**No se ejecutó** `slow-release`, `test_solver_centro_real` ni `test_solver_centro_sin_ef`.

### Expectativas pendientes de la corrida manual (declaradas)

- `test/frontend/test_solver_centro_real.cpp` (slow) afirma **876 clases factibles** y un techo de
  **300 s**. Con el tope duro y el presupuesto de una hora, ambas cosas quedan **pendientes de la
  corrida manual del usuario** con el centro real (36 h por docente). No se ha tocado el test ni se
  ha forzado la factibilidad para ocultar el resultado del experimento.
- `test/backend/test_solver_centro_sin_ef.cpp` (slow) afirma 848 clases y un techo de 300 s; solo se
  ha actualizado el comentario. Su resultado real se decidirá tras la corrida manual.

### Hallazgos

1. La línea base acotada (RNF-1) se cumple: las 62 pruebas de C1 y las 67 de C2 pasan sin ninguna
   expectativa del comportamiento sustituido. Los recuentos no varían respecto de T6 (se
   renombraron y se ajustaron comentarios; no se retiró cobertura).
2. El único resto de «tiempo determinista» en tests son arneses locales que no usan
   `Solver::ejecutar`; se dejan para no alterar la semántica de esos modelos construidos a mano.
3. El techo de 300 s del benchmark y de `centro_sin_ef` es un control de rendimiento del host, no
   el plazo del servicio; ahora está documentado como tal.

---

## T8 - Cierre del experimento (sin ejecutar el solver lento)

RNF-3, RNF-4, RNF-7, RNF-10.

### Base y commits

- Rama: `experimento/horas-duras-plazo-1h`, creada desde `feat/Motor` en
  **`455ba7f651d59e46f633753ccbe8368f0a37c0f9`**.
- **Commit de la spec/plan/tareas (base de implementación): `0801e56`** («docs(sdd): spec 020
  aprobada y plan del experimento»).
- **No hay `sha` por tarea:** la implementación de T1–T8 vive **sin commitear** en el árbol de
  trabajo (T8 no commitea ni hace push, por indicación). El `git status` está limitado a los
  archivos del plan (ver abajo).

### Resumen de tareas

| Tarea | Qué quedó implementado | Evidencia |
|---|---|---|
| **T1** | Tope de horas **duro** (`Σ clases ≤ horas_aula`) para titulares y suplentes; fuera la variable `e[p]`, su cota y S1; ancla sin `e*`; `horas_aula = 0`/ausente sin clases; V13 intacta; `PESO_HORAS` histórico | C1 62/62; journal T1 |
| **T1b** | `src/common/include` en el target `frontend` (defecto previo de C3: `common/tipo_general.hpp` sin ruta con `BUILD_BACKEND=OFF`) | target `frontend` 29/29 |
| **T2** | `desenlace_solver.hpp` (sin OR-Tools) + `desenlace_solver_backend.hpp` (`desenlaceDesdeEstado`, `pruebaOptimalidad`); `ResultadoSolver::desenlace`/`fasesNoOptimas`; guarda de 0 s | C1 62/62; journal T2 |
| **T3** | Servicio con `Estado::NoConcluyente`/`SinRespuesta` y switch por desenlace; H3 mínimo; `RESP_NO_CONCLUYENTE`; puerto con `desenlaceParaCodigo`; `InternalClient` sin cambios | C1 62/62, C2 67/67, C3 |
| **T4** | `demandaTotalHoras` / `sumaTopesHorasAula`; «no atribuible» con demanda y topes cuando solo fallan las horas; `docentesSinCapacidad` en el análisis y en el mensaje de NoFactible | C1 62/62, C2 67/67, C3 |
| **T5** | Presupuesto de **reloj** con deadline único de 55 min (`max_time_in_seconds`), fases blandas con lo que queda y 0 s sin llamar al motor; `PLAZO_MS_POR_DEFECTO` de 60 min; vista «hasta una hora» | C1 62/62, C2 67/67, C3 |
| **T6** | Tope posterior contra `horas_aula` (P5) que hace el resultado no presentable; reparto por docente + totales + sin capacidad; P4/P5 con mensajes distintos; no optimalidad por fase en la metadata y en el diálogo | C1 62/62, C2 67/67, C3 |
| **T7** | Barrido de tests superseded: sin exceso permitido, sin S1 activo, sin 60/20/10 ni 300 s vigentes; comentarios y un renombrado; `test_solver_centro_real`/`centro_sin_ef` no ejecutados | C1 62/62, C2 67/67, C3 |

### Alcance del diff (verificado)

- `git status --short` y `git diff --stat` **solo** muestran archivos del plan: solver (variables,
  restricciones, solver, objetivo/ancla), servicio (`ServicioGeneracion`, `servicio_solver`),
  validador de salida, IPC (`messages.h`, puerto), vista de generación, `CMakeLists.txt` de
  frontend, tests y `specs/020` (`tasks.md`, `journal.md`).
- Archivos **nuevos**: `desenlace_solver.hpp`, `desenlace_solver_backend.hpp` y `journal.md`
  (los dos primeros están previstos en el plan §2).
- Fuera de la lista literal del plan, con justificación: `restricciones_turnos.hpp` (comentario de
  la firma de la restricción cuya implementación cambia) y `test_solver_benchmark.cpp` (solo
  comentarios del techo de tiempo en T7). Ningún otro.
- **Sin tocar** (verificado con `git diff 455ba7f -- …`): `src/middleware/src/` (incluido
  `internalclient.cpp`), `src/app/src/gestor_proceso_backend.cpp`,
  `src/app/include/app/gestor_proceso_backend.hpp`, `src/backend/src/database/`, `src/datos/`,
  `src/seed/`, `src/consola/`, `CMakePresets.json` y `docs/contrato-solver-2.0.md`.

### Health-check y supersesiones

- **Health-check sin cambios** (RNF-4): `InternalClient` no tiene plazo y encola las solicitudes;
  no se ha añadido ningún `kill`, plazo de salud ni aviso de colapso, y
  `gestor_proceso_backend` no se ha tocado. `test_middleware_transport`,
  `test_middleware_health_check` y `test_ipc_aislamiento_bd` pasaron **sin modificar** en C1/C2.
- **No se aplicó ninguna supersesión**: `git diff 455ba7f` está vacío para `specs/005`,
  `specs/018`, `specs/002`, `specs/014`, `specs/013` y `docs/constitution.md`. Las propuestas de
  `spec.md` siguen **sin escribir** en esos documentos.
- `spec.md`, `plan.md` y `estado.md` **no se han editado** en la implementación (diff vacío desde
  `0801e56`); la spec **no se marca como implementada**.

### Verificación final

| Suite | Comando | Resultado |
|---|---|---|
| C1 (backend) | `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend` | **62/62 verde**, 0 fallos (18,38 s) |
| C2 (Release completa) | `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release` | **67/67 verde**, 0 fallos (15,84 s) |
| C3 (target `frontend`, sin OR-Tools) | `cmake -S . -B /tmp/opencode/build-fe-t8 -G Ninja -DBUILD_BACKEND=OFF -DBUILD_TESTING=OFF && cmake --build /tmp/opencode/build-fe-t8 --target frontend --parallel` | **29/29 verde**, `libfrontend.a` enlazada |

**No** se ejecutó `ctest --preset slow-release` ni el cálculo del centro real.

### Reversión (RNF-10)

Todo el experimento vive en esta rama y es reversible **sin tocar la rama de producto**:

1. **Volver al comportamiento anterior:** `git switch feat/Motor` (o `git checkout 455ba7f`). Ese
   commit contiene el tope blando (`Σ x ≤ exceso + tope` con S1 en el objetivo), el ancla con
   `e*` y el plazo de 300 s; la rama no ha modificado `feat/Motor`.
2. **Descartar el experimento:** `git branch -D experimento/horas-duras-plazo-1h` (los cambios de
   implementación están sin commitear; no hay estado persistente, ni datos convertidos, ni
   migración de esquema).
3. Si se quisiera conservar como referencia sin fusionar: dejar la rama como está y **no** aplicar
   las supersesiones propuestas en `spec.md`.

### Evidencia pendiente del usuario (no ejecutada por el agente)

- **Corrida manual del centro real** (36 h por docente, una hora de plazo) con el preset de
  configuración del centro: es el resultado del experimento (horario, NoFactible con causa, no
  concluyente o plazo agotado). RNF-2 lo reserva al usuario.
- **`test/frontend/test_solver_centro_real`** (slow): afirma **876 clases factibles** y un techo de
  300 s. Con el tope duro esa expectativa puede quedar obsoleta; se decide tras la corrida manual.
- **`test/backend/test_solver_centro_sin_ef`** (slow): afirma 848 clases y un techo de 300 s; solo
  compila en C1/C2. Su resultado real también queda pendiente de esa corrida.
- La comprobación con la hora real de cálculo no se ha hecho: los plazos de las pruebas se
  inyectaron cortos (T5).

### Hallazgos del cierre

1. **C1, C2 y C3 verdes** con las suites completas y el cambio limitado al plan; no hay archivos
   fuera de `src/`, `test/` y `specs/020` (salvo el `journal.md` nuevo, previsto).
2. **No se ha tocado** el middleware cliente/servidor, el gestor del proceso, la base, los datos,
   los presets, el contrato 2.0, el esquema ni la huella.
3. **Riesgo heredado y declarado**: el entorno real con 36 h puede dar NoFactible (es un resultado
   válido del experimento) y los dos tests lentos conservan expectativas de la etapa blanda
   (876/848 clases, 300 s) que no se han reescrito para no ocultar el resultado ni forzar la
   factibilidad.
