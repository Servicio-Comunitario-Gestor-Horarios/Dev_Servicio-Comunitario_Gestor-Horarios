# Plan 018 - Aforo simultáneo de la cancha

Base: `dfe529f3392648fc6ad46de16bbeec26c51d11f6` (`git rev-parse HEAD` el **2026-11-10**; `HEAD` de
`feat/Motor`, «docs(memory): cierre de la 017»). Sin cambios en el árbol de código al abrir la 018.

Línea de partida de la suite (referencia, **no** requisito — RNF-7): **61/61** en `backend` y **66/66**
en `full-release` (017, 2026-11-09). Los recuentos **subirán** con los tests nuevos; lo que se exige es
que **todo siga en verde** y que la suite normal no se degrade. Al abrir T1 se anota el conteo medido en
`journal.md`.

---

## 1. Alcance y RF cubiertos

Un **solo** cambio de modelo (un aula aloja hasta su **aforo** de cursos), su aritmética de capacidad y la
comprobación de la salida, más el informe del conjunto, la aceptación lenta y la documentación/normativa.
El aforo es una **regla del sistema por tipo de aula** (`cancha` → 2, resto → 1), **sin** contrato,
esquema, preset ni huella.

| RF | Qué se implementa | Tareas |
|---|---|---|
| RF-1 | Aforo 2 para `cancha`, 1 para el resto, sin configuración; sin tercer curso ni dos del mismo curso | T1, T2, T3 |
| RF-2 | Reconocimiento por clave normalizada; fallo explícito (motor y seed) sin aforo 1 silencioso | T2, T4 |
| RF-3 | Simultaneidad solo con cursos y docentes distintos | T2 |
| RF-4 | Nada más se relaja (tipo, referencia y bloqueo, bloques, huecos, separación, tope blando) | T2 |
| RF-5 | Solo pares completos; pares cruzados imposibles (modelo) y marcados (salida); cuenta de plazas de par | T1, T2, T3 |
| RF-6 | Capacidad = aulas × franjas × aforo; causa (2) con las cuatro cifras; precedencia intacta | T1, T4 |
| RF-7 | Salida por conteo con aforo (1 por defecto), clases sin aula fuera de la cuenta y pares cruzados | T3 |
| RF-8 | Conjunto «Generable: sí» con 48 ≤ 80, 96 ≤ 160 y 24 ≤ 40; datos intactos | T4 |
| RF-9 | Aceptación lenta del centro real: 876 clases, EF en la cancha, desvío 0, huecos, < 300 s | T5 |
| RF-10 | Sin causas nuevas; «no atribuible» se mantiene | T1, T3, T5 |
| RF-11 | Contrato, guías e interfaz (implementer); constitución y supersesiones de 005/010/014/017 (**planner**, T7) | T6, T7 |
| RNF-1…8 | 300 s, sin degradación, determinismo del diagnóstico, mensajes, datos intactos, traza, coste, contrato/huella | T1–T8 |

**Fuera de alcance (spec):** configurar el aforo, aforarlo a otros tipos, > 2, capacidad física, identidad
del curso con el plan, rendimiento, interfaz nueva, preset maestro, generación parcial.

---

## 2. Archivos y responsabilidades

| Archivo | Cambio |
|---|---|
| `src/common/include/common/aforo_aula.hpp` | **Nuevo.** Único literal y único predicado del aforo: `tipoCancha()`, `claveCancha()`, `aforoDeClave(clave)`. Reutiliza `tipo_general::claveNormalizada` (no duplica la normalización). **No** se publica `aforo(nombreTipo)`: todos los consumidores ya tienen la clave, y una segunda puerta invitaría a comparar por texto. |
| `src/backend/include/backend/solver/config/contrato_solver.hpp` | Declara `aforoDelTipo(clave)`, `aforoDelAula(config, índice)`, `posicionesDePar(turno, eventos)`; actualiza la doc de `capacidadDelTipo` (aulas × franjas × aforo). |
| `src/backend/src/solver/config/contrato_solver.cpp` | Implementa las tres y multiplica por el aforo en `capacidadDelTipo`. |
| `src/backend/src/solver/restricciones/restricciones_aulas.cpp` | `AtMostOne` por (día, franja, aula) → «como mucho el aforo del aula»; el resto de la restricción (tipo exacto, referencia, bloqueo) no cambia. |
| `src/backend/src/solver/solver.cpp` | Mensaje de la causa (2) con demanda, aulas, franjas y aforo. Orden y precedencia sin cambios. |
| `src/backend/src/services/ValidadorSalidaSolver.cpp` | P4 por **conteo** (no mapa), aforo por aula con **1 por defecto**, clases sin aula fuera de la cuenta y detección de **pares cruzados**. |
| `src/seed/include/seed/perfil.hpp` | `CapacidadCancha` y `capacidadCancha(perfil)` (pura); `tipoFinalDeMateria` documentada por **clave**; doc nueva de `motivoNoGenerable`. |
| `src/seed/src/perfil.cpp` | Fuera `kTipoCancha` (pasa a `aforo_aula`); **guarda del tipo** de la cancha por clave; **tipo de la cancha resuelto por clave** para `tipoFinalDeMateria` (devuelve el **nombre declarado**, p. ej. «CANCHA»); adaptador `TurnoConfig`/`EventoConfig` desde `seed::TurnoDef/EventoDef` para `posicionesDePar`; cuenta de plazas con aforo 2. |
| `src/seed/src/docentes.cpp` | **T4b**: posición **P033** (tarde, **Idiomas**) y **P028** con **Inglés**; prueba de docentes de 16/17; recuentos 33/165/272. |
| `src/seed/src/sembrador.cpp` | Líneas de informe de la cancha (franjas, plazas, pares) y `Generable: sí`; el `tiposDeAulas`/`idTipoAula` de las materias sigue casando porque `tipoFinalDeMateria` devuelve el **nombre declarado** del tipo. |
| `src/seed/src/main.cpp` | Ayuda de la CLI: el conjunto **sí** se declara generable; fuera el «aforo pendiente». |
| `test/backend/test_capacidad_tipo.cpp` | Aforo del tipo, capacidad con aforo, `posicionesDePar` y `toJson` sin `aforo`. |
| `test/backend/test_solver_aforo.cpp` | **Nuevo.** Modelo: dos cursos en la cancha, tres en la misma franja y **par cruzado forzado** (comprobación de modelo). |
| `test/backend/test_validador_salida.cpp` | P4 con aforo y pares cruzados (comprobación de salida). |
| `test/backend/test_seed_perfil.cpp` | `motivoNoGenerable` vacío + guarda del tipo; `capacidadCancha`. |
| `test/backend/test_seed_aplicacion.cpp` | Informe «Generable: sí» con las cifras nuevas. |
| `test/frontend/test_entrada_seed.cpp` | Informe «Generable: sí» con las cifras nuevas (sigue **sin** resolver). |
| `test/frontend/test_solver_centro_real.cpp` | **Nuevo y lento.** Siembra, compone desde la base y resuelve. |
| `test/CMakeLists.txt` | Alta de `test_solver_aforo` (backend_solver) y de `test_solver_centro_real` (etiqueta `slow`). |
| `docs/contrato-solver-2.0.md` | Capacidad con aforo, causa (2), comprobación de la salida y la nota «el aforo no viaja». |
| `docs/guias/datos-prueba.md`, `docs/guias/datos-prueba-frontend.md` | De «no generable» a «generable» con 48 ≤ 80, 96 ≤ 160, 24 ≤ 40. |
| `docs/interfaz-frontend.md` | Fuera el aforo pendiente y el `NoFactible` «en la vista» como deuda del aforo. |
| `docs/constitution.md` | §7.1.3, §7.1.5, §7.3 y §7 «Comportamiento». |
| `specs/005-nucleo-solver/spec.md`, `specs/010-…/spec.md`, `specs/014-…/spec.md`, `specs/017-…/spec.md` | Nota de supersesión fechada. |

---

## 3. El aforo en un solo punto

### 3.1 `common/aforo_aula.hpp` (nuevo)

```cpp
namespace aforo_aula {
inline QString tipoCancha()   { return QStringLiteral("Cancha"); }          // nombre visible
inline QString claveCancha()  { return tipo_general::claveNormalizada(tipoCancha()); } // "cancha"
inline int aforoDeClave(const QString& clave) { return clave == claveCancha() ? 2 : 1; }
}
```

Es el **único** sitio del literal y del 2. El seed, el motor y la comprobación de la salida lo consumen;
nadie más compara el nombre del tipo con `==`.

### 3.2 Contrato (funciones puras)

```cpp
int aforoDelTipo(const QString& clave);                    // = aforo_aula::aforoDeClave(clave)
int aforoDelAula(const SolverConfig& config, int indice);  // índice inválido → 1; si no, claveAula → aforo
int posicionesDePar(const TurnoConfig& turno, const QVector<EventoConfig>& eventos);
int capacidadDelTipo(const SolverConfig&, const QString& clave); // aulas × franjas × aforo
```

`aforoDelAula` es el puente **índice → tipo → clave → aforo** que necesitan la restricción de aula y el
validador; así no hay dos formas de resolver la clave de un aula. `posicionesDePar` = Σ ⌊|segmento|/2⌋
sobre `segmentosTurno(turno, eventos)` (1 + 3 = 4 por día en el centro).

### 3.3 Modelo (`restriccionAulaNoSolapada`)

```text
para cada grupo (día, franja, aula) → vector<y>:
    si el grupo tiene 0 ó 1 variable: nada
    tope ← aforoDelAula(config, aula)          // 1 salvo la cancha
    si tope == 1: AddAtMostOne(grupo)          // comportamiento actual, sin cambios
    si no:        AddLessOrEqual(Sum(grupo), tope)
```

Se usa `AddLessOrEqual(LinearExpr::Sum(...))` para no depender de la sobrecarga `AddAtMost(vector, int)`.
El grupo sigue construyéndose con **todas** las `y` de todos los cursos (desplazadas incluidas), que es lo
que garantiza que el aforo también cubre las clases fuera del aula de referencia.

### 3.4 Capacidad y causa (2)

```cpp
int capacidadDelTipo(const SolverConfig& c, const QString& clave) {
    return aulasDelTipo(c, clave) * franjasDelTipo(c, clave).size() * aforoDelTipo(clave);
}
```

Mensaje (mismo orden de causas, misma precedencia de (3) sobre (2)):

```text
Causa (2) conflicto de aula: el tipo «%1» necesita %2 bloques y sus %3 aulas ofrecen %4 plazas
(%5 franjas × aforo %6).
```

`%4` = capacidad, `%5` = franjas, `%6` = aforo. Para tipos de aforo 1 el texto es coherente con el actual
(`… ofrecen 8 plazas (8 franjas × aforo 1)`), y las aserciones vigentes («10 bloques», «8 franjas»,
««Laboratorio»») siguen cumpliéndose. Cancha: `1 × 80 × 2 = 160`.

### 3.5 Comprobación de la salida (P4)

```text
conteoAula[(aula, día, franja)]++   para cada asignación con aula >= 0
si conteo > aforoDelAula(config, aula) → conflicto con aula, día, franja, nº de clases y aforo
asignaciones con aula < 0: no cuentan (ni crean ni alivian conflictos)
```

Además, `comprobarParesCancha(horario, config, analisis)`:

```text
para cada (clave, cursoOut) de horario.horarios:
    curso ← CursoSolverConfig cuya claveCurso(anio, seccion) == clave; si no existe: seguir
    segmentos ← segmentosTurno(turnoDelCurso, config.eventos)
    para cada día:
        canchaSlots ← franjas del día cuya aula informada tiene aforo > 1
        para cada segmento, para k = 0; k + 1 < segmento.size(); k += 2:
            a ← segmento[k]; b ← segmento[k+1]
            si canchaSlots.contains(a) != canchaSlots.contains(b):
                conflicto «El curso «%1» ocupa la cancha en una sola franja del bloque %2–%3 (día %4).»
```

Con aforo 1 en todas las aulas (el resto del sistema) el bloque no añade nada: `canchaSlots` queda vacío.

### 3.6 Seed

- **Guarda del tipo** en `validarPerfil` (paso 4): la cancha se identifica por su **entrada declarada y su
  nombre** (`«Cancha»`), y su tipo se clasifica por **clave**:
  - si no existe el aula llamada `aforo_aula::tipoCancha()` → falla («El perfil debe declarar el aula
    «Cancha»»);
  - si su tipo tiene clave distinta de `cancha` → falla con **tipo recibido**, **clave esperada** y
    **acción**: `La cancha «Cancha» declara el tipo «%1» (clave «%2»); la clave esperada es «cancha»: el
    tipo de la cancha debe llamarse «Cancha». Corrija el tipo del aula o del catálogo.`
  - generales y canchas se cuentan también por clave (`tipo_general::clave()` / `claveCancha()`), no por
    igualdad de texto.
- **`capacidadCancha(perfil)`** (pura): `aulas`, `aforo`, `franjasPorTurno` (5 × `numSlots` = 40),
  `plazasPorTurno` (40 × 2 = 80), `plazasSemana` (80 × 2 turnos = 160), `posicionesParPorTurno` (4 × 5 =
  20), `plazasParPorTurno` (40), `paresEfPorTurno` (24), `clasesEfPorTurno` (48), `clasesEfSemana` (96).
  Las posiciones **no** se recalculan: `perfil.cpp` construye un `TurnoConfig` (`nombre`, `slot =
  0…numSlots-1`, `inicio`, `fin`) y un `EventoConfig` por evento (`turno`, `despues_de_slot =
  EventoDef.despuesDeSlot`, `orden`, `duracion`) desde `seed::TurnoDef/EventoDef` y llama a
  `posicionesDePar(turno, eventos)`; el ⌊|segmento|/2⌋ existe **una sola vez**.
- **Tipo de la cancha por clave.** `tipoEspecializadoDeMateria`/`tipoFinalDeMateria` dejan de comparar el
  texto `"Cancha"`: buscan en `tiposDeAulas` el **nombre declarado cuya clave es `cancha`**
  (`aforo_aula::claveCancha()`) y lo devuelven; si no hay ninguno, la materia de Educación Física cae al
  tipo general (el conjunto no tiene dónde impartirla y la aplicación falla por la guarda). Así un tipo
  llamado «CANCHA» o «cancha » funciona y `sembrador.cpp` sigue casando `idTipoAula` por nombre.
- **`motivoNoGenerable`** usa esa cuenta: 48 > 80 es falso en ambos turnos ⇒ `""`.
- **`resumenTexto`** añade, con las cifras calculadas:

```text
Cancha: 1 aula · 40 franjas por turno · aforo 2 → 80 plazas por turno · 160 en la semana
Educación Física: 48 clases por turno · 96 en la semana → 24 pares por turno · 48 en la semana
Pares de cancha: 20 posiciones por turno · 40 plazas de par → 24 ≤ 40 por turno
```

y la línea final pasa a `Generable: sí`.

---

## 4. Pares completos: por qué el par cruzado es imposible

Notación de la 014: `ocupada[c][d][s]` (el curso tiene clase en la franja), `par[c][d][s]` (empieza un
bloque en `s`), `y[c][d][s][a]` (aula real).

1. **R9 (huecos al final)** impone `ocupada[s] ≥ ocupada[s+1]` sobre las franjas ordenadas del turno ⇒ las
   franjas ocupadas de un curso y un día son un **prefijo** del turno.
2. **R2 (emparejamiento)** impone `ocupada[s] = par[s] + par[s−1]` dentro de cada segmento ⇒ (i) el número
   de franjas ocupadas de cada segmento es **par** y (ii) un prefijo par solo se cubre con pares que
   empiezan en los índices **0, 2, 4…** del segmento (en la primera franja libre `par` queda a 0 y la
   igualdad obliga a que el último par empiece en la franja anterior).
3. **R7 (misma aula del par)** impone `par[s] ⇒ y[s][a] = y[s+1][a]` ⇒ las dos franjas de un bloque están
   **siempre en la misma aula**; por tanto la presencia de un curso en la cancha es «par entero o nada».

De (1)+(2), **todos** los pares de todos los cursos empiezan en las mismas posiciones (0, 2, 4… del
segmento). De (3), un curso en la cancha ocupa siempre un par completo. Dos cursos solo pueden coincidir,
pues, en la **misma posición de par**, nunca en una franja suelta: el **par cruzado no es una solución**.

Comprobaciones (criterio 3, dos capas):

- **Comprobación de modelo (T2)**: `Solver::model` y `Solver::vars` son **privados**, así que el test
  **construye el modelo a mano**, como `test_solver_bloques.cpp:44-52`: `crearVariables(modelo, config)` +
  `agregarRestriccionesTurnos/Aulas/Profesores/Bloques` y un `CpSolver` con `num_search_workers = 1` y
  `random_seed = 0`. Sobre ese modelo se **fuerza** con `AddEquality(x[{p, m, c, d, s}], 1)` el bloque del
  curso: en las franjas **3 y 4** de un segmento que empieza en 0 → **INFEASIBLE** (R9 exige ocupada[2] ≥
  ocupada[3] = 1); en **0 y 1** con un segundo curso en 0 y 1 → **FEASIBLE** con dos clases en la cancha.
  Vía alternativa, si el forzado resultara frágil: **forzar por configuración** (un turno de solo dos
  franjas o una disponibilidad que deje una única posición) y comprobar el estado; el test elige una y lo
  documenta.
- **Comprobación de salida (T3)**: `comprobarParesCancha` marca cualquier franja de cancha sin su pareja.

**Dependencia declarada:** si una spec futura relajara R9 deja de haber demostración. Queda escrito en
`plan.md` y en el journal; un cambio así es cambio de requisitos.

---

## 5. Funciones puras (resumen de firmas)

| Función | Entrada | Salida | Notas |
|---|---|---|---|
| `aforo_aula::aforoDeClave(clave)` | clave normalizada | 2 / 1 | Sin E/S; sin reloj |
| `aforoDelTipo(clave)` | clave | 2 / 1 | Envuelve la anterior (una sola verdad) |
| `aforoDelAula(config, índice)` | config + índice | 2 / 1 | Índice inválido → 1 |
| `posicionesDePar(turno, eventos)` | turno + eventos | entero | Σ ⌊|segmento|/2⌋ |
| `capacidadDelTipo(config, clave)` | config + clave | entero | aulas × franjas × aforo |
| `seed::capacidadCancha(perfil)` | perfil | `CapacidadCancha` | Toda la aritmética del informe |
| `seed::motivoNoGenerable(perfil)` | perfil | QString | `""` para el centro |
| `analizarSalidaSolver(horario, config)` | salida + config | `AnalisisSalida` | Aforo y pares cruzados |

El **tiempo** no interviene en ninguna: el aforo no depende del reloj (RNF-3).

---

## 6. Decisiones y alternativas descartadas

| # | Decisión | Alternativa descartada y por qué |
|---|---|---|
| E1 | El aforo vive en `common/aforo_aula.hpp` y lo consumen contrato, modelo, salida y seed | Poner el literal y el 2 en cada consumidor: cuatro verdades que se desincronizan (D-1). Campo en el contrato: esquema v7, presets y huella (D-8). |
| E2 | Restricción genérica «≤ aforo» por (día, franja, aula) | Caso especial «si el aula es la cancha, dos»: duplicaría la regla y no valdría para otras aulas. `AtMostOne` global: impide la simultaneidad (situación actual). |
| E3 | Pares completos **por demostración** (R2 + R7 + R9) y **una comprobación de modelo** | Confiar en la alineación sin probarla: si alguien relaja R9, el modelo emitiría pares cruzados en silencio. Añadir una restricción redundante por posición de par: coste en variables sin cambio semántico (se puede añadir si el test revelara un hueco, y el plan deja la puerta abierta). |
| E4 | Capacidad multiplicada por el aforo del tipo | Dejar la capacidad sin aforo: la causa (2) diría «48 > 40» para la cancha y contradiría el informe de la 017 (D-7). |
| E5 | Salida por **conteo** con aforo por aula y 1 por defecto | Mantener el mapa «clave → curso»: la segunda clase siempre es conflicto y un horario válido se marcaría como fallo (el mismo error que la 010 corrigió en P4). |
| E6 | Guarda del tipo **en el seed** y fallo del motor por la validación vigente | Añadir una V20 al contrato: tocaría V1–V19 y la constitución §7.4 sin necesidad; el motor ya rechaza un tipo requerido sin aulas y el seed es quien aporta el tipo literal (D-3, D-11). |
| E7 | Aceptación lenta que **complementa** el fixture sin EF | Sustituirlo: perderíamos la comparación de rendimiento y la cobertura del escenario 848; retirarlo contradice la 014 (D-10). |
| E8 | `AddLessOrEqual(Sum(...), tope)` | `AddAtMost(vector, int)`: misma semántica, pero el plan evita depender de esa sobrecarga concreta. |

---

## 7. Aceptación lenta (T5)

> **Orden (2026-11-10):** T5 se ejecuta **después de T4b**, que añade P033. Antes de T4b, el centro real
> era `INFEASIBLE` por la plantilla (una sola docente de idiomas con 48 clases en 40 franjas), no por el
> aforo.

`test/frontend/test_solver_centro_real.cpp` (Qt Test, lanza `frontend`, `seed`, `backend_solver`, `Qt6::Sql`;
registro dentro de `if(TARGET frontend AND TARGET backend)` + `if(TARGET seed AND TARGET backend_solver)` con
`set_tests_properties(test_solver_centro_real PROPERTIES LABELS "slow")`).

1. Siembra en `QTemporaryDir`, abre `NucleoDatos`.
2. Compone la entrada con `GeneracionSolver::construirEntrada` y los 24 cursos del perfil (plan, turno y
   matrícula), igual que `test_entrada_seed`.
3. `validarEntradaSolver` (V1–V19) y `Solver::resolver(config)` con los valores por defecto (8 workers,
   `PresupuestoFases` 60/20/10).
4. Asserts (dos informes distintos, cada uno con su cabecera):
   - `#include "backend/services/analisis_bloques.hpp"` → `analizarBloques(horario, config)`:
     `exito`; `metadata.total_asignaciones == 876`; `analisis.desvio == 0`; `analisis.bloquesDeUno == 0`;
     `analisis.huecosNoFinal == 0`;
   - `#include "backend/services/ValidadorSalidaSolver.hpp"` → `analizarSalidaSolver(...).presentable()`;
   - **96** asignaciones de Educación Física y **todas** en el aula de clave `cancha`;
   - ninguna aula con más clases por franja que su aforo; **ningún par cruzado** (recálculo propio);
   - `tiempo_ms < 300000` en Release (`< 900000` en Debug) y se imprime la medición.
5. La aceptación **sin EF** de la 014 se mantiene: no se toca `test_solver_centro_sin_ef.cpp`.

Comando: `ctest --preset slow-release` (requiere configurar/compilar `full-release` antes).

---

## 8. Documentación y normativa

- **Contrato** (`docs/contrato-solver-2.0.md`): capacidad con aforo, las cuatro cifras de la causa (2),
  P4 por conteo con el aforo y la frase «el aforo es regla del motor y **no** viaja en el contrato».
- **Guías de datos** (las dos): el conjunto **se declara generable** con 48 ≤ 80, 96 ≤ 160 y 24 ≤ 40
  plazas de par; se explica el aforo de cursos frente a los 88 alumnos; **fuera toda promesa de aforo
  pendiente**. Líneas a reescribir (verificadas el 2026-11-10):
  - `docs/guias/datos-prueba.md`: **10** (nota de cabecera), **156** (`Generable: no — … aforo 1`),
    **202-204** (apartado «El conjunto no es generable»), **208** («aforo simultáneo pendiente») y
    **210-211** (aceptación diferida).
  - `docs/guias/datos-prueba-frontend.md`: **163-167** («no es generable» y deuda del aforo), **170**,
    **198-199** (checklist «Generación diferida») y **219** (tabla «no generable»).
  Tras el cambio, `grep -n "aforo\|generable"` en las dos guías **solo** puede devolver líneas que
  hablen del **aforo 2** o de `Generable: sí`.
- **Interfaz**: sin pantallas nuevas; se retira la deuda del `NoFactible` «en la vista» y el aforo
  pendiente (mencionados en `docs/interfaz-frontend.md`). La **vista de horario** (pintar dos cursos en la
  misma aula y franja) queda **fuera de alcance**: no hay cambios en `src/frontend/src`.
- **Constitución** (la escribe el **planner**, T7): §7.1.3 («ni un aula tiene más clases a la vez que su
  aforo»), §7.1.5 («la exclusividad rige también las desplazadas, hasta el aforo»), §7.3 (qué declara un
  aula; el aforo no viaja) y §7 «Comportamiento» (capacidad con aforo y causa (2) con las cuatro cifras).
  **§7.4 no se toca**: no hay validación nueva. La revisión es **a mano** (la constitución no usa
  encabezados `§` literales que se puedan grepear; el `grep` solo confirma que aparece «aforo» en las
  secciones).
- **Supersesiones fechadas** (también del **planner**, T7): 005 (D8), 010 (D8, capacidad, causa (2),
  comprobación de salida), 014 (la cancha compartida pasa a exigible) y 017 (RF-9/RF-10 y la aceptación
  diferida superadas). El **implementer no edita** `docs/constitution.md` ni `specs/`; solo **verifica**. 

---

## 9. Verificación por tarea

| Tarea | Comando exacto | Resultado esperado |
|---|---|---|
| T1 | `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend` | Compila y **toda** la suite de `backend` en verde, incluidos los casos nuevos de `test_capacidad_tipo` |
| T2 | `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend` | Verde; `TestSolverAforo` pasa (2 cursos factible, 3 en la misma franja infactible, par cruzado forzado infactible, control positivo y un solo docente que no coincide) |
| T3 | `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend` | Verde; `test_validador_salida` con los casos de aforo y pares cruzados |
| T4 | `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend` y `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release` | Ambas verdes; los tres tests del seed afirman `Generable: sí` y las cifras nuevas |
| T4b | `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend` y `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release` | Ambas verdes con **33 docentes / 165 disponibilidades / 272 asignaciones**; `gestor-horarios-seed` imprime `Generable: sí` |
| T5 | `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset slow-release` | **Después de T4b**: `test_solver_centro_real` verde con `876` clases y `tiempo_ms < 300000`; el lento sin EF de la 014 sigue verde |
| T6 | `grep -rn "no generable\|no es generable\|aforo pendiente\|aforo simultáneo\|aforo 1\|Generable: no" docs/guias/datos-prueba.md docs/guias/datos-prueba-frontend.md docs/interfaz-frontend.md` y `grep -n "aforo\|generable" docs/guias/datos-prueba.md docs/guias/datos-prueba-frontend.md` | La primera **no** devuelve nada; la segunda solo muestra líneas con **aforo 2** o **Generable: sí**; `grep -n "aulas × franjas × aforo" docs/contrato-solver-2.0.md` muestra la fórmula |
| T7 | **Tarea del planner** (no del implementer). El implementer solo verifica: `grep -n "aforo" docs/constitution.md` y `grep -n "018" specs/005-nucleo-solver/spec.md specs/010-solver-tipos-y-eventos/spec.md specs/014-bloques-de-dos-horas/spec.md specs/017-datos-reales-del-centro/spec.md` | §7.1.3/§7.1.5/§7.3/§7 con aforo (revisión **a mano**); las cuatro specs con su nota de supersesión |
| T8 | las tres suites (`backend`, `full-release`, `slow-release`) y `git status --short -- src test docs` | Todo verde; el diff de **`src/`, `test/` y `docs/`** se limita a los archivos de §2 (sin `src/backend/src/database/` ni `src/frontend/src/`). `specs/018-…/` está **sin seguimiento** y queda **fuera** del control (incluidos `tasks.md` y `journal.md`) |

---

## 10. Trazabilidad RF → tarea → verificación

| RF | Tarea(s) | Verificación |
|---|---|---|
| RF-1 | T1, T2, T3 | `aforoDelTipo("Cancha") == 2` y `("Aula de estudio") == 1`; dos cursos en la cancha factible; tercero infactible; `test_validador_salida` sin conflicto con dos y con conflicto con tres |
| RF-2 | T2, T4 | `test_solver_horarios`/V17: materia que exige cancha sin aulas → entrada inválida con materia y tipo; `test_seed_perfil` con el tipo renombrado → fallo con tipo recibido, clave esperada y acción |
| RF-3 | T2 | Dos cursos con docentes distintos comparten la cancha (factible, dos clases en la franja); con **un solo docente candidato** para dos cursos, el modelo no las coloca en la misma franja (una de ellas cae en otra posición o la instancia no cabe) |
| RF-4 | T2 | Suites de la 014 y `test_solver_bloques` verdes; ningún test de tipo/referencia/bloqueo cambia |
| RF-5 | T1, T2, T3 | `posicionesDePar` = 4/día, 20/turno; par cruzado forzado **infactible** (modelo) y **conflicto** (salida) |
| RF-6 | T1, T4 | En T1, la fórmula `aulas × franjas × aforo` sobre la config de prueba y la causa (2) con demanda, aulas, franjas y aforo (precedencia de (3) intacta); en T4, la cancha del centro da **160** (80 franjas × aforo 2) |
| RF-7 | T3 | Dos clases en cancha: sin conflicto; tres: conflicto; dos en aforo 1: conflicto; aula inexistente: aforo 1; clase sin aula: fuera de la cuenta |
| RF-8 | T4, T4b | `motivoNoGenerable` vacío; informe con `Cancha: … aforo 2`, `80 plazas por turno`, `160 en la semana`, `24 ≤ 40` y `Generable: sí`; **T4b** con **33 docentes / 165 disponibilidades / 272 asignaciones** |
| RF-9 | T5 | `test_solver_centro_real`: 876 clases, EF en la cancha, desvío 0, huecos al final, `< 300000` ms |
| RF-10 | T1, T3, T5 | El texto de la causa (2) cambia de cifras, no de categoría; una instancia no explicable sigue dando «no atribuible» |
| RF-11 | T6, T7 | `grep` de §8 y revisión de contrato, guías e interfaz |
| RNF-1 | T5 | `< 300000` ms |
| RNF-2 | T1, T2, T3, T8 | Suites completas verdes; los tests de aforo 1 no cambian |
| RNF-3 | T1, T3 | Dos ejecuciones de las funciones puras devuelven lo mismo; `posicionesDePar` no depende del reloj |
| RNF-4 | T1, T3, T4 | Mensajes con tipo/aula y cifras (asserts de texto) |
| RNF-5 | T4b, T5 | Los recuentos del seed quedan en **85/24/33/13/876** con **272 asignaciones** y **165 disponibilidades** (corrección de la 017, P033) y la base no cambia de esquema |
| RNF-6 | Todas | Esta tabla |
| RNF-7 | T5, T8 | `test_solver_centro_real` con `LABELS "slow"`; la suite normal no lo ejecuta; todo lo demás verde |
| RNF-8 | T1, T4 | `SolverConfig::toJson` de una config con cancha **no** contiene `aforo` (T1); el preset compuesto del centro valida y hace ida y vuelta igual en `test_entrada_seed` (T4) |

Ningún RF queda sin tarea ni verificación.

---

## 11. Tests existentes que cambian (y en qué tarea)

| Test | Cambio | Tarea |
|---|---|---|
| `test_capacidad_tipo.cpp` | Casos nuevos de `aforoDelTipo`/`aforoDelAula`/`posicionesDePar`, capacidad con aforo y `toJson` sin `aforo`, sobre un **`configConCancha()` aparte** (no se toca `configBase()`: sus **4** aserciones exigen `franjasDelTipo(«Cancha»)` vacía) | T1 |
| `test_solver_horarios.cpp` | Sin cambios de aserción: «10 bloques», «8 franjas» y ««Laboratorio»» siguen apareciendo en el mensaje nuevo | T1 |
| `test_validador_salida.cpp` | Mensaje de aula con nº de clases y aforo; casos nuevos de cancha (2 válido, 3 conflicto) y par cruzado | T3 |
| `test_seed_perfil.cpp` | `motivoNoGenerableExplicaLaCancha` → `motivoNoGenerable` vacío; nuevos casos de la guarda del tipo; `capacidadCancha` | T4 |
| `test_seed_aplicacion.cpp` | `resumenTexto_…YNoGenerable` → `…YGenerable`; el informe tras aplicar deja de citar el aforo pendiente | T4 |
| `test_entrada_seed.cpp` | `Generable: no` → `Generable: sí` con las cifras; confirma el **ida y vuelta del preset** (RNF-8); sigue sin resolver | T4 |
| `src/seed/src/docentes.cpp` + los tres tests del seed | **T4b** (corrección de la 017): **P033** (Idiomas) y **P028** (Inglés); recuentos **33 docentes / 165 disponibilidades / 272 asignaciones**; la cobertura y `Generable: sí` se mantienen | T4b |
| `test/CMakeLists.txt` | `test_solver_aforo` (T2) y `test_solver_centro_real` con `LABELS "slow"` (T5) | T2, T5 |
| `test_solver_bloques.cpp`, `test_solver_centro_sin_ef.cpp` | **Sin cambios** (aforo 1); se comprueba que siguen verdes | T2, T8 |

---

## 12. Riesgos y umbrales

| # | Riesgo | Umbral / mitigación |
|---|---|---|
| 1 | **Tiempo**: el centro pasa de 848 a **876** clases; la 014 midió **172,6 s** con 848 | Techo **300 s** en T5 (Release); se imprime la medición. Si se supera, **se para y se informa**: el rendimiento es otra spec; **no** se recorta alcance por la espalda |
| 2 | `max_deterministic_time` no es reloj de pared: el tiempo varía por host | Se mide en Release y se deja margen; el límite es el del contrato (300 s), no el del test |
| 3 | **Demostración de D-5** apoyada en R9 | Si la comprobación de modelo fallara, se añade la restricción explícita por posición de par (E3) en T2, sin cambiar la spec |
| 4 | **Suite**: los recuentos cambian | RNF-7: no son requisito; lo exigido es todo verde y sin degradación |
| 5 | El seed declara «Generable: sí» **antes** de la prueba de T5 | Orden T4→T5 dentro de la misma spec; si T5 falla, el informe es incorrecto y la spec **no** se cierra (se informa y se decide) |
| 6 | El aforo depende del **nombre** del tipo | Renombrar «Cancha» lo desactiva; la guarda del seed y el mensaje lo hacen explícito; en datos existentes, aforo 1 y causa (2) con el nombre visible |
| 7 | **Contrato/huella**: cualquier filtro del aforo al JSON rompería presets | Test de `toJson` sin `aforo` (T1) y preset del centro que valida e ida y vuelta en `test_entrada_seed` (**T4**) |
| 8 | **Debug** con OR-Tools en Windows | T5 se mide en Release; en Debug el techo del test es mayor y no es la medida de referencia |
| 9 | Aulas fuera de rango o clases sin aula en una salida guardada | Aforo 1 y fuera de la cuenta (RF-7); tests en T3 |
| 10 | El validador necesita los segmentos del curso | Si la clave del curso no está en la config, se omite el chequeo de pares (no se inventa turno); el aforo se comprueba igual |

---

## 13. Tamaño y aceptación del exceso

| Tarea | Estimación | ¿Supera 30 min? |
|---|---|---|
| T1 | ~30 min | no |
| T2 | ~45 min | **sí** |
| T3 | ~35 min | **sí** |
| T4 | ~40 min | **sí** |
| T5 | ~50 min | **sí** (incluye la ejecución lenta, ~3–5 min de reloj) |
| T6 | ~25 min | no |
| T7 | ~25 min (**planner**, no implementer) | no |
| T8 | ~20 min | no |

**Se pide al usuario la aceptación del exceso de T2, T3, T4 y T5.** Alternativa de corte si prefiere
tareas ≤ 30 min: 018a (T1–T4) y 018b (T5–T8). Ninguna tarea cierra con build o suite en rojo.
