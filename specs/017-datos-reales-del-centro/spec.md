# Spec 017 - Datos reales del centro (Liceo Nacional Robert Serra)

Estado: implementada (2026-11-09)

> **Corrección de datos (2026-11-10).** El centro real **no generaba**: en la tarde, **P028** era la única
> docente habilitada de **Idiomas** (36 clases de 31060) **e Inglés de 2.º** (12 de 31059) —**48 clases en un
> turno de 40 franjas**— y el motor devolvía `INFEASIBLE` incluso sin Educación Física. Se **añade un docente
> de apoyo, P033** (tarde), con **Idiomas** de 31060; **P028 conserva Inglés** de 31059 2.º. Con eso,
> **P033 = 36 clases** y **P028 = 12** (ambos ≤ 40 franjas). Los recuentos pasan a **33 docentes**,
> **165 disponibilidades** y **272 asignaciones** (las 5 filas de Idiomas pasan de P028 a P033: la suma no
> cambia); la capacidad docente es **1188** (**576** mañana + **612** tarde). El resto de la spec no cambia.

> **Supersesión por la 018 (2026-11-10).** Quedan **superados** el **RF-9** (el conjunto se declara «no
> generable»), el **RF-10** en lo que toca a la **aceptación diferida** y **D-5/D-9** (EF en la cancha con el
> aforo como spec futura): con el **aforo 2** de la 018 y la **corrección de datos P033** (2026-11-10) el
> informe pasa a **«Generable: sí»** y la aceptación se hace con el **test lento** de la 018 (876 clases,
> desvío 0, < 300 s). El resto de la 017 sigue vigente.

> **Alcance.** Esta spec define el **conjunto de datos de desarrollo con el centro real** del periodo
> **2026/2027**, que **sustituye** al conjunto sintético de la 008: dos planes (**31059** Media General y
> **31060** Ciencias y Tecnología), **24 cursos** repartidos en **dos turnos**, **12 aulas de uso general
> + 1 cancha**, **33 docentes** (30 del centro, 2 añadidos y 1 de apoyo) y la matrícula del centro (**889 alumnos**)
> documentada en el perfil. El conjunto **puebla una base de pruebas**; **no se declara generable** hasta
> que exista el aforo simultáneo de la cancha (spec futura).
>
> **De dónde salen los datos.** Del documento del centro que el usuario mantiene versionado en la raíz del
> repositorio (`seed.md`: estadística general de alumnos, turnos, aulas, plantilla docente y planes de
> estudio del periodo 2026/2027). Es la **fuente única**: las cifras de esta spec salen de él y se han
> **contrastado aritméticamente** entre sí.
>
> **Relación con las specs anteriores.**
> - La **008** queda sustituida en lo que toca a los datos del centro: sus recuentos, su plan General
>   «adaptado a 40 h», sus tres planes derivados, sus cuatro variantes de docentes, la regla «cada curso
>   con un aula propia y distinta» y la siembra de cursos **sin turno**.
> - La **014** dejó fuera de su alcance *«la sustitución del conjunto de datos de prueba por los planes
>   reales del centro»*: **es esta spec**. Su spec futura de **aforo simultáneo por aula** es la que
>   habilitará la generación de este conjunto.
> - La **011** (hora académica) se usa con **40 min** (decisión D-7) y la **015/016** (tipo general) se
>   mantienen intactas.

## Decisiones cerradas (2026-11-07)

| # | Decisión | Resolución |
|---|---|---|
| D-1 | Alcance | El centro real **sustituye** al sintético. **Se retiran** las cuatro variantes de docentes y los tres planes derivados (EXA/NAT/SOC). **Un solo conjunto, una sola aplicación.** |
| D-2 | Materias | **Una materia por área y año**, **compartida** entre planes cuando el nombre coincide. «Biología, Ambiente y Tecnología» y «Biología» son **áreas distintas**. El **5.º de 31060** se declara **sin cursos**. |
| D-3 | Horas del docente | Tope de **36 horas académicas (bloques)** para **todos** los docentes: las 40 h del centro incluyen «fuera de aula» y se descuentan. Son horas académicas, no horas de reloj. |
| D-4 | Áreas sin titular | Se **excluyen de los planes** «Participación en los Grupos de Creación, Recreación y Producción» y «Ciencias de la Tierra». Sus materias **permanecen en el catálogo** (sin horas en ningún plan) y se **añaden 2 docentes** para ellas, para nuestras pruebas. |
| D-5 | Cancha y EF | **Educación Física se imparte en la cancha.** El **aforo simultáneo (2 cursos con docentes distintos)** es una spec **posterior**; mientras no exista, **el conjunto no genera** y la aceptación de generación se difiere. |
| D-6 | Aulas | 12 aulas generales **compartidas entre turnos**, **aula de referencia fija** por curso, **capacidad 44**. |
| D-7 | Turno y franja | **40 min**; mañana **07:00–12:40** y tarde **12:40–18:10**, con **receso en ambos turnos** y **acto cívico** en la mañana. |
| D-8 | Turno del curso | El **turno se siembra en el curso** (es dato del centro). **Sobrescribirlo desde la configuración no existe hoy** (capacidad futura, spec 006). |
| D-9 | Aceptación | Ahora se validan **los datos** (planes, cursos, aulas, docentes, recuentos). La **generación** (876 clases sembradas; 944 en el plan real completo, menos de 300 s, desvío 0, separación informada, huecos al final) se exigirá **al cerrar el aforo**. |
| D-10 | Destino | **Solo conjunto de desarrollo.** El paquete de datos del liceo, más adelante. |
| D-11 | Matrícula | Capacidad de aula **44** y matrícula por curso **documentada en el perfil**, sin tocar el modelo. |
| D-12 | Cursos homónimos | En el conjunto, los cursos de 31060 que chocan con 31059 usan **letras libres** del año (documentado). La **identidad (año, sección, plan)** será una **spec propia**. |
| D-13 | Nombres | **Nombres ficticios** con la misma estructura: 30 posiciones, **mismas áreas y mismos turnos**, y **tope de aula 36**. |

> **Corrección aritmética de D-7 (documentada).** La mañana lleva **dos** eventos de 10 min (receso y
> acto cívico), así que su ventana cierra a las **12:40** (`8 × 40 + 10 + 10 = 340`), no a las 12:30: el
> fixture de la 014 tenía **solo el receso** (`8 × 40 + 10 = 330`). La tarde, con un solo receso, cierra a
> las **18:10**. La rejilla de clases va de 07:00 a **12:20** y de 12:40 a **18:00**; los eventos no
> desplazan las franjas (011/014).

> **Qué se persiste y qué vive en el perfil (aclaración de redacción del QA, 2026-11-07).** El **plan de
> cada curso** y el **turno de cada docente** son atributos del **perfil del conjunto** y **no se
> persisten** (el modelo de cursos no tiene plan y el de docentes no tiene turno): el plan lo aporta la
> configuración al generar y el turno del docente se **materializa en su disponibilidad**. La **duración
> de la hora académica** es **configuración** (una fila), no dato de dominio, y el conjunto la deja en
> **40 min**. Las **40 h / 36 h del documento** son el **contrato del centro**, no el tope sembrado: el
> tope es **36 horas académicas** para todos (D-3).

## Contexto y objetivo

La base del cliente se puebla hoy con un centro **ficticio** (plan 31059 adaptado, tres planes derivados,
13 cursos, cuatro variantes de profesorado). Sirve para probar el motor, pero no es el centro del
usuario. El centro ya confirmó sus datos del periodo 2026/2027 y quiere que **el conjunto de prueba sea su
centro** y que la aplicación lo muestre tal cual.

El conjunto real no cabe en las reglas del anterior: **12 aulas para 24 cursos** (el aula propia y
distinta por curso es imposible), **una sola cancha** para toda la Educación Física, **dos cursos
homónimos** («1.º A» en los dos planes) que la identidad vigente del curso (año + sección) no distingue, y
**dos áreas del plan sin ningún titular** en el documento. Esta spec fija cómo se siembra el centro con
esas restricciones, qué se excluye y qué queda **pendiente del aforo simultáneo**.

**Por qué la generación se difiere.** Con la exclusividad de aula vigente (una clase por aula y franja),
la Educación Física del centro no cabe en la cancha: **48 clases por turno** frente a **40 franjas** de
cancha por turno. El aforo simultáneo (dos cursos en la cancha con docentes distintos) lo resuelve
(**96 ≤ 160** en la semana; **48 ≤ 80** por turno) y es una spec aparte. Hasta entonces, el conjunto se
valida **por datos** y **no se declara generable**.

**Qué no es.** No es un importador (no se leen hojas de cálculo ni se captura a mano), no genera presets,
no cambia el motor, el esquema ni el contrato de configuración, y no modela el aforo simultáneo.

## Usuarios

- **Dirección y coordinación del liceo**, que quieren ver **su** centro (sus dos planes, sus 24
  secciones, sus 12 aulas, su cancha y su personal) y no un centro de ejemplo.
- **Usuario académico que genera horarios**, que necesita saber **por qué** todavía no se genera y qué
  falta, en lugar de recibir un error opaco.
- **Equipo de desarrollo y de pruebas**, que necesita un conjunto **determinista**, con recuentos
  conocidos, para escribir pruebas del centro real.
- **Responsable de los datos del centro**, que aporta el documento y necesita que lo sembrado coincida con
  lo declarado o que la diferencia esté documentada.

## Historias de usuario

- HU-1. Como dirección del liceo, quiero que la aplicación traiga **mi centro** (dos planes, 24 cursos,
  12 aulas y una cancha, 33 docentes) para trabajar con datos reales.
- HU-2. Como usuario académico, quiero poder **leer en el informe del conjunto y en la guía de datos de
  prueba** qué falta para generar mi horario (el aforo de la cancha), en lugar de encontrarme un fallo sin
  explicación.
- HU-3. Como usuario académico, quiero que las 12 aulas **se compartan entre turnos** y que cada curso
  tenga **su aula de referencia**, porque el liceo no tiene 24 aulas.
- HU-4. Como miembro del equipo de pruebas, quiero recuentos y estructuras estables del centro real para
  afirmar sobre ellos sin recalcularlos.
- HU-5. Como responsable de los datos, quiero que las diferencias entre el documento y lo sembrado
  (áreas excluidas, letras de sección, materias sin horas) estén **documentadas**.
- HU-6. Como responsable de datos personales, quiero que el conjunto versionado **no contenga nombres de
  personas reales**.

## Definiciones

- **Conjunto del centro:** datos persistidos del Liceo Nacional Robert Serra (2026/2027): planes, materias,
  cursos con su turno, turnos con sus eventos, aulas y cancha, docentes con sus materias y disponibilidad, y
  la matrícula de referencia en el perfil. **No** incluye configuración de cálculo ni horarios.
- **Conjunto único:** hay **un solo** conjunto y **una sola** forma de aplicarlo: no hay variantes de
  docentes, ni planes derivados, ni selección de perfil.
- **Hora académica:** una hora = una franja = un bloque (005/011). Su duración real es de **40 min**
  (D-7); no hay conversión.
- **Duración de la hora académica (configuración):** valor **global del centro** en una sola fila de
  configuración (011), **no** un dato de dominio. Aplicar el conjunto la deja en **40 min**; si ya vale 40,
  no cambia nada.
- **Materia-año:** un área en un año concreto («Castellano 1.º», «Física 3.º»). El modelo admite **una
  materia por plan y año**; por eso cada área se representa **por año**.
- **Materia compartida:** materia-año que usan **los dos planes** cuando el nombre del área coincide
  (Matemáticas, Educación Física, Física, Química). Una sola materia, con la carga de cada plan.
- **Materia sin carga (excepción de D-4):** materia-año del catálogo que **no pertenece a ningún plan**;
  existen **exactamente 6** («Participación…» 1.º–5.º y «Ciencias de la Tierra» 5.º) y se siembran para
  probar docentes habilitados sin materia que impartir.
- **Curso:** año, sección, **turno** y **aula de referencia**, que son lo que se **persiste**. Hay **24**.
  El **plan de cada curso no se persiste**: lo declara el **perfil del conjunto** (para recuentos y
  clases) y lo aporta la **configuración** al generar.
- **Letra libre:** letra de sección que ningún curso del mismo año usa todavía (D-12). En el conjunto, los
  cursos de 31060 la usan para no chocar con los de 31059; la letra **no** es la del documento y la
  corrección real (identidad con plan) es otra spec.
- **Aula de referencia:** aula de uso general asignada a un curso; es la que el motor usa por defecto
  cuando el tipo coincide (010). Su ubicación (edificio y piso) es **ficticia y opcional**.
- **Cancha:** instalación deportiva, **una sola**, de tipo propio, capacidad para **dos grupos**; no es
  aula de referencia de ningún curso.
- **Actividad «fuera de aula»:** tarea no lectiva incluida en las 40 h del centro (planificación y
  formación, CRP, atención familiar, administrativo, coordinación, bienestar, OC). Se **descuenta** y da el
  tope de **36 horas académicas**; el horario **no la programa**.
- **Generabilidad:** propiedad de permitir que el motor encuentre un horario válido con las reglas de la
  014. **Esta entrega no la exige** (D-5, D-9).

## Datos del conjunto

### Plan 31059 — Media General (tras excluir las dos áreas de D-4)

| Área de formación | 1.º | 2.º | 3.º | 4.º | 5.º |
|---|---|---|---|---|---|
| Castellano | 4 | 4 | 4 | 4 | 3 |
| Inglés y otras lenguas extranjeras | 4 | 4 | 4 | 4 | 3 |
| Matemáticas | 4 | 4 | 4 | 4 | 4 |
| Educación Física | 4 | 4 | 4 | 4 | 4 |
| Arte y Patrimonio | 4 | 4 | – | – | – |
| Ciencias Naturales | 6 | 6 | – | – | – |
| Física | – | – | 4 | 4 | 4 |
| Química | – | – | 4 | 4 | 4 |
| Biología | – | – | 4 | 4 | 4 |
| Geografía, Historia y Ciudadanía | 6 | 6 | 4 | 4 | 4 |
| Formación para la Soberanía Nacional | – | – | – | 2 | 2 |
| Orientación y Convivencia | 2 | 2 | 2 | 2 | 2 |
| **Total sembrado** | **34** | **34** | **34** | **36** | **34** |
| **Áreas por año** | 8 | 8 | 9 | 10 | 10 |
| *(Excluidas: Participación en los Grupos)* | 4 | 4 | 4 | 4 | 4 |
| *(Excluida: Ciencias de la Tierra)* | – | – | – | – | 2 |
| *Total del plan real* | *38* | *38* | *38* | *40* | *40* |

### Plan 31060 — Ciencias y Tecnología

| Componente | Área de formación | 1.º | 2.º | 3.º | 4.º | 5.º |
|---|---|---|---|---|---|---|
| General | Lengua y Literatura | 4 | 4 | 4 | 4 | 4 |
| General | Idiomas | 4 | 4 | 4 | 4 | 4 |
| General | Matemáticas | 4 | 4 | 4 | 4 | 4 |
| General | Educación Física | 4 | 4 | 4 | 4 | 4 |
| General | Biología, Ambiente y Tecnología | 4 | 4 | 4 | 4 | 4 |
| General | Física | 2 | 2 | 4 | 4 | 4 |
| General | Química | 2 | 2 | 4 | 4 | 4 |
| General | Geografía, Historia y Soberanía Nacional | 6 | 6 | 4 | 4 | 4 |
| Científico-tecnológico | Orientación Vocacional | 4 | 4 | 2 | 2 | 2 |
| Científico-tecnológico | Innovación Tecnológica y Productividad | 6 | 6 | 6 | 6 | 6 |
| | **Total sembrado** | **40** | **40** | **40** | **40** | **40** |
| | **Áreas por año** | 10 | 10 | 10 | 10 | 10 |

> El plan 31060 declara **5.º** con sus 10 áreas y 40 h, pero **no hay cursos de 5.º** este periodo
> (D-2). El 5.º del 31060 no aporta clases.

### Horas por curso y clases

| Plan | Año | Secciones | Horas/curso | Cursos | Clases |
|---|---|---|---|---|---|
| 31059 | 1.º | A, B | 34 | 2 | 68 |
| 31059 | 2.º | A, B, C | 34 | 3 | 102 |
| 31059 | 3.º | A, B, C | 34 | 3 | 102 |
| 31059 | 4.º | A, B, C | 36 | 3 | 108 |
| 31059 | 5.º | A, B, C, D | 34 | 4 | 136 |
| 31060 | 1.º | C, D, E | 40 | 3 | 120 |
| 31060 | 2.º | D, E | 40 | 2 | 80 |
| 31060 | 3.º | D, E | 40 | 2 | 80 |
| 31060 | 4.º | D, E | 40 | 2 | 80 |
| | | | | **24** | **876** |

- **Paridad:** todos los cursos suman un número **par** de horas (34, 36 o 40), requisito de los bloques
  de dos horas de la 014. **34 → 17 bloques** (margen 3, 6 huecos), **36 → 18** (margen 2, 4 huecos) y
  **40 → 20** (margen 0, sin huecos); **84 huecos** en el centro.
- **Clases:** **876** en el conjunto sembrado; **96** son de Educación Física (**4 h × 24 cursos**, 48 por
  turno) y van a la cancha; sin EF serían **780**. Las **68** clases de las dos áreas excluidas
  (60 de Participación + 8 de Ciencias de la Tierra) no se siembran; con ellas el plan real daría **944**.
- **Demanda por turno:** mañana **414** (12 cursos de 31059) y tarde **462** (31059 2.º: 102, y 31060: 360).

### Cursos, turnos y aulas de referencia

| Turno | Plan | Cursos | Nº | Aulas de referencia |
|---|---|---|---|---|
| **Mañana** | 31059 | 1A, 1B, 3A, 3B, 3C, 4A, 4B, 4C, 5A, 5B, 5C, 5D | 12 | 1–12 |
| **Tarde** | 31059 | 2A, 2B, 2C | 3 | 1–3 |
| **Tarde** | 31060 | 1C, 1D, 1E, 2D, 2E, 3D, 3E, 4D, 4E | 9 | 4–12 |

- **Plan por curso (no persistido):** el plan de la columna es del **perfil**; la base guarda **año,
  sección, turno y aula de referencia**. La configuración aporta el plan al generar.
- **Letras (D-12):** 31059 ocupa **A, B** en 1.º, **A, B, C** en 2.º–4.º y **A–D** en 5.º; 31060 usa las
  **letras libres** siguientes: **C, D, E** en 1.º y **D, E** en 2.º–4.º (31060 no tiene 5.º con cursos).
  Las 24 parejas año+sección son **únicas**; el documento asigna A/B/C también a 31060 y la colisión se
  resuelve así, documentada.
- **Erratas del documento (H19):** la fila «4to … A-B-C … 31059 … 9-8» de la tarde es de **31060**
  (4.º A y B, aulas 9-8), y el **1.º de 31060 tiene tres secciones** (A, B, C con aulas 10-11-12), no dos.
  El conjunto aplica la lectura corregida, ya confirmada en la 014.
- **Matrícula:** 32–44 alumnos por sección (4.º A y 4.º B de 31059 con 44); **889** en total (579 de 31059
  y 310 de 31060). Vive en el **perfil**, no en el modelo (D-11).

### Docentes

- **33 docentes:** las **30 posiciones** del documento (15 y 15, **sin cruzar turnos**) con **nombres
  ficticios** (D-13) y **mismas áreas y mismos turnos**, más **2 añadidos** para las materias sin carga de
  D-4 (uno por turno) y **1 de apoyo** (P033, tarde) para **Idiomas**, añadido en la **corrección de datos
  del 2026-11-10** (ver la nota tras el estado). El **tope sembrado es 36 horas académicas**; las 40 h/36 h
  del documento son su **contrato**, no el tope.
- **Tope:** **36 horas académicas** en los 33 (D-3). Capacidad total **1188**: **576** en la mañana
  (16 × 36) frente a **414** de demanda, y **612** en la tarde (17 × 36) frente a **462**.
- **Regla de asignación (fijada):** cada docente recibe **todas las materias-año del catálogo de las áreas
  que el documento le declara**, contando **una sola vez** las compartidas y **sin filtrar por año**; una
  materia se asigna a un docente **solo si su área tiene cursos en el turno de ese docente**. «BIOLOGÍA»
  habilita para **Biología** (31059, solo mañana) y para **Biología, Ambiente y Tecnología** (31060, solo
  tarde): por la regla, la primera va a los **tres** docentes de biología de la mañana y la segunda
  **solo a P029**, la única de BAT en la tarde. Las **materias de 5.º de 31060** (sin cursos) **sí cuentan**,
  porque su área tiene cursos en la tarde; los **2 añadidos** cuentan con sus **6** materias sin carga y
  **P033** con las **5** de Idiomas. **Total: 272 asignaciones**, derivables de la tabla siguiente
  (docentes habilitados × materias del área).
- **Mapeo de áreas del documento al catálogo** (nombres del documento → área sembrada):

| Documento | Área del catálogo |
|---|---|
| CASTELLANO | Castellano |
| INGLES | Inglés y otras lenguas extranjeras |
| MATEMATICAS | Matemáticas (compartida) |
| EDU. FISICA / EDUC. FISICA | Educación Física (compartida) |
| ARTE Y PATRIMONIO | Arte y Patrimonio |
| CS. NATURALES | Ciencias Naturales |
| FISICA | Física (compartida) |
| QUIMICA | Química (compartida) |
| BIOLOGIA | Biología **y** Biología, Ambiente y Tecnología (dos áreas distintas; habilita para ambas y la regla de turno decide a cuál se asigna) |
| GHC | Geografía, Historia y Ciudadanía |
| GHSN | Geografía, Historia y Soberanía Nacional |
| FSN | Formación para la Soberanía Nacional |
| ORIENTACION Y CONVIVENCIA | Orientación y Convivencia |
| ORIENTACION VOCACIONAL | Orientación Vocacional |
| LENGUAJE Y LITERATURA | Lengua y Literatura |
| IDIOMAS | Idiomas |
| ITP | Innovación Tecnológica y Productividad |

- **Los 2 añadidos:** uno imparte «Participación en los Grupos…» (5 materias-año sin carga) y el otro
  «Ciencias de la Tierra» (1 materia-año sin carga); **no** dan clase en ningún curso sembrado y **sí
  cuentan** en las asignaciones (5 + 1).
- **El docente de apoyo (P033, corrección del 2026-11-10):** **tarde**, área **Idiomas** (5 materias-año);
  **P028** pasa a **Inglés** (5). Reparto: **P033 = 36 clases** (las 9 secciones de 31060) y **P028 = 12**
  (Inglés de 2.º), ambos **por debajo de las 40 franjas** del turno. No se tocan horas ni el resto de la
  plantilla.
- **Derivación del total (272):**

| Área (materias-año) | Docentes habilitados | Asignaciones |
|---|---|---|
| Castellano (5) | 5 | 25 |
| Inglés (5) | 4 | 20 |
| Matemáticas (5) | 4 | 20 |
| Educación Física (5) | 4 | 20 |
| Arte y Patrimonio (2) | 2 | 4 |
| Ciencias Naturales (2) | 3 | 6 |
| Física (5) | 4 | 20 |
| Química (5) | 3 | 15 |
| Biología (3) | 3 (solo mañana) | 9 |
| Biología, Ambiente y Tecnología (5) | 1 (P029, tarde) | 5 |
| Geografía, Historia y Ciudadanía (5) | 6 | 30 |
| Formación para la Soberanía Nacional (2) | 1 | 2 |
| Orientación y Convivencia (5) | 8 | 40 |
| Lengua y Literatura (5) | 2 | 10 |
| Idiomas (5) | 1 (P033, tarde) | 5 |
| Geografía, Historia y Soberanía Nacional (5) | 2 | 10 |
| Orientación Vocacional (5) | 3 | 15 |
| Innovación Tecnológica y Productividad (5) | 2 | 10 |
| Participación en los Grupos… (5, sin carga) | 1 (añadido) | 5 |
| Ciencias de la Tierra (1, sin carga) | 1 (añadido) | 1 |
| **Total** | | **272** |

- **De las 272, 18 son asignaciones a docentes de la tarde** de materias de 5.º de 31060 (año sin
  cursos, pero con área en la tarde); el **total de esas 10 materias es 26**, porque las **4 compartidas**
  con 31059 5.º (Matemáticas, Educación Física, Física y Química) también se asignan a docentes de la
  mañana. Dato válido, no un error.
- **Turno del docente (no persistido):** el turno es del **perfil** y se **materializa en la
  disponibilidad** (un rango por día dentro de la ventana de ese turno); el modelo de docentes no tiene
  turno. La plantilla queda en **16 en la mañana y 17 en la tarde** (los 30 del centro + 2 añadidos +
  P033), y se comprueba en el perfil.
- **Cobertura por turno:** las **12 áreas** de 31059 en la mañana y las **8** de 2.º más las **10** de
  31060 en la tarde tienen al menos un docente habilitado en su turno.
- **Disponibilidad:** **un rango por día, de lunes a viernes**, que cubre la **ventana completa del turno**
  en reloj (07:00–12:40 o 12:40–18:10), de modo que cubre las 8 franjas.

### Recuentos esperados

| Entidad | Esperado |
|---|---|
| Duración de la hora académica (configuración, no dominio) | 40 min |
| Turnos | 2 |
| Eventos (receso ×2, acto cívico) | 3 |
| Tipos de aula (leídos de la base: general + cancha) | 2 |
| Tipos de aula declarados por el conjunto | 1 (cancha) |
| Aulas (12 generales + 1 cancha) | 13 |
| Materias del catálogo (79 con carga + 6 sin carga) | 85 |
| Planes de estudio | 2 |
| Cargas plan-materia | 95 |
| Cursos | 24 |
| Docentes | 33 |
| Materias asignadas | 272 |
| Disponibilidades (33 × 5 días) | 165 |
| Preferencias | 0 |

## Requisitos funcionales

### RF-1. Conjunto único del centro

- **EL SISTEMA dispondrá de un único conjunto de datos** del centro y lo aplicará **en una sola forma**:
  no habrá variantes de docentes, ni planes derivados, ni selección de perfil.
- **EL SISTEMA retirará del conjunto los planes derivados de la 008** (Ciencias Exactas, Ciencias
  Naturales y Ciencias Sociales) y **sus materias propias**; el catálogo solo contendrá áreas del 31059 y
  del 31060.
- **EL SISTEMA retirará las cuatro variantes de docentes** y su conmutación; la plantilla sembrada es la
  del centro más los dos docentes añadidos. **No habrá ninguna forma de elegir ni de conmutar variante**
  (ni una opción en la aplicación ni una operación posterior): el conjunto es uno solo.
- **EL SISTEMA mantendrá intactas las garantías de aplicación segura** de la 008: base de destino
  obligatoria, fallo si la base tiene datos, confirmación explícita y respaldo previo para el reemplazo
  total, atomicidad y ningún estado intermedio.

### RF-2. Planes de estudio y horas por año

- **EL SISTEMA sembrará los planes 31059 y 31060** con **sus áreas y sus horas académicas por año**, tal
  como figuran en las tablas de esta spec: **34/34/34/36/34** en 31059 (tras excluir las dos áreas de
  D-4) y **40/40/40/40/40** en 31060.
- **EL SISTEMA respetará la progresión de cada plan:** Arte y Patrimonio y Ciencias Naturales solo en 1.º
  y 2.º; Física, Química y Biología desde 3.º; Formación para la Soberanía Nacional desde 4.º; en el 31060,
  Orientación Vocacional e Innovación Tecnológica y Productividad en los cinco años.
- **EL SISTEMA declarará el 5.º del 31060 con sus áreas y horas y sin cursos**, y lo documentará.
- **EL SISTEMA verificará que cada año sembrado suma un número par de horas académicas** y que coincide con
  la tabla; si no, rechazará la aplicación indicando plan, año y diferencia.
- **EL SISTEMA declarará y contrastará las áreas por año:** 31059 **8/8/9/10/10**; 31060
  **10/10/10/10/10**.
- **EL SISTEMA no sembrará** «Participación en los Grupos de Creación, Recreación y Producción» ni
  «Ciencias de la Tierra» **en ningún plan**, y no las contará en las horas ni en las clases.

### RF-3. Catálogo de materias y tipos de aula

- **EL SISTEMA sembrará una materia por área y año**, con el **nombre completo del área y el año**
  («Castellano 1.º», «Geografía, Historia y Soberanía Nacional 4.º»).
- **SI dos planes incluyen la misma área en el mismo año, ENTONCES EL SISTEMA usará una sola materia**
  (Matemáticas, Educación Física, Física y Química, compartidas) y guardará **la carga de cada plan** por
  separado.
- **EL SISTEMA tratará «Biología» y «Biología, Ambiente y Tecnología» como áreas distintas** con materias
  distintas.
- **EL SISTEMA no sembrará dos materias con el mismo nombre.**
- **EL SISTEMA sembrará exactamente 6 materias sin carga**, que son la única excepción a la regla de no
  dejar materias fuera de todo plan: «Participación en los Grupos…» de 1.º a 5.º y «Ciencias de la
  Tierra» de 5.º, y lo documentará como excepción de prueba.
- **EL SISTEMA exigirá el tipo general a todas las materias salvo Educación Física**, que exigirá el tipo
  **cancha**.
- **EL SISTEMA rechazará la aplicación si alguna materia exige un tipo de aula sin aulas sembradas**,
  indicando materia y tipo.
- **EL SISTEMA no sembrará laboratorios ni talleres**: el centro no los tiene.

### RF-4. Cursos, secciones, turnos y letras

- **EL SISTEMA sembrará los 24 cursos** con **año, sección, turno y aula de referencia**, según la tabla de
  esta spec: **12 en la mañana** (31059) y **12 en la tarde** (31059 2.º y 31060).
- **EL SISTEMA declarará el plan de cada curso en el perfil** (para recuentos y clases) y **no lo
  persistirá**: el modelo de cursos no tiene plan y esta spec **no cambia el esquema**; el plan lo aporta
  la configuración al generar. Persistir el plan del curso es capacidad futura (spec 006).
- **EL SISTEMA sembrará el turno en el curso** y no lo dejará vacío. **Sobrescribir el turno del curso
  desde la configuración no existe hoy** y queda como capacidad futura (spec 006).
- **EL SISTEMA usará letras libres para los cursos de 31060** que chocarían con los de 31059, de modo que
  **cada pareja año+sección exista una sola vez**: 1.º → C, D, E; 2.º, 3.º y 4.º → D, E.
- **EL SISTEMA no permitirá** un curso sin turno, sin aula de referencia, con una letra repetida en el
  mismo año o en un turno que supere los 12 cursos.
- **EL SISTEMA documentará** que las letras sembradas **no son las del documento** y que la identidad con
  plan es una spec futura.
- **EL SISTEMA declarará en el perfil la matrícula de cada curso** (entre **32 y 44** alumnos; **44** en
  4.º A y 4.º B de 31059) y sus totales (**889**, con **579** de 31059 y **310** de 31060), y **no** la
  persistirá como dato del curso; la configuración la usará como número de estudiantes de cada curso.

### RF-5. Aulas y cancha

- **EL SISTEMA sembrará 12 aulas de uso general**, con nombre único, **capacidad 44** y ubicación
  **ficticia y opcional**, y **una cancha** de tipo propio y **capacidad 88** (dos grupos completos).
- **EL SISTEMA permitirá que dos cursos de turnos distintos compartan aula de referencia**, y rechazará
  que dos cursos **del mismo turno** la compartan.
- **EL SISTEMA no usará la cancha como aula de referencia** de ningún curso.
- **EL SISTEMA no creará aulas de tipos que el centro no tiene** (laboratorio, taller).
- **EL SISTEMA verificará que la capacidad de cada aula cubre el grupo más numeroso** (44) y que la de la
  cancha cubre dos grupos.
- **EL SISTEMA tratará la capacidad de la cancha como condición necesaria y no como prueba:** **80 franjas
  semanales** (**40 por turno**) sin aforo, **160 franjas-plaza** y **80 plazas por turno** con aforo 2,
  frente a **96** clases de Educación Física (**48 por turno**). Que la cuenta cuadre **no demuestra** que
  exista horario.

### RF-6. Docentes, materias y cobertura

- **EL SISTEMA sembrará 33 docentes:** las **30 posiciones** del centro con **nombres ficticios**, las
  **mismas áreas y los mismos turnos**, **2 docentes añadidos** para las materias sin carga y **1 de apoyo**
  (P033, tarde) para **Idiomas** (corrección del 2026-11-10).
- **EL SISTEMA asignará a cada posición del centro las áreas que el documento le declara**, mediante el
  mapeo de esta spec y la **regla de turno**: la materia se asigna **solo si su área tiene cursos en el
  turno del docente**. «Biología» habilita para las dos áreas biológicas; por la regla, **Biología** va a
  los tres docentes de la mañana y **Biología, Ambiente y Tecnología** **solo a P029** (tarde).
- **EL SISTEMA declarará 272 materias asignadas**, con la derivación por área de esta spec (incluidas las
  **6 materias sin carga** de los dos añadidos y las **18 asignaciones de tarde** de 5.º de 31060, **26** en
  total con las compartidas).
- **EL SISTEMA declarará el tope de horas de aula en 36 horas académicas** para todos los docentes y lo
  informará en el perfil (el **contrato** del centro es 40 h, y 36 h en dos docentes; el tope sembrado es
  36 para todos).
- **EL SISTEMA dará a los 30 docentes del centro su turno** (15 y 15, sin cruzar turnos), a los 2
  añadidos uno por turno y a **P033 la tarde** (16 en la mañana y 17 en la tarde en total). El turno **no se persiste**: es del perfil y se **materializa en la
  disponibilidad**; «15 y 15» se comprueba en el perfil.
- **EL SISTEMA declarará la disponibilidad como la ventana completa del turno** (07:00–12:40 o
  12:40–18:10), **un rango por día de lunes a viernes**.
- **EL SISTEMA garantizará la cobertura a nivel de datos:** cada materia de cada curso sembrado tiene al
  menos un docente habilitado **en su turno** con disponibilidad en las franjas de ese turno.
- **SI un área con cursos en un turno no tiene ningún docente habilitado, ENTONCES EL SISTEMA rechazará
  la aplicación** e identificará área, año y turno.
- **EL SISTEMA no programará las actividades «fuera de aula»** en el horario ni las convertirá en clases.
- **EL SISTEMA no declarará suplentes.**

> **Anotado por la 019 (2026-11-10).** El **tope sembrado de 36 horas académicas** deja de ser un tope
> único y pasa a ser el **valor de referencia editable** de las horas en aula: el usuario puede ajustarlo
> por **convenio** por encima o por debajo y **no se impone como inicio de la tabla** (las dos columnas de
> horas arrancan con las horas sugeridas). El **valor sembrado no cambia** (36 h), el **cálculo no se
> modifica** y el exceso solo se informa.

### RF-7. Turnos, franjas y eventos

- **EL SISTEMA sembrará dos turnos de 8 franjas, 5 días de semana y 40 minutos por franja**, con estas
  ventanas: **mañana 07:00–12:40** y **tarde 12:40–18:10**.
- **EL SISTEMA dejará la duración de la hora académica en 40 min** al aplicar el conjunto: es
  **configuración del centro** (una sola fila; el valor de fábrica es 35), no un dato de dominio. Lo
  informará, lo incluirá en los recuentos y aplicarlo dos veces deja el mismo valor.
- **EL SISTEMA sembrará tres eventos:** **receso** de mañana tras la segunda franja (08:20–08:30), **acto
  cívico** de mañana a continuación (08:30–08:40) y **receso** de tarde (14:00–14:10).
- **EL SISTEMA mantendrá que los eventos no crean ni quitan franjas** pero **definen los segmentos** del
  bloque de dos horas (014).
- **EL SISTEMA documentará la rejilla aritmética:** anclada en el inicio y con pasos de 40 min, termina en
  **12:20** (mañana) y **18:00** (tarde); el **sobrante hasta el fin de la ventana** es de **20 min** en la
  mañana (receso + acto cívico) y **10 min** en la tarde (receso). Leída en reloj, la última franja de la
  mañana caería en **12:00–12:40**.
- **EL SISTEMA alineará la disponibilidad con el reloj:** se declara con la **ventana completa** del turno
  (07:00–12:40 / 12:40–18:10), de modo que cubre las 8 franjas en cualquiera de las dos lecturas.
- **EL SISTEMA rechazará un turno cuyo encaje falle** (`franjas × 40 + eventos ≤ ventana`).
- **EL SISTEMA no declarará el número de días** en los datos: es propiedad de la configuración.

### RF-8. Determinismo, informe y recuentos

- **CUANDO se aplique el conjunto dos veces sobre bases vacías, EL SISTEMA dejará los mismos datos**:
  mismos nombres, recuentos y relaciones, sin azar ni dependencia del reloj.
- **CUANDO termine la aplicación, EL SISTEMA informará los recuentos de la tabla de esta spec** (duración
  de la hora académica, turnos, eventos, tipos de aula leídos de la base, aulas, materias, planes, cargas,
  cursos, docentes, **272** materias asignadas y disponibilidades), **las horas por año de cada plan**,
  **las clases por plan, por año y por turno** y los **cursos por turno**; todo coincidirá con el perfil.
- **EL SISTEMA informará las clases por plan y por año** con estas cifras del centro: **31059 = 516**
  (**68/102/102/108/136** en 1.º–5.º) y **31060 = 360** (**120/80/80/80** en 1.º–4.º), **876** en total;
  y **414/462** clases por turno (mañana/tarde). El **5.º de 31060** (sin cursos) **no se imprime**: la
  serie de 31060 lleva **cuatro** valores.
- **EL SISTEMA informará la capacidad de cada turno** (16 docentes × 36 h) frente a su demanda, y la
  **demanda de Educación Física** frente a las franjas de cancha.
- **SI la aplicación falla, ENTONCES EL SISTEMA terminará con fallo y un mensaje en español** que
  identifique plan, año, curso, área, turno o aula y la acción disponible.
- **EL SISTEMA conservará las marcas de auditoría fuera de la equivalencia** entre aplicaciones.

### RF-9. Verificación por datos y aceptación diferida de la generación

- **EL SISTEMA verificará el conjunto por datos**: recuentos, paridad de horas, unicidad de cursos, aulas
  por turno, cobertura de docentes por turno, encaje de turnos y eventos y **duración de 40 min**.
- **EL SISTEMA dejará escrito que el conjunto no es generable** mientras el modelo no admita dos cursos
  simultáneos en la cancha, con la causa y la acción («aforo simultáneo pendiente»), en el **informe de la
  aplicación** y en las **guías de datos de prueba**. **No es una función de interfaz en esta entrega**: no
  hay pantalla, aviso ni diálogo nuevos, y ninguna guía debe prometer comportamiento de interfaz; el
  informe del conjunto y la CLI son el único artefacto.
- **EL SISTEMA documentará el escenario de aceptación diferido** con sus cifras: **24 cursos**, **876
  clases** (96 de EF; 48 por turno), **12 aulas generales y 1 cancha**, **80 franjas semanales** de cancha
  (**40 por turno**) y **160 franjas-plaza / 80 plazas por turno** con aforo 2, frente a **48** clases de EF
  por turno; **desvío 0** alcanzable, separación y huecos informados, y plazo de **300 s** del proceso de
  cálculo. El aforo se prevé **fijado en 2** (dos cursos con docentes distintos) en su spec.
- **EL SISTEMA no exigirá ni medirá ninguna generación con este conjunto**: **ninguna verificación de esta
  spec genera un horario**. La prueba de generación que hoy usa el conjunto de ejemplo se **reescribe
  contra el escenario sin Educación Física de la 014** (**848 clases**) o se retira; la del conjunto real
  se hará al cerrar el aforo.
- **EL SISTEMA mantendrá los datos del centro como base de ese escenario**, de modo que la spec del aforo
  simultáneo solo añada la capacidad y no rehaga el conjunto.
- **EL SISTEMA no usará la ausencia de generación como excusa para relajar ninguna validación de datos.**

### RF-10. Documentación

- **EL SISTEMA documentará en español:** el contenido del conjunto y sus recuentos; las **dos áreas
  excluidas** y sus **6 materias sin carga**; las **letras de sección** usadas y por qué; las **erratas del
  documento de turnos** que se corrigen; la **matrícula** en el perfil; el **tope de 36 horas académicas**
  y que las actividades «fuera de aula» no se programan; la **EF en la cancha** y el **aforo simultáneo
  pendiente** con sus cifras; y que **el conjunto no genera** todavía.
- **EL SISTEMA actualizará las dos guías vivas de datos de prueba** (la del conjunto y la del frontend) con
  estos recuentos, la duración de 40 min y el estado «no generable».
- **EL SISTEMA documentará la retirada** de las variantes de docentes, de los planes derivados y de la
  regla «aula propia y distinta por curso», y las **supersesiones** de la 008, incluida su **ventana de
  mañana 07:00–12:30**, que pasa a **12:40**.
- **La documentación advertirá** de que los datos personales son **ficticios** y de que el centro real
  sustituye al conjunto de ejemplo; el **documento del centro sigue siendo la fuente versionada**, pero el
  conjunto derivado no contiene nombres reales.

## Requisitos no funcionales

- **RNF-1. Tiempo de aplicación:** aplicar el conjunto sobre una base vacía termina en menos de **15 s** en
  el entorno de referencia del proyecto.
- **RNF-2. Sin generación exigida en esta entrega:** el conjunto **no** se declara generable y no se mide
  un tiempo de generación; el escenario y sus cifras quedan documentados para la spec del aforo
  simultáneo.
- **RNF-3. Sin cambios de esquema, motor ni contrato:** no se toca el esquema (el turno del curso **ya
  existe** y la sección es de un carácter; el plan del curso y el turno del docente **no** se persisten),
  ni el contrato de configuración `2.0`, ni las invariantes, ni las reglas de la 014. La **duración de la
  hora académica** es un valor de configuración, no un cambio de esquema. El aforo simultáneo no se modela
  aquí.
- **RNF-4. Determinismo:** el conjunto no depende del reloj, del orden de inserción ni de la máquina; dos
  aplicaciones son equivalentes salvo las marcas de auditoría.
- **RNF-5. Mensajes en español:** todo rechazo identifica la entidad, el parámetro o la regla incumplida
  con el valor recibido y, cuando exista, la acción disponible.
- **RNF-6. Datos personales ficticios:** ningún nombre, correo ni teléfono real; los 33 docentes usan
  identidades ficticias con la estructura del centro (constitución §5).
- **RNF-7. Coherencia aritmética verificada:** la paridad de las horas, el total de clases (876), la
  demanda por turno (414/462), la capacidad docente (1188), la demanda de EF (96), las **272 materias
  asignadas** (contrastadas con la tabla por área) y la **duración de 40 min** se comprueban de forma
  automática; una cifra que no cuadre es un fallo.
- **RNF-8. Trazabilidad:** cada RF tendrá al menos una comprobación verificable en las tareas.

## Casos límite

- **Dos cursos del mismo año y letra en planes distintos:** se evita con **letras libres**; un choque es un
  fallo de la aplicación (RF-4).
- **Plan con un año sin cursos (31060 5.º):** se declara con sus áreas y horas; no aporta clases ni
  materias usadas por curso (RF-2).
- **Materia del catálogo sin carga (6):** válida y documentada; no entra en las horas ni en las clases, y
  un docente puede tenerla asignada sin dar clase (RF-3, RF-6).
- **Materia con un único docente habilitado en el turno:** **Idiomas** queda con **P033** (36 clases = su
  tope exacto) e **Inglés de 2.º** con **P028** (12). Antes, una sola docente acumulaba **48** clases en la
  tarde (**> 40 franjas**) y el conjunto **no generaba**; la **corrección de datos del 2026-11-10** lo
  resolvió con el docente de apoyo, sin tocar horas (RF-6, RF-8).
- **Materia asignada de un año que no tiene cursos en el turno del docente:** la regla asigna por **área** y
  turno, no por año, así que puede ocurrir (Arte 1.º a un docente de la tarde); es válida, no es
  ejercitable y no rompe la cobertura. Las **18** de 5.º de 31060 son el caso mayor (RF-6).
- **Curso con plan solo en el perfil:** la base no guarda el plan del curso; quien solo lea la base no
  puede derivarlo y lo aporta la configuración (RF-4).
- **Rejilla aritmética frente a la campana:** la última franja de la mañana termina en **12:20** por la
  rejilla, pero leída en reloj caería en **12:00–12:40**; el sobrante de **20 min** en la mañana y **10**
  en la tarde hasta el fin de la ventana corresponde a los eventos y es una consecuencia asumida
  (011/014) (RF-7).
- **Educación Física en la cancha:** 48 clases por turno frente a 40 franjas de cancha por turno; con la
  exclusividad vigente **no cabe** y el conjunto **no genera**; con aforo ×2 sí (48 ≤ 80) (RF-9).
- **12 cursos por turno y 12 aulas generales:** no hay margen; cuando un curso sale a la cancha, su aula de
  referencia queda libre y bloqueada mientras está fuera (010).
- **Curso de 40 h (31060):** llena las 40 franjas del turno, sin huecos ni margen (014).
- **Curso de 36 h (31059 4.º):** 18 bloques y 4 huecos al final del viernes (014).
- **Curso de 34 h (31059 1.º–3.º y 5.º):** 17 bloques y 6 huecos (014).
- **Materia de 3 h (Castellano e Inglés de 5.º):** sus dos restos comparten bloque del mismo tipo; son los
  únicos impares del centro (014).
- **Grupo de 44 alumnos:** cabe en cualquier aula (capacidad 44) y dos grupos caben en la cancha (88)
  (RF-5).
- **Aplicación sobre una base con datos:** falla sin escribir; con reemplazo total exige confirmación y
  respaldo (RF-1).
- **Aplicación interrumpida:** la base queda como estaba (atomicidad) (RF-1).

## Fuera de alcance

- **El aforo simultáneo por aula** (cancha × 2, con docentes distintos): spec **posterior**; esta solo
  documenta las cifras y la dependencia. Las **aulas compartidas entre turnos** **sí** se implementan aquí
  (D-6).
- **La identidad del curso con el plan** (año, sección, plan) y **el plan persistido en el curso**:
  specs propias; aquí se resuelve con letras libres y el plan vive en el perfil.
- **Sobrescribir el turno del curso desde la configuración:** capacidad futura (spec 006); hoy no existe.
- **El paquete de datos del liceo** (instalador y configuración de cálculo): más adelante (D-10). En
  particular, el **paquete de demostración** (`GESTOR_PACK_DEMO` y su carpeta de datos) **no se regenera**
  con el centro real: sigue con el conjunto sintético o se retira en otra spec.
- **Cambiar el esquema, el contrato `2.0` o las reglas del motor** (bloques, separación, huecos, tipos,
  V1–V19).
- **La captura o importación de datos** desde hojas de cálculo u otros sistemas, y la interfaz de usuario.
- **Personas y matrículas individuales:** no se siembran alumnos uno a uno.
- **La generación de horarios con este conjunto** y su rendimiento: diferidos con el aforo (D-5, D-9).
  **Ninguna verificación de esta spec genera** un horario con este conjunto.

## Criterios de finalización

1. Aplicar el conjunto por defecto sobre una base vacía deja **los dos planes reales** con sus áreas y
   horas, **24 cursos con turno y aula**, **12 aulas + 1 cancha**, **33 docentes** con sus materias, **2
   turnos con 3 eventos** y la **duración de la hora académica en 40 min** (RF-1 … RF-7).
2. La aritmética cuadra y se verifica: **34/34/34/36/34** y **40/40/40/40/40** (todas pares);
   **8/8/9/10/10** y **10/10/10/10/10** áreas por año; **876 clases** (96 de EF); **889 alumnos**;
   demanda **414/462** por turno y capacidad docente **1188** (RF-2, RNF-7).
3. El catálogo tiene **85 materias** (**79 con carga**, 16 compartidas entre planes, y **6 sin carga**
   documentadas) y **95 cargas plan-materia**; no hay materias duplicadas (RF-3).
4. Las 24 parejas año+sección son **únicas** gracias a las letras libres de 31060, todas de **un
   carácter**, y ningún curso queda sin turno ni sin aula (RF-4).
5. Ningún curso comparte aula de referencia con otro **del mismo turno**; dos de turnos distintos **sí**
   pueden compartirla; la cancha no es referencia de nadie; las capacidades son 44 y 88 (RF-5).
6. La cobertura por turno se cumple **a nivel de datos** para las 12 áreas de 31059 en la mañana y las 8 de
   2.º más las 10 de 31060 en la tarde; las **272** materias asignadas coinciden con la derivación por área
   (**Biología solo en la mañana**, «Biología, Ambiente y Tecnología» **solo a P029**, **Idiomas a P033** e
   **Inglés de 2.º a P028**) y los 2 docentes añadidos tienen sus **6** materias sin carga (RF-6, RNF-7).
7. Los turnos encajan: mañana **8 × 40 + 20** y tarde **8 × 40 + 10**, con receso en ambos y acto cívico en
   la mañana (RF-7).
8. El informe coincide con el perfil —recuentos, horas por año, **clases por plan (31059 516 · 31060 360)**,
   **clases por año (68/102/102/108/136 y 120/80/80/80)**, clases por turno (414/462) y cursos por turno— y
   la aplicación es determinista, atómica y no destructiva por defecto (RF-8).
9. El conjunto **no se declara generable** y queda escrito en el **informe** y en la **guía** con la causa y
   sus cifras; **ninguna verificación genera** un horario con él y no se mide generación (RF-9, RNF-2).
10. La documentación recoge áreas excluidas, materias sin carga, letras, erratas del documento, matrícula,
    tope de 36 h, EF en la cancha, aforo pendiente y supersesiones de la 008 (incluida la ventana
    07:00–12:30 → 12:40); **las dos guías de datos de prueba quedan actualizadas** y los datos personales
    son ficticios (RF-10, RNF-6).
11. Cada RF tiene al menos una comprobación verificable en las tareas (RNF-8).

## Dudas abiertas

**Ninguna.** Las trece decisiones (D-1 … D-13) se cerraron el **2026-11-07** y están incorporadas a los
requisitos, los datos, los casos límite y los criterios. Las **correcciones del QA (2026-11-07)** son de
redacción y no cambian ninguna decisión: el **plan del curso** y el **turno del docente** **no se
persisten** (viven en el perfil; el turno se materializa en la disponibilidad), la **duración de 40 min**
es **configuración** que el conjunto deja aplicada, el tope sembrado es **36** aunque el contrato del
centro sea 40/36, las **272** asignaciones se derivan de la regla de área **y turno** con su tabla, la
**ventana de la mañana cierra a las 12:40** por los dos eventos (desfase 20/10), y **ninguna verificación
genera** con este conjunto. La **corrección de datos del 2026-11-10** (P033, 33 docentes, 165
disponibilidades, capacidad 1188) está en la nota tras el estado y solo cambia cifras y una posición.
