# Journal 014a — Motor: bloques de dos horas

Registro de la implementación de las tareas del plan 014a. Una entrada por tarea ejecutada; las
entradas son **aditivas** (no se reescriben las anteriores).

- **Base:** `bf06df67c32fc3dceb13a4a67226c47eb65004b2` (rama `feat/Motor`, commit «backup»).
- **Fecha de apertura:** 2026-11-06.
- **Estado:** T1 completada; T2–T7 pendientes.
- **Comandos de referencia:**
  - backend: `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend`
  - final (T7): `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release -E test_generacion_seed`

---

## T1 — Retirar S3 del motor y de los tests; `PresupuestoFases` sin `agrupacion`

**Fecha:** 2026-11-06
**Requisito:** RF-6, RF-7. **Plan:** §2, §9 y §11.

### Código retirado

- `src/backend/include/backend/solver/config/contrato_solver.hpp` / `src/backend/src/solver/config/contrato_solver.cpp`
  - fuera `PESO_AGRUPACION` y su `static_assert` de jerarquía;
  - fuera `esInicioSegmento` y `calcularPenalizacionGrupo` (declaración e implementación);
  - **se conserva** `segmentosTurno` y la jerarquía `S1 > S2 > preferencia`.
- `src/backend/include/backend/solver/variables.hpp` / `src/backend/src/solver/variables.cpp`
  - fuera `hay`, `isolated`, `longGroup` y el pre-cálculo de segmentos que solo servía a S3.
- `src/backend/include/backend/solver/restricciones/restricciones_agrupacion.hpp` y
  `src/backend/src/solver/restricciones/restricciones_agrupacion.cpp`: **eliminados**; fuera la
  llamada a `agregarRestriccionesAgrupacion` en `Solver::agregarRestricciones` y la entrada en
  `src/backend/CMakeLists.txt`.
- `src/backend/src/solver/solver.cpp` / `solver.hpp`
  - objetivo de la fase 1 reducido a S1 + S2 (sin el término de `isolated`);
  - fuera la fase 2b completa: `construirObjetivoSoloS3`, la fijación del valor de preferencia y
    la lectura de `presupuesto.agrupacion`; la fase de preferencias (2a) se conserva con el anclaje
    `z*`/`e*` de la 013;
  - `PresupuestoFases`: `{ referencia = 6.0, separacionHuecos = 2.0, preferencias = 2.0 }`.
    `separacionHuecos` queda reservado (hueco) para T5a; `agrupacion` desaparece.
- `src/backend/include/backend/services/ValidadorSalidaSolver.hpp` / `src/backend/src/services/ValidadorSalidaSolver.cpp`
  - fuera `GrupoAgrupacion`, `AnalisisSalida::grupos`, `penalizacionAgrupacion` y el bloque de
    análisis S3; fuera el helper `indiceCurso` (solo lo usaba S3) y el include de
    `contrato_solver.hpp`;
  - **se conservan** P2 (avisos de exceso) y P4 (solapamientos por curso/docente/aula).

### Tests adaptados

- `test/backend/test_contrato_solver.cpp`: fuera `constantes_jerarquiaConAgrupacion`, los tres
  `esInicioSegmento_*` y los dos `calcularPenalizacionGrupo_*`; `constantes_jerarquiaConPreferencia`
  se queda con la jerarquía `PESO_HORAS > COSTE_SUPLENTE > PESO_PREFERENCIA`; los cuatro
  `segmentosTurno_*` intactos.
- `test/backend/test_validador_salida.cpp`: fuera los tres `agrupacion_*` de S3; P2/P4 intactos.
- `test/backend/test_solver_horarios.cpp`: fuera `solver_agrupacion_*` (4) y el helper
  `franjasDeMateria`, que solo usaban esos tests (los escenarios de emparejamiento vuelven en T3a/T4).
- `test/backend/test_solver_preferencias.cpp`: fuera los dos tests de la 2b
  (`fase2b_mejoraLaAgrupacionSinCambiarLaPreferencia`, `fase2bAgotada_devuelveLaFase2a`) y el helper
  `clasesAisladas`; `preferenciaGanaALaAgrupacion` pasa a `preferenciaEligeLasFranjasPreferidas`
  (misma aserción: la preferencia elige las franjas 0 y 3, sin depender de S3).
- `test/backend/test_variables_cp_sat.cpp`, `test/backend/test_solver_benchmark.cpp` y
  `test/backend/test_edge_cases.cpp`: **sin cambios**: no referenciaban ningún símbolo de S3
  (`hay`/`isolated`/`longGroup`) ni el desempate.

### Verificación

Grep de la tarea (sin restos de S3):

```text
$ grep -RIn "PESO_AGRUPACION\|calcularPenalizacionGrupo\|esInicioSegmento\|GrupoAgrupacion\|penalizacionAgrupacion\|restricciones_agrupacion\|longGroup\|isolated\|vars\.hay\|\.hay\[\|agrupacion" src test
(exit 1: sin coincidencias)
```

Suite backend en verde, sin cambios de número:

```text
$ cmake --preset backend && cmake --build --preset backend --parallel
[94/94] Linking CXX executable test/test_ipc_aislamiento_bd

$ ctest --preset backend
100% tests passed out of 58
Total Test time (real) =   8.70 sec
```

**Hallazgos:** los escenarios de `test_solver_benchmark` y `test_edge_cases` no dependían de S3
para ser factibles; no hubo que ajustar totales impares en esta tarea (queda para T4, cuando el
emparejamiento los haga infactibles). El árbol queda verde sin la fase 2b y con `z*`/`e*` de la
referencia.

---

## T2 — Funciones puras de la rejilla, desvío único y causa (5)

**Fecha:** 2026-11-06
**Requisito:** RF-1, RF-2, RF-3, RF-4, RF-5, RF-6, RF-10, RF-11. **Plan:** §3, §4.1, §4.2, §5 y §6.

### Módulo y API

Nuevo `src/backend/include/backend/solver/config/bloques.hpp` +
`src/backend/src/solver/config/bloques.cpp` (compilado en `backend`, sin OR-Tools), con:

```cpp
namespace rejilla {
struct Tramo { int dia, segmento, inicio, longitud; };
struct Par { int dia, segmento, inicio; };
struct PesosBlandos { int separacion, huecoFinal, huecoViernes; };

QVector<QVector<int>> segmentosDelCurso(config, curso);
QVector<Tramo> tramosDeCurso(config, curso, ocupadas);
QVector<Par>   paresDeCurso(config, curso, ocupadas);
int franjasDescubiertasDeMateria(config, curso, ocupadasMateria);
int franjasDescubiertasDeCurso(config, curso, ocupadasPorMateria);
int desvioDeMateria(config, curso, materia, ocupadasMateria);
int desvioDeCurso(config, curso, ocupadasPorMateria);
int bloquesDeUno(config, curso, ocupadas);
int aparicionesEsperadas(int horas);
int cotaDeCurso(config, curso);
bool cumpleTopeDeCurso(config, curso, ocupadasPorMateria);
bool causaCincoDeCurso(config, curso, QString* motivo);
PesosBlandos pesosBlandos(config);
}
```

- **Desvío único:** `desvioDeMateria = máx(0, franjasDescubiertas − h mod 2)`;
  `desvioDeCurso = Σ materias`. `bloquesDeUno` es una comprobación aparte del P4 (longitudes
  impares de los tramos del curso) y **no** suma al desvío.
- **Franjas descubiertas:** se emparejan las franjas de la materia desde el inicio de cada
  tramo (día + segmento); una materia impar deja su resto como única descubierta.
- **Causa (5):** primero total semanal impar; después `⌈h/2⌉ > nº_días`; textos del plan §6.
- **Pesos:** `huecoViernes = 1`, `huecoFinal = maxHuecos + 1`,
  `separacion = (maxHuecos + 1)²`, con `maxHuecos = Σ_c (|T(c)| − h_c)`.

### Desviaciones del plan (justificadas)

1. **`namespace rejilla` en vez de `bloques`:** `int bloques(int, int)` ya existe en el ámbito
   global (`contrato_solver.hpp`) y C++ no admite un namespace y una función homónimos en el
   mismo ámbito.
2. **`desvioDeCurso` recibe `QMap<int, QSet<FranjaSemana>>` (materia → franjas):** la firma del
   plan con un único `QSet<FranjaSemana>` del curso no permite calcular `franjas descubiertas_m`
   (la misma rejilla del curso admite repartos distintos por materia). Se añaden
   `franjasDescubiertasDeMateria/DeCurso` y `cumpleTopeDeCurso` para la comprobación del tope.
3. **Diferidas a T5a/T5b/T6:** `diasDeMateria`, `incumplimientosSeparacion`, `huecosDeCurso` y
   `huecoAlFinal` (su firma del plan tampoco basta: el hueco final depende de la ocupación). T2
   entrega solo lo que exige su «Hecho cuando»; el módulo se amplía en esas tareas.

### Tests

Nuevo `test/backend/test_bloques.cpp` (19 casos, `add_gtest` en el bloque backend). Cubre:
tramos de 2, 4, 5 y 6 franjas (el 5 no emparejable), evento que rompe el tramo, desvío con la
fórmula única (1, 2, 3, 4, 5, 6 h y 7–10/11 h en la causa), bloque de 1 que no suma al desvío,
bloque compartido legítimo, materia par en bloque compartido, cota, `cumpleTopeDeCurso`,
`aparicionesEsperadas`, causa (5) por total impar y por más apariciones que días (con textos y
`motivo` nulo/limpieza) y dominancia de `pesosBlandos`.

```text
$ cmake --build --preset backend --parallel --target test_bloques
[2/2] Linking CXX executable test/test_bloques

$ ./build-backend/test/test_bloques
[==========] 19 tests from 1 test suite ran. (0 ms total)
[  PASSED  ] 19 tests.
```

La primera ejecución tuvo 2 casos mal construidos en el propio test (se usó una materia de 6 h
para el caso de 5 h y `numDias = 1` en la causa (5)); corregidos, quedan 19/19. No se tocaron
el motor ni las suites existentes.

### Verificación

```text
$ cmake --preset backend && cmake --build --preset backend --parallel
[106/106] Linking CXX executable test/test_ipc_aislamiento_bd

$ ctest --preset backend
100% tests passed out of 59
Total Test time (real) =   7.24 sec
```

Sin warnings de compilación. `test_bloques` es el binario nuevo (58 → 59).

---

## T3a — Variables y emparejamiento (R1/R2)

**Fecha:** 2026-11-06
**Requisito:** RF-1, RF-7. **Plan:** §3.2, §3.3, §3.4, §3.6 y §5.

### Variables (`variables.hpp/.cpp`)

Se añaden a `VariablesSolver` (sin `hay`/`isolated`):

- `ocupada[c][d][s]` para **todas** las franjas del turno del curso (aunque no exista ninguna
  `x`: R1 la fija a 0);
- `par[c][d][s]` **solo** si `s+1` está en el mismo segmento (el último slot de cada segmento
  no tiene par que empiece en él);
- `materiaEnFranja[m][c][d][s]` solo donde hay al menos una `x` (sin candidato la expresión es 0
  y no se crea; la alineación de T4 tratará la ausencia como 0).

### Restricciones (`restricciones_bloques.{hpp,cpp}`, en `backend_solver`)

- **R1:** `ocupada[c,d,s] = Σ_{p,m} x[p,m,c,d,s]` (suma vacía → 0).
- **R3:** `materiaEnFranja[m,c,d,s] = Σ_p x[p,m,c,d,s]`.
- **R2:** por segmento y franja `s`, `ocupada[s] = par[s] + par[s−1]` (el término ausente en el
  borde es 0) y `par[s] ⇒ ocupada[s] = ocupada[s+1] = 1` (`OnlyEnforceIf`). `par[s]` solo se
  crea si `s+1` está en el segmento.

**Por qué no admite tramos impares:** sumando las igualdades de un segmento,
`Σ ocupada = 2·Σ par`, luego el número de franjas ocupadas de cada segmento es par. Además, en
la primera franja libre tras el tramo `ocupada = 0` fuerza `par[última] = 0`, así que la última
franja de un tramo impar quedaría sin par. Recíprocamente, cualquier tramo par se cubre con
pares desde su inicio y un tramo de 4 admite dos pares adyacentes (`par[0]` y `par[2]`).

La llamada se añade a `Solver::agregarRestricciones` después de las D-rules y el módulo a
`src/backend/CMakeLists.txt`.

### Tamaño del modelo (medido)

Modelo representativo del fixture (24 cursos, 9 materias, 12 aulas, 30 docentes, 5 días × 8
franjas, evento tras la franja 2), construido en `test_solver_bloques`:

```text
[ INFO ] modelo representativo: 30726 variables
         (8640 x, 11520 y, 960 ocupada, 720 par, 8640 materiaEnFranja), 12000 restricciones
```

Queda dentro del presupuesto **~30–40k** del plan §12: **no hace falta podar**.

### Tests

- `test_variables_cp_sat` (25 casos, +4): `OcupadaYPar_SeCreanPorSegmento`,
  `Par_RespetaElEventoDelTurno`, `MateriaEnFranja_SoloDondeHayClase` y
  `OcupadaYPar_ExistenAunqueNoHayaDocente`; `EmptyConfig` comprueba que los tres mapas nuevos
  están vacíos. Sin referencias a `hay`/`isolated`.
- Nuevo `test_solver_bloques` (`add_gtest`, 3 casos):
  - `segmentoLlenoProduceSusPares`: tramo de 4 lleno → `par[0] = par[2] = 1`, `par[1] = 0`,
    `par[3]` no existe, `ocupada` y `materiaEnFranja` a 1;
  - `tramoImparNoTieneSolucion`: 5 h en un segmento de 5 → `INFEASIBLE` (el modelo se construye
    con `par` en 0..3);
  - `tamanoDelModeloRepresentativo`: imprime el tamaño y exige 25k–40k.

```text
$ ./build-backend/test/test_backend_variables_cp_sat
[  PASSED  ] 25 tests.

$ ./build-backend/test/test_solver_bloques
[  PASSED  ] 3 tests.
```

### Escenarios existentes ajustados

El emparejamiento duro dejó infactibles escenarios con totales impares o turnos de 3 franjas
llenos. Se ajustaron manteniendo su intención:

- `test_solver_horarios` → `solver_causaDeAulaConCapacidadSuficienteNoInventaElConflicto`:
  el curso de 6 h y el de 3 h en turnos de 3 franjas no admiten tramos pares. Pasa a 4
  franjas/día, `Plan B` de 2 h y `Prof B` con 2 h de aula; sigue comprobando que el aula de
  referencia no inventa un conflicto (causa (2)).
- `test_solver_benchmark` → planes de los 8 cursos: los totales 12, 11 y 9 (y el 12 en un turno
  de 3 franjas) pasan a **10 h** por curso; con 3 franjas/día cada día lleva 2 clases (tramo
  par). El benchmark sigue midiendo el caso grande (80 asignaciones; 188 ms) y su sobrecoste
  con preferencias (338 ms, **83,7 %**; el umbral +50 % se documenta, no se aserta).
- `test_solver_preferencias`:
  - `sinPreferencias_salidaIdenticaALaActual`: el golden pasa a `1|A|0|0` y `1|A|0|1` (las dos
    clases forman el par que la nueva referencia determinista elige);
  - `configSuplente`: 5 h en segmentos de 1 franja → 4 h (par) en 5 días × 2 franjas, con
    titular/suplente de 4 h de aula; los dos tests de `evitadaAcumulada` siguen comprobando
    S1/S2 intactos con 4 clases;
  - `preferenciaEligeLasFranjasPreferidas`: la preferencia ya no puede elegir franjas no
    contiguas; se prefiere el par (0,1) y se comprueba que se elige.

### Verificación

```text
$ cmake --preset backend && cmake --build --preset backend --parallel
compilación completa sin warnings ni errores

$ ctest --preset backend
100% tests passed out of 60
Total Test time (real) =   8.23 sec
```

`test_solver_bloques` es el binario nuevo (**59 → 60**).

---

## T3b — Misma aula en el par (R7)

**Fecha:** 2026-11-06
**Requisito:** RF-9, RF-7. **Plan:** §4 (R7), §5 y §7.

### Formulación

En `src/backend/include/backend/solver/restricciones/restricciones_bloques.hpp` +
`src/backend/src/solver/restricciones/restricciones_bloques.cpp`, nueva
`restriccionMismaAulaDelPar` llamada desde `agregarRestriccionesBloques`:

```text
para cada par[c,d,s] y cada aula a:
    par[c,d,s] ⇒ y[c,d,s,a] = y[c,d,s+1,a]     (OnlyEnforceIf)
```

- Se indexan las `y` por `(c,d,s) → aula → BoolVar` y solo se publica la igualdad si el aula
  existe en las dos franjas.
- Si el par no está activo no se impone nada: dos pares distintos de un tramo de 4 pueden usar
  aulas distintas.
- Con D9 (una `y` por franja) y el tipo exacto de la 010, la igualdad obliga a que los dos
  restos de un bloque compartido requieran el **mismo tipo** de aula; si no, el bloque no se
  forma (RF-9). No se toca D8, la referencia/desplazamiento ni R2.

### Tests (`test_solver_bloques`, 6 casos: 3 nuevos)

- `parSimpleUsaUnaSolaAula`: materia de 2 h en un día de 2 franjas y 2 aulas (modo móvil); las
  dos franjas del par usan la misma aula.
- `parDeDosMateriasUsaUnaSolaAula`: dos materias de 1 h que comparten el par; las dos franjas
  usan la misma aula.
- `dosParesDelMismoTramoPuedenUsarAulasDistintas`: dos materias de 2 h con tipos de aula
  distintos en un tramo de 4; cada materia ocupa un par entero (mezclar tipos en un par es
  infactible) y los dos pares usan aulas distintas. Si R7 forzara la igualdad entre **todas** las
  franjas del tramo, el escenario sería infactible (control positivo).
- Siguen verdes los 3 casos de T3a (segmento lleno, tramo impar y tamaño).

### Tamaño del modelo

Sin variables nuevas: **30.726 variables** (sin cambios) y **20.640 restricciones** (R7 añade
8.640 implicaciones al modelo representativo). Dentro de ~40k: no se poda.

### Escenarios existentes ajustados

- `test_solver_horarios` → fixture `configDosCursosDesplazadosALaMismaFranja`: pasa de 3 a 4
  franjas por día (y docentes de 4 h). Con R7, cada par debe ser de un solo tipo; con 3
  franjas/día, el par común de un curso y el par de laboratorio del otro se solapan
  obligatoriamente en la referencia compartida y el bloqueo de la 010 lo hace infactible. Con 4
  franjas los pares caben sin solaparse y el escenario sigue probando que, con 2 laboratorios,
  la capacidad no inventa una causa (2).
- `test_solver_benchmark`: Tecnología (única materia de laboratorio) pasa de 3 a **2 h** y la
  materia 3 de 3 a **4 h** (el curso sigue sumando 10 h). Bajo R7 una materia de laboratorio de
  horas impares no tiene con quién compartir el resto (no hay otra de su tipo), así que el par
  exige horas pares. El benchmark sigue verde: 80 asignaciones, 197 ms; con preferencias 359 ms
  (**88,9 %** de sobrecoste, umbral +50 % documentado, no asertado).

### Verificación

```text
$ cmake --preset backend && cmake --build --preset backend --parallel
compilación completa sin warnings ni errores

$ ctest --preset backend
100% tests passed out of 60
Total Test time (real) =   8.34 sec
```

`test_solver_bloques` mantiene el binario y pasa de 3 a 6 casos.

---

## T4 — Alineación (R4/R5/R6) y causa (5)

**Fecha:** 2026-11-06
**Requisito:** RF-2, RF-5, RF-7, RF-10, RF-11. **Plan:** §4 (R4–R6), §6 y §7.

### Formulación

Nueva variable `bloqueSimple[m][c][d][s]` (solo si existe `par[c,d,s]` y `materiaEnFranja` en
las dos franjas). En `restricciones_bloques.cpp`, `restriccionAlineacion`:

- **R4:** `bloqueSimple = min(par, u[s], u[s+1])` con `AddMinEquality`.
- **R5:** `2·Σ simple ≥ h − (h mod 2)` por materia (implementado como
  `Σ simple ≥ ⌊h/2⌋`). Una materia par no puede tener ninguna franja fuera de un bloque simple;
  una impar deja exactamente una.
- **R6:** `Σ_{franjas del día} u − Σ_{pares del día} simple ≤ 1` por materia y día.

**Desviación necesaria del plan (R6):** la fórmula literal `Σ (u[s] + u[s+1] − simple)` por par
cuenta dos veces la franja de un par **inactivo** (con `par=0`) que roza un bloque simple; la
suma correcta es «franjas de la materia − bloques simples», que vale 1 por bloque simple, 1 por
franja en un par compartido y 2 cuando hay dos apariciones (dos simples o simple + resto).

Con R2 y D3, R4–R6 implican: una materia par nunca comparte par (no tiene resto), una impar deja
una sola franja y un par compartido contiene exactamente los dos restos de dos materias distintas.

### Causa (5)

En `Solver::primeraCausaNoFactible`, **después** de (4) y antes de «no atribuible», recorre los
cursos y devuelve el texto de `rejilla::causaCincoDeCurso` (total semanal impar o
`⌈h/2⌉ > nº_días`). Las categorías (1)–(4) siguen teniendo prioridad: un fallo de aula concreta
da (2) o «no atribuible», nunca (5).

### Tests (`test_solver_bloques`, 14 casos: 8 nuevos)

- `tresMasTresCompartenBloque`: dos materias de 3 h → 2 bloques simples + 1 compartido (una
  franja de cada); cada materia, 3 clases.
- `cuatroHorasNoComparte`: 4 h → 2 simples en días distintos, ningún compartido.
- `cincoHorasFormaDosMasDosMasUno`: 5 h + 1 h → 2 simples de la de 5 h + un compartido con el
  resto de la de 1 h.
- `seisHorasFormaDosMasDosMasDosEnTresDias`: 6 h → 3 simples, uno por día (R6).
- `causaCincoPorTotalImparDeCurso`: 11 h → `Causa (5)` «11 horas … impar».
- `causaCincoPorMasAparicionesQueDias`: 11 h + 1 h (total par) → «6 apariciones … 5 días».
- `falloDeAulaNoSeAtribuyeACausaCinco`: dos cursos de 2 h de laboratorio con un laboratorio →
  `Causa (2)`, nunca (5).
- `falloDeAulaConcretaEsNoAtribuibleNuncaCausaCinco`: referencia compartida y 3 franjas/día →
  «no atribuible», nunca (5).
- Se ajustó `segmentoLlenoProduceSusPares` (T3a) a dos materias de 2 h: con R6, una sola materia
  de 4 h ya no puede ocupar dos simples el mismo día.

### Tamaño del modelo

```text
[ INFO ] modelo representativo: 37206 variables
         (8640 x, 11520 y, 960 ocupada, 720 par, 8640 materiaEnFranja, 6480 bloqueSimple),
         28416 restricciones
```

Dentro de ~40k ⇒ **no se poda**.

### Escenarios existentes ajustados

- `test_solver_horarios`:
  - `solver_causaNoAtribuibleCuandoLaReferenciaCompartidaNoCabe`: 1A 2 comunes + 2 lab, 2B 4
    comunes, 2 días × 3 franjas (antes 2+4 y 3 h impares). Sigue probando la contención de A1 sin
    categoría.
  - `configDosCursosCasiLlenosConLaboratorios`: 3 días × 6 franjas, 4 h + 4 h comunes + 2 h de
    laboratorio por curso. Con 1 laboratorio A1 conserva 7 pares para 8 bloques comunes
    (infactible); con 2 laboratorios conserva 8 y es factible. Antes usaba 14 h en una materia
    (7 apariciones > 5 días).
  - `solver_causaNoAtribuibleCuandoElTipoTieneCapacidad`: 1 día × 2 franjas y tres cursos de 2 h
    (4 ≤ 4 de capacidad); el bloqueo de A1 por el laboratorio de 1A y A2 por 2B deja a 3C sin
    aula. Antes 6 h y 1 h.
  - `configLaboratorioEnTurnoDistinto`: 2 días × 6 franjas (los 4 h comunes necesitan dos
    apariciones); el docente de laboratorio sigue con una sola franja para 2 bloques → (4), y con
    disponibilidad completa es factible.
  - `configVentanasDeTurnoHall`: cursos de 2 h (un bloque simple) en TB {4,5} / TC {6,7}, con 1A
    y 2B compartiendo la referencia A1 y 3C desplazado a la misma aula; la capacidad del tipo
    pasa (6 ≤ 8) y el fallo de ventanas queda «no atribuible». Antes eran cursos de 1 h.
- `test_solver_benchmark`: se vacía el `turno` de los docentes (los cursos de 10 h ocupan 2
  clases **todos** los días y un docente con turno de 3 franjas no puede cubrir dos cursos el
  mismo día). Sigue midiendo el caso grande: 80 asignaciones, 355 ms; con preferencias 716 ms
  (**105,7 %**, umbral +50 % documentado, no asertado).

### Verificación

```text
$ cmake --preset backend && cmake --build --preset backend --parallel
compilación completa sin warnings ni errores

$ ctest --preset backend
100% tests passed out of 60
Total Test time (real) =  13.39 sec
```

---

## T5a — Separación blanda y fase 2a

**Fecha:** 2026-11-06
**Requisito:** RF-3, RF-7, RF-8. **Plan:** §4 (R8), §5 (fases) y §7.

### Expresión y linealización

Nuevas variables `apareceDia[m][c][d]` e `incumpleSeparacion[m][c][d]` (esta última para
`d < nº_días − 1`) y `restriccionSeparacion` en `restricciones_bloques.cpp`:

```text
apareceDia[m,c,d]     = Σ_{franjas del día} u[m,c,d,s] − Σ_{pares del día} simple[m,c,d,s]
incumpleSeparacion    = min(apareceDia[m,c,d], apareceDia[m,c,d+1])   (AddMinEquality)
objetivo 2a           = pesosBlandos(config).separacion · Σ incumpleSeparacion
```

`apareceDia` es 0/1 por R6 (una aparición por materia y día); el término efectivo es el de
**días consecutivos** (el mismo día ya lo impide R6). El peso es
`separacion = (maxHuecos + 1)²` de `rejilla::pesosBlandos` (plan §4.2). La fase 2a incluye solo
este término; los huecos llegan en T5b.

Se añaden también las funciones puras `rejilla::diasDeMateria` e
`rejilla::incumplimientosSeparacion` (plan §5), usadas por los tests para recalcular la
separación de una salida.

### Fases y presupuestos

| Fase | Objetivo | Presupuesto |
|---|---|---|
| 1. Referencia | S1 + S2 | `referencia = 6.0` |
| 2a. Separación | `separacion · Σ incumple` con `z*`/`e*` fijados por igualdad | **`separacionHuecos = 2.0`** |
| Preferencias (013) | `expresionPreferencias` con `z*`/`e*` fijados | `preferencias = 2.0` |

- La fase 2a se ejecuta **siempre**, también sin preferencias (RF-8).
- Escalera 2a→referencia: si 2a es `INFEASIBLE`/`UNKNOWN` se devuelve la referencia;
  `MODEL_INVALID` se propaga.
- La preferencia sigue después de 2a; T5b añadirá los huecos y la fase 2b que fija el valor de
  2a para que la preferencia no lo empeore.

### Tests

`test_solver_bloques` pasa a **17 casos** (3 nuevos):

- `seisHorasQuedaEnLunesMiercolesViernes`: 6 h (3 simples) en 5 días → días `{0,2,4}`, 0
  incumplimientos.
- `separacionInformadaIgualAlRecalculoYNoPeorQueLaReferencia`: la separación recalculada con
  `diasDeMateria`/`incumplimientosSeparacion` coincide con un recuento independiente; la 2a da
  **0** y la referencia (presupuesto 2a agotado) **2**; `0 ≤ 2`.
- `escalera2aDevuelveLaReferenciaSinError`: con `separacionHuecos = 0` el solver devuelve la
  referencia (separación 2), no un error.

`test_bloques` pasa a **20 casos** (+1): `diasDeMateriaYIncumplimientosDeSeparacion` cubre la
terna separada (0), los días consecutivos (2) y una materia fuera del plan (vacío).

### Tamaño del modelo

```text
[ INFO ] modelo representativo: 39150 variables
         (8640 x, 11520 y, 960 ocupada, 720 par, 8640 materiaEnFranja, 6480 bloqueSimple;
          1944 aparecen/incumplen), 30360 restricciones
```

Dentro de ~40k ⇒ no se poda.

### Tiempos

- Presupuestos deterministas: **6,0 s** (referencia) + **2,0 s** (2a) + **2,0 s**
  (preferencias) = **10,0 s** de tope; el reparto real por fase no se expone en
  `ResultadoSolver`.
- Benchmark grande medido: **762 ms** sin preferencias (fase 1 + 2a) y **1267 ms / 1188 ms**
  con preferencias (fase 1 + 2a + preferencias); sobrecoste **68,0 %** (umbral +50 %
  documentado, no asertado).

### Verificación

```text
$ cmake --preset backend && cmake --build --preset backend --parallel
compilación completa sin warnings ni errores

$ ctest --preset backend
100% tests passed out of 60
Total Test time (real) =  18.54 sec
```

No hizo falta ajustar escenarios existentes: la fase 2a es compatible con los óptimos actuales
(solo cambia la salida donde la separación podía mejorar).

---

## T5b — Huecos blandos, preferencia al final y fase 2b

**Fecha:** 2026-11-06
**Requisito:** RF-4, RF-7, RF-8. **Plan:** §4 (R9), §5 (fases) y §7.

### Huecos blandos

Nueva variable `huecoNoFinal[c][d][s]` (solo si hay alguna franja posterior en el mismo día) y
`restriccionHuecos` en `restricciones_bloques.cpp`, codificada con **implicaciones** (sin
variable `ocupadaDespues`):

```text
huecoNoFinal            ⇒ ¬ocupada[s]
huecoNoFinal            ⇒ (∨_{s' > s} ocupada[s'])
¬ocupada[s] ∧ ocupada[s'] ⇒ huecoNoFinal       (para cada s' > s)
```

- `hueco` es la expresión `1 − ocupada` (no necesita variable).
- `huecoNoFinal` es exactamente «hueco con clase después en el día».
- El objetivo de la **2a** es:

```text
separacion · Σ incumpleSeparacion
+ huecoFinal · Σ huecoNoFinal
+ huecoViernes · Σ_{d != 4} (1 − ocupada[c,d,s])
```

  con `pesosBlandos`: `huecoViernes = 1`, `huecoFinal = maxHuecos + 1`,
  `separacion = (maxHuecos + 1)²`. La separación domina a todos los huecos y el hueco no final a
  todos los de viernes. El viernes es el día de generación 4 (0 = lunes).

### Fases finales

| Fase | Objetivo | Presupuesto |
|---|---|---|
| 1. Referencia | S1 + S2 | `referencia = 6.0` |
| 2a. Separación y huecos | expresión anterior con `z*`/`e*` fijados | `separacionHuecos = 2.0` |
| 2b. Preferencia | `expresionPreferencias` con **el valor de la 2a fijado por igualdad** | `preferencias = 1.0` |

- **Sin preferencias se omite la 2b** y se devuelve la 2a.
- Escaleras: 2a→referencia y 2b→2a si `INFEASIBLE`/`UNKNOWN`; `MODEL_INVALID` se propaga.
- La preferencia no puede tocar reglas duras (son restricciones) ni empeorar la separación o los
  huecos (valor de 2a fijado).

### Tests

`test_solver_bloques` pasa a **21 casos** (4 nuevos):

- `huecosAlFinalYPreferentementeElViernes`: materia de 4 h en 5 días × 4 franjas → dos pares al
  inicio de sus días (huecos al final) y **viernes libre** (sus 4 huecos no cuestan).
- `preferenciaNoRompeLaSeparacionNiElValorDeLa2a`: preferencia por lunes/martes/miércoles en una
  materia de 6 h; la 2a ya eligió `{0,2,4}` con separación 0 y la 2b no lo empeora.
- `sinPreferenciasSeOmiteLa2b`: el presupuesto de 2b no altera la salida cuando no hay
  preferencias.
- `escalera2bDevuelveLa2a`: con la 2b agotada se devuelve la 2a (`{0,2,4}`), no un error.

`test_solver_preferencias` se ajustó:

- `configInvariantes` pasa a **2 aulas** (refs A1/A2): con una sola aula, mover las clases a la
  preferencia cambiaba el valor de la 2a y no había movimiento posible; con dos, el control de
  «la preferencia cambia dónde caen las clases» se mantiene.
- `sinPreferencias_salidaIdenticaALaActual`: el par determinista de la 2a cae en el día 1,
  franjas 0 y 1.
- `preferida_atraeLaClase`: la preferencia atrae al **inicio** del día preferido (0,1), la única
  posición que no empeora los huecos; preferir 2,3 dejaría huecos no finales.
- El chequeo de invariantes de `preferenciaNoRompeInvariantesDuras` indexa docente y aula en la
  clave (con dos aulas, dos cursos pueden coincidir en (día, slot) con recursos distintos).

### Tamaño del modelo

```text
[ INFO ] modelo representativo: 39990 variables
         (8640 x, 11520 y, 960 ocupada, 720 par, 8640 materiaEnFranja, 6480 bloqueSimple;
          1.944 separación + 840 huecoNoFinal), 35400 restricciones
```

Dentro de ~40k (39.990) ⇒ no se poda.

### Tiempos

- Presupuestos: **6,0 s** (referencia) + **2,0 s** (2a) + **1,0 s** (2b) = **9,0 s** de tope.
- Benchmark grande: **864 ms** sin preferencias (1+2a) y **1380 / 1398 ms** con preferencias
  (1+2a+2b); sobrecoste **61,2 %** (umbral +50 % documentado, no asertado).

### Verificación

```text
$ cmake --preset backend && cmake --build --preset backend --parallel
compilación completa sin warnings ni errores

$ ctest --preset backend
100% tests passed out of 60
Total Test time (real) =  19.11 sec
```

---

## T6 — Informe del resultado y validador de salida

**Fecha:** 2026-11-06
**Requisito:** RF-6, RF-12. **Plan:** §4.1, §7 y §8.

### Informe puro `AnalisisBloques`

Nuevo `src/backend/include/backend/services/analisis_bloques.hpp` +
`src/backend/src/services/analisis_bloques.cpp`, añadido a `backend`:

```cpp
struct BloqueSalida   { curso, diaDominio, slotInicio, slotFin, materiaA, materiaB,
                        compartido, aula };
struct MateriaSalida  { materia, apariciones, diasDominio, franjasDescubiertas, resto,
                        incumplimientosSeparacion };
struct HuecoSalida    { curso, diaDominio, slotInicio, slotFin };
struct AnalisisBloques {
    QVector<BloqueSalida> bloques;
    QVector<MateriaSalida> materias;
    QVector<HuecoSalida> huecos;
    int bloquesSimples, bloquesCompartidos, bloquesDeUno;
    int desvio, tope;                 // desvío ÚNICO y Σ(h mod 2)
    int incumplimientosSeparacion;
    int huecosNoFinal, huecosNoViernes;
};
AnalisisBloques analizarBloques(const HorarioSalida&, const SolverConfig&);
```

- Bloques: pares de `rejilla::paresDeCurso`; materia/aula reales de cada franja; `compartido`
  cuando las materias difieren.
- Materias: apariciones (bloques en los que participa), **días de dominio** `(d+1) % 7`,
  franjas descubiertas, resto y separación.
- Un único `desvio` y `tope`, con las funciones puras de T2; `bloquesDeUno` es la comprobación
  aparte (0 en toda solución).
- Huecos: tramos de franjas libres por día; `huecosNoFinal` y `huecosNoViernes` cuentan franjas.
- Se calcula sobre el `HorarioSalida`; **no cambia la salida del solver**.

### P4 ampliado

`ValidadorSalidaSolver` añade `comprobarBloques` (además de los solapamientos existentes):

- **bloque de 1** (`rejilla::bloquesDeUno > 0`) → conflicto;
- **materia mal alineada** (`franjas descubiertas > h mod 2`) → conflicto;
- **dos aulas distintas en un bloque** → conflicto.

### Campo aditivo

`ResultadoGeneracion` incorpora `AnalisisBloques bloques`, calculado en `alResponder` con
`analizarBloques(horario, m_config)`; `test_servicio_generacion::ok_presentable` comprueba el
campo (1 bloque simple, 0 compartidos, 0 bloques de 1, desvío 0, 1 materia).

### Tests

Nuevo `test_analisis_bloques` (`add_gtest` en `backend`, 6 casos):

- `bloquesSimplesYCompartido`: dos simples + un compartido, día de dominio, aula y materias.
- `desvioUnicoYTope`: desvío 0, tope 2, apariciones/días/resto/separación por materia.
- `huecosAlFinalYNoViernes`: tramo de huecos al final del día y conteo fuera del viernes.
- `p4DetectaBloqueDeUno`, `p4DetectaMateriaMalAlineada` y
  `p4DetectaDosAulasEnUnBloque`: los tres conflictos del P4.

### Verificación

```text
$ cmake --preset backend && cmake --build --preset backend --parallel
compilación completa sin warnings ni errores

$ ctest --preset backend
100% tests passed out of 61
Total Test time (real) =  21.21 sec
```

No se ajustó ningún escenario existente: el informe y el P4 ampliado no cambian el modelo ni la
salida del solver; los fixtures del validador que no declaran `dimensiones` (preexistentes) no
activan las comprobaciones de bloques, y los casos nuevos declaran dimensiones completas.

---

## T7 — BLOQUEADA: el fixture de aceptación no resuelve en plazo

**Fecha:** 2026-11-06
**Estado:** **no completada**; T7 sigue sin marcar. No se integró ningún test ni dato nuevo;
el árbol queda en T6 (backend 61/61).

### Qué se intentó

Se construyó el fixture del plan §8 (24 cursos, 8 de 34 h y 16 de 36 h = 848 clases, 12
aulas, turnos de 8 franjas, evento tras la franja 2) con dos planes y 30 docentes por
round-robin, y con variantes de desacoplamiento (planes por sección y un titular por entrada).
El modelo resultante ronda las **~40k variables**, como el modelo representativo de T3a (39.990
medidas).

### Medidas (backend Debug y full-release)

| Escenario | Presupuesto | Resultado | Tiempo de pared |
|---|---|---|---|
| Fixture 24 cursos (2 planes, 30 docentes) | referencia 6 s | **UNKNOWN** | ~23 s |
| Fixture 24 cursos (2 planes, 30 docentes) | referencia 60 s | **UNKNOWN** | ~266 s |
| Subconjunto de 2 cursos | 6 s | factible | 0,9 s |
| Subconjunto de 4 cursos | 6 s | factible | 2,4 s |
| Subconjunto de 8 cursos | 90 s | factible | 143 s |
| Fixture 24 (planes por sección, 222 docentes, poda de `y`) | Release 60 s | **UNKNOWN** | ~276 s |
| Fixture 24 con 8 workers | 10 s deterministas | factible | ~31–47 s |

La búsqueda crece de forma abrupta con el número de cursos: el subconjunto de 8 ya necesita
**143 s** con 90 s de presupuesto, y el de 24 no termina ni con 60 s. Con 8 workers el de 24
encuentra solución, pero el plan exige **1 worker** por RNF-1 y dos ejecuciones idénticas.

### Diagnóstico

- No es un fallo de los datos de horas: cada curso por separado resuelve y el modelo es
  factible en subconjuntos (8 cursos con 90 s).
- El problema es la **explosión combinatoria** del modelo global: rejilla casi llena
  (34/36 h sobre 40), emparejamiento/alineación exactos y cursos que comparten docentes,
  con fuerte simetría de días y secciones. CP-SAT no la rompe ni la descompone.
- Por tanto, **no se cumple ni el objetivo de 9,4 s ni el límite duro de <60 s** con el
  modelo actual; bajar el presupuesto o relajar una comprobación sería rebajar el criterio en
  silencio (prohibido por RF-7/RNF-2).

### Opciones para el coordinator (no aplicadas)

1. **Descomposición/hint**: resolver por curso/turno y usar el horario construido como
   `AddHint`; o descomponer el modelo por componentes conexas de docentes/aulas. Es un cambio
   de motor que excede T7.
2. **Revisar el fixture/RNF-2**: reducir el número de cursos de aceptación o el uso de la
   rejilla (más holgura) y recalcular las cifras (848 clases, 17/18 bloques, 6/4 huecos).
3. **Ampliar el presupuesto** de la referencia (p. ej., >60 s) y ejecutar el fixture en
   `full-release` con 8 workers; exigiría cambiar RNF-1/RNF-2 y la promesa de determinismo.

### Estado del árbol

Se retiraron los archivos de fixture/test de T7 y su registro en `test/CMakeLists.txt`. La
poda de `y` que se probó también se revirtió. `ctest --preset backend` sigue en **61/61**.

---

## T7 — Addendum: medición con el límite ampliado (sin cancha)

**Fecha:** 2026-11-06
**Estado:** sigue **bloqueada**; T7 sin marcar.

Se midió el fixture del plan §8 (24 cursos, 8×34 h + 16×36 h = 848 clases, 30 docentes,
12 aulas) con **1 worker y seed 0**, en `full-release` (Release), con `separacionHuecos = 0`
(solo fase 1: horario válido) y una escalera de `max_deterministic_time`:

| `max_deterministic_time` | Pared | Estado | Clases / desvío / bloques de 1 |
|---|---|---|---|
| 60 s | 286,5 s | **UNKNOWN** | — |
| 120 s | 544,0 s | **UNKNOWN** | — |
| 300 s | >1.500 s (cortado a los 25 min) | **sin terminar** | — |
| 60 s, **8 workers (solo referencia)** | **24,3 s** | **FEASIBLE** | 848 / 0 / 0 |

**Número de 1 worker:** **no aparece un horario válido dentro de los ~25 min de pared** del
límite de medición (60 y 120 s deterministas agotados; 300 s no completado). Con 8 workers la
solución aparece en 24,3 s, pero ese número no cumple RNF-1 (determinismo con 1 worker).

No se puede fijar un umbral «tiempo real + margen» para el test de aceptación con 1 worker:
no hay tiempo real medido. Opciones para el coordinator: (a) aceptar 8 workers y verificar
determinismo empírico; (b) dar más margen de medición (el motor puede tardar >25 min) y fijar
un umbral mayor; (c) añadir `AddHint`/descomposición (cambio de motor). No se integra el test
de T7 ni se marca la tarea; el árbol vuelve a T6 y las suites normales quedan verdes.

```text
backend:      100% tests passed out of 61
full-release: 100% tests passed out of 65   (ctest --preset full-release -E test_generacion_seed)
```

---

## T7 — Cierre: aceptación con 8 workers (producto en 1 worker)

**Fecha:** 2026-11-06
**Requisito:** RF-11, RNF-1, RNF-2, RNF-4, RNF-5, RNF-6.

### Workers

`Solver::PresupuestoFases` incorpora `int workers = 1;` (al final). `Solver::ejecutar(double,
int workers)` pasa el valor a `SatParameters::set_num_search_workers` (con guarda `> 0`). El
camino de producción usa `PresupuestoFases{}` → **1 worker y seed 0**; solo la prueba de
aceptación usa 8.

### Fixture y test

`test/backend/fixtures/centro_sin_ef.{hpp,cpp}` y `test/backend/test_solver_centro_sin_ef.cpp`
(24 cursos, 8×34 h + 16×36 h, 12 aulas, 30 docentes, turnos de 8 franjas, evento tras la
franja 2).

El modelo del fixture mide **40.710 variables y 50.544 restricciones**.

Resultados con 8 workers (las dos ejecuciones completas):

- **848 clases**, **desvío 0**, **bloques de 1 = 0**, 420 bloques simples + 4 compartidos.
- Separación informada **2**; recálculo independiente **2**; referencia con
  `separacionHuecos = 0` **149** (2 ≤ 149).
- P4 `presentable`.
- Tiempos medidos en backend (Debug): **380,8 s** y **642,5 s**; `ctest --preset slow`
  total **1153,67 s**. En Release, con solo fase 1 (la medición del número de referencia),
  **24,3 s**; el test completo en Release no terminó en el corte de 10 min de esta medición.
- **Determinismo con 8 workers: NO.** Las dos ejecuciones no coinciden; el test imprime el
  aviso y **no aserta** la igualdad (el producto sigue con 1 worker y seed 0, que conserva
  RNF-1).

Techo del test: **1.200.000 ms (20 min)** por ejecución, holgado sobre los 380,8/642,5 s
medidos en el entorno donde corre (`backend`, Debug). No se baja a 120 s porque el test se
ejecuta en Debug; el número de 24,3 s era Release y solo fase 1.

### Política de ejecución

- El test lleva `LABELS "slow"`.
- Los presets de test `backend`, `full` y `full-release` **excluyen** la etiqueta `slow`.
- `ctest --preset backend -L slow` no encuentra tests porque el preset ya excluye la etiqueta
  (limitación del harness); se ejecuta con:
  - `ctest --preset slow` (preset de test con `include label slow`, backend), o
  - `ctest --test-dir build-backend -L slow`,
  - y en Release: `ctest --preset slow-release`.
- La suite normal queda: `ctest --preset backend` y
  `ctest --preset full-release -E test_generacion_seed`.

### Verificación

```text
$ ctest --preset backend
100% tests passed out of 61

$ cmake --preset full-release && cmake --build build-rel --parallel \
    && ctest --preset full-release -E test_generacion_seed
100% tests passed out of 65

$ ctest --preset slow
1/1 Test #59: test_solver_centro_sin_ef ... Passed  1153.67 sec
100% tests passed out of 1
```

---

## 014b — B1/B2: defecto de 8 workers y aceptación

**Fecha:** 2026-11-06
**Requisito:** RNF-1, RNF-2.

### B1 — Presupuesto por defecto

`Solver::PresupuestoFases` queda: `referencia = 60.0`, `separacionHuecos = 20.0`,
`preferencias = 10.0`, `workers = 8`; `random_seed = 0` se mantiene y la guarda `> 0` sigue.
`Solver::ejecutar(double, int workers)` ya usaba `presupuesto.workers` desde T7. Test puro
`TestSolverBloques.presupuestoPorDefectoDeLa014b` comprueba los cuatro valores.

### B2 — Aceptación

`test_solver_centro_sin_ef` usa `Solver()` por defecto (60/20/10, 8 workers), conserva
`LABELS "slow"`, ya no compara dos ejecuciones y mantiene 848 clases, desvío 0, 0 bloques de 1,
separación informada = recálculo y ≤ referencia, y P4 presentable.

**Medición Release** (host actual): resolución completa **156,7 s** (el test completo con la
referencia: 200,57 s). El techo de test es 200 s (Release) / 20 min (Debug) para no quedar por
debajo de lo medido. **El techo de 60 s del servicio no se cumple con 60/20/10**: el 2a con 20 s
consume ~132 s de pared. Se deja constancia para B3/B4/spec; una medición con
`separacionHuecos = 5` dio **46,6 s** (separación 90 ≤ 140), pero no se cambia el defecto de B1
sin decisión.

### Fallos de determinismo (para B4, no corregidos)

Con 8 workers fallan exactamente las comparaciones de horario entre dos ejecuciones:

- `test_solver_horarios::solver_esReproducible_mismaEntradaMismaSalida`
- `test_solver_horarios::solver_reutilizarLaInstancia_noContaminaConOtraEntrada`
- `test_solver_benchmark::benchmark_SolverGrandeConPreferencias` (dos ejecuciones con
  preferencias)

No se tocan: son RNF-1 y B4 los relaja a validez/invariantes o fija `workers = 1` explícito en
esos tests. El test de aceptación ya no aserta determinismo.

### Suites

```text
ctest --preset backend                       60/61  (falla test_solver_horarios: 2 casos RNF-1)
ctest --preset full-release -E seed          64/65  (falla test_solver_benchmark: RNF-1)
ctest --preset slow                          1/1    228,73 s
ctest --preset slow-release                  1/1    200,57 s (resolución 156,7 s)
```

B1/B2 se marcan como implementadas; los fallos de RNF-1 quedan trazados a B4 y el 60 s del
servicio queda como objetivo incumplido con los presupuestos 20/10 actuales.

---

## 014b — B1b/B3/B4: plazo 300 s, constitución y regresión

**Fecha:** 2026-11-06

### B1b — Plazo del proceso de cálculo

- `src/backend/include/backend/services/ServicioGeneracion.hpp`: `plazoMs = 300000` por defecto;
  cabecera y `@param` actualizados a **300 s**.
- `src/frontend/src/views/schedule_generation_widget.cpp`: el `ServicioGeneracion` del frontend
  pasa de 60000 a **300000**.
- Salud y apagado siguen en **5 s** (no se tocan). El middleware no fija este plazo por su
  cuenta: solo el frontend construye `ServicioGeneracion`.

### B3 — Constitución, guías y supersesiones

- `docs/constitution.md` §7 «Comportamiento»: se sustituye «Para la misma entrada el resultado es
  reproducible» por: «Para la misma entrada el resultado es **válido con las reglas vigentes**;
  la búsqueda usa **8 workers** con semilla fija y **no se promete el mismo horario entre
  ejecuciones**. Los diagnósticos y los cálculos puros siguen siendo deterministas.»
- `docs/interfaz-frontend.md`: plazo **300 s** por defecto (texto y firma `plazoMs = 300000`) y
  nota de que el horario es **válido**, no necesariamente idéntico entre ejecuciones.
- `docs/solver/Reglas-y-Configuracion-Solver.md`: la promesa de reproducibilidad pasa a validez
  con 8 workers/semilla fija y la nota de determinismo de diagnósticos/cálculos puros.
- Verificadas las notas de supersesión de **005** (RNF-1) y **013** (RNF-2); el contrato
  `2.0` no cita plazo ni reproducibilidad del horario.

### B4 — Regresión y goldens

- `test_solver_horarios`:
  - `solver_dosEjecucionesValidas_mismaEntrada` (antes `solver_esReproducible...`): compara
    éxito, número de clases y diagnóstico, no el resumen exacto; los infactibles comparan los
    errores (diagnósticos deterministas).
  - `solver_reutilizarLaMismaInstancia_daUnResultadoValido` y
    `solver_reutilizarLaInstancia_noContaminaConOtraEntrada`: validez y número de clases; la
    tercera ejecución no hereda el infactible.
- `test_solver_benchmark::benchmark_SolverGrandeConPreferencias`: se elimina la igualdad de
  `resumenDeSalida`; ambas ejecuciones deben ser válidas y con el mismo número de clases. El
  techo de 60 s se mantiene.
- `test_solver_preferencias::sinPreferencias_daUnParAdyacenteValido` (antes golden exacto):
  comprueba exito, dos clases, mismo día y franjas adyacentes.
- El golden de `huellaDatos` de `test_servicio_generacion` es de un cálculo puro y se mantiene;
  no se toca `test_generacion_seed` (014b/seed).

### Verificación

```text
ctest --preset backend                        61/61
ctest --preset full-release -E seed           65/65
ctest --preset slow-release                   1/1   190,36 s (resolución ~156 s)
ctest --preset slow                           1/1   228,07 s
```

El plazo de 300 s cubre la resolución completa (156,7 s) y deja margen para la variabilidad de
los 8 workers.

**Nota B4 (comparaciones exactas restantes).** Además de los tres tests trazados, se revisaron
las comparaciones exactas de `test_solver_preferencias`: `dosPreferenciasSolapadas...`,
`ordenDeDeclaracionNoInfluye`, `fase2SinSolucion_devuelveLaReferencia`,
`preferenciaEligeLasFranjasPreferidas`, `preferida_atraeLaClase` y la referencia de
`evitada_mueveLaClase...` usan `unSoloWorker()` (`workers = 1` **solo en el test**, configs
pequeñas). Es una decisión de test para conservar su semántica de igualdad; ninguna ruta
funcional usa 1 worker. El golden `huellaDatos` es de un cálculo puro y no se toca.

---

## 014b — B5: separación y huecos al final duros; causa (5) con 7 h

**Fecha:** 2026-11-06

### Restricciones

- **Separación dura:** se elimina `incumpleSeparacion`; `restriccionSeparacion` deja
  `apareceDia[m,c,d] = franjas del día − bloques simples del día` y añade
  **`apareceDia[d] + apareceDia[d+1] ≤ 1`**.
- **Huecos al final duros:** se elimina `huecoNoFinal` y sus implicaciones; `restriccionHuecosAlFinal`
  impone **`ocupada[c,d,s] ≥ ocupada[c,d,s+1]`** en las franjas ordenadas de cada día.
- **Objetivo:** la fase 2a minimiza **solo** `Σ_{d≠4} (1−ocupada)` (viernes blando);
  `PesosBlandos` queda con `huecoViernes = 1` (sin `separacion` ni `huecoFinal`). El orden blando
  es **S1 > S2 > viernes > preferencia**; 2b fija el valor de 2a.
- **Causa (5):** umbral `⌈h/2⌉ > ⌈nº_días/2⌉` (con 5 días, **7 h o más**); texto «la semana solo
  admite %4 días no consecutivos». El mensaje de «no atribuible» cita **(1)–(5)** y el
  diagnóstico de bloques (apariciones/días no consecutivos, restos sin pareja del mismo tipo,
  huecos que no caben al final).

### Tests

- `test_bloques` (19): causa (5) para 7–10 h con 5 días y 6 h válida; `pesosBlandos` solo
  viernes; se retiraron los tests de dominancia de separación/hueco final.
- `test_solver_bloques` (23): 6 h en L/X/V con separación 0; 7 h + 1 h → causa (5) con
  «4 apariciones … 3 días no consecutivos»; configuraciones de 3 h+3 h, 4 h, 5 h+1 h y 6 h
  ampliadas a los días no consecutivos necesarios; escalas 2a/2b y huecos al final verdes.
- `test_solver_centro_sin_ef`: asevera **separación 0** y **`huecosNoFinal == 0`** además de
  848 clases, desvío 0, 0 bloques de 1 y P4.
- Escenarios de `test_solver_horarios` ajustados a días no consecutivos: referencias
  bloqueadas/exclusividad (3–5 días), laboratorio en turno distinto (3 días), turnos distintos
  (3 días), capacidad suficiente (3 días), referencia compartida (2B 2 h) y el fixture
  «casi lleno» (5 días, 3 materias de 4 h comunes + laboratorio de 4 h; 1 lab infactible,
  2 lab factible). El fixture de aceptación pasa de **30 a 82 docentes** (un titular por
  entrada del catálogo): con 30, la concentración en días no consecutivos hacía infactible el
  centro; cursos, horas, aulas y 848 clases no cambian.

### Verificación

```text
ctest --preset backend                        61/61
ctest --preset full-release -E seed           65/65
ctest --preset slow-release                   1/1   63,95 s (centro: 55,3 s)
ctest --preset slow                           1/1   183,42 s
```

El modelo del centro mide 39.034 variables y 47.276 restricciones.

---

## 014b — B5b: vuelve la separación blanda con prioridad alta

**Fecha:** 2026-11-06

### Restricciones y objetivo

- **Separación blanda:** se retira `apareceDia[d] + apareceDia[d+1] ≤ 1`; vuelve
  `incumpleSeparacion[m,c,d] = min(apareceDia[d], apareceDia[d+1])`. `PesosBlandos`:
  **`separacion = maxHuecos + 1`**, `huecoViernes = 1` (una violación domina a todos los huecos
  no-viernes).
- **Huecos al final duros:** se mantiene `ocupada[c,d,s] ≥ ocupada[c,d,s+1]`; no hay
  `huecoNoFinal` ni peso de hueco final.
- **Fase 2a:** `PESO_SEPARACION·Σ incumple + Σ_{d≠4} hueco` con `z*`/`e*` fijados. Orden blando:
  **S1 > S2 > separación > viernes > preferencia**; 2b fija el valor de 2a.
- **Causa (5):** vuelve a `⌈h/2⌉ > nº_días` (5 días → **11 h o más**); 7–10 h se informa como
  incumplimiento de separación. El mensaje de «no atribuible» mantiene (1)–(5) y el diagnóstico
  de bloques.

### Tests

- `test_bloques` (19): causa (5) para 11 h+ y 7–10 h válidas; `pesosBlandos` con
  `separacion = maxHuecos + 1` dominando al viernes.
- `test_solver_bloques` (23): 6 h tiende a L/X/V; 7 h+1 h resuelve e informa separación > 0;
  11 h+1 h → causa (5) con «6 apariciones … 5 días»; separación de la 2a ≤ referencia;
  `huecosNoFinal = 0`.
- `test_solver_centro_sin_ef`: separación informada ≤ referencia, `huecosNoFinal == 0` y las
  demás aserciones de validez.

### Fixture con 30 docentes (el centro real)

Se revierte la alternativa de 82 docentes de B5: con separación blanda el fixture del plan es
**factible con 30 docentes** (round-robin del plan §8). Medición Release con los valores por
defecto (60/20/10, 8 workers):

- **848 clases**, desvío 0, 420 simples + 4 compartidos, **0 bloques de 1**.
- **Separación informada 2** (referencia con 2a agotada: **91**); **huecos no finales 0**.
- Modelo **39.870 variables / 46.344 restricciones**.
- Tiempo del centro: **172,6 s** (test lento completo **188,7 s**); bajo el techo de 200 s del
  test y del plazo de 300 s del servicio.

### Verificación

```text
ctest --preset backend                        61/61
ctest --preset full-release -E seed           65/65
ctest --preset slow-release                   1/1   188,71 s
```

B5 queda superada; B5b marcada.

---

## 014b — B6/B7: informe ampliado y seed a horas pares

**Fecha:** 2026-11-07

### B6 — Informe ampliado (RF-12)

- `MateriaSalida` gana `curso` («año|sección»), `franjas` (`AparicionSalida` con día de
  dominio e inicio/fin de cada bloque), `diasIncumplidosSeparacion` (días de dominio
  implicados; se informa, no es conflicto) y conserva contador/resto/descubiertas.
- `CursoSalida`: por curso, desvío, tope, bloques simples/compartidos/de 1, huecos,
  huecos no-viernes e incumplimientos de separación; `AnalisisBloques.cursos`.
- `HuecoSalida.esViernes` (dominio 5 / generación 4).
- **P4**: añade el conflicto **hueco que no es sufijo**. La **separación incumplida NO es
  conflicto**: así lo fijan la spec RF-3/RF-12 y la tarea B6; además, convertirla en conflicto
  rechazaría el propio fixture (separación 2) y contradiría B5b. Se prueba explícitamente en
  `separacionIncumplidaSeInformaNoEsConflictoP4`.
- `test_analisis_bloques`: 8 casos (curso por materia, apariciones con franjas, días
  incumplidos, agregados, huecos con `esViernes`, P4 hueco no sufijo y separación informada).

### B7 — Seed sintético

- `src/seed/src/perfil.cpp`: **Ciencias Naturales 1.º/2.º: 5 → 6 h** (laboratorio par, sin
  resto que emparejar) y compensación **GHC 1.º/2.º: 6 → 5 h** (general; quedan cuatro
  generales impares que emparejan sus restos). Cada año sigue sumando **40 h**; el plan General
  no tiene ninguna materia de laboratorio/taller impar sin pareja.
- Nota añadida a `specs/008-datos-de-prueba/spec.md` (B7). `test_seed_perfil`: **38/38**;
  `test_generacion_seed`: **3/3** (48,7 s). No se tocó `seed.md`.

### Suites

```text
ctest --preset backend                        61/61
ctest --preset full-release                   66/66  (incluido test_generacion_seed)
```

B6/B7 marcadas. B3/B4 no se tocan en esta llamada.

---

## 014b — B3 (docs): constitución, contrato y guías alineadas

**Fecha:** 2026-11-07

### `docs/constitution.md`

- §7 «Comportamiento»: catálogo de causas **(1)–(5)** (entrada inválida, aula, persona,
  asignación de docente, **conflicto de bloques**) y «no atribuible» que cita (1)–(5) y el
  diagnóstico de bloques; se mantiene la enmienda de determinismo (8 workers, horario válido, no
  idéntico).
- §7.1: nuevo invariante **6 «Forma dura del reparto en bloques»**: pares adyacentes, tramos
  impares no válidos, materia en bloques enteros salvo su resto, un bloque por materia y día,
  resto compartido entre dos materias impares del mismo tipo de aula, misma aula en el bloque y
  **huecos al final** (`ocupada[s] ≥ ocupada[s+1]`).
- §7.2: eliminada la agrupación de bloques y su peso; orden blando **S1 > S2 > separación >
  viernes > preferencia**; separación y viernes blandos; resto duro. Constantes vigentes
  `PESO_HORAS > COSTE_SUPLENTE > PESO_PREFERENCIA` (1000 > 100 > 25); pesos de separación/viernes
  por instancia.
- §7.3/§7.5: los eventos **no crean ni quitan franjas** pero **definen los segmentos** del bloque
  (dos franjas separadas por un evento no forman bloque); ya no se describen como metadata que el
  solver ignora.

### Resto de docs

- `docs/contrato-solver-2.0.md`: eventos como segmentos (no metadata ignorada).
- `docs/interfaz-frontend.md`: plazo **300 s** / `plazoMs = 300000` y nota de horario **válido**; en
  `_cuadricula`, los `recesos` visuales son los eventos y definen segmentos.
- `docs/solver/Reglas-y-Configuracion-Solver.md`: «Recesos» → **Eventos** (definen segmentos) y
  reproducibilidad → validez con 8 workers (ya alineado).
- `docs/solver/Motor-Solver-Plan-Completo.md` y `docs/solver/Restricciones-clasificacion-actualizada.md`:
  notas históricas actualizadas (el evento no ocupa franja, pero desde la 014 corta el segmento).
- Las notas de supersesión de **005 (RNF-1)** y **013 (RNF-2)** ya están puestas por el planner; no se
  tocaron specs.

### Verificación

`grep -rn "PESO_AGRUPACION|S3|1 worker|reproducible|ignora los eventos" docs/` solo devuelve:
- códigos de **Sprint 3** (`S3-Ix`) en informes/arquitectura históricos,
- «reproducible» referido a la generación byte a byte de informes de actividad o a criterios
  empíricos ajenos al solver,
- ninguna promesa vigente de 1 worker, S3, `PESO_AGRUPACION` ni eventos ignorados.

No se tocó código, tests, `seed.md` ni specs; sin commits. B3 (parte docs) cerrada.

---

## 014b — B4: regresión y goldens finales (re-ejecutada tras B5b/B6/B7)

**Fecha:** 2026-11-07

### Suites

```text
ctest --preset backend                        61/61   20,24 s
ctest --preset full-release                   66/66  128,59 s
ctest --preset slow-release                   1/1    198,16 s (centro; umbral 200 s)
```

- `test_seed_perfil`: **38/38**.
- `test_generacion_seed`: **verde** (incluido en los 66, sin exclusiones).

### Goldens de horario exacto

Revisados todos los que comparan horarios:

- `test_solver_preferencias`: las igualdades de `resumenDeSalida`
  (`dosPreferenciasSolapadas...`, `ordenDeDeclaracion...`, `fase2SinSolucion...`,
  `preferenciaElige...`, `preferida_atrae...`, referencia de `evitada_mueve...`) usan
  **`unSoloWorker()`** (`workers = 1` solo en el test). La única comparación con el defecto es
  `EXPECT_NE` como control de que la preferencia cambia el horario, no una igualdad.
- `test_solver_horarios`: los dos tests de RNF-1 se relajaron a **éxito/número de clases/
  diagnóstico** (sin resumen exacto); `resumenDeSalida` solo se usa para comprobar que no está vacío.
- `test_solver_benchmark::benchmark_SolverGrandeConPreferencias`: compara validez y **mismo número
  de clases**, no el horario.
- `test_servicio_generacion`: el golden que queda es la **huella** de datos (cálculo puro), no un
  horario.
- `test_generacion_seed`: comprueba recuentos, metadatos y rejilla, no un horario exacto.
- `test_solver_centro_sin_ef`: no compara dos ejecuciones (el determinismo no es requisito); mide
  tiempo y valida 848 clases, desvío 0, `bloquesDeUno = 0`, separación ≤ referencia y
  `huecosNoFinal = 0`.

Ninguna ruta funcional usa 1 worker: el defecto de `PresupuestoFases` es 8.

B4 marcada.

---

## Ajuste del techo del test lento (2026-11-10, cierre de la 018 / T8)

**No es un cambio de reglas del motor**: solo del margen del test.

- `test/backend/test_solver_centro_sin_ef.cpp:102` (rama Release): `kTechoMs` pasa de **200 000** a
  **300 000 ms**, alineado con el criterio de esta spec (**RNF-2**: el escenario de referencia se resuelve
  por debajo del **plazo del proceso de cálculo, 300 s**, criterio 14) y con el techo equivalente de
  `test_solver_centro_real` (018). La rama Debug (`1 200 000`) no cambia.
- **Motivo:** el techo de 200 s era un margen **más estricto que la spec** y resultó frágil con la carga del
  host. Mediciones del mismo binario, sin cambios de código, con 8 workers de CP-SAT sobre 4 núcleos:
  **149,44 s** (host en reposo), **157,88 s** y **173,80 s** (T8, `load 2,2–7,7`), **200,64 s**,
  **210,29 s**, **220,31 s**, **238,88 s** y **321,34 s** (carga alta, `load` hasta 8,96). Rango: **149–321 s**.
  Todos los asserts funcionales pasaban siempre (848 clases, desvío 0, 420 simples, 4 compartidos,
  separación ≤ referencia, `huecosNoFinal` 0).
- **Efecto:** el test deja de fallar por varianza de tiempo y solo falla si se incumple el plazo real del
  servicio. El rendimiento como tal sigue siendo materia de otra spec; aquí solo se elimina un umbral
  intermedio que la spec no exige.
- **Verificación:** `ctest --preset slow-release` → **2/2 verde** (sin EF **173,80 s**; centro real de la
  018 **161,67 s**). `backend` **62/62** y `full-release` **67/67**. Sin commits.
