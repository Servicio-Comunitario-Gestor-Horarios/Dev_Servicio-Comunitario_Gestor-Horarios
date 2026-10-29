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

## Residuales de la validación: avisos P1/P3, metadata y estado del solver (2026-10-29)

**Base:** `57725b6`. Deudas de `estado.md` («Residuales») y de la decisión I2 de la 005.

### F1 - Retirada de los avisos P1 y P3

- `ValidadorSalidaSolver.cpp`: se elimina la generación del aviso de horas no cubiertas (P1) y la de
  capacidad de aula (P3), con sus conteos y los ayudantes que solo ellas usaban
  (`indiceCurso`, `horasEsperadas`). Se conserva **P2** (exceso de horas de profesor) y **P4**
  (solapamientos, no presentable).
- Cabecera: solo P2 genera avisos; P4 no es presentable; se anota que I2 retiró P1/P3.
- Tests: `horasNoCubiertas_noGeneraAviso`, `excesoDeCapacidadDeAula_noGeneraAviso` y
  `aulaFijaConClasesEnAulasDistintas_esPresentableSinAvisos` (antes contaban avisos; el `grep`
  previsto no los detectó porque los nombres no contenían las cadenas literales). P2 y P4 siguen
  cubiertos y en verde. Suite **54/54**.
- **Docs pendientes (no editadas):** `docs/interfaz-frontend.md:607` («avisos P1–P3 y conflictos
  P4») y `:667-669` (descripción de P1/P2/P3 en la tabla de `ValidadorSalidaSolver`).

### F2 - Metadata de la salida con datos reales

- `Solver::extraerSolucion` cuenta las **clases emitidas** (`total_asignaciones`), los **docentes
  distintos** con al menos una clase (`profesores_asignados`) y publica el **tiempo** medido por el
  solver (`tiempo_ejecucion_ms`, rellenado en `resolver` desde el `QElapsedTimer`).
  `cursos_generados` sigue siendo el número de cursos.
- Test `solver_metadataCuadraConLaSalida`: con el fixture mínimo, la metadata cuadra con lo emitido
  (8 clases y los docentes de la salida). Antes `total_asignaciones` valía 48 (variables del modelo).
  Suite **54/54**.

### F3 - `Solver::resolver` sin estado arrastrado

- `resolver` reinicia `model` (`CpModelBuilder`) y `vars` (`VariablesSolver`) al inicio; dos llamadas
  a la **misma** instancia equivalen a dos nuevas.
- Tests: `solver_reutilizarLaMismaInstancia_daElMismoResultado` (mismo horario y mismos errores) y
  `solver_reutilizarLaInstancia_noContaminaConOtraEntrada` (factible → infactible → factible sin
  heredar nada). Antes, la segunda resolución daba otro horario y la tercera era INFEASIBLE.
  Suite **54/54**.

## T10 - Cierre de deudas: RF-13, cobertura RNF-4 y objetivo/contrato (2026-10-29)

**Base:** `b1b9711`. Tres bloques, un commit cada uno.

### C1 - Los planes de los docentes también se embeben (RF-13)

- `ConstructorEntradaSolver`: `embeberPlan(codigo)` evita duplicados, resuelve el plan del centro y
  lo añade; primero se recorren los **cursos** y después los **docentes**, en el orden de sus listas
  (orden determinista). Un `plan` vacío no añade nada (D11) y un código inexistente se ignora (V10/V11
  lo juzgan).
- Tests: `planSoloDeDocentes_seEmbebeAlFinal` (plan sin curso que lo use viaja como segundo plan, con
  su materia resuelta), `planDeDocenteQueCoincideConElDelCurso_noSeDuplica`,
  `planDeDocenteInexistenteOVacio_seIgnora`. Rojo previo en el primero (planes 1 ≠ 2). Suite **54/54**.

### C2 - Cobertura RNF-4 del validador (V7, V8, V15, D13)

- `v7_turnoAusente_falla`: curso sin `turno` → V7 («no declara turno»).
- `v8_modoMovilSinAulaDeCurso_pasa`: `aula_fija = false` y curso sin `aula` → la entrada pasa (el
  aula se ignora).
- `v15_movilSinAulasEnElCentro_falla`: móvil con `aulas` vacío y curso sin aula → V15.
- `v13_d13_docenteDeOtroAnioNoEsElegible_falla`: el único docente declara `anios = [2]` y el curso es
  de 1.º → sin candidato elegible (V13; la regla D13 está en `docenteCandidatoElegible`).
- Los cuatro casos pasan con el código actual: no destaparon incumplimientos. Suite **54/54**.

### C3 - Objetivo y contrato (RNF-4)

- `solver_excesoDeHorasEsPenalizadoYNoInfactible` (S1): un docente con `horas_aula = 2` cubre 4
  bloques; el modelo es factible y el exceso se penaliza con `ContratoSolver::PESO_HORAS`.
- `solver_titularAlTope_usaAlSuplente` (S2): el titular (2 h de aula, 4 bloques) no da ninguna clase y
  el suplente elegible cubre las 4 (coste `COSTE_SUPLENTE`). El caso simétrico «sin suplente → exceso
  permitido» es el de S1.
- **Aula de salida como índice:** ya estaba cubierto en los dos modos
  (`solver_aulaFijaUsaLaReferenciaSiElTipoCoincide` afirma `asig.aula == 0`;
  `solver_modoMovilEligeAulaDelTipoRequerido` afirma los índices 0/1/2), así que no se duplicó.
- `test_contrato_solver::idaYVuelta2_0_sinPerdida` amplía: un par suplente con `peso = 7` sobrevive
  `toJson`/`fromJson` y el JSON vuelve idéntico.
- Suite **54/54**; `test_solver_horarios` 48 casos y `test_contrato_solver` 13.
