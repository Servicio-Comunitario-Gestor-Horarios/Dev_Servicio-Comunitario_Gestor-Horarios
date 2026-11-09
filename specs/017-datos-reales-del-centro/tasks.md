# Tareas 017 - Datos reales del centro

Base: `b5844b580d495397903e5d3bfd8756aa41d98f05` (sha real de `HEAD`; la `Base` del encargo no existe).

> **Cada tarea cierra con el build y la suite en verde.** No hay estados intermedios: la tarea que cambia un
> dato o un símbolo actualiza en el mismo paso a todos sus consumidores y a los tests que lo usan. La única
> excepción, **declarada**, es el hueco de `test_generacion_seed`: se retira de `test/CMakeLists.txt` en T1
> (su dato cambia y no puede generar), su sustituto `test_entrada_seed` entra en T4 y el archivo se **borra**
> en el cierre (T10).
>
> **Exceso de duración:** T1 (~150 min), T4 (~50) y T5 (~40) superan los 30 min. **El usuario aprobó las 10
> tareas con el exceso el 2026-11-08.**
>
> **Corrección de la validación (2026-11-09):** **V3** aplicado por el implementer (informe con clases por
> plan y año) y **V6** cerrado **con deuda declarada** (la guía del frontend aún menciona el `NoFactible`
> «en la vista»; se retirará con el aforo). **T2** y **T7** quedan `[x]`; el resto sigue `[x]` y **T10** se
> cerró con el borrado de `test_generacion_seed.cpp`.

- [x] **T1. Sustitución del conjunto: perfil, docentes, sembrador y tests del seed.** RF-1, RF-2, RF-3,
      RF-4, RF-5, RF-6, RF-7, RF-8; RNF-4, RNF-6, RNF-7, RNF-8
    - `perfil.hpp`/`perfil.cpp`: `MateriaAnio.area/sinCarga`, `CursoDef.plan/turno/**matricula**`,
      `Perfil.duracionMinutos`, `Recuentos.duracionMinutos/configuraciones`; tablas reales (planes,
      catálogo 79 + 6 sin carga, 13 aulas con cancha, 24 cursos con letras libres y matrícula, 3 eventos);
      `validarPerfil` nueva (paridad, tablas canónicas, progresión por plan, `sinCarga`, aula de referencia
      general, sin aula repetida en el turno, 12/turno, matrícula 32–44); salen `esMateriaNueva`,
      `seccionesDeAnio`, `cargaMateria`, `cargaAsignada`, `horasMaximas`; entran `horasDeCurso`,
      `demandaTotal`, `demandaPorTurno`, `areaConCursosEnTurno`.
    - `docentes.hpp`/`docentes.cpp`: fuera `VarianteDocentes`, `nombreVariante`, `varianteDesdeTexto`,
      `coberturaDeVariante`, `docentesCumplenCarga`; **32 docentes** con áreas exactas (§4.6 del plan, con
      «Biología» **y** «Biología, Ambiente y Tecnología» en las posiciones 12, 13, 15 y 29), regla §5.4,
      P031/P032, tope 36, 5 filas de disponibilidad por turno; `validarDocentes` y `coberturaDelCentro`.
    - `sembrador.hpp`/`sembrador.cpp`: `OpcionesAplicacion` sin `variante`/`soloDocentes`,
      `EstadoAplicacion` sin `VarianteDesconocida`, configuración **40**, `crearCurso(..., curso.turno, …)`,
      sin conmutación, `recuentosEsperados(perfil)` comparado con duración y configuraciones,
      `resumenTexto(perfil, recuentos)`. `main.cpp`: fuera el parseo de `--variante`/`--solo-docentes`.
    - Tests: **`test_seed_perfil` reescrito completo** (planes, catálogo, cargas, cursos con matrícula
      889/579/310, aulas, turnos/eventos, duración, docentes, **272** y **18** de 5.º, cobertura, rechazos);
      **`test_seed_aplicacion`** con los recuentos nuevos (85/95/24/13/3/2/32/272/160, 40 min), sin slots
      de variantes, suplentes ni `--solo-docentes`; **`test_selector_generacion`**: `"NAT"` → **`"31059"`**;
      **baja de `test_generacion_seed`** de `test/CMakeLists.txt`.
    - Hecho cuando:
      `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend --output-on-failure` → **todas verdes** (línea base **62**; si `ctest -N` diera otro número, se anota el medido y el criterio es «todas verdes»), y
      `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release --output-on-failure` → **todas verdes** (línea base **67 − 1** por la baja de `test_generacion_seed`), y
      `grep -n 'kRecortes\|kMateriasNuevas\|"EXA"\|"SOC"\|"GEN"\|esMateriaNueva' src/seed/src/perfil.cpp src/seed/include/seed/perfil.hpp` → **sin resultados**, y
      `build-backend/src/seed/gestor-horarios-seed --db /tmp/c17.db` imprime `Materias: 85`, `Cursos: 24`, `Docentes: 32`, `Materias asignadas: 272`, `Disponibilidades: 160` y `Hora académica: 40 min (configuraciones: 1)`.

- [x] **T2. Informe «no generable» y motivo.** RF-8, RF-9
    - `motivoNoGenerable` (§5.5); `resumenTexto` añade centro, `Clases: 876 (EF 96; 48 por turno)`,
      **clases por plan y por año** (`31059 516 — 68/102/102/108/136` · `31060 360 — 120/80/80/80`, con el
      5.º de 31060 a 0), **clases por turno 414/462**, `Matrícula: 889 (579 + 310)`, capacidad 576, y
      `Generable: no — <motivo>`.
      Tests en `test_seed_perfil` (`motivoNoGenerable` cita 48 y 40) y en `test_seed_aplicacion` (el
      informe contiene las líneas y **no** falla la aplicación).
    - Hecho cuando: `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend -R "test_seed_perfil|test_seed_aplicacion" --output-on-failure` → verde, con el motivo citando **48** clases de EF y **40** franjas de cancha, y el informe con `876`/`889`, **clases por plan (516 · 360)**, **por año (68/102/102/108/136 · 120/80/80/80)** y **por turno (414/462)**.

- [x] **T3. CLI: ayuda, códigos y binario.** RF-1
    - Ayuda sin `--variante` ni `--solo-docentes`, descripción del centro real, aviso de que **no genera**;
      código 6 retirado; `imprimirResumen` con la firma nueva.
    - Hecho cuando: `cmake --preset backend && cmake --build --preset backend --parallel` y
      `build-backend/src/seed/gestor-horarios-seed --ayuda` **no** menciona `--variante`/`--solo-docentes`;
      `build-backend/src/seed/gestor-horarios-seed --db /tmp/x.db --variante holgada` → **código 1**;
      `build-backend/src/seed/gestor-horarios-seed --db /tmp/x.db --solo-docentes` → **código 1**; y
      `grep -rn -- "--variante\|--solo-docentes\|VarianteDocentes\|soloDocentes" src/seed` → **sin resultados**.

- [x] **T4. `test_entrada_seed` (sustituye a `test_generacion_seed`).** RF-9, RNF-2
    - Nuevo `test/frontend/test_entrada_seed.cpp`: siembra, compone con `GeneracionSolver::construirEntrada`
      los **24 cursos** con plan/turno del perfil y **`numEstudiantes = curso.matricula`**, exige sin avisos,
      valida con `validarEntradaSolver` (V1–V19), ida y vuelta del preset (24 cursos, 32 docentes, 40 min) y
      **no resuelve**; comprueba el «no generable» del informe. Alta en `test/CMakeLists.txt`:
      `add_qtest(test_entrada_seed frontend/test_entrada_seed.cpp frontend seed Qt6::Sql)` +
      `target_include_directories(... src/frontend/src)`.
    - Hecho cuando: `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release -R test_entrada_seed --output-on-failure` → verde, y `grep -n "resolverEntradaSolver" test/frontend/test_entrada_seed.cpp` → **sin resultados**.

- [x] **T5. `test_seed_aplicacion`: seguridad, determinismo y 15 s.** RF-8, RNF-1, RNF-4
    - Reemplazo total con respaldo y confirmación; base con datos falla sin escribir; atomicidad (fallo
      simulado); determinismo (dos bases con `filasDe` idénticas); reutilización del tipo general + cancha;
      `resumenTexto`; `tiempoDeAplicacion` **< 15 s**.
    - Hecho cuando: `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend -R test_seed_aplicacion --output-on-failure` → verde completo, con el cronómetro por debajo de 15000 ms y dos bases idénticas.

- [x] **T6. Guía `datos-prueba.md`.** RF-10
    - §1 recuentos (2 planes, 85 materias, 95 cargas, 24 cursos con turno y matrícula, 13 aulas, 32 docentes,
      272 asignaciones, 160 disponibilidades, 40 min), §3 uso sin variantes, §4 profesorado del centro y
      tope 36, §5 seguridad, §6 límites (no generable, EF/cancha, aforo pendiente, paquete demo fuera de
      alcance) y §7 verificación.
    - Hecho cuando: `grep -n "57 materias\|210 cargas\|65 docentes\|16 aulas\|13 cursos\|holgada" docs/guias/datos-prueba.md` → **sin resultados** y `grep -n "85 materias\|95 cargas\|24 cursos\|32 docentes\|272\|889\|40 min" docs/guias/datos-prueba.md` → **todas presentes**.

- [x] **T7. Guía `datos-prueba-frontend.md`.** RF-10 — **cierre con deuda declarada (V6)**
    - §2 recuentos, §4 «los cursos **ya traen** turno» (y la configuración no lo sobrescribe), §5 sin
      variantes, §6 checklist y §7 resumen. **No** debe prometer comportamiento de interfaz (ni pantalla,
      ni aviso, ni diálogo): el «no generable» se describe en el **informe del conjunto**, la **CLI** y la
      guía, y no contradice RF-9.
    - **Deuda declarada:** la guía aún describe el `NoFactible` «en la vista»; se retirará cuando el
      frontend implemente el aforo. No bloquea la 017 (no se toca la interfaz) y queda anotada en
      `estado.md` y en el journal.
    - Hecho cuando: `grep -n "57 materias\|210 cargas\|65 docentes\|16 aulas\|13 cursos\|holgada" docs/guias/datos-prueba-frontend.md` → **sin resultados** y `grep -n "31059\|31060\|24 cursos\|32 docentes\|889" docs/guias/datos-prueba-frontend.md` → presentes; la promesa de interfaz queda como **deuda** (arriba), no como criterio de cierre.

- [x] **T8. Supersesión de la 008.** RF-10
    - Nota en `specs/008-datos-de-prueba/spec.md` con fecha 2026-11-08: 4 planes → 2, 57 → 85 materias,
      13 → 24 cursos, 16 → 13 aulas, 4 → 1 formas de aplicación, plan General a 40 h → planes reales, «aula
      propia y distinta» → 12 compartidas, cursos sin turno → **con turno**, ventana 07:00–12:30 → **12:40**,
      35 → **40 min**. Parche de `MEMORY.md` entregado al coordinator.
    - Hecho cuando: `grep -n "017" specs/008-datos-de-prueba/spec.md` → al menos una nota con las
      supersesiones y la fecha, y el parche de `MEMORY.md` está en la respuesta al coordinator.

- [x] **T9. La interfaz con el centro real.** RF-4, RF-6, RF-10
    - Revisar `test_selector_generacion` con **24 cursos y 32 docentes** (tablas, combos de turno y plan
      `31059`/`31060`, prefs del docente) y ajustar lo que dependiera de 13/65; comprobar que la vista no
      asume «sin turno».
    - Hecho cuando: `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release -R test_selector_generacion --output-on-failure` → verde; `grep -n "65\|13 cursos\|NAT" test/frontend/test_selector_generacion.cpp` → sin resultados; `grep -n "31059\|24 cursos\|32 docentes\|889\|40 min" docs/interfaz-frontend.md docs/guias/migracion-frontend.md` → presentes; y `grep -n "017" docs/guias/desarrollo/CMAKE_GUIDE.md` → presente. **`CMAKE_GUIDE.md` queda excluido del grep de recuentos obsoletos**: su apartado del **paquete demo** describe el conjunto sintético del Plan 31059 (`13 cursos`, `65 docentes`, `57 materias`, `holgada`), que **no se regenera** con el centro real (fuera de alcance, H7).

- [x] **T10. Cierre.** RNF-3, RNF-7
    - Suites completas, tiempos medidos (aplicación), `test_selector_generacion` y verificación de alcance.
    - Hecho cuando:
      `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend --output-on-failure` **y**
      `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release --output-on-failure` → **ambas verdes**;
      `git diff --stat` sin cambios en `src/backend/src/database`, `src/backend/src/solver` ni `src/frontend/src`;
      `grep -rn "resolverEntradaSolver" test/backend/test_seed_aplicacion.cpp test/frontend/test_entrada_seed.cpp` → **sin resultados**.
