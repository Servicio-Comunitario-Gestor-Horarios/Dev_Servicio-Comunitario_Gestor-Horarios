# Estado 013

Fase: cerrada
Aprobaciones: spec **2026-11-04** · plan/tareas **2026-11-04** (10 tareas; exceso de T1/T5/T6 sobre la guía aceptado por el usuario) · **implementación 2026-11-06** · **validación 2026-11-06**
Base: **`c7b9427`** (HEAD tras committear la **015** y la **016**: esquema **v6**, fila del tipo general, disparadores, comprobación en tres momentos, ayuda estricta, V18, huella y docs) **+ la 013 en el árbol de trabajo** (la implementación de la 013 no se ha commiteado todavía). El `Base` original era `bb1767af343c108b20de2b2b4de7ebe1d7658c2f`; la re-base del 2026-11-05 lo sustituye por `c7b9427`, y **el ancla de RNF-4/criterio 11 es el escenario posterior a la 015/016** (seed sin «Aula de estudio», base **v6** con el tipo general garantizado), no el escenario de `bb1767a`. Dependencias **cerradas e implementadas**: 009-tipos-de-aula-y-eventos, 010-solver-tipos-y-eventos, 011-horas-academicas, 012-preferencias-docente, `009-agrupacion-bloques-materias` (S3) y **015**; esquema vigente **v6**, contrato vigente **2.0**.
Dudas abiertas: ninguna
Siguiente paso: **nada pendiente en esta spec**. Fuera de ella: la **corrección H2** (guía de la API, misma entrega), las **deudas** de «Deudas y notas», la **014** (materias en bloques de dos horas) y la decisión de **commits** de la 013.
Veredicto: **APROBADO (2026-11-06)** por el reviewer contra `Base: c7b9427` + la 013 en el árbol. QA de la spec: CAMBIOS NECESARIOS (2026-11-04) → H1–H17 incorporados; re-QA: **B1–B3 incorporados**; spec aprobada (2026-11-04). QA del plan: CAMBIOS NECESARIOS (2026-11-04) → **P1–P15 incorporados**; plan y tareas aprobados (2026-11-04). Re-base documental sobre la 015/016 (2026-11-05). **Suite 58/58 (`backend`) y 63/63 (`full-release`).**
Notas: ver «Implementado», «Deudas y notas», «Respuesta al QA», «Decisiones cerradas (2026-11-04)», «Plan y tareas (2026-11-04)» y «Riesgos declarados».


## Implementado (2026-11-06)

Diez tareas, una por sesión y con tests en rojo antes del código; evidencia completa en `journal.md`.
**Suite 58/58 (`backend`) y 63/63 (`full-release`).**

- **T1 — contrato y V19.** `PreferenciaSolverConfig{docente, dia, slot[], sentido}` (ya proyectada) en
  `solver_config.hpp`, parser y serialización en `solver_config.cpp` con la sección **omitida si está
  vacía**, **V19** tras V18 (docente → día → franja → sentido; catálogo **V1–V19**) y `guardar` de presets
  que **elimina** la clave (el preset no persiste preferencias).
- **T2 — rejilla y proyección.** `proyeccion_preferencias` con `ordenarTurnos`/`slotsDeFranja`
  **compartidos con la disponibilidad**, `proyectarPreferencias` puro (bloque entero, todos los turnos,
  evento que no corta), filtro de día dominio → generación y motivos **(c) > (a) > (b)**; 17 casos.
- **T3 — construcción.** `EntradaConstruida{json, intervinientes, noAplicadas}` con
  `construirJsonEntrada` de envoltorio, preferencias **siempre de los datos** (el preset se ignora) y
  validación **después**; golden `sinPreferencias_jsonIdenticoAlDeHoy`.
- **T4 — huella y frontend.** `huellaDatos(datos, D, intervinientes)` con bloque `pref:` solo si hay
  intervinientes y orden canónico; `Composicion.huella`/`preferenciasNoAplicadas`,
  `huellaActual(nucleo, opciones)` con `leerOpciones()` y el diagnóstico pasado a `generar`.
- **T5 — dos fases.** Fase 1 = modelo de hoy (referencia) y fase 2 con `z*` y `e*` **fijados por igualdad**;
  objetivo `PESO_PREFERENCIA` (25) con **deduplicación por (docente, día, slot)** y fallback
  `INFEASIBLE`/`UNKNOWN` → referencia, `MODEL_INVALID` propagado; 11 casos.
- **T6 — jerarquía y medición.** Fase **2b** lexicográfica: fija el valor de preferencia de la 2a y
  minimiza S3; escalera 2b → 2a → referencia; benchmark con techo de 60 s asertado.
- **T7 — cumplimiento.** `preferencias_cumplimiento` puro (día de **dominio**, `MotivoPreferencia::Ninguno`,
  **clases incumplidas** `{curso, materia, slot}`, precedencia no aplicada → incumplida → respetada) y
  `ResultadoGeneracion.cumplimiento` con los tres totales; el contrato de salida **no** cambia.
- **T8 — documentación.** `docs/contrato-solver-2.0.md` (diez secciones obligatorias y **dos opcionales**,
  sección `preferencias`, `meta` como objeto tolerado, **V1–V19**) y `docs/constitution.md` **§7.2**
  (S1/S2/preferencia/S3 con su orden de pesos), **§7.4** (V1–V19) y **§7.5**.
- **T9 — supersesiones.** Notas de acotación de la equivalencia en la **005** y la **010**, supersesión del
  RF-5 y el «Fuera de alcance» de la **012**, nota en `009-agrupacion-bloques-materias`, citas de rango
  corregidas y riesgo documental de las dos «009» y del doble uso de «S3».
- **T10 — regresión.** Tres goldens (JSON de la entrada, huella `3c50f3ab…` y horario del solver)
  capturados antes de consumir preferencias, más el escenario del seed (0 preferencias, sin sección) y las
  dos suites en verde.

**Medición (RNF-3):** escenario grande del benchmark, referencia **3937 ms** → con preferencias
**4676 ms** (**+18,8 %**); techo de 60 s asertado y +50 % documentado. `PESO_PREFERENCIA = 25`.

## Deudas y notas

- **Cobertura pendiente** (no bloquea el veredicto): preferencias de **sentidos opuestos solapadas**, un
  **cambio de preset que altere el conjunto de intervinientes**, **turnos con franjas solapadas** y una
  mejora **estricta** de la fase 2b (hoy el test solo exige `≤` en la agrupación).
- **Aristas de RF-2:** un preset **con preferencias inválidas** se **rechaza al cargar** en vez de
  ignorarse (la sección se ignora como fuente, pero se valida si está), y una sección de **tipo
  incorrecto** se tolera como vacía.
- **Arista de RF-8:** en `NoFactible`/`ContratoInvalido` el `cumplimiento` queda **vacío**; el diagnóstico
  de no aplicadas es **argumento opcional** de `generar` (si no se pasa, no se informan las no aplicadas).
- **Deuda pre-existente del motor:** el objetivo solo suma **`isolated`** (no `longGroup`); la fase 2b
  minimiza el S3 vigente para no cambiar la fase 1 ni la referencia.
- **Documentación histórica:** `docs/solver/Informe-Motor-Solver.md` sigue diciendo «V1–V13» (histórico, no
  se toca).
- **Rendimiento medido:** 3937 ms → 4676 ms (**+18,8 %**), con el techo de 60 s asertado (ver arriba).
- **Guía de la API:** se alinea en la **corrección H2** de la misma entrega, no en esta 013.

## Plan y tareas (2026-11-04, corregidos tras el QA del plan)

- **`plan.md`** con `Base: bb1767af343c108b20de2b2b4de7ebe1d7658c2f` **+ la 015 y la 016 en el árbol** (§1, «Re-base»). Diseño destacado:
  - **Contrato**: `PreferenciaSolverConfig{docente, dia, slot[], sentido}` (proyectada) en
    `solver_config.hpp`, parser y serialización en `solver_config.cpp`, **V19** tras V18 con el orden
    docente → día → franja → sentido, catálogo **V1–V19**; `CargadorConfiguracionSolver::guardar` elimina
    la clave para que **el preset no persista preferencias** (B2).
  - **Proyección**: módulo nuevo `proyeccion_preferencias` con la rejilla **compartida** con la
    disponibilidad (`ordenarTurnos`/`slotsDeFranja` salen del constructor), `proyectarPreferencias(...)`
    puro sobre la configuración parcial y `DatosDominio`; reutiliza `docenteCandidatoElegible` y
    `slotsTurno`, así que «participa» e «interviene» son la misma noción que V13. Filtro de día:
    dominio 0=domingo → generación 0=lunes, domingo descartado, sábado solo si `num_dias` lo incluye.
  - **Construcción**: `EntradaConstruida{json, intervinientes, noAplicadas}` con `construirJsonEntrada`
    de envoltorio (las ~30 llamadas actuales no cambian); la sección del preset se ignora; se valida
    **después**.
  - **Objetivo**: **dos fases**. Fase 1 = modelo actual (S1+S2+S3) = **generación de referencia**; fase 2
    con `z*` y `e*` **fijados por igualdad** y objetivo
    `PESO_PREFERENCIA·(evitadas) − PESO_PREFERENCIA·(preferidas) + PESO_AGRUPACION·S3`. La garantía de
    RF-1 se cumple **por construcción**, no por magnitudes. `PESO_PREFERENCIA = 25`.
  - **Huella**: `huellaDatos(datos, D, intervinientes)` con bloque `pref:` solo si hay intervinientes;
    `Composicion::huella` en el frontend por el mismo camino.
  - **Resultado**: `analizarPreferencias(config, horario, noAplicadas)` puro y
    `ResultadoGeneracion::cumplimiento`; el contrato de salida **no** cambia.
- **`tasks.md`** (aprobadas 2026-11-04; **T1–T10 ejecutadas**): **T1–T10** (contrato+V19+preset · rejilla+proyección · construcción · huella · dos fases
  · estado por preferencia · cableado del frontend · contrato+constitución · supersesiones · medición y
  suite). Cabe en el límite de 10.
- **Presupuesto** (RNF-3): fase 1 con 6.0 de tiempo determinista (como hoy) y fase 2 con 2.0; umbral de
  T10: **60 s** totales y **≤ +50 %** sobre la referencia.

## Historial

- **2026-11-04 — apertura de la spec (planner).** `spec.md` en **borrador** con 9 RF, 7 RNF, casos
  límite, criterios y **8 dudas abiertas** (D-1 a D-8). Cubre el **consumo** de las preferencias por la
  generación; el modelo persistido sigue siendo de la 012.
- **2026-11-04 — dudas D-1 a D-8 resueltas por el usuario.** Se reescribe `spec.md` con texto normativo
  cerrado; las dudas quedan en «Ninguna».
- **2026-11-04 — QA de la spec: CAMBIOS NECESARIOS (H1–H17).** Se reescribe `spec.md` incorporando los
  diecisiete hallazgos **sin cambiar ninguna decisión del usuario**: la garantía de no cambiar de docente
  pasa a formularse **sobre el resultado** con jerarquía (H1); la representación de la preferencia en el
  contrato queda en **día + franjas + sentido por índice** y la proyección es de la construcción (H2); se
  define **una sola** noción de «interviene» con motivos (a)–(d) (H3); la huella se condiciona a que exista
  alguna preferencia interviniente (H4); se separan **reloj → franja** y **filtro de día**, con el mapeo
  domingo/lunes (H5); se fija el orden preset → construcción → V19 (H6); la ida y vuelta pasa a ser de la
  configuración **en memoria** y el preset no persiste preferencias (H7); se define el **estado por
  preferencia** con precedencia (H8); se fija el **artefacto** (resultado del backend, sin tocar el
  contrato de salida) (H9); **V19 única** y catálogo **V1–V19**, con las citas obsoletas a corregir (H10);
  RNF-2 se reformula (H11); el solape de sentidos opuestos se declara como **no producible por los datos**
  y, si llega a mano, ambos términos se compensan (H12); se añaden los bordes de turno y el recorrido
  multi-turno (H13); RF-3/RF-6 se redactan en EARS (H14); se cita la spec de S3 y se anota la colisión
  documental de las dos «009» y del doble uso de «S3» (H15); RNF-3 se cuantifica (H16); el recuento se
  ajusta a **10 tareas** (H17). Incorporados además O1, O2, O4, O5, O6 y O7.

- **2026-11-04 — re-QA: tres bloqueantes residuales (B1–B3).** Se corrigen sin tocar las decisiones del
  usuario: **B1** motivos de «no aplicada» **exhaustivos, disjuntos y con precedencia (c) > (a) > (b)**,
  con «docente participante» definido una sola vez; **B2** preset/ida y vuelta/acotación de la equivalencia
  de la 005 y la 010 (guardar un preset omite la sección, cargarla la ignora, la configuración la
  conserva) y «no hay cambio incompatible» en la forma de los presets; **B3** las secciones opcionales son
  **`aula_fija`** y **`preferencias`**, y `meta` es un objeto tolerado del constructor, no una sección.
- **2026-11-04 — spec APROBADA; plan y tareas escritos.** `spec.md` pasa a `aprobada (2026-11-04)`; se escriben `plan.md` (Base `bb1767af343c108b20de2b2b4de7ebe1d7658c2f`) y `tasks.md` (T1–T10), **pendientes de QA y aprobación**.
- **2026-11-04 — plan y tareas APROBADOS; la implementación NO ha comenzado.** El usuario aprueba
  `plan.md` y `tasks.md` tal como quedaron tras el QA: **10 tareas, sin dividir la spec**, con **RF-8 dentro
  de la 013** y aceptando que **T1, T5 y T6** superen la guía de 20–30 min. La fase pasa a
  `implementacion` como estado documental, pero **no se ha escrito código ni marcado ninguna casilla**:
  T1–T10 esperan la indicación del usuario. `journal.md` queda abierto con las diez secciones vacías.
- **2026-11-04 — QA del plan: CAMBIOS NECESARIOS (P1–P15), corregidos.** Se reescriben `plan.md` y `tasks.md`: deduplicación por franja en el objetivo (P1); jerarquía preferencia > S3 **lexicográfica** en la fase 2 (2a optimiza la preferencia, 2b desempata con S3) y test renombrado a `preferenciaGanaALaAgrupacion` (P2); `toJson` omite la sección vacía y el golden del JSON queda válido (P3); **diez secciones obligatorias y dos opcionales** (P4); estado con **día de dominio**, `MotivoPreferencia::Ninguno` y lista de clases incumplidas (P5); fallback de la fase 2 solo para `INFEASIBLE`/`UNKNOWN` (P6); escalera de plazo y umbrales de RNF-3 sin convertirlo en cambio de requisitos (P7); reparto en **10 tareas** con T1/T5/T6 por encima de 30 min y corte propuesto (P8); estado documental corregido y plan **no** aprobado (P9); `leerOpciones()` (P10); clave `std::tuple<int,int,int>` (P11); verificaciones alineadas entre plan y tareas (P12); ida y vuelta de T1 sin comparar `toJson` (P13); exceso calculado en el test (P14); huella con intervinientes explícitos (P15).
- **2026-11-05 — re-base de la 013 sobre la 015/016 (T8 de la 016).** La 013 se implementará **después** de
  la 015 (esquema **v6** y tipo general garantizado y protegido) y de la 016. Se actualizan `Base`
  (`bb1767a` **+ 015/016 en el árbol, sin commits**; si se commitean antes, el `Base` es ese sha), la
  restricción de esquema (**v6**, la 013 no lo cambia) y el **ancla de la no regresión** (RNF-4 y
  criterio 11): pasa al escenario **posterior a la 015/016** —seed sin «Aula de estudio», base v6 con el
  tipo general garantizado—, con nota de supersesión. **No cambian RF, RNF ni criterios de
  comportamiento** y no se toca código. Sigue **sin implementar**: ninguna casilla marcada.
- **2026-11-06 — T1–T10 implementadas, validación APROBADA y spec cerrada.** Las diez tareas se ejecutaron
  con tests en rojo antes del código; `journal.md` recoge la evidencia de cada una. **Veredicto:
  APROBADO (2026-11-06)** contra `Base: c7b9427` + la 013 en el árbol; **58/58** (`backend`) y
  **63/63** (`full-release`). `spec.md` pasa a `implementada (2026-11-06)`. Quedan anotadas las
  **deudas y notas** de la validación, sin bloquear el cierre: cobertura de aristas (sentidos
  opuestos solapados, cambio de preset que altere los intervinientes, turnos solapados, mejora
  estricta de la 2b), aristas de RF-2 y RF-8, `longGroup` fuera del objetivo (deuda del motor),
  el informe histórico en «V1–V13» y la guía de la API, que se alinea en la corrección H2.

## Respuesta al QA (hallazgo → resolución)

| # | Hallazgo | Cómo se resolvió |
|---|---|---|
| H1 | «Nunca cambia de docente» no se sigue de un orden por unidad | **RF-1** fija la garantía **sobre el resultado**: asignación de docente por materia-curso y exceso de horas por docente **idénticos** a la generación de referencia, con prueba comparativa como criterio (criterio 1). **RF-5** añade la jerarquía S1 > S2 > preferencia > S3 y declara que la relación por unidad **no basta** por ser el objetivo aditivo; el riesgo de dominancia en agregado y su coste se declaran y los resuelve y mide el plan |
| H2 | Representación contradictoria (reloj vs franjas) | **RF-2**: la sección lleva **día de la generación + franjas + sentido** por **índice de docente**, nunca rangos de reloj. **RF-3** atribuye la proyección a la **construcción**. **RF-7/V19** valida lo que llega (índice, día, franja, sentido) y **no** rangos de reloj ni coherencia con el turno |
| H3 | «Interviene» vs «no aplicada» y «cursos del docente» | **RF-3**: noción única, **calculada antes de resolver** (candidato elegible + franja del turno de ese curso) y **no circular**; motivos **(a)–(d)** excluyentes y en orden; franja = `(día de la generación, slot absoluto)`. **RF-6** y **RF-8** se apoyan en la misma noción |
| H4 | «La huella cambia en toda generación nueva» era falso sin preferencias | **RF-6** condiciona el cambio a que exista **al menos una preferencia interviniente** y declara las entradas: instantánea de datos + duración + **proyección de las intervinientes**; un cambio de preset que altere ese conjunto **sí** cambia la huella; sin intervinientes, la huella no cambia |
| H5 | El filtro de día no era «idéntico a la disponibilidad» | **RF-3** separa **reloj → franja** (idéntica a la disponibilidad) y **filtro de día** (propio): dominio 0 = domingo … 6 = sábado → generación 0 = lunes; **domingo no aplica nunca**, **sábado solo si la semana declarada lo incluye**; se descarta **antes** de construir (coherente con V12). Se elimina el absoluto «sábado y domingo no intervienen» |
| H6 | Orden entre sobrescritura del preset y V19 | **RF-2**: orden explícito **ignorar sección del preset → construir desde los datos → validar**. **RF-7** declara que V19 solo es alcanzable por **invocación directa del solver** con una configuración hecha a mano |
| H7 | «El preset no las guarda» vs «ida y vuelta reproduce las preferencias» | **RF-2**: la ida y vuelta sin pérdida es la de la **configuración en memoria** (sección incluida); **un preset no persiste preferencias** y al cargarlo se releen de los datos. Se elimina de `estado.md` la nota que decía que al guardar un preset se escribirían las preferencias |
| H8 | Falta la regla de estado por preferencia | **RF-8**: precedencia **no aplicada (a–d) → incumplida → respetada**; evitada incumplida si alguna franja lleva clase; preferida incumplida si intervino y **ninguna** franja alcanzable lleva clase; cubre parcialmente ocupada, docente sin disponibilidad suficiente y docente sin cursos candidatos |
| H9 | No había artefacto observable | **RF-8** fija el **resultado de la generación del backend** (no la UI) con estado por preferencia y cumplimiento agregado; el estado se calcula sobre el horario devuelto y la proyección previa y **no cambia el contrato de salida del solver** |
| H10 | Identificadores y citas sin fijar | **RF-7**: **V19 única**, catálogo final **V1–V19**, cuatro defectos en orden. **RF-9**: corregir citas obsoletas en la constitución §7.4, en `docs/contrato-solver-2.0.md` («V1–V15» y «V1–V18») y en las specs 005 y 010 |
| H11 | RNF-2 se leía como «mismo horario con y sin preferencias» | **RNF-2** reformulado: determinista y reproducible **en ambos casos**; las preferencias **sí** cambian el horario (RF-4); el orden de declaración no influye (canonización) y el desempate entre soluciones equivalentes se defiere como en la 005 |
| H12 | Solape de sentidos opuestos | Declarado **no producible por los datos** (la 012 lo prohíbe al guardar); si llega a mano, V19 no lo rechaza (no comprueba solapes) y **ambos términos se compensan**; en la huella aparecen las dos entradas. Se anota en casos límite y en «Fuera de alcance» |
| H13 | Proyección con dos turnos y bordes | **RF-3**: se recorren **todos los turnos del día**; casos de borde añadidos (rango antes del turno, después del turno, cruce entre turnos, solape de rejillas) |
| H14 | EARS en RF-3/RF-6 | RF-3 y RF-6 reescritos con `CUANDO / MIENTRAS / SI … ENTONCES`; las definiciones puras viven en «Definiciones» |
| H15 | S3 y las dos «009» | **RF-9**: se cita `specs/009-agrupacion-bloques-materias/` como origen de S3, se añade a las supersesiones y se anota como **riesgo documental** la colisión de las dos «009» y el doble uso de «S3» (agrupación vs cobertura de emergencia de la 005), **sin resolverlo** |
| H16 | RNF-3 sin umbral | **RNF-3**: plazo vigente de **60 s** y **≤ 50 %** sobre el tiempo de la generación de referencia en el mismo escenario; el plan mide y documenta |
| H17 | Recuento de tareas | Ajustado a **10 tareas**, justo en el límite (ver «Tamaño previsto»); no se divide aún |
| O1 | Premio como coste | **RF-4** lo redacta como **coste negativo equivalente** |
| O2 | Dos secciones opcionales | **RF-9**: el contrato pasa a tener dos secciones opcionales, **`aula_fija`** (ya existente) y **`preferencias`** (nueva); **`meta` no es una sección**, es un objeto tolerado/extra que añade el constructor (B3 corrige esta fila) |
| O4 | Canonización del orden | **RNF-2**: las preferencias se **canonizan antes** de construir el objetivo; el orden de declaración no altera el resultado |
| O5 | Glosario | «Definiciones» añade franja, hora académica/bloque (y el aviso de que «bloque» nunca significa «dos horas»), agrupación (S3) y proyección |
| O6 | Anclaje de la no regresión | **RNF-4** y criterio 11 anclados al escenario de datos de prueba y su preset vigentes en `bb1767a`; **superado en la referencia por la re-base del 2026-11-05**: el ancla es el escenario posterior a la 015/016 (seed sin «Aula de estudio», base v6 con el tipo general garantizado). El requisito no cambia |
| O7 | `estado.md` | `Fase: clarificacion` y `Base` se fija al abrir el plan |
| **B1** | Motivos de «no aplicada» no exhaustivos (caso mañana/tarde) | Se define **docente participante** (candidato elegible de al menos una materia-curso, independiente de la configuración y del horario) y la lista de motivos queda **exhaustiva, disjunta y con precedencia (c) > (a) > (b)**: **(c)** el docente no participa · **(a)** día fuera de la semana declarada · **(b)** el rango no alcanza ninguna franja del turno de un curso donde el docente es candidato elegible. Se elimina (d) (queda absorbido por (c)) y (b) cubre el caso de preferencia en la tarde para un docente candidato solo en cursos de la mañana. Actualizados **Definiciones**, **RF-3**, **RF-6**, **RF-8**, los casos límite y el criterio 4 |
| **B2** | Preset, ida y vuelta y equivalencia de la 005/010 | **RF-2** fija las tres reglas: **guardar un preset omite la sección**, **cargar un preset la ignora**, y **la serialización de la configuración sí la conserva**, que es donde se cumple la equivalencia sin pérdida. **RF-9** añade la **acotación de la equivalencia de ida y vuelta** de la 005 (definición «Configuración equivalente») y de la 010: para esta sección, la equivalencia es de la configuración, no del preset. En «Fuera de alcance» se sustituye «no hay cambio de forma de los presets» por **«no hay cambio incompatible»**, con el motivo (sección opcional que el preset omite; los presets 2.0 anteriores siguen cargando) |
| **B3** | La sección opcional existente no es «metadatos» | **RF-9** y el criterio 10 nombran las dos secciones opcionales: **`aula_fija`** (ya existente) y **`preferencias`** (nueva); **`meta`** queda descrito como objeto tolerado/extra del constructor, **no** como sección del contrato |

## Decisiones cerradas (2026-11-04)

- **D-1 → Prioridad blanda.** Penalización del objetivo; **no** cambia la factibilidad ni añade causas de
  infactibilidad (RF-4, RF-8).
- **D-2 → Ponderación.** `PESO_PREFERENCIA` **constante del motor**, con
  `PESO_AGRUPACION (10) < PESO_PREFERENCIA < COSTE_SUPLENTE (100)`; la spec fija la relación de orden y el
  rango (20–50), el **valor exacto se decide en el plan**. Se **penaliza lo evitado y se premia lo
  preferido** con el mismo peso y la misma unidad (**por franja**). Orden del objetivo:
  **S1 > S2 > preferencia > S3**, con la garantía estructural exigida **sobre el resultado** (RF-1, RF-5).
- **D-3 → Proyección del rango.** Franja **completa** dentro del rango (regla de la disponibilidad); un
  **evento** no corta el rango (RF-3).
- **D-4 → Contrato.** Sigue en **2.0** con **sección opcional**; sin sección equivale a «sin
  preferencias»; **no** se toca la regla de rechazo de versiones de la 010 ni se rompen los presets 2.0.
  Las preferencias **siempre vienen de los datos**: el preset no las aporta ni las sobrescribe (precedente
  de la duración, 011). La sección viaja **ya proyectada** (día + franjas + sentido, docente por índice)
  (RF-2).
- **D-5 → Huella.** Entran **solo las preferencias intervinientes, ya proyectadas** sobre la semana
  declarada.
  - **Precisión registrada a petición del usuario:** el motor cuenta en **horas académicas = bloques**
    (`bloques(horas) = horas`, sin `ceil` ni división por la duración; enmienda del 2026-10-17,
    constitución §7), así que **no hay conversión horas → bloques**. **Pero** las preferencias se
    persisten como **hora de reloj `HH:mm`** mientras el contrato usa **índices de franja**, de modo que
    **sí hay una proyección reloj → franja**, la misma de la disponibilidad (inicio del turno + duración
    de la hora académica, franja que cabe entera). **No es una conversión de horas a bloques: es poner el
    rango sobre la rejilla existente.** La huella se calcula sobre las franjas proyectadas (RF-6).
- **D-6 → Validación.** Solo **forma y existencia**: índice de docente, día en rango, franja en rango y
  sentido válido (V19). Nada de coherencia con el turno, el plan o los años (RF-7).
- **D-7 → Preferencias inaplicables.** Se **ignoran** y el resultado las expone como **no aplicadas** con
  su motivo **(a)–(d)** (RF-3, RF-8).
- **D-8 → Alcance.** Solo las **clases del propio docente**; **S2 intacto**; la preferencia no participa
  en la elección de docente (RF-1).
- **Constitución §7.2 (requisito de esta spec).** Hoy solo recoge **S1 y S2** y no menciona **S3**; la
  enmienda debe listar **S1, S2, la preferencia horaria del docente y S3** con su orden de pesos
  (`PESO_HORAS > COSTE_SUPLENTE > PESO_PREFERENCIA > PESO_AGRUPACION`). Es **requisito y tarea del plan**;
  **no** se edita la constitución en la fase de spec (RF-9).
- **Orden entre specs (vigente).** Primero **013** y después **014** (materias en bloques de dos horas).
  La 014 no se diseña aquí.
- **Generación parcial (antigua 007): descartada/diferida por decisión del usuario (2026-11-04).** No se
  abre spec, no se implementa por ahora y no se planifica; si se retoma, se abrirá con número nuevo.

## Riesgos declarados

- **Coste de la garantía estructural (H1).** Para que la preferencia no altere S1/S2 en agregado hace
  falta una jerarquía que domine el objetivo (o resolver por etapas), lo que puede encarecer la
  resolución. El plan debe elegir la estrategia y **medirla** contra el umbral de RNF-3 (60 s y +50 %).
- **Valor exacto de `PESO_PREFERENCIA`.** Si queda cerca del mínimo (20), el efecto percibido puede ser
  pequeño frente a S3; el plan lo fija con una prueba de cumplimiento con y sin preferencias.
- **Premio a lo preferido.** Con el mismo peso, mover una clase de una franja evitada a una preferida
  mejora el objetivo en dos unidades; conviene comprobar que el premio no produce horarios peor legibles
  (no puede empeorar S1/S2 por la jerarquía).
- **Datos sin interfaz.** La 012 dejó la pantalla de preferencias como deuda y el seed siembra **0**
  preferencias: hoy solo se declaran por los servicios. El efecto no será visible en la aplicación hasta
  que exista pantalla; hay que probarlo con datos propios y documentarlo.
- **Cambio de la huella.** Cualquier generación con preferencias intervinientes cambia de huella; los
  horarios guardados antes pasan a «datos anteriores» y **no** se migran (consecuencia declarada en RF-6).
- **Preset como espejo.** Aunque el preset no sea fuente de preferencias y su sección se ignore, el
  formato comparte contrato: hay que documentar que su contenido nunca las fija.
- **Colisión documental de las dos «009» y del doble uso de «S3».** Anotada en RF-9 como riesgo; no se
  resuelve en esta spec.
- **Desalineación normativa previa.** §7.2 sin S3 y §7.4 sin V19, más las citas contradictorias del
  documento del contrato: la corrección debe entrar en esta entrega o quedará una segunda verdad.

## Tamaño

**Diez tareas** (`tasks.md`, T1–T10), justo en el límite y en orden de dependencia, **ejecutadas y cerradas**: contrato+V19+preset ·
rejilla compartida y proyección · construcción de la entrada · huella · objetivo en dos fases · estado
por preferencia y resultado · cableado mínimo del frontend · documentación del contrato y de la
constitución · supersesiones, citas y riesgo documental · medición de desempeño, determinismo y suite
completa. **No se dividió** (decisión del usuario, 2026-11-04).

## Notas

- `plan.md` y `tasks.md` están escritos (2026-11-04), aprobados con la spec e **implementados**
  (2026-11-06); `spec.md` queda en `implementada (2026-11-06)`.
- El planner no escribe fuera de `specs/`: el parche de `MEMORY.md` (fase actual, specs activas, 013 en
  013 **implementada y cerrada**, 014 planificada, generación parcial descartada) se entrega al
  coordinator. **Sin permiso de escritura en `MEMORY.md`; no está aplicado.**
