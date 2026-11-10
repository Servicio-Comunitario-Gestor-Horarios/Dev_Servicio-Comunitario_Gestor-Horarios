# Tareas 018 - Aforo simultáneo de la cancha

Base: `dfe529f3392648fc6ad46de16bbeec26c51d11f6`. Una tarea cada vez, **tests en rojo primero**; ninguna
cierra con build o suite en rojo, y la que cambia un símbolo actualiza en el mismo paso a sus consumidores
y a sus tests. La evidencia va en `journal.md`.

- [x] **T1. Aforo en un solo punto, capacidad del tipo y causa (2).** RF-1, RF-5, RF-6, RNF-3, RNF-8
    - Crear `src/common/include/common/aforo_aula.hpp` (`tipoCancha`, `claveCancha`, `aforoDeClave`,
      sin `aforo(nombreTipo)`) sobre `tipo_general::claveNormalizada`; en `contrato_solver` añadir `aforoDelTipo(clave)`,
      `aforoDelAula(config, índice)` (índice inválido → 1) y `posicionesDePar(turno, eventos)`; multiplicar
      por el aforo en `capacidadDelTipo`; mensaje de la causa (2) con demanda, aulas, franjas y aforo;
      orden y precedencia de causas sin tocar.
    - Los casos nuevos van sobre un **`configConCancha()` aparte**: **no** se toca `configBase()`, cuyas
      **4** aserciones exigen `franjasDelTipo(«Cancha»)` vacía.
    - Hecho cuando: los casos nuevos de `test_capacidad_tipo` (aforo cancha 2 / resto 1, `aforoDelAula`
      con índice inválido, cancha `1 × franjas × 2`, `posicionesDePar` = 4 por día y 20 por turno,
      `SolverConfig::toJson` **sin** la clave `aforo`) pasan y toda la suite sigue verde con
      `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend`
      (incluido `TestSolverHorarios`, cuyas aserciones «10 bloques», «8 franjas» y ««Laboratorio»» se
      mantienen). Si T1 se desborda, el **mensaje de la causa (2)** se puede mover a T2 sin cambiar la spec
      (el resto de T1 no depende de él).

- [x] **T2. Modelo: hasta el aforo por aula y pares completos.** RF-1, RF-2, RF-3, RF-4, RF-5, RNF-2
    - `restriccionAulaNoSolapada`: agrupación intacta por (día, franja, aula) con **todas** las `y`;
      `aforo == 1` → `AddAtMostOne`; `aforo > 1` → `AddLessOrEqual(Sum(grupo), aforoDelAula(config, a))`.
      Nuevo `test/backend/test_solver_aforo.cpp` (backend_solver) registrado en `test/CMakeLists.txt`.
      **El forzado se hace sobre un modelo construido en el test** (`Solver::model`/`Solver::vars` son
      privados): `crearVariables(modelo, config)` + `agregarRestriccionesTurnos/Aulas/Profesores/Bloques`
      + `CpSolver` con 1 worker y seed 0, como `test_solver_bloques.cpp:44-52`; después
      `AddEquality(x[{p, m, c, d, s}], 1)`. Casos:
      (a) dos cursos de Educación Física forzados en **0 y 1** (misma posición de par) → **factible** con
      dos clases en la cancha; (b) **tres** cursos forzados en la franja 0 → **infactible**;
      (c) un bloque forzado en **3 y 4** de un segmento que empieza en 0 (**par cruzado**) → **infactible**
      (R9); (d) control positivo de (c) en 0–1 → factible; (e) **un solo docente candidato** para dos
      cursos → las dos clases **no** coinciden en la misma franja (o la instancia no cabe).
      Si el forzado resultara frágil, la vía alternativa es **forzar por configuración** (turno de dos
      franjas o disponibilidad restringida) y comprobar el estado; se documenta la elegida.
    - Hecho cuando: `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset
      backend` verde, con los **cinco** casos de `TestSolverAforo` (2 factible, 3 infactible, par cruzado
      infactible, control positivo y un solo docente que no coincide) y sin cambios en `test_solver_bloques`
      ni en `test_solver_centro_sin_ef` (aforo 1). Si el caso del par cruzado no resultara infactible, se
      añade la restricción explícita por posición de par **en esta misma tarea** (plan §6, E3) y se anota
      en `journal.md`.

- [x] **T3. Comprobación de la salida con aforo y pares cruzados.** RF-5, RF-7, RF-10, RNF-4
    - `ValidadorSalidaSolver`: la ocupación de aula pasa de mapa a **conteo** por (aula, día, franja) y se
      compara con `aforoDelAula` (**1** para índice fuera de rango o tipo no reconocido); las clases con
      aula negativa **no cuentan**; mensaje con aula, día, franja, nº de clases y aforo; nuevo
      `comprobarParesCancha` (segmentos del turno del curso, pares alineados) que marca la franja de
      cancha sin su pareja. Sin categorías nuevas: los solapamientos de curso y docente se siguen marcando.
    - Hecho cuando: `test_validador_salida` pasa con los casos nuevos (dos en la cancha sin conflicto, tres
      con conflicto, dos en un aula de aforo 1 con conflicto, aula inexistente con aforo 1, clase sin aula
      fuera de la cuenta, par cruzado con conflicto) y `cmake --preset backend && cmake --build --preset
      backend --parallel && ctest --preset backend` queda verde.

- [x] **T4. Seed: guarda del tipo, cuenta con aforo 2 e informe generable.** RF-2, RF-8, RNF-4
    - `perfil.hpp`: `CapacidadCancha` y `capacidadCancha(perfil)` (aulas, aforo, 40 franjas y 80 plazas por
      turno, 160 en la semana, 20 posiciones y 40 plazas de par, 24 pares y 48 clases de EF por turno;
      `posicionesDePar` sobre los turnos del perfil, con un `TurnoConfig`/`EventoConfig` construido en
      `perfil.cpp` desde `seed::TurnoDef/EventoDef` —no se duplica el ⌊|segmento|/2⌋—). `perfil.cpp`:
      fuera `kTipoCancha` (se usa `aforo_aula::tipoCancha()/claveCancha()`), clasificación de aulas **por
      clave**, **tipo de la cancha resuelto por clave** en `tipoEspecializadoDeMateria`/`tipoFinalDeMateria`
      (devuelve el **nombre declarado**, p. ej. «CANCHA», para que `sembrador.cpp` siga casando
      `idTipoAula`), guarda de la cancha declarada («Cancha») con fallo que incluya **tipo recibido**,
      **clave esperada** y **acción**, y `motivoNoGenerable` con el aforo 2 (⇒ vacío). `sembrador.cpp`: líneas `Cancha: … aforo 2 → 80 plazas
      por turno · 160 en la semana`, `Educación Física: 48 clases por turno · 96 en la semana → 24 pares
      por turno · 48 en la semana`, `Pares de cancha: 20 posiciones por turno · 40 plazas de par → 24 ≤ 40
      por turno` y `Generable: sí`. `main.cpp`: ayuda sin «aforo pendiente» y con el conjunto generable. `sembrador.cpp:580` **no** cambia:
      recibe el nombre declarado del tipo y sigue casando `idTipoAula` por nombre.
    - Actualizar **en esta tarea** los tres tests que afirman «Generable: no» (`test_seed_perfil.cpp:652`,
      `test_seed_aplicacion.cpp:854`, `test/frontend/test_entrada_seed.cpp:181`) y los dos casos del
      informe tras aplicar; añadir los casos de la guarda: tipo de **otra clave** («Campo») → fallo con
      tipo recibido/clave esperada/acción, aula «Cancha» ausente → fallo, y tipo con **nombre visible
      distinto y misma clave** («CANCHA») → **válido**, con las materias de Educación Física apuntando al
      tipo declarado. Confirmar en `test_entrada_seed` el **ida y vuelta del preset** (RNF-8).
    - Hecho cuando: `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset
      backend` **y** `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset
      full-release` verdes, con `motivoNoGenerable` vacío para el centro, las cifras nuevas en el informe,
      el preset del centro validando e **ida y vuelta** (RNF-8) y los recuentos del conjunto intactos
      (85/24/32/13/876, 40 min; **T4b** lo deja en **85/24/33/13/876** con **272** asignaciones y **165**
      disponibilidades).

- [x] **T4b. Docente de apoyo P033 (corrección de datos de la 017) y tests del seed.** RF-8, RNF-5
    - `src/seed/src/docentes.cpp`: añadir la posición **P033** (**tarde**, área **Idiomas**) y dejar **P028**
      con **Inglés** (sin Idiomas); `validarDocentes` pasa a **16 mañana / 17 tarde**. Recuentos:
      **33 docentes**, **165 disponibilidades**, **272 asignaciones** (las 5 filas de Idiomas pasan de P028
      a P033: la suma no cambia) y capacidad **1188** (576 + 612). `MotivoNoGenerable` sigue vacío.
    - Actualizar **en esta tarea** `test_seed_perfil.cpp` (docentes 33, derivación con Idiomas→P033 e
      Inglés→P028, 165 disponibilidades), `test_seed_aplicacion.cpp` (recuentos del informe) y
      `test/frontend/test_entrada_seed.cpp` (compone con 33 docentes; sigue **sin resolver** y con
      `Generable: sí`). El reparto lo fija la 017 (decisión del 2026-11-10).
    - Hecho cuando: `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset
      backend` **y** `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset
      full-release` verdes, y `build-backend/src/seed/gestor-horarios-seed --db /tmp/c18.db` imprime
      `Docentes: 33`, `Materias asignadas: 272`, `Disponibilidades: 165` y `Generable: sí`.

- [x] **T5. Aceptación lenta del centro real.** RF-9, RNF-1, RNF-5, RNF-7 — **ejecutar después de T4b**
    - Nuevo `test/frontend/test_solver_centro_real.cpp`: siembra el centro en un directorio temporal,
      compone la entrada desde la base con los 24 cursos del perfil, valida V1–V19 y **resuelve**; asserts:
      876 clases; `#include "backend/services/analisis_bloques.hpp"` → `analizarBloques` con
      `desvio == 0`, `bloquesDeUno == 0` y `huecosNoFinal == 0`; `#include
      "backend/services/ValidadorSalidaSolver.hpp"` → `analizarSalidaSolver(...).presentable()`; EF (96)
      solo en la cancha; ninguna aula por encima de su aforo; **ningún par cruzado** (recálculo propio); y
      `tiempo_ms < 300000` en Release (imprime la medición). Registro en `test/CMakeLists.txt` dentro de `if(TARGET frontend AND TARGET backend)` +
      `if(TARGET seed AND TARGET backend_solver)` con `set_tests_properties(... LABELS "slow")`. No se toca
      `test_solver_centro_sin_ef.cpp` (la aceptación sin EF de la 014 se mantiene como comparación).
    - Hecho cuando: `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset
      full-release` verde (el lento no se ejecuta) y `ctest --preset slow-release` verde con
      `test_solver_centro_real` y `test_solver_centro_sin_ef`, con la medición anotada en `journal.md`.
      Si el plazo de 300 s no se cumple: **parar e informar** (el rendimiento es otra spec).
    - **BLOQUEADA (2026-11-10, journal §T5) y DESBLOQUEADA por la corrección de la 017.** Test implementado
      y registrado con `LABELS "slow"`, pero el centro sembrado resolvía **`INFEASIBLE`** en ~25 s: la
      docente de idiomas (**P028**) acumulaba **48 clases semanales exclusivas** (Idiomas 36 + Inglés de 2.º
      12) y un docente solo tiene 40 franjas (5 × 8). No era el aforo: sin EF seguía siendo infactible; la
      mañana sola era factible y la tarde sola no. **Resolución (decisión del usuario, 2026-11-10):** la
      **017** añade el docente de apoyo **P033** (Idiomas) y deja **P028** con Inglés → **T4b**. T5 se
      ejecuta **después** de T4b; si no genera, **parar e informar**.
    - **HECHA (2026-11-10, journal §T5):** `ctest --preset slow-release` **2/2 verde**;
      `test_solver_centro_real` con **876 clases**, EF **96/96** en la cancha, desvío 0, `bloquesDeUno`
      0, `huecosNoFinal` 0, **0 pares cruzados**, `presentable()` y **231 939 ms** (< 300 000);
      `test_solver_centro_sin_ef` **209,16 s** verde. Suites: `backend` 62/62 y `full-release` 67/67.

- [x] **T6. Contrato, guías e interfaz.** RF-11 (`RNF-8` se verifica en T4)
    - `docs/contrato-solver-2.0.md`: capacidad `aulas × franjas × aforo`, causa (2) con las cuatro cifras,
      P4 por conteo con aforo y la nota «el aforo es regla del motor y no viaja en el contrato».
      `docs/guias/datos-prueba.md` (líneas **10**, **156**, **202-204**, **208** y **210-211**) y
      `docs/guias/datos-prueba-frontend.md` (**163-167**, **170**, **198-199** y **219**): «Generable: sí»
      con 48 ≤ 80, 96 ≤ 160 y 24 ≤ 40 plazas de par, aforo de cursos frente a 88 alumnos, **fuera toda
      promesa de aforo pendiente**. `docs/interfaz-frontend.md`: fuera el aforo pendiente y el `NoFactible`
      «en la vista» como deuda del aforo. Sin pantallas nuevas: la **vista de horario** queda fuera de
      alcance (`src/frontend/src` no se toca).
    - Hecho cuando: `grep -rn "no generable\|no es generable\|aforo pendiente\|aforo simultáneo\|aforo 1\|
      Generable: no" docs/guias/datos-prueba.md docs/guias/datos-prueba-frontend.md docs/interfaz-frontend.md`
      **no** devuelve nada; `grep -n "aforo\|generable" docs/guias/datos-prueba.md
      docs/guias/datos-prueba-frontend.md` solo muestra líneas con **aforo 2** o **Generable: sí**; y
      `grep -n "aulas × franjas × aforo" docs/contrato-solver-2.0.md` devuelve la fórmula.

- [x] **T7. Constitución y supersesiones (tarea del PLANNER; el implementer NO la ejecuta).** RF-11
    - **Hecho (2026-11-10):** las **cuatro notas de supersesión** están puestas y verificadas
      (`grep -ln "018" …` → los **cuatro** archivos: 005, 010, 014 y 017).
    - **Enmienda de `docs/constitution.md`: entregada como PARCHE** al coordinator (el planner **no tiene
      permiso de escritura** en `docs/`; solo en `specs/`). Las cuatro zonas (§7.1.3, §7.1.5, §7.3 y §7
      «Comportamiento») y el bloque de enmienda del 2026-11-10 están redactados en la respuesta de T7;
      **§7.4 no se toca** (V1–V19 se mantienen). Queda **aplicar el parche** antes de que T8 dé la
      verificación manual por buena.

- [x] **T8. Cierre: suites, alcance y estado.** RNF-2, RNF-6
    - Pasar las tres suites y comprobar el alcance del diff; anotar la línea base medida (los recuentos
      nuevos) en `journal.md` y `estado.md`.
    - Hecho cuando: `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset
      backend`, `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset
      full-release` y `ctest --preset slow-release` están **verdes**; `git status --short -- src test docs`
      muestra solo archivos del plan §2 (sin cambios en `src/backend/src/database/` ni en
      `src/frontend/src/`); y se confirma que `docs/constitution.md` y las notas de las specs 005/010/014/017
      están puestas (T7 del planner). `specs/018-…/` está **sin seguimiento** y queda **fuera** del control
      de alcance (incluidos `tasks.md` y `journal.md`).
    - **NO CUMPLIDA (2026-11-10, tres intentos; journal §T8).** Las dos suites rápidas están **verdes**
      (`backend` 62/62, `full-release` 67/67) y el alcance del diff es el del plan §2, pero faltan dos
      cosas que **no se pueden resolver desde el implementer**:
      **(1)** la **enmienda de `docs/constitution.md`** sigue sin aplicar: el perfil del agente
      (`~/.config/opencode/agents/implementer.md:21-26`) tiene `edit` → `deny` sobre ese fichero y la
      autorización expresa del coordinator no lo sustituye (no se elude por `shell`); la aplica quien
      tenga permiso de escritura en `docs/` con el parche de T7.
      **(2)** `ctest --preset slow-release` da **1/2**: `test_solver_centro_real` verde (244,12 s) y
      `test_solver_centro_sin_ef` **rojo por su techo de 200 000 ms** (`tiempo_ms` 321 338 / 200 641 /
      210 285 en tres corridas; todos los asserts funcionales pasan). Es varianza de tiempo del host
      (`load average` 1,7–9,0 con 8 workers en 4 núcleos), no una regresión del aforo, y el test es de la
      014: **subir el techo o tolerarlo es decisión del usuario**, fuera del alcance de la 018.
    - **HECHA (2026-11-10, journal §T8, cuarto intento).** Desbloqueada por el usuario: el **techo del test
      de la 014** pasa de **200 000** a **300 000 ms** (`test/backend/test_solver_centro_sin_ef.cpp:102`),
      alineado con el criterio de la 014 (**< 300 s**, RNF-2); es cambio del margen del test, **no** de las
      reglas del motor, y queda anotado en el journal de la **014**. La **enmienda de la constitución** la
      aplicó el coordinator (`grep -n "aforo" docs/constitution.md` → **10** líneas en las **cuatro** zonas;
      §7.4 intacto) y las **cuatro** notas de supersesión de 005/010/014/017 están puestas.
      Suites: **backend 62/62** (19,16 s), **full-release 67/67** (13,79 s) y **slow-release 2/2**
      (335,48 s): `test_solver_centro_real` **876 clases**, EF **96/96** en la cancha, desvío 0,
      `bloquesDeUno` 0, huecos 0, **0 pares cruzados**, **161 463 ms**; `test_solver_centro_sin_ef`
      **157 884 ms** (848 clases, desvío 0, separación 2 ≤ 83, huecos 0). Alcance: **26** entradas en
      `git status --short -- src/ test/ docs/` = los 24 del plan §2 más `src/seed/include/seed/docentes.hpp`
      (doc de P033) y `test/backend/test_solver_centro_sin_ef.cpp` (el techo); sin
      `src/backend/src/database/` ni `src/frontend/src/`. Sin commits.
