# Tareas 020 - Tope duro de horas y plazo de una hora

Base: `455ba7f651d59e46f633753ccbe8368f0a37c0f9` · Rama `experimento/horas-duras-plazo-1h`

> **Antes de T1:** el coordinator **commitea la spec y este plan/tareas**; su `sha` es la base de
> implementación y se registra en `journal.md` al cerrar (T8).
>
> Una tarea cada vez: **tests en rojo → código → tests en verde → marcar → parar**. La evidencia va en
> `journal.md`. Versiones: **C1** = `cmake --preset backend && cmake --build --preset backend --parallel &&
> ctest --preset backend`; **C2** = `cmake --preset full-release && cmake --build build-rel --parallel &&
> ctest --preset full-release`; **C3** = `cmake --preset dev-frontend && cmake --build build-fe --parallel`.
> **Nunca** `slow-release` ni el cálculo del centro real: lo lanza el usuario.

- [x] **T1. Tope duro: sin exceso, ancla sin `e*`, tope 0 y ausente; V13 intacta.** RF-1, RF-4, RF-6, RNF-8
    - Archivos: `src/backend/include/backend/solver/variables.hpp`, `src/backend/src/solver/variables.cpp`,
      `src/backend/src/solver/restricciones/restricciones_turnos.cpp`, `src/backend/src/solver/solver.cpp`,
      `src/backend/include/backend/solver/solver.hpp`,
      `src/backend/include/backend/solver/config/contrato_solver.hpp` (comentario de `PESO_HORAS` histórico).
    - Tests primero: en `test/backend/test_solver_horarios.cpp`, reescribir
      `solver_excesoDeHorasEsLaValvulaSinCapacidad` como `solver_topeDuroSinCapacidadEsInfactible` y añadir
      `solver_topeCeroNoRecibeClases`; en `test/backend/test_validador_entrada.cpp`, añadir
      `v13_noFiltraPorCapacidadDeHoras` (docente con `horas_aula = 0` y disponibilidad ⇒ la entrada pasa).
      Ajustar `test_variables_cp_sat.cpp` si cuenta variables.
    - Hecho cuando: `AddLessOrEqual(Sum(x), tope)` sin exceso; el objetivo no suma S1; `AnclajeEstructural`
      sin `e*`; **no existe el miembro `VariablesSolver::exceso` ni `cotaExceso`** (verificado con
      `grep -R "exceso" src/backend/include/backend/solver/variables.hpp src/backend/src/solver/variables.cpp`
      y `grep -R "cotaExceso\|excesoPorDocente" src/` sin resultados); `PESO_HORAS` y su `static_assert`
      quedan **documentados como históricos** (comentario; el `static_assert` se conserva); los tests
      nuevos en verde y **C1** con 0 fallos.

- [x] **T1b. `common/include` en el target `frontend` (soporte de C3).** RNF-7, RF-4
    - Origen: defecto **previo** a T1. `src/frontend/src/services/tipos_aula_ui.hpp` incluye
      `common/tipo_general.hpp` y, con `BUILD_BACKEND=OFF`, la ruta solo llegaba por herencia del
      target `backend` (inexistente en ese modo): C3 fallaba al compilar `classroom_list_widget.cpp`.
    - Archivos: `src/frontend/CMakeLists.txt` (añade `${CMAKE_SOURCE_DIR}/src/common/include` al
      bloque PUBLIC de `frontend`, sin duplicar rutas).
    - Hecho cuando: el target `frontend` compila y enlaza en un directorio de build nuevo con
      `-DBUILD_BACKEND=OFF -DBUILD_TESTING=OFF`; **C1** sigue con 0 fallos. El enlace del ejecutable
      `gestor-horarios` en ese modo falla por un defecto previo **distinto** (`NucleoDatos` en
      widgets), declarado en `journal.md` y fuera del alcance de esta tarea.
    - Evidencia: `journal.md` → T1b (C3 target `frontend` 29/29; C1 62/62).

- [x] **T2. Desenlaces del solver: enum limpio, mapeo y optimalidad por fase.** RF-2, RF-3, RNF-5
    - Archivos: `src/backend/include/backend/solver/desenlace_solver.hpp` (nuevo, **sin OR-Tools**),
      `src/backend/include/backend/solver/desenlace_solver_backend.hpp` (nuevo, **inline** con OR-Tools),
      `src/backend/include/backend/solver/solver.hpp`, `src/backend/src/solver/solver.cpp`.
    - Tests primero: en `test/backend/test_solver_horarios.cpp` (enlaza `backend_solver`), añadir
      `desenlaceDesdeEstado` (`OPTIMAL`/`FEASIBLE` → `Exito`, `INFEASIBLE` → `Infactible`, `UNKNOWN` →
      `NoConcluyente`, `MODEL_INVALID` → `ErrorContrato`) y
      `presupuestoCero_esNoConcluyenteSinEjecutar` (no llama a `ejecutar`, no lanza, desenlace
      `NoConcluyente`). **No** se pone en `test_contrato_solver`, que enlaza solo `backend` y no tiene
      OR-Tools.
    - Hecho cuando: `desenlace_solver.hpp` **no incluye OR-Tools** (solo el enum); `desenlaceDesdeEstado` es
      **inline** en el header de backend; `ResultadoSolver` expone `desenlace` y `fasesNoOptimas`; si
      `restante <= 0` **no se llama a `ejecutar`** (0 = sin límite); **C1** en verde.

- [x] **T3. IPC y servicio: códigos, puerto y estados distintos.** RF-2, RF-3, RNF-4
    - Archivos: `src/middleware/include/middleware/messages.h` (**solo** `RESP_NO_CONCLUYENTE`),
      `src/app/src/aplicacion_backend.cpp`, `src/frontend/src/services/puerto_solver_ipc.cpp/.hpp`,
      `src/backend/include/backend/services/ServicioGeneracion.hpp`, `src/backend/src/services/ServicioGeneracion.cpp`.
      **Sin cambios** en `src/middleware/src/client/internalclient.cpp`.
    - Tests primero: `test/backend/test_contrato_generacion.cpp` añade `NoConcluyente` y `SinRespuesta`;
      `test/backend/test_servicio_generacion.cpp` con doble para `NoConcluyente`, `SinRespuesta` y
      `ErrorContrato`; `test/frontend/test_conversion_entrada.cpp` con `desenlaceParaCodigo`
      (`RESP_EXITO→Exito`, `RESP_SIN_SOLUCION→Infactible`, `RESP_NO_CONCLUYENTE→NoConcluyente`,
      `RESP_ERROR→SinRespuesta`, `RESP_TIEMPO_AGOTADO→SinRespuesta`; respuesta **sin `code`** →
      `ErrorContrato`; `RESP_INVALIDO→ErrorContrato`).
    - Hecho cuando: `RespuestaSolver` usa `DesenlaceSolver`; `Estado` tiene `NoConcluyente` y `SinRespuesta`;
      `RESP_ERROR`/`RESP_TIEMPO_AGOTADO` **solo** se traducen a `SinRespuesta` en el puerto del solver (las
      demás operaciones conservan su comportamiento); **`test_middleware_transport`,
      `test_middleware_health_check` y `test_ipc_aislamiento_bd` pasan sin modificar**; **C1, C2 y C3** en
      verde.

- [x] **T4. Causa mínima (H3), demanda vs topes y docentes sin capacidad.** RF-1, RF-2, RNF-5
    - Archivos: `src/backend/src/services/ServicioGeneracion.cpp`, `src/backend/src/solver/solver.cpp`
      (el diagnóstico demanda/topes vive en `analizarFallo`),
      `src/backend/include/backend/solver/config/contrato_solver.hpp` / `.cpp`,
      `src/backend/src/services/ValidadorSalidaSolver.cpp` / `.hpp`.
    - Tests primero: `test/backend/test_servicio_generacion.cpp` con `noFactible_conservaElTextoDelMotor`,
      `noFactibleSinTexto_informaQueNoHayCausaAtribuible`,
      `noFactibleSoloPorHoras_informaDemandaYTopes` y `docenteSinHorasAula_apareceEnElMensaje`;
      `test/backend/test_contrato_solver.cpp` con `demandaTotalHoras` y `sumaTopesHorasAula`;
      `test/backend/test_validador_salida.cpp` con `docentesSinCapacidad_incluyeTopeCeroDeclaradoYAusente`
      (un `fromJson` sin `horas_aula` se lee como 0).
    - Hecho cuando: el mensaje de NoFactible usa `respuesta.error`; si viene vacío dice **«no hay causa
      atribuible»**; solo-horas añade demanda y suma de topes; `docentesSinCapacidad` incluye el 0 declarado
      y el ausente; **C1** en verde.

- [x] **T5. Plazo 60/55/5 de reloj y textos.** RF-3, RNF-2
    - Archivos: `src/backend/include/backend/solver/solver.hpp`, `src/backend/src/solver/solver.cpp`,
      `src/backend/include/backend/services/ServicioGeneracion.hpp`,
      `src/backend/src/services/ServicioGeneracion.cpp`,
      `src/frontend/src/views/schedule_generation_widget.cpp`.
    - Tests primero: `test/backend/test_solver_bloques.cpp` reescribe `presupuestoPorDefectoDeLa014b`
      (3300 s, 8 workers) y la escalera con `ejecutarSeparacionHuecos`/`ejecutarPreferencias`;
      `test/backend/test_servicio_generacion.cpp` mantiene el timeout con `plazoMs = 50` y comprueba
      `PLAZO_MS_POR_DEFECTO`; se actualiza `test/backend/test_solver_centro_sin_ef.cpp` (lento, solo compila).
    - Hecho cuando: `ejecutar` usa `max_time_in_seconds` y **nunca** recibe 0; el deadline de 55 min es
      único y cada fase recibe el restante; el servicio espera 60 min; la vista instancia 60 min y anuncia
      «hasta una hora»; **C1, C2 y C3** en verde.

- [x] **T6. Tope posterior contra `horas_aula`, reparto y separación P4/P5.** RF-5, RF-1
    - Archivos: `src/backend/include/backend/services/ValidadorSalidaSolver.hpp`,
      `src/backend/src/services/ValidadorSalidaSolver.cpp`, `src/backend/src/services/ServicioGeneracion.cpp`,
      `src/frontend/src/views/schedule_generation_widget.cpp`.
    - Tests primero: `test/backend/test_validador_salida.cpp` con `repartoPorDocenteYtotales`,
      `excesoDeTopeDeHorasAula_noEsPresentable`, `p2YElTopeDeHorasAulaSonComprobacionesDistintas`,
      `hayConflictoSolapamiento_seConserva` y `repartoTexto_incluyeSinCapacidad`; en
      `test/backend/test_servicio_generacion.cpp`, `calculoFallidoPorSolapamiento_noSeConfundeConTope` y
      `calculoFallidoPorTopeDeHorasAula_mensajeDistinto`, más `reparto_enElResultado` y
      `fasesNoOptimas_enElResultado`.
    - Hecho cuando: `presentable()` exige `excesosTopeHoras` vacío; `hayConflictoSolapamiento()` se conserva;
      **P4 (solapamientos) y P5 (exceso de `horas_aula`) tienen mensajes distintos**; P2 sigue midiendo
      `horas_requeridas`; el reparto (una línea por docente + totales + sin capacidad) se muestra en el
      diálogo existente y la no optimalidad se refiere a la fase; **C1, C2 y C3** en verde.

- [x] **T7. Barrido de tests superseded y suites completas.** RNF-1, RF-3, RF-4
    - Archivos: `test/backend/test_contrato_solver.cpp` (pesos históricos: `PESO_HORAS` documentado, solo
      `COSTE_SUPLENTE > PESO_PREFERENCIA` como activo), `test/backend/test_solver_horarios.cpp`,
      `test/backend/test_solver_preferencias.cpp`, `test/backend/test_solver_bloques.cpp`,
      `test/backend/test_solver_aforo.cpp`, `test/backend/test_analisis_bloques.cpp`,
      `test/backend/test_servicio_generacion.cpp`, `test/backend/test_contrato_generacion.cpp`,
      `test/backend/test_solver_centro_sin_ef.cpp` (solo compilación), `test/frontend/test_conversion_entrada.cpp`.
    - Tests primero: localizar y reescribir lo que afirme el comportamiento sustituido (exceso permitido,
      penalización S1, ancla `e*`, presupuestos 60/20/10, plazo 300 s, P2 como tope, estados de generación).
      Ajustarlos **no es regresión** (RNF-1).
    - **No ejecutados por el agente:** `test/backend/test_solver_centro_sin_ef.cpp` y
      `test/frontend/test_solver_centro_real.cpp` (etiqueta `slow`). Este último afirma **876 clases
      factibles**; si con el tope duro el centro es **NoFactible con 36 h**, esa expectativa queda
      **obsoleta** y se decide tras la corrida manual del usuario. No se fuerza la factibilidad para
      ocultar el resultado del experimento.
    - Hecho cuando: **C1 y C2** con 0 fallos y ningún test rápido en rojo; no se ha ejecutado el lento; la
      lista de tests reescritos y de expectativas obsoletas queda en `journal.md`.

- [x] **T8. Cierre del experimento (sin ejecutar el solver lento).** RNF-3, RNF-4, RNF-7, RNF-10
    - Archivos: ninguno de código; `journal.md` y, si procede, `estado.md`.
    - Hecho cuando: `journal.md` registra **el `sha` del commit de la spec/plan** (base de implementación) y
      los `sha` de las tareas si los hay; `git status --short -- src/ test/` solo muestra archivos del plan;
      `git diff --stat` contra `455ba7f` no toca `src/backend/src/database/`, `src/middleware/src/client/`,
      presets, esquema ni datos; **C1, C2 y C3** verdes; queda la nota de reversión (volver a tope blando y
      300 s) y la confirmación de que **no** se ha lanzado `slow-release` ni el centro real.

**Exceso declarado:** T1 (~40 min), T3 (~40 min), T5 (~40 min) y T6 (~40 min) superan los 30 min
recomendados; el resto queda en ~15–30 min. Total: **8 tareas**, dentro del máximo de 10.
