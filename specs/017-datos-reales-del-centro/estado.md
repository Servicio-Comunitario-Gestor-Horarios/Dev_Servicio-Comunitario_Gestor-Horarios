# Estado 017

Fase: **cerrada** (implementación T1–T10, validación **APROBADA** y cierre el **2026-11-09**)
Aprobaciones: **spec 2026-11-08** · **plan/tareas 2026-11-08** (QA del plan aplicado) · **exceso de
duración de T1/T4/T5 aceptado por el usuario el 2026-11-08** · **implementación 2026-11-09** · **validación
2026-11-09** (CAMBIOS NECESARIOS → 6 hallazgos + 3 residuales aplicados y verificados)
Base: `b5844b580d495397903e5d3bfd8756aa41d98f05` (**sha real de `HEAD`**, verificado con `git rev-parse
HEAD` el 2026-11-08; la `Base` del encargo, `b5844b515a0c6ac9f0e0d4a6b3c1e5f7a9b2c3d4`, **no existe**: solo
coincidía el prefijo corto `b5844b5`).
Dudas abiertas: **ninguna**.
Siguiente paso: **nada pendiente en esta spec.** Después: **aforo simultáneo por aula (cancha ×2, fijado en
2)**, **rendimiento/descomposición** y **decidir los commits** de la 017 (hoy en el árbol, sin commitear).
Veredicto: **APROBADO (2026-11-09)**.

## Decisiones cerradas (2026-11-07)

| # | Resolución |
|---|---|
| D-1 | El **centro real sustituye** al sintético; **se retiran** las 4 variantes de docentes y los planes derivados EXA/NAT/SOC. Un solo conjunto. |
| D-2 | **Una materia por área y año**, **compartida** entre planes si el nombre coincide; «Biología, Ambiente y Tecnología» y «Biología» son distintas; el **5.º de 31060** se declara **sin cursos**. |
| D-3 | Tope de **36 horas académicas (bloques)** para **todos** los docentes (se descuentan las actividades «fuera de aula»). |
| D-4 | Se **excluyen de los planes** «Participación en los Grupos…» y «Ciencias de la Tierra»; sus materias **quedan en el catálogo sin carga** y se **añaden 2 docentes** para ellas. |
| D-5 | **EF en la cancha**; el **aforo simultáneo (2 cursos con docentes distintos)** va **después** del seed. Mientras no exista, **el conjunto no genera** y la aceptación de generación se difiere. |
| D-6 | 12 aulas generales **compartidas entre turnos**, **aula de referencia fija** y **capacidad 44**. |
| D-7 | **40 min**; mañana **07:00–12:40** y tarde **12:40–18:10**; receso en ambos y **acto cívico** en la mañana. |
| D-8 | El **turno se siembra en el curso**; sobrescribirlo desde la configuración **no existe hoy** (futuro, spec 006). |
| D-9 | Ahora se validan **los datos**; la **generación** (876 clases, <300 s, desvío 0, separación informada, huecos al final) se exigirá **al cerrar el aforo**. |
| D-10 | **Solo conjunto de desarrollo**; el paquete del liceo, más adelante. |
| D-11 | Capacidad **44** y matrícula por curso **documentada en el perfil**, sin tocar el modelo. |
| D-12 | Los cursos de 31060 que chocan usan **letras libres** del año (documentado); la **identidad (año, sección, plan)** es **spec propia**. |
| D-13 | **Nombres ficticios** con la misma estructura (30 posiciones, **mismas áreas y turnos**) y **tope de aula 36**. |

## Cifras verificadas (2026-11-07, tras el QA)

- **Horas por año:** 31059 **34/34/34/36/34** (todas **pares**); 31060 **40/40/40/40/40**.
- **Áreas por año:** 31059 **8/8/9/10/10**; 31060 **10/10/10/10/10**.
- **Clases:** **876** en el conjunto sembrado; **96** de EF (**48 por turno**) y **780** sin EF; el plan
  real completo (con las dos áreas excluidas) daría **944**.
- **Demanda por turno:** mañana **414**, tarde **462**. **Capacidad docente:** 32 × 36 = **1152**
  (576 por turno).
- **Catálogo:** **85** materias = **79 con carga** (16 compartidas entre planes) + **6 sin carga**;
  **95** cargas plan-materia.
- **Cursos:** **24** (12 mañana 31059; 12 tarde = 31059 2.º + 31060). Parejas año+sección **únicas** con
  letras libres: 31059 **A, B** (1.º), **A, B, C** (2.º–4.º), **A–D** (5.º); 31060 **C, D, E** (1.º) y
  **D, E** (2.º–4.º).
- **Aulas:** **13** (12 generales de capacidad 44 + 1 cancha de 88). **Tipos:** 2 leídos de la base
  (general + cancha), 1 declarado por el conjunto.
- **Docentes:** **32** (30 del centro + 2 añadidos); **272** materias asignadas; **160** disponibilidades
  (32 × 5); **0** preferencias; **3** eventos.
- **Duración de la hora académica (configuración):** **40 min** (el valor de fábrica es 35).
- **Huecos del centro (futuro):** 12 cursos con **6**, 3 con **4** y 9 con **0** → **84**.

## QA del 2026-11-07: hallazgo → corrección

Las **13 decisiones no cambian**; se corrige su **redacción** para que sea coherente con el modelo real.
Las líneas se refieren a `spec.md`.

| # | Hallazgo | Corrección (línea) |
|---|---|---|
| **H1** | El **plan del curso no se persiste** (`Cursos`: año, sección, turno, aula; `CursoDTO` sin plan). | Nota de persistencia (**51**); definición de curso (**127**); curso en el perfil y no persistido (**218**); RF-4 lo declara y lo deja como futuro (**381**). |
| **H2** | Las **275** asignaciones no se deducían de la regla escrita. | Regla fijada (**238**): área **y turno**, compartidas una sola vez, sin filtrar por año; **BAT solo a P029**; 5.º de 31060 y los 2 añadidos **sí cuentan**. Derivación por área (**271**–**295**), total en datos (**311**, **322**) y en RF-6 (**418**); RNF-7 y criterio 6 lo exigen (**523**, **598**). **Total: 272**. |
| **H3** | Los **40 min** son configuración (fábrica 35); el seed actual fuerza 35. | El conjunto **deja `duracion_minutos = 40`**, lo informa y lo cuenta (**117**, **311**, **439**). |
| **H4** | El **turno del docente no se persiste** (`Profesores` sin turno). | Turno en el perfil, **materializado en la disponibilidad** (**51**, **299**, **423**, **426**); «15 y 15» se comprueba en el perfil. |
| **H5** | D-13 «mismas horas» choca con D-3. | «Mismas **áreas y turnos**; **tope de aula 36**» (**43**, **238**); las 40/36 del documento son **contrato**. |
| **H6** | «No se declara generable» sin artefacto. | Queda escrito en el **informe** y en la **guía** (**473**); HU-2 ajustada (**96**). |
| **H7** | Paquete demo. | **Fuera de alcance**: `GESTOR_PACK_DEMO`/`datos-demo/` **no se regeneran**; siguen con el sintético o se retiran en otra spec (**574**). Alternativa cerrada abajo. |
| **H8** | Cifras de cancha imprecisas. | **80 franjas semanales (40 por turno)**; con aforo **160 franjas-plaza / 80 plazas por turno**; capacidad **necesaria, no prueba** (**405**, **476**–**479**). |
| **H9** | «Aulas compartidas» en fuera de alcance. | Se retiran del fuera de alcance; **D-6 sí las implementa** (**567**). |
| **H10** | «La configuración podrá sobrescribir el turno». | Eliminado: **no existe hoy**, futuro spec 006 (**38**, **384**, **572**). |
| **H11** | «Decisión 0». | **Desvío 0** (**479**). |
| **H12** | Letras de 5.º. | 31059 ocupa **A–D** en 5.º; 31060 **D–E** solo en 2.º–4.º; 5.º de 31060 **sin cursos** (**220**). |
| **H13** | «Área con un solo docente». | **Materia con un único docente habilitado en el turno**: Inglés + Idiomas con **P028** en la tarde, **48** clases y **12** de exceso (**537**). |
| **H14** | Supersesión de la ventana de la 008. | Añadida **07:00–12:30 → 12:40** (**499**). |
| **H15** | Desfase de la rejilla. | Rejilla **aritmética** 07:00–12:20 / 12:40–18:00; **desfase 20 min** (mañana, última franja en reloj 12:00–12:40) y **10 min** (tarde); **disponibilidad en reloj** con la ventana completa (**446**, **450**). |
| **H16** | «Ubicación» del aula. | **Ficticia y opcional** (**133**, **397**). |
| **H17** | `--variante` y `--solo-docentes`. | **Desaparecen**: no hay forma de elegir ni conmutar variante (**336**); los flags, en el plan. |
| **H18** | Aceptación. | **Ninguna verificación genera** con este conjunto; la prueba de generación se **reescribe contra el escenario sin EF de la 014 (848 clases)** o se retira (**481**). |
| **H19** | Erratas de la tabla de turnos. | Documentadas: la fila «4to … 31059 … 9-8» es de **31060**; 1.º de 31060 tiene **3 secciones** (**224**). |
| **H20** | `seed.md` **sí está trackeado** con nombres reales. | Corregido abajo: **se mantiene como fuente versionada** (ya está en el histórico; borrarlo no elimina el dato) y **no se amplía**; el conjunto derivado usa **ficticios**. Retirarlo del histórico exigiría reescribir el repo (fuera de esta spec). |

**Opcionales aplicados.** El aforo se prevé **fijado en 2** en su spec (dos cursos con docentes
distintos) y se nombran las dos guías vivas: `docs/guias/datos-prueba.md` y
`docs/guias/datos-prueba-frontend.md`.

**Alternativa cerrada de H7.** El paquete de demostración **no** se regenera con el centro real: seguirá
con el conjunto sintético (que ya no se mantiene) o se retira. Se decide **retirarlo cuando el sintético
deje de existir**; mientras tanto queda como está y **no** es parte del entregable de la 017.

## Correcciones y hallazgos de la redacción

1. **Ventana de la mañana: 12:40, no 12:30.** El turno tiene **dos** eventos de 10 min (receso y acto
   cívico): `8 × 40 + 10 + 10 = 340` → 07:00 + 5 h 40 = **12:40**. El fixture de la 014 solo tenía el
   receso (`330` → 12:30). Es la única corrección sobre lo decidido y está documentada en la spec.
2. **«Participación en los Grupos» son 60 clases, no 96.** El área existe **solo en 31059** (15 cursos):
   `4 h × 15 = 60`; con Ciencias de la Tierra (`2 h × 4 = 8`) las áreas excluidas suman **68** clases
   (944 − 68 = **876**). Las **96** clases de 4 h son las de **Educación Física** (24 cursos).
3. **`Cursos` ya tiene `turno` y `UNIQUE(anio, seccion)`; `seccion` es de un carácter.** D-8 no exige
   esquema nuevo y D-12 (letras libres) encaja en la restricción vigente; el prefijo «A60» del fixture de
   la 014 **no** sería válido en la base. La identidad con plan es spec futura.
4. **Cargas forzadas por la plantilla real:** **Inglés + Idiomas** en la tarde suman **48** clases y solo
   hay una docente habilitada (**P028**, la única de Inglés+Idiomas en la tarde): **12 h de exceso** blando
   (S1), no infactibilidad. **Biología, Ambiente y Tecnología** tiene un solo docente y **36** clases: tope
   exacto, sin margen. **ITP + EF** de la tarde (54 + 48) se reparten entre tres docentes (108 de
   capacidad): 6 h de margen.
5. **El conjunto no genera** (D-5): la EF de cada turno es **48** clases frente a **40 franjas** de cancha.
   Con aforo ×2: 96 ≤ 160 en la semana y 48 ≤ 80 por turno.
6. **Materias sin carga = 6** (5 de Participación 1.º–5.º + 1 de Ciencias de la Tierra 5.º): es una
   excepción explícita a la regla «catálogo sin huérfanas» de la 008 y debe quedar documentada.

## Riesgos y arrastres

| # | Elemento | Efecto | Tratamiento |
|---|---|---|---|
| 1 | **008 (datos de prueba)** | Recuentos, plan General «adaptado a 40 h», planes derivados, 4 variantes, «aula propia y distinta», cursos **sin turno** y la **ventana 07:00–12:30** quedan **superados**. | Anotar las supersesiones en la 008 al aprobar (RF-1, RF-10); actualizar las **dos** guías. |
| 2 | **Identidad año+sección+plan** | Hoy `UNIQUE(anio, seccion)` obliga a letras artificiales para 31060; el día que se corrija habrá **renombrado de secciones** y presets/horarios guardados afectados. | Spec propia; la 017 solo documenta la letra sembrada. |
| 3 | **Aforo simultáneo (cancha ×2)** | Sin él, el conjunto **no genera**; la aceptación de generación (876 clases, <300 s, desvío 0, separación, huecos) queda **diferida**. | Spec posterior; la 017 deja las cifras y el escenario listos, con el aforo previsto **fijado en 2**. |
| 4 | **`test_generacion_seed` (frontend)** | Hoy **genera** con el seed sintético; con el centro real y EF en la cancha **no puede generar**. | En la fase de tareas: retirarlo o reescribirlo contra el fixture de la 014 (**848 clases**); **no** puede usar el seed real hasta el aforo. |
| 5 | **`test_seed_perfil` / `test_seed_aplicacion`** | Afirman sobre 13 cursos, 57 materias, 16 aulas, 4 variantes y duración 35. | Reescritura completa con los recuentos nuevos (**85 / 95 / 24 / 32 / 272 / 160 / 40 min**). |
| 6 | **CLI del seed** | `--variante` y `--solo-docentes` **desaparecen**; el reemplazo total sigue siendo necesario. | Simplificar la interfaz en el plan (`--db`, `--reemplazar`, `--si`, `--ayuda`, `--version` y los códigos). |
| 7 | **Guías y `GESTOR_PACK_DEMO`** | Las guías describen el centro sintético; el paquete demo incluye `datos-demo/` del Plan 31059. | Actualizar las **dos** guías; el paquete demo **no** se regenera con el centro real (H7). |
| 8 | **Preset maestro** | La app necesita un preset (plan/turno por curso, opciones de docente) para generar; **no** se genera en esta entrega (D-10). | Documentar que el preset se construirá con el aforo. |
| 9 | **Privacidad (H20)** | `seed.md` **sí está trackeado** y contiene **nombres reales** (commiteado en `bf06df6`); ya están en el histórico. | Se **mantiene como fuente versionada** y **no se amplía**; el conjunto derivado usa **ficticios** (D-13). Retirarlo del histórico (reescritura) no forma parte de esta spec. |
| 10 | **Plan del curso no persistido (H1)** | Un consumidor que solo lea la base no puede derivar el plan del curso ni contarlo. | El plan vive en el perfil y en la configuración; la persistencia es spec futura. |

## Plan y tareas (2026-11-08; corregido tras el QA del plan)

`plan.md` con `Base: b5844b5…` (sha real) y `tasks.md` con **10 tareas**. Resumen:

| # | Tarea | RF |
|---|---|---|
| T1 | **Sustitución del conjunto** (perfil + docentes + sembrador) y **sus tres tests**; baja de `test_generacion_seed` | RF-1…RF-8, RNF-4/6/7/8 |
| T2 | Informe «no generable» y `motivoNoGenerable` | RF-8, RF-9 |
| T3 | CLI: ayuda, códigos y binario | RF-1 |
| T4 | `test_entrada_seed` (composición y validación, sin resolver) — cierra el hueco de T1 | RF-9, RNF-2 |
| T5 | `test_seed_aplicacion`: seguridad, determinismo y 15 s | RF-8, RNF-1/4 |
| T6 | Guía `datos-prueba.md` | RF-10 |
| T7 | Guía `datos-prueba-frontend.md` | RF-10 |
| T8 | Supersesión de la 008 (+ parche de `MEMORY.md` entregado) | RF-10 |
| T9 | La interfaz con el centro real (24 cursos, 32 docentes) | RF-4, RF-6, RF-10 |
| T10 | Cierre: suites completas y alcance | RNF-3, RNF-7 |

**Sin estados intermedios:** cada tarea cierra con el build y la suite en verde; la que cambia un dato o un
símbolo actualiza en el mismo paso a sus consumidores y a sus tests. **Único hueco declarado:**
`test_generacion_seed` se retira de `test/CMakeLists.txt` en T1 (no puede generar con el centro real) y su
sustituto `test_entrada_seed` entra en T4.

**Exceso de duración:** T1 (~150 min), T4 (~50) y T5 (~40) superan los 30 min. **El usuario aprobó las 10
tareas con el exceso el 2026-11-08**; el corte 017a (T1–T5) / 017b (T6–T10) quedó como alternativa **no
usada**. La línea base se actualiza a **62** (`backend`) y **67** (`full-release`) según el QA; el número
medido se anota en `journal.md` al abrir T1.


## Implementado (T1–T10, 2026-11-08/09)

Una línea por tarea; la evidencia completa está en `journal.md`.

| Tarea | Qué quedó |
|---|---|
| **T1** | **Centro real sembrado**: 2 planes (31059 34/34/34/36/34 y 31060 40×5), **85 materias** (79 con carga + 6 sin carga), **95 cargas**, **24 cursos con turno** (12 + 12; letras libres en 31060) y **matrícula 889** (579 + 310), **13 aulas** (12 de 44 compartidas entre turnos + **cancha** de 88), **32 docentes** (30 + 2 añadidos), **272 asignaciones**, **160 disponibilidades**, tope **36 h**, **40 min**, **3 eventos** y **876 clases** (96 de EF, 48 por turno). Fuera variantes, planes derivados, conmutación y `--variante`/`--solo-docentes`. |
| **T2** | Informe con `Centro`, `Clases: 876 (EF 96; 48 por turno)`, **`Clases por plan: 31059 516 · 31060 360`**, **`Clases por año: 31059 68/102/102/108/136 · 31060 120/80/80/80`** (el 5.º de 31060, sin cursos, no se imprime), `Matrícula: 889 (579 + 310)`, capacidad 576 / demanda 414–462 y **`Generable: no`** con `motivoNoGenerable` (**48** clases de EF frente a **40** franjas de cancha por turno). **V3 aplicado** (informe y tests). |
| **T3** | **CLI sin variantes**: ayuda con el centro real y aviso de que no genera; `--variante` y `--solo-docentes` → **código 1**; código 6 retirado. |
| **T4** | **`test_entrada_seed`**: compone con `construirEntrada` los 24 cursos (`numEstudiantes = matricula`), valida **V1–V19** y hace ida y vuelta del preset (24 cursos, 32 docentes, 40 min) **sin resolver**. |
| **T5** | `test_seed_aplicacion`: seguridad (falla con datos, confirmación, respaldo), **atomicidad**, **determinismo** (dos bases idénticas), tipo general reutilizado + cancha, informe y **`tiempoDeAplicacion` < 15 s**. |
| **T6** | `docs/guias/datos-prueba.md` reescrita: recuentos reales, uso sin variantes, profesorado del centro y tope 36, seguridad, límites (no generable, EF/cancha, aforo pendiente, paquete demo fuera de alcance) y verificación. |
| **T7** | `docs/guias/datos-prueba-frontend.md` reescrita: 40 min, recuentos, «los cursos **ya traen** turno» (D-8), un solo profesorado y checklist. **Cerrada con deuda declarada (V6)**: la mención al `NoFactible` «en la vista» se retirará cuando el frontend implemente el aforo. |
| **T8** | **Supersesión de la 008**: nota al principio de su `spec.md` (centro real, retirada de variantes y derivados, aula compartida, cursos con turno, ventana 12:30 → 12:40, no generable hasta el aforo) y sección «Superación» + historial en su `estado.md`. |
| **T9** | Interfaz con el centro real: `test_selector_generacion` con **31059** y 24 cursos/32 docentes; `docs/interfaz-frontend.md`, `migracion-frontend.md` y `CMAKE_GUIDE.md` alineados (turno de la base, 40 min, `test_entrada_seed`); `src/frontend/src` sin literales de la 008. |
| **T10** | **Cierre**: suites completas verdes, alcance comprobado (`git diff` sin `database/`, `solver/` ni `frontend/src`) y los tests del conjunto **no resuelven**. |

**Cierre técnico.** `test/frontend/test_generacion_seed.cpp` se **eliminó** del árbol en el cierre
(`git status: D`, sustituido por `test_entrada_seed`); **no** queda como deuda. El único uso legítimo de
`resolverEntradaSolver` fuera del conjunto es `test_ipc_aislamiento_bd` (spec 002/010, ajeno a la 017).

**Mediciones (2026-11-09).**

| Medición | Resultado |
|---|---|
| Aplicación del seed sobre base nueva | **0,089 s** (0,089–0,18 s en las ejecuciones; límite 15 s) |
| `ctest --preset backend` | **61/61** (verificado el 2026-11-09 tras V3) |
| `ctest --preset full-release` | **66/66** (sin `test_generacion_seed`, con `test_entrada_seed`) |
| Semilla (base nueva) | 85 materias · 24 cursos · 32 docentes · 272 asignaciones · 160 disponibilidades · 13 aulas · 3 eventos · 95 cargas · 2 tipos · **40 min** · matrícula **889** |
| Informe del conjunto | `Clases por plan: 31059 516 · 31060 360` y `Clases por año: 31059 68/102/102/108/136 · 31060 120/80/80/80`; `Generable: no` (48 > 40) |

**Línea base:** **61** (`backend`) y **66** (`full-release`), medida el 2026-11-08 (el QA del plan anotaba
62/67; manda el número medido).

## Validación (2026-11-09)

**Veredicto: APROBADO (2026-11-09).** La primera validación devolvió **CAMBIOS NECESARIOS**. Se aplicaron
**6 hallazgos** (V1–V6) y **3 residuales** (el informe con **`31060 120/80/80/80`** —cuatro valores, no un
0—; **T9** con el grep de las **dos guías de datos** y la nota de la 017, **sin `CMAKE_GUIDE`**; y **T2/T7
marcadas**). **Todo verificado:** el binario imprime `Clases por plan: 31059 516 · 31060 360` y `Clases por
año: 31059 68/102/102/108/136 · 31060 120/80/80/80`, las dos guías no tienen literales obsoletos y las
suites están en **61/61** y **66/66**. La tabla queda como registro de los hallazgos.

| # | Hallazgo | Resolución |
|---|---|---|
| **V1** | El estado afirmaba que el archivo del test seguía en el árbol y que su borrado era deuda | Corregido: **se eliminó** en el cierre técnico (`git status: D`, sustituido por `test_entrada_seed`); **no** es deuda. Recogido en «Implementado → Cierre técnico», en el historial y en `plan.md` §3; el journal lo anota |
| **V2** | Nombres reales del documento en las specs | Fuera los **nombres reales** de dos docentes: se sustituyen por **posición/rol** —**P029** («única de BAT en la tarde») y **P028** («única de Inglés+Idiomas en la tarde»)—, sin apellidos, en `spec.md` y en este estado |
| **V3** | El informe no imprime clases por plan ni por año (RF-8 / criterio 8) | **RF-8** y el **criterio 8** exigen las cifras: 31059 **516** (68/102/102/108/136) y 31060 **360** (120/80/80/80); total **876**. **Aplicado**: el binario imprime `Clases por plan: 31059 516 · 31060 360` y `Clases por año: 31059 68/102/102/108/136 · 31060 120/80/80/80`, y `test_seed_aplicacion` las aserta |
| **V4** | El «18» era ambiguo | Precisado: son **18 asignaciones a docentes de la tarde** de materias de 5.º de 31060; el **total de esas 10 materias es 26** (las 4 compartidas con 31059 5.º también se asignan a docentes de la mañana) |
| **V5** | Faltaban archivos en la traza del plan | Añadidos a `plan.md` §3: `docs/interfaz-frontend.md`, `docs/guias/migracion-frontend.md`, `docs/guias/desarrollo/CMAKE_GUIDE.md`, `specs/008-datos-de-prueba/estado.md`, `src/seed/CMakeLists.txt` y el **borrado** de `test_generacion_seed.cpp`; **T9** verifica `docs/interfaz-frontend.md` y `docs/guias/migracion-frontend.md` con los recuentos nuevos y **excluye `CMAKE_GUIDE.md`** del grep de recuentos obsoletos, porque su apartado del **paquete demo** describe el conjunto sintético (fuera de alcance, H7) |
| **V6** | La guía del frontend prometía comportamiento de interfaz | **Informar el «no generable» no es función de interfaz en esta entrega** (RF-9 y `plan.md` §9): se informa en el **informe del conjunto**, la **CLI** y las **guías**, sin pantallas ni avisos nuevos. **Deuda declarada:** la guía aún menciona el `NoFactible` «en la vista»; se retirará cuando el frontend implemente el aforo |
| **V7** | El 5.º de 31060 sin cursos | El informe **no imprime un 0**: `31060` lleva **cuatro** valores (`120/80/80/80`). Corregido en **RF-8**, en el **criterio 8** y en `plan.md` §5.7 |

**Residuales del QA de validación (aplicados y verificados):** (1) informe con `31060 120/80/80/80`;
(2) T9 acotado a las dos guías de datos y a la nota de la 017, con `CMAKE_GUIDE` excluido; (3) T2 y T7
marcadas. Sin pendientes para el veredicto.

## Deudas y notas

- **El conjunto no genera** hasta el **aforo simultáneo** de la cancha: **48** clases de EF por turno frente
  a **40** franjas (con aforo ×2: **96 ≤ 160** y **48 ≤ 80**). La generación y su medición se harán en esa
  spec.
- **Paquete demo:** `GESTOR_PACK_DEMO`/`datos-demo/` **no se regenera** con el centro real; sigue con el
  conjunto sintético o se retira en otra spec.
- **«No generable» no es función de interfaz** en esta entrega: vive en el informe del conjunto, la CLI y
  las guías. La UI (y la mención al `NoFactible` «en la vista» de la guía del frontend) es **deuda del
  aforo**.
- **`seed.md`** sigue versionado con **nombres reales**; el conjunto derivado usa **ficticios**
  (`P001`–`P032`) y no se amplía el documento.
- **Specs futuras:** la **identidad (año, sección, plan)** y el **aforo simultáneo por aula**.
- **`CMAKE_GUIDE.md`** conserva los recuentos del conjunto sintético en su apartado del **paquete demo**
  (por eso queda **excluido** del grep de T9).
- **`sembrador.cpp`** fija en duro «12 Mañana · 12 Tarde» al componer el informe (opcional; no afecta a
  los datos).
- **Medición:** seed en **0,089–0,18 s**; suites **61/61** (`backend`) y **66/66** (`full-release`);
  `test_entrada_seed` sustituye a `test_generacion_seed`, que se **borró** del árbol.

## Historial

- **2026-11-07 — spec redactada (borrador).** Se leen `seed.md`, la 008, la 014 (spec, estado y fixture de
  aceptación), la 015/016 y el código del seed; se contrasta la aritmética y se crean `spec.md` y
  `estado.md` con 13 dudas abiertas (10 del usuario + 3 hallazgos).
- **2026-11-07 — decisiones incorporadas.** El usuario cierra **D-1 … D-13**; se reescribe `spec.md`
  **sin RF condicionales** (10 RF, 8 RNF, datos, casos límite y 11 criterios), se recalculan horas, clases,
  catálogo, cursos, aulas y recuentos, y se deja `Dudas abiertas: ninguna` con **Estado: borrador**.
- **2026-11-07 — QA aplicado (clarificación).** Se aplican **H1–H20**: el plan del curso y el turno del
  docente **no se persisten**; la **duración de 40 min** es configuración y el conjunto la deja aplicada;
  la **regla de asignación** se fija por área **y turno** y el total pasa de 275 a **272** con su
  derivación (BAT solo a P029); la cancha se expresa en **80/160 franjas y 80 plazas**; el «no
  generable» queda en el **informe y las guías**; se cierran el **paquete demo** y la **alternativa del
  turno sobrescrito**; se corrigen «desvío 0», las letras de 5.º, el caso de **P028**, el **desfase
  20/10**, la ventana de la 008 y las **erratas del documento**; **ninguna verificación genera**. Se
  corrige también que `seed.md` **sí está trackeado** con nombres reales. **No** se ha escrito plan ni
  tareas y **no** se ha tocado código. **`MEMORY.md` no se ha podido editar** (permiso denegado): el
  parche se entrega al coordinator.
- **2026-11-08 — spec aprobada; plan y tareas escritos.** El usuario aprueba la spec y `spec.md` pasa a
  **`aprobada (2026-11-08)`**. Se escriben `plan.md` (datos exactos del perfil, funciones puras,
  validaciones, sembrador, CLI, tests, trazabilidad y riesgos) y `tasks.md` (**10 tareas**). **La `Base` del
  encargo no existe**: se usa el **sha real** `b5844b58…`. Decisiones del plan: **plan del curso en el
  perfil** (no persistido); **duración en `Recuentos`** (40 y 1 configuración, comparadas en
  `recuentosCoinciden`); **`motivoNoGenerable`** como artefacto del informe; **`test_generacion_seed` se
  reescribe como `test_entrada_seed` sin resolver** (retirarlo perdería la única cobertura de
  `construirEntrada`; el fixture de la 014 es un `SolverConfig` y no ejercita el constructor desde la base);
  **6 materias sin carga = 5 de Participación + 1 de Ciencias de la Tierra** (el encargo decía «4 de
  Participación»; el documento las declara en los cinco años). **No** se ha tocado código.
- **2026-11-08 — QA del plan aplicado.** Se aplican **B1–B4** y las diez notas menores: **B1** el plan ya no
  declara estados intermedios y **T1 es la unidad atómica** (perfil + docentes + sembrador + sus tres tests;
  baja de `test_generacion_seed` con hueco declarado, cerrado en T4); **B2** `test_selector_generacion` pasa
  de `"NAT"` a `"31059"` en T1 y se revisa con 24/32 en T9; **B3** las posiciones 12/13/15/29 declaran las
  **dos** áreas biológicas con su nombre exacto y la regla compara nombres exactos → **272** y **18** de 5.º;
  **B4** `CursoDef.matricula` (32–44, Σ **889** = 579 + 310), usada como `numEstudiantes` en T4 y documentada
  en T6/T7. Menores: `grep` de T1 sin `kAreas31059`; `--variante` se grepea en T3; binario en
  `build-backend/…`; T6/T7 grepean **recuentos obsoletos**; riesgo 7 alineado con D-8/H10; T10 grepea
  `resolverEntradaSolver`; línea base **62/67**; la excepción D-4 es **§5.4**. Duraciones reales declaradas
  (T1 ~150, T4 ~50, T5 ~40) y **aceptación del exceso** pedida al usuario, con corte alternativo 017a/017b.
- **2026-11-08 — implementación (T1–T5) y T8.** El implementer ejecuta **T1–T5**: sustitución del conjunto
  (85 materias, 95 cargas, 24 cursos con turno y matrícula, 13 aulas + cancha, 32 docentes, **272**
  asignaciones, **160** disponibilidades, **40 min**), informe con **876** clases y «no generable» (48 vs
  40), CLI sin variantes y `test_entrada_seed` (que sustituye a `test_generacion_seed`). Suites **61/61**
  (`backend`) y **66/66** (`full-release`); la semilla tarda **0,089 s**. El planner ejecuta **T8**: nota
  de supersesión al principio de `specs/008-datos-de-prueba/spec.md`, sección «Superación» e historial en
  su `estado.md` y **T8 marcada**. **Pendientes: T6, T7 y T9**; **T10 la cierra el implementer**.
- **2026-11-09 — implementación completa; estado alineado con `tasks.md`.** Las **10 tareas** están `[x]`:
  el conjunto real quedó cerrado (**85 materias**, 95 cargas, **24 cursos**, **32 docentes**, **272
  asignaciones**, **160** disponibilidades, **13 aulas** + cancha, **matrícula 889**, **40 min**, **876
  clases** con **96 de EF**), el informe declara **«no generable»** (**48 > 40** por turno), la **CLI**
  queda sin variantes, **`test_entrada_seed`** sustituye a `test_generacion_seed` (**borrado** del árbol en
  el cierre), las **dos guías** están al día y la **008** marcada como superada.
  Suites **61/61** y **66/66**; seed en **0,089 s**. La fase pasa a **validación**; **la spec no se marca
  `implementada`** hasta el `VEREDICTO: APROBADO` del reviewer.
- **2026-11-09 — hallazgos de la validación aplicados en `specs/` (V1–V6).** **V1**: el estado deja de dar
  por presente el archivo del test (se **borró** en el cierre) y de tratarlo como deuda; **V2**: fuera los
  nombres reales (**P029** y **P028** por posición/rol); **V3**: RF-8 y el criterio 8 exigen las clases por
  plan (**31059 516** · **31060 360**) y por año (68/102/102/108/136 y 120/80/80/80); **V4**: el «18» queda
  como **asignaciones a docentes de la tarde** (26 en total con las compartidas); **V5**: la traza del plan
  recoge los cinco archivos que faltaban y el borrado del test, y **T9** verifica las tres guías; **V6**:
  el «no generable» **no** es función de interfaz en esta entrega. Se registra también la **aceptación del
  exceso de duración por el usuario el 2026-11-08**. Fase **validación**; **no** se marca `implementada`.
- **2026-11-09 — V3 y V6 cerrados; suites verificadas.** El implementer añade al informe **`Clases por
  plan` (31059 516 · 31060 360)** y **`Clases por año` (31059 68/102/102/108/136 · 31060 120/80/80/80)**,
  con sus aserciones en `test_seed_aplicacion`; **T2** queda marcada. **V6** se cierra con **deuda
  declarada** (la guía del frontend aún menciona el `NoFactible` «en la vista») y **T7** marcada. Se
  corrige **V7**: el 5.º de 31060, sin cursos, **no se imprime como 0** (la serie de 31060 lleva cuatro
  valores) en RF-8, el criterio 8 y `plan.md` §5.7. **T9** excluye `CMAKE_GUIDE.md` del grep de recuentos
  obsoletos (su apartado demo describe el conjunto sintético, H7) y verifica solo las dos guías de datos y
  la nota de la 017. Suites **61/61** y **66/66** verificadas. Fase **validación**; siguiente paso,
  **comprobación final y cierre**. **No** se marca `implementada`.
- **2026-11-09 — cerrada (APROBADO).** La validación devolvió **CAMBIOS NECESARIOS**; se aplicaron los **6
  hallazgos** y los **3 residuales** (informe con `31060 120/80/80/80`, T9 sin `CMAKE_GUIDE`, T2/T7
  marcadas) y el coordinator los verificó con las guías limpias y las suites **61/61** y **66/66**.
  `spec.md` pasa a **`implementada (2026-11-09)`** y `Veredicto: APROBADO (2026-11-09)`. El parche de
  `MEMORY.md` se entrega al coordinator. Deudas en «Deudas y notas»; siguiente: **aforo (cancha ×2),
  rendimiento y commits**.
