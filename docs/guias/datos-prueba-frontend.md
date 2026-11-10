# Datos de prueba del centro — guía para el equipo de frontend

> Complemento de `docs/guias/datos-prueba.md` (guía del seed) orientado al **equipo de
> frontend**. Resume **qué cambió** con la spec 017 (**centro real del Liceo Nacional Robert
> Serra, 2026/2027**), **qué hay en la base sembrada** y **qué debe hacer la interfaz** con unos
> cursos que **ya traen su turno**.
>
> No repite la guía del seed: para compilar, aplicar, seguridad y códigos de salida del CLI, ver
> `docs/guias/datos-prueba.md`. El contrato de consumo sigue siendo
> `docs/interfaz-frontend.md`.

---

## 1. La hora académica es un bloque (y ahora dura 40 min)

**Regla vigente (005-T11, constitución §7):** la carga de una materia se mide en **horas
académicas**, y **1 hora académica = 1 bloque**. No existe la conversión
`ceil(horas × 60 / duracion_minutos)`.

| Dato | Significado para el frontend |
|---|---|
| `PlanEstudio_Materia.horas` | **Horas académicas semanales = bloques.** El número que ves ya es el número de bloques. |
| `duracion_minutos` (**40** en el centro) | **Solo la duración real de la franja** (rejilla horaria, eventos y disponibilidad). **No** es factor de conversión. |
| Conversión | **No multiplicar por 60** ni **dividir por `duracion_minutos`**. `bloques(horas) = horas`. |

**Cómo mostrarlo:**

- Cada año suma lo que declara su plan: **34/34/34/36/34** horas académicas en **31059** y
  **40/40/40/40/40** en **31060**. Muéstralo tal cual («34 h académicas/semana»).
- El **tiempo real** es solo de presentación, con `duracion_horaria::minutosDeHorasAcademicas`:
  34 bloques × 40 min = **1360 min** (22 h 40), 36 × 40 = **1440 min** (24 h) y 40 × 40 =
  **1600 min** (26 h 40).

> `duracion_minutos` vive en la base, en la configuración global `ConfiguracionHoraria` (una fila;
> en el centro, **40**). Se edita con `NucleoDatos::guardarConfiguracionHoraria()` y el campo del
> preset es **derivado**: la construcción lo sobrescribe siempre con el valor de la configuración y
> nunca lo usa. `Turnos` guarda `slots` (= 8), `inicio` y `fin`.

Referencias: `docs/interfaz-frontend.md` §2.4.2 y §2.4.4 · `specs/005-nucleo-solver/spec.md`
(D3, S1, V14 y definiciones de «hora académica», «bloque» y `duracion_minutos`) ·
`docs/constitution.md` §7.

---

## 2. Qué hay en la base sembrada (recuentos exactos)

| Entidad | Cantidad | Detalle |
|---|---|---|
| Planes de estudio | 2 | **31059** Media General · **31060** Ciencias y Tecnología |
| Materias-año (catálogo) | 85 | 79 con carga + 6 sin carga; **16 compartidas** entre planes |
| Cargas plan-materia | 95 | tabla `PlanEstudio_Materia` (`codigo_PlanEstudio`, `id_Materia`, `curso`, `horas`) |
| Cursos | **24** | 12 de mañana y 12 de tarde; **todos con `turno`** y aula de referencia |
| Aulas | 13 | 12 generales de capacidad 44 (compartidas entre turnos) + 1 cancha de 88 |
| Turnos | 2 | Mañana 07:00–12:40 · Tarde 12:40–18:10; **8 franjas de 40 min** |
| Eventos | 3 | receso 08:20–08:30 y acto cívico 08:30–08:40 (mañana); receso 14:00–14:10 (tarde) |
| Tipos de aula | 2 | general del esquema (`Aula de estudio`) + `Cancha`; toda aula y materia referencia uno |
| Docentes | 33 | 272 materias asignadas y 165 disponibilidades; tope de 36 horas académicas; 16 en la mañana y 17 en la tarde |
| Matrícula | 889 | 579 de 31059 + 310 de 31060; 32–44 por curso (44 en 4.º A y 4.º B) |
| Preferencias | 0 | el conjunto no siembra preferencias |
| Clases | 876 | 96 de Educación Física (48 por turno); 414 de mañana y 462 de tarde |

**Planes (materias por año y carga):**

| Plan | Materias por año (1.º–5.º) | Carga por año |
|---|---|---|
| `31059` | 8 / 8 / 9 / 10 / 10 | 34/34/34/36/34 h académicas |
| `31060` | 10 / 10 / 10 / 10 / 10 | 40/40/40/40/40 h académicas |

- **Compartidas:** Matemáticas 1.º–5.º, Educación Física 1.º–5.º, Física 3.º–5.º y Química
  3.º–5.º. Se siembran una sola vez con la carga de cada plan.
- **6 materias sin carga:** «Participación en los Grupos de Creación, Recreación y Producción» de
  1.º a 5.º y «Ciencias de la Tierra» de 5.º; no están en ningún plan.
- **5.º de 31060** se declara con sus 10 áreas y 40 h, pero **no tiene cursos** en este periodo.
- **Tipos:** todas las materias usan el tipo general salvo Educación Física, que exige `Cancha`.

**Turnos (rejilla aritmética):** los bloques se anclan en el `inicio` del turno y avanzan en pasos
de **40 min**; los eventos **no desplazan bloques**. La rejilla de clases termina en **12:20**
(Mañana) y **18:00** (Tarde) (`8 × 40 = 320`), mientras que `Turnos.fin` guardado es **12:40/18:10**
porque añade los eventos (20 min de mañana y 10 de tarde): `8 × 40 + 20 = 340` y
`8 × 40 + 10 = 330`.

---

## 3. Cómo abrir y usar la base sembrada

El seed escribe **archivos de base SQLite**. Sugerencia de ubicación para desarrollo (la carpeta
`datos-prueba/` está ignorada por git en `.gitignore`):

```bash
mkdir -p datos-prueba
build-backend/src/seed/gestor-horarios-seed --db datos-prueba/centro-2026-2027.db
```

> `--db <ruta>` acepta cualquier ruta; el nombre `centro-2026-2027.db` es solo una convención
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

> **Importante:** el seed **no genera presets** del solver. Solo puebla la base. El **plan de cada
> curso**, el **número de estudiantes**, los filtros de los docentes y las suplencias viven en la
> configuración; el **turno** y el **aula** ya vienen de la base.

---

## 4. Qué debe hacer el frontend con los cursos: ya traen turno

Los **24 cursos se siembran con su `turno`** (12 Mañana y 12 Tarde) y su aula de referencia
(`CursoDTO.turno` y `CursoDTO.aula` no vacíos). El **plan** no se persiste: lo aporta el preset.

| Campo del curso | Origen | Obligatorio |
|---|---|---|
| `anio` / `seccion` / `aula` / `turno` | **Base de datos** (ya sembrados) | sí |
| `plan` | **Preset** | **sí** (sin él, V10/V11 rechaza) |
| `num_estudiantes` | **Preset** (sugerido por la matrícula del perfil) | no (opcional, V9) |
| modo de aula fija | **Preset** (global, por defecto **activo**) | no |

- La columna **Turno** debe mostrar el valor que **ya trae la base** (Mañana/Tarde). La opción
  «Sin turno» queda solo para cursos sin asignar; los del centro **no** están en ese caso.
- **La configuración no sobrescribe el turno del curso** (D-8): sobrescribirlo desde la
  configuración no existe hoy y es capacidad futura (spec 006). El preset replica el turno de la
  base.
- La columna **Plan** es un desplegable con los planes de la base: **31059** y **31060**.
- El **aula ya viene de la base** y el modo de aula fija es global (activo por defecto): con el modo
  activo el aula se toma de la base **sin que el preset la repita**. Doce aulas generales se
  **comparten entre turnos** (dos cursos de turnos distintos pueden tener la misma aula) y la cancha
  **no** es aula de referencia de ningún curso.
- **Matrícula:** 32–44 alumnos por curso (**889** en total). Si el preset no declara
  `num_estudiantes`, el curso sigue siendo válido (V9 es opcional); si lo declara, debe ser entero
  positivo.

**Reparto plan/turno del centro (ya declarado en la base y en el perfil):**

| Turno | Plan | Cursos |
|---|---|---|
| Mañana | `31059` | 1A, 1B, 3A, 3B, 3C, 4A, 4B, 4C, 5A, 5B, 5C, 5D |
| Tarde | `31059` | 2A, 2B, 2C |
| Tarde | `31060` | 1C, 1D, 1E, 2D, 2E, 3D, 3E, 4D, 4E |

> Las letras de 31060 son **libres** (1.º C–E; 2.º–4.º D–E) para que cada pareja año+sección sea
> única; no son las letras del documento del centro (la identidad con el plan es spec futura).

---

## 5. Un solo profesorado y un conjunto que genera

- **No hay profesorados alternativos ni conjuntos de ejemplo inválidos.** La plantilla es única:
  **33 docentes** (30 posiciones del centro + 2 añadidos + el apoyo de idiomas **P033**) con **272
  materias asignadas** y **165 disponibilidades**, tope de **36** horas académicas y disponibilidad
  de lunes a viernes en la ventana de su turno (16 en la mañana y 17 en la tarde). La clasificación
  de errores V13 por conjuntos inválidos ya no aplica.
- **El conjunto es generable (018).** La cancha tiene **aforo 2**: dos cursos con docentes distintos
  pueden compartirla si su bloque de dos horas ocupa **las mismas dos franjas** (nunca pares
  cruzados). La Educación Física de cada turno son **48 clases** frente a **80 plazas** de cancha
  (40 franjas × 2) y **96 ≤ 160** en la semana; en pares completos, **24 ≤ 40 plazas de par**. El
  informe del seed declara **«Generable: sí»** y la aceptación lenta
  (`test_solver_centro_real`) resuelve **876 clases** en menos de 300 s (medido: 232 s).
- **La vista de horario y su estimación de ocupación quedan fuera del alcance de la 018.** No es
  deuda de esta entrega: `src/frontend/src` no se toca y no se exige a la vista pintar dos cursos en
  la misma aula y franja ni informar del aforo 2 de la cancha. El aforo es **regla del motor** (no
  viaja en el DTO ni en el contrato) y la generación no cambia por ello.
- **Sin exceso de idiomas (018):** P028 (Inglés, **12 clases**) y P033 (Idiomas, **36 clases**)
  reparten las 48 clases que antes recaía en una sola docente; ambos quedan por debajo de las 40
  franjas del turno y de las 36 h de aula. Es información, no un error de la interfaz.
- **Lo que sí debe validarse:** la composición de la entrada con el contrato (V1–V19). El test
  `test_entrada_seed` compone los **24 cursos** con plan, turno y matrícula, valida y hace ida y
  vuelta del preset; la generación real la cubre `test_solver_centro_real` (018, etiqueta `slow`).

---

## 6. Checklist de integración

- [ ] **No reconvertir horas.** `PlanEstudio_Materia.horas` ya son bloques; no multiplicar por 60 ni
      dividir por `duracion_minutos`. Mostrar «34 h académicas/semana», «36» o «40» según el plan y
      el año (y el tiempo real solo como dato de presentación).
- [ ] **Mostrar el turno que ya trae la base** para los 24 cursos (12 Mañana / 12 Tarde); no asumir
      que están «sin turno».
- [ ] **No sobrescribir el turno del curso** desde la configuración (D-8; capacidad futura).
- [ ] **Desplegable de plan con 31059 y 31060**, leído de `PlanEstudio`.
- [ ] **Matrícula 32–44** (889 en total): si el preset declara estudiantes, entero positivo; la
      matrícula no está en el modelo.
- [ ] **Respetar `duracion_minutos` al pintar la rejilla** (**40 min** por bloque, anclada en el
      `inicio` del turno) y recordar que los **eventos no desplazan los bloques**.
- [ ] **Eventos como metadata:** los 3 eventos del turno (`despues_de_slot`, `orden`, `duracion`)
      son metadata; el solver los ignora, pero cuentan para el **encaje**
      (`nº franjas × D + Σ eventos ≤ ventana`).
- [ ] **Aula fija global:** el aula ya viene de la base y el modo es global (activo por defecto); no
      pedir aula por curso al usuario del preset. Las 12 aulas se comparten entre turnos.
- [ ] **No hay profesorados alternativos** ni opciones de recarga parcial de docentes.
- [ ] **Generación real (018):** el conjunto genera con el **aforo 2** de la cancha; la aceptación
      está en `test_solver_centro_real` (etiqueta `slow`). La vista de horario y su estimación de
      ocupación quedan **fuera del alcance de la 018**: no es deuda de esta entrega y
      `src/frontend/src` no se toca.
- [ ] **Usar los recuentos para las pruebas:** 2 planes (31059/31060), 85 materias, 95 cargas,
      **24 cursos**, **33 docentes**, 13 aulas, 2 turnos/3 eventos, 2 tipos de aula, 272
      asignaciones, 165 disponibilidades, 40 min y matrícula 889.
- [ ] **Enlazar la documentación:** remitir a `docs/guias/datos-prueba.md` (uso del seed) y a
      `docs/interfaz-frontend.md` (contrato del solver).

---

## 7. Resumen de cambios

| Cambio | Archivos/alcance |
|---|---|
| 017 — el centro real sustituye al conjunto sintético: 2 planes (31059/31060), 85 materias, 95 cargas, 24 cursos con turno, 13 aulas (12 + cancha), 33 docentes, 272 asignaciones y 165 disponibilidades | `src/seed/` (`perfil`, `docentes`, `sembrador`, `main`), CLI `gestor-horarios-seed` |
| Hora académica de **40 min** (configuración global) y **3 eventos** (receso ×2 + acto cívico) | `src/seed/src/perfil.cpp`; configuración `ConfiguracionHoraria` |
| Los cursos **ya traen turno**; el plan y la matrícula viven en el perfil/preset | `src/seed/src/perfil.cpp`; preset (spec 005) |
| Sin profesorados alternativos ni conjuntos inválidos de ejemplo | `src/seed/` (se retiraron) |
| `test_generacion_seed` se retira; `test_entrada_seed` compone y valida **sin resolver** | `test/frontend/test_entrada_seed.cpp`, `test/CMakeLists.txt` |
| **018** — aforo 2 de la cancha (**1** en el resto de tipos): capacidad `aulas × franjas × aforo`, sin campo en el contrato; el conjunto pasa a **«Generable: sí»** (48 ≤ 80, 96 ≤ 160, 24 ≤ 40 pares) | `src/common/include/common/aforo_aula.hpp`, `backend_solver`, `ValidadorSalidaSolver`, `src/seed/` |
| **018** — docente de apoyo **P033** (Idiomas) y P028 con Inglés: **33 docentes / 165 disponibilidades**, 16 mañana y 17 tarde | `src/seed/src/docentes.cpp`; guías de datos |
| **018** — aceptación lenta `test_solver_centro_real` (876 clases, EF en cancha, 0 pares cruzados, desvío 0, < 300 s) | `test/frontend/test_solver_centro_real.cpp`, `test/CMakeLists.txt` |
