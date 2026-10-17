# Estado 005

Fase: cerrada
Aprobaciones: spec 2026-10-17 (rectificada) · plan/tareas 2026-10-17 · implementación 2026-10-17
Base: f94acba455a04ae80364c82e1f07ae16d9f156ec
Dudas abiertas: ninguna
Veredicto: APROBADO (2026-10-17)
Siguiente paso: los pendientes de **T10** pasan a una spec futura (006/007 o una nueva); abrir PR de la rama a `develop` cuando el usuario lo pida. No hay más trabajo en esta spec.

## Implementado

- **T11 (rectificación 2026-10-17):** hora académica = bloque. `bloques(horas, duracion) = horas` y
  `bloquesAula = horas_aula` (guarda `0` si `duracion_minutos ≤ 0`, defensiva); `duracion_minutos` solo
  define la duración real de la franja. V14 sobre `Σ_m horas_m`; D3 y S1 sin conversión. Tests de
  contrato, validador y solver actualizados; `docs/constitution.md` §7 enmendada.
- Suite **46/46** en verde en el contenedor (`ctest --preset full`), incluidos los 2 tests del seed.

## Pendientes (T10, abierta y documentada)

- **RF-13:** `ConstructorEntradaSolver` no embebe los planes de los **docentes** (solo los de los cursos).
- **RNF-4:** faltan casos rojo→verde de V2 (`peso` negativo, `duracion_minutos ≤ 0`), V7, V8, V14, V15,
  D13, S1/S2, reproducibilidad (RNF-1), aula de salida como índice y `peso` en el ida y vuelta.
- **Decisión I2:** `ValidadorSalidaSolver` sigue emitiendo los avisos retirados (capacidad de aula,
  horas no cubiertas) y `test_validador_salida` los exige.
- **Residuales:** P1/P2 comparan horas vs bloques; `metadata.total_asignaciones` cuenta variables;
  `profesores_asignados`/`tiempo_ejecucion_ms` sin rellenar; `Solver` acumula estado si se reutiliza;
  no se validan materias duplicadas ni de 0 horas en un plan.
- **Menor:** `.clangd` sin ignorar en `.gitignore`.
