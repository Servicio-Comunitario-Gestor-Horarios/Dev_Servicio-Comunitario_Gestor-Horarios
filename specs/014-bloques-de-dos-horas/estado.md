# Estado 014

Fase: **cerrada** — 014a (T1–T7) y 014b (B1–B7) **implementadas y aprobadas**.
Aprobaciones: **spec 2026-11-06** · **plan y tareas de la 014a 2026-11-06 (implementados; `journal.md` T1–T7)** · **plan y tareas de la 014b 2026-11-06 (implementados: 8 workers, plazo 300 s, huecos al final duros y separación blanda, informe y seed)** · **correcciones de los bloqueantes residuales aplicadas el 2026-11-07** · **veredicto del reviewer: APROBADO (2026-11-07)** · **spec cerrada como `implementada (2026-11-07)`**.
Base: **`bf06df67c32fc3dceb13a4a67226c47eb65004b2`** (HEAD, commit «backup» del usuario). La `spec.md`, este `estado.md` y las **notas de supersesión** en 005/009/010/013 están **en el árbol** respecto de ese commit; el código de la 014a y de la 014b también está **en el árbol** (sin commitear). El reviewer diferencia contra ese sha.
Dudas abiertas: **ninguna**. Las **quince decisiones** (D-1 … D-10 y N-1 … N-7, con D-1/N-1 y D-7/N-2 unificadas) están cerradas; el QA no cambió ninguna, solo su formulación. La **cancha** quedó **confirmada compartida por dos cursos** (160 ≥ 96); el **aforo simultáneo por aula** es una **spec futura**, no una duda de la 014.
Siguiente paso: **014 cerrada**; el trabajo continúa fuera de esta spec con el **seed con los planes reales**, la spec de **aforo simultáneo por aula**, el **rendimiento** y los **commits** del árbol.
Veredicto: **APROBADO (2026-11-07)** — la validación devolvió 4 bloqueantes residuales de coherencia documental, se corrigieron y la re-verificación los dio por cerrados.
Notas: spec **solo del QUÉ y el POR QUÉ**; la 014a y la 014b tienen código en el árbol (no commiteado). Las notas de supersesión de la 005/009/010/013 son **aditivas**. `seed.md` es del usuario y **no se commitea**.

## Implementado

- **014a — motor (T1–T7, `journal.md`):** funciones puras (tramos, pares, desvío, causa (5) y diagnóstico), variables y restricciones duras (emparejamiento de la rejilla, alineación, ausencia de bloques de 1, misma aula), separación y huecos, aula, informe `AnalisisBloques`, validador ampliado (P4) y fixture de aceptación sin EF (848 clases).
- **014b — cambio de requisitos (B1–B7):** 8 workers y presupuestos 60/20/10 (B1), plazo del servicio 300 s (B1b), aceptación con el defecto y medición (B2), constitución/contrato/supersesiones (B3), regresión y goldens (B4), **huecos al final duros y separación blanda** con causa (5) en 11 h (B5b), informe ampliado curso/materia con apariciones y agregados (B6) y seed sintético a horas pares (B7).

## Deudas y notas

- **Cancha:** confirmada para **dos cursos simultáneos** (160 ≥ 96); el **aforo simultáneo por aula** queda como **spec futura** (hoy el motor no admite dos cursos en la misma aula).
- **Determinismo superado:** el motor busca con **8 workers**; la misma entrada produce un horario **válido**, no idéntico. **Plazo del proceso: 300 s** (002).
- **Medición:** **172,6 s** el centro completo (Release, 848 clases, desvío 0, separación 2) y **183,58 s** el test lento (`ctest --preset slow-release`); ambos dentro del plazo de 300 s.
- **Test de aceptación:** usa **30 docentes** (el centro real), no un perfil reducido.
- **Pendiente opcional:** añadir aserciones **por curso** de huecos **6/4** y `bloquesCompartidos == 4` en `test_solver_centro_sin_ef`; hoy se comprueban los agregados y `huecosNoFinal == 0`.
- **`docs/solver/Tareas-Implementacion.md`:** conserva el umbral «<60 s» **histórico**; su actualización es del implementer (no se toca `docs/` desde `specs/`).

## QA de la spec (2026-11-06): CAMBIOS NECESARIOS incorporados

> **Nota (2026-11-06, posterior).** Este QA y las decisiones que recoge son de **antes** del cambio de
> reglas: los **huecos al final** son ahora **duros**, la **separación** volvió a **blanda** con prioridad
> alta y el **viernes** es blando. **7–10 h** se informa y **11 h o más** es **causa (5)**. Las
> referencias a «7–10 h blando», «11 h o más» o «separación dura» de este apartado son **históricas**.

### Bloqueantes

- **B1 — Jerarquía.** Se separan **reglas duras** (siempre obligatorias; un horario que las incumpla no es
  solución) y **orden blando entre soluciones factibles** (**S1 > S2 > separación > huecos >
  preferencia**). D-6 se lee como prioridad del objetivo blando, **no** como permiso para violar una regla
  dura. Reescritos RF-7, RF-8 y las referencias a «nunca viola una restricción dura».
- **B2 — Causa (5).** Se llama **«(5) conflicto de bloques»** (sin «separación»). Tiene **condiciones
  detectables** sobre la entrada, análogas al conteo de capacidad de la (2): **(a)** total semanal impar
  del curso; **(b)** una materia con más apariciones que días (`⌈h/2⌉ > nº_días`; con 5 días,
  h ≥ 11). Se coloca **después de (1)–(4)** y antes de **«no atribuible»**. Un **aula concreta** que no
  sostiene el bloque va a **(2)** o a **«no atribuible»**, nunca a (5). La separación, al ser blanda, no
  puede causar (5).

### Hallazgos

| # | Hallazgo | Cómo se resolvió |
|---|---|---|
| 3 | Separación blanda ≠ factibilidad | Se separan los tres planos: (a) factibilidad dura, (b) satisfacción de la separación (blanda) y (c) factibilidad de la instancia (docentes/aulas/cancha). La construcción L/X/V se presenta como **candidata**, no como prueba; se define **margen**; los criterios 4/6/7 se condicionan al **fixture sin EF**. |
| 4 | «Total par ⇒ desvío 0» es falso | RF-5 fija las condiciones **necesarias y suficientes** del desvío 0 (total par, máximo de apariciones por semana (≤ días) y restos emparejables del mismo tipo). Se corrige el límite: **7–10 h** incumplen la **separación** (blanda); **11 h o más** incumplen las **reglas duras** (causa 5). |
| 5 | Métrica con doble conteo | Un **único desvío** = `Σ máx(0, franjas descubiertas − h mod 2)`. La alineación se expresa como «franjas descubiertas ≤ h mod 2». Un bloque de 1 no es un segundo término: **no es solución**. |
| 6 | Eventos con efecto normativo | Añadida la supersesión expresa en 010 (§7.3/§7.5) y en la 009 («por receso»): los eventos **no crean ni quitan franjas**, pero **definen los segmentos**; llevado a RF-13. |
| 7 | Instancia real y datos | **Resuelto el 2026-11-06:** turnos, docentes y aulas **confirmados** (la fila «4to A-B-C 31059» de la tarde era un error; los 4º de la tarde son 31060 4A/4B, y 31060 1C va en la tarde). La **cancha** se confirmó **compartida por dos cursos** (160 ≥ 96): deja de ser un problema de capacidad; el **aforo simultáneo por aula** es una **spec futura**. El **fixture de aceptación excluye EF** (34/36 h, 848 clases) y los criterios y el benchmark se anclan a ese fixture. |
| 8 | Supersesión completa de 013 y 009 | Enumeradas: la 009 queda **superada entera** (métrica, `PESO_AGRUPACION`, criterios y **AC-8**); de la 013 quedan superados **RF-5**, **criterios 6 y 10**, definiciones de **S3/referencia/peso** y la **prueba de contrato que fija `PESO_AGRUPACION == 10`**. La garantía estructural se entiende **respecto de la nueva referencia**, no como igualdad con los valores de la 013. |
| 9 | Citas de la constitución | Corregidas: causas **(1)–(5)** en §7 **«Comportamiento»** (no §7.4, que es V1–V19); **§7.1.6** (invariante 6 de §7.1) para la forma **dura** (incluido un bloque por materia y día); **§7.2** para la separación (blanda) y los huecos (**duros**; corregido el 2026-11-07). |
| 10 | Aritmética | Corregido: un curso de 38 h tiene **2 huecos**; «20 bloques que ocupan 40 franjas» (no «20 huecos»); **margen** definido (capacidad de bloques − bloques necesarios) y distinguido de la **capacidad libre para otras materias** (no es un hueco). |
| 11 | Tramo de 4 | Precisado: son **dos bloques válidos**, y **no pueden ser dos de la misma materia** el mismo día. |
| 12 | RF-4 | «**Intentará** colocar» para lo blando; la **paridad es dura**; con 4 huecos, los **4 al final del viernes** son mejores que 2+2. |
| 13 | Numeración de días | Una sola convención en el resultado: **día de dominio (0 = domingo … 6 = sábado)**, la de la 013; la generación interna usa 0 = lunes y la conversión forma parte del informe. |
| 14 | Criterio 9 | Dice «respecto de la **nueva** generación de referencia». |
| 15 | RNF-1 | Se limita a **la misma configuración** (mismo orden de materias y días); no se exige canonización (la 013 solo canonizaba preferencias). |
| 16 | Tamaño | Decisión: **dividir** en **014a (motor)** y **014b (coherencia documental y regresión)**; no se recorta el alcance. |

### Opcionales aplicados

- Recuento de decisiones: **quince** (la tabla no duplica N-3 y unifica D-1/N-1 y D-7/N-2).
- `estado.md`: fase = **clarificación**; `Base` = **`bf06df6`**.
- Aclarado «nunca una materia entera más un resto»: **nunca una materia sin resto (horas pares)**.
- Añadida a las supersesiones la nota del **«bloque» de la 005** (1 hora = 1 bloque) y la del **doble uso
  de S3** (agrupación en la 009, cobertura de emergencia en la 005).
- Precisado que «la huella de datos no cambia» es **respecto de la 013**.
- Las cifras 359/76/3 y 166 se conservan **justificadas** como mediciones del conjunto sintético anterior
  (13 cursos), **no del centro real**, y solo como ilustración del estado previo.

## Datos confirmados (2026-11-06)

**24 cursos, 12 por turno:**

| Turno | Plan | Cursos | Nº |
|---|---|---|---|
| **Mañana** | 31059 | 1A, 1B, 3A, 3B, 3C, 4A, 4B, 4C, 5A, 5B, 5C, 5D | **12** |
| **Tarde** | 31059 | 2A, 2B, 2C | 3 |
| **Tarde** | 31060 | 1A, 1B, 1C, 2A, 2B, 3A, 3B, 4A, 4B | 9 |

- La fila «4to A-B-C 31059» de la tarde del documento era un **error**: los 4º de la tarde son **31060 4A y
  4B**; el **31059 4A/B/C está en la mañana** y el **31060 1C** va en la **tarde**.
- **Docentes:** **15 por turno** (30 en total), **sin cruzar turnos**.
- **Aulas:** **12 de uso general por turno**, **compartidas** entre turnos.
- **Cancha:** 1, **admite dos cursos a la vez** (ver abajo).
- **Fixture de aceptación (sin EF):** **34/36 h**, **848 clases**, **17/18 bloques**, margen **3/2 bloques**,
  **6/4 huecos** (ver abajo).
- **889 alumnos.**

## Hechos verificados y análisis (candidato, no prueba)

**Datos del centro (confirmados el 2026-11-06; tabla completa en «Datos confirmados»):**

- **31059**: 1º–3º **38 h** y 4º–5º **40 h**; **31060**: **40 h** en los cinco años.
- **24 cursos** (15 de 31059 y 9 de 31060), **889 alumnos**, **12 por turno**.
- **Solo dos materias impares:** Castellano 3 e Inglés 3, en **5º de 31059**; suman 40 (par).
- Materias de **6 h**: Cs. Naturales y GHC en 1º–2º de 31059; GHC e ITP en 1º–2º de 31060; ITP en 3º–4º de
  31060. **14 de los 24 cursos**.
- **944 clases por semana con Educación Física** (8×38 + 16×40). Turnos de **8 franjas** con un evento tras
  la franja 2 → segmentos de **2 y 6**, ambos pares.

**Fixture de aceptación (sin Educación Física):** EF se **excluye** y se marca **«no evaluada»**. Los 24
cursos pasan a **34 h** (31059 1º–3º) y **36 h** (31059 4º–5º y 31060), con **848 clases** (8×34 + 16×36):
**8 cursos de 34 h** y **16 de 36 h**. Bloques: **17** y **18** sobre una capacidad de 20; **margen** de
**3 bloques (6 h)** y **2 bloques (4 h)**; **huecos** de **6** y **4** por curso. «EF en aula general» queda
como **prueba de sensibilidad opcional**, no como criterio.

**Análisis por curso (fixture sin EF; candidato, sin docentes ni aulas):**

| Curso | Turno | Cursos | h | Materias de 6 h | Bloques | Capacidad | Margen | Días que exige la separación |
|---|---|---|---|---|---|---|---|---|
| 31059 1º | Mañana | 2 | 34 | Cs. Nat. y GHC | 17 | 20 | **3 bloques (6 h)** | las dos de 6 h → L/X/V; 6 huecos (4+2) |
| 31059 2º | Tarde | 3 | 34 | Cs. Nat. y GHC | 17 | 20 | **3 bloques (6 h)** | las dos de 6 h → L/X/V; 6 huecos (4+2) |
| 31059 3º | Mañana | 3 | 34 | — | 17 | 20 | **3 bloques (6 h)** | 6 huecos |
| 31059 4º | Mañana | 3 | 36 | — | 18 | 20 | **2 bloques (4 h)** | 4 huecos |
| 31059 5º | Mañana | 4 | 36 | — (3 h + 3 h) | 18 | 20 | **2 bloques (4 h)** | las dos de 3 h → 2 apariciones; 4 huecos |
| 31060 1º | Tarde | 3 | 36 | GHC e ITP | 18 | 20 | **2 bloques (4 h)** | las dos de 6 h → L/X/V; 4 huecos |
| 31060 2º | Tarde | 2 | 36 | GHC e ITP | 18 | 20 | **2 bloques (4 h)** | las dos de 6 h → L/X/V; 4 huecos |
| 31060 3º | Tarde | 2 | 36 | ITP | 18 | 20 | **2 bloques (4 h)** | ITP → L/X/V; 4 huecos |
| 31060 4º | Tarde | 2 | 36 | ITP | 18 | 20 | **2 bloques (4 h)** | ITP → L/X/V; 4 huecos |

**Conclusiones (candidatas):**

1. Los 24 cursos del fixture admiten un reparto con desvío 0 (17 o 18 bloques en 40 franjas). **Candidato
   construido a mano**; 8 cursos con **3 bloques (6 h)** de margen y 16 con **2 bloques (4 h)**.
2. La separación se cumpliría en los 24 cursos: las materias de 6 h en **lunes/miércoles/viernes**; las de
   3 h (5º) con 2 apariciones; los dos restos de 5º compartiendo bloque.
3. Los martes y jueves son los días de holgura de los cursos con materias de 6 h (capacidad libre para
   otras materias: 4 h en L/X/V y 8 h en M/J con dos materias de 6 h; 6 h en L/X/V con una). En el fixture
   hay margen de 2 o 3 bloques, no 0.
4. **Huecos:** 34 h → **6** (4 al final del viernes y 2 al final de otro día si hay dos materias de 6 h;
   sin ellas, 6 al final del viernes); 36 h → **4**, al final del viernes.
5. **Un total semanal impar no admite solución** (causa 5). En el fixture, todos los cursos son pares.
6. Con la separación **blanda**, una materia de **7–10 h** se informa como incumplimiento y **11 h o más**
   no es solución (causa 5). No hay ninguna en los planes.

**Conjunto sintético de pruebas (13 cursos):** sigue siendo **factible** con las reglas duras (40 h llenas,
todos pares), pero **no puede cumplir la separación** (cuatro materias de 5 h en 1º–3º). Con N-5 blanda
**genera con incumplimientos informados**. El **escenario de verificación** pasa al **fixture sin EF**;
sustituir el seed de la app es otra spec.

## Cancha y Educación Física (confirmado el 2026-11-06)

> **Confirmado: la cancha puede usarse por dos cursos a la vez.** Capacidad `5 días × 16 franjas × 2 = 160`
> frente a **96 clases** de EF (4 h × 24 cursos). **Deja de ser un problema de capacidad**: el centro es
> factible por ese lado y **no hay causa (2)** por la cancha.

**Consecuencia de modelado (fuera de la 014):**

- El solver actual **no admite dos cursos a la vez en la misma aula** (D8/exclusividad de aula, 010).
  Sostener la cancha compartida exige una **capacidad de aforo simultáneo por aula**, que es un **requisito
  nuevo**: se anota como **spec futura «aforo simultáneo por aula / aulas compartidas»**, con la
  justificación (cancha × 2, 96 ≤ 160). Mientras no exista, la configuración no puede expresar «dos cursos
  en la cancha a la vez».
- El **fixture de aceptación sigue sin EF** (34/36 h, 848 clases), como está aprobado; **EF en la cancha no
  se usa como criterio**. «EF en aula general» es una **prueba de sensibilidad opcional**.
- Cuando exista el aforo simultáneo, el fixture podrá volver a los **planes completos (944 clases)**.

## Alcance final

1. **Bloque** de la rejilla; simple o compartido (dos restos). **No hay bloques de 1 de clase.**
2. **Alineación dura:** cada materia en bloques enteros salvo su resto; el resto se comparte; un bloque por
   materia y día.
3. **Tope duro:** franjas descubiertas `≤ h mod 2` por materia y `≤ Σ(h mod 2)` por curso; desvío 0.
4. **Separación blanda con prioridad alta:** al menos un día entre apariciones; se informa lo que no se
   logre (no vuelve infactible).
5. **Huecos al final duros:** la ocupación del día es monótona (franjas libres = sufijo); el **viernes** es
   la preferencia blanda.
6. **Orden blando:** S1 > S2 > **separación** > **viernes** > preferencia; las reglas duras (emparejamiento,
   alineación, misma aula y huecos al final) se cumplen siempre; **S3 eliminada**.
7. **Misma aula** en el bloque (simple y compartido).
8. **Contrato `2.0` intacto**; el resultado informa bloques, desvío, tope, **curso por materia**,
   apariciones con franjas, días incumplidos y **agregados por curso**.
9. **Todos** los cursos y materias; **seed sintético a horas pares** (RNF-8).

## Tamaño y división

**Dos entregas, sin recortar alcance:**

- **014a — motor (implementado el 2026-11-06; `journal.md` T1–T7):** funciones puras · restricciones duras ·
  separación y huecos (la 014a los implementó **blandos**; la 014b deja los **huecos al final duros** y la
  separación blanda) · aula · causa (5) y
  diagnóstico · informe · fixture de aceptación sin EF. **9 tareas:** T1, T2, **T3a/T3b**, T4,
  **T5a/T5b**, T6, T7. T3/T5 se parten (45–60 min declarados) para que ninguna unidad mezcle emparejamiento
  con alineación ni separación con huecos.
- **014b — cambio de requisitos y coherencia documental (2026-11-06; implementada el 2026-11-07):** **B1**
  workers 8 y presupuestos
  60/20/10 · **B1b** plazo del servicio **300 s** · **B2** aceptación con el defecto y medición del centro ·
  **B3** constitución/contrato/supersesiones (notas aditivas en 005/009/010/013 en `specs/`; la parte de
  `docs/` la ejecutó el implementer) · **B4** regresión y goldens · **B5** separación y
  huecos al
  final duros (implementada y **superada**) · **B5b** **devolver la separación a blanda** y mantener los
  **huecos al final duros**, causa (5) con 11 h y jerarquía · **B6** **informe ampliado** (curso por
  materia, apariciones con franjas, agregados) · **B7** **seed a horas pares**. **9 tareas** en el lote
  014b de `tasks.md`, **todas ejecutadas**. El
  usuario decide que el
  **determinismo no es esencial** (8 workers; misma entrada ⇒ válido, no idéntico) y que la **separación
  vuelve a ser blanda** con prioridad alta, mientras los **huecos al final siguen duros** (el viernes,
  blando; **11 h o más** es causa (5); **7–10 h** se informa). Además, el **plazo del proceso sube de 60 s
  a 300 s** (002): centro completo **172,6 s** y test lento **183,58 s** (medición final; ver el desempeño).

Ninguno de los dos lotes supera el máximo de 10 tareas.

## Riesgos declarados

- **Margen del fixture.** El fixture sin EF tiene margen **3 bloques (34 h)** y **2 bloques (36 h)**; el
  centro con EF (38/40 h) tenía margen 0/1. En el centro con EF, la disponibilidad del docente y las aulas
  son la única fuente de infactibilidad real; cualquier restricción añadida en el futuro puede romperlo. El
  riesgo de falta de margen se traslada al centro con EF y a la cancha, no al fixture de aceptación.
- **Separación blanda ≠ garantía.** En los planes del documento se cumple, pero si cambia un dato (una
  materia de 7 h, una disponibilidad estrecha) el horario se genera **con incumplimientos**. El informe
  debe ser visible.
- **La reordenación de la 013.** La preferencia pasa de segunda prioridad blanda a última; algunas
  preferencias que hoy se respetan dejarán de respetarse. Es una consecuencia de D-5/D-6.
- **S3 eliminada de raíz.** Cualquier consumidor (validador, informes, tests de la 009, prueba de contrato
  con `PESO_AGRUPACION == 10`) deja de tener sentido; hay que retirarlo, no dejarlo «a cero».
- **Seed sintético.** Sigue siendo factible, pero **no cumple la separación** (cuatro materias de 5 h en
  1º–3º); el benchmark y `test_generacion_seed` deben medirse con el **fixture sin EF** o aceptar
  incumplimientos documentados.
- **Cancha (confirmada, no es riesgo de la 014).** La cancha admite **dos cursos a la vez**: 160 franjas
  frente a 96 clases de EF; **deja de ser un problema de capacidad**. El **aforo simultáneo por aula** que
  haría falta para explotarlo es una **spec futura** (el motor actual no lo soporta). El **fixture de
  aceptación sigue sin EF** y no usa la cancha.
- **Turnos (confirmados).** La contradicción del documento (31059 4º A/B/C en Mañana y Tarde; 31060 1º C sin
  turno) era un **error**: la tabla confirmada está en «Datos confirmados». **No hay riesgo abierto.**
- **Desempeño.** Las reglas duras estrechan el problema y la separación y los huecos añaden objetivos. La
  medición de T7 (journal): con **1 worker**, 60 s det ⇒ **UNKNOWN** (286,5 s); con **8 workers**, 60 s det
  (fase 1) ⇒ **FEASIBLE en 24,3 s** (848 clases, desvío 0). La 014b fija 8 workers y 60/20/10; el **centro
  completo** tarda **172,6 s** (Release, 848 clases, desvío 0, separación 2) y el **test lento 183,58 s**
  (`ctest --preset slow-release`); el **plazo del proceso sube a 300 s** (002). Con el plazo de 60 s la
  separación empeoraba de **2 a 90**.
- **Bloque compartido y tipos de aula.** Con la misma aula (N-2), dos restos de tipos distintos no pueden
  compartir; en 5º funciona (Castellano e Inglés son de aula general), pero un plan futuro con restos de
  laboratorio y aula general no podría compartirlos.

## Arrastres fuera de la 014

- **`specs/009-agrupacion-bloques-materias/`:** **superada entera**. Su métrica de S3, su `PESO_AGRUPACION`
  y sus criterios (incluido **AC-8**) desaparecen; se conserva la noción de **segmento contiguo**, ahora
  definida por los **eventos** (no solo por el receso).
- **013:** la **fase 2b** (desempate con S3) **desaparece**; su lugar lo ocupan la separación y los huecos,
  **antes** que la preferencia. Se rehacen la jerarquía de fases, el **RNF-4/criterio 11** y el **golden
  del horario**; quedan superados su **RF-5**, sus **criterios 6 y 10**, sus definiciones de S3/referencia/
  peso y la **prueba de contrato que fija `PESO_AGRUPACION == 10`**. Se mantiene la garantía estructural
  **respecto de la nueva referencia**; la **huella no cambia respecto de la 013**.
- **010:** quinta categoría de causa y su orden **(1)–(5)**; **efecto normativo de los eventos** (no crean
  ni quitan franjas, pero definen segmentos), que supersede §7.3/§7.5.
- **Constitución:** **§7 «Comportamiento»** (causas (1)–(5)), **§7.1.6** (invariante 6 de §7.1: forma dura
  del reparto), **§7.2** (separación blanda, **huecos al final duros**, **S3 eliminada**). **§7.4 (V1–V19)
  no se toca.**
- **005:** se conserva «1 hora académica = 1 bloque = 1 franja»; esta spec usa **«bloque de dos horas»**;
  se anota el **doble uso de S3** (agrupación en la 009, cobertura de emergencia en la 005).
- **Seed de pruebas:** el sintético de 5 h **no cumple la separación** (blando, se informa); el escenario de
  verificación pasa al **fixture sin EF**. Sustituir el seed de la app es otra spec.
- **Benchmark y goldens:** el escenario de 13 cursos deja de ser representativo; hay que medir el **fixture
  sin EF** (24 cursos, **848 clases**, 34/36 h) y rehacer el golden del horario de la 013.
- **Aforo simultáneo por aula (spec futura):** la cancha admite dos cursos a la vez (160 ≥ 96), pero el
  motor actual no lo expresa (D8/exclusividad de aula). El requisito nuevo —capacidad de aforo simultáneo
  por aula / aulas compartidas— se abrirá en su propia spec; entonces el fixture podrá volver a los planes
  completos (944 clases).
- **Frontend:** el contrato no cambia; el informe nuevo es aditivo en el resultado de la generación.

## Historial

- **2026-11-06 — apertura de la spec (planner).** Reparto en pares **por materia**; 10 dudas abiertas.
- **2026-11-06 — corrección de alcance tras leer `seed.md`.** Emparejamiento **de la rejilla del curso**,
  bloque compartido, N-1…N-4; 11 dudas abiertas.
- **2026-11-06 — cierre de decisiones.** El usuario resuelve todas: dura con causa, alineación, **S3
  eliminada**, preferencia al final, misma aula, causa (5), contrato intacto, **huecos al final/viernes**,
  tramos de 4 válidos, separación **blanda**, toda aparición cuenta, solo el curso. Spec sin
  condicionalidad y sin dudas.
- **2026-11-06 — QA de la spec: CAMBIOS NECESARIOS (B1, B2 y 14 hallazgos), incorporados.** Se separan
  reglas duras y orden blando; la causa (5) pasa a «conflicto de bloques» con condiciones detectables y
  relación con (2)/«no atribuible»; se separan los tres planos (dura/blanda/instancia); se corrigen la
  aritmética, la métrica (un solo desvío), la numeración de días (dominio), las citas de la constitución,
  las supersesiones completas de la 013 y la 009, y el tamaño (**división 014a/014b**). `estado.md` pasa a
  **clarificación** con `Base` = **`bf06df6`**.
- **2026-11-06 — datos del centro confirmados.** Turnos (12 y 12 cursos), docentes (15 por turno, sin cruzar)
  y aulas (12 de uso general por turno, compartidas). Se corrige el error del documento («4to A-B-C 31059»
  en la tarde: los 4º de la tarde son 31060 4A/4B; el 31059 4A/B/C está en la mañana; 31060 1C va en la
  tarde). La **cancha** quedó **pendiente ese día** y se **confirmó compartida** el mismo 2026-11-06 (ver el
  cierre al final del historial); el fixture **excluye EF** y los criterios del centro y el benchmark se
  anclan a ese fixture. **Sin dudas abiertas**; la spec queda lista para el **re-QA y la aprobación**.
- **2026-11-06 — bloqueantes residuales B1–B4 corregidos.**
  - **B1:** «nunca empareja una materia de 2 h o más» → **«nunca empareja una materia sin resto (horas
    pares)»** en las tres apariciones; una materia impar de 3 h o más sí tiene resto y puede compartirlo.
  - **B2:** apariciones = **`⌈h/2⌉`** (no `⌈h/2⌉ + h mod 2`); condición de causa (5) **`⌈h/2⌉ > nº_días`**
    (11 h o más con 5 días). «Máximo de apariciones por día» → **por semana (≤ días) / un bloque por
    materia y día**.
  - **B3:** el incumplimiento **blando** de separación es **7–10 h**; **11 h o más** es causa (5)/no
    solución.
  - **B4:** **un solo fixture de aceptación, sin EF** (EF «no evaluada»): **34/36 h**, **848 clases**,
    **17/18 bloques**, margen **3/2 bloques**, **6/4 huecos**; criterios 4–7 y 14 y benchmark re-derivados;
    «EF en aula general» queda como prueba de sensibilidad opcional; la cancha se **confirmó compartida** el
    mismo día (ver el cierre al final del historial).
- **2026-11-06 — spec aprobada; plan y tareas de la 014a escritos.** El usuario aprueba la 014
  (`Estado: aprobada (2026-11-06)`) y pide empezar. Se escriben `plan.md` y `tasks.md` de la **014a
  (motor, T1–T7)** con `Base` = **`bf06df67c32fc3dceb13a4a67226c47eb65004b2`** y la nota de que la spec y
  este estado están **en el árbol** respecto de ese commit. La **014b** (constitución, contrato,
  supersesiones, goldens y benchmark final) queda para su propio plan. Fase: **plan**; QA/aprobación del
  plan pendientes.
- **2026-11-06 — QA del plan 014a: P1–P5 y notas aplicadas.**
  - **P1:** el desvío vuelve a la **fórmula única** `Σ máx(0, descubiertas − h mod 2)`; el bloque de 1 es
    **conflicto del P4**, no término de la métrica (`desvioDeCurso`/`bloquesDeUno` separados).
  - **P2:** **T1** incluye `PresupuestoFases` sin `agrupacion` y **todos** los tests que tocaban S3
    (`test_solver_preferencias`, `test_solver_horarios`, `test_solver_benchmark`, `test_edge_cases`,
    `test_contrato_solver`, `test_validador_salida`, `test_variables_cp_sat`), con grep ampliado; T1 cierra
    en verde.
  - **P3:** el test del bloque compartido se mueve a **T4** (la alineación llega allí); **T3a/T3b** solo
    prueban emparejamiento y misma aula con pares válidos.
  - **P4:** T7 comprueba el **valor informado y su cota** (≤ la referencia), no el óptimo blando; distingue
    34 h con dos materias de 6 h (4+2) de 34 h sin ellas (6), y 36 h (4).
  - **P5:** T7 **no depende de la 014b**: ejecuta `full-release` **excluyendo `test_generacion_seed`**.
  - Notas: T3/T5 partidos en **T3a/T3b** y **T5a/T5b** (**9 tareas**); RNF-1/3/4/5/6 en la trazabilidad;
    tamaño del modelo medido en T3a/T3b/T4 (~30–40k booleanas); rationale de R7 corregido; sin constantes de
    peso en el header (se calculan por instancia).
- **2026-11-06 — cancha confirmada: dos cursos a la vez.** La cancha del liceo admite **dos cursos
  simultáneos**: `5 días × 16 franjas × 2 = 160` frente a **96 clases** de EF → **deja de ser un problema de
  capacidad** (no hay causa (2) por la cancha). Se cierra la cuestión abierta. **Consecuencia de modelado
  fuera de la 014:** el motor actual no admite dos cursos en la misma aula (D8/exclusividad de la 010);
  sostenerlo exige una **capacidad de aforo simultáneo por aula**, que se anota como **spec futura «aforo
  simultáneo por aula / aulas compartidas»** (justificación: cancha × 2, 96 ≤ 160). El **fixture de
  aceptación sigue sin EF** (34/36 h, 848 clases) y EF en cancha no se usa como criterio; «EF en aula
  general» sigue como prueba de sensibilidad opcional. Cuando exista el aforo simultáneo, el fixture podrá
  volver a los **planes completos (944 clases)**.
- **2026-11-06 — cambio de requisitos: el determinismo no es esencial; 8 workers.** El usuario decide que,
  si el motor resuelve con más workers, es mejor así. La **014** cambia su **RNF-1** a **validez + 8
  workers** (misma entrada ⇒ horario válido, **no** necesariamente idéntico); el criterio 16 se ajusta y la
  005 (RNF-1) y la 013 (RNF-2) quedan **superadas en la reproducibilidad del horario** (notas puestas). La
  constitución §7 «Comportamiento» se enmienda en la **014b (B3)**. La **014b** pasa a **4 tareas**
  (B1–B4): `workers = 8` por defecto y presupuestos **60/20/10** (justificados por la medición de T7:
  8 workers, 60 s det ≈ 24,3 s de pared en Release; 1 worker, 60 s det ⇒ UNKNOWN), aceptación con el
  defecto y etiqueta `slow`, medición del centro de **848 clases** y regresión **61/61 + 65/65**.
- **2026-11-06 — plazo del proceso de cálculo: 60 s → 300 s.** El usuario sube el techo de la **002** para
  conservar la calidad con 8 workers: el centro completo tarda **172,6 s** (Release, 848 clases, desvío 0,
  **separación 2**; medición final) y con el plazo de 60 s la separación empeoraba de **2 a 90**. Se actualizan la **002**
  (RF-1 y caso límite) con la nota de supersesión y la **014** (B1b: `ServicioGeneracion` 300 000 ms;
  RNF-2/criterio 14 a 300 s; medición anotada). **Salud y apagado siguen en 5 s.** El
  `docs/interfaz-frontend.md` (plazo 60 s / `60000`) se ajusta en **B3**.
- **2026-11-06 — separación y huecos al final duros; seed a horas pares.** *(Superado el mismo día por la
  entrada siguiente: la separación vuelve a blanda.)* El usuario decide que la
  **separación de un día** (`apareceDia[d] + apareceDia[d+1] ≤ 1`) y los **huecos al final del día**
  (ocupación monótona, `ocupada[s] ≥ ocupada[s+1]`) pasan a **reglas duras**; el **viernes** sigue
  **blando**. Desaparecen `PESO_SEPARACION`, `huecoNoFinal` y `incumpleSeparacion`; la fase 2a queda solo
  para el **viernes**; el orden blando es **S1 > S2 > viernes > preferencia**. Con separación dura,
  **7 h o más** es **infactible por causa (5)** (4+ apariciones en días no consecutivos); se elimina el
  «7–10 h se informa como incumplimiento». El **informe** añade **curso por materia**, **apariciones con
  franjas**, **días incumplidos** y **agregados por curso**; el mensaje de «no atribuible» cita **(1)–(5)**
  y el diagnóstico de bloques. El **seed sintético** pasa a **horas pares** (ninguna materia impar de
  laboratorio/taller sin pareja del mismo tipo). Tareas nuevas: **B5** (reglas duras, causa 7 h,
  jerarquía), **B6** (informe) y **B7** (seed); **B3** enumera las enmiendas documentales
  (§7.1.6/§7.2/§7.3/§7.5 sin S3 y con eventos como segmentos; 010 catálogo (1)–(5); 013 sin S3 ni
  `PESO_AGRUPACION`; 009 AC-8; 005 cuatro categorías y «reproducible» → válido).
- **2026-11-06 — la separación vuelve a blanda; los huecos al final siguen duros.** El usuario decide
  revertir la separación a **blanda con prioridad alta**: vuelve `incumpleSeparacion` con
  `PESO_SEPARACION`, la fase 2a es **separación + viernes** y el orden blando queda **S1 > S2 > separación
  > viernes > preferencia**. Los **huecos al final siguen duros** (ocupación monótona). La causa (5) vuelve
  a **11 h o más** (`⌈h/2⌉ > nº_días`); **7–10 h** pasa a «separación incumplida informada». Criterios:
  **4** pasa a «separación informada y ≤ la referencia» (no 0); **3** vuelve a 11 h; **5** y **7** se
  mantienen (huecos duros; el 5º con el bloque compartido y la separación informada). **B5** queda
  **superada**; se añade **B5b** (devolver la separación a blanda, mantener los huecos duros y la causa en
  11 h; aceptación con separación ≤ referencia y `huecosNoFinal == 0`). **B3/B4** se ajustan.
- **2026-11-07 — B3, parte de `specs/` (supersesiones documentales).** Notas **aditivas** (sin tocar el
  contenido normativo) en: **005** («bloque = 1 franja» convive con el «bloque de dos horas» de la 014;
  catálogo de causas **(1)–(5)** con la nueva «conflicto de bloques»; la promesa «reproducible» queda
  **superada** por «misma entrada ⇒ válido» con 8 workers); **009-agrupacion-bloques-materias** (**superada
  entera**: S3 eliminada, la sustituyen emparejamiento y alineación; incluye **AC-8** y sus tests; se
  conserva la noción de segmento); **010** (los **eventos** no crean ni quitan franjas pero **definen los
  segmentos** del bloque; catálogo **(1)–(5)**); **013** (superadas **RF-5**, criterios **6 y 10**, S3,
  `PESO_AGRUPACION` y la prueba de contrato `PESO_AGRUPACION == 10`; fases **1/2a/2b**; garantía estructural
  respecto de la **nueva referencia**). La parte de **`docs/`** de B3 (constitución, contrato `2.0`,
  `interfaz-frontend.md`) la ejecutó el implementer.
- **2026-11-07 — 4 bloqueantes residuales de coherencia documental corregidos.** (1) La 014 `spec.md`
  deja de describir la **separación como dura** en el alcance, la consecuencia 2, las prioridades blandas y
  la tabla de decisiones (D-6): **S1 > S2 > separación > viernes > preferencia**, separación y viernes
  **blandos**, huecos al final **duros**. (2) El plan deja de decir que la separación es dura y causa (5):
  es **blanda**, se informa y **no es causa (5)**. (3) Las citas de la forma dura del reparto pasan a
  **§7.1.6** (invariante 6 de §7.1); la sección «Horas de la materia-curso» no es la forma del reparto.
  (4) La
  **RNF-3 de la 013** pasa a **300 s** y el «+50 %» frente a la generación de referencia queda superado por
  la 014 (8 workers, fases 1/2a/2b). Además, la 005 anota la supersesión de su umbral de 60 s
  (**fixture de 848 clases**, plazo 300 s) y las mediciones se actualizan a **172,6 s** (centro) y
  **183,58 s** (test lento).
- **2026-11-07 — VEREDICTO APROBADO y cierre.** El reviewer da **APROBADO (2026-11-07)**; la `spec.md`
  pasa a **`Estado: implementada (2026-11-07)`** y esta fase queda **cerrada**. Se anotan las deudas
  aceptadas: **cancha** para dos cursos simultáneos con **aforo simultáneo por aula** como spec futura;
  **determinismo superado** (8 workers) y **plazo 300 s**; medición **172,6 s / 183,58 s**; test de
  aceptación con **30 docentes**; pendiente opcional de aserciones por curso (huecos 6/4 y
  `bloquesCompartidos == 4`); `docs/solver/Tareas-Implementacion.md` con «<60 s» histórico. El trabajo
  sigue fuera de esta spec: **seed con los planes reales**, **aforo simultáneo**, **rendimiento** y
  **commits**.

## Notas

- **Parche de `MEMORY.md`: NO aplicado** (el planner solo escribe en `specs/`). Contenido exacto propuesto
  para el coordinator:
  1. «Fase actual»: **014 implementada y cerrada (2026-11-07, VEREDICTO APROBADO)**; 014a T1–T7 y 014b
     B1–B7 en el árbol y sin commitear.
  2. Tabla «Specs activas», fila 014: `implementada (2026-11-07) | cerrada; 8 workers y plazo 300 s; S3
     eliminada; centro real factible`.
  3. «Siguiente paso»: **seed con los planes reales**, **spec de aforo simultáneo por aula**,
     **rendimiento** y **commits** del árbol.
  4. Decisiones del épico: **8 workers** (válido, no idéntico) y **plazo 300 s**; **S3 eliminada** (la 009
     queda superada); **centro real factible**: **848 clases, desvío 0, separación 2**; **medición 172,6 s**
     (centro) y **183,58 s** (test lento); **cancha confirmada** para dos cursos simultáneos y **aforo
     simultáneo por aula = spec futura**.
- **`seed.md` no se versiona** (raíz, sin seguimiento). La spec no lo cita por nombre.
- **Precisión de unidades:** «bloque de dos horas» = **dos franjas** (dos horas académicas), no dos horas de
  reloj.
