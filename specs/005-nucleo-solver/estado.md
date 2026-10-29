# Estado 005

Fase: cerrada
Aprobaciones: spec 2026-10-17 (rectificada) · plan/tareas 2026-10-17 · implementación 2026-10-17 · **T10 2026-10-29**
Base: f94acba455a04ae80364c82e1f07ae16d9f156ec
Dudas abiertas: ninguna
Veredicto: APROBADO (2026-10-17); **T10 cerrada el 2026-10-29**
Siguiente paso: nada pendiente en esta spec. Abrir el PR de la rama a `develop` cuando el usuario lo pida.

## Implementado

- **T11 (rectificación 2026-10-17):** hora académica = bloque. `bloques(horas, duracion) = horas` y
  `bloquesAula = horas_aula` (guarda `0` si `duracion_minutos ≤ 0`, defensiva); `duracion_minutos` solo
  define la duración real de la franja. V14 sobre `Σ_m horas_m`; D3 y S1 sin conversión. Tests de
  contrato, validador y solver actualizados; `docs/constitution.md` §7 enmendada.
- Suite **46/46** en verde en el contenedor (`ctest --preset full`), incluidos los 2 tests del seed.

## Pendientes (T10, cerrada el 2026-10-29)

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

### Cierre de T10 (2026-10-29)

Suite **54/54** (`ctest --preset backend`, en serie y `-j4`). El **`--preset full`** incluye la interfaz
(fuera de alcance desde la 001; builder roto por la 011, deuda del equipo del frontend) y se exigirá
cuando esa deuda se cierre: es ajena a la 005.

| Tarea | Commit | Qué dejó |
|---|---|---|
| **RF-13** — planes de los docentes | `7e3ee0b` | `ConstructorEntradaSolver` embebe también los planes que usan los **docentes** (además de los de los cursos), sin duplicar y en orden determinista (cursos primero, docentes después); vacío o inexistente se ignora. Tests: plan solo de docentes, coincidente sin duplicar, vacío/inexistente ignorado. |
| **RNF-4** — reglas V7/V8/V15/D13 | `70ea509` | Casos nuevos `v7_turnoAusente_falla`, `v8_modoMovilSinAulaDeCurso_pasa`, `v15_movilSinAulasEnElCentro_falla` y `v13_d13_docenteDeOtroAnioNoEsElegible_falla`. **Ninguno destapó un fallo de producción**: V7, V8, V15 y D13 ya cumplían. |
| **RNF-4** — S1/S2 y contrato | `7354c7a` | `solver_excesoDeHorasEsPenalizadoYNoInfactible` (S1), `solver_titularAlTope_usaAlSuplente` (S2) y `peso = 7` en el ida y vuelta (`test_contrato_solver`). |

- **Cobertura de RNF-4:** cubiertos V7, V8, V15, D13, S1, S2 y `peso` en el ida y vuelta. V2 (`peso`
  negativo) y el rango de `duracion_minutos` ya estaban cubiertos (este último por la 011), y el aula de
  salida como índice ya se aseveraba en ambos modos, así que no se duplicó.
- **T10:** ya no tiene pendientes.
- **Fuera de la spec:** la validación de **materias duplicadas o de 0 horas** en un plan no está en la
  005 —no la menciona ningún RF—; si se quiere, es un requisito nuevo y se abre en otra spec.
