# Tareas 005 — Núcleo del solver y contrato de configuración

Base: f94acba455a04ae80364c82e1f07ae16d9f156ec

- Spec: `specs/005-nucleo-solver/spec.md` (**aprobada**)
- Plan: `specs/005-nucleo-solver/plan.md`
- Depende de: `specs/004-persistencia-campos-curso` (**implementada**: `Cursos(anio, seccion, turno, aula)`, esquema único, cascadas con anulación).
- Interfaz de consumo (otro equipo): `docs/interfaz-frontend.md`. **`src/frontend` no se toca.**
- Comando de tests (contenedor `gestor-dev`, OR-Tools solo en Docker):
  `docker exec gestor-dev bash -lc "cd /workspace && cmake --preset full && cmake --build build && ctest --preset full --output-on-failure"`
- Registro de tests: `test/CMakeLists.txt` con `add_qtest(<nombre> backend/test/<archivo>.cpp backend)` (o `add_gtest`).
- Regla: **tests primero** (rojo → código → verde → marcar → parar). Una tarea cada vez.
- **Modo didáctico:** el **agente escribe los tests**; el **usuario escribe el código de producción**; el agente revisa.

> **Nota de secuencia.** **T1 cambia las estructuras del contrato `SolverConfig`** (aula fija global, curso por identidad, planes embebidos, `anios`): al hacerlo, los sitios que construyen `CursoSolverConfig`/`ProfesorSolverConfig` quedan **sin compilar** hasta que T3–T4 los adaptan. T1 debe dejar el árbol **compilando** (adaptar call sites a la firma nueva) aunque los tests de `constructor_entrada`/`validador_entrada`/`servicio_generacion`/`solver_*` sigan **rojos/degradados** hasta T3, T4, T6–T9. Los 4 rojos de la suite (`test_constructor_entrada`, `test_validador_entrada`, `test_ipc_aislamiento_bd`, `test_solver_horarios`/`benchmark`) se cierran a lo largo de T3–T9; la suite vuelve a verde en **T10**.

---

- [x] **T1. Contrato de configuración: estructuras y serialización del modelo nuevo.** RF-7, RF-8, RF-10
    - Tests (rojo): `test/backend/test_cargador_configuracion.cpp` **(modificado)** — ida y vuelta `fromJson(toJson())` equivalente con: `aula_fija` global (bool, por defecto `true`), curso `{anio, seccion, turno, plan, num_estudiantes, aula}`, **sin `nombre`**, `planes` embebidos (con materias+horas+año), docente con `anios` y `materias_suplente[{materia_idx, peso}]`, `num_estudiantes`/`horas_requeridas` ausentes; el JSON serializado **no** contiene `nombre`; **campos desconocidos** presentes se toleran (no son error ni pérdida); versión `"1.0"`.
    - Código: `solver_config.hpp` — `SolverConfig` gana `bool aula_fija = true` y `QVector<PlanSolverConfig> planes`; `CursoSolverConfig {anio, seccion, turno, plan, num_estudiantes=-1, aula=-1}` (fuera `nombre`, `aula_fija` int, `materias`); `ProfesorSolverConfig` gana `QVector<int> anios` y `horas_requeridas=-1`; nuevos `PlanSolverConfig{codigo, materias}` y `MateriaPlan{materiaIDx, curso, horasSemanales}`. `solver_config.cpp` — `fromJson`/`toJson` nuevos (parseo de `aula_fija` global, `planes`, `anios`, `peso`; **ignora `nombre`**); helper puro `materiasDelCurso`. Adaptar los call sites que construyen `CursoSolverConfig`/`ProfesorSolverConfig` para que **compile**.
    - Hecho cuando: `ctest` pasa `test_cargador_configuracion`; el JSON serializado no contiene `nombre` y el ida y vuelta reproduce los campos del contrato.
    - Nota: al cerrar T1, `test_constructor_entrada`/`test_validador_entrada`/`test_servicio_generacion`/`test_solver_*` quedan **rojos** hasta T3/T4/T6–T9 (nota de secuencia).

- [x] **T2. Funciones puras del contrato y constantes del motor.** RF-1, RF-3, RF-5
    - Tests (rojo): `test/backend/test_contrato_solver.cpp` **(nuevo)** — `bloques`/`bloquesAula` con `duracion_minutos` ≠ 60 (redondeo hacia arriba; `duracion_minutos ≤ 0` → 0); `claveCurso(1,"A")` canónica y determinista (distinta para distinto año o sección); `PESO_HORAS > COSTE_SUPLENTE > 0`. Registrar con `add_qtest(test_contrato_solver backend/test/test_contrato_solver.cpp backend)` en `test/CMakeLists.txt`.
    - Código: `contrato_solver.hpp`/`.cpp` (nuevos) — `bloques`, `bloquesAula`, `claveCurso` (puras, sin OR-Tools) y las constantes `PESO_HORAS`, `COSTE_SUPLENTE`, `COSTE_MAX_SUPLENTE` (reservada). Añadir el `.cpp` al target `backend` en `src/backend/CMakeLists.txt`.
    - Hecho cuando: `ctest` pasa `test_contrato_solver`; las funciones no dependen del reloj ni de OR-Tools.

- [x] **T3. Validación previa V1–V15 en el contrato.** RF-11, RF-7, RF-8, RF-10
    - Tests (rojo): `test/backend/test_validador_entrada.cpp` **(modificado)** — un caso por regla: `peso` negativo (V2), `duracion_minutos ≤ 0` (V2), índices en rango (V4–V6), turno de curso ausente (V7), aula fija válida según el **modo global** (V8: activo exige aula; inactivo la ignora), `num_estudiantes` ausente/presente (V9), plan obligatorio y único (V10/V11), disponibilidad en rango (V12), sin candidato elegible por plan/turno/año/disponibilidad (V13), más bloques que franjas (V14), móvil sin aulas (V15), versión ausente/no reconocida. **Se retira la inyección `TODO(spec 005)`** (las materias ya vienen del plan).
    - Código: `solver_config.cpp` — ejecuta V1–V15 en `fromJson` usando `bloques`/`materiasDelCurso` (V14) y la elegibilidad sobre contratos (V13); mensajes en español con causa y entidad. `ValidadorEntradaSolver` sin cambios (fachada).
    - Hecho cuando: `ctest` pasa `test_validador_entrada` con un caso por regla; ninguna validación invoca al solver.

- [x] **T4. Construcción de la entrada desde BD + preset (RF-13).** RF-13, RF-7, RF-8, RF-10
    - Tests (rojo): `test/backend/test_constructor_entrada.cpp` **(modificado)** — el JSON emitido arma cada curso con `anio`/`seccion`/`turno`/`aula`(índice) de la BD y `plan`/`num_estudiantes` del preset; las materias del curso se derivan del plan; `planes` embebe **solo** los referenciados (cursos + docentes); `aula_fija` global del preset; modo móvil no enlaza aulas; sigue **consumible** por `fromJson`; pureza con instante inyectado. **Se retira la inyección `TODO(spec 005)`**. En `test/backend/test_servicio_generacion.cpp` **se retira su inyección `TODO(spec 005)`**. `test/test_ipc_aislamiento_bd.cpp` pasa a **verde** (la entrada construida ya es válida).
    - Código: `ConstructorEntradaSolver.hpp` — `DatosDominio` lleva los planes compuestos (`QVector<PlanEstudio>` con materias+horas); `ConstructorEntradaSolver.cpp` — merge BD+preset por identidad, embebido de planes referenciados, traducción de aula a índice, `aula_fija` global; **retirar el `TODO(spec 005)`** y el uso de `nombreCurso`. Compone los planes desde `ServicioComposicionPlan`/`PlanEstudio_Materia`.
    - Hecho cuando: `ctest` pasa `test_constructor_entrada`, `test_servicio_generacion` y `test_ipc_aislamiento_bd`; `grep -R "TODO(spec 005)" src test` no devuelve nada.

- [x] **T5. Variables CP-SAT: activación docente y aulas móviles.** RF-1, RF-3, RF-4
    - Tests (rojo): `test/backend/test_variables_cp_sat.cpp` **(modificado)** — poda por **años** (D13), creación de `asignacionDocente[p,c,m]`, `aulaAssignment` solo cuando el modo es **móvil** (`aula_fija == false`), respeto de disponibilidad y turno; conteos exactos con materias derivadas del plan.
    - Código: `variables.hpp`/`.cpp` — `VariablesSolver` gana `asignacionDocente`; `crearVariables` recorre las materias del curso vía `materiasDelCurso`, aplica elegibilidad por plan/turno/año/disponibilidad y crea `aulaAssignment` según `config.aula_fija`.
    - Hecho cuando: `ctest` pasa `test_variables_cp_sat` con los conteos esperados.

- [x] **T6. Restricciones duras D1–D5, D8, D9, D11, D13.** RF-1, RF-2, RF-3, RF-4
    - Tests (rojo): `test/backend/test_solver_horarios.cpp` **(reescrito)** — D1 (cada materia-curso la dicta **exactamente** un docente), D3 (exactamente `bloques(horas)`), D4/D5 (sin solapamientos de curso/profesor), D8 (aula no solapada, fijas incluidas), D9 (curso móvil ocupa exactamente un aula), D10 (aula fija), D11 (dentro del turno), D13 (años). Se **elimina** la comprobación de D12 (materia única por aula).
    - Código: `restricciones_profesores.cpp` (`restriccionUnDocentePorMateria` = D1+D2+D3 por `z`; `restriccionDocentePorAnio` = D13; conserva D5), `restricciones_turnos.cpp` (D4 + D3 con `bloques`), `restricciones_aulas.cpp` (D8 incluyendo fijas; **elimina** D12; `restriccionCursoOcupaUnAula` = D9).
    - Hecho cuando: `ctest` pasa los casos de invariantes duras de `test_solver_horarios`; ninguna materia-curso se reparte entre dos docentes.

- [x] **T7. Función objetivo S1+S2 y reproducibilidad.** RF-5, RNF-1
    - Tests (rojo): `test/backend/test_solver_horarios.cpp` — **S1**: con docentes «de sobra» **hay** solución (no INFEASIBLE por horas); el exceso se permite y se minimiza (último recurso). **S2**: un titular a tope (sin capacidad libre) es cubierto por un **suplente** elegible; con titular con capacidad, la cubre el titular. **Reproducibilidad**: la misma entrada produce el mismo resultado.
    - Código: `restricciones_turnos.cpp` — `restriccionTopeHorasProfesor` (tope + exceso `e[p]`), **sustituye** la igualdad dura de horas; `solver.cpp` — fija el objetivo `PESO_HORAS·Σ e[p] + COSTE_SUPLENTE·Σ z_suplente` y los parámetros deterministas (`num_search_workers = 1`, `random_seed = 0`).
    - Hecho cuando: `ctest` pasa los casos de objetivo de `test_solver_horarios`; ninguna restricción dura se relaja para mejorar el objetivo.

- [x] **T8. Extracción de la solución: clave = identidad y aula = índice.** RF-3, RF-6, RF-9, RNF-3
    - Tests (rojo): `test/backend/test_solver_horarios.cpp` — la salida agrupa por `claveCurso(anio,seccion)` (p. ej. `"1|A"`), **no** por nombre; cada clase lleva el **índice** de aula; aula fija vs móvil. `test/backend/test_validador_salida.cpp` **(modificado)** — el análisis casa por la **identidad** del curso (P1/P2/P4), no por nombre. Fallos con causa en español.
    - Código: `solver.cpp` — `extraerSolucion` usa `claveCurso(c.anio, c.seccion)` y el aula como índice (D10 fija / D9 móvil); `analizarFallo` con las categorías de causa (RF-6). `ValidadorSalidaSolver.cpp` — `indiceCurso` casa por identidad.
    - Hecho cuando: `ctest` pasa los casos de salida de `test_solver_horarios` y `test_validador_salida`; la clave del horario es la identidad y el aula un índice.

- [x] **T9. Rendimiento del escenario de gran tamaño.** RF-12, RNF-2
    - Tests (rojo): `test/backend/test_solver_benchmark.cpp` **(reescrito)** — el escenario de gran tamaño se resuelve en **< 60 s** (entorno de referencia del banco de pruebas) y la salida no tiene solapamiento de profesor ni de aula, con todos los cursos identificados por identidad.
    - Código: el necesario para que el escenario sea soluble en plazo (ajuste del modelo ya introducido en T5–T8; sin relajar invariantes).
    - Hecho cuando: `ctest` pasa `test_solver_benchmark` en < 60 s con salida consistente.

- [ ] **T10. Suite completa en verde y limpieza.** RF-12, RNF-4
    - Tests (verificación): la **suite completa** pasa con el comando del contenedor; `test_constructor_entrada`, `test_validador_entrada`, `test_ipc_aislamiento_bd` y `test_solver_horarios`/`benchmark` (los 4 rojos) quedan **verdes**.
    - Verificación: `grep -R "TODO(spec 005)" src test` no devuelve nada; no queda ninguna referencia a `curso.nombre`/`aula_fija` por curso en el solver.
    - Hecho cuando: `ctest --preset full --output-on-failure` pasa al 100 % con el modelo anterior a la rectificación. El cierre y el cambio de Estado a **implementada** se hacen en **T11** (no aquí).

- [x] **T11. Rectificación en código: hora académica = bloque.** RF-1, RF-5, RF-11
    - Tests (rojo): `test/backend/test_contrato_solver.cpp` — `bloques`/`bloquesAula` son la **identidad**: `bloques(3,45) == 3` (antes 4), `bloques(2,45) == 2` (antes 3), `bloques(4,40) == 4`, `bloques(6,60) == 6`; la guarda `0` con `duracion_minutos ≤ 0` se conserva. `test/backend/test_validador_entrada.cpp` — V14 con `Σ horas_m` (un curso con 41 h académicas se rechaza; con 40 se acepta). `test/backend/test_solver_horarios.cpp` — D3 asigna exactamente `horas_m` bloques y S1 compara con `horas_aula` sin ×60.
    - Código: `contrato_solver.cpp` — `bloques`/`bloquesAula` devuelven `horas`/`horas_aula` (guarda `duracion ≤ 0 → 0`); revisar `solver_config.cpp` (V14), `solver.cpp` y `restricciones_*` para que no quede ninguna multiplicación por 60 ni división por `duracion_minutos` en la conversión.
    - Hecho cuando: `cmake --preset full && cmake --build build && ctest --preset full --output-on-failure` pasa al 100 %; `grep -R "\\* 60\\|60 /" src/backend/src/solver` no devuelve conversiones de horas; cada RF sigue con comprobación verificable (RNF-4). **Cierre:** actualizar `MEMORY.md` y el campo Estado de `spec.md` a **implementada**.
