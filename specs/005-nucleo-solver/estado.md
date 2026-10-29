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

### Resueltos (2026-10-29)

- **Decisión I2 — avisos retirados** (`36cd7b2`; docs en `ebeb3bf`): `ValidadorSalidaSolver` ya no emite
  **P1** (horas no cubiertas) ni **P3** (capacidad de aula). **P2** (exceso de horas del docente) es el
  único aviso y **P4** sigue como conflicto no presentable. `test_validador_salida` alineado.
- **`metadata` de la salida con datos reales** (`d356780`): `total_asignaciones` cuenta **clases** (no
  variables), `profesores_asignados` cuenta docentes distintos con al menos una clase y
  `tiempo_ejecucion_ms` sale del cronómetro de la resolución.
- **`Solver` acumulaba estado** (`57f5be8`): `resolver` reinicia `model`/`vars` al inicio; reutilizar la
  instancia equivale a instanciar de nuevo.
- **Retiradas por desfasadas:** `.clangd` **ya está** en `.gitignore`; y «P1/P2 comparan horas vs
  bloques» **dejó de ser un problema** con la 011 (1 hora académica = 1 bloque, sin conversión).
- **Flaky del middleware mal clasificado:** `test_middleware_transport` **no era ajeno al solver**; era
  una colisión del **nombre fijo del socket IPC** entre los tests que levantan `InternalServer`
  (`health_check`, `transport`, `ipc_aislamiento_bd`) al correr en paralelo. Corregido el 2026-10-29
  con un `RESOURCE_LOCK` de CTest (`fd7ac33`): antes 5/5 en rojo con `-j4`, ahora 5/5 en verde con
  `-j4` y `-j8`.

### Sigue abierto

- **RF-13:** `ConstructorEntradaSolver` no embebe los planes de los **docentes** (solo los de los cursos).
- **RNF-4:** faltan casos rojo→verde de V7, V8, V14, V15, D13, S1/S2, aula de salida como índice y
  `peso` en el ida y vuelta. (V2 ya cubre `peso` negativo y el rango de `duracion_minutos` lo cubre la
  011; la reproducibilidad tiene un caso en la 010, pero no cubre el ida y vuelta ni el resto.)
- **Validación de materias:** no se validan materias duplicadas ni de 0 horas en un plan.
