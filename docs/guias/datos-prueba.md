# Datos de prueba del centro — `gestor-horarios-seed`

> Herramienta de **desarrollo** que puebla una base de datos con el **centro real del Liceo
> Nacional Robert Serra** (periodo **2026/2027**): **2 planes** de estudio, **24 cursos** en dos
> turnos, **12 aulas** de uso general y **una cancha**, **33 docentes** y la **matrícula del centro**.
> Es un **conjunto único**: no hay profesorados alternativos, planes derivados ni selección de perfil.
> El módulo vive en `src/seed/`.
>
> Solo puebla la **base de datos**. **No genera horarios ni presets del solver** (ver §6); el
> conjunto **sí se declara generable** con el **aforo 2 de la cancha** (spec 018): 48 clases de
> Educación Física ≤ 80 plazas por turno y 96 ≤ 160 en la semana.

---

## 1. Qué contiene el conjunto

| Entidad | Cantidad |
|---|---|
| Duración de la hora académica (configuración, una fila; no es dato de dominio) | **40 min** |
| Turnos (Mañana 07:00–12:40 · Tarde 12:40–18:10, 8 franjas de 40 min) | 2 |
| Eventos (receso y acto cívico de la mañana; receso de la tarde) | 3 |
| Tipos de aula leídos de la base (`Aula de estudio` + `Cancha`; declarado por el conjunto: 1) | 2 |
| Aulas (12 generales de capacidad 44, compartidas entre turnos, + cancha de 88) | 13 |
| Materias-año del catálogo (**79 con carga + 6 sin carga**) | **85 materias** |
| Planes de estudio (31059 y 31060) | 2 |
| Cargas plan-materia | **95 cargas** |
| Cursos (12 de mañana + 12 de tarde, **con turno**) | **24 cursos** |
| Docentes (30 posiciones del centro + 2 añadidos + 1 apoyo de idiomas) | **33 docentes** |
| Materias asignadas | **272** |
| Disponibilidades (33 docentes × 5 días) | **165** |
| Preferencias (no se siembran) | 0 |
| Matrícula total declarada en el perfil (579 de 31059 + 310 de 31060) | **889** |
| Clases del conjunto (96 de Educación Física; 48 por turno) | **876** |
| Aforo de la cancha (cursos a la vez) | **2** |

### Planes

| Plan | Nombre | Áreas | Horas por año (1.º–5.º) | Materias por año (1.º–5.º) | Cargas |
|---|---|---|---|---|---|
| **31059** | Media General | 12 | **34/34/34/36/34** | 8/8/9/10/10 | 45 |
| **31060** | Ciencias y Tecnología | 10 | **40/40/40/40/40** | 10/10/10/10/10 | 50 |

- **31059** (tras excluir las dos áreas sin titular, ver abajo): Castellano 4/4/4/4/3 · Inglés y
  otras lenguas extranjeras 4/4/4/4/3 · Matemáticas 4×5 · Educación Física 4×5 · Arte y Patrimonio
  4/4 · Ciencias Naturales 6/6 · Física 3.º–5.º 4/4/4 · Química 3.º–5.º 4/4/4 · Biología 3.º–5.º
  4/4/4 · Geografía, Historia y Ciudadanía 6/6/4/4/4 · Formación para la Soberanía Nacional 4.º–5.º
  2/2 · Orientación y Convivencia 2×5. **Todas las sumas son pares.**
- **31060**: Lengua y Literatura 4×5 · Idiomas 4×5 · Matemáticas 4×5 · Educación Física 4×5 ·
  Biología, Ambiente y Tecnología 4×5 · Física 2/2/4/4/4 · Química 2/2/4/4/4 · Geografía, Historia y
  Soberanía Nacional 6/6/4/4/4 · Orientación Vocacional 4/4/2/2/2 · Innovación Tecnológica y
  Productividad 6×5.

### Catálogo de materias

- **Una materia por área y año** (`«<Área> <año>º»`). **16 compartidas** entre los dos planes:
  Matemáticas 1.º–5.º, Educación Física 1.º–5.º, Física 3.º–5.º y Química 3.º–5.º (4 en 5.º). Se
  siembran **una sola vez** con la carga de cada plan.
- **6 materias sin carga** (excepción de prueba): «Participación en los Grupos de Creación,
  Recreación y Producción» de 1.º a 5.º y «Ciencias de la Tierra» de 5.º. Sus áreas **no están en
  ningún plan**; existen para probar docentes habilitados sin materia que impartir.
- **Tipos de aula:** todas las materias usan el tipo general salvo **Educación Física**, que exige
  **`Cancha`**. El centro **no tiene laboratorios ni talleres**.

### Cursos, turnos y aulas

| Turno | Plan | Cursos | Aulas de referencia |
|---|---|---|---|
| **Mañana** | 31059 | 1A, 1B, 3A, 3B, 3C, 4A, 4B, 4C, 5A, 5B, 5C, 5D | 01–12 |
| **Tarde** | 31059 | 2A, 2B, 2C | 01–03 |
| **Tarde** | 31060 | 1C, 1D, 1E, 2D, 2E, 3D, 3E, 4D, 4E | 10–12, 04–05, 07–06, 09–08 |

- **Turno en el curso:** se siembra en `Cursos.turno` (dato del centro). **12 y 12** cursos; ninguna
  aula de referencia se repite **dentro** del mismo turno y las 12 aulas se **comparten** entre
  turnos.
- **Letras libres (D-12):** 31059 usa A, B (1.º), A–C (2.º–4.º) y A–D (5.º); 31060 usa C, D, E (1.º)
  y D, E (2.º–4.º). Las 24 parejas año+sección son únicas y de un carácter. **No son las letras del
  documento**: la identidad con el plan es una spec futura.
- **Matrícula:** 32–44 alumnos por curso (44 en 4.º A y 4.º B). Vive en el **perfil**, no en el
  modelo; total **889** (579 + 310). La consumirá el preset como número de estudiantes.
- **El plan de cada curso no se persiste** (el modelo no tiene plan): lo declara el perfil y lo
  aporta la configuración al generar.

### Horas, eventos y duración

| Turno | Ventana | Rejilla | Eventos |
|---|---|---|---|
| Mañana | **07:00–12:40** | 8 × 40, termina a las 12:20 | Receso 08:20–08:30 · Acto cívico 08:30–08:40 |
| Tarde | **12:40–18:10** | 8 × 40, termina a las 18:00 | Receso 14:00–14:10 |

- Encaje: mañana `8 × 40 + 20 = 340` y tarde `8 × 40 + 10 = 330`. El desfase entre la rejilla
  aritmética y el fin de la ventana (20/10 min) es el tiempo de los eventos; **los eventos no crean
  ni quitan franjas**.
- **Duración 40 min:** es configuración global (una sola fila). Aplicar el conjunto la deja en **40**
  (el valor de fábrica es 35) y es idempotente. **1 hora académica = 1 bloque**: 34/36/40 horas son
  34/36/40 bloques y la duración no es factor de conversión.
- **Clases:** **876** (414 de mañana + 462 de tarde). **96 son de Educación Física** (4 h × 24
  cursos; **48 por turno**) y van a la cancha.

## 2. Requisitos y compilación

El ejecutable depende de `seed` → `datos` → `backend` (Qt6 Core/Sql). Se compila con
`BUILD_BACKEND=ON` y `BUILD_SEED=ON` (por defecto); con `BUILD_BACKEND=OFF` (preset `dev-frontend`)
el seed **no** se compila.

```bash
cmake --preset backend
cmake --build --preset backend --parallel
# ejecutable: build-backend/src/seed/gestor-horarios-seed
```

Con los presets `full` (Docker) y `full-release` el binario queda en
`build/src/seed/gestor-horarios-seed` y `build-rel/src/seed/gestor-horarios-seed`,
respectivamente. En el contenedor del proyecto:

```bash
docker exec -it gestor-dev bash -lc "cd /workspace && cmake --preset full && cmake --build build"
docker exec -it gestor-dev bash -lc "cd /workspace && ./build/src/seed/gestor-horarios-seed --db /tmp/centro.db"
```

## 3. Uso

```bash
gestor-horarios-seed --db <ruta> [--reemplazar] [--si] [--ayuda] [--version]
```

| Opción | Efecto |
|---|---|
| `--db <ruta>` | **Obligatoria.** Base destino (se crea con el esquema si no existe). |
| `--reemplazar` | Reemplazo total (borra y repuebla; exige `--si` y deja respaldo). |
| `--si` | Confirma la operación destructiva (sin terminal interactiva). |
| `--ayuda`, `--help`, `-h` | Muestra la ayuda. |
| `--version` | Muestra la versión. |

Las antiguas opciones de **selección de profesorado** y de **recarga parcial de docentes** ya no
existen: el conjunto es uno solo y cualquier argumento desconocido termina con **código 1**.

**Códigos de salida:** `0` ok · `1` uso/argumentos · `2` base con datos · `3` confirmación requerida ·
`4` respaldo fallido · `5` apertura/esquema · `7` escritura/atomicidad · `8` perfil o docentes
incoherentes.

### Ejemplo de salida en éxito

```text
Base: /tmp/centro.db
Centro: Liceo Nacional Robert Serra (2026/2027)
Hora académica: 40 min (configuraciones: 1)
Turnos: 2   Eventos: 3   Aulas: 13   Materias: 85   Planes: 2   Materias de plan: 95
Cursos: 24 (12 Mañana · 12 Tarde)   Docentes: 33   Materias asignadas: 272   Disponibilidades: 165   Tipos de aula: 2
Preferencias: 0
Clases: 876 (EF 96; 48 por turno)
Clases por plan: 31059 516 · 31060 360
Clases por año: 31059 68/102/102/108/136 · 31060 120/80/80/80
Materias por año: 31059 8/8/9/10/10 · 31060 10/10/10/10/10
31059  Media General         34/34/34/36/34 h académicas
31060  Ciencias y Tecnología 40/40/40/40/40 h académicas
Matrícula: 889 (579 + 310)
Capacidad docente: 1188 (576 + 612)   Demanda: 414 Mañana · 462 Tarde
Cancha: 1 aula · 40 franjas por turno · aforo 2 → 80 plazas por turno · 160 en la semana
Educación Física: 48 clases por turno · 96 en la semana → 24 pares por turno · 48 en la semana
Pares de cancha: 20 posiciones por turno · 40 plazas de par → 24 ≤ 40 por turno
Generable: sí
Respaldo: no aplica
ok
```

La aplicación sobre una base nueva tarda menos de **1 s** en el entorno de referencia (límite de
RNF-1: 15 s).

## 4. Profesorado del centro y tope de horas

- **33 docentes** con identidad ficticia `P001`…`P033` (nombres, correos y teléfonos inventados,
  RNF-6): las **30 posiciones** del centro con sus mismas áreas y turnos, más **2 añadidos** para las
  materias sin carga (uno por turno) y el **apoyo de idiomas de la tarde (P033)** de la 018.
- **Tope de 36 horas académicas de aula** para todos (D-3): las 40 h del contrato del centro
  incluyen actividades «fuera de aula» (planificación, CRP, atención familiar, coordinación…) que se
  descuentan y **no se programan**.
- **Turnos:** **16 docentes en la mañana** (15 del centro + 1 añadido) y **17 en la tarde** (15 + 1
  añadido + P033). El turno vive en el perfil y **no se persiste**: se materializa en la
  disponibilidad, con **un rango por día de lunes a viernes** y la ventana completa del turno en reloj
  (07:00–12:40 / 12:40–18:10).
- **Regla de asignación:** cada docente recibe **todas las materias-año del catálogo de sus áreas**,
  contando **una sola vez** las compartidas, **sin filtrar por año** y **solo si el área tiene cursos
  en su turno**. «Biología» y «Biología, Ambiente y Tecnología» son áreas distintas: la primera queda
  en la mañana y la segunda **solo** en la tarde. Las materias de **5.º de 31060** (año sin cursos)
  cuentan porque su área tiene cursos en la tarde.
- **272 materias asignadas** y **165 disponibilidades**. Los dos añadidos suman **6** materias sin
  carga (5 de Participación y 1 de Ciencias de la Tierra); **P033** aporta **5 de Idiomas** y **P028**
  se queda con **5 de Inglés** (spec 018). La suma de asignaciones no cambia: se mueven, no se crean.
- **Cobertura por turno:** cada materia de cada curso tiene al menos un docente habilitado en el
  turno de ese curso; si faltara, la aplicación se rechazaría identificando área, año y turno.
- **Sin exceso de idiomas (018):** P028 (Inglés, **12 clases** de 31059 2.º) y P033 (Idiomas, **36
  clases** de 31060) reparten las 48 clases que antes recaían en una sola docente. Ambos quedan por
  debajo de las 40 franjas del turno y de las 36 h de aula, y el conjunto **genera**.

## 5. Seguridad (protección de la base)

- **Falla con datos:** si la base ya tiene datos y no se pide reemplazo, la aplicación falla **sin
  escribir** (`La base ya tiene datos; use «--reemplazar» para reemplazarlos.`, código **2**).
- **Reemplazo total:** exige **`--si`**; sin él sale con código **3** (`Confirmación requerida`)
  **antes** de respaldar o borrar.
- **Respaldo obligatorio:** antes de borrar deja un respaldo (`VACUUM INTO`) con ruta única
  (`<base>.respaldo-<fecha-hora>`; si existe, prueba `-2`, `-3`…). Si el respaldo falla, **aborta sin
  borrar nada** (código **4**).
- **Atomicidad:** toda aplicación o reemplazo es una única transacción: o quedan todos los datos del
  perfil, o no queda ninguno (una interrupción a mitad deja la base como estaba).

## 6. Límites y fuera de alcance

- **El conjunto es generable (018, RF-8) con el aforo 2 de la cancha.** La Educación Física de cada
  turno son **48 clases** y la cancha, con **aforo 2** (dos cursos con docentes distintos en la
  **misma posición de par**), ofrece **80 plazas por turno** (40 franjas × 2) y **160 en la semana**:
  **48 ≤ 80** y **96 ≤ 160**. En pares completos hay **20 posiciones de par por turno** y **40 plazas
  de par** frente a **24 pares** de Educación Física: **24 ≤ 40**.
  - El **informe** de la aplicación y esta guía lo declaran **«Generable: sí»** con esas cifras.
  - La aceptación de generación es el test lento **`test_solver_centro_real`** (etiqueta `slow`):
    siembra, compone la entrada desde la base y resuelve **876 clases** con la EF en la cancha,
    desvío 0, huecos al final y **0 pares cruzados** en menos de **300 s** (medido: **232 s**).
  - La cancha no admite **más de dos cursos** a la vez ni **pares cruzados**; el resto de tipos
    conserva **una clase por franja**.
- **Educación Física en la cancha:** 96 clases en la semana y 48 por turno, todas en el aula de clave
  `cancha`. La **capacidad física** del aula (88 alumnos) es un dato distinto del **aforo 2** (cursos
  simultáneos) y no prueba por sí sola que exista horario: eso lo verifica el test lento de aceptación.
- **La duración no cambia la carga:** 1 hora académica = 1 bloque; cada año suma
  34/34/34/36/34 (31059) o 40×5 (31060) horas académicas. Los **40 minutos** son la duración real de
  la franja (rejilla, eventos y disponibilidad): 34 bloques × 40 min = 1360 min (22 h 40) en 1.º–3.º
  y 5.º de 31059, 36 × 40 = 1440 min (24 h) en 4.º y 40 × 40 = 1600 min (26 h 40) en 31060.
- **Nombres ficticios:** el conjunto derivado no contiene nombres reales; el documento del centro
  (`seed.md`) sigue siendo la fuente versionada y **no se amplía**.
- **No genera presets:** el seed solo describe el centro persistido. El **plan de cada curso**, el
  **número de estudiantes** (la matrícula del perfil), los filtros del docente y las suplencias
  viven en la configuración del solver, que este conjunto **no** genera. Los cursos **ya traen su
  turno** en la base.
- **Paquete de demostración fuera de alcance:** `GESTOR_PACK_DEMO` y su carpeta `datos-demo/` **no se
  regeneran** con el centro real: siguen con el conjunto sintético o se retiran en otra spec.
- **Semana de referencia:** 5 días (lunes a viernes); es propiedad de la configuración y **no se
  persiste**.
- **Determinismo:** aplicar el mismo perfil dos veces sobre bases vacías deja los mismos datos
  (nombres, códigos, recuentos y relaciones), salvo las marcas de auditoría.
- **Fuera de alcance:** no genera horarios, no añade pantallas, no importa datos reales y no cambia
  el esquema ni el modelo (el turno del curso y la sección de un carácter ya existían).

## 7. Verificación

Suites del proyecto (desde la raíz):

```bash
cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend
cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release
```

Solo los tests del seed (más rápido):

```bash
ctest --preset backend -R "test_seed_perfil|test_seed_aplicacion" --output-on-failure
ctest --preset full-release -R "test_entrada_seed|test_selector_generacion" --output-on-failure
```

Aplicación real y comprobación del informe:

```bash
build-backend/src/seed/gestor-horarios-seed --db /tmp/centro.db
# esperado: 85 materias, 24 cursos (12/12), 33 docentes, 272 asignaciones,
#           165 disponibilidades, 13 aulas, 3 eventos, 40 min y «Generable: sí»
```
