# Plan 005 — Núcleo del solver y contrato de configuración

> **OBSOLETO — pendiente regenerar tras el cambio de modelo.** Este plan responde al contrato anterior
> (`aula_fija` y materias por curso, sin planes embebidos). Tras el cambio de modelo (2026-10-15) la
> spec 005 volvió a **borrador**; regenerar este plan (y redactar `tasks.md`) cuando la spec se
> apruebe.

Estado: obsoleto (superseded por el cambio de modelo; ver la spec 005 en borrador).
Spec de referencia: `specs/005-nucleo-solver/spec.md` (aprobada, sin dudas abiertas).
Constitución: `docs/constitution.md` §3 (separación lógica/UI), §4 (tests primero), §5 (protección de datos), §6 (idioma), §7 (contrato del solver).
Specs relacionadas: `specs/004-persistencia-campos-curso/spec.md` (**prerrequisito**, precede a esta spec y aporta los campos de curso persistidos en el dominio/DTO); `specs/002-generacion-horarios-ipc/spec.md` (su contrato JSON queda superado para los campos de curso y del modelo).

> Este documento describe el CÓMO. No introduce dependencias nuevas: C++17 · Qt6 (Core/Sql) · OR-Tools CP-SAT · CMake/Ninja · CTest + Qt Test / Google Test.
> Nota de alcance: la **interfaz Qt** (`src/frontend`) y la **persistencia de los campos nuevos** (spec 004) están fuera de esta spec. Aquí solo se documenta el **contrato que la interfaz consume** (§6), sin tocar `src/frontend`.

> Numeración: el plan cubre todos los RF de la spec (RF-1…RF-12), con numeración continua.

---

## 0. Frontera y capas (constitución §3, §7)

- **`backend_solver`** (OR-Tools): única pieza que decide el horario. Recibe `SolverConfig` ya validada y devuelve `HorarioSalida` o un fallo con causa. No abre la base de datos ni depende de Qt Widgets.
- **`backend`** (Qt6 Core/Sql, sin OR-Tools): alberga el **contrato de configuración** (`SolverConfig`) y su serialización, las **funciones puras** del contrato y la **validación previa**. La entrada se valida aquí.
- **`src/frontend`** (otro equipo): pinta el horario; traduce índice de aula→nombre. **No se modifica** (`docs/interfaz-frontend.md` documenta el contrato).

```
      datos reales + preset                 SolverConfig (validada)                HorarioSalida (JSON)
 BD ──► ConstructorEntradaSolver ──► SolverConfig::fromJson (V1–V15) ──► Solver ──► extraerSolucion ──► HorarioSalida
        (backend; campos persistidos)    (backend)                         (backend_solver)   (backend_solver)
```

---

## 1. Archivos y responsabilidades

### 1.1 `backend` — contrato de configuración, funciones puras y validación

| Archivo | Acción | Responsabilidad | RF |
|---|---|---|---|
| `src/backend/include/backend/solver/config/contrato_solver.hpp` + `src/backend/src/solver/config/contrato_solver.cpp` | **nuevo** | Funciones puras y constantes del contrato: `bloques(horas, duracion_minutos)`, `bloquesAula(horas_aula, duracion_minutos)`, `costeSuplente(peso, coste_maximo)`; constantes `PESO_HORAS` y `COSTE_MAXIMO_SUPLENTE`. **Sin `nombreCurso`**: el solver no deriva nombres. Sin estado, sin E/S, sin OR-Tools. | RF-1 (D3), RF-5 (S1/S2) |
| `src/backend/include/backend/solver/config/solver_config.hpp` | **modificado** | `CursoSolverConfig`: se sustituye `nombre` por `anio` (int) y `seccion` (QString); `aula_fija` pasa a `bool` con `aula` (índice, `-1` = ninguna). `ProfesorSolverConfig`: añade `anios` (QVector<int>); `horas_requeridas` pasa a **opcional** (`-1` = ausente). Se conserva `MateriaSuplente.peso`. | RF-7, RF-8, RF-10 |
| `src/backend/src/solver/config/solver_config.cpp` | **modificado** | `fromJson`: parsea `anio`/`seccion`, `aula_fija` (booleano) + `aula` (índice), `anios` del docente y `peso` por par; ignora `nombre` si llega; ejecuta V1–V15. `toJson`: escribe el contrato nuevo **sin** `nombre`, agrupando por año/sección. Ida y vuelta sin pérdida. | RF-7, RF-8, RF-10, RF-11 |
| `src/backend/include/backend/services/ValidadorEntradaSolver.hpp` + `.cpp` | **sin cambios** | Fachada de la validación previa; el detalle vive en `SolverConfig::fromJson`. | RF-11 |
| `src/backend/src/services/ValidadorSalidaSolver.cpp` + `src/backend/include/backend/services/ValidadorSalidaSolver.hpp` | **modificado** | `indiceCurso` casa por la **identidad del curso** (`anio`+`seccion`), no por un nombre almacenado ni derivado. Ajusta P1/P2/P4 al contrato nuevo. | RF-3, RF-9 |
| `src/backend/src/services/ConstructorEntradaSolver.cpp` | **modificado** | Emite el contrato nuevo leyendo los **campos persistidos** del `CursoDTO` (los aporta la spec 004): `anio`/`seccion` (sin nombre) y `aula_fija` bool + `aula` índice. | RF-7, RF-8, RF-10 |
| `src/backend/CMakeLists.txt` | **modificado** | Añade `src/solver/config/contrato_solver.cpp` al target `backend` (junto a `solver_config.cpp`). | — |

### 1.2 `backend_solver` — variables, restricciones, objetivo y salida

| Archivo | Acción | Responsabilidad | RF |
|---|---|---|---|
| `src/backend/include/backend/solver/variables.hpp` + `src/backend/src/solver/variables.cpp` | **modificado** | Crea `assignment[p,m,c,d,s]` (como hoy, aplicando elegibilidad por plan/turno/**año**/disponibilidad), `aulaAssignment[c,d,s,a]` (solo cursos móviles) y la **nueva** variable de activación `asignacionDocente[p,c,m]` (docente asignado a la materia-curso). | RF-1 (D1/D2), RF-3 (D9), RF-4 (D6/D7/D13) |
| `src/backend/src/solver/restricciones/restricciones_profesores.cpp` + `.hpp` | **modificado** | Conserva D5 (profesor no solapado); añade `restriccionUnDocentePorMateria` (D1+D2+D3 por `asignacionDocente`) y `restriccionDocentePorAnio` (D13; D6/D7 ya van por poda en variables). | RF-1, RF-2 (D5), RF-4 (D13) |
| `src/backend/src/solver/restricciones/restricciones_turnos.cpp` + `.hpp` | **modificado** | Conserva D4 (curso no solapado); reescribe `restriccionBloquesRequeridos` (D3 con `bloques()`); **sustituye** la igualdad dura `restriccionHorasSemanalesProfesor` por `restriccionTopeHorasProfesor` (S1: tope + exceso). | RF-1 (D3), RF-2 (D4), RF-5 (S1) |
| `src/backend/src/solver/restricciones/restricciones_aulas.cpp` + `.hpp` | **modificado** | Conserva D8 (aula no solapada) **incluyendo cursos de aula fija**; **elimina** `restriccionMateriaUnicaPorAula` (D12 suprimida); añade `restriccionCursoOcupaUnAula` (D9: enlace clase↔aula). | RF-2 (D8), RF-3 (D9/D10) |
| `src/backend/include/backend/solver/solver.hpp` + `src/backend/src/solver/solver.cpp` | **modificado** | Construye el modelo, añade las restricciones, fija la **función objetivo** (S1+S2) y parámetros deterministas; extrae la solución indexando por la **identidad del curso** (`anio`+`seccion`) y con el **aula como índice**; mensajes de fallo en español. | RF-3, RF-5, RF-6, RF-9, RNF-1, RNF-3 |
| `src/backend/src/solver/servicio_solver.cpp` | **sin cambios** | Adapta JSON→`SolverConfig`→`Solver`→respuesta. | RF-6 |

---

## 2. Funciones puras de lógica

Regla de pureza (constitución §4 y política del proyecto): **nada lee el reloj ni el entorno**; el instante se **inyecta como parámetro** (`ahora`). Las funciones del contrato no dependen del tiempo, así que **no reciben reloj** (añadirlo sería un parámetro falso): son puras y deterministas por construcción.

| Función | Firma | Semántica | RF | ¿Reloj? |
|---|---|---|---|---|
| `bloques` | `int bloques(int horas, int duracion_minutos)` | `ceil(horas × 60 / duracion_minutos)`; 0 si `duracion_minutos ≤ 0`. Conversión de horas de materia a bloques (D3). | RF-1 | no |
| `bloquesAula` | `int bloquesAula(int horas_aula, int duracion_minutos)` | `ceil(horas_aula × 60 / duracion_minutos)`; mismo `duracion_minutos` que `bloques` (S1). | RF-5 | no |
| `costeSuplente` | `int costeSuplente(int peso, int coste_maximo)` | `max(1, coste_maximo − peso)` para `peso ≥ 0`: decreciente, estrictamente `> 0`, `≤ coste_maximo`, `peso = 0 → coste_maximo`. | RF-5 (S2) | no |
| `construirJsonEntrada` | `QJsonObject construirJsonEntrada(const DatosDominio&, const SolverConfig&, const QDateTime& ahora)` | Pura (sin E/S); estampa `meta.fecha_modificacion` con el **instante inyectado**. | RF-7, RF-8, RF-10 | **sí (`ahora`)** |
| `estamparFechaGeneracion` | `HorarioSalida estamparFechaGeneracion(const HorarioSalida&, const QDateTime& ahora)` | Pura; copia el horario y fija `metadata.fecha_generacion` desde el instante inyectado. | RF-10 | **sí (`ahora`)** |
| `huellaDatos` | `QString huellaDatos(const DatosDominio&)` | Pura; huella canónica SHA-256 del snapshot (sin datos sensibles, constitución §5). | RF-6 (reproducibilidad de la detección de cambios) | no |

Constantes del motor (enteras, en `contrato_solver.hpp`): `PESO_HORAS = 1000`, `COSTE_MAXIMO_SUPLENTE = 100`, con `PESO_HORAS > COSTE_MAXIMO_SUPLENTE > 0` (constitución §7.2, RF-5).

---

## 3. Modelo CP-SAT: variables, restricciones y objetivo (pseudocódigo)

### 3.1 Variables

```
para cada curso c, materia m de c, sección/sección y docente p candidato de m:
    si p.plan no vacío y c.plan ≠ p.plan            → descartar (D7)
    si c.turno no vacío y p.turno no vacío y c.turno ≠ p.turno → descartar (D6, ver nota)
    si p.anios no vacía y c.anio ∉ p.anios          → descartar (D13)
    para cada día d y slot s ∈ (turno(c) ∩ disponibilidad(p)):
        x[p,m,c,d,s] = BoolVar()
    z[p,c,m] = BoolVar()                    # activación "p dicta la materia-curso"
para cada curso móvil c, día d, slot s ∈ turno(c), aula a:
    y[c,d,s,a] = BoolVar()                  # D9/D10
para cada docente p:
    e[p] = IntVar(0, ∞)                     # exceso de bloques sobre su tope (S1)
```

> Nota D6: la spec exige que un docente no dé clase de cursos de otro turno solo «si tiene turno definido». En la práctica el turno del docente restringe su disponibilidad; se implementa por **poda** (coherente con `crearVariables` actual) y, si hiciera falta, con `x = 0` explícito. Docente sin turno → sin restricción (D6 condicional).

### 3.2 Restricciones duras (RF-1…RF-4)

```
# D1 + D2 + D3 — un docente por materia-curso, solo él la dicta, con sus horas exactas
para cada (c, m):
    Σ_p z[p,c,m] == 1
    para cada p: Σ_{d,s} x[p,m,c,d,s] == bloques(horas_m, duracion_minutos) · z[p,c,m]

# D4 — un curso no tiene dos clases a la vez
para cada (c,d,s): Σ_{p,m} x[p,m,c,d,s] ≤ 1

# D5 — un profesor no tiene dos clases a la vez
para cada (p,d,s): Σ_{m,c} x[p,m,c,d,s] ≤ 1

# D8 — un aula no aloja dos clases a la vez (fijas + móviles)
para cada (a,d,s):
    Σ_{c móvil} y[c,d,s,a] + Σ_{c fijo con aula=a} Σ_{p,m} x[p,m,c,d,s] ≤ 1

# D9 — un curso móvil ocupa exactamente un aula cuando tiene clase
para cada (c móvil, d, s): Σ_a y[c,d,s,a] == Σ_{p,m} x[p,m,c,d,s]

# D10 — aula fija (RF-3): no hay variable; la extracción declara c.aula (ver §4)

# D11/D6/D7/D13 — se garantizan por poda (3.1); D13 además reforzada por la poda por año
```

### 3.3 Función objetivo (RF-5)

```
Minimizar(
    PESO_HORAS · Σ_p e[p]                                    # S1: exceso de horas de aula
  + Σ_{(p,c,m): p es suplente de m y no titular} costeSuplente(peso(p,m), COSTE_MAXIMO_SUPLENTE) · z[p,c,m]   # S2
)

con  Σ_{m,c,d,s} x[p,m,c,d,s] ≤ bloquesAula(p.horas_aula, duracion_minutos) + e[p]   # tope + exceso
```

- Un **titular** no aporta coste (coste 0). `peso` 0 → coste máximo entre suplentes (RF-5).
- La regla «un suplente solo cubre cuando ningún titular puede» **emerge** de la optimización.
- **Reproducibilidad (RNF-1):** `SatParameters` con `num_search_workers = 1` y `random_seed = 0`; los empates de `peso` se resuelven de forma determinista. La misma entrada produce el mismo resultado.
- Nunca se relaja una restricción dura para mejorar el objetivo (RF-5).

---

## 4. Extracción de la solución y contrato de salida (RF-3, RF-6, RF-9, RNF-3)

```
para cada (p,m,c,d,s) con x = true:
    aula = (c.aula_fija ? c.aula : índice a con y[c,d,s,a] = true)   # D10 / D9
    añadir AsignacionOutput{slot=s, materia=m, profesor=p, aula=aula} al día d del curso
clave del curso = identidadCanonica(c.anio, c.seccion)   # p. ej. "1|A": identidad de datos, NO el nombre derivado
agrupar horarios[clave]; ordenar días y slots de forma estable
metadata: total_asignaciones, cursos_generados, profesores_asignados (|{p}|), tiempo_ejecucion_ms
```

- `AsignacionOutput.aula` es siempre un **índice** del pool de aulas (RF-9); la traducción a nombre es de la interfaz.
- **Clave del curso = identidad `anio`+`seccion`** (canónica y determinista, p. ej. `"1|A"`), nunca el nombre almacenado ni el nombre derivado. El nombre derivado (`"1ro A"`) lo calcula **quien muestra** (frontend/consola) a partir de la identidad; el solver no lo produce (RF-3, RF-9).
- Fallo: `INFEASIBLE`/`MODEL_INVALID`/`UNKNOWN` devuelven mensajes en español con la **causa** (RNF-3), sin inventar datos (RF-6). No se completa ni corrige la entrada.

---

## 5. Validación previa V1–V15 (RF-11)

Se ejecuta en `SolverConfig::fromJson`, antes de invocar al solver; si alguna falla, **no se invoca**.

- **V1–V12 (se conservan):** estructura/tipos, dimensiones, índices en rango, turnos válidos, **V8 aula fija válida**, `num_estudiantes`, listas no vacías, un plan por curso, disponibilidad en rango. **V1 ampliada:** `peso` de un par suplente **negativo** → rechazo. **V8 al contrato nuevo:** `aula_fija == true` exige `aula` índice válido; `aula_fija == false` → `aula` se ignora.
- **`horas_requeridas` opcional:** si está presente (`> 0`), se comprueba `horas_aula ≤ horas_requeridas`; si **no** está (`-1`), el chequeo se omite y no invalida la entrada. No es una regla del solver.
- **V13:** cada materia-curso tiene al menos un candidato elegible (plan ∩ turno ∩ **año** ∩ disponibilidad) → si no, rechazo con causa.
- **V14:** para cada curso, `Σ_m bloques(horas_m) ≤ |slots del turno(c)|` (no más bloques que franjas útiles) → si no, rechazo.
- **V15:** curso móvil con `num_aulas == 0` → rechazo.

---

## 6. Interfaz — cómo se pinta (contrato de consumo, **sin cambios en `src/frontend`**)

La interfaz Qt la aporta otro equipo (fuera de alcance, spec §Fuera de alcance). Este plan **fija el contrato que pinta** y lo documenta en `docs/interfaz-frontend.md` (tarea de documentación del implementador).

- **Clave del curso:** la salida llega indexada por la **identidad** del curso (`anio`+`seccion`, p. ej. `"1|A"`); la UI **deriva el nombre para mostrar** (`"1ro A"`, `"4to B"`) al pintar (RF-3, RF-7, RF-9). No hay nombre almacenado ni derivado en la salida. `ScheduleVisualizationWidget` sigue listando `horarios[...]`, ahora por la clave de identidad.
- **Aula:** cada clase llega con el **índice** de aula (RF-9); la UI traduce índice→nombre para pintar la celda (hoy `schedule_model.hpp` guarda `aula` como texto). Esa traducción es del frontend; nosotros solo garantizamos el índice.
- **Forma del dato:** `HorarioSalida` conserva la forma actual (curso → días → asignaciones con `slot`, `materia`, `profesor`, `aula`); lo que cambia es el **valor** de la clave (la identidad `anio`+`seccion`, no el nombre derivado) y la naturaleza del aula (índice). No hay cambio estructural que rompa el `toJson`/`fromJson` (RF-10).
- La UI muestra el bloque con materia, docente y aula; los conflictos de arrastre los detecta su `schedule_model.hpp` (fuera de alcance).

---

## 7. Decisiones técnicas (con alternativa descartada)

| # | Decisión | Alternativa descartada | Motivo |
|---|---|---|---|
| D-1 | **Tope + exceso penalizado** para las horas de aula (S1). | (a) Igualdad dura «cada docente da exactamente sus horas»; (b) minimizar el «déficit» de horas. | (a) vuelve INFEASIBLE el problema con docentes «de sobra» (causa de los tests rojos, HU-2); (b) es **inerte** porque `Σ` horas asignadas es constante (fijado por D3). |
| D-2 | **Variable de activación `z[p,c,m]`** con `Σ_p z = 1` y `Σ_{d,s} x = bloques·z`. | Solo `Σ_{p} x[p,m,c,d,s] ≤ 1` por franja. | Esa alternativa permite **repartir** la materia entre varios docentes por franjas; D1/D2 exigen que la dicte **exactamente uno**. |
| D-3 | Curso identificado por **`anio` + `seccion`**; se elimina `nombre` de la config. | Conservar `nombre` y validar coherencia. | RF-7: el nombre se **deriva** y no es dato de identidad; guardarlo invita a duplicados/incoherencias. |
| D-4 | `aula_fija` **booleano + `aula` índice**; `-1` = móvil. | Entero `-1`/índice (contrato anterior) o nombre de aula. | §7.3/RF-7/V8: el índice es el identificador; el nombre lo traduce la UI. |
| D-5 | D9 como **igualdad** `Σ_a y = Σ_{p,m} x`. | `Σ_a y ≤ 1` (relajado). | D9 exige **exactamente un** aula por franja con clase; `≤ 1` permitiría clase sin aula. |
| D-6 | Curso de **aula fija sin variable**: la extracción escribe `c.aula` (D10). | Crear también `aulaAssignment` para fijas y fijarla. | Ahorra variables y garantiza siempre su aula; D8 ya impide el solapamiento de esa aula incluyendo las fijas. |
| D-7 | `costeSuplente(peso) = max(1, C − peso)`. | `C / (1 + peso)` (división entera). | La división entera **llega a 0** para `peso ≥ C`, igualando el coste de un titular y rompiendo S2. |
| D-8 | `horas_requeridas` **opcional** (`-1` = ausente); solo chequeo de coherencia. | Exigirla siempre como ahora. | RF-11: si falta, el chequeo se omite y no invalida; el solver solo usa `horas_aula`. |
| D-9 | **Parámetros deterministas** (`num_search_workers = 1`, `random_seed = 0`). | Búsqueda paralela por defecto. | RNF-1: los empates de `peso` deben resolverse igual siempre (misma entrada ⇒ mismo resultado). |
| D-10 | Validaciones **en `SolverConfig::fromJson`** (contrato), expuestas por `ValidadorEntradaSolver`. | Validar solo en el servicio de generación. | RF-11 y el contrato manda: la validación viaja con el contrato y se prueba sin UI ni BD. |
| D-11 | Eliminar `restriccionMateriaUnicaPorAula` (D12). | Conservarla. | Redundante con D8 (aula no solapada); spec marca D12 como eliminada. |
| D-12 | La **clave de la salida** es la **identidad** del curso (`anio`+`seccion`; p. ej. `"1|A"`). | Mantener el **nombre derivado** (`"1ro A"`) como clave de la salida. | El solver **no** es responsable de la presentación: el nombre lo deriva quien muestra (frontend/consola); la identidad es el dato estable y no ambiguo. |

> Nota: el antiguo puente temporal que derivaba año/sección del nombre **no existe**; la persistencia de los campos de curso es la **spec 004** (prerrequisito), así que `ConstructorEntradaSolver` lee los campos ya persistidos.

---

## 8. Estrategia de tests

Comando del proyecto (AGENTS.md, `docs/constitution.md` §4), **en el contenedor** (OR-Tools solo está ahí):

```bash
cmake --preset full && cmake --build build && ctest --preset full --output-on-failure
# dentro del contenedor gestor-dev:
docker exec gestor-dev bash -lc "cd /workspace && cmake --preset full && cmake --build build && ctest --preset full --output-on-failure"
```

Política: **tests primero** (rojo → código → verde → marcar). Un caso por regla; RNF-4 (trazabilidad).

| Test | Framework | Qué cubre | RF |
|---|---|---|---|
| `test/backend/test_contrato_solver.cpp` **(nuevo)** | Qt Test | `bloques`/`bloquesAula` con `duracion_minutos` ≠ 60 (redondeo hacia arriba), `costeSuplente` (peso 0 → máximo, decreciente, `> 0`, `≤ máximo`), constantes `PESO_HORAS > COSTE_MAXIMO_SUPLENTE > 0`. (La derivación del nombre es **presentación** y se prueba en la spec 004, no aquí.) | RF-1, RF-5 |
| `test/backend/test_validador_entrada.cpp` **(modificado)** | Qt Test | V1–V15: peso negativo, `aula_fija` bool + `aula`, `horas_requeridas` ausente/presente, V8, V13 (sin candidato por plan/turno/año/disponibilidad), V14 (más bloques que franjas), V15 (móvil sin aulas). JSON construido y mutado una sección por caso. | RF-7, RF-8, RF-11 |
| `test/backend/test_cargador_configuracion.cpp` **(modificado)** | Qt Test | Ida y vuelta `fromJson(toJson())` con año/sección, aula fija/índice, años de docente y `peso`; el JSON serializado **no** contiene `nombre`. | RF-10 |
| `test/backend/test_constructor_entrada.cpp` **(modificado)** | Qt Test | El JSON emitido usa año/sección (no `nombre`) y `aula_fija` bool + índice; sigue siendo consumible por `fromJson`; pureza con instante inyectado. | RF-7, RF-8, RF-10 |
| `test/backend/test_variables_cp_sat.cpp` **(modificado)** | Google Test | Poda por **años**, creación de `asignacionDocente`, aulas fijas/móviles, disponibilidad y turnos; conteos exactos. | RF-1, RF-4 |
| `test/backend/test_solver_horarios.cpp` **(reescrito)** | Qt Test | D1 (un docente por materia-curso, XOR), D3 (bloques), D4/D5 (sin solapamientos), D8, D9/D10 (enlace y aula fija), D11/D13, S1 (tope+exceso: con docentes de sobra **hay** solución), S2 (titular antes que suplente; entre suplentes, mayor `peso`; titular no elegible → cubre suplente), clave = identidad (año, sección), aula = índice, reproducibilidad. | RF-1…RF-9, RF-12 |
| `test/backend/test_solver_benchmark.cpp` **(reescrito)** | Qt Test | Escenario de gran tamaño resuelto en < 60 s y consistencia (profesor/aula sin solapamiento). | RF-12, RNF-2 |
| `test/backend/test_validador_salida.cpp` **(modificado)** | Qt Test | El análisis de salida casa por la **identidad del curso** (año+sección) (P1/P2/P4) y no por un nombre almacenado ni derivado. | RF-3, RF-9 |

`test/CMakeLists.txt` **(modificado):** registra `test_contrato_solver` con `add_qtest(... backend)` (sin OR-Tools); los tests del solver siguen en `add_qtest/add_gtest(... backend_solver)`.

Alcance de las reescrituras (RF-12): las pruebas del contrato anterior (`nombre` almacenado, `aula_fija` entero, sin años) se reescriben al contrato nuevo **verificando el comportamiento correcto**, no ajustándolas para pasar.

---

## 9. Trazabilidad RF → partes del plan

| RF | Plan (secciones y archivos) |
|---|---|
| RF-1 (D1–D3) | §1.2 variables + `restricciones_profesores.cpp` (z, D1/D2) + `restricciones_turnos.cpp` (D3) + `contrato_solver.cpp` (`bloques`) |
| RF-2 (D4/D5/D8) | `restricciones_turnos.cpp` (D4), `restricciones_profesores.cpp` (D5), `restricciones_aulas.cpp` (D8) |
| RF-3 (D9/D10, clave/aula) | `restricciones_aulas.cpp` (D9), `solver.cpp` (extracción, clave = identidad `anio`+`seccion`), `ValidadorSalidaSolver.cpp` |
| RF-4 (D6/D7/D11/D13) | `variables.cpp` (poda) + `restricciones_profesores.cpp` (D13) + `solver_config.cpp` (validación) |
| RF-5 (S1/S2) | `restricciones_turnos.cpp` (tope+exceso) + `solver.cpp` (objetivo) + `contrato_solver.cpp` (coste, constantes) |
| RF-6 (contrato/comportamiento) | `solver.cpp` (factibilidad, causa, determinismo), `servicio_solver.cpp` |
| RF-7 (cursos) | `solver_config.hpp/.cpp` + `ConstructorEntradaSolver.cpp` (campos persistidos de la spec 004); el nombre derivado es **presentación** (frontend/consola), no del solver |
| RF-8 (docentes) | `solver_config.hpp/.cpp` (`anios`, `peso`) |
| RF-9 (índice de aula) | `solver.cpp` (extracción), `ValidadorSalidaSolver.cpp` |
| RF-10 (serialización) | `solver_config.cpp` (`fromJson`/`toJson`), `CargadorConfiguracionSolver` (sin cambios), `ConstructorEntradaSolver.cpp` |
| RF-11 (V1–V15) | `solver_config.cpp` (validaciones) + `ValidadorEntradaSolver.cpp` |
| RF-12 (pruebas verdes) | §8 tests reescritos (`test_solver_horarios`, `test_solver_benchmark`, etc.) |
| RNF-1 | `solver.cpp` (parámetros deterministas) |
| RNF-2 | `test_solver_benchmark.cpp` (< 60 s) |
| RNF-3 | `solver.cpp` (`analizarFallo`), `solver_config.cpp` (mensajes) |
| RNF-4 | §8 y §9 (cobertura verificable por RF) |

---

## 10. Notas y fuera de alcance heredado

- **Persistencia (spec 004):** aquí no se toca la tabla `Cursos`; los campos `anio`/`seccion` en la BD y los servicios de cursos son de la **spec 004**, que **precede** a esta. El único contacto es que `ConstructorEntradaSolver` lee los campos ya persistidos (sin puente ni parseo de nombres). La **gestión de presets** sí es de esta spec (contrato/serialización, RF-10).
- **UI (`src/frontend`):** fuera de alcance; solo se documenta el contrato de consumo (§6) en `docs/interfaz-frontend.md`.
- **Turnos automáticos (006) y generación parcial (007):** fuera de alcance.
- **Capacidad de aula y cobertura de emergencia:** fuera del catálogo (no se implementan). La capacidad sigue solo como aviso (P3) del análisis de salida, sin restricción del modelo.
- **Reparto equilibrado min-max:** fuera de alcance.
- **`docs/solver/`:** referencia histórica; no obliga.
