# Plan 009 — Agrupación de bloques contiguos por materia-curso

Base: 7e2688674ac587a808aa94fb76cd991b02db3bda
Spec de referencia: `specs/009-agrupacion-bloques-materias/spec.md` (**aprobada**, sin dudas abiertas)
Constitución: §2 (spec manda), §3 (lógica/UI), §4 (tests primero), §7 (contrato del solver)

> Este documento describe el **CÓMO**. No añade dependencias: C++17 · Qt6 · OR-Tools CP-SAT · CMake/Ninja · CTest + Google Test / Qt Test.
> Alcance: `src/backend` y `src/backend/src/solver` (target `backend_solver`). **No** se toca `src/frontend`.

---

## 0. Frontera y flujo

La nueva prioridad blanda **S3** se añade a la función objetivo del solver existente. Recibe `SolverConfig` ya validada; no abre BD. La salida no cambia (misma clave `anio|seccion`, aula = índice).

```
SolverConfig (validado V1–V15)
        │
        ▼
┌─────────────────────────────────────────────────────────────┐
│  Modelo CP-SAT existente + NUEVAS variables de arranque     │
│  • x[p,m,c,d,s] (asignación) — ya existe                    │
│  • start[m,c,d,seg,s] ∈ {0,1} — NUEVO: 1 si x=1 y (x_prev=0 │
│    o s = inicio de segmento)                                │
│  • len[m,c,d,seg,s] ∈ ℕ — NUEVO: tamaño del grupo desde     │
│    cada arranque (o formulación equivalente)                │
│  • e[p] (exceso horas) — ya existe (S1)                     │
│  • z[p,c,m] (docente asignado) — ya existe (S2)             │
└─────────────────────────────────────────────────────────────┘
        │
        ▼
Función objetivo:
  PESO_HORAS · Σ e[p]  +  COSTE_SUPLENTE · Σ z_suplente  +  PESO_AGRUPACION · Σ penalizaciones_S3
        │
        ▼
Solver (determinista: num_search_workers=1, random_seed=0)
        │
        ▼
HorarioSalida (sin cambios) + ValidadorSalida (nuevas comprobaciones AC-1..AC-9)
```

---

## 1. Archivos y responsabilidades

| Archivo | Acción | Responsabilidad | RF |
|---|---|---|---|
| `src/backend/include/backend/solver/config/contrato_solver.hpp` + `.cpp` | **modificado** | Añadir constante `PESO_AGRUPACION = 10`. Función auxiliar `segmentosTurno(const TurnoConfig&)` → `QVector<QVector<int>>` (lista de segmentos contiguos por receso). Sin E/S, sin OR-Tools. | RF-1, RF-2, RF-4 |
| `src/backend/include/backend/solver/variables.hpp` + `.cpp` | **modificado** | Crear variables `start[m,c,d,seg,s]` y `len[m,c,d,seg,s]` (o equivalente). Derivar segmentos por turno+receso. O(cursos × materias × días × segmentos × franjas) booleanas auxiliares. | RF-1, RF-2 |
| `src/backend/src/solver/restricciones/restricciones_turnos.cpp` + `.hpp` (o nuevo `restricciones_agrupacion.cpp`) | **modificado / nuevo** | Añadir restricciones que ligan `start` y `len` a `x`: `start = 1 ⇔ x=1 ∧ (x_prev=0 ∨ s=inicio_segmento)`. `len` = suma de `x` consecutivos desde `start` hasta 0 o fin de segmento. Penalización en objetivo según tabla RF-2. | RF-1, RF-2, RF-5 |
| `src/backend/src/solver/solver.cpp` | **modificado** | Extender `construirObjetivo` con término S3: `PESO_AGRUPACION · Σ start · f(len)`. Verificar que S3 no empeora S1/S2 (tests). | RF-4, RF-5, RF-6 |
| `src/backend/include/backend/services/ValidadorEntradaSolver.hpp` + `.cpp` | **modificado** (opcional) | Validar `PESO_AGRUPACION > 0` si se expusiera en JSON; hoy es constante interna → sin cambios obligatorios. | — |
| `src/backend/src/services/ValidadorSalidaSolver.cpp` | **modificado** | Añadir análisis de grupos contiguos por (curso, materia, día, segmento): contar tamaños, calcular penalización esperada, verificar que coincide con objetivo. | AC-1..AC-9 |
| `test/backend/test_solver_horarios.cpp` | **modificado** | Tests AC-1..AC-9 (fixtures: 4h/semana, 1h/semana, 3h 2+1, 5h, receso cross, cross-segmento, benchmark). | RF-1, RF-2, RF-3, RF-5, RF-6, AC-1..AC-9 |
| `test/backend/test_validador_salida.cpp` | **modificado** | Verificar que `analizarSalidaSolver` reporta grupos y penalizaciones correctas. | AC-1..AC-9 |
| `test/backend/test_contrato_solver.cpp` | **modificado** | Verificar `PESO_AGRUPACION = 10` y jerarquía `1000 > 100 > 10 > 0`. | RF-4 |
| `src/backend/CMakeLists.txt` | **modificado** | Si se crea `restricciones_agrupacion.cpp`, añadirlo a `backend_solver`. | — |
| `test/CMakeLists.txt` | **modificado** | Registrar tests nuevos/modificados. | — |

---

## 2. Funciones puras de lógica (nuevas / extendidas)

| Función | Firma | Semántica | RF | ¿Reloj? |
|---|---|---|---|---|
| `segmentosTurno` | `QVector<QVector<int>> segmentosTurno(const TurnoConfig&, const QVector<RecesoConfig>&)` | Devuelve lista de segmentos contiguos de franjas para un turno, cortando en cada receso (`despues_de_slot`). Pura, sin E/S, sin OR-Tools. | RF-1 | no |
| `esInicioSegmento` | `bool esInicioSegmento(const QVector<QVector<int>>& segmentos, int s)` | True si `s` es el primer índice de algún segmento. | RF-1 | no |
| `calcularPenalizacionGrupo` | `int calcularPenalizacionGrupo(int tamaño, int horasDia)` | Implementa tabla RF-2: `tamaño==1 → (horasDia>1?1:0)`, `2/3→0`, `≥4→tamaño-3`. Pura. | RF-2 | no |

Constante del motor (`contrato_solver.hpp`): `PESO_AGRUPACION = 10`, con `PESO_HORAS (1000) > COSTE_SUPLENTE (100) > PESO_AGRUPACION (10) > 0`.

---

## 3. Modelo CP-SAT: variables, restricciones y objetivo (pseudocódigo)

### 3.1 Variables nuevas

```
para cada curso c, materia m del curso, día d, segmento seg ∈ segmentosTurno(turno(c)), franja s ∈ seg:
    start[m,c,d,seg,s] = BoolVar()   # arranque de grupo
    len[m,c,d,seg,s]   = IntVar(0, |seg|)  # longitud del grupo desde este arranque (0 si no arranca)

# Nota: se puede formular sin len explícito usando transiciones 0→1 y 1→0,
# pero len facilita la penalización lineal f(len) en el objetivo.
```

### 3.2 Restricciones de ligadura (duras, nunca violadas)

```
# start = 1 ⇔ x=1 y (x_prev=0 o s es inicio de segmento)
para cada (m,c,d,seg,s):
    sea s_prev = s-1 si s-1 ∈ seg else -1
    start[m,c,d,seg,s] == 1  ⇔  ( Σ_p x[p,m,c,d,s] == 1  ∧  (s_prev==-1 ∨ Σ_p x[p,m,c,d,s_prev] == 0) )

# len: si start=1, len = cuenta de x=1 consecutivos hasta 0 o fin de segmento
# Formulación con BoolVars de continuación (run) o usando AddMaxEquality / circuit.
# Alternativa equivalente: variable run[k] = 1 si el grupo sigue en franja k desde arranque s.
# El plan no fija la formulación exacta; el implementador elige la que no explote en variables.
```

### 3.3 Función objetivo extendida

```
Minimizar(
    PESO_HORAS      · Σ_p e[p]                                    # S1
  + COSTE_SUPLENTE  · Σ_{(p,c,m): p suplente de m} z[p,c,m]      # S2
  + PESO_AGRUPACION · Σ_{m,c,d,seg,s} start[m,c,d,seg,s] · f(len[m,c,d,seg,s], horasDia[m,c,d])
)
```

donde `f(len, horasDia)` = tabla RF-2 y `horasDia[m,c,d] = Σ_{seg,s} Σ_p x[p,m,c,d,s]`.

### 3.4 Reproducibilidad y determinismo

Parámetros ya fijados en solver: `num_search_workers = 1`, `random_seed = 0`. La adición de variables booleanas auxiliares no rompe la reproducibilidad.

---

## 4. Decisiones técnicas (con alternativa descartada)

| # | Decisión | Alternativa descartada | Motivo |
|---|---|---|---|
| D-1 | **Variables de arranque `start` por (m,c,d,seg,s)** | Una variable por cada grupo posible (combinatorio). | O(cursos·materias·días·segmentos·franjas) ≈ 13·10·5·2·8 ~ 10k booleanas, no explosivo. Grupo-por-grupo sería O(2^franjas). |
| D-2 | **Segmentos derivados de turno + receso** (función pura `segmentosTurno`) | Hardcodear segmentos {0,1},{2..7} etc. en el solver. | Los recesos vienen en configuración; la función pura es testeable y reutilizable. |
| D-3 | **Penalización en objetivo con `len` (o equivalente)** | Penalizar solo `start` con peso fijo (no distingue tamaños). | RF-2 exige distinguir 1 vs 2–3 vs ≥4; `len` (o transiciones 1→0) permite `f(len)`. |
| D-4 | **`PESO_AGRUPACION = 10` constante en `contrato_solver.hpp`** | Exponer en preset JSON (campo opcional). | Decisión D-3 → A de la spec: constante interna, sin versionar JSON. |
| D-5 | **Nueva restricción en `restricciones_turnos.cpp` o archivo propio `restricciones_agrupacion.cpp`** | Mezclar en `solver.cpp` directamente. | Separación de concerns; testeable aisladamente; coherente con arquitectura actual (D1–D13 en archivos dedicados). |
| D-6 | **Validación de salida (ValidadorSalidaSolver) cuenta grupos y penalizaciones** | Solo tests unitarios. | AC-8 exige verificación en datos reales (13 cursos, 520 clases); validador de salida es el lugar natural. |

---

## 5. Estrategia de tests

Comando del proyecto (contenedor `gestor-dev`, OR-Tools solo ahí):

```bash
docker exec gestor-dev bash -lc "cd /workspace && cmake --preset full-release && cmake --build build-rel && LD_LIBRARY_PATH=$HOME/.local/ortools/lib ctest --preset full-release --output-on-failure"
```

Política: **tests primero** (rojo → código → verde → marcar). Un caso por regla/AC; RNF-3 (trazabilidad).

| Test | Framework | Qué cubre | AC / RF |
|---|---|---|---|
| `test/backend/test_contrato_solver.cpp` | Qt Test | `PESO_AGRUPACION = 10`; jerarquía `1000 > 100 > 10 > 0`; `segmentosTurno` devuelve segmentos correctos para Mañana/Tarde con receso tras slot 1/9. | RF-1, RF-4 |
| `test/backend/test_solver_horarios.cpp` | Qt Test | **AC-1**: 4h/semana → grupos de 2 preferidos. **AC-2**: S2 gana a S3 (suplente vs titular). **AC-3**: 1h/día pen=0; 3h 2+1 pen=1. **AC-4**: 5h grupo → pen=2. **AC-5**: receso rompe (slots 1,2 Mañana = dos grupos de 1). **AC-6**: reproducibilidad (2 runs, mismo seed). **AC-7**: benchmark ≤ 5 s. **AC-8**: integración 13 cursos / 520 clases (D1–D13 intactas, grupos medidos). **AC-9**: cross-segmento (1 slot en {0,1} + 1 en {2..7} → 2 arranques, pen=2). | RF-1..RF-6, AC-1..AC-9 |
| `test/backend/test_validador_salida.cpp` | Qt Test | `analizarSalidaSolver` extrae grupos por (curso,materia,día,segmento), calcula penalización y coincide con objetivo. | AC-1..AC-9 |
| `test/backend/test_solver_benchmark.cpp` | Qt Test | Escenario de gran tamaño ≤ 5 s + aserción: S1/S2 no empeoran al activar S3 (comparar objetivo con/sin S3 en fixture controlado). | AC-7, RF-5 |

---

## 6. Trazabilidad RF → partes del plan

| RF | Plan (secciones y archivos) |
|---|---|
| RF-1 (grupo contiguo, receso rompe) | §1 `contrato_solver::segmentosTurno`, §3.1 `start`, §3.2 ligadura `start ⇔ x` |
| RF-2 (penalización exacta por tamaño) | §2 `calcularPenalizacionGrupo`, §3.2 `len` + `f(len, horasDia)`, §3.3 objetivo S3 |
| RF-3 (ámbito por curso,materia) | §3.1 variables indexadas por `m,c` (no por `p`) |
| RF-4 (`PESO_AGRUPACION = 10`, jerarquía) | §1 `contrato_solver.hpp` constante, §3.3 objetivo |
| RF-5 (S3 nunca empeora S1/S2/duras) | §3.3 objetivo (suma aditiva, pesos jerárquicos), §5 test benchmark con aserción |
| RF-6 (reproducibilidad) | §3.4 parámetros deterministas heredados |
| RF-7 (sin cambios JSON) | §1 sin cambios en `SolverConfig`/`solver_config.hpp` |
| RF-8 (contigüidad dentro del turno) | §2 `segmentosTurno` usa `turno(c)` del curso, §3.1 variables por `seg ∈ segmentosTurno(turno(c))` |

---

## 7. Comandos de verificación (lo que ejecutará el implementador)

```bash
# 1. Build y tests unitarios del solver (incluye test_contrato_solver, test_solver_horarios)
cmake --preset full-release
cmake --build build-rel
LD_LIBRARY_PATH=$HOME/.local/ortools/lib ctest --preset full-release --output-on-failure

# 2. Test específico de los nuevos AC (filtro por nombre si hace falta)
LD_LIBRARY_PATH=$HOME/.local/ortools/lib ./build-rel/test/backend/test_solver_horarios --gtest_filter="*Agrupacion*"

# 3. Test de validador de salida (grupos y penalizaciones)
LD_LIBRARY_PATH=$HOME/.local/ortools/lib ./build-rel/test/backend/test_validador_salida --gtest_filter="*Agrupacion*"

# 4. Benchmark de rendimiento y aserción S1/S2
LD_LIBRARY_PATH=$HOME/.local/ortools/lib ./build-rel/test/backend/test_solver_benchmark

# 5. Test manual end-to-end con preset real (opcional, para QA final)
LD_LIBRARY_PATH=$HOME/.local/ortools/lib ./build-rel/src/app/gestor-horarios-seed --db /tmp/test-009.db --variante holgada
LD_LIBRARY_PATH=$HOME/.local/ortools/lib ./build-rel/src/app/gestor-horarios --backend --config preset-test.json --output /tmp/horario-009.json
# Verificar en /tmp/horario-009.json: grupos por (curso,materia,día,segmento) según AC-8
```