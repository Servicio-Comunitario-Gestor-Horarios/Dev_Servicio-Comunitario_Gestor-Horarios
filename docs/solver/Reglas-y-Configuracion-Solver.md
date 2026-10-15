# Reglas y configuración del solver (actualizado)

> **Documento de referencia consolidado.** Refleja el comportamiento **vigente** del solver de
> horarios (OR-Tools CP-SAT). La fuente de verdad es `docs/constitution.md` §7 y las specs
> `specs/004-persistencia-campos-curso` y `specs/005-nucleo-solver`. Los **demás** documentos de
> `docs/solver/` son referencia **histórica** y no obligan.
>
> Última actualización: 2026-10-15 (rediseño del modelo de curso: catálogo estable en BD + parámetros
> de cálculo en la configuración del solver).

## 1. Frontera del solver

- El solver es la **única** pieza que decide el horario. **No** abre la base de datos ni depende de la
  interfaz.
- Recibe una **configuración ya validada** y devuelve una asignación **o** un fallo con causa. No
  completa ni corrige la entrada.
- Para la misma entrada, el resultado es **reproducible**.

## 2. Modelo persistido (base de datos del cliente)

El solver **no** lee la BD; lo hace el cliente y lo empaqueta en la entrada. En la BD vive solo el
**catálogo estable**:

| Entidad | Notas |
|---|---|
| `Cursos` | `(id, anio, seccion, turno, aula, fecha_*)`. **Sin** `nombre`, `num_estudiantes`, `codigo_plan` ni `aula_fija`. `(anio, seccion)` es **único en todo el sistema**. `aula` es **FK a `Aulas(id)`** (opcional). |
| `Aulas` | Catálogo de aulas. |
| `Profesores` | Docentes (materias titular, disponibilidad, …). |
| `Materias` | Catálogo de materias. |
| `PlanEstudio` / `PlanEstudio_Materia` | Planes de estudio y sus materias+horas por año. |
| `Turnos` / `Recesos` | Turnos y sus recesos. |

- **No existe** la relación persistida **curso–materia** (`Curso_Materia`): las materias de un curso
  se derivan de su **plan**.
- **Esquema único:** la base se crea entera de una sola vez; **no** hay pasos versionados ni migración
  de datos.

## 3. Configuración del solver (entrada JSON)

La entrada se construye a partir de los **datos persistidos** (cursos, aulas, docentes, materias,
turnos y **planes**) **más** un **preset** (parámetros de cálculo). El preset es una configuración
completa y reutilizable.

### 3.1 Parámetros globales

| Campo | Significado |
|---|---|
| `aula_fija` | **Booleano global** (por defecto **`true`**). `true` → cada curso usa su aula fija; `false` → aulas móviles (las decide el solver). **No** es un campo por curso. |
| `franja_horaria.duracion_minutos` | Duración de cada franja. Fija la conversión horas↔bloques. |
| `dimensiones` | `num_dias`, `num_slots_dia` (derivadas/validadas). |
| `generacion` / `penalizaciones` | Parámetros de generación y penalizaciones. |

### 3.2 Cursos (por curso/sección)

- `anio` (entero) + `seccion` (un carácter) → **identidad**. El **nombre** se deriva (`"1ro A"`) y es
  **presentación** (lo calcula quien muestra, no el solver).
- `turno` (**opcional**; ver §5 «turno automático», diferido).
- `num_estudiantes`.
- `plan` (código del plan; de él se derivan sus **materias y horas**).
- `aula`: proviene de los **datos del curso** (BD); se usa cuando `aula_fija` está activo. Se traduce a
  **índice** de aula.

### 3.3 Planes embebidos

La entrada **incluye los planes de estudio** (con sus materias y horas semanales) **embebidos**, para
que el solver resuelva **sin tocar la BD**. Se embeben los planes **referenciados** por los cursos
presentes.

### 3.4 Aulas, turnos y recesos

- **Aulas:** se referencian por su **índice** en el conjunto de aulas (no por nombre). La interfaz/CSV
  traducen índice→nombre.
- **Turnos:** nombre + franjas (slots absolutos del día, por orden de inicio).
- **Recesos:** metadata; el solver los ignora.

### 3.5 Profesores

- `nombre`, `materias_asignadas` (**titular**), `materias_suplente` (**lista de pares
  `{materia, peso}`**; a mayor `peso`, más preferido).
- `turno` y `plan` (opcionales) → filtran su elegibilidad.
- `anios` (años a los que puede dictar; **vacío = cualquiera**).
- `horas_aula` (**tope** del contrato). `horas_requeridas` es **dato de dominio opcional** que el
  solver **no usa** (solo un chequeo de coherencia).

## 4. Reglas del modelo (CP-SAT)

### 4.1 Variables

| Variable | Significado |
|---|---|
| `assignment[p,m,c,d,s]` ∈ {0,1} | El prof. `p` enseña la materia `m` del curso `c`, día `d`, franja `s`. |
| `aulaAssignment[c,d,s,a]` ∈ {0,1} | El curso `c` (móvil) usa el aula `a` en (día, franja). |
| `asignado[p,m,c]` ∈ {0,1} | El prof. `p` es **el** docente de `(m,c)`. |

Conversión de horas a **bloques**: `bloques(horas) = ceil(horas × 60 / duracion_minutos)`. **Toda
comparación de horas se hace en bloques**.

### 4.2 Invariantes duras (el solver nunca las viola)

- **D1.** Cada materia-curso la dicta **exactamente un** profesor (no se reparte).
- **D2.** Solo el docente asignado a una materia-curso recibe clases de esa materia-curso.
- **D3.** Cada materia-curso recibe **exactamente** sus horas semanales en bloques.
- **D4.** Un curso no tiene dos clases a la vez.
- **D5.** Un profesor no tiene dos clases a la vez.
- **D6.** Un docente no da clase fuera de su turno (si lo tiene definido).
- **D7.** Un docente no da clase de cursos de otro plan (si lo tiene definido).
- **D8.** Un aula no aloja dos clases a la vez.
- **D9.** Con `aula_fija` **inactivo**, un curso móvil ocupa **exactamente un** aula en cada franja en
  que tiene clase.
- **D10.** Con `aula_fija` **activo**, todo curso usa siempre su aula fija.
- **D11.** Ninguna clase de un curso sale de las franjas de su turno.
- **D12.** *(Eliminada: era redundante con D8.)*
- **D13.** Un docente no da clase de cursos fuera de sus años.

### 4.3 Prioridades blandas y función objetivo

El solver minimiza una penalización **entera** y **nunca** viola una dura para mejorarla.

- **S1 — no exceder el tope de horas de aula.** Cada docente respeta
  `bloques_aula(p) = ceil(horas_aula(p) × 60 / duracion_minutos)`; si la demanda lo obliga, se permite
  un **exceso** penalizado con `PESO_HORAS` y se minimiza. *(Minimizar «déficit» es inerte; por eso el
  modelo es tope + exceso.)*
- **S2 — titulares antes que suplentes.** Un titular cuesta 0; un suplente cuesta un entero positivo
  que **decrece con su `peso` de par**. Regla emergente: un suplente solo cubre cuando ningún titular
  elegible puede.

Constantes **enteras del motor**: `PESO_HORAS` y `coste_máximo_de_suplente`, con
`PESO_HORAS > coste_máximo_de_suplente > 0`. El `peso` de cada par es **dato de la entrada**.

### 4.4 Modos de aula (global)

- **`aula_fija = true` (por defecto):** cada curso usa su aula (de la BD), traducida a índice.
- **`aula_fija = false`:** los cursos son **móviles**; el solver les asigna aula (D9). *(Diferido: «en
  aulas móviles el profesor es fijo en un salón».)*

### 4.5 Validaciones previas (si alguna falla, no se invoca al solver)

- **V1–V12.** Estructura y tipos · coherencia de dimensiones · índices en rango · turnos válidos ·
  aula fija válida (según el modo global) · número de estudiantes · materias/profesores no vacíos · un
  plan por curso · disponibilidad en rango. (Incluye `peso` de suplente **no negativo**.)
- **V13.** Cada materia-curso tiene **al menos un candidato elegible** (titular o suplente) que respete
  plan, turno, año y disponibilidad.
- **V14.** Un curso no necesita más bloques que franjas útiles tiene su turno.
- **V15.** Si el modo de aulas móviles está activo y el centro no dispone de aulas → rechazo.

### 4.6 Salida

- La **clave del curso** en el horario y en el archivo exportado es su **identidad** (`anio` +
  `seccion`), **no** el nombre derivado.
- El **aula** de cada clase se informa como **índice** en el conjunto de aulas (aula fija, o la
  asignada si el curso es móvil).
- El **nombre derivado** lo calcula **quien muestra** (frontend/consola), no el solver.

## 5. Diferido / fuera de alcance

- **Turno automático (toggle de turno, spec 006):** si el curso **no** declara turno aplicable, el
  **solver elige** el turno. Si aplica, se usa el turno de la BD. *(Diferido.)*
- **Generación parcial** y horarios previos (spec 007).
- **Capacidad de aula** y **cobertura de emergencia**.
- **Reparto equilibrado (min-max)** de la carga entre docentes.
- **Profesor fijo en un salón** en modo de aulas móviles.
- La **interfaz Qt** y la traducción de índice de aula a nombre (otro equipo).
