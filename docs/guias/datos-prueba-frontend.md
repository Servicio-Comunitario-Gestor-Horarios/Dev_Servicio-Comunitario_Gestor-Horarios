# Datos de prueba del centro — guía para el equipo de frontend

> Complemento de `docs/guias/datos-prueba.md` (guía del seed) orientado al **equipo de
> frontend**. Resume **qué cambió** para la interfaz con la spec 008 (seed del Plan 31059) y la
> rectificación 005-T11 (**hora académica = bloque**), y **qué debe hacer la interfaz** con los
> datos sembrados y con los cursos sin turno.
>
> No repite la guía del seed: para compilar, aplicar, variantes, seguridad y códigos de salida del
> CLI, ver `docs/guias/datos-prueba.md`. El contrato de consumo sigue siendo
> `docs/interfaz-frontend.md`.

---

## 1. Qué cambió para el frontend: la hora académica es un bloque

**Regla nueva (005-T11, constitución §7):** la carga de una materia se mide en **horas
académicas**, y **1 hora académica = 1 bloque**. Ya **no** existe la conversión
`ceil(horas × 60 / duracion_minutos)`.

| Dato | Significado para el frontend |
|---|---|
| `PlanEstudio_Materia.horas` | **Horas académicas semanales = bloques.** El número que ves ya es el número de bloques. |
| `duracion_minutos` (40 en el conjunto) | **Solo la duración real de la franja** (rejilla horaria, eventos y disponibilidad). **No** es factor de conversión. |
| Conversión | **No multiplicar por 60** ni **dividir por `duracion_minutos`**. `bloques(horas) = horas`. |

**Cómo mostrarlo:**

- Un año suma **40 horas académicas/semana** → muéstralo tal cual: «40 h académicas/semana».
- Si se quiere el **tiempo real**, la conversión es **para presentación, no para el solver**:
  40 bloques × 40 min = 1600 min = **26 h 40 min**.

> `duracion_minutos` **no es una columna de la base**: es un campo de la configuración del solver
> (el preset). La base solo guarda `Turnos.slots` (= 8), `Turnos.inicio` y `Turnos.fin`. El valor
> de 40 min lo declara el preset que acompañe al conjunto (ver §4).

Referencias: `docs/interfaz-frontend.md` (contrato) · `specs/005-nucleo-solver/spec.md` (D3, S1,
V14 y definiciones de «hora académica», «bloque» y `duracion_minutos`) · `docs/constitution.md` §7.

---

## 2. Qué hay en la base sembrada (recuentos exactos)

> **Nota (spec 009):** la base sembrada ya trae **eventos** (nombre, posición, orden y duración) y el
> catálogo de **tipos de aula**. La interfaz **aún no ha migrado** a ese contrato
> (`docs/interfaz-frontend.md` §5): sus formularios no envían `idTipoAula` y sus vistas todavía no
> consumen eventos ni tipos de aula. Los recuentos de esta sección describen **los datos de la base**,
> no las firmas que la interfaz compila hoy.

| Entidad | Cantidad | Detalle |
|---|---|---|
| Planes de estudio | 4 | `GEN` General · `EXA` Ciencias Exactas · `NAT` Ciencias Naturales · `SOC` Ciencias Sociales |
| Materias-año (catálogo) | 57 | 51 del 31059 + 6 nuevas de los derivados (compartidas por los 4 planes) |
| Cargas plan-materia | 210 | tabla `PlanEstudio_Materia` (`codigo_PlanEstudio`, `id_Materia`, `curso`, `horas`) |
| Cursos | 13 | 3 secciones en 1.º–3.º (A/B/C) y 2 en 4.º–5.º (A/B); **todos con `turno` NULL** |
| Aulas | 16 | 13 para cursos (Aula 01…13) + reserva + laboratorio + taller |
| Turnos | 2 | Mañana 07:00–12:30 · Tarde 12:40–18:10, **8 franjas de 40 min** |
| Eventos | 2 | evento «Receso» de 10 min tras la 2.ª franja: 08:20–08:30 (Mañana) y 14:00–14:10 (Tarde) |
| Tipos de aula | 3 | catálogo `Aula de estudio` · `Laboratorio` · `Taller`; toda aula y materia referencia uno |

**Planes (materias por año y carga):**

| Plan | Materias por año (1.º–5.º) | Carga por año |
|---|---|---|
| `GEN` | 9 / 9 / 10 / 11 / 12 | 40/40/40/40/40 h académicas |
| `EXA` | 9 / 9 / 10 / 12 / 13 | 40/40/40/40/40 h académicas |
| `NAT` | 9 / 9 / 10 / 12 / 13 | 40/40/40/40/40 h académicas |
| `SOC` | 9 / 9 / 10 / 12 / 13 | 40/40/40/40/40 h académicas |

**Turnos (rejilla aritmética, spec 005):** los bloques se anclan en el `inicio` del turno y
avanzan en pasos de 40 min; **el evento «Receso» no desplaza bloques**. La rejilla termina en **12:20**
(Mañana) y **18:00** (Tarde), mientras que `Turnos.fin` guardado es 12:30/18:10 porque incluye el
evento «Receso» (desfase de 10 min asumido y documentado en la spec 008).

**Las 4 variantes de docentes** (el resto de los datos es idéntico entre variantes):

| Variante | Docentes | Materias asignadas | Disponibilidades | Estado |
|---|---|---|---|---|
| `holgada` (por defecto) | 65 | 65 | 325 | válida |
| `suplentes` | 43 | 57 | 215 | válida |
| `sin-profesorado` | 64 | 64 | 320 | **inválida a propósito** (V13) |
| `sin-disponibilidad` | 65 | 65 | 325 | **inválida a propósito** (V13) |

---

## 3. Cómo abrir y usar la base sembrada

El seed escribe **archivos de base SQLite**. Sugerencia de ubicación para desarrollo (la carpeta
`datos-prueba/` está ignorada por git en `.gitignore`):

```bash
mkdir -p datos-prueba
gestor-horarios-seed --db datos-prueba/centro-31059-holgada.db --variante holgada
gestor-horarios-seed --db datos-prueba/centro-31059-suplentes.db --variante suplentes
```

> `--db <ruta>` acepta cualquier ruta; el nombre `centro-31059-*.db` es solo una convención
> sugerida para los archivos **generados** (no se versionan).

**Base que usa la aplicación (interfaz real):**

```
~/.local/share/LiceoRobertSerra/GestorHorarios/gestor_horarios.db
```

(`ContextoBaseDatos::rutaPorDefecto()` = `AppDataLocation` + `gestor_horarios.db`; org
`LiceoRobertSerra`, app `GestorHorarios`.) Si vas a **copiar una base sembrada encima de la base de
la app**, **haz respaldo antes**:

```bash
cp ~/.local/share/LiceoRobertSerra/GestorHorarios/gestor_horarios.db \
   ~/.local/share/LiceoRobertSerra/GestorHorarios/gestor_horarios.db.respaldo
```

> **Importante:** el seed **no genera presets** del solver. Solo puebla la base. El turno y el plan
> de cada curso, el `num_estudiantes`, los filtros de los docentes y las suplencias viven en el
> preset (spec 005). Ver `docs/guias/datos-prueba.md` §6.

---

## 4. Qué debe hacer el frontend con los cursos sin turno

Los **13 cursos se siembran con `turno` NULL** (a nivel de `NucleoDatos`, `CursoDTO.turno` vacío y
`CursoDTO.aula` = id del aula asignada, nunca `-1`). El **turno** y el **plan** no se persisten en
los datos: **los aporta el preset** (archivo JSON de configuración del solver, spec 005) al generar.

| Campo del curso | Origen | Obligatorio |
|---|---|---|
| `anio` / `seccion` / `aula` | **Base de datos** (ya sembrados) | sí |
| `plan` | **Preset** | **sí** (sin él, V10/V11 rechaza) |
| `turno` | **Preset** | **sí** (sin él, V7 rechaza) |
| `num_estudiantes` | **Preset** | no (opcional, V9) |
| modo de aula fija | **Preset** (global, por defecto **activo**) | no |

- El **aula ya viene de la base** y el **modo de aula fija es global** (activo por defecto), así que
  con el modo activo el aula se toma de la base **sin que el preset la repita**.
- Mientras **no exista preset**, la interfaz **debe mostrar los cursos «sin turno»** (p. ej. «Sin
  turno» en la columna de turno) y **no** asumir mañana/tarde.
- **Sin `plan` ni `turno` en el preset, el solver rechaza la entrada** (V7 para el turno, V10/V11
  para el plan). Cualquier reparto válido sirve; la tabla siguiente es **solo un ejemplo** (no
  obligatoria):

| Curso | Plan (ejemplo) | Turno (ejemplo) |
|---|---|---|
| 1º A | `GEN` | Mañana |
| 1º B | `EXA` | Mañana |
| 1º C | `NAT` | Mañana |
| 2º A | `SOC` | Mañana |
| 2º B | `GEN` | Mañana |
| 2º C | `EXA` | Mañana |
| 3º A | `NAT` | Tarde |
| 3º B | `SOC` | Tarde |
| 3º C | `GEN` | Tarde |
| 4º A | `EXA` | Tarde |
| 4º B | `NAT` | Tarde |
| 5º A | `SOC` | Tarde |
| 5º B | `GEN` | Tarde |

> Cualquier reparto plan/turno es válido **siempre que** el preset declare `plan` y `turno` para los
> 13 cursos y el `num_estudiantes` (si se incluye) sea entero positivo. Ver detalles de las
> validaciones en `specs/005-nucleo-solver/spec.md` RF-7, RF-11 (V7–V11) y `docs/constitution.md` §7.3.

---

## 5. Variantes de docentes y pantallas de error

Cuatro variantes intercambiables (ver §2). Las dos últimas son **deliberadamente inválidas** para
ejercitar la **validación previa V13** del solver:

- `holgada` y `suplentes` → **válidas**: generan horario sin rechazo de cobertura.
- `sin-profesorado` → `Física 3º` queda **sin ningún docente** (V13: sin candidato elegible, en
  ambos turnos).
- `sin-disponibilidad` → los docentes de `Física 3º` tienen disponibilidad **19:00–21:00** (solape
  nulo con ambos turnos) (V13).

**Alternar variante sin tocar el resto de los datos** (solo recarga docentes, materias y
disponibilidad; exige `--si` y deja respaldo):

```bash
gestor-horarios-seed --db datos-prueba/centro-31059.db --variante suplentes --solo-docentes --si
```

**Qué debe mostrar el frontend:** cuando la generación falle con las variantes inválidas, el
`ResultadoGeneracion` (spec 002) llega con un estado de rechazo de validación previa
(`ContratoInvalido` / `EntradaInvalida`), con `mensaje` en español identificando la causa. La
interfaz debe **informar la causa** (materia-año sin candidato elegible) y **no ofrecer guardar** el
horario. Ver `docs/interfaz-frontend.md` §3.1 («EntradaInvalida», «ContratoInvalido»).

> Nota: el seed **no produce el veredicto del solver**; comprobar V13 o generar un horario requiere
> un preset de prueba (aportado por las pruebas), que el conjunto **no genera** (spec 008, nota de
> RF-8).

---

## 6. Checklist de integración

- [ ] **No reconvertir horas.** `PlanEstudio_Materia.horas` ya son bloques; no multiplicar por 60 ni
      dividir por `duracion_minutos`. Mostrar «40 h académicas/semana» (y 26 h 40 min solo como dato
      de tiempo real).
- [ ] **Mostrar «sin turno»** para los 13 cursos mientras no exista preset (no asumir mañana/tarde).
- [ ] **Respetar `duracion_minutos` al pintar la rejilla** (40 min por bloque, anclada en el `inicio`
      del turno) y recordar que el **evento «Receso» no desplaza los bloques**.
- [ ] **Eventos como metadata:** los eventos del turno (`despues_de_slot`, `orden`, `duracion`) son
      metadata del turno; el solver los ignora (constitución §7.5). No descuentes su duración de las
      franjas útiles.
- [ ] **No asumir que los cursos tienen turno ni plan en la base:** ambos los aporta el preset al
      generar; sin ellos, el solver rechaza (V7, V10/V11).
- [ ] **Aula fija global:** el aula ya viene de la base (cada curso con aula distinta) y el modo de
      aula fija es global (activo por defecto); no pedir aula por curso al usuario del preset.
- [ ] **Usar los recuentos para las pruebas:** 4 planes, 57 materias, 210 cargas, 13 cursos, 16
      aulas, 2 turnos/2 eventos, 3 tipos de aula y las variantes 65/43/64/65.
- [ ] **Enlazar la documentación:** remitir a `docs/guias/datos-prueba.md` (uso del seed) y a
      `specs/005-nucleo-solver/spec.md` (contrato del solver).

---

## 7. Resumen de cambios

| Cambio | Archivos/alcance |
|---|---|
| 005-T11 — rectificación **hora académica = bloque** (`bloques(horas) = horas`) | `src/backend/.../contrato_solver.{hpp,cpp}`; tests `test_contrato_solver.cpp`, `test_validador_entrada.cpp`, `test_solver_horarios.cpp` |
| 008 — seed de datos de prueba (Plan 31059) | `src/seed/` (`perfil`, `docentes`, `sembrador`, `main`), CLI `gestor-horarios-seed`, tests `test_seed*` |
| Constitución §7 — enmienda (bloque, `duracion_minutos` solo duración real) | `docs/constitution.md` |
| Documentación del seed (uso, variantes, recuentos, «no genera presets») | `docs/guias/datos-prueba.md` |
