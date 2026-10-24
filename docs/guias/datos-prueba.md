# Datos de prueba del centro — `gestor-horarios-seed`

> Herramienta de **desarrollo** que puebla una base de datos con un **centro de Educación Media
> General completo, ficticio, determinista y reutilizable**: el **Plan de Estudio 31059** (Resolución
> DM/0033, Gaceta Oficial 41.221 del 24-08-2017) con **cuatro planes** y **cuatro variantes de
> profesorado** intercambiables. El módulo vive en `src/seed/`.
>
> Solo puebla la **base de datos**. **No genera horarios ni presets del solver** (ver §6).

---

## 1. Qué contiene el conjunto

| Entidad | Cantidad |
|---|---|
| Turnos (Mañana 07:00–12:30 · Tarde 12:40–18:10, 8 franjas de 40 min) | 2 |
| Eventos (un «Receso» de 10 min tras la 2.ª franja: 08:20–08:30 y 14:00–14:10) | 2 |
| Tipos de aula (`Aula de estudio`, `Laboratorio`, `Taller`) | 3 |
| Aulas (13 para cursos + reserva + laboratorio + taller) | 16 |
| Materias-año del catálogo (51 del 31059 + 6 nuevas de los derivados) | 57 |
| Planes de estudio (General + 3 áreas de desarrollo) | 4 |
| Cargas plan-materia | 210 |
| Cursos (3 secciones en 1.º–3.º, 2 en 4.º–5.º; **sin turno**) | 13 |

- **Escala de carga:** cada año de cada plan suma **40 horas académicas** (la hora del 31059 es una
  **hora académica = un bloque**; la escala 44/44/46/46/46 del plan oficial se reduce al techo semanal
  de 40 bloques = 8 franjas × 5 días). **1 hora académica = 1 bloque**, sin conversión ni redondeo.
- **Planes:** `GEN` (General), `EXA` (Ciencias Exactas), `NAT` (Ciencias Naturales) y `SOC` (Ciencias
  Sociales). Los tres derivados añaden **2 materias nuevas de su eje** en 4.º/5.º y recortan 1 h en
  tres áreas base del mismo año, de modo que **todos los años suman 40/40/40/40/40 h académicas**.

  Materias por año: `GEN 9/9/10/11/12 · EXA 9/9/10/12/13 · NAT 9/9/10/12/13 · SOC 9/9/10/12/13`.

- **Docentes ficticios** (nombres, correos y teléfonos inventados, con acentos y eñes), con sus
  materias asignadas y su disponibilidad.

## 2. Requisitos y compilación

El ejecutable depende de `seed` → `datos` → `backend` (Qt6 Core/Sql), sin OR-Tools. Se compila con el
preset **`full`** (`BUILD_BACKEND=ON`, que exige OR-Tools presente en el entorno de configuración; por
eso se compila dentro del contenedor `gestor-dev`).

```bash
cmake --preset full
cmake --build build
# ejecutable: build/src/seed/gestor-horarios-seed
```

Opciones de CMake: `BUILD_SEED` (por defecto `ON`). Con `BUILD_BACKEND=OFF` (preset `dev-frontend`) el
seed **no** se compila.

Dentro del contenedor:

```bash
docker exec -it gestor-dev bash -lc "cd /workspace && cmake --preset full && cmake --build build"
docker exec -it gestor-dev bash -lc "cd /workspace && ./build/src/seed/gestor-horarios-seed --db /tmp/centro.db"
```

## 3. Uso

```bash
gestor-horarios-seed --db <ruta> [--variante <v>] [--reemplazar | --solo-docentes] [--si] [--ayuda] [--version]
```

| Opción | Efecto |
|---|---|
| `--db <ruta>` | **Obligatoria.** Base destino (se crea con el esquema si no existe). |
| `--variante <v>` | Variante de docentes: `holgada` (por defecto), `suplentes`, `sin-profesorado` o `sin-disponibilidad`. |
| `--reemplazar` | Reemplazo total (borra y repuebla; exige `--si` y deja respaldo). |
| `--solo-docentes` | Recarga solo docentes, sus materias y su disponibilidad (exige `--si` y respaldo); el resto de los datos no se toca. |
| `--si` | Confirma la operación destructiva (sin terminal interactiva). |
| `--ayuda`, `--help`, `-h` | Muestra la ayuda. |
| `--version` | Muestra la versión. |

**Códigos de salida:** `0` ok · `1` uso/argumentos · `2` base con datos · `3` confirmación requerida ·
`4` respaldo fallido · `5` apertura/esquema · `6` variante desconocida · `7` escritura/atomicidad ·
`8` perfil incoherente.

### Ejemplo de salida en éxito

```
Base: /tmp/centro.db
Variante: holgada
Turnos: 2   Eventos: 2   Aulas: 16   Materias: 57   Planes: 4   Materias de plan: 210
Cursos: 13 (sin turno)   Docentes: 65   Materias asignadas: 65   Disponibilidades: 325   Tipos de aula: 3
Materias por año: GEN 9/9/10/11/12 · EXA 9/9/10/12/13 · NAT 9/9/10/12/13 · SOC 9/9/10/12/13
GEN  General            40/40/40/40/40 h académicas
EXA  Ciencias Exactas   40/40/40/40/40 h académicas
NAT  Ciencias Naturales 40/40/40/40/40 h académicas
SOC  Ciencias Sociales  40/40/40/40/40 h académicas
Respaldo: no aplica
ok
```

## 4. Las cuatro variantes de docentes

Cada variante es un conjunto **completo e intercambiable** de docentes, con sus materias asignadas y su
disponibilidad. **El resto de los datos (planes, cursos, aulas, turnos) es idéntico** entre variantes.

| Variante | Docentes | Materias asignadas | Disponibilidades | Propósito |
|---|---|---|---|---|
| `holgada` | 65 | 65 | 325 | Titulares con carga **≤ 60 % de su tope** (holgada); cubre **todas** las materias-año en **ambos turnos**. |
| `suplentes` | 43 | 57 | 215 | Titulares **cerca de su tope** (≥ 80 %, ratio hasta ~1,15); Σ topes al **88,65 % de la demanda** y **24 suplentes sin materias** para que el preset declare suplencias. |
| `sin-profesorado` | 64 | 64 | 320 | **Inválida a propósito:** `Física 3º` queda sin ningún docente (cobertura falla en ambos turnos). |
| `sin-disponibilidad` | 65 | 65 | 325 | **Inválida a propósito:** los docentes de `Física 3º` tienen disponibilidad `19:00–21:00` (solape nulo con ambos turnos). |

Las dos últimas son **deliberadamente inválidas** para ejercitar los dos modos de rechazo de la
validación previa del solver (sin docente elegible / sin solape de disponibilidad).

### Alternar la variante sin tocar el resto de los datos

```bash
# De holgada a suplentes: solo se borran y recargan docentes, materias y disponibilidad.
gestor-horarios-seed --db /tmp/centro.db --variante suplentes --solo-docentes --si
```

Al aplicar dos veces la misma variante, el estado final es el mismo (sin duplicados). Pedir una
variante inexistente sale con código **6** y **no toca ningún dato**.

## 5. Seguridad (protección de la base)

- **Falla con datos:** si la base ya tiene datos y no se pide una operación destructiva, la aplicación
  falla **sin escribir** (`La base ya tiene datos; use «--reemplazar» para reemplazarlos.`, código **2**).
- **Operaciones destructivas** (`--reemplazar` o `--solo-docentes`) exigen **`--si`**; sin él, salen
  con código **3** (`Confirmación requerida`) **antes** de respaldar o borrar.
- **Respaldo obligatorio:** antes de borrar, deja un respaldo (`VACUUM INTO`) con ruta única
  (`<base>.respaldo-<fecha-hora>`; si existe, prueba `-2`, `-3`…). Si el respaldo falla, **aborta sin
  borrar nada** (código **4**).
- **Atomicidad:** toda aplicación o reemplazo es una única transacción: o quedan todos los datos del
  perfil, o no queda ninguno (una interrupción a mitad deja la base como estaba).

## 6. Límites y fuera de alcance

- **Datos ficticios:** nombres, correos y teléfonos inventados; **ninguna persona real** (constitución
  §5). No es un importador de datos reales ni una carga de producción.
- **No genera presets del solver:** el conjunto solo describe el centro persistido. El **turno de cada
  curso** (los cursos se siembran **sin turno**), el **número de estudiantes**, los **filtros del
  docente** (turno, plan, años, horas de aula) y las **suplencias con su `peso`** viven en el preset
  (spec 005), que este conjunto **no** genera. Solo garantiza que los datos persistidos permiten
  construir presets coherentes y ejercitar las cuatro variantes.
- **La duración de la franja no cambia la carga:** 1 hora académica = 1 bloque, y cada año suma 40
  horas académicas (= 40 bloques). Los **40 minutos** son la duración **real** de la franja (rejilla
  horaria, eventos y disponibilidad), no un factor de conversión. El evento «Receso» de 10 minutos
  tras la 2.ª franja se descuenta en la fórmula `(ventana − Σ eventos) / franjas`.
- **Semana de referencia:** 5 días (lunes a viernes); es una propiedad de la configuración que acompaña
  al conjunto y **no se persiste**.
- **Determinismo:** aplicar el mismo perfil dos veces sobre bases vacías deja los mismos datos
  (nombres, códigos, recuentos y relaciones), salvo las marcas de auditoría (`fecha_creacion` y
  `fecha_modificacion`).
- **Fuera de alcance:** no genera horarios, no añade pantallas, no importa datos reales y no cambia el
  esquema ni el modelo.

## 7. Verificación

Suite completa:

```bash
docker exec -it gestor-dev bash -lc "cd /workspace && cmake --preset full && cmake --build build && ctest --preset full --output-on-failure"
```

Solo los tests del seed (más rápido):

```bash
docker exec -it gestor-dev bash -lc "cd /workspace && cmake --build build && ctest --test-dir build -R 'test_seed' --output-on-failure"
```
