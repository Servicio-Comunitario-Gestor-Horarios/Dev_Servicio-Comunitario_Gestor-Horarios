# Plan 005 — Núcleo del solver y contrato de configuración

Base: f94acba455a04ae80364c82e1f07ae16d9f156ec

Estado: vigente (rectificado el 2026-10-17: **hora académica = bloque**).
Spec de referencia: `specs/005-nucleo-solver/spec.md` (**aprobada**, sin dudas abiertas).
Constitución: §2 (spec manda), §3 (lógica/UI), §4 (tests primero), §5 (datos), §6 (idioma), §7 (contrato del solver).
Specs relacionadas: `specs/004-persistencia-campos-curso` (**prerrequisito implementado**: aporta `Cursos(anio, seccion, turno, aula)`, esquema único y la **construcción de la entrada se movió a esta spec**); `specs/002-generacion-horarios-ipc` (su contrato JSON de curso/ modelo queda **superado** por esta spec).

> Este documento describe el **CÓMO**. No añade dependencias: C++17 · Qt6 (Core/Sql) · OR-Tools CP-SAT · CMake/Ninja · CTest + Google Test / Qt Test.
> Alcance: `src/backend` y `src/backend/src/solver` (target `backend_solver`). **No** se toca `src/frontend`; el contrato de consumo se documenta (§7) sin modificar el frontend. La **gestión de presets** como archivos no entra (solo el contrato y su serialización).

---

## 0. Frontera y flujo

```
 datos persistidos (BD)                                SolverConfig                  HorarioSalida (JSON)
 cursos (anio,seccion,turno,aula)                 ┌──  aula_fija global        ┐
 aulas · materias · docentes · planes ────────────┤    cursos (identidad+plan) ├─► extraerSolucion ─► horarios[ "anio|seccion" ]
 (PlanEstudio_Materia: curso+horas)               │    planes embebidos        │     aula = índice
                              + preset ──────────►│    profesores (anios,peso) │
     ConstructorEntradaSolver (backend, RF-13)    └────────────────────────────┘
              (puro si E/S: solo devuelve JSON)          SolverConfig::fromJson (V1–V15)
                                                         ↓
                                              Solver (backend_solver): variables + D1–D11,D13 + objetivo S1/S2
```

- **`backend`** (Qt6 Core/Sql, sin OR-Tools): contrato de configuración (`SolverConfig`), funciones puras del contrato y **validación previa V1–V15**; y la **construcción de la entrada** (RF-13).
- **`backend_solver`** (OR-Tools): variables, invariantes, objetivo y extracción. Recibe `SolverConfig` **ya validada**; no abre la BD.
- **`src/frontend`** (otro equipo): pinta; traduce índice de aula→nombre y deriva el nombre del curso. **No se modifica.**

---

## 1. Archivos y responsabilidades

### 1.1 `backend` — contrato, funciones puras, validación y construcción de la entrada

| Archivo | Acción | Responsabilidad | RF |
|---|---|---|---|
| `src/backend/include/backend/solver/config/contrato_solver.hpp` + `src/backend/src/solver/config/contrato_solver.cpp` | **nuevo** | Funciones puras del contrato: `bloques(horas, duracion_minutos)` (identidad: devuelve `horas`), `bloquesAula(horas_aula, duracion_minutos)` (identidad), `claveCurso(anio, seccion)`. Constantes del motor: `PESO_HORAS`, `COSTE_SUPLENTE` (y `COSTE_MAX_SUPLENTE`, reservada). Sin `nombreCurso` (el nombre es presentación, spec 004). Sin estado, sin E/S, sin OR-Tools. | RF-1 (D3), RF-3 (clave), RF-5 (S1/S2) |
| `src/backend/include/backend/solver/config/solver_config.hpp` | **modificado** | Modelo nuevo: `SolverConfig` gana `bool aula_fija = true` (global) y `QVector<PlanSolverConfig> planes` (embebidos). `CursoSolverConfig` pasa a `{ int anio; QString seccion; QString turno; QString plan; int num_estudiantes = -1; int aula = -1; }` (se **elimina** `nombre`, `aula_fija` int y `materias`). `ProfesorSolverConfig` gana `QVector<int> anios`; `horas_requeridas = -1` = ausente. Nuevo `PlanSolverConfig { QString codigo; QVector<MateriaPlan> materias; }` y `MateriaPlan { int materiaIDx; int curso; int horasSemanales; }`. Se conserva `MateriaSuplente.peso`. | RF-7, RF-8, RF-10 |
| `src/backend/src/solver/config/solver_config.cpp` | **modificado** | `fromJson`: parsea `aula_fija` global, `anio`/`seccion`/`plan`/`num_estudiantes`/`aula` por curso, `planes` embebidos, `anios` del docente y `peso` por par; **ignora `nombre`** si llega; **tolera campos desconocidos**; valida **versión `1.0`**; ejecuta V1–V15. `toJson`: escribe el contrato nuevo **sin `nombre`**. Ida y vuelta sin pérdida. Helper puro `materiasDelCurso(const CursoSolverConfig&)` = materias del plan del curso cuyo `curso == anio`. | RF-7, RF-8, RF-10, RF-11 |
| `src/backend/include/backend/services/ValidadorEntradaSolver.hpp` + `.cpp` | **sin cambios** | Fachada de la validación previa; el detalle vive en `SolverConfig::fromJson`. | RF-11 |
| `src/backend/include/backend/services/ConstructorEntradaSolver.hpp` | **modificado** | `DatosDominio` pasa a llevar los **planes compuestos** (`QVector<PlanEstudio>` con materias+horas) en lugar de `QVector<PlanDTO>` sin contenido. | RF-13 |
| `src/backend/src/services/ConstructorEntradaSolver.cpp` | **modificado** | Arma cada curso mezclando **BD** (`anio`/`seccion`/`turno`/`aula`) y **preset** (`plan`/`num_estudiantes`); **embebe solo los planes referenciados**; traduce el aula a **índice** (modo fijo) o `-1` (modo móvil); toma `aula_fija` global del preset; **se retiran los `TODO(spec 005)`**. | RF-7, RF-8, RF-10, RF-13 |
| `src/backend/CMakeLists.txt` | **modificado** | Añade `src/solver/config/contrato_solver.cpp` al target `backend`. | — |

### 1.2 `backend_solver` — variables, restricciones, objetivo y salida

| Archivo | Acción | Responsabilidad | RF |
|---|---|---|---|
| `src/backend/include/backend/solver/variables.hpp` + `src/backend/src/solver/variables.cpp` | **modificado** | Crea `assignment[p,m,c,d,s]` (aplicando elegibilidad por plan/turno/**año**/disponibilidad, y materias **derivadas del plan**), `aulaAssignment[c,d,s,a]` (solo cursos móviles: `!config.aula_fija`) y la **nueva** variable de activación `asignacionDocente[p,c,m]` (docente asignado a la materia-curso). | RF-1 (D1/D2), RF-3 (D9/D10), RF-4 (D6/D7/D13) |
| `src/backend/src/solver/restricciones/restricciones_profesores.cpp` + `.hpp` | **modificado** | Conserva D5; añade `restriccionUnDocentePorMateria` (D1+D2+D3 por `asignacionDocente`) y `restriccionDocentePorAnio` (D13). D6/D7 siguen por poda (`x = 0`). | RF-1, RF-2 (D5), RF-4 (D13) |
| `src/backend/src/solver/restricciones/restricciones_turnos.cpp` + `.hpp` | **modificado** | Conserva D4; reescribe `restriccionBloquesRequeridos` (D3: `Σ x = horas_m · z`, sin conversión); **sustituye** la **igualdad dura** `restriccionHorasSemanalesProfesor` por `restriccionTopeHorasProfesor` (S1: tope + exceso). | RF-1 (D3), RF-2 (D4), RF-5 (S1) |
| `src/backend/src/solver/restricciones/restricciones_aulas.cpp` + `.hpp` | **modificado** | Conserva D8 (aula no solapada) **incluyendo cursos de aula fija**; **elimina** `restriccionMateriaUnicaPorAula` (D12 suprimida); añade `restriccionCursoOcupaUnAula` (D9: enlace clase↔aula). | RF-2 (D8), RF-3 (D9/D10) |
| `src/backend/include/backend/solver/solver.hpp` + `src/backend/src/solver/solver.cpp` | **modificado** | Construye el modelo, fija la **función objetivo** (S1+S2) y **parámetros deterministas**; extrae la solución indexando por la **identidad** `claveCurso(anio,seccion)` y con el **aula como índice**; mensajes de fallo en español. | RF-3, RF-5, RF-6, RF-9, RNF-1, RNF-3 |
| `src/backend/src/solver/servicio_solver.cpp` | **sin cambios** | Adapta JSON→`SolverConfig`→`Solver`→respuesta. | RF-6 |

---

## 2. Funciones puras de lógica

Regla de pureza (constitución §4): **nada lee el reloj ni el entorno**; el instante se **inyecta como parámetro**. Las funciones del contrato no dependen del tiempo: son puras y deterministas por construcción (añadir un reloj sería un parámetro falso).

| Función | Firma | Semántica | RF | ¿Reloj? |
|---|---|---|---|---|
| `bloques` | `int bloques(int horas, int duracion_minutos)` | **Identidad:** `horas` (1 hora académica = 1 bloque). `duracion_minutos` **no** interviene; se conserva en la firma para no tocar los call sites y como **guarda defensiva**: `0` si `duracion_minutos ≤ 0` (V2 ya rechaza ese caso antes de resolver). | RF-1 | no |
| `bloquesAula` | `int bloquesAula(int horas_aula, int duracion_minutos)` | **Identidad:** `horas_aula`; misma guarda que `bloques` (S1). | RF-5 | no |
| `claveCurso` | `QString claveCurso(int anio, const QString& seccion)` | Identidad canónica determinista del curso, p. ej. `"1|A"`; **no** deriva el nombre. | RF-3, RF-9 | no |
| `materiasDelCurso` | `const QVector<MateriaPlan>* materiasDelCurso(const SolverConfig&, const CursoSolverConfig&)` | Materias+horas del curso = materias del `planes[codigo==c.plan]` con `curso == c.anio`. | RF-7, RF-13 | no |
| `construirJsonEntrada` | `QJsonObject construirJsonEntrada(const DatosDominio&, const SolverConfig&, const QDateTime& ahora)` | Pura (sin E/S); estampa `meta.fecha_modificacion` con el **instante inyectado**. | RF-13, RF-10 | **sí (`ahora`)** |
| `estamparFechaGeneracion` | `HorarioSalida estamparFechaGeneracion(const HorarioSalida&, const QDateTime& ahora)` | Pura; copia el horario y fija `metadata.fecha_generacion`. | RF-10 | **sí (`ahora`)** |
| `huellaDatos` | `QString huellaDatos(const DatosDominio&)` | Pura; huella canónica SHA-256 del snapshot (sin datos sensibles, §5). | RF-6 | no |

Constantes del motor (`contrato_solver.hpp`): `PESO_HORAS = 1000`, `COSTE_SUPLENTE = 100`, `COSTE_MAX_SUPLENTE = 100` (reservada, **sin uso**), con `PESO_HORAS > COSTE_SUPLENTE > 0` (§7.2, RF-5).

---

## 3. Modelo CP-SAT: variables, restricciones y objetivo (pseudocódigo)

### 3.1 Variables

```
para cada curso c, materia-plan (m, horas) de c (derivadas de su plan):
    para cada docente p candidato de m (titular o suplente declarado):
        si p.plan definido y c.plan ≠ p.plan            → descartar (D7)
        si p.anios no vacía y c.anio ∉ p.anios          → descartar (D13)
        para cada día d y slot s ∈ (turno(c) ∩ disponibilidad(p)):
            si p.turno definido y s ∉ turno(p)          → descartar (D6)
            x[p,m,c,d,s] = BoolVar()
        z[p,c,m] = BoolVar()                            # activación "p dicta la materia-curso"
para cada curso móvil c (config.aula_fija == false), día d, slot s ∈ turno(c), aula a:
    y[c,d,s,a] = BoolVar()                              # D9/D10
para cada docente p:
    e[p] = IntVar(0, ∞)                                 # exceso de bloques sobre su tope (S1)
```

> La poda por **turno/plan/año** implementa la semántica **condicional** de D6/D7/D13 (definido ⇒ restringe; ausente ⇒ cualquiera). D11 se garantiza creando `x` **solo** en las franjas del turno **declarado** del curso.

### 3.2 Restricciones duras (RF-1…RF-4)

```
# D1 + D2 + D3 — un docente por materia-curso, solo él la dicta, con sus horas exactas
para cada (c, m):
    Σ_p z[p,c,m] == 1
    para cada p: Σ_{d,s} x[p,m,c,d,s] == horas_m · z[p,c,m]          # horas académicas = bloques

# D4 — un curso no tiene dos clases a la vez
para cada (c,d,s): Σ_{p,m} x[p,m,c,d,s] ≤ 1

# D5 — un profesor no tiene dos clases a la vez
para cada (p,d,s): Σ_{m,c} x[p,m,c,d,s] ≤ 1

# D8 — un aula no aloja dos clases a la vez (fijas + móviles)
para cada (a,d,s):
    Σ_{c móvil} y[c,d,s,a] + Σ_{c fijo con aula(c)=a} Σ_{p,m} x[p,m,c,d,s] ≤ 1

# D9 — un curso móvil ocupa exactamente un aula cuando tiene clase
para cada (c móvil, d, s): Σ_a y[c,d,s,a] == Σ_{p,m} x[p,m,c,d,s]

# D10 — aula fija: no hay variable; la extracción declara c.aula (ver §4)
# D11/D6/D7/D13 — garantizadas por la poda de §3.1
```

### 3.3 Función objetivo (RF-5)

```
Minimizar(
    PESO_HORAS · Σ_p e[p]                                    # S1: exceso de horas de aula
  + Σ_{(p,c,m): p es suplente de m y no titular} COSTE_SUPLENTE · z[p,c,m]   # S2: coste fijo
)
con  Σ_{m,c,d,s} x[p,m,c,d,s] ≤ p.horas_aula + e[p]   # tope + exceso (horas académicas = bloques)
```

- Un **titular** no aporta coste (0). Todo **suplente elegible** cuesta lo mismo (`COSTE_SUPLENTE`); el `peso` del par **se acepta pero no se usa** (diferido).
- La regla «un suplente solo cubre cuando ningún titular con capacidad libre puede» **emerge** de la optimización (con `PESO_HORAS > COSTE_SUPLENTE`).
- **Reproducibilidad (RNF-1):** `SatParameters` con `num_search_workers = 1` y `random_seed = 0`; misma entrada ⇒ mismo resultado (no se desempatan titulares/suplentes equivalentes).
- Nunca se relaja una restricción dura para mejorar el objetivo (RF-5).

---

## 4. Extracción de la solución y contrato de salida (RF-3, RF-6, RF-9, RNF-3)

```
para cada (p,m,c,d,s) con x = true:
    aula = (config.aula_fija ? c.aula : índice a con y[c,d,s,a] = true)   # D10 / D9
    añadir AsignacionOutput{slot=s, materia=m, profesor=p, aula=aula} al día d
clave del curso = claveCurso(c.anio, c.seccion)   # p. ej. "1|A": identidad de datos, NO el nombre derivado
agrupar horarios[clave]; ordenar días y slots de forma estable
metadata: total_asignaciones, cursos_generados, profesores_asignados, tiempo_ejecucion_ms
```

- `AsignacionOutput.aula` es siempre un **índice** del pool de aulas (RF-9); la traducción a nombre es de la interfaz.
- **Clave del curso = identidad** (`anio`+`seccion`, canónica y determinista), nunca un nombre. El nombre derivado (`"1ro A"`) lo calcula **quien muestra**; el solver no lo produce (RF-3, RF-9).
- Fallo: `INFEASIBLE`/`MODEL_INVALID`/`UNKNOWN` devuelven mensajes en español con la **causa** según las categorías de RF-6 (RNF-3), sin inventar datos (RF-6).

---

## 5. Validación previa V1–V15 (RF-11)

Se ejecuta en `SolverConfig::fromJson`, antes de invocar al solver; si alguna falla, **no se invoca**.

- **V1. Estructura:** JSON parseable y con las secciones requeridas.
- **V2. Tipos y rangos:** — **NUEVAS**: `peso` de cada par (docente, materia-suplente) entero **≥ 0**; `duracion_minutos > 0**.
- **V3. Dimensiones** coinciden con los tamaños de las listas.
- **V4/V5/V6.** Índices de materias, profesores y aulas en rango.
- **V7. Turno de curso presente y válido** (obligatorio; si falta → rechazo) y perteneciente al conjunto de turnos.
- **V8. Aula fija válida según el modo global:** con `aula_fija == true`, cada curso **declara** un aula (índice válido, obligatoria); con `aula_fija == false`, el aula del curso **puede estar ausente** (se ignora).
- **V9. `num_estudiantes` (opcional):** **solo si está presente** se comprueba su forma (entero positivo); no tiene regla de negocio.
- **V10. Listas no vacías:** cada curso **declara su plan** y tiene materias (derivadas del plan); cada profesor, materias asignadas o suplentes.
- **V11. Exactamente un plan por curso.**
- **V12. Disponibilidad en rango** de días y bloques.
- **`horas_requeridas` (opcional):** si está presente, `horas_aula ≤ horas_requeridas`; si falta (`-1`), el chequeo se omite.
- **V13.** Cada materia-curso tiene ≥ 1 **candidato elegible** (titular o suplente declarado ∧ plan ∧ turno ∧ año ∧ disponibilidad), evaluado sobre los contratos, **sin** capacidad de horas.
- **V14.** `Σ_m horas_m ≤ nº_días × |franjas del turno(c)|` (1 hora académica = 1 bloque).
- **V15.** Curso móvil con `num_aulas == 0` → rechazo.
- **Versión/parseo (RF-10):** versión ausente o no reconocida (distinta de `1.0`) y fallo de parseo → causa **(1) entrada inválida**.

---

## 6. Construcción de la entrada (RF-13) — pseudocódigo

```
construirJsonEntrada(datos, parametros, ahora):
    # 1. pool global de materias (espacio de índices)
    para m en datos.materias: salida.materias += { nombre: m.nombre }
    # 2. aulas (el aula se referencia por índice)
    para a en datos.aulas: salida.aulas += { nombre, tipo:"regular", capacidad }
    # 3. turnos (slots absolutos por orden de inicio) y recesos — como hoy
    # 4. aula_fija GLOBAL (del preset; por defecto true)
    salida.aula_fija = parametros.aula_fija
    # 5. planes EMBEBIDOS: solo los referenciados por cursos + docentes
    referenciados = { c.plan : c ∈ parametros.cursos }  ∪  { p.plan : p ∈ parametros.profesores, p.plan ≠ "" }
    para codigo en referenciados:
        plan = componerDesdeBD(datos.planes, codigo)   # materias { materia_idx, curso, horas }
        salida.planes += { codigo, materias }
    # 6. cursos: merge BD + preset por identidad (anio, seccion)
    para cBD en datos.cursos:
        cPre = buscarPreset(parametros.cursos, cBD.anio, cBD.seccion)
        salida.cursos += {
            anio = cBD.anio, seccion = cBD.seccion, turno = cBD.turno,
            aula = (salida.aula_fija ? indiceAula(datos.aulas, cBD.aula) : -1),
            plan = cPre.plan, num_estudiantes = cPre.num_estudiantes }
    # 7. docentes: entidad del dominio + parámetros del preset casados por nombre
    para d en datos.docentes:
        prof = { nombre = d.nombre,
                 horas_requeridas/horas_aula/turno/plan/anios/materias_suplente = preset(d.nombre),
                 materias_asignadas = índices de d.materias en el pool,
                 disponibilidad = franjas(d) mapeadas a slots }
        salida.profesores += prof
    # 8. dimensiones derivadas (conteos) + num_dias del preset
    json = salida.toJson(); json["meta"] = { descripcion, fecha_modificacion = ahora }
    devolver json
```

- **Cada curso mezcla dos orígenes (I1)** — de la **BD**: `anio`/`seccion`/`turno`/`aula`; del **preset**: `plan`/`num_estudiantes`. El **plan del curso no se persiste**.
- **Solo se embeben los planes referenciados** (cursos + docentes). El solver resuelve **sin consultar la BD** (§7).
- El **aula se traduce a índice** con el modo fijo; en modo móvil **no se enlazan aulas** por curso.

---

## 7. Interfaz — contrato de consumo (**sin cambios en `src/frontend`**)

La UI la aporta otro equipo (fuera de alcance; spec §Fuera de alcance). Aquí solo se **fija el contrato** y se documenta en `docs/interfaz-frontend.md`.

- **Clave del curso:** la salida llega indexada por la **identidad** (`claveCurso(anio,seccion)`, p. ej. `"1|A"`); la UI **deriva el nombre para mostrar** (`"1ro A"`) al pintar (RF-3, RF-7, RF-9). No hay nombre almacenado ni derivado en la salida.
- **Aula:** cada clase llega con el **índice** de aula (RF-9); la UI traduce índice→nombre.
- **Forma del dato:** `HorarioSalida` conserva la forma (`curso → días → asignaciones con slot, materia, profesor, aula`); cambia el **valor** de la clave (identidad) y la naturaleza del aula (índice). No hay cambio estructural en `toJson`/`fromJson`.

---

## 8. Decisiones técnicas (con alternativa descartada)

| # | Decisión | Alternativa descartada | Motivo |
|---|---|---|---|
| D-1 | **Tope + exceso penalizado** para las horas de aula (S1). | (a) Igualdad dura «cada docente da exactamente sus horas»; (b) minimizar el «déficit». | (a) vuelve INFEASIBLE el problema con docentes «de sobra» (causa de los tests rojos, HU-2); (b) es **inerte** (Σ horas asignadas constante, fijado por D3). |
| D-2 | **Variable de activación `z[p,c,m]`** con `Σ_p z = 1` y `Σ_{d,s} x = bloques·z`. | Solo `Σ_p x[p,m,c,d,s] ≤ 1` por franja. | Esa alternativa permite **repartir** la materia entre varios docentes; D1/D2 exigen que la dicte **exactamente uno**. |
| D-3 | **Curso identificado por `anio`+`seccion`**; se elimina `nombre`. | Conservar `nombre` y validar coherencia. | RF-7: el nombre es **presentación** (spec 004) y no es identidad; guardarlo invita a duplicados. |
| D-4 | `aula_fija` **global** (`bool`, por defecto `true`) en la raíz del contrato. | Campo por curso (contrato anterior) o dentro de `planificacion`. | RF-7/§7.3: el modo es **del centro**, no de cada curso; en la raíz se lee sin ambigüedad y no se mezcla con la planificación. |
| D-5 | Aula referenciada por **índice** (`-1` = móvil). | Nombre de aula. | §7.3/RF-9: el índice es el identificador; el nombre lo traduce la interfaz. |
| D-6 | **Planes embebidos** (`planes[]`) + helper `materiasDelCurso` filtra por `curso == anio`. | (a) Aplanar las materias del curso en `cursos[].materias` al construir; (b) que el solver consulte la BD. | (a) contradice RF-7 («no acepta una lista de materias por curso») y duplica datos; (b) viola §7 (el solver no abre la BD). El filtro por año usa `PlanEstudio_Materia.curso` (año del plan). |
| D-7 | D9 como **igualdad** `Σ_a y = Σ_{p,m} x`. | `Σ_a y ≤ 1` (relajado). | D9 exige **exactamente un** aula por franja con clase; `≤ 1` permitiría clase sin aula. |
| D-8 | Curso de **aula fija sin variable**: la extracción escribe `c.aula` (D10). | Crear también `aulaAssignment` para fijas y fijarla. | Ahorra variables y garantiza siempre su aula; D8 ya impide el solapamiento incluyendo las fijas. |
| D-9 | **Coste de suplente fijo `COSTE_SUPLENTE`**; `peso` aceptado sin uso; `COSTE_MAX_SUPLENTE` reservada. | `costeSuplente(peso)` (fórmula diferida). | Decisión del usuario (2026-10-15): en esta versión el `peso` no afecta al objetivo; la fórmula futura queda documentada. |
| D-10 | `horas_requeridas` **opcional** (`-1` = ausente); solo chequeo de coherencia. | Exigirla siempre. | RF-11: si falta, el chequeo se omite y no invalida; el solver solo usa `horas_aula`. |
| D-11 | **Parámetros deterministas** (`num_search_workers = 1`, `random_seed = 0`). | Búsqueda paralela por defecto. | RNF-1: misma entrada ⇒ mismo resultado. |
| D-12 | Validaciones **en `SolverConfig::fromJson`**, expuestas por `ValidadorEntradaSolver`. | Validar solo en el servicio de generación. | RF-11 y §7: la validación viaja con el contrato y se prueba sin UI ni BD. |
| D-13 | Eliminar `restriccionMateriaUnicaPorAula` (D12). | Conservarla. | Redundante con D8 (aula no solapada); la spec marca D12 como eliminada. |
| D-14 | La **clave de la salida** es `claveCurso(anio,seccion)` (p. ej. `"1|A"`). | Mantener el nombre derivado (`"1ro A"`) como clave. | El solver **no** es responsable de la presentación; la identidad es el dato estable (RF-3, RF-9). |
| D-15 | **1 hora académica = 1 bloque**: `bloques(horas, duracion) = horas` y `bloquesAula(horas_aula, duracion) = horas_aula`. Se conserva la firma con `duracion_minutos` y la guarda `0` si `≤ 0` (defensiva; V2 la rechaza antes). | Cambiar la firma a `bloques(int horas)` y adaptar los 5 call sites. | La firma estable evita un refactor transversal sin valor; `duracion_minutos` sigue siendo dato del contrato y la guarda documenta el caso inválido. La identidad es lo que exige la enmienda de la constitución §7 (hora académica, no hora de reloj). |

---

## 9. Estrategia de tests

Comando del proyecto (§4, AGENTS.md), **en el contenedor** (OR-Tools solo está ahí):

```bash
docker exec gestor-dev bash -lc "cd /workspace && cmake --preset full && cmake --build build && ctest --preset full --output-on-failure"
```

Política: **tests primero** (rojo → código → verde → marcar). Un caso por regla; RNF-4 (trazabilidad).

| Test | Framework | Qué cubre | RF |
|---|---|---|---|
| `test/backend/test_contrato_solver.cpp` **(nuevo)** | Qt Test | `bloques`/`bloquesAula` son la **identidad** (`bloques(3,45) == 3`, `bloques(2,45) == 2`, `bloques(4,40) == 4`, `bloques(6,60) == 6`), con la guarda `0` si `duracion_minutos ≤ 0`; `claveCurso` canónica, constantes `PESO_HORAS > COSTE_SUPLENTE > 0`. | RF-1, RF-3, RF-5 |
| `test/backend/test_validador_entrada.cpp` **(modificado)** | Qt Test | V1–V15: `peso` negativo, `duracion_minutos ≤ 0`, `aula_fija` global + `aula` índice (V8), `num_estudiantes` ausente/presente (V9), V7 (turno obligatorio), V10/V11 (plan obligatorio/único), V13 (sin candidato por plan/turno/año/disponibilidad), V14 (más horas académicas que franjas), V15 (móvil sin aulas), versión ausente/no reconocida. **Se retira la inyección `TODO(spec 005)`** (las materias ya vienen del plan). | RF-7, RF-8, RF-10, RF-11 |
| `test/backend/test_cargador_configuracion.cpp` **(modificado)** | Qt Test | Ida y vuelta `fromJson(toJson())` con `anio`/`seccion`, `aula_fija` global, `planes` embebidos, `anios` de docente y `peso`; el JSON serializado **no** contiene `nombre`; campos desconocidos **tolerados**; versión ausente/no reconocida → rechazo. | RF-7, RF-8, RF-10 |
| `test/backend/test_constructor_entrada.cpp` **(modificado)** | Qt Test | El JSON emitido: curso con `anio`/`seccion`/`turno`/`aula` (índice) y `plan`/`num_estudiantes`; materias del curso **derivadas del plan**; `planes` **solo** los referenciados; `aula_fija` global; **consumible** por `fromJson`; pureza con instante inyectado. **Se retira la inyección `TODO(spec 005)`**. | RF-7, RF-8, RF-10, RF-13 |
| `test/backend/test_composicion_plan.cpp` **(verificación)** | Qt Test | Sigue en verde: la composición de planes desde `PlanEstudio_Materia` (materias+horas+año) es la fuente de los planes embebidos. | RF-13 |
| `test/backend/test_servicio_generacion.cpp` **(modificado)** | Qt Test | **Se retira la inyección `TODO(spec 005)`**: la entrada construida ya es válida sin parches (plan/materias/`aula_fija`). `huellaDatos`/`estamparFechaGeneracion` puras. | RF-6, RF-13 |
| `test/backend/test_variables_cp_sat.cpp` **(modificado)** | Google Test | Poda por **años**, creación de `asignacionDocente`, aulas fijas/móviles según `aula_fija` global, disponibilidad y turnos; conteos exactos. | RF-1, RF-3, RF-4 |
| `test/backend/test_solver_horarios.cpp` **(reescrito)** | Qt Test | D1 (un docente por materia-curso, XOR), D2, D3 (bloques), D4/D5, D8, D9/D10 (enlace y aula fija), D11/D13, **S1** (con docentes «de sobra» **hay** solución; exceso como último recurso), **S2** (titular antes que suplente; titular a tope → cubre suplente), clave = identidad, aula = índice, **reproducibilidad**. | RF-1…RF-9, RF-12, RNF-1 |
| `test/backend/test_validador_salida.cpp` **(modificado)** | Qt Test | El análisis de salida casa por la **identidad del curso** (`anio`+`seccion`) (P1/P2/P4), no por un nombre. | RF-3, RF-9 |
| `test/backend/test_solver_benchmark.cpp` **(reescrito)** | Qt Test | Escenario de gran tamaño resuelto en **< 60 s** y consistencia (profesor/aula sin solapamiento). | RF-12, RNF-2 |
| `test/test_ipc_aislamiento_bd.cpp` **(verificación)** | Qt Test | **Se cierra en rojo→verde**: la entrada construida por el cliente ya es válida y el proceso de cálculo no abre la BD. | RF-13, RF-6 |

`test/CMakeLists.txt` **(modificado):** registra `test_contrato_solver` con `add_qtest(... backend)` (sin OR-Tools); los tests del solver siguen en `add_qtest/add_gtest(... backend_solver)`.

Alcance de las reescrituras (RF-12): las pruebas del contrato anterior (`nombre` almacenado, `aula_fija` entero/campo de curso, materias por curso) se reescriben al contrato nuevo **verificando el comportamiento correcto nuevo**, no ajustándolas para pasar.

---

## 10. Trazabilidad RF → partes del plan

| RF | Plan (secciones y archivos) |
|---|---|
| RF-1 (D1–D3) | §1.2 `variables.cpp` (z) + `restricciones_profesores.cpp` (D1/D2) + `restricciones_turnos.cpp` (D3) + `contrato_solver.cpp` (`bloques`) |
| RF-2 (D4/D5/D8) | `restricciones_turnos.cpp` (D4), `restricciones_profesores.cpp` (D5), `restricciones_aulas.cpp` (D8) |
| RF-3 (D9/D10, clave/aula) | `restricciones_aulas.cpp` (D9), `solver.cpp` (extracción, `claveCurso`), `ValidadorSalidaSolver.cpp` |
| RF-4 (D6/D7/D11/D13) | `variables.cpp` (poda) + `restricciones_profesores.cpp` (D13) + `solver_config.cpp` (validación) |
| RF-5 (S1/S2) | `restricciones_turnos.cpp` (tope+exceso) + `solver.cpp` (objetivo) + `contrato_solver.hpp` (constantes) |
| RF-6 (contrato/comportamiento) | `solver.cpp` (factibilidad, causa, determinismo), `servicio_solver.cpp` |
| RF-7 (cursos) | `solver_config.hpp/.cpp` + `ConstructorEntradaSolver.cpp` (campos persistidos de la 004 + preset); el nombre derivado es **presentación** |
| RF-8 (docentes) | `solver_config.hpp/.cpp` (`anios`, `peso`) + `ConstructorEntradaSolver.cpp` |
| RF-9 (índice de aula) | `solver.cpp` (extracción), `ValidadorSalidaSolver.cpp` |
| RF-10 (serialización/versión) | `solver_config.cpp` (`fromJson`/`toJson`, versión, tolerancia), `ConstructorEntradaSolver.cpp` |
| RF-11 (V1–V15) | `solver_config.cpp` (validaciones) + `ValidadorEntradaSolver.cpp` |
| RF-12 (pruebas verdes) | §9 tests reescritos (`test_solver_horarios`, `test_solver_benchmark`, `test_constructor_entrada`, `test_validador_entrada`, `test_ipc_aislamiento_bd`) |
| RF-13 (construcción de la entrada) | `ConstructorEntradaSolver.{hpp,cpp}` + `DatosDominio` (planes compuestos) |
| RNF-1 | `solver.cpp` (parámetros deterministas) |
| RNF-2 | `test_solver_benchmark.cpp` (< 60 s) |
| RNF-3 | `solver.cpp` (`analizarFallo`), `solver_config.cpp` (mensajes) |
| RNF-4 | §9 y §10 (cobertura verificable por RF) |

---

## 11. Notas y fuera de alcance heredado

- **Persistencia (spec 004):** no se toca la tabla `Cursos`; los campos `anio`/`seccion`/`turno`/`aula` y los servicios de cursos son de la **spec 004** (implementada). El único contacto es que `ConstructorEntradaSolver` **lee** los campos ya persistidos y compone los planes.
- **UI (`src/frontend`):** fuera de alcance; solo se documenta el contrato de consumo (§7) en `docs/interfaz-frontend.md`.
- **Turnos automáticos (006) y generación parcial (007):** fuera de alcance. El `turno` del curso es **obligatorio** (V7).
- **Capacidad de aula y cobertura de emergencia:** fuera del catálogo (no se implementan).
- **Reparto equilibrado mín-máx** y **desempate entre titulares/suplentes equivalentes:** diferidos.
- **`peso` en el objetivo** y la fórmula `COSTE_MAX_SUPLENTE − min(peso, …)`: diferidos; aquí el coste es fijo.
- **`docs/solver/`:** referencia histórica; no obliga.
