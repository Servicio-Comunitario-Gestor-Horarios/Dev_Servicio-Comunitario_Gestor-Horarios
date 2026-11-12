# Tareas 014 — 014a (motor) y 014b (cambio de requisitos: 8 workers)

Base: `bf06df67c32fc3dceb13a4a67226c47eb65004b2`. Comando base de cada tarea:

```bash
cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend
```

Tests primero en todas; ninguna tarea cierra con el árbol roto. T7 usa además:

```bash
cmake --preset full-release && cmake --build build-rel --parallel \
  && ctest --preset full-release -E test_generacion_seed
```

- [x] **T1. Retirar S3 de motor y tests; dejar `PresupuestoFases` sin `agrupacion`.** RF-6, RF-7
    - Hecho cuando: `PresupuestoFases` ya no tiene `agrupacion` y deja el hueco `separacionHuecos`; no
      quedan `PESO_AGRUPACION`, `calcularPenalizacionGrupo`, `esInicioSegmento`, `restricciones_agrupacion`,
      `construirObjetivoSoloS3`, `GrupoAgrupacion`, `penalizacionAgrupacion`, `longGroup`, `isolated` ni
      `vars.hay`/`hay[`; se adaptan **todos** los tests que tocan S3 (`test_solver_preferencias`,
      `test_solver_horarios`, `test_solver_benchmark`, `test_edge_cases`, `test_contrato_solver`,
      `test_validador_salida`, `test_variables_cp_sat`) y `segmentosTurno` se conserva; el grep
      `grep -RIn "PESO_AGRUPACION\|calcularPenalizacionGrupo\|esInicioSegmento\|GrupoAgrupacion\|penalizacionAgrupacion\|restricciones_agrupacion\|longGroup\|isolated\|vars\.hay\|\.hay\[\|agrupacion" src test`
      no devuelve nada; `ctest --preset backend` verde.

- [x] **T2. Funciones puras de la rejilla, el desvío único y la causa (5).** RF-1, RF-2, RF-3, RF-4,
      RF-5, RF-6, RF-10, RF-11
    - Hecho cuando: `test_bloques` verde con tramos de 2, 4, 5 y 6 franjas (el 5 no es emparejable),
      materias de 1, 2, 3, 4, 5, 6, 7–10 y 11 h, **`desvioDeCurso` con la única fórmula**
      `Σ máx(0, descubiertas − h mod 2)` (un bloque de 1 **no** suma; `bloquesDeUno` lo cuenta aparte),
      `cotaDeCurso`, `causaCincoDeCurso` (total impar y `⌈h/2⌉ > días`, con sus textos) y `pesosBlandos`
      (la separación domina a los huecos y el hueco final al de viernes).

- [x] **T3a. Variables y emparejamiento (R1/R2).** RF-1, RF-7
    - Hecho cuando: `test_variables_cp_sat` verde con `ocupada`/`par`/`materiaEnFranja` (sin `hay`/`isolated`);
      `test_solver_bloques` verde con (a) un segmento lleno que produce sus pares, (b) un tramo impar que
      **no** tiene solución y (c) el modelo construido imprime su tamaño (**~30–40k booleanas**; se
      registra en el journal y se poda si supera ~40k).

- [x] **T3b. Misma aula en el par (R7).** RF-9, RF-7
    - Hecho cuando: `test_solver_bloques` verde con (a) un **par simple** (dos franjas de la misma materia)
      que usa una sola aula, (b) un **par de dos materias** que usa una sola aula y (c) dos pares distintos
      del mismo tramo de 4 que **pueden** usar aulas distintas; el tamaño del modelo se registra.

- [x] **T4. Alineación (R5/R6), resto compartido y causa (5).** RF-2, RF-5, RF-7, RF-10, RF-11
    - Hecho cuando: `test_solver_bloques` verde con 3 h+3 h compartiendo bloque, 4 h sin compartir, 5 h en
      2+2+1, 6 h en 2+2+2, una materia de 11 h y un curso de total impar con la causa (5) exacta, y un
      fallo de aula concreta que da (2) o «no atribuible» (nunca (5)); `test_solver_horarios` ajustado a
      totales pares y huecos al final; el tamaño del modelo se registra.

- [x] **T5a. Separación blanda y fase 2a (separación).** RF-3, RF-7, RF-8
    - Hecho cuando: `test_solver_preferencias`/`test_solver_bloques` verdes con (a) una materia de 6 h en
      lunes/miércoles/viernes, (b) el valor de separación informado igual al recálculo y **≤** el de la
      referencia (con `separacionHuecos` agotado), y (c) la escalera 2a→referencia;
      `PresupuestoFases.separacionHuecos` se usa.

- [x] **T5b. Huecos blandos, preferencia al final y fase 2b.** RF-4, RF-7, RF-8
    - Hecho cuando: `test_solver_centro_sin_ef`/`test_solver_preferencias` verdes con (a) los huecos al
      final del día y preferentemente el viernes, (b) la preferencia que **no** rompe una regla dura,
      (c) la fase 2b que fija el valor de 2a, (d) sin preferencias se omite 2b y (e) 2b→2a en la escalera.

- [x] **T6. Informe del resultado y validador de salida.** RF-6, RF-12
    - Hecho cuando: `test_analisis_bloques` verde con bloques simples/compartidos, `bloquesDeUno` (0), **un
      solo `desvio`**, tope, días de dominio (0 = domingo), huecos y separación; el campo aditivo `bloques`
      llega a `ResultadoGeneracion`; P4 detecta un bloque de 1, una materia mal alineada o dos aulas en un
      bloque como **conflicto**; la salida del solver no cambia.

- [x] **T7. Fixture de aceptación sin EF, benchmark y suites.** RF-11, RNF-1, RNF-2, RNF-4, RNF-5, RNF-6,
      RNF-7
    - Hecho cuando: el fixture de 24 cursos (34/36 h) resuelve con **848 clases** y **desvío 0** (fórmula
      única) y **`bloquesDeUno` 0**; el informe coincide con el recálculo; la separación informada es
      **≤** la de la referencia (obtenida con `separacionHuecos` agotado), no se asume 0, y se registra;
      los huecos se registran y la colocación
      medida se documenta con la distinción esperada (**34 h con dos materias de 6 h** → 4 al final del
      viernes + 2 al final de otro día; **34 h sin ellas** → 6 al final del viernes; **36 h** → 4), sin
      tratarla como óptimo blando; dos ejecuciones dan el mismo horario/desvío; tiempo **< 60 s** en la 014a
      (superado: hoy el **plazo es 300 s**, ver la nota); objetivo
      **≤ 9,4 s**, registrado; `ctest --preset backend` verde y `full-release` **excluyendo
      `test_generacion_seed`** verde (T7 no depende de la 014b); `git diff --stat` no toca base ni esquema.

> **Cambio de requisitos (2026-11-06).** El usuario decide que el determinismo **no es esencial**: el
> motor busca con **8 workers** y se exige **validez**, no el mismo horario entre ejecuciones. La
> comprobación de «dos ejecuciones idénticas» y el objetivo de **≤ 9,4 s** de T7 quedan **superados**; B1–B4
> los sustituyen por «ambas válidas», el **plazo de 300 s** (002) y el defecto de 8 workers con 60/20/10.

## 014b — Cambio de requisitos: 8 workers y presupuestos

Comandos de la 014b:

```bash
cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend
cmake --preset full-release && cmake --build build-rel --parallel \
  && ctest --preset full-release -E test_generacion_seed
ctest --preset slow            # test lento en backend (Debug)
ctest --preset slow-release    # test lento en Release
```

- [x] **B1. `PresupuestoFases` con 8 workers por defecto y presupuestos 60/20/10.** RNF-1, RF-7
    - Hecho cuando: `workers = 8` por defecto (la guarda `> 0` se conserva), `referencia = 60.0`,
      `separacionHuecos = 20.0`, `preferencias = 10.0`; `Solver::ejecutar` usa `presupuesto.workers` y
      `random_seed = 0`; un test puro comprueba los cuatro valores por defecto; `ctest --preset backend`
      verde (61/61).

- [x] **B1b. Plazo del proceso de cálculo a 300 s en el servicio.** RF-1 (002), RNF-2
    - Hecho cuando: `ServicioGeneracion` tiene `plazoMs = 300000` por defecto (comentario de cabecera
      actualizado); salud y apagado siguen en 5 s; `ctest --preset backend` verde (61/61); la 002 cita el
      plazo de 300 s y el `docs/interfaz-frontend.md` se ajusta en B3.

- [x] **B2. Aceptación con el defecto (8) y medición del centro.** RNF-1, RNF-2
    - Hecho cuando: `test_solver_centro_sin_ef` usa `Solver()` **por defecto** (sin `PresupuestoFases`
      explícito) y conserva `LABELS "slow"`; el centro completo (**848 clases**) resuelve en **Release**
      con los valores por defecto (desvío 0, 0 bloques de 1) y el **tiempo de pared se registra en
      `journal.md`**; `ctest --preset slow` (Debug) y `ctest --preset slow-release` verdes.
    - **Medición (journal):** **172,6 s** en Release, **848 clases**, **desvío 0**, **separación 2**; **183,58 s**
      el test lento (`slow-release`).
    - **Ampliación (B5b):** el test asevera **separación informada ≤ la referencia** y **`huecosNoFinal
      == 0`**; el viernes se informa (preferencia blanda).

- [x] **B3. Constitución, contrato y supersesiones del determinismo, del plazo y de las reglas duras.**
      RNF-1, RF-13 *(ampliación 2026-11-06: B5b–B7)*
    - Hecho cuando: la constitución §7 «Comportamiento» sustituye «Para la misma entrada el resultado es
      reproducible» por la **enmienda de la 014** (misma entrada ⇒ horario **válido**; 8 workers; no se
      promete el mismo horario); **§7.1.6** (forma dura; invariante 6 de §7.1), **§7.2** (**huecos al final duros**, **separación
      y viernes blandos**, orden **S1 > S2 > separación > viernes > preferencia**), **§7.3/§7.5** (eventos
      como segmentos, sin S3) quedan enmendados; las **notas aditivas de la 005, la 009, la 010 y la 013**
      están puestas (ver el sub-bullet de la parte de `specs/`); la **002** cita el plazo de **300 s**; el
      contrato `2.0` y el **`docs/interfaz-frontend.md`**
      (plazo 60 s / `60000` → 300 s) quedan revisados; **010** con el catálogo **(1)–(5)**; **013** sin S3 ni
      `PESO_AGRUPACION`; **009 AC-8**; **005** con cuatro categorías → **(1)–(5)** y «reproducible» →
      **válido**; `grep` no deja una promesa de reproducibilidad, un plazo de cálculo anterior a **300 s**, una **separación dura**
      ni un **7 h como causa** en la documentación.
    - **Parte de `specs/` hecha (2026-11-07).** Notas **aditivas** de supersesión puestas en **005**
      (bloque de 2 h, catálogo (1)–(5), reproducibilidad), **009** (superada entera, AC-8 y sus tests),
      **010** (eventos que definen segmentos, catálogo (1)–(5)) y **013** (RF-5, criterios 6/10, S3,
      `PESO_AGRUPACION` y su test de contrato, fases 1/2a/2b, garantía estructural). Sin cambios en el
      contenido normativo de esas specs.
    - **Pendiente (implementer):** la parte de **`docs/`** — constitución **§7/§7.1.6/§7.2/§7.3/§7.5**,
      `docs/contrato-solver-2.0.md` y **`docs/interfaz-frontend.md`** (plazo 60 s → 300 s) — y el `grep`
      final. El planner no escribe fuera de `specs/`.

- [x] **B4. Regresión y goldens de horario.** RNF-1, RNF-2, RNF-4 *(se re-ejecuta tras B5b–B7)*
    - Hecho cuando: `ctest --preset backend` **61/61**, `ctest --preset full-release -E
      test_generacion_seed` **65/65** y el test lento verde; los tests de comparación exacta
      (`test_solver_preferencias` `sinPreferencias_salidaIdenticaALaActual`, `test_solver_benchmark`
      `benchmark_verificaConsistencia`, el golden de `test_servicio_generacion` y el de la 013) se
      **relajan a validez/invariantes** o se fijan con **`workers = 1` explícito solo en ese test**, con la
      decisión documentada en `journal.md`; ninguna ruta funcional usa 1 worker.

- [x] **B5. Separación y huecos al final duros; causa (5) con 7 h; jerarquía.** RF-3, RF-4, RF-5, RF-7,
      RF-8, RF-10, RF-11 *(superada por B5b, 2026-11-06)*
    - Hecho cuando: la separación es **`apareceDia[m,c,d] + apareceDia[m,c,d+1] ≤ 1`** y los huecos son
      **sufijo** (`ocupada[c,d,s] ≥ ocupada[c,d,s+1]`); desaparecen `PESO_SEPARACION`, `huecoNoFinal` y
      `incumpleSeparacion`; la fase 2a es solo **viernes**; el orden blando es **S1 > S2 > viernes >
      preferencia**; la causa (5) cubre **7 h o más** (`⌈h/2⌉ > ⌈nº_días/2⌉`); el mensaje de «no
      atribuible» cita **(1)–(5) y el diagnóstico de bloques**; `test_solver_bloques` verde con 6 h en
      L/X/V, **7 h → causa (5)** y 0 incumplimientos; el **test de aceptación** asevera **separación 0** y
      **huecos todos al final**; `ctest --preset backend` y el test lento verdes.

- [x] **B5b. Devolver la separación a blanda; mantener los huecos al final duros.** RF-3, RF-4, RF-5, RF-7,
      RF-8, RF-10, RF-11
    - Hecho cuando: la separación es **blanda** (`incumpleSeparacion = min(apareceDia[d], apareceDia[d+1])`
      con `PESO_SEPARACION`) y los huecos siguen **sufijo** (`ocupada[c,d,s] ≥ ocupada[c,d,s+1]`, sin
      `huecoNoFinal`); la fase 2a es **separación + viernes**; el orden blando es **S1 > S2 > separación >
      viernes > preferencia**; la causa (5) vuelve a **11 h o más** (`⌈h/2⌉ > nº_días`) y **7–10 h** se
      informa; el mensaje de «no atribuible» cita **(1)–(5) y el diagnóstico de bloques**;
      `test_solver_bloques` verde con 6 h tendiendo a L/X/V, **7–10 h informado**, **11 h → causa (5)** y
      separación **≤ la referencia**; el **test de aceptación** asevera **separación informada ≤ la
      referencia** y **`huecosNoFinal == 0`**; `ctest --preset backend` y el test lento verdes.

- [x] **B6. Informe ampliado (RF-12).** RF-12, RNF-3
    - Hecho cuando: `MateriaSalida` lleva **curso**, **una aparición por bloque con sus franjas** y los
      **días incumplidos** (se informa); `CursoSalida` expone los **agregados por curso** (desvío, tope,
      huecos no-viernes); P4 detecta **hueco no sufijo** como conflicto (la separación se informa, **no** es
      conflicto); `test_analisis_bloques` y `test_validador_salida` verdes; el contrato de salida del solver
      no cambia.

- [x] **B7. Seed sintético a horas pares.** RNF-8
    - Hecho cuando: el **seed** no tiene ninguna materia impar de **laboratorio/taller** sin **pareja del
      mismo tipo** (o todas son pares); `test_seed_perfil` verde; el seed valida y las pruebas que lo usan
      no dependen de horas impares sin pareja; el cambio queda anotado en la spec 008 (nota) y en
      `journal.md`.

