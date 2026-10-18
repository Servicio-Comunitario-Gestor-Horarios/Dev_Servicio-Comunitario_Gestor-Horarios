# Tareas 009 — Agrupación de bloques contiguos por materia-curso

Base: 7e2688674ac587a808aa94fb76cd991b02db3bda
Spec: `specs/009-agrupacion-bloques-materias/spec.md` (**aprobada**, sin dudas abiertas)
Plan: `specs/009-agrupacion-bloques-materias/plan.md`

- Comando de tests (contenedor `gestor-dev`):
  `docker exec gestor-dev bash -lc "cd /workspace && cmake --preset full-release && cmake --build build-rel && LD_LIBRARY_PATH=$HOME/.local/ortools/lib ctest --preset full-release --output-on-failure"`
- Regla: **tests primero** (rojo → código → verde → marcar → parar). Una tarea cada vez.

---

- [x] **T1. Constante del motor y función pura de segmentos.** RF-1, RF-4
    - Tests (rojo): `test/backend/test_contrato_solver.cpp` — `PESO_AGRUPACION = 10`; jerarquía `1000 > 100 > 10 > 0`; `segmentosTurno(turno Mañana, receso tras slot 1)` → `{{0,1},{2,3,4,5,6,7}}`; `segmentosTurno(turno Tarde, receso tras slot 9)` → `{{8,9},{10,11,12,13,14,15}}`; `esInicioSegmento` y `calcularPenalizacionGrupo(tamaño, horasDia)` según tabla RF-2.
    - Código: `contrato_solver.hpp/.cpp` — añadir `constexpr int PESO_AGRUPACION = 10`; implementar `segmentosTurno`, `esInicioSegmento`, `calcularPenalizacionGrupo` (puras, sin E/S, sin OR-Tools).
    - Hecho cuando: `ctest` pasa `test_contrato_solver` con los casos arriba.

- [x] **T2. Variables de arranque y longitud en el modelo CP-SAT.** RF-1, RF-2
    - Tests (rojo): `test/backend/test_solver_horarios.cpp` — fixture mínimo: 1 curso, 1 materia 2h/día, 1 segmento; comprobar que se crean `start[m,c,d,seg,s]` y `len[m,c,d,seg,s]` con conteos correctos; `start=1` solo en arranques reales.
    - Código: `variables.hpp/.cpp` — en `crearVariables`, recorrer cursos, materias, días, segmentos (de `segmentosTurno`), franjas; crear `start` y `len` (o formulación equivalente con transiciones). Añadir a `VariablesSolver`.
    - Hecho cuando: `ctest` pasa el fixture de variables; conteos coinciden con franjas del segmento.

- [x] **T3. Restricciones de ligadura start/len ↔ x.** RF-1, RF-2
    - Tests (rojo): `test/backend/test_solver_horarios.cpp` — forzar asignaciones `x` en patrón conocido (ej. 1,1,0,1 en un segmento); verificar que `start` y `len` toman valores esperados (arranques en 0 y 3, len=2 y 1).
    - Código: `restricciones_turnos.cpp` (o nuevo `restricciones_agrupacion.cpp`) — añadir `agregarRestriccionesAgrupacion(model, config, vars)` que vincula `start ⇔ x=1 ∧ (x_prev=0 ∨ inicio_segmento)` y define `len` como suma de `x` consecutivos desde `start`. Registrar en `solver.cpp::agregarRestricciones`.
    - Hecho cuando: `ctest` pasa los casos de ligadura; `start`/`len` son consistentes con `x` en todos los patrones de prueba.

- [x] **T4. Función objetivo extendida con S3.** RF-4, RF-5, RF-6
    - Tests (rojo): `test/backend/test_solver_horarios.cpp` — AC-2: escenario donde usar suplente permite mejor agrupación; verificar que solver prefiere titular (S2 gana). AC-6: dos runs con misma entrada → mismo horario. AC-5: receso rompe (slots 1 y 2 Mañana = dos grupos de 1, penalización 2 si horas día > 1).
    - Código: `solver.cpp::construirObjetivo` — añadir término `PESO_AGRUPACION · Σ start · f(len, horasDia)`. `horasDia[m,c,d] = Σ_{seg,s} Σ_p x[p,m,c,d,s]`. `f` usa `calcularPenalizacionGrupo` (o lógica inline equivalente). Verificar que parámetros deterministas (`num_search_workers=1`, `random_seed=0`) siguen activos.
    - Hecho cuando: `ctest` pasa AC-2, AC-5, AC-6; objetivo incluye S3 y no rompe reproducibilidad.

- [x] **T5. Tests de penalización exacta (AC-1, AC-3, AC-4, AC-9).** RF-2, RF-3
    - Tests (rojo): `test/backend/test_solver_horarios.cpp` — AC-1: 4h/semana (1h×2días + 2h×1día) → grupos de 2 en día de 2h. AC-3: 1h/semana → pen 0; 3h/día 2+1 → pen 1. AC-4: 5h en un día/segmento → pen 2. AC-9: cross-segmento (1 slot en {0,1} + 1 en {2..7}) → 2 arranques, horas día=2 → pen 2.
    - Código: fixtures en `test_solver_horarios.cpp` que configuran disponibilidad y cursos para permitir/forzar cada patrón; comprobar `ValidadorSalidaSolver` reporta penalizaciones esperadas.
    - Hecho cuando: `ctest` pasa AC-1, AC-3, AC-4, AC-9.

- [x] **T6. Validador de salida: análisis de grupos y penalizaciones.** AC-1..AC-9
    - Tests (rojo): `test/backend/test_validador_salida.cpp` — `analizarSalidaSolver` devuelve mapa `(curso,materia,día,segmento) → {grupos: [{inicio, fin, tamaño, penalización}], penalización_total_día}`; verificar contra fixture conocido.
    - Código: `ValidadorSalidaSolver.cpp` — en `analizarSalidaSolver`, recorrer asignaciones por (curso,materia,día), agrupar por segmento (usando `segmentosTurno`), detectar arranques contiguos, calcular `tamaño`, `penalización` con `calcularPenalizacionGrupo`, sumar. Añadir a `AnalisisSalida` campo opcional `agrupacion` (no rompe compatibilidad).
    - Hecho cuando: `ctest` pasa `test_validador_salida` con fixtures AC-1..AC-9.

- [x] **T7. Integración completa con datos de prueba (AC-7, AC-8).** RF-5, RF-6, RF-8
    - Tests (rojo): `test/backend/test_solver_horarios.cpp` y `test/backend/test_solver_benchmark.cpp` — generar horario con preset holgada (13 cursos, 520 clases); verificar: (a) tiempo ≤ 5 s; (b) ningún grupo de 1 injustificado (solo 1h/día); (c) ningún grupo >3 sin penalización contabilizada; (d) D1–D13 intactas (usar `ValidadorSalidaSolver`); (e) objetivo S1/S2 no empeora vs run sin S3 (comparar en fixture controlado).
    - Código: ajustes finos en formulación si hay regresión de rendimiento; asegurar que variables auxiliares no explotan.
    - Hecho cuando: `ctest` pasa `test_solver_horarios` (integración) y `test_solver_benchmark` (< 5 s, D1–D13 OK, S1/S2 no empeoran).

- [x] **T8. Suite completa en verde y limpieza.** RF-1..RF-8, RNF-1..RNF-3
    - Tests (verificación): suite completa `ctest --preset full-release --output-on-failure` pasa al 100%. `grep -R "TODO(spec 009)" src test` no devuelve nada.
    - Verificación: `cmake --build build-rel` sin warnings nuevos; constantes en `contrato_solver.hpp`; sin cambios en `solver_config.hpp` ni JSON.
    - Hecho cuando: suite verde; código compila limpio; spec 009 marcada `implementada` en `MEMORY.md` y `spec.md` → `Estado: implementada (2026-10-18)` tras veredicto reviewer.