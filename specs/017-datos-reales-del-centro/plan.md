# Plan 017 - Datos reales del centro

Base: `b5844b580d495397903e5d3bfd8756aa41d98f05` (`git rev-parse HEAD` el 2026-11-08; `HEAD` de
`feat/Motor`, «docs(memory): cierre de la 014»). **La `Base` propuesta en el encargo
(`b5844b515a0c…`) no existe**: solo coincidía el prefijo corto `b5844b5`; se usa el sha real.

Línea base de la suite: **62** tests en `backend` y **67** en `full-release` (`ctest -N`, QA del
2026-11-08; el conteo local del planner dio 61/66 — **al abrir T1 se anota el medido** en `journal.md`).

---

## 1. Alcance y RF cubiertos

Sustituir el conjunto sintético de la 008 por el **centro real 2026/2027**, en **una sola forma de
aplicación**. **No se toca** el motor, el esquema, el contrato `2.0` ni `src/frontend` (solo un test).

| RF | Qué se implementa | Tarea |
|---|---|---|
| RF-1 | Conjunto único: fuera variantes, conmutación y planes derivados | T1, T3 |
| RF-2 | Planes 31059 y 31060 con horas reales, paridad y tablas canónicas | T1 |
| RF-3 | Catálogo de 85 materias (79 con carga, 6 sin carga), compartidas y tipos | T1 |
| RF-4 | 24 cursos con año, sección, **turno**, aula y **matrícula**; plan solo en el perfil | T1 |
| RF-5 | 12 aulas generales compartidas + cancha; capacidades; cancha no es referencia | T1 |
| RF-6 | 32 docentes, regla de asignación, tope 36 y cobertura por turno | T1 |
| RF-7 | Turnos de 40 min, ventanas 07:00–12:40 / 12:40–18:10 y 3 eventos | T1 |
| RF-8 | Determinismo, recuentos e informe (duración incluida) | T1, T2, T5 |
| RF-9 | Verificación por datos y «no generable» con cifras; nada genera | T2, T4 |
| RF-10 | Guías, supersesión de la 008 y verificación en la interfaz | T6, T7, T8, T9 |
| RNF-1…8 | 15 s, sin generación, sin esquema, determinismo, mensajes, ficticios, aritmética, traza | T1–T10 |

**Fuera de alcance (spec):** aforo simultáneo, identidad año+sección+plan, plan persistido, turno
sobrescribible, paquete del liceo y `GESTOR_PACK_DEMO`.

## 2. Hallazgos del QA del plan (B1–B4 + menores) y dónde se resuelven

| # | Hallazgo | Resolución |
|---|---|---|
| **B1** | Alguna tarea cerraba con build/suite roja | **T1 es la unidad atómica** (perfil + docentes + sembrador + sus tres tests + baja de `test_generacion_seed`): cierra con **ambas suites verdes**. El resto de tareas solo añaden o ajustan, y **ninguna deja un consumidor roto**. Se retira del plan la nota de «estado intermedio» y las frases «el resto se cierra en T…». |
| **B2** | `test_selector_generacion` usa `preferencia.plan = "NAT"` | **T1** cambia `"NAT"` → **`"31059"`** (el plan del centro) en la preferencia y en su verificación; **T9** revisa la interfaz con 24 cursos y 32 docentes. |
| **B3** | Las 272 no se alcanzan con la tabla de posiciones | **§4.6**: las posiciones 12, 13, 15 y 29 declaran **las dos áreas biológicas con su nombre exacto** («Biología» **y** «Biología, Ambiente y Tecnología»); `materiasDeDocente` compara **nombres exactos** (sin expandir). Resultado **272** y **18** de 5.º de 31060, verificados en T1. |
| **B4** | Falta la matrícula (889; 579/310; 32–44) | **`CursoDef.matricula`** en T1, validada (32–44) y probada (Σ = 889; 579 de 31059 y 310 de 31060); **T4** la usa como `numEstudiantes`; **T6/T7** la documentan; **RF-4** y §10 la recogen. |
| Menor | El `grep` de T1 prohibía `kAreas31059`, que T1 crea | T1 grepea solo lo viejo: `'kRecortes\|kMateriasNuevas\|"EXA"\|"SOC"\|"GEN"\|esMateriaNueva'`. |
| Menor | `grep --variante` en T3 | Se mueve a **T3 (CLI)**, que es cuando desaparece el texto de ayuda. |
| Menor | Ruta del binario | **`build-backend/src/seed/gestor-horarios-seed`** (preset `backend`). |
| Menor | `grep` de T9 (docs) demasiado amplio | Grepea **recuentos obsoletos** (`57 materias`, `210 cargas`, `65 docentes`, `16 aulas`, `13 cursos`, `holgada`) y comprueba los nuevos. |
| Menor | Riesgo 7 desalineado con D-8/H10 | §12: el turno **se escribe en el curso**; la configuración **no** lo sobrescribe. |
| Menor | `grep resolver` en T10 | Grepea **`resolverEntradaSolver`**. |
| Menor | Línea base 61/66 | §0: **62/67** del QA, con el medido anotado en T1. |
| Menor | La excepción D-4 estaba citada como §5.3 | Es **§5.4**. |

## 3. Archivos y responsabilidades

| Archivo | Cambio |
|---|---|
| `src/seed/include/seed/perfil.hpp` | `MateriaAnio` gana `area` y `sinCarga`; `CursoDef` gana `plan`, `turno` y **`matricula`**; `Perfil` gana `duracionMinutos`; `Recuentos` gana `duracionMinutos` y `configuraciones`; salen `seccionesDeAnio`, `esMateriaNueva`, `cargaMateria`, `cargaAsignada`, `horasMaximas`; nuevas `horasDeCurso`, `demandaTotal`, `demandaPorTurno`, `areaConCursosEnTurno`, `motivoNoGenerable`. |
| `src/seed/src/perfil.cpp` | Tablas reales (áreas, aulas, cursos con matrícula), catálogo con deduplicación, 6 materias sin carga y `validarPerfil` nueva. |
| `src/seed/include/seed/docentes.hpp` | Sin `VarianteDocentes`, `nombreVariante`, `varianteDesdeTexto`, `coberturaDeVariante`, `docentesCumplenCarga`; `DocenteDef` gana `turno`; nuevas `docentesDelCentro`, `validarDocentes`, `coberturaDelCentro`; `recuentosEsperados(perfil)`. |
| `src/seed/src/docentes.cpp` | 30 posiciones (turno + áreas exactas), regla de asignación, 2 añadidos, disponibilidad por turno, tope 36. |
| `src/seed/include/seed/sembrador.hpp` | `OpcionesAplicacion` sin `variante`/`soloDocentes`; `EstadoAplicacion` sin `VarianteDesconocida`; `ResultadoAplicacion` con `recuentos`; `resumenTexto(perfil, recuentos)`. |
| `src/seed/src/sembrador.cpp` | Configuración con `perfil.duracionMinutos`, turno en el curso, sin conmutación, recuentos con duración, informe. |
| `src/seed/src/main.cpp` | CLI sin `--variante`/`--solo-docentes`; ayuda y códigos. |
| `test/backend/test_seed_perfil.cpp` | Reescritura completa (T1). |
| `test/backend/test_seed_aplicacion.cpp` | Recuentos, variantes fuera, configuración y turnos (T1); seguridad y 15 s (T5). |
| `test/frontend/test_selector_generacion.cpp` | `"NAT"` → `"31059"` (T1); revisión con 24 cursos (T9). |
| `test/frontend/test_generacion_seed.cpp` | **Baja de `test/CMakeLists.txt`** en T1 (hueco declarado) y **borrado del archivo** en el cierre (T10); su sustituto entra en T4. |
| `test/frontend/test_entrada_seed.cpp` | **Nuevo** (T4). |
| `test/CMakeLists.txt` | Baja `test_generacion_seed` (T1) y alta `test_entrada_seed` (T4). |
| `src/seed/CMakeLists.txt` | Comentario del target `seed` sin mención a las variantes (T3). |
| `docs/guias/datos-prueba.md` | Reescritura para el centro real (T6). |
| `docs/guias/datos-prueba-frontend.md` | Reescritura; **sin** prometer comportamiento de interfaz (T7). |
| `docs/interfaz-frontend.md` | Turno de la base, 40 min, `test_entrada_seed` y actualización 2026-11-08 (T9). |
| `docs/guias/migracion-frontend.md` | Datos del seed vigentes y deuda de `test_generacion_seed` resuelta (T9). |
| `docs/guias/desarrollo/CMAKE_GUIDE.md` | Nota de la 017 en el paquete de demostración (T9). **Queda excluido del grep de recuentos obsoletos**: su apartado demo describe el conjunto sintético (H7). |
| `specs/008-datos-de-prueba/spec.md` | Nota de supersesión al principio (T8). |
| `specs/008-datos-de-prueba/estado.md` | `Fase`, «Superación» e historial (T8). |
| `MEMORY.md` | Parche entregado al coordinator (el planner no puede editarlo). |

## 4. Modelo del perfil (datos exactos)

### 4.0 Definiciones

- **Conjunto:** datos persistidos del centro (2 planes, 85 materias, 95 cargas, 24 cursos con turno y
  aula, 13 aulas, 2 turnos con 3 eventos, 32 docentes, 272 asignaciones, 160 disponibilidades) más el
  **perfil**, que declara y aporta lo que no se persiste (**plan de cada curso**, **turno de cada
  docente**, **matrícula**) y la **duración** de la hora académica.
- **Duración:** configuración global (una fila). El perfil la declara (**40**), el sembrador la escribe y
  los recuentos la verifican.
- **Semana de referencia:** **5 días** (lunes–viernes) para disponibilidad, huecos y capacidad. `num_dias`
  es configuración y el conjunto no lo persiste.

### 4.1 Planes

**31059 — Media General** (12 áreas sembradas):
Castellano 4/4/4/4/3 · Inglés y otras lenguas extranjeras 4/4/4/4/3 · Matemáticas 4/4/4/4/4 · Educación
Física 4/4/4/4/4 · Arte y Patrimonio 4/4/–/–/– · Ciencias Naturales 6/6/–/–/– · Física –/–/4/4/4 ·
Química –/–/4/4/4 · Biología –/–/4/4/4 · Geografía, Historia y Ciudadanía 6/6/4/4/4 · Formación para la
Soberanía Nacional –/–/–/2/2 · Orientación y Convivencia 2/2/2/2/2.
**Total 34/34/34/36/34** (todas pares); áreas **8/8/9/10/10**.

**31060 — Ciencias y Tecnología** (10 áreas, 40 h cada año): Lengua y Literatura 4 · Idiomas 4 ·
Matemáticas 4 · Educación Física 4 · Biología, Ambiente y Tecnología 4 · Física 2/2/4/4/4 · Química
2/2/4/4/4 · Geografía, Historia y Soberanía Nacional 6/6/4/4/4 · Orientación Vocacional 4/4/2/2/2 ·
Innovación Tecnológica y Productividad 6/6/6/6/6. Áreas **10/10/10/10/10**.

Códigos `31059` y `31060`; nombres «Media General» y «Ciencias y Tecnología».

### 4.2 Catálogo (85) y cargas (95)

- Construcción **por año (1→5)**; dentro de cada año, primero las áreas de 31059 y después las de 31060.
  Nombre: `«<área> <año>º»`; si ya existe (compartida), **no** se duplica el catálogo, solo se añade la
  carga del otro plan.
- **Compartidas (16):** Matemáticas 1.º–5.º, Educación Física 1.º–5.º, Física 3.º–5.º y Química 3.º–5.º.
  Las horas coinciden en los dos planes. **En 5.º son 4** (Matemáticas, Educación Física, Física y Química).
- **Con carga:** 16 + 16 + 15 + 16 + 16 = **79**. **Sin carga (6):** «Participación en los Grupos de
  Creación, Recreación y Producción» **1.º a 5.º** (5) y «Ciencias de la Tierra 5.º» (1), al final, con
  `sinCarga = true` y sin enlace a ningún plan.
- **Cargas:** 31059 **45**, 31060 **50** → **95**.
- **Tipos:** general para todo salvo Educación Física (**Cancha**). Sin laboratorios ni talleres.

### 4.3 Cursos (24) con turno, aula y matrícula

| Turno | Plan | Curso | Aula | Alumnos | | Turno | Plan | Curso | Aula | Alumnos |
|---|---|---|---|---|---|---|---|---|---|---|
| Mañana | 31059 | 1A | 01 | 39 | | Tarde | 31059 | 2A | 01 | 37 |
| Mañana | 31059 | 1B | 02 | 39 | | Tarde | 31059 | 2B | 02 | 35 |
| Mañana | 31059 | 3A | 03 | 39 | | Tarde | 31059 | 2C | 03 | 34 |
| Mañana | 31059 | 3B | 04 | 35 | | Tarde | 31060 | 1C | 10 | 37 |
| Mañana | 31059 | 3C | 05 | 34 | | Tarde | 31060 | 1D | 11 | 36 |
| Mañana | 31059 | 4A | 12 | 44 | | Tarde | 31060 | 1E | 12 | 36 |
| Mañana | 31059 | 4B | 11 | 44 | | Tarde | 31060 | 2D | 04 | 32 |
| Mañana | 31059 | 4C | 10 | 41 | | Tarde | 31060 | 2E | 05 | 33 |
| Mañana | 31059 | 5A | 09 | 35 | | Tarde | 31060 | 3D | 07 | 37 |
| Mañana | 31059 | 5B | 08 | 42 | | Tarde | 31060 | 3E | 06 | 35 |
| Mañana | 31059 | 5C | 07 | 42 | | Tarde | 31060 | 4D | 09 | 32 |
| Mañana | 31059 | 5D | 06 | 39 | | Tarde | 31060 | 4E | 08 | 32 |

- **Matrícula:** 24 valores, todos en **32–44**; **Σ = 889** (**579** en 31059 y **310** en 31060).
  Se declara en el perfil (`CursoDef.matricula`) y **no** se persiste; la consumirá el preset.
- **Letras:** 31059 A, B (1.º), A–C (2.º–4.º), A–D (5.º); 31060 libres C–E (1.º) y D–E (2.º–4.º). Las 24
  parejas año+sección son únicas y de **un carácter**.
- **Plan y turno:** en `CursoDef`; se persiste **solo el turno** (`Cursos.turno`, ya existente). La
  configuración **no** sobrescribe el turno (capacidad futura, spec 006).
- 12 cursos por turno; ninguna aula se repite **dentro** de un turno.

### 4.4 Aulas (13) y tipos (2)

- `Aula 01`…`Aula 12`, **capacidad 44**, edificio y piso ficticios; `Cancha`, **capacidad 88**, tipo
  **`Cancha`**, ubicación «Patio».
- Tipos: general del esquema (`aula de estudio`) + `Cancha` → **2** leídos de la base.

### 4.5 Turnos, eventos y duración

| Turno | Ventana | Franjas | Eventos |
|---|---|---|---|
| Mañana | **07:00–12:40** | 8 × 40 | Receso (pos. 2, orden 1, 10 min, 08:20–08:30) · Acto cívico (pos. 2, orden 2, 10 min, 08:30–08:40) |
| Tarde | **12:40–18:10** | 8 × 40 | Receso (pos. 2, orden 1, 10 min, 14:00–14:10) |

Encaje: `8 × 40 + 20 = 340` y `8 × 40 + 10 = 330`. Rejilla aritmética: 07:00–12:20 y 12:40–18:00;
**desfase 20/10 min** hasta el fin de la ventana. Duración **40** (configuración).

### 4.6 Docentes (32)

Áreas declaradas **con el nombre exacto del catálogo**. Regla (§5.4): materias del área, una sola vez, sin
filtrar por año, y **solo si el área tiene cursos en el turno**; excepción `esAñadido` (D-4).

**Posiciones de la mañana (15):**
1. Matemáticas · Química — **10**
2. Castellano · Orientación y Convivencia — **10**
3. Física · Ciencias Naturales — **7**
4. Química · Ciencias Naturales — **7**
5. Geografía, Historia y Ciudadanía · Formación para la Soberanía Nacional · Orientación y Convivencia — **12**
6. Geografía, Historia y Ciudadanía — **5**
7. Castellano — **5**
8. Educación Física · Orientación y Convivencia — **10**
9. Matemáticas · Orientación y Convivencia — **10**
10. Arte y Patrimonio · Geografía, Historia y Ciudadanía — **7**
11. Educación Física · Castellano · Orientación y Convivencia — **15**
12. Biología · **Biología, Ambiente y Tecnología** · Física — **8** (Biología 3 + Física 5; BAT sin cursos en la mañana)
13. Inglés y otras lenguas extranjeras · Castellano · Biología · **Biología, Ambiente y Tecnología** — **13**
14. Inglés y otras lenguas extranjeras · Orientación y Convivencia — **10**
15. Biología · **Biología, Ambiente y Tecnología** · Inglés y otras lenguas extranjeras · Orientación y Convivencia — **13**

**Posiciones de la tarde (15):**
16. Orientación y Convivencia · Orientación Vocacional — **10**
17. Química · Física — **10**
18. Educación Física — **5**
19. Geografía, Historia y Ciudadanía · Geografía, Historia y Soberanía Nacional — **10**
20. Ciencias Naturales · Geografía, Historia y Ciudadanía · Orientación Vocacional — **12**
21. Matemáticas — **5**
22. Geografía, Historia y Ciudadanía · Arte y Patrimonio · Geografía, Historia y Soberanía Nacional — **12**
23. Lengua y Literatura — **5**
24. Educación Física · Innovación Tecnológica y Productividad — **10**
25. Innovación Tecnológica y Productividad — **5**
26. Orientación Vocacional — **5**
27. Castellano · Lengua y Literatura — **10**
28. Inglés y otras lenguas extranjeras · Idiomas — **10**
29. Biología · **Biología, Ambiente y Tecnología** — **5** (solo BAT: Biología no tiene cursos en la tarde)
30. Matemáticas · Física — **10**

**Añadidos:** **P031** «Participación en los Grupos…» (**mañana**) — **5** · **P032** «Ciencias de la
Tierra» (**tarde**) — **1**.

- Suma: mañana **147**, tarde **125** → **272** (comprobado por áreas en §4.7).
- **Identidad:** `P001`…`P032`, nombre ficticio del generador actual, correo `pNNN@centro-de-prueba.edu.ve`.
- **Tope 36**; **disponibilidad 5 filas** (días 1–5) con la ventana del turno en reloj (07:00–12:40 /
  12:40–18:10); el turno vive en el perfil (`DocenteDef::turno`) y **no** se persiste.
- **Anclas:** P029 solo BAT (5) y **no** Biología; P012/P013/P015 reciben Biología (3 cada una); P028 **10**.

### 4.7 Asignaciones: 272 (derivación por área)

| Área | Materias | Docentes | Total | Área | Materias | Docentes | Total |
|---|---|---|---|---|---|---|---|
| Castellano | 5 | 5 | 25 | Lengua y Literatura | 5 | 2 | 10 |
| Inglés y otras lenguas extranjeras | 5 | 4 | 20 | Idiomas | 5 | 1 | 5 |
| Matemáticas | 5 | 4 | 20 | Geografía, Historia y Soberanía Nacional | 5 | 2 | 10 |
| Educación Física | 5 | 4 | 20 | Orientación Vocacional | 5 | 3 | 15 |
| Arte y Patrimonio | 2 | 2 | 4 | Innovación Tecnológica y Productividad | 5 | 2 | 10 |
| Ciencias Naturales | 2 | 3 | 6 | Participación en los Grupos… (sin carga) | 5 | 1 | 5 |
| Física | 5 | 4 | 20 | Ciencias de la Tierra (sin carga) | 1 | 1 | 1 |
| Química | 5 | 3 | 15 | | | | |
| Biología | 3 | 3 (mañana) | 9 | | | | |
| Biología, Ambiente y Tecnología | 5 | 1 (P029) | 5 | | | | |
| Geografía, Historia y Ciudadanía | 5 | 6 | 30 | | | | |
| Formación para la Soberanía Nacional | 2 | 1 | 2 | | | | |
| Orientación y Convivencia | 5 | 8 | 40 | | | | |

**Total 272.** De ellas, **18** son **asignaciones a docentes de la tarde** de materias de **5.º de 31060**
(Lengua 2, Idiomas 1, Matemáticas 2, EF 2, BAT 1, Física 2, Química 1, GHS 2, OV 3, ITP 2); el **total de
esas 10 materias es 26**, porque las **4 compartidas** con 31059 5.º también se asignan a docentes de la
mañana. **6** son las materias sin carga (5 + 1) y **160** las disponibilidades (32 × 5).

### 4.8 Recuentos esperados (tabla única de verdad)

| Entidad | Esperado |
|---|---|
| `duracionMinutos` / `configuraciones` | **40** / **1** |
| turnos / eventos | 2 / 3 |
| tiposAula (general + cancha) | **2** |
| aulas | 13 |
| materias (79 con carga + 6 sin carga) | 85 |
| planes / planMaterias | 2 / 95 |
| cursos (12 + 12) | 24 |
| docentes / asignaciones / disponibilidades | 32 / **272** / **160** |
| preferencias | 0 |
| **matrícula (Σ cursos)** | **889** (579 + 310) |

## 5. Funciones puras (sin E/S, deterministas)

### 5.1 `int horasDeCurso(const Perfil&, const CursoDef&)`
Suma las `horas` de las cargas del plan del curso en su año. **Descartada:** `horasMaximas ×
seccionesDeAnio` (008), que mezcla planes y da 8 h en 1.º en vez de 34.

### 5.2 `int demandaTotal(...)` / `int demandaPorTurno(..., turno)`
Σ `horasDeCurso` global (**876**) o por turno (**414** / **462**).

### 5.3 `bool areaConCursosEnTurno(const Perfil&, const QString& area, const QString& turno)`
```
para cada curso c con c.turno == turno:
    para cada carga k del plan(c.plan) con k.anio == c.anio:
        si areaDe(k.materia) == area: true
false
```

### 5.4 `QStringList materiasDeDocente(const Perfil&, const QStringList& areas, const QString& turno, bool esAñadido)`
```
salida = []
para cada m en perfil.materias (orden del catálogo):
    si m.area no está en areas (comparación EXACTA): continuar
    si esAñadido o areaConCursosEnTurno(m.area, turno): salida << m.nombre
salida
```
**Excepción de D-4:** los dos añadidos pasan `esAñadido = true`. Sin expansión de nombres: el perfil debe
declarar **«Biología» y «Biología, Ambiente y Tecnología»** por separado (B3).

### 5.5 `QString motivoNoGenerable(const Perfil&)`
```
cancha = aulas de tipo "Cancha"; si no hay → "sin cancha"
dias = 5; aforo = 1
para cada turno:
    plazas = dias × turno.numSlots × aforo
    demandaEF = Σ horas de Educación Física de los cursos del turno
    si demandaEF > plazas → "«<turno>»: <demandaEF> clases de Educación Física frente a <plazas> franjas
                            de cancha (aforo 1). Pendiente el aforo simultáneo (dos cursos), que lo
                            resolverá su spec."
""
```
Con el centro: **48 > 40** en cada turno. No es validación: la aplicación no falla.

### 5.6 `Recuentos recuentosEsperados(const Perfil&)`
Tabla §4.8 (incluye `duracionMinutos = perfil.duracionMinutos`, `configuraciones = 1`).

### 5.7 `QString resumenTexto(const Perfil&, const Recuentos&)`
Centro; `Hora académica: 40 min (configuraciones: 1)`; recuentos; `Cursos: 24 (12 Mañana · 12 Tarde)`;
`Clases: 876 (EF 96; 48 por turno)`; **clases por plan y por año** (`31059 516 — 68/102/102/108/136` ·
`31060 360 — 120/80/80/80`, **sin** el 5.º, que no tiene cursos); **clases por turno 414/462**; materias por año;
horas por plan; `Matrícula: 889 (579 + 310)`; capacidad 576 y demanda 414/462;
`Generable: no — <motivoNoGenerable>`.

## 6. Validaciones (`validarPerfil` + `validarDocentes`)

1. **Duración** 30–90; **2 turnos** con nombre, 8 franjas y ventanas 07:00–12:40 / 12:40–18:10; eventos con
   nombre/posición/orden único/duración y **encaje con `perfil.duracionMinutos`** (no con la constante 35).
2. **Tipos:** catálogo no vacío y único; + general; cada aula con tipo del catálogo; **cada materia con un
   tipo del que haya aulas**.
3. **Catálogo:** nombres únicos, año 1–5, `sinCarga` coherente con los enlaces y **exactamente 6** sin carga.
4. **Progresión por plan** (31059 restringe; 31060 no).
5. **Cada plan:** sin repetidas, sin años vacíos, horas ≥ 0, **Σ ≤ 40 y par**.
6. **Tablas canónicas:** 31059 8/8/9/10/10 y 34/34/34/36/34 (45 cargas); 31060 10×5 y 40×5 (50).
7. **Recuentos:** 85 materias, 2 planes, 95 cargas.
8. **Cursos:** 24; año 1–5; sección de un carácter en mayúscula; (año, sección) única; plan y turno
   declarados; aula existente y **de tipo general**; **sin aula repetida dentro del turno**; 12 por turno;
   el plan tiene materias en ese año; **`matricula` en 32–44**.
9. **Aulas:** 13; nombres únicos; generales ≥ 44; **una** cancha ≥ 88.
10. **Docentes:** 32; ids/nombres/emails únicos; tope 36; **5 filas** en días 1–5 dentro de la ventana de su
    turno; **272** asignaciones; **cobertura**: cada (curso, materia) tiene docente en el turno del curso.

## 7. Sembrador

`aplicar`: validar perfil y docentes → apertura/esquema → datos de dominio → confirmación y respaldo (si
destruye) → transacción única: configuración **40** → tipo `Cancha` + id del general → turnos y eventos (3)
→ aulas (13) → materias (85) → planes (2) y **95** cargas por SQL directo → cursos (24) con
`crearCurso(anio, seccion, curso.turno, aula)` → docentes (32), asignaciones (272) y disponibilidades (160)
→ `recuentosCoinciden` (incluye duración y configuraciones) → `commit`.

Se elimina la rama de conmutación y las variantes; se conserva el reemplazo total, el respaldo, la
atomicidad y `simularFalloTrasDocentes`.

## 8. Tests

- **`test_seed_perfil` (T1, reescritura completa).** Planes (tablas, paridad, áreas por año), catálogo
  (85/79+6, 16 compartidas, 4 en 5.º, orden), cargas (95), cursos (24, turnos, letras, aulas, 12/turno,
  **matrícula 32–44 y Σ 889/579/310**), aulas/cancha, turnos y eventos (encaje 340/330 con 40), duración,
  docentes (32, tope, 5 filas, turno), asignaciones (**272**, tabla por área, P029 solo BAT, **18** de 5.º,
  añadidos 5+1), cobertura, `motivoNoGenerable` y rechazos.
- **`test_seed_aplicacion` (T1 + T5).** T1: recuentos §4.8 leídos de la base, 40 min y 1 configuración,
  24 cursos con 12/12 turnos, tipos 2, fuera variantes/`--solo-docentes`/suplentes. T5: determinismo,
  reemplazo con respaldo, atomicidad, `resumenTexto` con «no generable» y **15 s**.
- **`test_selector_generacion` (T1).** `"NAT"` → `"31059"` (preferencia del docente y su verificación).
- **`test_entrada_seed` (T4, sustituye a `test_generacion_seed`).** Siembra, compone con
  `construirEntrada` los **24 cursos** (plan, turno y **`numEstudiantes = matricula`**), exige sin avisos,
  valida (V1–V19), ida y vuelta del preset (24 cursos, 32 docentes, 40 min) y **no resuelve**; comprueba el
  «no generable».
  **Descartadas:** retirarlo (pierde la única cobertura de `construirEntrada`) y reescribirlo «contra el
  fixture de la 014» (es un `SolverConfig`, no datos de base).

## 9. Documentación

`docs/guias/datos-prueba.md` (T6), `docs/guias/datos-prueba-frontend.md` (T7), `docs/interfaz-frontend.md` y
`docs/guias/migracion-frontend.md` y `docs/guias/desarrollo/CMAKE_GUIDE.md` (T9), supersesión en
`specs/008-datos-de-prueba/spec.md` y su `estado.md` (T8) y parche de `MEMORY.md` entregado (T8).

**V6:** informar el «no generable» **no es función de interfaz** en esta entrega. El artefacto es el
**informe del conjunto** (y la CLI); las guías lo **describen**, no lo prometen como pantalla, aviso o
diálogo, y la guía del frontend no debe contradecir RF-9. **Estado:** la guía del frontend aún menciona el
`NoFactible` «en la vista» → **deuda declarada** (se retira cuando el frontend implemente el aforo).

## 10. Trazabilidad RF → tarea → verificación

| RF / RNF | Tarea | Verificación (comando + resultado) |
|---|---|---|
| RF-1 | T1, T3 | `ctest --preset backend -R test_seed_aplicacion` verde sin variantes; `--variante holgada` → **código 1** |
| RF-2 | T1 | `-R test_seed_perfil` → 34/34/34/36/34, 40×5, paridad, 8/8/9/10/10, 10×5 |
| RF-3 | T1 | → 85 (79+6), 95, 16 compartidas, 4 en 5.º, tipos con aulas |
| RF-4 | T1, T9 | → 24 cursos, 12/12, letras únicas, turno persistido, plan no; **matrícula 889 (579+310)**; T9 en la UI |
| RF-5 | T1 | → 13 aulas, 44/88, cancha no referencia, sin repetir aula en el turno |
| RF-6 | T1, T9 | → 32 docentes, 272, 160, tope 36, cobertura por turno |
| RF-7 | T1 | → ventanas, 3 eventos, encaje 340/330 con 40 |
| RF-8 | T1, T2, T5 | → recuentos §4.8, determinismo, `resumenTexto` con **clases por plan (31059 516 · 31060 360)**, **por año (68/102/102/108/136 y 120/80/80/80)** y por turno (414/462) |
| RF-9 | T2, T4 | → `motivoNoGenerable` 48/40; `-R test_entrada_seed` válida sin resolver |
| RF-10 | T6, T7, T8, T9 | → guías de **datos** sin recuentos obsoletos y con los nuevos (T6/T7/T9); `docs/interfaz-frontend.md` y `docs/guias/migracion-frontend.md` al día (T9); `CMAKE_GUIDE.md` solo con la **nota de la 017** (su apartado demo queda fuera del grep, H7); nota de la 008 (T8); UI con 24/32 (T9) |
| RNF-1 | T5 | `tiempoDeAplicacion` < 15 s |
| RNF-2 | T4 | `grep resolverEntradaSolver` sin resultados en el test |
| RNF-3 | T10 | `git diff --stat` sin `database/`, `solver/`, `frontend/src` |
| RNF-4 | T1, T5 | dos bases con `filasDe` idénticas |
| RNF-5 | T1 | rechazos con plan/año/curso/área y valor |
| RNF-6 | T1 | `P0NN` ficticios; sin apellidos del documento |
| RNF-7 | T1, T10 | tabla §4.8 y §10 sin RF sin verificación |
| RNF-8 | T1 | aritmética §4.8 verificada |

## 11. Verificación

```bash
# backend (perfil, docentes, sembrador, recuentos):
cmake --preset backend && cmake --build --preset backend --parallel
ctest --preset backend --output-on-failure

# completa (incluye frontend):
cmake --preset full-release && cmake --build build-rel --parallel
ctest --preset full-release --output-on-failure

# binario del preset backend:
build-backend/src/seed/gestor-horarios-seed --db /tmp/c17.db
```

**Criterio de cierre de cada tarea:** `cmake --build` en verde (backend y, si toca frontend, full-release) y
**sus** tests acotados en verde. Ninguna tarea cierra con el build ni con la suite en rojo.

## 12. Riesgos

| # | Riesgo | Mitigación |
|---|---|---|
| 1 | **Literales de la 008** (57/13/16/65/210) en tests y validación | Se sustituyen en T1 junto con el dato; `grep` al cerrar |
| 2 | **Tiempo de aplicación** (32 docentes, 272 asignaciones, 160 disponibilidades) | `tiempoDeAplicacion` < 15 s (T5) |
| 3 | **`test_selector_generacion` y el frontend** | T1 cambia `"NAT"` → `"31059"` (B2); T9 revisa la UI con 24/32 |
| 4 | **Los 2 añadidos sin carga** | La cobertura se exige por curso; el test fija 5 y 1 |
| 5 | **`test_generacion_seed`** | Baja de CMake en T1 (hueco **declarado**), cerrado por `test_entrada_seed` en T4 |
| 6 | **`test_seed_aplicacion`** | Se actualiza **en T1** (no se retira): literales y slots de variantes |
| 7 | **Turno del curso** | **Se escribe en el curso** (D-8); la configuración **no** lo sobrescribe (H10, spec 006); el frontend no se toca |
| 8 | **Paquete demo** | `GESTOR_PACK_DEMO`/`datos-demo/` **no** se regeneran (T6/T8 lo documentan) |
| 9 | **Privacidad** | `seed.md` versionado con nombres reales; el conjunto usa ficticios y no se amplía |
| 10 | **Exceso de 12 h en Inglés/Idiomas (tarde)** | Blando (S1); no se valida como tope; se documenta (T6) |

## 13. Tamaño y aceptación del exceso

**10 tareas.** Duraciones previstas: **T1 ≈ 150 min** (unidad atómica: perfil + docentes + sembrador +
sus tres tests), **T2 ≈ 40**, **T3 ≈ 30**, **T4 ≈ 50**, **T5 ≈ 40**, T6–T8 ≈ 20–30 cada una, **T9 ≈ 30**,
**T10 ≈ 40**.

**El usuario aprobó las 10 tareas con el exceso el 2026-11-08** (T1 ≈ 150, T4 ≈ 50 y T5 ≈ 40 por encima de
30 min). La razón es estructural: el perfil, los docentes y el sembrador **no se pueden sustituir por capas**
sin dejar la suite roja (las variantes y la plantilla vieja dependen del catálogo viejo).

**Alternativa propuesta si no se acepta el exceso (corte en dos entregas):**

- **017a — sustitución del conjunto (T1–T5):** datos, docentes, sembrador, recuentos, CLI y la suite del
  seed. Incluye el único test nuevo (`test_entrada_seed`).
- **017b — documentación y verificación en la interfaz (T6–T10):** las dos guías, la supersesión de la 008 y
  la revisión de la UI con el centro real.

La numeración de tareas no cambia con el corte; solo se entregan en dos tandas.
