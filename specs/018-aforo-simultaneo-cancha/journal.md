# Journal 018 - Aforo simultáneo de la cancha

Base: `dfe529f3392648fc6ad46de16bbeec26c51d11f6` (`docs(memory): cierre de la 017`).
Ejecución del implementer: **2026-11-10**. Tareas **T1–T5**; **T5 queda bloqueada** (ver su
sección y la conclusión). T6/T7/T8 no se ejecutan en esta llamada.

Estado de las suites al cerrar esta llamada:

| Suite | Comando | Resultado |
|---|---|---|
| backend | `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend` | **62/62 verde** (17,22 s) |
| full-release | `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release` | **67/67 verde** (11,99 s; el lento no se ejecuta) |
| slow-release | `ctest --preset slow-release` | **1/2**: `test_solver_centro_sin_ef` **verde** (179,87 s) · `test_solver_centro_real` **ROJO** (25,30 s, `INFEASIBLE`) |

> **Actualización (T4b/T5, misma fecha, ver secciones finales).** Con el docente de apoyo **P033**
> (T4b), el centro real **genera**: `test_solver_centro_real` **VERDE** con **876 clases**, EF
> 96/96 en la cancha, desvío 0, huecos al final, 0 pares cruzados y **231 939 ms**. Las tres suites
> quedan verdes (`backend` 62/62, `full-release` 67/67, `slow-release` 2/2). El bloque de T5 de más
> abajo se conserva como evidencia histórica del bloqueo.

---

## T1 - Aforo en un solo punto, capacidad del tipo y causa (2)

RF-1, RF-5, RF-6, RNF-3, RNF-8.

- **Nuevo** `src/common/include/common/aforo_aula.hpp`: `tipoCancha()` («Cancha»),
  `claveCancha()` (derivada con `tipo_general::claveNormalizada`) y `aforoDeClave(clave)`
  (cancha → 2, resto → 1). Único literal y único 2; sin `aforo(nombreTipo)`.
- `contrato_solver.{hpp,cpp}`: `aforoDelTipo(clave)`, `aforoDelAula(config, índice)`
  (índice inválido → 1) y `posicionesDePar(turno, eventos)` (Σ ⌊|segmento|/2⌋). Capacidad =
  `aulas × franjas × aforo`.
- `solver.cpp`: causa (2) → «… sus %3 aulas ofrecen %4 plazas (%5 franjas × aforo %6)»; mismo
  orden y precedencia. Las aserciones vigentes («10 bloques», «8 franjas», ««Laboratorio»») se
  conservan: `test_solver_horarios` verde sin tocarlo.
- `test_capacidad_tipo.cpp`: **`configConCancha()` aparte** (no se toca `configBase()`), con 5
  casos nuevos: aforo del tipo (cancha 2 / resto 1 / clave con caja), `aforoDelAula` con índice
  inválido, capacidad `1 × 8 × 2 = 16`, `posicionesDePar` = 4 por día (segmentos 1+3) y
  `5 × 4 = 20` por turno, y `toJson` **sin** la clave `aforo` (raíz, aula y JSON serializado).
- Evidencia: `test_capacidad_tipo` **12/12**; suite `backend` **62/62** verde.

## T2 - Modelo: hasta el aforo por aula y pares completos

RF-1…RF-5, RNF-2.

- `restriccionAulaNoSolapada`: mismo agrupamiento por (día, franja, aula) con **todas** las `y`;
  `aforo <= 1` → `AddAtMostOne` (idéntico al comportamiento previo); `aforo > 1` →
  `AddLessOrEqual(LinearExpr::Sum(grupo), aforoDelAula(config, aula))`.
- **Nuevo** `test/backend/test_solver_aforo.cpp` (registrado como `backend_solver`). Construye el
  modelo con `crearVariables` + `agregarRestriccionesTurnos/Aulas/Profesores/Bloques` y un
  `CpSolver` con 1 worker y seed 0; fuerza `x` con `AddEquality`. **5/5 casos verdes**:
  - (a) dos cursos de EF forzados en 0–1 (misma posición de par) → **FACTIBLE**, dos clases en la
    cancha;
  - (b) tres cursos forzados en la franja 0 → **INFEASIBLE** (aforo 2);
  - (c) un bloque forzado en **3–4** de un segmento que empieza en 0 → **INFEASIBLE** (R9 exige
    prefijo; no hizo falta añadir la restricción explícita por posición de par de plan §6/E3);
  - (d) control positivo del mismo bloque en **0–1** → **FACTIBLE**;
  - (e) un solo docente candidato para dos cursos, 2 días: sin forzar, las dos clases **no**
    coinciden en ninguna (día, franja); forzarlas en el mismo par → **INFEASIBLE** (la variante
    de 1 día no cabe: R9 obliga a usar la única posición y el docente es el mismo; se documenta
    la variante elegida: 2 días).
- **Demostración del par completo** (plan §4): R9 ⇒ franjas ocupadas = prefijo; R2 ⇒ nº par de
  franjas y pares que empiezan en 0, 2, 4… del segmento; R7 ⇒ las dos franjas del par comparten
  aula. Luego un curso en la cancha ocupa el par entero y dos cursos solo coinciden en la misma
  posición de par. (c) lo prueba sobre el modelo: un par no alineado no tiene solución.
- Evidencia: `test_solver_aforo` **5/5**; `test_solver_bloques` y `test_solver_centro_sin_ef` sin
  cambios; suite `backend` **62/62** verde.

## T3 - Comprobación de la salida con aforo y pares cruzados

RF-5, RF-7, RF-10, RNF-4.

- `ValidadorSalidaSolver`: la ocupación de aula pasa de mapa «clave → curso» a **conteo** por
  (aula, día, franja); se compara con `aforoDelAula` (**1** para índice fuera de rango o tipo no
  reconocido); las clases con aula negativa **no cuentan**; mensaje «El aula «%1» tiene %2 clases
  en la misma franja (día %3, slot %4) y su aforo es %5.».
- Nuevo `comprobarParesCancha`: segmentos del turno del curso, pares alineados (k += 2); marca la
  franja de cancha sin su pareja («… ocupa la cancha en una sola franja del bloque %2–%3 (día
  %4).»). Con aforo 1 en todas las aulas no añade nada.
- Sin categorías nuevas: curso y docente se siguen marcando.
- `test_validador_salida.cpp`: **21 funciones verdes**, con los 6 casos nuevos: dos en la cancha
  en el mismo par (sin conflicto), tres en la cancha (conflicto con 3 clases y aforo 2), dos en
  un aula de aforo 1 (conflicto con aforo 1), aula inexistente (aforo 1, «(desconocida)»), clase
  sin aula fuera de la cuenta (cancha con 2 + una sin aula → presentable) y par cruzado
  (conflicto). Los casos previos (incluidos «aula «A1»» y «aula «(desconocida)»») siguen verdes.
- Evidencia: suite `backend` **62/62** verde.

## T4 - Seed: guarda del tipo, cuenta con aforo 2 e informe generable

RF-2, RF-8, RNF-4.

- `perfil.hpp`: `CapacidadCancha` y `capacidadCancha(perfil)` (pura). `tipoFinalDeMateria`
  documentada por **clave** y devuelve el **nombre declarado**; `motivoNoGenerable` con el aforo
  2.
- `perfil.cpp`: fuera `kTipoCancha` (usa `aforo_aula`); tabla de aulas con bandera `cancha` para
  no repetir el literal; clasificación de aulas y guarda **por clave**:
  - aula «Cancha» ausente → «El perfil debe declarar el aula «Cancha».»;
  - tipo de otra clave → «La cancha «Cancha» declara el tipo «Campo» (clave «campo»); la clave
    esperada es «cancha»: el tipo de la cancha debe llamarse «Cancha». Corrija el tipo del aula o
    del catálogo.»;
  - «CANCHA» (misma clave) → **válido** y las materias de EF apuntan al tipo declarado «CANCHA».
  - `capacidadCancha` construye `TurnoConfig`/`EventoConfig` desde `TurnoDef/EventoDef` y delega
    en `posicionesDePar`: **1 aula, aforo 2, 40 franjas/turno, 80 plazas/turno, 160/semana, 20
    posiciones, 40 plazas de par, 24 pares de EF/turno, 48 clases EF/turno, 96/semana**.
- `sembrador.cpp`: tres líneas nuevas del informe («Cancha: 1 aula · 40 franjas por turno · aforo
  2 → 80 plazas por turno · 160 en la semana», «Educación Física: 48 clases por turno · 96 en la
  semana → 24 pares por turno · 48 en la semana», «Pares de cancha: 20 posiciones por turno · 40
  plazas de par → 24 ≤ 40 por turno») y «Generable: sí». `main.cpp`: ayuda sin «aforo pendiente».
- Tests actualizados en el mismo paso: `test_seed_perfil` (26/26; `motivoNoGenerable` vacío,
  `capacidadCancha`, guarda por clave), `test_seed_aplicacion` (23/23; informe generable + tras
  aplicar) y `test_entrada_seed` (informe generable y **ida y vuelta del preset**, RNF-8).
- Evidencia: suite `backend` **62/62** verde y `full-release` **67/67** verde.

## T5 - Aceptación lenta del centro real: **BLOQUEADA**

RF-9, RNF-1, RNF-5, RNF-7.

Implementado `test/frontend/test_solver_centro_real.cpp` (Qt Test, etiqueta `slow`,
`QTEST_FUNCTION_TIMEOUT=900000` para que el vigilante de Qt no corte antes del techo de 300 s):siembra el centro en `QTemporaryDir`, abre `NucleoDatos`, compone la entrada con
`GeneracionSolver::construirEntrada` y los 24 cursos del perfil, valida V1–V19 y resuelve. Aserta
876 clases, desvío 0, `bloquesDeUno == 0`, `huecosNoFinal == 0`, `presentable()`, EF (96) en la
cancha, ninguna aula por encima de su aforo, ningún par cruzado (recálculo propio) y `< 300000` ms
en Release. `test_solver_centro_sin_ef` **no se toca**.

Nota de implementación: el test va con Qt Test (`add_qtest`), como el plan §7; un binario con
`gtest_main` **no crea `QCoreApplication`** y `QSqlDatabase::open()` segfaulta al sembrar. Se fija
`QTEST_FUNCTION_TIMEOUT=900000` en `test/CMakeLists.txt` porque el vigilante por defecto de Qt
(5 min) cortaría justo en el techo de 300 s del criterio.

**Resultado: `INFEASIBLE` en ~25 s.** El fallo **no es del aforo** y es previo a esta spec:

```
FAIL!  : ... 'resultado.exito' returned FALSE. (El modelo no tiene solución factible (INFEASIBLE).
Causa: no atribuible. ... Entorno: 32 docentes, 24 cursos, 85 materias, 13 aulas.)
```

Diagnóstico (medido sobre la entrada compuesta desde la base, no sobre una copia a mano):

1. **Sin EF** (horas de EF a 0) e **incluso con EF de 2 h**: sigue `INFEASIBLE`. El bloqueo no es
   la cancha.
2. **Solo Mañana** (12 cursos): **FACTIBLE**. **Solo Tarde** (12 cursos): **INFEASIBLE**. Cada plan
   por separado en la tarde (`31059` 3 cursos; `31060` 9 cursos): **FACTIBLE**. El fallo aparece al
   juntar los dos planes en la tarde.
3. **Carga exclusiva de docentes** (docentes que son el único candidato elegible de una
   materia-curso, contando bloques de todos los cursos de la tarde): **Dolores Ñáñez (P027) = 48 h
   exclusivas** (`Idiomas 1º/2º/3º/4º` = 36 h + `Inglés y otras lenguas extranjeras 2º` = 12 h). Un
   docente solo tiene **40 franjas semanales** (5 días × 8): **48 > 40 ⇒ infactible**. Los demás
   docentes con carga exclusiva están por debajo del tope físico (Formación 14 h, Química 26 h,
   Biología 36 h, Castellano 12 h, Arte 8/12 h, Ciencias Naturales 18 h, Orientación 6 h).
4. **Contraprueba**: añadiendo un docente de apoyo con las mismas materias que P027, la tarde pasa
   a **FACTIBLE**. (El centro completo con ese apoyo no llegó a resolverse dentro del vigilante de
   5 min de Qt antes de fijar `QTEST_FUNCTION_TIMEOUT`; no se usa como evidencia de tiempo.)

> **Nota del planner (2026-11-10).** La posición con Idiomas + Inglés es **P028** (la 28.ª,
> «Dolores Ñáñez»); **P027** es Castellano + Lengua y Literatura. La corrección de datos de la 017 va
> sobre **P028** (Idiomas → **P033**; P028 se queda con Inglés).

Conclusión: **el conjunto del centro de la 017 no es generable con su plantilla de 32 docentes**,
con independencia del aforo: la única docente de idiomas (P027) acumula 48 clases semanales y no
cabe en 40 franjas. Esto choca con RF-9/RF-8 de la 018 y con RNF-5 («datos intactos»), así que
**no se arregla dentro de esta spec**: o se corrige la plantilla (dato del centro: un docente más
de idiomas y, previsiblemente, revisar cargas; cambio de datos con su propia spec) o se revisa el
criterio de aceptación. Es el riesgo 5 del plan §12 («si T5 falla, el informe es incorrecto y la
spec no se cierra; se informa y se decide»). **T5 no se marca como hecha** y `tasks.md` queda con
la nota.

Suites al cerrar: `backend` **62/62** y `full-release` **67/67** verdes; `slow-release` **1/2**
(`test_solver_centro_sin_ef` 179,87 s verde; `test_solver_centro_real` rojo por `INFEASIBLE`).

---

## T4b - Docente de apoyo P033 (corrección de datos de la 017) y tests del seed

RF-8, RNF-5. Decisión del usuario (2026-11-10): la tarde no cabe con una sola docente de idiomas;
se añade **P033** y **P028** se queda con Inglés.

- `src/seed/src/docentes.cpp`: **P028** pasa a `{Tarde, 1, {"Inglés y otras lenguas extranjeras"}}`
  (5 asignaciones; 12 clases de 31059 2º) y se añade al final de la tabla **P033**
  `{Tarde, 1, {"Idiomas"}}` (5 asignaciones; 36 clases de 31060). `kNumDocentes = 33`,
  `kAsignacionesEsperadas = 272` (sin cambios: las 5 filas de Idiomas se mueven, no se crean) y
  `validarDocentes` exige **16 mañana / 17 tarde** (`kDocentesManana`/`kDocentesTarde`), con el
  mensaje de turno actualizado. No se tocan horas (`36`) ni el resto de la plantilla.
- `sembrador.cpp`: la línea de capacidad docente pasa a **`Capacidad docente: 1188 (576 + 612)`**
  (16 × 36 + 17 × 36); el resto del informe sigue con `Generable: sí`. `main.cpp`: ayuda con
  **33 docentes**. Docs de `perfil.hpp`/`docentes.hpp`: 33 docentes y 165 disponibilidades.
- Tests actualizados en el mismo paso:
  - `test_seed_perfil` (26/26): `recuentosEsperados` 33/272/165; plantilla con **P033** y **P028**
    separados (Inglés ↔ P028, Idiomas ↔ P033, 5+5), 16 mañana / 17 tarde, 147 asignaciones en la
    mañana y 125 en la tarde (sin cambios), `P033` con 5 filas de disponibilidad; la validación
    cubre el recuento (33) y el reparto por turno (mover P032 a la mañana deja 17/16 y se rechaza).
  - `test_seed_aplicacion` (23/23): recuentos 33 docentes / 165 disponibilidades, filas
    `Profesores = 33`, `Disponibilidad_Profesor = 165`, `Profesor_Materia = 272`; el informe
    comprueba `Capacidad docente: 1188 (576 + 612)`.
  - `test_entrada_seed` (verde): compone con **33 docentes**, el preset conserva 24 cursos / 33
    docentes / 40 min (ida y vuelta, RNF-8) y el informe sigue siendo **`Generable: sí`**.
- Salida real del sembrador:
  `Cursos: 24 (12 Mañana · 12 Tarde)   Docentes: 33   Materias asignadas: 272   Disponibilidades: 165   Tipos de aula: 2`,
  `Capacidad docente: 1188 (576 + 612)   Demanda: 414 Mañana · 462 Tarde`, `Generable: sí`.
  Recuentos 2/85/95/24/13/40/889 y 876 clases sin cambios.
- Suites: `backend` **62/62** verde (101,04 s en esta máquina, carga alta) y `full-release`
  **67/67** verde (19,76 s).

## T5 - Aceptación lenta del centro real (DESBLOQUEADA Y VERDE)

RF-9, RNF-1, RNF-5, RNF-7. Tras T4b, `test_solver_centro_real` (ya implementado) siembra el
centro, compone la entrada desde la base, valida V1–V19 y resuelve.

- Ejecución verbosa (`ctest --preset slow-release -R test_solver_centro_real -V`):

  ```
  [   INFO   ] centro real: 876 clases, EF 96 en la cancha, desvío 0, bloques de uno 0,
                huecos no finales 0, separación 6, pares cruzados 0, tiempo 231939 ms
  Totals: 3 passed, 0 failed, 0 skipped, 0 blacklisted, 232013ms
  ```

  **Medición: 231 939 ms (≈ 232 s) < 300 000 ms** (RNF-1) y la separación blanda informada es 6.
- Todos los asserts verdes: 876 clases; `analizarBloques` con desvío 0, `bloquesDeUno == 0` y
  `huecosNoFinal == 0`; `analizarSalidaSolver(...).presentable()`; **EF 96/96 en el aula de clave
  cancha**; ninguna aula por encima de su aforo (recálculo propio); **0 pares cruzados**
  (recálculo propio).
- Suite lenta completa: `ctest --preset slow-release` → **2/2 verde**:
  `test_solver_centro_sin_ef` **209,16 s** (la aceptación sin EF de la 014 se mantiene) y
  `test_solver_centro_real` **263,67 s** de pared (siembra + composición + resolución + asserts;
  el proceso de cálculo es 231,94 s). Total 472,83 s.

---

## T6 - Contrato, guías e interfaz (documentación)

RF-11. Sin tocar `src/`, `specs/` ni la sección del paquete demo.

- **`docs/guias/datos-prueba.md`**: cabecera con **33 docentes** y **«Generable: sí»** (aforo 2,
  48 ≤ 80 y 96 ≤ 160); recuentos **33 docentes / 165 disponibilidades** y fila del **aforo de la
  cancha (2)**; salida de ejemplo real (Docentes 33, Disponibilidades 165,
  `Capacidad docente: 1188 (576 + 612)`, las tres líneas de cancha/EF/pares y `Generable: sí`);
  §4 con el apoyo **P033** (Idiomas) y **P028** (Inglés), **16 mañana / 17 tarde** y la nota de que
  el exceso de idiomas ya no existe; §6 reescrita a **generable** con **80/160 plazas**, **24 ≤ 40
  pares**, la aceptación lenta (**876 clases, 232 s**) y la distinción capacidad física (88) vs aforo
  (2); comando de verificación esperado con 33/165/`Generable: sí`.
- **`docs/guias/datos-prueba-frontend.md`**: tabla de recuentos **33 docentes / 165 disponibilidades**
  (16/17); §5 pasa a **«un conjunto que genera»** (aforo 2, 48 ≤ 80, 96 ≤ 160, 24 ≤ 40, aceptación
  lenta 232 s), sin `NoFactible` «en la vista» como deuda del aforo y con **la vista de horario y su
  estimación de ocupación fuera del alcance de la 018**; checklist y tabla de cambios con las filas
  de la 018 (aforo, P033 y aceptación).
- **`docs/contrato-solver-2.0.md`**: nota «el aforo no viaja en el contrato» (regla del motor:
  cancha 2, resto 1; sin campo en JSON/preset/huella); §5.1 con
  **`capacidad(t) = aulas × franjas × aforo(t)`**, el mensaje de la causa (2) con las **cuatro cifras**
  (demanda, aulas, franjas y aforo) y la nota de que la capacidad es necesaria y no prueba; §6 con
  P4 por **conteo** contra el aforo (sin aula = no cuenta; índice inválido = aforo 1) y los **pares
  cruzados** como conflicto.
- **`docs/interfaz-frontend.md`**: nota del **aforo como regla del motor** (sin campo en el DTO, el
  preset ni la huella), causa (2) con el mensaje nuevo, **vista de horario y estimación de ocupación
  fuera del alcance de la 018** (no es deuda); §3.6 actualizada al conjunto generable y al test lento;
  actualización fechada de la 018 (33/165, generable, 232 s) y el «no se declara generable» de la 017
  reescrito como estado histórico.
- Verificación (`grep`): la prohibición de T6 no devuelve nada
  (`no generable|no es generable|aforo pendiente|aforo simultáneo|aforo 1|Generable: no` en las dos
  guías y la interfaz); en las guías, toda línea con `aforo|generable` contiene **aforo 2** o
  **Generable: sí**; `grep -n "aulas × franjas × aforo" docs/contrato-solver-2.0.md` devuelve la
  fórmula (línea 385).

---

## T8 - Intento de cierre: **NO cerrada** (2026-11-10, fase 4)

RNF-2, RNF-6. Dos bloqueos, ninguno imputable a una tarea anterior mal hecha. **T8 queda sin marcar en
`tasks.md`** y `estado.md` no se toca.

### Bloqueo 1 - la enmienda de la constitución no se puede aplicar

- `docs/constitution.md` **no tiene** ninguna ocurrencia de `aforo` (`grep -n "aforo" docs/constitution.md`
  → rc=1) y **no** aparece el bloque «Enmienda (2026-11-10, spec 018)»; solo están las enmiendas del
  **2026-10-17** y del **2026-11-07** (líneas 49 y 69). `git diff -- docs/constitution.md` está vacío.
- La edición está **denegada por el perfil del agente**, no por el fichero:
  `~/.config/opencode/agents/implementer.md` incluye `action: edit` + `resource: "docs/constitution.md"`
  y `"**/docs/constitution.md"` → `effect: deny` (y `"*"` → `allow`). El intento de `edit` devuelve
  `permission.rejected`. La autorización expresa del coordinator **no** puede saltarse esa regla, y no se
  ha usado `shell` para eludirla. `docs/` en general **sí** es editable (sonda sobre
  `docs/contrato-solver-2.0.md` sin error de permiso): el `deny` es específico de `constitution.md`.
- Ningún otro perfil puede escribirla: `coordinator` y `reviewer` tienen `edit "*" → deny`; `planner`
  solo `specs/**`. **La enmienda sigue pendiente** y el «Hecho cuando» de T8 («se confirma que
  `docs/constitution.md` … están puestas») **no se cumple**.
- Las **cuatro notas de supersesión** sí están: `grep -ln "018"` devuelve
  `specs/005-nucleo-solver/spec.md`, `specs/010-solver-tipos-y-eventos/spec.md`,
  `specs/014-bloques-de-dos-horas/spec.md` y `specs/017-datos-reales-del-centro/spec.md`.

### Bloqueo 2 - la suite lenta no queda verde en dos intentos (techos de tiempo)

| Suite | Comando | Resultado (intento 1) | Resultado (intento 2) |
|---|---|---|---|
| backend | `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend` | **62/62 verde** (27,76 s) | — |
| full-release | `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release` | **67/67 verde** (19,16 s) | — |
| slow-release | `ctest --preset slow-release` | **1/2**: `test_solver_centro_sin_ef` **ROJO** (238,88 s; `tiempo_ms` **220 312** > techo **200 000**) · `test_solver_centro_real` **verde** (165,98 s) | **1/2**: `test_solver_centro_sin_ef` **verde** (216,93 s) · `test_solver_centro_real` **ROJO** (369,95 s; `tiempo_ms` **369 707 ≥ 300 000**) |

- En los dos intentos **todos los asserts funcionales pasan**: centro sin EF 848 clases, desvío 0,
  420 simples, 4 compartidos, separación 2 ≤ referencia 95, huecos no finales 0; y el centro real
  genera (el intento 1 lo dio verde completo). **Lo único que falla son los techos de tiempo**
  (200 000 ms en `test_solver_centro_sin_ef:106` de la 014; 300 000 ms en
  `test_solver_centro_real:227`).
- La causa es **varianza del host/CP-SAT** (8 workers sobre 4 núcleos): en la misma máquina y sin
  cambios de código, el centro real midió **165 980 ms** en un intento y **369 707 ms** en el siguiente
  (×2,2), y el sin EF **220 312** / (verde) **~217 s**. No hay proceso residual ni carga alta
  (`uptime` 1,7; sin throttling térmico: 40 °C). No se tocó ni el fixture sin EF ni sus techos: son de
  la 014 y la 018 la mantiene como comparación.
- Conclusión: **la suite lenta no se puede declarar 2/2 verde en esta máquina hoy**; es un problema de
  **estabilidad de tiempo**, no del aforo (que no cambia el modelo con aforo 1: `AddAtMostOne`). Queda
  como decisión pendiente (subir techos, marcar el test con tolerancia, o medir en un host estable).

### Alcance del diff (sin commits)

`git status --short -- src/ test/ docs/`: solo los **24** archivos del plan §2 (21 modificados + 3 nuevos). `docs/constitution.md`
**no** aparece. Sin `src/backend/src/database/` y sin `src/frontend/src/`. Nuevos sin seguimiento:
`src/common/include/common/aforo_aula.hpp`, `test/backend/test_solver_aforo.cpp` y
`test/frontend/test_solver_centro_real.cpp`. Fuera de `src/ test/ docs/` solo `specs/` (005, 010, 014,
017 con sus notas de supersesión y `specs/018-…/` sin seguimiento).

### Repetición de la fase 4 (2026-11-10, tercer intento): mismos dos bloqueos

Reintento con la autorización expresa del coordinator para editar `docs/constitution.md`. **El resultado
no cambia**; se añaden mediciones.

- **Enmienda:** `edit` y `write` sobre `docs/constitution.md` → `permission.rejected` (dos veces, con la
  autorización en el mismo mensaje). La causa está en `~/.config/opencode/agents/implementer.md:21-26`:
  `action: edit` + `resource: "**/docs/constitution.md"` / `"docs/constitution.md"` → `effect: deny`
  (tras `"*"` → `allow`). Una autorización del coordinator en el prompt **no** reescribe ese perfil y no
  se ha usado `shell` para eludirlo. `grep -n "aforo" docs/constitution.md` → **sin salida** (rc=1).
- **Suites:** `backend` **62/62 verde** (54,67 s) y `full-release` **67/67 verde** (28,03 s).
  `ctest --preset slow-release` → **1/2**: `test_solver_centro_real` **verde (244,12 s)**;
  `test_solver_centro_sin_ef` **ROJO**, por su techo de **200 000 ms** (014, `:106`):
  `tiempo_ms` **321 338** (1.ª vuelta de la suite), y en dos repeticiones aisladas **200 641** y
  **210 285**. Todos los asserts funcionales del sin EF pasan (848 clases, desvío 0, 420 simples,
  4 compartidos, separación 2, huecos no finales 0).
- **Carga del host durante las mediciones:** `load average` **1,73 → 6,82 → 8,96 → 1,81** (4 núcleos,
  CP-SAT con 8 workers). Es el mismo patrón de varianza ya registrado arriba, no una regresión del aforo:
  el fixture sin EF no cambia de modelo (aforo 1 → `AddAtMostOne`) y su resultado funcional es idéntico.
- **Alcance:** `git status --short -- src/ test/ docs/` → **24** entradas, las mismas de arriba (21 `M` +
  3 `??`); sin `src/backend/src/database/` y sin `src/frontend/src/`. `src/seed/include/seed/docentes.hpp`
  entra por la documentación de **P033** de T4b.
- **Conclusión:** **T8 sigue sin marcar**; `estado.md` no se toca porque el «Hecho cuando» no se cumple
  (falta el parche de la constitución y falta una corrida lenta estable). El parche de la enmienda está
  redactado en T7 y en este journal; lo tiene que aplicar un perfil con escritura en `docs/`.

---

## T8 - Cierre: **HECHA** (2026-11-10, fase 4, cuarto intento)

RNF-2, RNF-6. Los dos bloqueos del intento anterior quedan resueltos: la **enmienda de la constitución** la
aplicó el coordinator antes de esta vuelta y el **techo de 200 s del test de la 014** se subió a **300 s**
(decisión del usuario; ver la nota añadida en `specs/014-bloques-de-dos-horas/journal.md`).

### Constitución y supersesiones (verificación de T8, no edición)

- `grep -n "aforo" docs/constitution.md` → **10** líneas, con las **cuatro zonas** enmendadas:
  bloque «Enmienda (2026-11-10, spec 018)» (§7), **§7.1.3** (punto 3: «hasta su aforo simultáneo… cancha → 2»),
  **§7.1.5** (punto 5: exclusividad con el aforo del tipo), **§7.3** (aulas: «el aforo … es una regla del
  motor por tipo de aula … no viaja en la configuración») y **§7 «Comportamiento»** (causa (2):
  `aulas × franjas × aforo` y las cuatro cifras). **§7.4 intacto** (V1–V19).
- `grep -ln "018"` → `specs/005-nucleo-solver/spec.md`, `specs/010-solver-tipos-y-eventos/spec.md`,
  `specs/014-bloques-de-dos-horas/spec.md` y `specs/017-datos-reales-del-centro/spec.md`: las **cuatro**
  notas de supersesión puestas (T7).

### Suites (las tres, verdes)

| Suite | Comando | Resultado |
|---|---|---|
| backend | `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend` | **62/62 verde** (19,16 s) |
| full-release | `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release` | **67/67 verde** (13,79 s) |
| slow-release | `ctest --preset slow-release` | **2/2 verde** (335,48 s) |

- `test_solver_centro_sin_ef` (014): **Pasa**, **173,80 s**; `tiempo_ms` **157 884** < techo **300 000**
  (antes 200 000). Asserts: **848 clases**, desvío **0**, 420 simples, 4 compartidos, separación **2**
  (referencia 83), huecos no finales **0**.
- `test_solver_centro_real` (018): **Pasa**, **161,67 s**; `tiempo_ms` **161 463** < 300 000. Asserts:
  **876 clases**, **EF 96/96** en la cancha, desvío **0**, `bloquesDeUno` **0**, `huecosNoFinal` **0**,
  separación 6, **0 pares cruzados**, `analizarSalidaSolver(...).presentable()`.
- Carga del host durante la corrida: `load average` **2,22 → 7,73** (8 workers de CP-SAT sobre 4 núcleos).
  Mediciones del sin EF acumuladas entre la 014 y hoy: **149,44 / 157,88 / 173,80 / 200,64 / 210,29 /
  220,31 / 238,88 / 321,34 s** — de ahí el techo de 300 s (criterio de la spec) en vez de 200.
- La cifra de referencia de T5 para el centro real fue **231 939 ms**; esta corrida midió **161 463 ms**.
  Es varianza del host, no un cambio del modelo: no se ha tocado código de resolución desde T5.

### Línea base medida (RNF-2/RNF-6)

`876` clases · desvío `0` · bloques de uno `0` · huecos no finales `0` · pares cruzados `0` · EF `96/96` en
la cancha · centro real **161,46 s** (rango observado 161–232 s) · sin EF **149–321 s** (típico 157–174 s,
techo del test 300 s) · suites **62/62**, **67/67**, **2/2**.

### Alcance del diff (sin commits)

`git status --short -- src/ test/ docs/` → **26** entradas (23 `M` + 3 `??`): los **24** archivos del plan §2
(ahora con `docs/constitution.md` modificado por el coordinator) **más** dos desviaciones declaradas:

1. `src/seed/include/seed/docentes.hpp` — documentación de **P033** (T4b, ya declarada en T4b/T8).
2. `test/backend/test_solver_centro_sin_ef.cpp` — **subida del techo 200 000 → 300 000 ms** en esta tarea
   (cambio de margen del test de la 014, **no** de reglas del motor; autorizado por el usuario para cerrar
   T8). No toca `src/`.

Sin cambios en `src/backend/src/database/` ni en `src/frontend/src/` (`git status` de ambas rutas → **0**
entradas). `specs/018-…/` sigue sin seguimiento y fuera del control de alcance. Sin commits.
