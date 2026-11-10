# Estado 018

Fase: **cerrada** (implementación T1–T8 + T4b, validación **APROBADA** y cierre el **2026-11-10**; la
enmienda de la constitución está **aplicada** en `docs/constitution.md`)
Aprobaciones: **spec 2026-11-10** · **plan/tareas 2026-11-10** (8 tareas + **T4b** del 2026-11-10) ·
**implementación 2026-11-10** · **validación 2026-11-10** (CAMBIOS NECESARIOS → **H1/H2**, documentales,
aplicados)
Base: `dfe529f3392648fc6ad46de16bbeec26c51d11f6` (`git rev-parse HEAD` el **2026-11-10**; `HEAD` de
`feat/Motor`, «docs(memory): cierre de la 017»)
Dudas abiertas: **ninguna** (D-1 … D-11 cerradas el 2026-11-09)
Siguiente paso: **nada pendiente en esta spec.** Después: **spec de rendimiento/descomposición** (el centro
real mide **161–264 s** y hubo una corrida de **369,7 s**: el margen frente al plazo de **300 s** es
pequeño) y **decidir los commits** de la 018 (hoy en el árbol, sin commitear).
Veredicto: **APROBADO (2026-11-10)**.

## Decisiones cerradas (2026-11-09)

| # | Resolución |
|---|---|
| D-1 | **Regla del sistema por tipo de aula**: `Cancha` → **2**, resto → **1**. **Hardcodeado**: no viaja en el contrato, no toca el esquema, no entra en presets, no es configurable. |
| D-2 | **Solo la cancha**; las 12 aulas generales siguen con una clase por franja. |
| D-3 | Reconocimiento por la **clave normalizada `cancha`**; con materias que exigen cancha y el tipo sin reconocer, **el motor falla con mensaje claro** (nunca aforo 1 en silencio). |
| D-4 | Simultaneidad válida = **cursos distintos con docentes distintos**; sin condiciones añadidas (D4 y D5 ya lo garantizan). |
| D-5 | **Solo pares completos**: dos cursos comparten la cancha solo si su **bloque de 2 h ocupa exactamente las mismas dos franjas**. **Pares cruzados prohibidos**; es requisito del modelo, no de presentación (la alineación la imponen **R2 + R7 + R9** de la 014). |
| D-6 | El **aula de referencia no cambia**: vacía y bloqueada mientras hay desplazamiento, también con dos cursos desplazados a la vez. |
| D-7 | Capacidad del tipo = **aulas × franjas × aforo** (cancha **1 × 80 × 2 = 160** frente a 96); el mensaje de la causa (2) cita **demanda, aulas, franjas y aforo**. |
| D-8 | **Contrato 2.0 intacto**, presets sin cambios y **huella de datos sin cambios**. |
| D-9 | **Datos del seed intactos**; el informe pasa a **«Generable: sí»** con las cifras nuevas y el seed **comprueba que el tipo de la cancha se reconoce**. *(Matizado el 2026-11-10: la **plantilla** recibe **P033** por la corrección de datos de la 017; modelo, contrato y esquema no cambian.)* |
| D-10 | **Test lento** del centro real (etiqueta de lentos): siembra, **compone la entrada desde la base** y **resuelve**: 876 clases, EF en la cancha con aforo 2, desvío 0, huecos al final y **< 300 s**. **Complementa** al fixture sin EF de la 014. |
| D-11 | Enmienda de la constitución **§7.1.3, §7.1.5, §7.3 y §7 «Comportamiento»** (capacidad con aforo y causa (2)); **§7.4 solo si aparece una validación nueva** (no prevista); supersesiones en **005 (D8)**, **010 (D8, capacidad, causa (2), comprobación de salida)**, **014 (cancha compartida exigible)** y **017 (RF-9/RF-10 superadas)**. |

## Cifras verificadas (2026-11-09)

| Concepto | Valor | Origen |
|---|---|---|
| Franjas de la cancha por turno / semana | **40** / **80** (5 días × 8 franjas) | 017 |
| Plazas de la cancha por turno / semana con aforo 2 | **80** / **160** | 40 × 2 / 80 × 2 |
| Pares adyacentes por día / **posiciones de par** por día | **6** pares adyacentes; solo **4 posiciones** alineadas al inicio del segmento (1 + 3) | 014 + 018 |
| Posiciones de par por turno | **20** (4 por día × 5 días) | 014 + 018 |
| Plazas de par por turno con aforo 2 | **40** | 20 × 2 |
| Pares de Educación Física por turno | **24** (12 cursos × 4 h / 2) | 017 |
| Clases de Educación Física | **96** (**48 por turno**) | 017 |
| Holgura | **48 ≤ 80** y **96 ≤ 160** por turno; **24 ≤ 40** pares | 017 + 018 |
| Capacidad física de la cancha | **88 alumnos** (**no** es el aforo) | seed |
| Capacidad del tipo cancha | **1 × 80 × 2 = 160** | D-7 |
| Clases del centro real | **876** (780 sin EF) | 017 |
| Docentes / tope | **33** × **36 h** = **1188** (576 mañana + 612 tarde) | 017 (corrección 2026-11-10) |
| Duración de la hora académica | **40 min** (configuración) | 017 |
| Medición de la 014 (848 clases) | **172,6 s** el centro; **183,58 s** el test lento | 014 |
| Plazo del proceso de cálculo | **300 s** | 002/014 |
| Línea base de la suite (referencia, no requisito) | **61/61** (`backend`) · **66/66** (`full-release`) al 2026-11-09; los recuentos cambian con los tests nuevos | 017 |

## Lo comprobado en el sistema (para el plan)

- **Exclusividad de aula.** Hoy se impone «como mucho una» por (día, franja, aula) con las variables de
  **todos** los cursos, desplazados incluidos: es lo que impide los dos cursos en la cancha. Pasa a
  «como mucho el aforo del aula»; para el aforo 1 el efecto es idéntico.
- **Pares y huecos.** La 014 define `par[c][d][s]` solo si `s+1` está **en el mismo segmento**, con
  `ocupada[s] = par[s] + par[s−1]`; los huecos son un **sufijo** del día. Esa combinación hace que los
  pares de un curso estén **alineados al inicio del segmento** y que un tramo impar no sea solución:
  **los pares cruzados ya son estructuralmente imposibles**. D-5 exige **no asumirlo en silencio**:
  hay que **probarlo** y **comprobarlo en la salida**.
- **Misma aula del par.** La implicación solo se impone con el par activo y se aplica a las dos franjas
  con `y`; por eso un bloque nunca se reparte entre dos aulas y la simultaneidad siempre es por par
  completo.
- **Tipo exacto.** La clase de una materia solo puede ocupar aulas de **su** tipo (EF → cancha). Se
  mantiene; no hay que tocarlo.
- **Referencia.** La referencia es del tipo general; la cancha nunca es referencia; el bloqueo se aplica
  por (curso, día, franja, aula de referencia) y no depende de cuántos cursos haya desplazados.
- **Capacidad y causa (2).** Hoy capacidad = `nº de aulas × franjas de la unión de los turnos`; el
  mensaje cita demanda, aulas y franjas; no descuenta bloqueos; el orden es (1) → (2) → (3) → (4) → (5)
  → «no atribuible», con precedencia de (3) sobre (2) cuando un curso no cabe en su turno.
- **Comprobación de la salida.** Indexa la ocupación por (recurso, día, franja) con un mapa
  «clave → curso»: detecta la segunda clase y la informa. Con aforo 2 sin cambiar, un horario **válido**
  se marcaría como fallo (la misma trampa que la 010 corrigió en P4).
- **Seed.** Identifica la cancha por **igualdad exacta del tipo** (comparación de texto, no de clave) en
  su validación y en su motivo de no generabilidad; su cuenta fija **aforo 1** en duro (48 > 40 por
  turno) y su texto menciona el aforo pendiente; la cancha se siembra con capacidad **88** y tipo
  **`Cancha`**; **renombrar** un tipo que no es el general está permitido hoy (el general es el único
  protegido).
- **Contrato 2.0.** Un aula declara nombre, tipo y capacidad; la capacidad es informativa y la antigua
  comprobación de capacidad de la salida está retirada. **El aforo no viaja en el contrato** (D-1/D-8).
- **Aceptación lenta.** Existe la convención de etiquetar como lento el test de aceptación y dejarlo
  fuera de la suite normal (014).

## Puntos técnicos que el plan debe resolver

1. **Un único punto para el aforo.** Una función pura «aforo del tipo, dada su clave» (cancha → 2, resto
   → 1) que consuman **el modelo, la capacidad, la comprobación de la salida y la guarda del seed**;
   nada de literales dispersos ni de leer el aforo de la configuración.
2. **Exclusividad por aforo.** La restricción de no solapamiento pasa de «como mucho una» a «como mucho
   el aforo»; debe seguir agrupando por (día, franja, aula) con todas las `y`, desplazadas incluidas.
3. **Pares completos (D-5).** **Resuelto en `plan.md` §4**: se demostró que **R2 + R7 + R9** hacen
   imposible el par cruzado (prefijo + emparejamiento par + misma aula del par), se **prueba** con la
   comprobación de modelo (par cruzado forzado infactible) y se **detecta** en la salida; la cuenta es
   **6 pares adyacentes por día pero 4 posiciones alineadas** (1 + 3) → **20 posiciones → 40 plazas** por
   turno frente a 24 pares. Queda declarado que relajar R9 rompería la demostración.
4. **Capacidad y causa (2).** `capacidad(t) = aulas(t) × franjas(t) × aforo(t)`; el mensaje cita las
   cuatro cifras; las **posiciones/plazas de par** van al **informe** (RF-8), no a la causa (RF-6)
   — resuelto así en `plan.md` §3.4/§3.6.
5. **Comprobación de la salida.** Pasa de mapa a **conteo** por (aula, día, franja) y compara con el
   aforo del aula (índice → tipo → clave → aforo), **con aforo 1 por defecto** para índices fuera de
   rango y tipos no reconocidos; **las clases sin aula no cuentan**; mensaje con número de clases y
   aforo; añadir la detección de pares cruzados en la cancha (dos cursos que comparten una sola franja
   de un par).
6. **Guarda del tipo de la cancha.** El seed identifica la cancha por su **entrada declarada y su
   nombre** («Cancha») y **compara el tipo por clave** (`claveTipo`), no por igualdad de texto; si la
   clave no es `cancha` y hay materias que la exigen, rechaza con **tipo recibido + clave esperada +
   acción**. El motor, con materias que exigen cancha y sin aulas de ese tipo, ya falla por la
   validación vigente: el plan debe **verificar el mensaje** y no duplicar la validación.
7. **Seed: de «no» a «sí».** La cuenta del motivo de no generabilidad pasa a usar el aforo del tipo (2)
   → el centro deja de tener motivo; el texto del informe y la ayuda deben dejar de hablar de aforo
   pendiente y citar **48 ≤ 80** y **96 ≤ 160** (y las plazas de par). Tres tests afirman hoy
   «Generable: no» (perfil, aplicación y entrada): se actualizan en la **misma** tarea.
8. **Aceptación lenta.** Debe **sembrar** el centro, **componer la entrada desde la base** (no reusar el
   fixture sintético sin EF) y **resolver**; comprobar 876 clases, EF solo en la cancha, aforo, ausencia
   de pares cruzados, desvío 0, huecos al final y **< 300 s**; etiqueta de lentos; **no** retirar la
   aceptación sin EF de la 014. Ojo al tiempo: la 014 midió 172,6 s con 848 clases y aquí hay 876.
9. **Huella y contrato.** Confirmar con un test que el aforo **no** entra en la huella de datos y que un
   preset vigente se acepta sin cambios.
10. **Documentación.** Contrato (capacidad, causa (2), comprobación de la salida), guías de datos (de
    «no generable» a «generable» con las cifras nuevas), documentación de la interfaz y constitución.

## Plan y tareas (escritos el 2026-11-10)

`plan.md` con `Base: dfe529f…` y `tasks.md` con **8 tareas** (sin cambios de alcance respecto a las
nueve previstas: la comprobación de modelo y la de salida van en T2/T3 y el cierre absorbe la línea base).

| # | Tarea | RF | Estimación |
|---|---|---|---|
| T1 | Aforo en un solo punto (`common/aforo_aula.hpp`), `aforoDelTipo`/`aforoDelAula`/`posicionesDePar`, capacidad con aforo y mensaje de la causa (2) | RF-1, RF-5, RF-6, RNF-3, RNF-8 | ~30 min |
| T2 | Modelo «hasta el aforo» y **comprobación de modelo** de los pares completos (par cruzado forzado infactible) | RF-1…RF-5, RNF-2 | ~45 min (exceso) |
| T3 | **Comprobación de salida**: conteo con aforo (1 por defecto), clases sin aula y pares cruzados | RF-5, RF-7, RF-10, RNF-4 | ~35 min (exceso) |
| T4 | Seed: guarda del tipo por clave, cuenta con aforo 2, informe «Generable: sí» y los tres tests en el mismo paso | RF-2, RF-8, RNF-4 | ~40 min (exceso) |
| T5 | Aceptación lenta del centro real (876 clases, < 300 s, desvío 0, huecos), etiqueta `slow` | RF-9, RNF-1/5/7 | ~50 min (exceso) |
| T6 | Contrato, las dos guías de datos y la documentación de la interfaz | RF-11 (`RNF-8` se verifica en T4) | ~25 min |
| T7 | Constitución §7.1.3/§7.1.5/§7.3/§7 «Comportamiento» y supersesiones 005/010/014/017 — **tarea del planner**; el implementer solo verifica | RF-11 | ~25 min |
| T8 | Cierre: las tres suites, alcance y línea base medida | RNF-2, RNF-6 | ~20 min |

**Exceso declarado:** T2, T3, T4 y T5 superan los 30 min; el plan pide la aceptación del usuario. Corte
alternativo si se prefiere: 018a (T1–T4) y 018b (T5–T8). **T7 no la ejecuta el implementer**
(`docs/constitution.md` y `specs/` son del planner). El **preset valida e ida y vuelta (RNF-8)** se
verifica en **T4** (`test_entrada_seed`), no en T5.

## Riesgos y arrastres

| # | Elemento | Efecto | Tratamiento |
|---|---|---|---|
| 1 | **005 (D8)** | Su «un aula no tiene dos clases a la vez» queda **matizado** | Nota de supersesión; texto nuevo: «hasta el aforo del aula» |
| 2 | **010 (D8, capacidad, causa (2), P4)** | Exclusividad, aritmética de capacidad y detección de dos clases en un aula se leen distinto | Supersesión; contracto y guía de causas actualizados; tipo, referencia, V16–V18 intactos |
| 3 | **014 (cancha compartida)** | Su nota «aforo futuro» pasa a **exigible**; la aceptación sin EF se conserva | Nota de supersesión; el fixture sin EF se mantiene como comparación |
| 4 | **017 (RF-9, RF-10, D-5, D-9)** | Su «no generable» y la aceptación diferida quedan **superadas** | Nota de supersesión; el conjunto pasa a «generable»; datos intactos |
| 5 | **Seed (informe, textos y tests)** | Cuenta con aforo 1 en duro; textos de aforo pendiente; tres tests afirman «Generable: no» | Cambiar cuenta, textos y tests en la **misma** tarea (**T4**) |
| 6 | **Pares completos (D-5)** | Si la alineación de **R2 + R7 + R9** no cubriera algún caso (bordes de segmento, aulas móviles), el par cruzado sería una solución | **Comprobación de modelo** (par cruzado forzado debe ser infactible); si falla, restricción explícita por **posición de par** |
| 7 | **Comprobación de la salida** | Sin cambio, un horario válido se marcaría como fallo (P4 de la 010); un índice de aula raro o un tipo no reconocido no deben abrir un hueco | Conteo contra el aforo, **aforo 1 por defecto**, clases sin aula fuera de la cuenta, y detección de pares cruzados |
| 8 | **Rendimiento** | El centro pasa de 848 a **876** clases con EF en la cancha; techo **300 s** | Medir en la aceptación lenta; el aforo no debería endurecer la búsqueda, pero se mide |
| 9 | **Reconocimiento del tipo (D-3)** | Renombrar «Cancha» desactivaría el aforo; el sistema no puede adivinar la intención; el seed compara hoy por texto exacto | Predicado por **clave normalizada** en la cancha declarada (por nombre/rol) + **fallo explícito** con tipo recibido, clave esperada y acción; en datos existentes, aforo 1 con el nombre visible |
| 10 | **Contrato/presets/huella** | Ninguno por D-1/D-8; cualquier desvío futuro (aforo por aula) sería esquema v7 y contrato nuevo | Fuera de alcance; un test confirma huella y preset intactos |
| 11 | **Frontend (otro equipo)** | Su estimación de ocupación y sus mensajes asumen una clase por aula; la guía menciona el aforo en la vista | Solo documentación; sin pantallas nuevas (RF-11) |
| 12 | **Preset maestro del centro** | Sigue sin generarse (deuda de la 017); la aceptación compone la entrada desde la base | Declarado fuera de alcance (D-10) |
| 13 | **Tiempo de la suite y recuentos** | La aceptación lenta añade ~3 min y los **recuentos de tests cambian** (61/61 y 66/66 no son un requisito) | Etiqueta de lentos y fuera de la suite normal; **línea base = todo verde y sin degradación**, tests rápidos nuevos admitidos (RNF-7) |

## Bloqueo de T5 y resolución (2026-11-10)

- **Bloqueo.** Con T1–T4 hechas y el aforo operativo, **T5** (lenta) quedó **bloqueada**: el centro
  sembrado resolvía **`INFEASIBLE`** en ~25 s. La causa **no era el aforo**: en la tarde, la docente de
  idiomas (**P028**) era la única habilitada de **Idiomas** (36 clases de 31060) e **Inglés de 2.º** (12 de
  31059) → **48 clases en un turno de 40 franjas**. Sin EF seguía infactible; la mañana sola era factible y
  la tarde sola no. Chocaba con **RF-9** y con **RNF-5** («datos intactos»), así que no se arregló dentro de
  esta spec.
- **Resolución (decisión del usuario, 2026-11-10).** La **017** recibe una **corrección de datos**: se
  añade el docente de apoyo **P033** (tarde, **Idiomas**) y **P028** pasa a **Inglés**. Recuentos del seed:
  **33 docentes** (16 mañana / 17 tarde), **165 disponibilidades**, **272 asignaciones** (la suma no cambia)
  y capacidad **1188**. La generación no cambia de criterio (876 clases, < 300 s).
- **Efecto en esta spec:** **RNF-5** pasa a 85/24/**33**/13/876 con 272/165; **D-9** («datos del seed
  intactos») se matiza: el **modelo, el contrato y el esquema** no cambian, y la **plantilla** recibe P033
  por la corrección de la 017. **T4b** aplica el alta y actualiza los tres tests del seed; **T5** se ejecuta
  **después**. **T5 sigue sin marcar.**

## Implementado (T1–T8 + T4b, 2026-11-10)

Una línea por tarea; la evidencia está en `journal.md`.

| Tarea | Qué quedó |
|---|---|
| **T1** | **Aforo en un solo punto** (`common/aforo_aula.hpp`: cancha **2**, resto **1**), `aforoDelTipo`/`aforoDelAula`, capacidad **`aulas × franjas × aforo`** y causa (2) con las **cuatro cifras**; `CapacidadCancha` y `posicionesDePar` en el perfil. |
| **T2** | **Modelo** con hasta el **aforo por aula** y **pares completos** (dos cursos en la cancha solo en las mismas dos franjas); D4/D5 intactas. |
| **T3** | **Comprobación de salida** con aforo (P4 por conteo) y **pares cruzados** prohibidos; mensaje con nº de clases y aforo del aula. |
| **T4** | **Seed**: guarda del tipo de la cancha por **clave**, cuenta con **aforo 2** e informe **«Generable: sí»**; tests del seed. |
| **T4b** | **Corrección de datos de la 017**: **P033** (Idiomas) y **P028** (Inglés); **33 docentes**, **165 disponibilidades**, **272 asignaciones**, capacidad **1188** (576 + 612). |
| **T5** | **Aceptación lenta del centro real**: **876** clases, **96** de EF en la cancha, **0 pares cruzados**, desvío **0**, huecos **0**; **161–264 s** (una corrida de 369,7 s). |
| **T6** | **Contrato y guías**: capacidad con aforo, causa (2) con las cuatro cifras, «aforo 2» y «Generable: sí» en las guías y en el contrato 2.0 (sin campos nuevos). |
| **T7** | **Constitución** (§7.1.3, §7.1.5, §7.3 y §7 «Comportamiento»; **§7.4** intacto) y **supersesiones** 005, 010, 014 y 017. |
| **T8** | **Cierre**: suites **62/62** (`backend`), **67/67** (`full-release`) y **2/2** (lento); alcance sin cambios en el esquema ni en la interfaz. |

## Deudas y notas

- **Tiempo del centro real (spec de rendimiento).** 161–264 s medidos y **una corrida de 369,7 s**; el techo
  es **300 s** y el margen es pequeño. La **spec de rendimiento/descomposición** es la siguiente.
- **Notas opcionales del reviewer:** el **caso literal** del par cruzado (dos cursos con el mismo par de
  franjas invertido) no tiene test propio; la **precedencia (3) > (2)** no tiene un test cruzado; el helper
  `capacidadCancha` usa `turnos[0]`; y el **resto impar de cancha** no está cubierto en
  `comprobarParesCancha`.
- **Docs con «32 docentes»** en `CMAKE_GUIDE`/`migracion-frontend`: son **histórico** (paquete demo y
  migración) y no se actualizaron.
- **Sin cambios de modelo, contrato ni esquema**; presets y huella de datos intactos.

## Historial

- **2026-11-09 — spec redactada (borrador) con 11 dudas abiertas.** Se leen la 017 (spec, estado y
  journal), la 014, la 010/005 (exclusividad, tipo, referencia, capacidad y causas), la constitución §7 y
  el código del motor, la comprobación de la salida, el contrato 2.0 y el seed. Se comprueba que la
  exclusividad de aula impide los dos cursos en la cancha, que la capacidad del tipo no cuenta aforo, que
  la comprobación de la salida marcaría fallo un horario válido y que el seed declara «no generable» con
  aforo 1. Se crean `spec.md` y este `estado.md`. Se corrige la referencia a §7.1.2 del encargo (la
  exclusividad vive en §7.1.3 y §7.1.5). **`MEMORY.md` no se pudo editar** (permiso denegado): el parche
  se entrega al coordinator.
- **2026-11-09 — decisiones incorporadas (D-1…D-11).** El usuario cierra las once dudas. Se reescribe
  `spec.md` **sin RF condicionales**: tabla de decisiones cerradas, aforo como **regla del sistema por
  tipo** (cancha 2, resto 1, hardcodeado y fuera de contrato/esquema/presets/huella), **solo pares
  completos** con prohibición expresa de pares cruzados y su cuenta de plazas de par (40 ≥ 24), capacidad
  **aulas × franjas × aforo** con la causa (2) citando las cuatro cifras, comprobación de la salida
  hasta el aforo más la detección de pares cruzados, reconocimiento por clave `cancha` con **fallo
  explícito** (sin aforo 1 en silencio) y guarda en el seed, conjunto **generable** con las cifras
  nuevas, aceptación lenta que **complementa** al fixture sin EF de la 014, y enmienda constitucional con
  supersesiones en 005/010/014/017. `Dudas abiertas: ninguna`; `Estado: borrador`. **No** se ha escrito
  plan ni tareas y **no** se ha tocado código. `MEMORY.md` sigue sin permiso de edición.
- **2026-11-09 — correcciones del QA de la fase 2 (B1–B3 y notas 4–9).** Sin cambiar ninguna de las 11
  decisiones. **B1**: «**posición de par**» pasa a ser el par **alineado al inicio del segmento** (6 pares
  adyacentes por día, **4 posiciones**: 1 + 3) y «**plaza de par**» = posición × aforo; se mantienen 20
  posiciones y 40 plazas por turno frente a 24 pares, y se ajustan RF-5, RF-8 y los criterios 5 y 8.
  **B2**: el seed identifica la cancha declarada por **nombre/rol** y compara el tipo por **clave**
  (la igualdad de texto exacta deja de ser el predicado), con fallo explícito que incluye **tipo
  recibido, clave esperada (`cancha`) y acción**; RF-2 y los criterios 7 y 12 lo recogen. **B3**: el
  criterio 3 se parte en **comprobación de modelo** (par cruzado forzado sin solución) y **comprobación
  de salida** (un horario con par cruzado se marca). **Notas**: la capacidad y la causa (2) se citan en
  **§7 «Comportamiento»** (no en §7.4, que es V1–V19) y D-11 incluye la **017**; RNF-7 redefine «línea
  base» como «todo verde y sin degradación, con tests rápidos nuevos admitidos» y los recuentos de tests
  dejan de ser requisito; RF-5 aclara que la invariante de la 014 es **por materia** (una sola EF por
  curso en el centro); RF-7 fija **aforo 1** para índices fuera de rango o tipos no reconocidos y saca
  de la cuenta las **clases sin aula**; la justificación de D-5 cita **R2 + R7 + R9**. Se actualizan
  `spec.md`, este estado, las cifras, los puntos técnicos del plan, las tareas previstas y los riesgos.
  **No** se ha escrito plan ni tareas y **no** se ha tocado código.
- **2026-11-10 — spec aprobada; plan y tareas escritos.** `spec.md` pasa a **`aprobada (2026-11-10)`**.
  Se escriben `plan.md` (`Base: dfe529f…`, el aforo en un solo punto `common/aforo_aula.hpp` consumido por
  modelo, capacidad, salida y seed; `AddLessOrEqual(Sum, aforo)` por (día, franja, aula); **demostración**
  de que el par cruzado es imposible con **R2 + R7 + R9** y sus dos comprobaciones; capacidad
  `aulas × franjas × aforo`; P4 por conteo con aforo 1 por defecto y pares cruzados; seed con guarda por
  clave e informe «Generable: sí»; aceptación lenta que complementa al fixture sin EF; constitución
  §7.1.3/§7.1.5/§7.3/§7 «Comportamiento» y supersesiones en 005/010/014/017) y `tasks.md` con **8 tareas**
  en orden de dependencia, con los comandos exactos de `backend`, `full-release` y `slow-release`.
  **Exceso declarado en T2, T3, T4 y T5**, pendiente de la aceptación del usuario. **No** se ha tocado
  código.
- **2026-11-10 — QA del plan aplicado (12 hallazgos).** **T7 pasa a ser tarea del planner** (el
  implementer solo verifica; no edita constitución ni specs). **Seed**: el tipo de la cancha se resuelve
  **por clave** también en `tipoEspecializadoDeMateria`/`tipoFinalDeMateria` (devuelve el **nombre
  declarado**, p. ej. «CANCHA»), con el caso en los tests de la guarda; `posicionesDePar` se alimenta con
  un `TurnoConfig`/`EventoConfig` construido en `perfil.cpp`. **Fixture**: los casos nuevos usan un
  `configConCancha()` aparte (no se toca `configBase()`, con 4 aserciones de cancha vacía). **T2**: el
  forzado se hace sobre un modelo construido en el test (`crearVariables` + restricciones + `CpSolver`,
  `Solver::model`/`vars` son privados) o por configuración, y se añade el caso de **un solo docente
  candidato**. **T4** asume el **ida y vuelta del preset (RNF-8)** y la línea base de `test_entrada_seed`;
  **T5** nombra `analizarBloques` (`analisis_bloques.hpp`) para desvío/bloques/huecos y
  `analizarSalidaSolver` para `presentable()`. **T6** enumera las líneas de las dos guías
  (`datos-prueba.md` 10/156/202-204/208/210-211; `datos-prueba-frontend.md` 163-167/170/198-199/219) y su
  «Hecho cuando» exige que ninguna prometa aforo pendiente; la **vista de horario** queda fuera de
  alcance. **T8** limita el control a `src/`, `test/` y `docs/` (specs/018 está sin seguimiento). Se
  retira `aforo(nombreTipo)` del API. Sin cambios en las 11 decisiones ni en la spec; siguen **8 tareas**
  y ninguna cierra en rojo.
- **2026-11-10 — T1–T4 hechas; T5 bloqueada y desbloqueada.** El aforo (T1–T4) queda implementado y el
  informe en «Generable: sí». **T5** (lenta) se bloquea: el centro real es **`INFEASIBLE`** por la
  **plantilla** (una docente de idiomas con **48 clases** en 40 franjas), no por el aforo. El usuario
  decide **añadir el docente de apoyo P033** en la **017** (Idiomas; P028 pasa a Inglés). Se añade **T4b**
  (alta de P033 + tests del seed con **33/165/272**) y **T5 se ejecuta después**. **T5 sigue sin marcar.**
- **2026-11-10 — T7 hecha (planner): constitución y supersesiones.** Notas aditivas en **005**, **010**,
  **014** y **017** (`grep -ln "018"` → los cuatro). La **enmienda de `docs/constitution.md`** (§7.1.3,
  §7.1.5, §7.3 y §7 «Comportamiento», con **§7.4 intacto**) queda **entregada como parche**: el planner no
  tiene permiso de escritura en `docs/`, solo en `specs/`. Pendiente de aplicar antes de T8.
- **2026-11-10 — T8 HECHA: suites, alcance y línea base medidas.** El usuario desbloquea el cierre subiendo
  el **techo del test de la 014** (`test/backend/test_solver_centro_sin_ef.cpp`) de **200 000** a
  **300 000 ms**, alineado con el criterio de la 014 (**< 300 s**, RNF-2): es margen del test, **no** regla
  del motor. La **enmienda de la constitución** la aplica el coordinator (`grep -n "aforo"
  docs/constitution.md` → **10** líneas en §7.1.3, §7.1.5, §7.3 y §7 «Comportamiento»; §7.4 intacto) y las
  **cuatro** notas de supersesión (005/010/014/017) están puestas. Suites: **backend 62/62** (19,16 s),
  **full-release 67/67** (13,79 s) y **slow-release 2/2** (335,48 s). **Línea base medida:** centro real
  **876** clases, EF **96/96** en la cancha, desvío **0**, `bloquesDeUno` **0**, huecos no finales **0**,
  pares cruzados **0**, `presentable()`, **161 463 ms** (rango 161–232 s); sin EF **848** clases, desvío 0,
  separación 2 (referencia 83), huecos 0, **157 884 ms** (rango histórico 149–321 s). Alcance:
  **26** entradas en `git status --short -- src/ test/ docs/` = 24 del plan §2 + `seed/docentes.hpp`
  (P033) + `test_solver_centro_sin_ef.cpp` (techo); sin `src/backend/src/database/` ni `src/frontend/src/`.
  Sin commits.
- **2026-11-10 — cerrada (APROBADO).** La validación devolvió **CAMBIOS NECESARIOS** con **dos hallazgos
  documentales** (H1: la spec 018 seguía diciendo **32 docentes** y «los datos no cambian»; H2: el estado
  decía **32 × 36 = 1152**). **Aplicados**: la spec 018 enmienda sus cinco líneas y **RF-8/RF-9/RNF-5/
  criterio 8** con **P033** (**33 docentes**, **165 disponibilidades**, **272 asignaciones**, capacidad
  **1188**) y el estado pasa a **33 × 36 = 1188**. El motor, los datos y la aceptación **no cambian**
  (62/62, 67/67, lento 2/2; 876 clases, EF 96 en cancha, 0 pares cruzados, desvío 0, huecos 0). `spec.md`
  pasa a **`implementada (2026-11-10)`** y `Veredicto: APROBADO (2026-11-10)`. Siguiente: **rendimiento** y
  **commits**.
