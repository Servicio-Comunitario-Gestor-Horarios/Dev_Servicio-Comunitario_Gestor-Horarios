# Spec 014 - Materias en bloques de dos horas

Estado: implementada (2026-11-07)

> **Alcance.** Esta spec define **cómo se reparten las horas de cada curso en bloques de dos horas**,
> **cada cuántos días puede repetirse una materia** y **dónde quedan las franjas libres**. En cada
> **(curso, día, segmento)** las franjas ocupadas se agrupan en **pares adyacentes**; un par puede ser
> **2 h de la misma materia** (**bloque simple**) o **1 h + 1 h de dos materias** (**bloque compartido**,
> que empareja los dos restos impares). **No hay bloques de clase de una hora.** Además, dos apariciones
> de la misma materia en el mismo curso se separan **al menos un día** (**blanda con prioridad alta**), y las
> franjas libres van **al final del día** (duro), con prioridad al **viernes** (blando). Cubre el bloque, la
> alineación, la separación, los huecos, la cobertura, la métrica, la jerarquía, el aula, el diagnóstico, el
> alcance y el informe.
>
> **Fuera de alcance:** la interfaz de usuario; el modelo persistido y el esquema; el contrato de
> configuración; la hora académica (sigue siendo **una franja**); las invariantes D1–D13, S1, S2 y las
> validaciones V1–V19; la generación parcial y los turnos automáticos.
>
> **Estado de las decisiones: aprobadas (2026-11-06) y cerradas; la spec está implementada desde el
> 2026-11-07.** D-1/N-1 regla **dura**;
> D-2 rejilla del curso y bloque compartido; D-3 mismo día, turno y segmento; D-4 cada materia en bloques
> enteros con su resto compartido; **D-5 S3 eliminada**; D-6 la preferencia al final del orden blando;
> D-7 y N-2 misma aula; D-8 causa **(5)**; D-9 todos los cursos; D-10 contrato `2.0` intacto;
> **N-3 huecos al final y viernes**; N-4 tramos de 4 válidos; **N-5 separación** (blanda con prioridad
> alta; actualizada el 2026-11-06); N-6 toda
> aparición cuenta; N-7 solo el curso.
>
> **QA de la spec (2026-11-06): CAMBIOS NECESARIOS incorporados.** Se separan **reglas duras** y
> **orden blando** (B1), se define la causa (5) con **condiciones detectables** y su relación con (2) y
> «no atribuible» (B2), se separa factibilidad dura / reglas de separación y huecos / instancia real, se corrigen la
> aritmética, la métrica (un solo desvío), la numeración de días (dominio), las citas de la constitución,
> las supersesiones completas de la 013 y la 009 y el tamaño (división 014a/014b).
>
> **Orden de trabajo.** Después de la **013** (preferencias del docente, cerrada el 2026-11-06). La 013
> mantiene su garantía estructural **respecto de la nueva generación de referencia**, pero su fase de
> desempate con S3 desaparece y se rehace en el plan (RF-8).

## Contexto y objetivo

Hoy una materia de varias horas semanales se imparte en franjas sueltas y sin relación con sus vecinas:
la agrupación (**S3**) solo penaliza los grupos de una franja de una materia, con un peso pequeño
(`PESO_AGRUPACION`, 10), y no distingue un par de un grupo de tres; la preferencia horaria (013) pesa más
(25) y deshace los pares. El centro quiere que **todo bloque de clase sea de dos horas académicas**, que
**una materia no se vea dos días seguidos** y que, cuando sobren franjas, **el curso salga temprano** y
preferentemente el **viernes**.

**Los datos del centro (confirmados el 2026-11-06).** Dos planes: **31059** (Media General, 1º–3º con
**38 h** y 4º–5º con **40 h**) y **31060** (Ciencias y Tecnología, **40 h** en los cinco años).
**24 cursos** (15 de 31059 y 9 de 31060), **889 alumnos**, **12 cursos por turno**:

| Turno | Plan | Cursos | Nº |
|---|---|---|---|
| **Mañana** | 31059 | 1A, 1B, 3A, 3B, 3C, 4A, 4B, 4C, 5A, 5B, 5C, 5D | **12** |
| **Tarde** | 31059 | 2A, 2B, 2C | 3 |
| **Tarde** | 31060 | 1A, 1B, 1C, 2A, 2B, 3A, 3B, 4A, 4B | 9 |

**15 docentes por turno** (30 en total), **sin cruzar turnos**. **12 aulas de uso general por turno,
compartidas entre turnos**. En todo el centro **solo hay dos materias impares**: **Castellano 3** e
**Inglés 3**, las dos en **5º de 31059**, y suman 40 (par). Las materias de **6 h** son: Ciencias Naturales
y GHC en **1º y 2º de 31059**; GHC e ITP en **1º y 2º de 31060**; ITP en **3º y 4º de 31060**. **14 de
los 24 cursos** tienen al menos una materia de 6 h.

> **Cancha y Educación Física (confirmado el 2026-11-06).** La cancha del liceo **puede usarse por dos
> cursos a la vez**: su capacidad es `5 días × 16 franjas × 2 = 160` frente a **96 clases** de Educación
> Física (4 h × 24 cursos). **Deja de ser un problema de capacidad** y el centro es **factible por ese
> lado**: no hay causa (2) por la cancha.
>
> **Consecuencia de modelado (fuera de la 014).** El solver actual **no admite dos cursos a la vez en la
> misma aula** (D8/exclusividad de aula, spec 010). Sostener la cancha compartida exige una **capacidad de
> aforo simultáneo por aula**, que es un **requisito nuevo** y **no entra en la 014**: se anota como **spec
> futura «aforo simultáneo por aula / aulas compartidas»**, con la justificación (cancha × 2, 96 ≤ 160).
> Mientras no exista, la configuración no puede expresar «dos cursos en la cancha a la vez».
>
> **Fixture de aceptación (sin Educación Física).** Para las pruebas se **excluye Educación Física** del
> horario y se marca **«no evaluada»**. Los 24 cursos pasan a **34 h** (31059 1º–3º, antes 38) y **36 h**
> (31059 4º–5º y 31060, antes 40), con **848 clases** en total (antes 944): **8 cursos de 34 h** y **16 de
> 36 h**. Bloques: **17** y **18** sobre una capacidad de 20; **margen** de **3 bloques (6 h)** y **2
> bloques (4 h)**; **huecos** de **6** y **4** por curso. Los criterios 4–7 y 14 y el benchmark se calculan
> sobre este fixture; **EF en la cancha no se usa como criterio**. «EF en aula general» sigue como **prueba
> de sensibilidad opcional**. Cuando exista el aforo simultáneo, el fixture podrá volver a los **planes
> completos (944 clases)**.

**Consecuencia 1 (candidata, no prueba): el emparejamiento es alcanzable.** Todos los cursos suman un
número **par** de horas y los segmentos son pares (2 y 6), así que **todos pueden quedar sin bloques de
1**. En 5º el 3+3 se resuelve 2+1 / 2+1 con **los dos restos compartiendo un bloque**. Se construyó un
reparto **candidato** por curso (conteo y construcción a mano); **no es una prueba de factibilidad de la
instancia real**, porque no considera docentes ni aulas.

**Consecuencia 2: la rejilla de un curso lleno ya está emparejada.** Un curso de 40 h ocupa todas las
franjas de su turno: los tramos miden 6 y 2, siempre par, y la rejilla se empareja **por construcción**,
sea cual sea el reparto por materias. Lo que cambia el horario es que **cada materia ocupe bloques
enteros** (D-4) y que **no se repita en días consecutivos** (**blanda con prioridad alta**).

**Consecuencia 3: la separación de un día determina los días de las materias largas.** Con 5 días y
bloques de 2 h, dos apariciones separadas por al menos un día solo admiten las parejas (lunes, miércoles),
(lunes, jueves), (lunes, viernes), (martes, jueves), (martes, viernes) y (miércoles, viernes). Por tanto:

| Horas de la materia | Apariciones | Días que cumplen la separación |
|---|---|---|
| 1 h | 1 (resto) | cualquiera |
| 2 h | 1 (bloque simple) | cualquiera |
| 3 h | 2 (2 h + resto) | 6 combinaciones |
| 4 h | 2 (dos bloques simples) | 6 combinaciones |
| 5 h | 3 (2+2+1) | **solo lunes/miércoles/viernes** |
| 6 h | 3 (2+2+2) | **solo lunes/miércoles/viernes** |
| 7–10 h | 4 o 5 | no cabe con un día intermedio → **separación incumplida informada** |
| 11 h o más | 6 o más | **no es solución** (causa (5)): más apariciones que días |

**Consecuencia 4: el margen de la rejilla.** *Margen* = capacidad de bloques de la rejilla
(`nº_días × |franjas del turno| / 2`) − bloques que necesitan las materias. En el **centro con EF**, un
curso de 40 h necesita **20 bloques que ocupan 40 franjas**: margen **0** y ningún hueco. Un curso de 38 h
necesita **19 bloques en 40 franjas**: margen **1 bloque (2 h)**, que es el hueco del curso. En los cursos con dos materias de
6 h, el lunes, el miércoles y el viernes quedan **4 h de capacidad libre para otras materias** (dos
bloques) y el martes y el jueves, 8 h; en los de una materia de 6 h, 6 h. Esa capacidad **no es un
hueco**: son franjas que ocupan otras materias. Los 16 cursos de 40 h quedan **sin margen**; los 8 de
38 h, con **1 bloque**. En el **fixture sin EF** los totales son **36/34 h**: **18/17 bloques**, margen de
**2/3 bloques** y **4/6 huecos**.

**Consecuencia 5: los huecos al final son la forma natural de cuadrar la paridad.** Si las franjas libres
de un curso se colocan **al final del día**, la parte ocupada es un **prefijo**; con un número **par** de
huecos por día, cada tramo queda par y no aparece ninguna clase suelta. En los 8 cursos de 38 h basta con
**los 2 huecos al final del viernes**: el curso sale temprano ese día. Si un curso tuviera 4 huecos, los
**4 al final del viernes** son mejores que repartirlos. En el **fixture sin EF**, los cursos de **36 h**
tienen **4 huecos** (los 4 al final del viernes) y los de **34 h**, **6** (4 al final del viernes y 2 al
final de otro día cuando hay dos materias de 6 h, que ya ocupan 4 h del viernes).

**Tres planos que no se mezclan.** (a) **Factibilidad dura:** el emparejamiento y la alineación se cumplen
o el horario no es solución. (b) **Huecos al final: duros; separación y viernes: blandos**; un horario con
un hueco en medio no es solución; la separación se intenta y se informa. (c) **Factibilidad de la
instancia:** docentes, aulas y cancha
(confirmada compartida por dos cursos; el **aforo simultáneo por aula** es una spec futura); es un problema
de los datos y de la causa (2)/(3)/(4) de la 010, no de esta spec.

**Por qué no basta con S3 y por qué se elimina.** S3 cuenta grupos de 1 **de una materia**, no distingue un
par de un grupo de 3, no suma los grupos de 4 o más (deuda del motor) y pesa menos que la preferencia. Las
cifras del conjunto sintético anterior (359 grupos de 1, 76 de 2, 3 de 3 y penalización 166) son
**mediciones de ese conjunto, no del centro real**, y se citan solo como ilustración del estado previo: la
rejilla está llena pero las materias no están en bloques. La agrupación se mide ahora **solo** con el
desvío de esta spec, y **S3 desaparece** (D-5); su lugar en el objetivo lo ocupan la separación y los
huecos.

## Usuarios

- **Usuario académico que genera horarios:** espera que cada clase dure dos horas académicas, que una
  materia no se repita al día siguiente y que, si sobran franjas, el curso salga temprano; y entender
  cuándo no ha podido ser.
- **Jefe de estudios / coordinación:** espera que el bloque de dos horas sea un **bloque real** (dos
  franjas seguidas, sin recreo en medio, en la misma aula) y que los restos impares se compartan.
- **Usuario que mantiene docentes:** no debe tener que capturar ningún dato nuevo.
- **Equipo del solver:** necesita una definición única, medible y con pruebas por caso, y el orden exacto
  de las prioridades.
- **Equipo del frontend:** necesita saber que el contrato no cambia y de dónde sale el informe.
- **Equipo de desarrollo:** necesita que la constitución y las specs 005, 009 (agrupación), 010 y 013 no
  contradigan el comportamiento nuevo.

## Historias de usuario

- HU-1. Como usuario académico, quiero que toda clase se imparta en un bloque de dos horas académicas.
- HU-2. Como usuario académico, quiero que los restos impares de dos materias se junten en un bloque
  compartido en lugar de quedar como horas sueltas.
- HU-3. Como usuario académico, quiero que una misma materia no se vea dos días seguidos, para que el
  estudiante tenga tiempo de asimilarla, y saber cuándo no se ha podido.
- HU-4. Como usuario académico, quiero que, si un curso tiene franjas libres, salga temprano y
  preferentemente el viernes.
- HU-5. Como usuario académico, quiero que estas reglas no me dejen sin horario cuando los datos lo
  permiten, y que si no lo permiten se me diga por qué.
- HU-6. Como usuario académico, quiero saber cuántos bloques, compartidos y huecos tiene el horario, en
  qué días cae cada materia y qué separaciones no se cumplieron.
- HU-7. Como jefe de estudios, quiero que el bloque no tenga un recreo en medio ni cambie de aula.
- HU-8. Como miembro del equipo del solver, quiero una métrica única y verificable, con al menos una
  prueba por caso límite.
- HU-9. Como responsable de los datos, quiero que la regla no exija capturar nada nuevo ni migrar la
  base, los presets o los horarios guardados.

## Definiciones

- **Hora académica y franja:** una hora académica es una **franja** (`bloques(horas) = horas`, spec 005).
  El tamaño real de la franja no cambia. **«Bloque de dos horas»** (o **bloque**) es la unidad de dos
  franjas de esta spec.
- **Rejilla del curso:** las franjas del turno declarado del curso, organizadas por **día** y **segmento
  contiguo**.
- **Segmento contiguo del turno:** las franjas de un turno divididas por sus **eventos** (receso, acto
  cívico). Los eventos **no crean ni quitan franjas**, pero **definen los segmentos**; un bloque nunca
  cruza un evento ni el final del turno (D-3).
- **Franja ocupada / hueco:** ocupada = con una clase; **hueco** = franja del turno sin clase. Un hueco
  separa tramos y no forma parte de ningún bloque.
- **Capacidad libre para otras materias:** franjas de un día que no ocupa una materia concreta y que
  pueden ocupar otras; **no** es un hueco.
- **Tramo ocupado:** secuencia máxima de franjas ocupadas consecutivas de un día y segmento.
- **Bloque de dos horas:** dos franjas adyacentes y ocupadas del mismo día y segmento. **Simple** si son
  de la misma materia-curso; **compartido** si son de dos materias distintas. Un **tramo de 4** son dos
  bloques adyacentes, y **no pueden ser dos bloques de la misma materia** el mismo día (D-4).
- **Bloque de 1 de clase:** franja ocupada que no forma bloque de dos horas. **No es solución** (regla
  dura); el término se conserva para el diagnóstico.
- **Aparición de una materia-curso:** cada bloque de dos horas en el que la materia participa. Una
  materia de 3 h tiene 2 apariciones (un bloque simple y su resto, que comparte bloque); una de 6 h, 3.
- **Separación (blanda con prioridad alta):** dos apariciones de la misma materia-curso en el mismo curso
  distan **al menos un día intermedio** (diferencia de días ≥ 2). Prohíbe el mismo día (duro, D-4) y el día
  siguiente (blando); se informa lo que no se logre y **no vuelve infactible** el horario (N-5).
- **Incumplimiento de separación:** par de apariciones de la misma materia-curso en **días consecutivos**.
  Se cuenta y se informa; es una **prioridad blanda de alta prioridad**.
- **Hueco al final del día (duro):** en cada (curso, día) las franjas libres son un **sufijo**, es decir,
  la ocupación es monótona: `ocupada[s] ≥ ocupada[s+1]`. La parte ocupada del día es un **prefijo**.
- **Franja descubierta de una materia-curso:** franja suya que **no** forma parte de un bloque de dos
  horas suyo (está en un bloque compartido o en un bloque de 1).
- **Alineación:** cada materia-curso tiene **como máximo `h mod 2` franjas descubiertas**; su resto (si
  `h` es impar) es exactamente una. Un bloque compartido descubre una franja de cada una de sus dos
  materias; **nunca** empareja una materia **sin resto** (horas pares).
- **Desvío (único):** suma, sobre las materias de un curso, de `máx(0, franjas descubiertas − h mod 2)`.
  **No hay un segundo término por bloques de 1:** un bloque de 1 no es solución y no se suma dos veces.
  En una solución factible el desvío es 0.
- **Tope de franjas descubiertas (N-1):** por materia, `franjas descubiertas ≤ h mod 2`; por curso, el
  total no supera `Σ(h mod 2)`. Es la forma verificable del «desvío máximo».
- **Margen de la rejilla:** capacidad de bloques (`nº_días × |franjas del turno| / 2`) − bloques que
  necesitan las materias. 40 h: 0; 38 h: 1 bloque (2 h).
- **Total semanal del curso:** suma de las horas de sus materias. **Debe ser par** para que todo bloque
  sea de dos franjas; un total impar **no admite solución** (causa (5)).
- **Máximo de apariciones de una materia:** `⌈h/2⌉ ≤ nº_días`; con 5 días, **h ≤ 10** (par) y **h ≤ 9**
  (impar). **11 h o más no es solución** (causa (5)). Con separación blanda, una materia de **7–10 h** no
  puede cumplir la separación y se informa como incumplimiento.
- **Día de dominio:** **0 = domingo … 6 = sábado**, la convención del resultado (la misma de la 013). La
  generación numera internamente 0 = lunes; la conversión a día de dominio forma parte del informe.
- **Generación de referencia:** la misma configuración **sin la sección de preferencias**, ya con las
  reglas de esta spec. Es la referencia de la garantía estructural.
- **Jerarquía:** las **reglas duras** (emparejamiento, alineación, misma aula y **huecos al final del día**)
  son obligatorias; **entre las soluciones factibles**, el orden blando es **S1 > S2 > separación >
  viernes > preferencia**. **S3 ya no existe.**

## Reglas y prioridades (resumen normativo)

**Reglas duras (siempre obligatorias; no son un nivel del objetivo).** Un horario que las incumpla **no es
solución**, y ninguna prioridad blanda puede compensarlo.

1. **Emparejamiento:** todas las franjas ocupadas de la rejilla del curso se cubren con bloques de dos
   horas adyacentes; no hay bloques de 1 de clase.
2. **Alineación:** cada materia ocupa bloques enteros salvo su resto, y el resto comparte bloque con el
   resto de otra materia del mismo curso y segmento. Un bloque compartido contiene exactamente dos restos;
   nunca una materia **sin resto** (horas pares).
3. **Tope de franjas descubiertas:** por materia, `≤ h mod 2`; por curso, `≤ Σ(h mod 2)`. El desvío
   (exceso sobre el tope) es 0.
4. **Total par y máximo de apariciones:** un curso con total impar, o con una materia de más apariciones
   que días (`⌈h/2⌉ > nº_días`; con 5 días, h ≥ 11), no admite solución.
5. **Un bloque por materia y día.**
6. **Huecos al final del día:** en cada (curso, día), `ocupada[s] ≥ ocupada[s+1]`; las franjas libres son
   un sufijo.

**Orden blando entre soluciones factibles.** Se optimiza sin violar las reglas duras, de mayor a menor:

1. **S1** exceso de horas de aula (`PESO_HORAS`).
2. **S2** titulares antes que suplentes (`COSTE_SUPLENTE`).
3. **Separación** (N-5): minimizar los pares de apariciones en días consecutivos.
4. **Viernes** (N-3): minimizar los huecos que no caen en **viernes**; el «al final del día» ya es duro.
5. **Preferencia horaria del docente** (013).

**S3: eliminada** (D-5). No hay término de agrupación distinto del desvío.

## Requisitos funcionales

### RF-1. Bloque de dos horas y emparejamiento de la rejilla

- **EL SISTEMA considerará tramo ocupado de un curso a una secuencia máxima de franjas ocupadas
  consecutivas dentro de un mismo día y segmento.**
- **EL SISTEMA cubrirá cada tramo ocupado con bloques de dos horas adyacentes:** un tramo de longitud par
  queda cubierto por `L/2` bloques; **un tramo de longitud impar no es solución** (regla dura).
- **EL SISTEMA considerará bloque simple al de dos franjas de la misma materia-curso y bloque compartido
  al de dos materias distintas.**
- **CUANDO un tramo tenga 4 franjas, EL SISTEMA lo considerará dos bloques de dos horas válidos**, y
  **no permitirá que los dos sean de la misma materia** el mismo día (D-4).
- **EL SISTEMA no considerará bloque a dos franjas separadas por un evento, por un hueco, por el final de
  un turno o por un cambio de día:** son tramos distintos (D-3).
- **EL SISTEMA no contará como bloque compartido a un hueco junto a una franja ocupada.**
- **EL SISTEMA mantendrá la identidad hora académica = franja** y no redimensionará la rejilla.

### RF-2. Alineación de las materias con los bloques (D-4 = A)

- **EL SISTEMA exigirá que cada materia-curso ocupe bloques enteros**, de modo que sus horas queden
  cubiertas por bloques simples, salvo su **resto**.
- **SI una materia-curso tiene horas pares, ENTONCES no tendrá ninguna franja descubierta; SI tiene horas
  impares, ENTONCES tendrá exactamente una** (su resto).
- **CUANDO una materia-curso tenga un resto, EL SISTEMA lo emparejará en un bloque compartido con el
  resto de otra materia del mismo curso, en el mismo día y segmento.**
- **EL SISTEMA garantizará que un bloque compartido contiene exactamente dos restos** (uno de cada
  materia); **nunca empareja una materia sin resto** (horas pares).
- **EL SISTEMA no permitirá más de un bloque de la misma materia por día** (regla de D-4).
- **SI una materia no puede alinearse, ENTONCES el curso no tendrá solución** y se aplicará RF-10: causa
  (5) si se cumple una condición detectable; (2)/(3)/(4) si su conteo lo explica; si no, **no atribuible**.

### RF-3. Separación de un día entre apariciones (blanda con prioridad alta, N-5 = blanda, N-6 = A, N-7 = A)

- **EL SISTEMA intentará que dos apariciones cualesquiera de la misma materia-curso en el mismo curso estén
  separadas por al menos un día intermedio:** no el mismo día ni el día siguiente. Es una **prioridad
  blanda de alta prioridad**.
- **EL SISTEMA contará como aparición todo bloque en el que la materia participa**, sea simple o
  compartido; un bloque de 1, si existiera, contaría igual (N-6), aunque no es solución.
- **EL SISTEMA aplicará la separación solo dentro del curso:** la misma materia en dos cursos distintos
  no se restringe entre sí (N-7).
- **CUANDO una materia-curso tenga 3 apariciones, EL SISTEMA preferirá situarlas en lunes, miércoles y
  viernes**, la única combinación de tres días con diferencia ≥ 2 en una semana de cinco.
- **SI una materia-curso tuviera 7–10 h, ENTONCES no podrá cumplir la separación** y se informará como
  incumplimiento, no como infactibilidad; **11 h o más no es solución** (causa (5)).
- **EL SISTEMA contará y informará las apariciones, los días y los incumplimientos de separación de cada
  materia**; no declarará no factible por esta regla (es blanda).
- **EL SISTEMA no permitirá dos apariciones de la misma materia el mismo día** (regla dura de D-4).

### RF-4. Huecos al final del día (duro) y viernes (blando, N-3)

- **EL SISTEMA exigirá que, en cada (curso, día), las franjas libres sean un sufijo:** `ocupada[s] ≥
  ocupada[s+1]` dentro del día. La parte ocupada es un **prefijo** y el curso sale temprano **siempre**.
- **EL SISTEMA exigirá que cada tramo ocupado quede de longitud par:** con la ocupación monótona, eso
  fija que el número de huecos por día tenga la paridad del segmento; un tramo impar no es solución.
- **EL SISTEMA preferirá el viernes** para los huecos (prioridad **blanda**): minimizará los huecos que no
  caen en viernes. **Si un curso tiene 4 huecos, preferirá los 4 al final del viernes** antes que
  repartirlos en dos días.
- **EL SISTEMA contará y informará, por curso y día, los huecos (cuántos y en qué franjas) y si caen en
  viernes**; el «hueco fuera del final» es **0** en toda solución factible y el «no viernes» es la
  preferencia blanda.
- **SI no existe una disposición de huecos que respete la ocupación monótona y la paridad, ENTONCES el
  resultado es no factible** con la causa que corresponda (RF-10), sin inventar una.
- **EL SISTEMA no admitirá una clase suelta** (bloque de 1): la paridad es dura.

### RF-5. Cobertura, paridad y condiciones del desvío 0

- **EL SISTEMA cubrirá todas las franjas ocupadas de la rejilla con bloques**, sin solapes y sin dejar
  franjas sin clasificar.
- **SI el total semanal del curso es impar, ENTONCES no hay solución** (no se cubre un número impar de
  franjas con bloques de dos); causa (5).
- **SI alguna materia necesita más apariciones que días tiene la semana (`⌈h/2⌉ > nº_días`),
  ENTONCES no hay solución**; causa (5). Con 5 días, una materia de **11 h o más**.
- **El desvío 0 es alcanzable —en la rejilla del curso— SI Y SOLO SI:** el total del curso es par,
  ninguna materia supera el máximo de apariciones por semana (≤ días), **los restos del curso se pueden
  emparejar por día y segmento con el mismo tipo de aula** y **las franjas libres son un sufijo** (huecos
  al final). **«Total par» es necesario, no suficiente.** La factibilidad con docentes y aulas es la de la
  instancia (plano (c) del contexto), no esta condición de rejilla.
- **CUANDO el curso tenga huecos, EL SISTEMA los situará de modo que cada tramo quede de longitud par**
  (huecos al final y en número par por día, RF-4).
- **EL SISTEMA contará el desvío de cada curso, lo comparará con el tope `Σ(h mod 2)` y lo informará.**
- **EL SISTEMA no declarará desvío 0 si existe un bloque de 1 o una franja descubierta en exceso.**

### RF-6. Métrica única (S3 eliminada, D-5 = C)

- **EL SISTEMA medirá la agrupación con un único desvío:** `Σ sobre las materias de
  máx(0, franjas descubiertas − h mod 2)`. **No hay término separado por bloques de 1.**
- **EL SISTEMA expresará la alineación por materia como «franjas descubiertas ≤ h mod 2»** y la usará como
  tope; el desvío es el exceso sobre ese tope.
- **EL SISTEMA no contará dos veces la misma franja:** un bloque de 1 no es solución y no suma un segundo
  término; un bloque compartido legítimo (dos restos) no es desvío.
- **EL SISTEMA no aplicará la métrica de la 009** (grupos de 1 por materia; 2–3 sin penalizar; 4 o más sin
  sumar en el motor) **ni mantendrá S3**: la agrupación de la 009 queda **superada entera** y solo se
  conserva su noción de segmento contiguo.
- **EL SISTEMA informará el desvío y el tope `Σ(h mod 2)` por curso**; en toda solución factible el desvío
  es 0 y el tope se cumple con holgura.
- **EL SISTEMA no usará el desvío como prioridad a optimizar:** lo fijan las reglas duras; las prioridades
  blandas son **S1, S2, la separación y el viernes**.

### RF-7. Naturaleza de las reglas y garantía

- **EL SISTEMA tratará el emparejamiento, la alineación, la ausencia de bloques de 1 y los huecos al final
  del día como reglas duras y siempre obligatorias:** un horario que las incumpla **no es solución**, y
  ninguna prioridad blanda puede compensarlo.
- **EL SISTEMA exigirá los topes de RF-2, RF-4 y RF-5:** por materia `franjas descubiertas ≤ h mod 2`, por
  curso `≤ Σ(h mod 2)`, el máximo de apariciones por semana (≤ días) y la ocupación monótona
  (`ocupada[s] ≥ ocupada[s+1]`); si no puede, declarará no factible con la causa de RF-10.
- **EL SISTEMA mantendrá la garantía estructural de la 013 respecto de la nueva generación de
  referencia:** con las mismas preferencias, el reparto de docente por materia-curso y el exceso de horas
  por docente son los de la generación de referencia de esta spec (no los valores de la 013).
- **EL SISTEMA no cambiará qué docente dicta cada materia-curso ni el exceso de horas de cada docente**
  para mejorar el emparejamiento, la separación, los huecos o el viernes.
- **EL SISTEMA no relajará ninguna regla en silencio.**

### RF-8. Orden blando, preferencia horaria y rework de la 013 (D-6 = A, D-5 = C)

- **Entre las soluciones factibles, EL SISTEMA aplicará el orden blando S1 > S2 > separación > viernes >
  preferencia horaria**, sobre el resultado y no por la magnitud de un peso por unidad. **Las reglas duras
  (incluidos los huecos al final) no son un nivel de este orden:** se cumplen siempre.
- **CUANDO respetar una preferencia horaria obligue a romper una regla dura, EL SISTEMA no lo hará:** la
  regla dura se mantiene y la preferencia se incumple; el resultado lo informará como hoy (RF-8 de la 013).
- **EL SISTEMA podrá situar clases en franjas evitadas para cumplir la separación o los huecos al final**,
  y el usuario verá el coste en el informe de preferencias.
- **EL SISTEMA eliminará la fase de desempate con S3 de la 013:** su lugar lo ocupan la **separación** y el
  **viernes** (blandos), que se resuelven **antes** que la preferencia; el desvío y los huecos al final
  quedan fijados por el modelo.
- **EL SISTEMA rehará en el plan** la jerarquía de fases de la 013, su RNF-4/criterio de no regresión y su
  golden de horario; **la huella de datos no cambia respecto de la 013** (los datos son los mismos).
- **CUANDO la generación no tenga preferencias intervinientes, EL SISTEMA aplicará igualmente las reglas
  duras y el orden blando** y el horario podrá diferir del de la 013.

### RF-9. Aula durante el bloque (D-7 = A, N-2 = A)

- **MIENTRAS el modo de aula fija esté activo, EL SISTEMA impartirá las dos franjas de un bloque simple
  en la misma aula:** el aula de referencia si su tipo coincide con el requerido, o la misma aula del
  tipo requerido si la clase se desplaza.
- **MIENTRAS el modo de aula fija esté inactivo, EL SISTEMA impartirá las dos franjas de un bloque simple
  en la misma aula elegida**, libre en ambas franjas.
- **CUANDO el bloque sea compartido, EL SISTEMA lo impartirá en la misma aula en sus dos franjas**, de
  modo que solo podrá juntar dos restos de materias que requieran el **mismo tipo** de aula.
- **EL SISTEMA mantendrá intactas las reglas de tipo exacto, de exclusividad de aula y de referencia
  desplazada** de la 010.
- **SI la misma aula no puede sostener las dos franjas del bloque, EL SISTEMA no formará el bloque.** La
  causa será la **(2)** si el conteo de capacidad del tipo lo explica, o **no atribuible** si depende de
  un aula concreta; **nunca la (5)**, que no mide aulas.

### RF-10. Factibilidad y diagnóstico (D-8 = A, B2)

- **SI no se cumplen las reglas duras, EL SISTEMA declarará el problema no factible con la causa «(5)
  conflicto de bloques»** cuando se cumpla alguna de estas **condiciones detectables sobre la entrada**,
  análogas al conteo de capacidad de la (2):
  1. el **total semanal** de un curso es **impar**;
  2. alguna materia del curso necesita **más apariciones que días** tiene la semana
     (`⌈h/2⌉ > nº_días`; con 5 días, h ≥ 11).
- **EL SISTEMA colocará la causa (5) después de las cuatro categorías existentes** y no las sustituirá ni
  reordenará; el catálogo pasa a **(1)–(5)**.
- **SI ninguna categoría (1)–(5) explica el fallo, ENTONCES la causa será «no atribuible»**, como en la
  010, sin inventar una. **El mensaje citará (1)–(5) y el diagnóstico de bloques** (materias con más
  apariciones que días, restos sin pareja del mismo tipo, huecos que no caben al final), conforme a RNF-3.
- **EL SISTEMA no usará la causa (5) por** la **separación** (es blanda), **ni por la preferencia del
  viernes** (blanda), **ni por un aula concreta** (es (2) o no atribuible), **ni por la disposición de los
  huecos** si depende de la disponibilidad (será (3)/(4) si su conteo aplica o no atribuible).
- **EL SISTEMA no atribuirá a estas reglas una infactibilidad que venga de una falta de aulas o de
  capacidad:** esos casos siguen siendo la causa (2) de la 010.
- **EL SISTEMA no relajará ninguna regla en silencio.**

### RF-11. Alcance y excepciones (D-9 = A)

- **EL SISTEMA aplicará las reglas a todas las materias-curso de todos los cursos**, sin excepciones por
  área, tipo de aula, taller, educación física ni docente.
- **SI una materia tuviera 1 hora semanal, EL SISTEMA la tratará como resto** y la emparejará en un bloque
  compartido con otro resto; si no hubiera otro, el curso no será solución.
- **EL SISTEMA no requerirá ningún dato nuevo del centro.**
- **SI un plan diera 11 h o más a una materia, ENTONCES no cumplirá las reglas duras** (causa (5)); entre
  **7 y 10 h** la separación se informará como incumplimiento, pero el emparejamiento y la alineación
  siguen siendo posibles si los restos pueden compartir.

### RF-12. Observabilidad y contrato (D-10 = A)

- **CUANDO termine una generación, EL SISTEMA informará, por curso:** número de bloques de dos horas
  (simples y compartidos), número de bloques de 1 (0), **desvío y tope `Σ(h mod 2)` agregados del curso**,
  franjas descubiertas, huecos por día (cuántos y en qué franjas) y ubicación de los bloques compartidos.
- **EL SISTEMA informará, por materia-curso: su curso**, sus apariciones con su **día de dominio (0 =
  domingo)** y **las franjas de cada aparición**, su resto, sus franjas descubiertas, **los días incumplidos
  de separación** (se informa) y si la separación se cumple.
- **EL SISTEMA informará los totales de la generación** (bloques, compartidos, desvío, incumplimientos de
  separación y huecos que no caen en viernes).
- **EL SISTEMA calculará el informe sobre el horario devuelto**, sin cambiar el contrato de salida del
  solver.
- **EL SISTEMA mantendrá el contrato de configuración en `2.0`, sin secciones ni campos nuevos**; la
  huella de datos no cambia **respecto de la 013**.

### RF-13. Coherencia normativa, eventos y supersesiones

- **EL SISTEMA mantendrá alineada la constitución del proyecto:**
  - **§7 «Comportamiento»:** el catálogo de causas pasa a **(1)–(5)** y se añade la causa **(5) conflicto
    de bloques** con sus condiciones detectables; **§7.4 es V1–V19 y no se toca**.
  - **§7.1.6:** la forma **dura** del reparto, es decir, la **invariante 6 de §7.1** (cada materia en
    bloques enteros salvo su resto, resto compartido, un bloque por materia y día). *(No es la sección
    «Horas de la materia-curso».)*
  - **§7.2:** los **huecos al final del día** como regla **dura**; la **separación** y el **viernes** como
    prioridades **blandas**; el orden blando **S1 > S2 > separación > viernes > preferencia**; **S3
    eliminada**.
  - **§7.3 y §7.5:** los eventos **definen los segmentos** (no son metadata ignorada) y no queda ninguna
    referencia a **S3**.
  - **§7.1.6** (ya citado): la forma dura del reparto, sin S3.
- **EL SISTEMA anotará el efecto normativo de los eventos:** **no crean ni quitan franjas**, pero
  **definen los segmentos** del bloque. Esto supersede la lectura de la 010 (§7.3/§7.5, «metadata que el
  solver ignora») y la noción de segmento de la 009 («por receso»).
- **EL SISTEMA enumerará las supersesiones completas:**
  - **009-agrupación:** superada **entera** (su métrica de S3, su `PESO_AGRUPACION` y sus criterios de
    aceptación, incluido **AC-8**); se conserva la noción de **segmento contiguo**, ahora definida por los
    eventos.
  - **013:** quedan superados su **RF-5** (jerarquía con S3), sus **criterios 6 y 10**, sus definiciones
    de **S3**, **generación de referencia** y **peso**, y la **prueba de contrato que fija
    `PESO_AGRUPACION == 10`**; **sin S3 ni `PESO_AGRUPACION`**; se mantiene su garantía estructural
    **respecto de la nueva referencia** y su RNF-4/criterio 11 se rehacen.
  - **010:** el **catálogo de causas pasa a (1)–(5)**; se añade la (5) y el efecto normativo de los
    eventos.
  - **005:** el **catálogo de causas pasa a (1)–(5)** (hoy (1)–(4)); «reproducible» pasa a **válido** (ver
    el último punto); se conserva «1 hora académica = 1 bloque = 1 franja» y la nota del **doble uso de
    S3**.
  - **002:** el plazo del proceso de cálculo pasa a **300 s** (B1b, 014b; supersede el plazo anterior de un
    minuto); salud y apagado siguen
    en **5 s**; el centro completo con 8 workers tarda **172,6 s** y con el plazo anterior la separación
    empeoraba de **2 a 90**.
  - **Reproducibilidad del horario (005 y 013):** queda **superada** por esta spec. La 014 busca con **8
    workers** y **no promete el mismo horario entre ejecuciones**: la misma entrada produce un horario
    **válido**, no necesariamente idéntico. La **validez**, las **invariantes** y los **diagnósticos y
    cálculos puros** se mantienen; la garantía estructural de la 013 se mantiene **dentro de cada
    ejecución**, respecto de su propia generación de referencia.
- **EL SISTEMA dejará constancia de que no hay migración** de bases, presets ni horarios guardados, y de
  que los horarios anteriores no se reinterpretan.
- **EL SISTEMA documentará el bloque simple, el compartido, la alineación, la separación, los huecos, el
  tope y el máximo de 10 h por materia.**

## Requisitos no funcionales

- **RNF-1. Validez y búsqueda con 8 workers:** el resultado de una generación debe ser **válido** (todas
  las reglas duras y los topes de la 014 se cumplen) y el motor buscará con **8 workers** (semilla fija 0).
  **No se garantiza el mismo horario entre ejecuciones:** dos generaciones con la misma configuración
  pueden devolver horarios distintos, siempre válidos. Los **diagnósticos y cálculos puros** (tramos,
  desvío, causa (5), informe) siguen siendo deterministas.
- **RNF-2. Desempeño:** el escenario de referencia se resuelve por debajo del **plazo vigente del proceso de
  cálculo, 300 s** (002, supersesión de la 014b); el plan mide y documenta el margen. La medición se hace
  sobre el **fixture sin EF** (24 cursos, **848 clases**, **34/36 h**, **17/18 bloques**); referencia
  medida: **172,6 s** el centro completo con 8 workers, separación blanda y huecos al final duros, y
  **183,58 s** el **test lento** (`ctest --preset slow-release`). El
  conjunto sintético sigue siendo la referencia de regresión. La instancia con Educación Física en la
  cancha no se mide hasta que exista el **aforo simultáneo por aula** (spec futura).
- **RNF-3. Mensajes en español:** todo informe identifica curso, materia, bloques, compartidos, desvío,
  tope, días de cada materia y huecos; todo rechazo identifica el valor recibido y la acción disponible. **El
  mensaje de «no atribuible» citará las categorías (1)–(5) y el diagnóstico de bloques** (materias con más
  apariciones que días, restos sin pareja del mismo tipo, huecos que no caben al final).
- **RNF-4. Sin regresión en lo ajeno:** las invariantes D1–D13, S1, S2, las validaciones V1–V19, la regla
  de tipo y referencia de aula y el contrato `2.0` no cambian; la huella de datos tampoco **respecto de la
  013**.
- **RNF-5. Verificación sin la interfaz:** las reglas se prueban con la lógica del servidor y sus
  pruebas, sin la interfaz.
- **RNF-6. Protección de datos:** no se modifica ni migra ningún dato persistido, preset ni horario
  guardado; no hay cambio de esquema.
- **RNF-7. Trazabilidad:** cada RF tendrá al menos una comprobación verificable en las tareas.
- **RNF-8. Datos de prueba (seed sintético):** el conjunto sintético **no tendrá materias impares sin pareja
  del mismo tipo**: toda materia de **laboratorio/taller** con horas impares debe tener **otra del mismo
  tipo con resto**, o tener **horas pares**. La corrección del seed y su verificación son de la **014b
  (B7)**; no cambia el modelo del solver.

## Casos límite

- **Materia de 1 h:** una aparición (resto); comparte bloque con otro resto o el curso no es solución
  (RF-2, RF-11).
- **Materia de 2 h:** una aparición (bloque simple); cualquier día (RF-2, RF-3).
- **Materia de 3 h:** dos apariciones (bloque simple y resto compartido); seis combinaciones de días
  (RF-2, RF-3).
- **Materia de 4 h:** dos apariciones (dos bloques simples); seis combinaciones de días, nunca dos el
  mismo día (RF-2, RF-3).
- **Materia de 5 h:** tres apariciones (2+2+1); se **preferirán** lunes/miércoles/viernes (separación
  blanda); si no caben, se informa el incumplimiento (RF-3).
- **Materia de 6 h:** tres apariciones (2+2+2); se **preferirán** lunes/miércoles/viernes; si no caben, se
  informa (RF-3).
- **Materia de 7–10 h:** cuatro o cinco apariciones; la separación se **informará como incumplimiento**
  (blanda) (RF-3, RF-5).
- **Materia de 11 h o más:** más apariciones que días; **no es solución** (causa (5)) (RF-3, RF-5, RF-10).
- **Curso de 36 h (fixture sin EF):** 18 bloques que ocupan 36 franjas y **4 huecos**; margen de 2 bloques;
  los 4 huecos al final del viernes y el curso sale temprano ese día (RF-4, RF-5).
- **Curso de 34 h (fixture sin EF):** 17 bloques y **6 huecos**; margen de 3 bloques; **4 huecos al final
  del viernes y 2 al final de otro día** cuando hay dos materias de 6 h (el viernes ya cede 4 h a esas dos
  materias) (RF-4, RF-5).
- **Curso de 40 h (centro con EF, no es el fixture):** 20 bloques que ocupan 40 franjas, margen 0 y ningún
  hueco; lunes/miércoles/viernes con 2 o 3 bloques de capacidad libre para otras materias según las materias
  de 6 h (RF-4, RF-5).
- **Curso de 38 h (centro con EF, no es el fixture):** 19 bloques y **2 huecos** al final del viernes, todos
  los tramos pares (RF-4, RF-5).
- **Curso con 4 huecos:** los 4 al final del viernes (preferencia blanda); si la disponibilidad no lo
  permite, al final de otros días, con el viernes como preferencia (RF-4).
- **Curso con total impar:** no admite solución; causa (5) (RF-5, RF-10).
- **Curso de 34 h con dos materias de 6 h:** el viernes solo puede ceder 4 h (las dos materias de 6 h); los 6
  huecos se reparten 4 al final del viernes y 2 al final de otro día (RF-3, RF-4).
- **Bloque compartido y separación:** el bloque compartido cuenta como aparición para las dos materias;
  la otra aparición de cada una debe estar a ≥ 2 días (RF-2, RF-3).
- **Bloque compartido de dos restos con tipos de aula distintos:** imposible con la misma aula; el curso
  no es solución y la causa es (2) o no atribuible, **nunca (5)** (RF-9, RF-10).
- **Hueco en medio de un día:** la parte ocupada deja de ser un prefijo; **no es solución** (regla dura).
  Si no hay disposición con los huecos al final, la causa es (3)/(4) si su conteo aplica o **no
  atribuible** (RF-4, RF-10).
- **Tramo de 4 franjas:** son dos bloques válidos, **nunca dos de la misma materia** el mismo día (RF-1).
- **Misma materia en dos cursos distintos con días consecutivos:** permitido; la separación es solo del
  curso (RF-3).
- **Misma materia con el mismo docente en dos cursos en días consecutivos:** la regla no lo impide
  (RF-3).
- **Bloque que cruzaría un receso o un acto cívico:** no es bloque; son dos tramos (RF-1).
- **Bloque que cruzaría el final del turno o el cambio de día:** no es bloque (RF-1).
- **Materia que ocupa una franja de un bloque y otra franja suelta:** es una franja descubierta; si no es
  su resto, el curso no es solución (RF-2, RF-6).
- **Preferencia evitada que cubre la única franja o el único día que permitiría la regla:** la regla dura
  se mantiene y la preferencia se incumple; se informa (RF-8).
- **Falta de capacidad de un tipo de aula** (por ejemplo, un laboratorio con pocas franjas para la demanda
  del centro): no es un problema de bloques; la causa es la (2) de la 010 (RF-9, RF-10). La cancha **no**
  es este caso: se confirmó que admite **dos cursos a la vez** (96 ≤ 160).
- **Horario generado antes de esta spec:** no se reinterpreta; si se vuelve a generar, puede cambiar sin
  que cambien los datos ni la huella (RF-13).

## Fuera de alcance

- **Cambiar la hora académica, el tamaño de la franja o la conversión de horas a franjas.**
- **Cambiar las invariantes D1–D13, las prioridades S1 y S2 y las validaciones V1–V19.**
- **Cambiar el contrato `2.0`, el esquema, los presets o los horarios guardados.**
- **La interfaz de usuario:** la observabilidad es el resultado de la generación.
- **Configurar las reglas por centro, curso o materia.**
- **Ordenar las materias por otros criterios.**
- **La capacidad y el aforo de las aulas**, que siguen sin regla; el **aforo simultáneo por aula** es una
  **spec futura** (cancha × 2); incluye la dotación de canchas y laboratorios.
- **La disponibilidad del docente**, que no cambia.
- **La sustitución del conjunto de datos de prueba por los planes reales del centro:** es trabajo de otra
  spec; aquí solo se documenta que el escenario de verificación debe ser factible con estas reglas.
- **La generación parcial y los turnos automáticos.**

## Criterios de finalización

> Los criterios marcados **«fixture sin EF»** se verifican sobre el **fixture de aceptación**: los 24 cursos
> con los turnos, docentes y aulas confirmados y **Educación Física excluida** (marcada «no evaluada»),
> **34/36 h** y **848 clases**. **EF en la cancha no se usa como criterio** (el aforo simultáneo por aula
> es una spec futura); «EF en aula general» es una **prueba de sensibilidad opcional**. Cuando exista el
> aforo simultáneo, el fixture podrá volver a los planes completos (944 clases).

1. Un **bloque** se define y se verifica como dos franjas adyacentes ocupadas del mismo día y segmento;
   un evento, un hueco, el borde del turno y el cambio de día lo impiden (RF-1).
2. Todo tramo ocupado queda cubierto por bloques de dos horas; **no hay ningún bloque de 1 de clase**
   (RF-1, RF-5).
3. **Alineación:** ninguna materia tiene más franjas descubiertas que su resto; una materia de 3 h tiene
   un bloque simple y un resto compartido; una de 4 h, dos bloques simples; una de 5 h, 2+2+1; una de
   6 h, 2+2+2; **una de 11 h o más no es solución** (RF-2, RF-5).
4. **Separación (blanda):** en el **fixture sin EF** el incumplimiento **informado es ≤ el de la
   referencia** (no se exige 0); una materia de 6 h tiende a lunes/miércoles/viernes; una de **7–10 h** se
   informa como incumplimiento y una de **11 h o más no es solución** (causa (5)) (RF-3). *(Fixture sin
   EF.)*
5. **Huecos:** los **8 cursos de 34 h** tienen **6 huecos** y los **16 de 36 h**, **4**; **todos al final
   del día por regla dura** (ocupación monótona); el **viernes se informa** (preferencia blanda) y, en los
   cursos con dos materias de 6 h, el viernes solo cede 4 h, así que 2 de los 6 huecos van al final de otro
   día (RF-4). *(Fixture sin EF.)*
6. **Desvío:** los 24 cursos del fixture tienen **desvío 0**; 8 de 34 h con **17 bloques** y margen de **3
   bloques (6 h)**, y 16 de 36 h con **18 bloques** y margen de **2 bloques (4 h)** (RF-5, RF-7). *(Fixture
   sin EF.)*
7. **5º de 31059** se resuelve con **un bloque compartido** entre los restos de Castellano e Inglés, sin
   ningún bloque de 1; la otra aparición de cada materia se **prefiere** a ≥ 2 días y su separación se
   informa (blanda) (RF-2, RF-3). *(Fixture sin EF.)*
8. **Tope y causa:** ninguna materia supera `h mod 2` franjas descubiertas; si no se alcanza, el
   resultado es **no factible** con la causa **(5)** cuando se cumple una condición detectable y, si no,
   **no atribuible** (RF-7, RF-10).
9. El **reparto de docente por materia-curso** y el **exceso de horas por docente** son los de la
   **nueva generación de referencia** (la misma configuración sin preferencias, con las reglas de esta
   spec); una prueba comparativa lo comprueba (RF-7, RF-8).
10. El **orden blando S1 > S2 > separación > viernes > preferencia** está fijado, se comprueba con un caso
    en el que la preferencia y la separación compiten, y **S3 no aparece** (RF-8).
11. Un bloque usa la **misma aula** en sus dos franjas, simple y compartido, sin relajar la regla de tipo
    ni la exclusividad (RF-9).
12. El **contrato `2.0` y el esquema no cambian** y la **huella de datos** no cambia **respecto de la
    013** (RF-12, RNF-4, RNF-6).
13. El **resultado de la generación** informa bloques simples, compartidos, desvío, tope, franjas
    descubiertas, **el curso de cada materia**, sus apariciones con **día de dominio y franjas**, los
    **días incumplidos de separación** y los huecos, con **agregados por curso** (desvío/tope) (RF-12).
14. El escenario de referencia queda **por debajo de 300 s** (plazo del proceso de cálculo, 002); la
    medición se hace sobre el **fixture sin EF** (24 cursos, **848 clases**, 34/36 h). Referencia medida:
    **172,6 s** el centro completo con 8 workers, separación blanda y huecos al final duros, y **183,58 s**
    el **test lento** (`ctest --preset slow-release`) (RNF-2).
15. La **constitución §7 «Comportamiento» (causas (1)–(5))**, **§7.1.6** (forma dura; invariante 6 de §7.1), **§7.2**
    (**huecos al final duros**, **separación y viernes blandos**, S3 eliminada), **§7.3/§7.5** (eventos como
    segmentos, sin S3), la spec de agrupación (AC-8), la 010 (catálogo (1)–(5)), la 013 (sin S3 ni
    `PESO_AGRUPACION`) y la 005 (catálogo (1)–(5); «reproducible» → válido) quedan supersedidas y
    alineadas, sin migración (RF-13, RNF-6).
16. El resultado es **válido** (reglas duras y topes de la 014) con la búsqueda por defecto de **8
    workers**; **no** se exige que dos ejecuciones coincidan; los cálculos puros y los diagnósticos sí son
    deterministas (RNF-1).
17. Cada RF tiene al menos una **comprobación verificable** en las tareas (RNF-7).
18. El **seed sintético** cumple **RNF-8**: ninguna materia impar de laboratorio/taller sin pareja del
    mismo tipo (o todas pares); el perfil valida y las pruebas del seed pasan (B7). *(014b.)*

## Tamaño y división

La spec **se divide en dos entregas** (decisión del planner, 2026-11-06); no se recorta el alcance:

- **014a — motor:** funciones puras de la rejilla · restricciones duras (emparejamiento y alineación) ·
  separación y huecos al final (la 014a los implementó **blandos**; **B5b** mantiene la separación blanda y
  deja los **huecos al final duros**) · aula del
  bloque · causa (5) y diagnóstico · informe del resultado.
- **014b — cambio de requisitos y coherencia:** **B1/B1b/B2** workers 8, presupuestos 60/20/10 y plazo
  300 s · **B3** constitución/contrato/supersesiones · **B4** regresión · **B5b** **separación blanda** y
  **huecos al final duros**, causa (5) con 11 h · **B6** informe ampliado · **B7** seed a horas pares.

La 014b no rehace el motor: ajusta reglas, informe, datos y documentación. Las tareas se escriben en dos
lotes separados (ninguno supera el máximo de 10).

## Dudas abiertas

**Ninguna.** Las **quince decisiones** (D-1 … D-10 y N-1 … N-7, con D-1/N-1 y D-7/N-2 unificadas) quedaron
cerradas por el usuario el **2026-11-06** y están incorporadas a los RF, las definiciones, los casos
límite y los criterios:

| Decisión | Contenido |
|---|---|
| D-1 / N-1 | Regla **dura**: tope de franjas descubiertas `Σ(h mod 2)`; si no se alcanza, no factible con causa (5) |
| D-2 | Emparejamiento de la **rejilla del curso**; bloque compartido de dos restos válido |
| D-3 | Mismo día, turno y segmento; el evento rompe el bloque |
| D-4 | Cada materia en **bloques enteros** salvo su resto, que se comparte; un bloque por materia y día |
| D-5 | **S3 eliminada**; la agrupación se mide solo con el desvío nuevo |
| D-6 | **Orden blando** S1 > S2 > **separación** > **viernes** > preferencia; **la separación y el viernes son blandos; los huecos al final son duros** |
| D-7 / N-2 | **Misma aula** en las dos franjas del bloque, simple y compartido |
| D-8 | **Quinta causa** «(5) conflicto de bloques», con condiciones detectables |
| D-9 | **Todos** los cursos y materias, sin excepciones |
| D-10 | Contrato **`2.0` intacto**; el resultado informa bloques, desvío, días y huecos |
| N-3 | Huecos **al final del día**, en número par por día, **prioridad viernes**; sin clase suelta |
| N-4 | Un tramo de 4 franjas son **dos bloques válidos**, nunca dos de la misma materia |
| N-5 | **Separación blanda con prioridad alta** (actualizada el 2026-11-06) |
| N-6 | **Toda aparición cuenta**, incluido el resto |
| N-7 | La separación alcanza **solo al curso** |

> **Cambios posteriores (2026-11-06).** Los **huecos al final del día** pasan a **regla dura**; la
> **separación** vuelve a ser **blanda con prioridad alta** y el **viernes** sigue blando; la causa (5)
> cubre **11 h o más** (7–10 h se informa); el **informe** lleva **curso por materia**, apariciones con
> franjas y agregados por curso; el **seed sintético** pasa a **horas pares** (RNF-8). Se implementan en la
> **014b (B5b–B7)**.
