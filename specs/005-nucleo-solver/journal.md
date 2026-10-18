# Journal 005 — Núcleo del solver y contrato de configuración

Registro de la implementación de las tareas del plan 005. Una entrada por tarea ejecutada.

---

## T11 — Rectificación en código: hora académica = bloque

**Fecha:** 2026-10-17

### Qué se hizo

- Se cambió la conversión de `bloques()`/`bloquesAula()` a la **identidad** (1 hora académica = 1 bloque), según la enmienda de la constitución §7 y la decisión D-15 del plan: `bloques(horas, duracion) = horas`, `bloquesAula(horas_aula, duracion) = horas_aula`, conservando la firma con `duracion_minutos` y la guarda `0` si `duracion_minutos <= 0`.
- Se actualizaron los tests afectados y se añadieron las comprobaciones de la rectificación:
  - `test_contrato_solver.cpp`: `bloques`/`bloquesAula` identidad (`bloques(3,45)==3`, `bloques(2,45)==2`, `bloques(4,40)==4`, `bloques(6,60)==6`, `bloquesAula(8,45)==8`), conservando la guarda `duracion_minutos <= 0 → 0`.
  - `test_validador_entrada.cpp`: V14 con `Σ horas_m` — un curso con 41 h académicas se rechaza y con 40 se acepta (franjas útiles = 5 días × 8 franjas = 40).
  - `test_solver_horarios.cpp`: D3 asigna exactamente `horas_m` bloques y S1 compara con `horas_aula` sin ×60 (materia de 2 h con `duracion_minutos = 45` ocupa exactamente 2 bloques, no `ceil(2×60/45)=3`).

### Archivos tocados

- `src/backend/src/solver/config/contrato_solver.cpp` — `bloquesConDuracion` pasa de `ceil(horas × 60 / duracion_minutos)` a la identidad (con guarda `duracion_minutos <= 0 → 0`).
- `test/backend/test_contrato_solver.cpp` — tests de identidad.
- `test/backend/test_validador_entrada.cpp` — helpers `ponerHorasPlan`/`conFranjasUtiles40` y tests `v14_masHorasQueFranjas_falla`/`v14_horasDentroDeFranjas_pasa`.
- `test/backend/test_solver_horarios.cpp` — test `solver_d3_bloquesSonHoras_s1_topeSinConversion`.

No se tocó ningún otro cálculo del solver: los call sites de `bloques`/`bloquesAula` (`solver.cpp` `bloquesMateria`/`bloquesCurso`, `restricciones_turnos.cpp` `restriccionTopeHorasProfesor`, `restricciones_profesores.cpp` `restriccionUnDocentePorMateria`, `solver_config.cpp` V14) ya pasaban por estas funciones y quedan correctos con la identidad.

### Verificación (rojo → verde)

1. **Baseline verde** (antes de tocar nada): `test_contrato_solver`, `test_validador_entrada` y `test_solver_horarios` pasaban con el modelo anterior.

2. **Rojo** (tests primero, tras escribir los tests pero antes del código):
   ```
   FAIL!  TestContratoSolver::bloques_esIdentidad() — Actual (bloques(3,45)): 4, Expected: 3
   FAIL!  TestContratoSolver::bloquesAula_esIdentidad() — Actual (bloquesAula(8,45)): 11, Expected: 8
   test_validador_entrada: 1 failed (v14_horasDentroDeFranjas_pasa: 40 h → 54 bloques → rechazado)
   FAIL!  TestSolverHorarios::solver_d3_bloquesSonHoras_s1_topeSinConversion()
          — INFEASIBLE: el curso 1|A necesita 3 bloques y su turno ofrece 2 franjas
   ```

3. **Verde** (tras la identidad):
   ```
   docker exec gestor-dev bash -lc "cd /workspace && cmake --preset full && cmake --build build && ctest --preset full --output-on-failure"
   → 100% tests passed, 0 tests failed out of 44
   ```

4. **Grep de conversiones** (sin resultados):
   - `grep -R "\* 60\|60 /\|ceil" src/backend/src/solver` → sin coincidencias.
   - `grep -R "/ duracion\|duracion_minutos /\|/ c\.franja\|/ dur" src/backend/src/solver` → sin coincidencias.

### Hallazgos y notas

- La única conversión de horas a bloques vivía en `contrato_solver.cpp` (`bloquesConDuracion`); el resto del solver ya pasaba por `bloques`/`bloquesAula`, por lo que con la identidad quedó todo correcto sin más cambios.
- En `ConstructorEntradaSolver.cpp` (`src/backend/src/services/`, **fuera** del directorio `src/backend/src/solver/`) sigue existiendo `duracionMinutos * 60` en `slotsDeFranja`, pero es la traducción de una franja de **reloj** a índices de slot (rejilla horaria/disponibilidad), no la conversión de horas académicas a bloques; queda fuera del alcance de T11 y es correcta según la definición de `duracion_minutos` («duración real de la franja»).
- La identidad del tope S1 (`bloquesAula = horas_aula`) no es observable de forma aislada de D3 (comparten la misma fórmula de conversión), por lo que se cubre explícitamente en `test_contrato_solver.cpp` (`bloquesAula_esIdentidad`) y de forma end-to-end en el test D3 del solver.
- El «Cierre» de la tarea (actualizar `MEMORY.md` y el campo Estado de `spec.md` a **implementada**) **no** se ejecuta en esta pasada por instrucción explícita del coordinador (no modificar `spec.md`/`plan.md`/`estado.md`); queda pendiente para una pasada posterior.

---

## Corrección puntual — P4: clave de ocupación por entidad (docente/aula)

**Fecha:** 2026-10-18

### Qué se hizo

- `analizarSalidaSolver` usaba `QMap<QPair<int,int>, QString>` con clave `(día, slot)` para la ocupación de docentes y aulas, así que **cualquier** segunda clase en la misma franja se marcaba como solapamiento aunque fuese otro docente en otra aula. Con datos reales (13 cursos, 520 clases, 80 casillas) producía 880 conflictos falsos y `presentable()` devolvía `false`, rechazando en la UI un horario válido del solver.
- La clave pasa a `std::tuple<int,int,int>`:
  - `ocupacionProfesor{asig.profesor, dia.dia, asig.slot}`
  - `ocupacionAula{asig.aula, dia.dia, asig.slot}`
- Guardas: no se comprueba ni registra ocupación cuando `asig.profesor < 0` ni cuando `asig.aula < 0` (evita agrupar todos los «sin asignar» bajo la misma clave). El chequeo D4 de curso (`slotsDelCurso`) queda intacto, igual que los mensajes en español, los conteos P1/P2 y la validación P3.

### Archivos tocados

- `src/backend/src/services/ValidadorSalidaSolver.cpp` — `#include <tuple>`, claves compuestas y guardas de `-1`.
- `test/backend/test_validador_salida.cpp` — test de regresión `dosCursosMismoDiaYSlot_distintoDocenteYAula_esPresentable` (config local con segundo docente «Luis» y segunda aula «A2»; ambos cursos en día 0, slots 0 y 1).

### Verificación (rojo → verde)

1. **Rojo** (test nuevo contra el validador sin arreglar):
   ```
   FAIL!  : TestValidadorSalida::dosCursosMismoDiaYSlot_distintoDocenteYAula_esPresentable()
           'a.conflictos.isEmpty()' returned FALSE.
   Totals: 8 passed, 1 failed, 0 skipped
   ```
2. **Verde** (`cmake --build build-rel --target test_validador_salida` + ejecución):
   ```
   Totals: 9 passed, 0 failed, 0 skipped, 0 blacklisted, 1ms
   ```
   Incluye los tests previos de solapamiento real (`solapamientoDeProfesor_noPresentable`, `solapamientoDeAula_noPresentable`), que siguen detectando el conflicto.
3. **Suites relacionadas** (consumidores del validador):
   - `test_backend_contrato_interfaz` → `Totals: 12 passed, 0 failed`.
   - `test_servicio_generacion` → `Totals: 12 passed, 0 failed`.
   - `test_contrato_generacion` → `Totals: 7 passed, 0 failed`.

### Hallazgos y notas

- Aviso de compilación preexistente y ajeno: `horasEsperadas` declarada y no usada en `ValidadorSalidaSolver.cpp` (línea 21). No se elimina.
- `src/backend/src/solver/solver.cpp` (`metadata.total_asignaciones`) y el frontend quedan fuera de esta corrección; `src/frontend/src/views/classroom_list_widget.hpp` ya tenía cambios sin commitear en el árbol de trabajo y no se ha tocado.

---

## Corrección puntual — `metadata.total_asignaciones` cuenta clases, no variables

**Fecha:** 2026-10-18

### Qué se hizo

- `Solver::extraerSolucion` asignaba `metadata.total_asignaciones = vars.assignment.size()`, es decir el número de **variables booleanas** del modelo (5 840 con los datos de prueba), no de **clases** extraídas (520). La UI mostraba ese metadato.
- Ahora se acumula `totalAsignaciones` en el mismo bucle que construye `porCursoDia`, incrementando solo cuando `SolutionBooleanValue(response, it.value())` es cierto (sin recorridos extra).
- `metadata.cursos_generados` y `metadata.profesores_asignados` quedan intactos, igual que el resto de la extracción.

### Archivos tocados

- `src/backend/src/solver/solver.cpp` — contador `totalAsignaciones` + comentario en la asignación del metadato.
- `test/frontend/test_generacion_seed.cpp` — invariante: `metadata.total_asignaciones` del JSON `respuesta.salida` == suma de `asignaciones` de todos los `dias` de los 13 `horarios` (incluye `<QJsonArray>`, `<QJsonObject>`).

### Verificación (rojo → verde)

1. **Rojo** (test del invariante con el solver anterior):
   ```
   FAIL!  : TestGeneracionSeed::componeYResuelveHorarioConDatosSembrados() Compared values are not the same
      Actual   (totalMetadato)     : 5840
      Expected (asignacionesReales): 520
   ```
2. **Verde**:
   ```
   LD_LIBRARY_PATH="$HOME/.local/ortools/lib" ./build-rel/test/test_solver_horarios
   → Totals: 16 passed, 0 failed
   LD_LIBRARY_PATH="$HOME/.local/ortools/lib" ./build-rel/test/test_generacion_seed
   → Totals: 3 passed, 0 failed
   ```
   Los mismos datos de 13 cursos pasan de 5 840 a 520 asignaciones, coincidiendo con la suma real del horario.
3. **Regresión relacionada**: `test_servicio_generacion` → `Totals: 12 passed, 0 failed`.
