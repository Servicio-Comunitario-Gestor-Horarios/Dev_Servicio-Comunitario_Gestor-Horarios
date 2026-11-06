# Spec 014 - Materias en bloques de dos horas

Estado: borrador

> **Alcance.** Esta spec define **cómo se reparten las horas de cada curso en bloques de dos horas** y
> **cada cuántos días puede repetirse una materia**. En cada **(curso, día, segmento)** las franjas
> ocupadas se agrupan en **pares adyacentes**; un par puede ser **2 h de la misma materia** (**bloque
> simple**) o **1 h + 1 h de dos materias** (**bloque compartido**, que empareja los dos restos impares).
> Un bloque de una sola hora es un **bloque de 1** y es lo que se quiere evitar. Además, **dos
> apariciones de la misma materia en el mismo curso deben estar separadas por al menos un día
> intermedio**: ni el mismo día ni el día siguiente. Cubre la definición del bloque, la alineación de las
> materias, la separación, la cobertura y la paridad, la métrica, la jerarquía, el aula, el diagnóstico,
> el alcance y el informe.
>
> **Fuera de alcance:** la interfaz de usuario; el modelo persistido y el esquema; el contrato de
> configuración (salvo que D-10 decida lo contrario); la hora académica (sigue siendo **una franja**);
> las invariantes D1–D13, S1, S2 y las validaciones V1–V19; la generación parcial y los turnos
> automáticos.
>
> **Estado de la spec: borrador.** Decisiones cerradas (2026-11-06): **D-1/N-1** regla **dura**
> (desvío máximo por curso `Σ(h mod 2)`; si no se alcanza, no factible con causa) · **D-2** el
> emparejamiento es de la **rejilla del curso** y el **bloque compartido** de dos restos es válido ·
> **D-3** mismo día, turno y segmento (el evento lo rompe) · **D-4** cada materia ocupa **bloques
> enteros** salvo su resto, que se comparte · **N-2** el bloque compartido usa **la misma aula**.
> Quedan **once dudas abiertas**: D-5 a D-10 y N-3 a N-7. La redacción sigue la **propuesta** del planner
> («Propuesta de formulación»), que **no está aprobada**.
>
> **Orden de trabajo.** Después de la **013** (preferencias del docente, cerrada el 2026-11-06). La 013
> mantiene su **garantía estructural** (el reparto de docentes y el exceso de horas no cambian); aquí se
> decide el lugar del emparejamiento en la jerarquía (D-6).

## Contexto y objetivo

Hoy una materia de varias horas semanales se imparte en franjas sueltas y sin relación con sus vecinas:
la agrupación (**S3**) solo penaliza los grupos de una franja de una materia, con un peso pequeño
(`PESO_AGRUPACION`, 10), y no distingue un par de un grupo de tres; la preferencia horaria (013) pesa
más (25) y deshace los pares. El centro quiere que **todo bloque de clase sea de dos horas académicas** y
que **una materia no se vea dos días seguidos**.

**Los datos reales del centro (documento de datos del usuario).** Dos planes: **31059** (Media General,
1º–3º con **38 h** y 4º–5º con **40 h**) y **31060** (Ciencias y Tecnología, **40 h** en los cinco años).
**24 cursos** (15 de 31059 y 9 de 31060), **889 alumnos**, **12 cursos por turno**, **12 aulas de uso
general y 1 cancha**, **15 docentes por turno**. En todo el centro **solo hay dos materias impares**:
**Castellano 3** e **Inglés 3**, las dos en **5º de 31059**, y suman 40 (par). Las materias de **6 h**
son: Ciencias Naturales y GHC en **1º y 2º de 31059**; GHC e ITP en **1º y 2º de 31060**; ITP en **3º y
4º de 31060**. **14 de los 24 cursos** tienen al menos una materia de 6 h.

**Consecuencia 1: el emparejamiento es alcanzable.** Todos los cursos suman un número **par** de horas y
los segmentos son pares (2 y 6), así que **todos pueden quedar sin ningún bloque de 1**. En 5º el 3+3 se
resuelve 2+1 / 2+1 con **los dos restos compartiendo un bloque**.

**Consecuencia 2: la rejilla de un curso lleno ya está emparejada.** Un curso de 40 h ocupa todas las
franjas de su turno: los tramos miden 6 y 2, siempre par, y la rejilla se empareja **por construcción**,
sea cual sea el reparto por materias. La regla de rejilla sola no cambia el horario de esos 16 cursos. Lo
que cambia —y lo que pide el centro— es que **cada materia ocupe bloques enteros** (D-4) y que **no se
repita en días consecutivos** (regla nueva).

**Consecuencia 3: la separación de un día fuerza los días de las materias largas.** Con 5 días y bloques
de 2 h, dos apariciones de la misma materia separadas por al menos un día solo admiten las parejas
(lunes, miércoles), (lunes, jueves), (lunes, viernes), (martes, jueves), (martes, viernes) y (miércoles,
viernes). Por tanto:

| Horas de la materia | Apariciones | Días posibles |
|---|---|---|
| 1 h | 1 (bloque de 1) | cualquiera |
| 2 h | 1 (bloque simple) | cualquiera |
| 3 h | 2 (2 h + 1 h de resto) | 6 combinaciones |
| 4 h | 2 (dos bloques simples) | 6 combinaciones |
| 5 h | 3 (2+2+1) | **solo lunes/miércoles/viernes** |
| 6 h | 3 (2+2+2) | **solo lunes/miércoles/viernes** |
| 7 h o más | 4 o más | **imposible** en 5 días |

**Consecuencia 4: el margen de los cursos llenos es cero.** Con D-4 (un bloque por materia y día) y la
separación, un curso de 40 h reparte 8 h/día = **4 bloques/día**, y en la semana necesita **20 bloques en
20 huecos**. Los 16 cursos de 40 h tienen **margen 0**; los 8 de 38 h, **2 h**. En los cursos con dos
materias de 6 h, el lunes, el miércoles y el viernes quedan con **4 h libres** (dos bloques) y el martes
y el jueves con **8 h**; en los de una materia de 6 h, **6 h libres** los tres días forzados. La
construcción explícita de un reparto válido para los 24 cursos es posible (se comprobó por conteo y por
construcción), pero **sin margen**: cualquier restricción adicional (aulas, disponibilidad del docente)
puede volver infactible el curso.

**Por qué no basta con S3.** S3 cuenta grupos de 1 **de una materia**, no distingue un par de un grupo
de 3, no suma los grupos de 4 o más (deuda del motor) y pesa menos que la preferencia. Con el conjunto
sintético anterior (13 cursos llenos) se midieron 359 grupos de 1, 76 de 2 y 3 de 3, y la 013 midió una
penalización de 166. La rejilla está llena pero las **materias** no están en bloques: falta la alineación
y la separación.

## Usuarios

- **Usuario académico que genera horarios:** espera que cada clase dure dos horas académicas y que una
  materia no se repita al día siguiente, y entender cuándo no ha podido ser.
- **Jefe de estudios / coordinación:** espera que el bloque de dos horas sea un **bloque real** (dos
  franjas seguidas, sin recreo en medio, en la misma aula) y que los restos impares se compartan.
- **Usuario que mantiene docentes:** no debe tener que capturar ningún dato nuevo para que la regla
  funcione.
- **Equipo del solver:** necesita una definición única, medible y con pruebas por caso, y saber qué
  prioridad manda sobre cuál.
- **Equipo del frontend:** necesita saber si el contrato cambia (D-10) y de dónde sale el informe.
- **Equipo de desarrollo:** necesita que la constitución y las specs 005, 009 (agrupación), 010 y 013 no
  contradigan el comportamiento nuevo.

## Historias de usuario

- HU-1. Como usuario académico, quiero que toda clase se imparta en un bloque de dos horas académicas,
  para que la duración sea la que el liceo exige.
- HU-2. Como usuario académico, quiero que los restos impares de dos materias se junten en un bloque
  compartido en lugar de quedar como horas sueltas.
- HU-3. Como usuario académico, quiero que una misma materia no se vea dos días seguidos, para que el
  estudiante tenga tiempo de asimilarla.
- HU-4. Como usuario académico, quiero que estas reglas no me dejen sin horario cuando los datos lo
  permiten, y que si no lo permiten se me diga por qué.
- HU-5. Como usuario académico, quiero saber cuántos bloques, compartidos y bloques de 1 tiene el
  horario y en qué días cae cada materia, para decidir si cambio datos del centro o lo acepto.
- HU-6. Como jefe de estudios, quiero que el bloque no tenga un recreo en medio ni cambie de aula.
- HU-7. Como miembro del equipo del solver, quiero una métrica única y verificable, con al menos una
  prueba por caso límite.
- HU-8. Como responsable de los datos, quiero que la regla no exija capturar nada nuevo ni migrar la
  base, los presets o los horarios guardados.

## Definiciones

- **Hora académica y franja:** una hora académica es una **franja** (`bloques(horas) = horas`, spec 005).
  El tamaño real de la franja no cambia. **«Bloque de dos horas»** (o **bloque**) es la unidad de dos
  franjas de esta spec.
- **Rejilla del curso:** las franjas del turno declarado del curso, organizadas por **día** y
  **segmento contiguo**. Es la unidad del emparejamiento.
- **Segmento contiguo del turno:** las franjas de un turno divididas por sus **eventos** (receso, acto
  cívico). Un bloque nunca cruza un evento ni el final del turno (D-3).
- **Franja ocupada / libre:** ocupada = con una clase; libre = sin clase. Una franja libre separa tramos
  y no forma parte de ningún bloque.
- **Tramo ocupado:** secuencia máxima de franjas ocupadas consecutivas de un día y segmento.
- **Bloque de dos horas:** dos franjas adyacentes y ocupadas del mismo día y segmento. **Simple** si son
  de la misma materia-curso; **compartido** si son de dos materias distintas.
- **Bloque de 1:** franja ocupada de un tramo de longitud impar que no forma bloque de dos horas.
- **Aparición de una materia-curso:** cada bloque de dos horas **o** bloque de 1 en el que la materia
  participa. Una materia de 3 h tiene 2 apariciones (un bloque simple y su resto); una de 6 h, 3.
- **Separación:** dos apariciones de la misma materia-curso en el mismo curso están separadas por **al
  menos un día intermedio** (días con diferencia ≥ 2). Prohíbe el mismo día y el día siguiente.
- **Franja descubierta de una materia-curso:** franja suya que **no** forma parte de un bloque de dos
  horas suyo (está en un bloque de 1 o en un bloque compartido).
- **Alineación:** propiedad de una materia-curso por la que tiene **como máximo `h mod 2` franjas
  descubiertas**; su resto (si `h` es impar) es exactamente una. Un bloque compartido descubre una franja
  de cada una de sus dos materias.
- **Desvío de rejilla de un curso:** número de bloques de 1 de su rejilla (equivale al número de tramos
  de longitud impar).
- **Desvío por materia de un curso:** suma, sobre sus materias, de
  `máx(0, franjas descubiertas − h mod 2)`.
- **Desvío del curso:** desvío de rejilla + desvío por materia. **Desvío de la generación:** suma sobre
  los cursos.
- **Tope duro del desvío (N-1):** por curso, `desvío de rejilla ≤ Σ(h mod 2)`; por materia, `desvío por
  materia = 0` (alineación). El objetivo, dentro de esos topes, es **minimizar** el desvío (0 en el
  centro real).
- **Máximo de apariciones de una materia:** 3 por semana (h ≤ 6). Un plan que dé **7 h o más** a una
  materia no puede cumplir la separación en una semana de 5 días.
- **Día de la generación:** 0 = lunes … `num_dias − 1` (como la 013).
- **Generación de referencia:** la misma configuración **sin la sección de preferencias**, ya con las
  reglas de esta spec.
- **Jerarquía:** S1 (exceso de horas) > S2 (titulares) > **emparejamiento, alineación y separación**
  (duras) > preferencia (013) > S3, si se conserva.

## Propuesta de formulación (no aprobada)

> Resumen de la propuesta del planner. **No es normativa** mientras el estado sea `borrador`.

1. **Bloque** de la rejilla: dos franjas adyacentes ocupadas, simples o compartidas; un bloque de 1 solo
   aparece en un tramo impar.
2. **Alineación dura (D-4 A):** cada materia ocupa bloques enteros salvo su resto, y el resto comparte
   bloque con el de otra materia. Sin esto la regla es vacía en los cursos llenos.
3. **Separación dura (N-5 propuesta A):** dos apariciones de la misma materia en el mismo curso, con al
   menos un día intermedio; toda aparición cuenta (N-6 propuesta A); solo dentro del curso (N-7
   propuesta A).
4. **Tope duro (N-1):** desvío de rejilla `≤ Σ(h mod 2)` por curso y alineación por materia; si no se
   alcanza, **no factible con causa nueva**. El objetivo minimiza el desvío (0 en el centro real).
5. **Jerarquía:** S1 > S2 > reglas duras de bloque > preferencia > (S3) (D-6).
6. **Aula:** la misma en las dos franjas del bloque, simple y compartido (N-2 decidida; D-7 pendiente
   para el bloque simple).
7. **Sin datos nuevos y sin contrato nuevo:** el resultado informa bloques, compartidos, bloques de 1,
   desvío, días de cada materia y ubicación (D-10).
8. **Todos** los cursos y materias, sin excepciones (D-9).

## Requisitos funcionales

### RF-1. Bloque de dos horas y emparejamiento de la rejilla

- **EL SISTEMA considerará tramo ocupado de un curso a una secuencia máxima de franjas ocupadas
  consecutivas dentro de un mismo día y segmento.**
- **EL SISTEMA emparejará las franjas de cada tramo de dos en dos:** un tramo de longitud par queda
  cubierto por bloques de dos horas; un tramo de longitud impar produce `(L−1)/2` bloques de dos horas y
  **exactamente un bloque de 1**, cuya posición elige el sistema para favorecer la alineación.
- **EL SISTEMA considerará bloque simple al de dos franjas de la misma materia-curso y bloque compartido
  al de dos materias distintas.**
- **EL SISTEMA no considerará bloque a dos franjas separadas por un evento, por una franja libre, por el
  final de un turno o por un cambio de día:** son tramos distintos (D-3).
- **EL SISTEMA no contará como bloque compartido a una franja libre junto a una ocupada.**
- **EL SISTEMA mantendrá la identidad hora académica = franja** y no redimensionará la rejilla.

### RF-2. Alineación de las materias con los bloques (D-4 = A)

- **EL SISTEMA exigirá que cada materia-curso ocupe bloques enteros**, de modo que sus horas queden
  cubiertas por bloques simples, salvo su **resto**.
- **SI una materia-curso tiene horas pares, ENTONCES no tendrá ninguna franja descubierta; SI tiene horas
  impares, ENTONCES tendrá exactamente una** (su resto).
- **CUANDO una materia-curso tenga un resto, EL SISTEMA lo emparejará en un bloque compartido con el
  resto de otra materia del mismo curso, en el mismo día y segmento**; no lo emparejará con una franja
  libre ni lo dejará solo si hay otro resto disponible.
- **EL SISTEMA garantizará que un bloque compartido contiene exactamente dos restos** (uno de cada
  materia), nunca una materia entera más un resto.
- **EL SISTEMA no permitirá que una materia tenga más de un resto:** el número de franjas descubiertas
  legítimas es `h mod 2`.
- **SI una materia no puede alinearse (su resto no encuentra pareja o su disponibilidad no permite los
  bloques), ENTONCES el curso no tendrá solución** y se aplicará RF-10 (regla dura, N-1).

### RF-3. Separación de un día entre apariciones (regla nueva)

- **EL SISTEMA exigirá que dos apariciones cualesquiera de la misma materia-curso en el mismo curso estén
  separadas por al menos un día intermedio:** no el mismo día ni el día siguiente (diferencia de días
  ≥ 2).
- **EL SISTEMA contará como aparición todo bloque en el que la materia participa**, sea un bloque simple,
  un bloque compartido o un bloque de 1 (N-6).
- **EL SISTEMA aplicará la separación solo dentro del curso:** la misma materia en dos cursos distintos
  no se restringe entre sí (N-7).
- **CUANDO una materia-curso tenga 3 apariciones, EL SISTEMA solo podrá situarlas en lunes, miércoles y
  viernes** (la única combinación de tres días con diferencia ≥ 2 en una semana de cinco).
- **SI una materia-curso tuviera 4 o más apariciones (7 h o más), ENTONCES la separación no podrá
  cumplirse** y el curso no tendrá solución; el sistema lo informará con la causa de RF-10.
- **EL SISTEMA no permitirá dos apariciones de la misma materia el mismo día** (consecuencia directa de
  la separación), aunque tuvieran bloques distintos.
- **[NECESITA ACLARACIÓN: N-5, N-6, N-7]** La dureza de la separación, si el bloque de 1 cuenta y si
  alcanza solo al curso están en N-5, N-6 y N-7.

### RF-4. Cobertura, paridad y cota

- **EL SISTEMA cubrirá todas las franjas ocupadas de la rejilla del curso con bloques**, sin solapes y
  sin dejar franjas sin clasificar.
- **SI el curso suma un número par de horas, ENTONCES el desvío de rejilla 0 es alcanzable** (las franjas
  libres se sitúan de modo que cada tramo quede de longitud par); **SI suma un número impar, ENTONCES al
  menos un bloque de 1 es inevitable**.
- **CUANDO el curso tenga franjas libres, EL SISTEMA las situará de modo que cada tramo quede de longitud
  par**, siempre que la disponibilidad lo permita.
- **EL SISTEMA contará el desvío de rejilla y el desvío por materia de cada curso**, y los comparará con
  la cota `Σ(h mod 2)`.
- **EL SISTEMA no declarará desvío 0 si existe un bloque de 1 o una franja descubierta en exceso.**
- **[NECESITA ACLARACIÓN: N-3]** Qué se hace si la disponibilidad impide la paridad en un curso sin
  restos está en N-3.

### RF-5. Métrica del desvío

- **EL SISTEMA medirá, por curso, el desvío de rejilla** (bloques de 1) **y el desvío por materia**
  (franjas descubiertas en exceso).
- **EL SISTEMA expresará el desvío en unidades enteras** (un bloque de 1 = una unidad; una franja
  descubierta en exceso = una unidad) y lo sumará sobre todos los cursos.
- **EL SISTEMA no contará dos veces la misma franja:** un bloque compartido legítimo (dos restos) no es
  desvío; un bloque de 1 sí.
- **CUANDO todos los cursos tengan desvío 0 —sin bloques de 1 y con todas las materias alineadas—, EL
  SISTEMA informará desvío 0.**
- **EL SISTEMA minimizará el desvío dentro de los topes duros de RF-6**, de modo que en el centro real el
  resultado sea 0 y no solo «≤ Σ(h mod 2)».

### RF-6. Naturaleza de las reglas y garantía

- **EL SISTEMA tratará el emparejamiento, la alineación y la separación como reglas duras:** un horario
  que las incumpla **no es solución** (D-1/N-1, D-4, N-5).
- **EL SISTEMA exigirá por curso `desvío de rejilla ≤ Σ(h mod 2)` y por materia `desvío por materia = 0`**;
  si no puede, declarará el problema no factible con la causa de RF-10.
- **EL SISTEMA minimizará el desvío dentro de esos topes**, con la técnica que decida el plan; no exigirá
  un peso por unidad.
- **EL SISTEMA mantendrá la garantía estructural de la 013:** con las mismas preferencias, el reparto de
  docente por materia-curso y el exceso de horas por docente son los de la generación de referencia.
- **EL SISTEMA no cambiará qué docente dicta cada materia-curso ni el exceso de horas de cada docente
  para mejorar el emparejamiento, la alineación o la separación.**
- **EL SISTEMA no relajará ninguna de las tres reglas en silencio.**

### RF-7. Relación con la agrupación (S3)

- **EL SISTEMA medirá la agrupación con la métrica de esta spec** (bloques de 1 y franjas descubiertas en
  exceso) y **no** con la de la 009 (grupos de 1 por materia; 2–3 sin penalizar; 4 o más sin sumar en el
  motor).
- **EL SISTEMA no sumará dos veces la misma franja.**
- **EL SISTEMA anotará la supersesión de la métrica de S3 en la spec de agrupación**, que sigue siendo la
  fuente de la noción de segmento contiguo.
- **[NECESITA ACLARACIÓN: D-5]** Si S3 se subsume, se mantiene aparte o se elimina está en D-5.

### RF-8. Relación con la preferencia horaria y jerarquía

- **EL SISTEMA aplicará la jerarquía S1 > S2 > reglas duras de bloque (emparejamiento, alineación,
  separación) > preferencia horaria > (S3 si se conserva).**
- **CUANDO respetar una preferencia horaria obligue a romper un bloque o una separación, EL SISTEMA
  mantendrá la regla dura** y la preferencia se incumplirá; el resultado informará el cumplimiento como
  hoy (RF-8 de la 013).
- **EL SISTEMA podrá situar clases en franjas evitadas para cumplir las reglas duras**, y el usuario verá
  el coste en el informe de preferencias.
- **EL SISTEMA no usará las reglas duras para cambiar de titular, de suplente ni de horas de un docente.**
- **CUANDO la generación no tenga preferencias intervinientes, EL SISTEMA aplicará igualmente las reglas
  duras** y el horario podrá diferir del de la 013.
- **[NECESITA ACLARACIÓN: D-6]** El lugar del emparejamiento frente a la preferencia (por encima o por
  debajo) está en D-6.

### RF-9. Aula durante el bloque

- **MIENTRAS el modo de aula fija esté activo, EL SISTEMA impartirá las dos franjas de un bloque simple
  en la misma aula:** el aula de referencia si su tipo coincide con el requerido, o la misma aula del
  tipo requerido si la clase se desplaza.
- **MIENTRAS el modo de aula fija esté inactivo, EL SISTEMA impartirá las dos franjas de un bloque simple
  en la misma aula elegida**, libre en ambas franjas.
- **CUANDO el bloque sea compartido, EL SISTEMA lo impartirá en la misma aula en sus dos franjas** (N-2),
  de modo que solo podrá juntar dos restos de materias que requieran el **mismo tipo** de aula.
- **EL SISTEMA mantendrá intactas las reglas de tipo exacto, de exclusividad de aula y de referencia
  desplazada** de la 010.
- **SI la misma aula no puede sostener las dos franjas del bloque, ENTONCES EL SISTEMA no formará el
  bloque** y se aplicará RF-4/RF-6 (desvío o no factible).
- **[NECESITA ACLARACIÓN: D-7]** La misma aula en el bloque **simple** está en D-7 (la del compartido ya
  está decidida en N-2).

### RF-10. Factibilidad y diagnóstico

- **SI el centro no puede cumplir el emparejamiento, la alineación o la separación, ENTONCES EL SISTEMA
  declarará el problema no factible con una causa propia** que nombre el curso, la materia, el día o el
  segmento y el motivo concreto, sin sustituir ni reordenar las categorías existentes (N-1).
- **EL SISTEMA no relajará la regla en silencio** ni la convertirá de dura en blanda sin informarlo.
- **EL SISTEMA no atribuirá a estas reglas una infactibilidad que venga de una falta de aulas o de
  capacidad:** esos casos siguen siendo la causa (2) de la 010.
- **[NECESITA ACLARACIÓN: D-8]** La forma y el orden de la causa nueva (quinta categoría o dentro de las
  existentes) y su relación con la separación están en D-8.

### RF-11. Alcance y excepciones

- **EL SISTEMA aplicará las reglas a todas las materias-curso de todos los cursos**, sin excepciones por
  área, tipo de aula, taller, educación física ni docente.
- **SI una materia tuviera 1 hora semanal, ENTONCES EL SISTEMA la tratará como resto** y la emparejará en
  un bloque compartido con otro resto; si no hubiera otro, será un bloque de 1.
- **EL SISTEMA no requerirá ningún dato nuevo del centro.**
- **SI un plan diera 7 h o más a una materia, ENTONCES la separación no podrá cumplirse** y el sistema lo
  informará con la causa de RF-10 (límite estructural de la regla, no un defecto del dato).
- **[NECESITA ACLARACIÓN: D-9]** Las excepciones (cancha, talleres) y su coste están en D-9.

### RF-12. Observabilidad y contrato

- **CUANDO termine una generación, EL SISTEMA informará, por curso:** número de bloques de dos horas
  (simples y compartidos), número de bloques de 1, desvío de rejilla, desvío por materia y la ubicación
  de los bloques de 1 y de los compartidos (día y franjas).
- **EL SISTEMA informará, por materia-curso:** sus apariciones con su día y sus franjas, su resto y sus
  franjas descubiertas, para que el usuario vea si la separación se cumple y qué materia quedó
  fragmentada.
- **EL SISTEMA informará los totales de la generación** (bloques, compartidos, bloques de 1, desvío y
  tope `Σ(h mod 2)`).
- **EL SISTEMA calculará el informe sobre el horario devuelto**, sin cambiar el contrato de salida del
  solver.
- **SI el desvío es mayor que 0 o que la cota, ENTONCES EL SISTEMA lo informará como bloques incompletos**
  con la diferencia y los cursos afectados.
- **MIENTRAS no se decida otra cosa (D-10), EL SISTEMA mantendrá el contrato en `2.0`, sin secciones ni
  campos nuevos**, y la huella de datos no cambiará.
- **[NECESITA ACLARACIÓN: D-10]** La configurabilidad y la forma exacta del informe están en D-10.

### RF-13. Coherencia normativa y documentación

- **EL SISTEMA mantendrá alineados la constitución del proyecto §7.1.2 y §7.2:** el primero sigue
  exigiendo que cada materia reciba exactamente sus horas en franjas y añade la forma del reparto y la
  separación; el segundo lista las reglas duras en su lugar.
- **EL SISTEMA anotará las supersesiones:** la **009** (agrupación) en su métrica de S3; la **013** en la
  jerarquía (si D-6 la cambia) y en su RNF-4/criterio de no regresión, porque el horario **sin**
  preferencias cambia; la **005** en la lectura de D3 (las horas siguen siendo franjas y se añade cómo se
  reparten y cada cuándo se repiten).
- **EL SISTEMA dejará constancia de que no hay migración** de bases, presets ni horarios guardados, y de
  que los horarios anteriores no se reinterpretan.
- **EL SISTEMA documentará el bloque simple, el compartido, el bloque de 1, la alineación, la separación,
  la cota y el límite de 6 h por materia.**

## Requisitos no funcionales

- **RNF-1. Determinismo y reproducibilidad:** con la misma configuración y el mismo motor, dos
  generaciones producen el mismo horario y el mismo desvío; el orden de las materias y de los días no
  altera el resultado.
- **RNF-2. Desempeño:** el escenario de referencia se resuelve por debajo del plazo vigente de **60 s**;
  el plan mide y documenta el margen. Las reglas duras nuevas estrechan el problema y pueden acercarlo al
  límite: el plan debe medir el escenario del centro real (24 cursos, 944 clases) y no solo el sintético.
- **RNF-3. Mensajes en español:** todo informe identifica curso, materia, bloques, compartidos, bloques
  de 1, desvío, tope y días de cada materia; todo rechazo (si lo hubiera) identifica el valor recibido y
  la acción disponible.
- **RNF-4. Sin regresión en lo ajeno:** las invariantes D1–D13, S1, S2, las validaciones V1–V19, la regla
  de tipo y referencia de aula y el contrato `2.0` no cambian; la huella de datos tampoco.
- **RNF-5. Verificación sin la interfaz:** las reglas se prueban con la lógica del servidor y sus
  pruebas, sin la interfaz.
- **RNF-6. Protección de datos:** no se modifica ni migra ningún dato persistido, preset ni horario
  guardado; no hay cambio de esquema.
- **RNF-7. Trazabilidad:** cada RF tendrá al menos una comprobación verificable en las tareas.

## Casos límite

- **Materia de 1 h:** una aparición (bloque de 1); puede compartir bloque con otro resto o quedar suelta
  (RF-2, RF-11).
- **Materia de 2 h:** una aparición (un bloque simple); cualquier día (RF-2, RF-3).
- **Materia de 3 h:** dos apariciones (un bloque simple y su resto); seis combinaciones de días; el resto
  comparte bloque con otro resto en su mismo día (RF-2, RF-3).
- **Materia de 4 h:** dos apariciones (dos bloques simples); seis combinaciones de días; nunca dos el
  mismo día (RF-2, RF-3).
- **Materia de 5 h:** tres apariciones (2+2+1) → **solo lunes/miércoles/viernes**, con el resto en uno de
  esos días compartiendo bloque (RF-3).
- **Materia de 6 h:** tres apariciones (2+2+2) → **solo lunes/miércoles/viernes** (RF-3).
- **Materia de 7 h o más:** cuatro o más apariciones → la separación **no puede cumplirse**; no factible
  con causa (RF-3, RF-11).
- **Curso de 40 h:** 20 bloques en 20 huecos, margen 0; lunes/miércoles/viernes con 2 o 3 bloques libres
  según las materias de 6 h (RF-4).
- **Curso de 38 h con 2 franjas libres:** si caen en el mismo segmento-día, todos los tramos quedan
  pares; si caen en segmentos distintos, aparecen dos bloques de 1 y, sin restos con los que compartir,
  el curso no se puede alinear → no factible (RF-4, N-3).
- **Curso de 38 h con una materia de 6 h:** lunes/miércoles/viernes con 6 h libres cada uno y martes y
  jueves con 8 h; el margen de 2 h debe caer en un segmento par (RF-4).
- **Bloque compartido y separación:** el bloque compartido cuenta como aparición para las dos materias;
  la otra aparición de cada una debe estar a ≥ 2 días (RF-2, RF-3).
- **Bloque compartido de dos restos con tipos de aula distintos:** imposible con la misma aula (N-2); las
  dos materias quedan sin compartir y el curso no se puede alinear → no factible, salvo que N-2 se revise
  (RF-9).
- **Misma materia en dos cursos distintos con días consecutivos:** permitido; la separación es solo
  dentro del curso (RF-3, N-7).
- **Misma materia con el mismo docente en dos cursos en días consecutivos:** la regla no lo impide (N-7);
  si se quisiera, sería una restricción del docente, no del curso.
- **Bloque que cruzaría un receso o un acto cívico:** no es bloque; son dos tramos (RF-1).
- **Bloque que cruzaría el final del turno o el cambio de día:** no es bloque (RF-1).
- **Franja libre entre dos ocupadas:** separa dos tramos; los tramos impares producen bloques de 1 (RF-1).
- **Tramo de 3 franjas:** un bloque de 2 y un bloque de 1; la posición del bloque de 1 se elige para
  alinear el resto de una materia (RF-1, RF-2).
- **Materia que ocupa una franja de un bloque y otra franja suelta:** es una franja descubierta; si no es
  su resto, suma desvío por materia (RF-5).
- **Preferencia evitada que cubre la única franja o el único día que permitiría la regla:** la regla dura
  se mantiene y la preferencia se incumple; se informa (RF-8).
- **Falta de capacidad de un tipo de aula** (por ejemplo, una sola cancha para la educación física de todo
  el centro): no es un problema de bloques; la causa es la (2) de la 010 (RF-10).
- **Horario generado antes de esta spec:** no se reinterpreta; si se vuelve a generar, puede cambiar sin
  que cambien los datos ni la huella (RF-13).

## Fuera de alcance

- **Cambiar la hora académica, el tamaño de la franja o la conversión de horas a franjas.**
- **Cambiar las invariantes D1–D13, las prioridades S1 y S2 y las validaciones V1–V19.**
- **Cambiar el contrato `2.0`, el esquema, los presets o los horarios guardados** (salvo D-10).
- **La interfaz de usuario:** la observabilidad es el resultado de la generación.
- **Configurar las reglas por centro, curso o materia** (salvo D-9 o D-10).
- **Ordenar las materias por otros criterios.**
- **La capacidad y el aforo de las aulas**, que siguen sin regla; incluye la dotación de canchas y
  laboratorios.
- **La disponibilidad del docente**, que no cambia.
- **La sustitución del conjunto de datos de prueba por los planes reales del centro:** es trabajo de otra
  spec; aquí solo se documenta que el escenario de verificación debe ser factible con estas reglas.
- **La generación parcial y los turnos automáticos.**

## Criterios de finalización

1. Un **bloque** se define y se verifica como dos franjas adyacentes ocupadas del mismo día y segmento;
   un evento, una franja libre, el borde del turno y el cambio de día lo impiden (RF-1).
2. Todo tramo ocupado queda cubierto por bloques; un tramo impar produce **exactamente un** bloque de 1
   (RF-1).
3. **Alineación:** ninguna materia tiene más franjas descubiertas que su resto; una materia de 3 h tiene
   un bloque simple y un resto compartido; una de 4 h, dos bloques simples; una de 5 h, 2+2+1; una de
   6 h, 2+2+2 (RF-2).
4. **Separación:** dos apariciones de la misma materia en un curso distan al menos 2 días; una materia de
   5 o 6 h cae exactamente en lunes/miércoles/viernes; una de 7 h o más no es factible (RF-3).
5. **Centro real:** los 24 cursos admiten un reparto con **desvío 0**; 16 con margen 0 y 8 con margen de
   2 h (RF-4, RF-6).
6. **5º de 31059** se resuelve con **un bloque compartido** entre los restos de Castellano e Inglés, sin
   ningún bloque de 1, y el bloque compartido cae a ≥ 2 días de los bloques de 2 h de cada materia
   (RF-2, RF-3).
7. **Tope duro:** ningún curso supera `Σ(h mod 2)` bloques de 1 y ninguna materia tiene desvío por
   materia; si no se alcanza, el resultado es **no factible con la causa nueva** (RF-6, RF-10).
8. El **reparto de docente por materia-curso** y el **exceso de horas por docente** son los de la
   generación de referencia de la 013; una prueba comparativa lo comprueba (RF-6, RF-8).
9. La jerarquía **S1 > S2 > reglas duras > preferencia > (S3)** está fijada y se comprueba con un caso en
   el que la preferencia y una regla dura compiten (RF-8).
10. Un bloque usa la **misma aula** en sus dos franjas, simple y compartido (D-7 y N-2), sin relajar la
    regla de tipo ni la exclusividad (RF-9).
11. El **contrato `2.0` y el esquema no cambian** y la **huella de datos** tampoco (RF-12, RNF-4, RNF-6).
12. El **resultado de la generación** informa bloques simples, compartidos, bloques de 1, desvío, tope,
    días de cada materia y ubicación, por curso y por materia (RF-12).
13. El escenario de referencia queda **por debajo de 60 s**, medido también con el centro real (RNF-2).
14. La **constitución §7.1.2/§7.2**, la spec de agrupación y la 013 quedan alineadas, con las
    supersesiones anotadas, y no hay migración (RF-13, RNF-6).
15. Dos generaciones con la misma configuración dan el **mismo horario y el mismo desvío** (RNF-1).
16. Cada RF tiene al menos una **comprobación verificable** en las tareas (RNF-7).

## Dudas abiertas

### Decisiones cerradas (2026-11-06)

- **D-1 → Dura, confirmada por N-1.** El emparejamiento, la alineación y la separación son reglas duras; el
  desvío se minimiza solo dentro de los topes.
- **D-2 → El emparejamiento es de la rejilla del curso**; el **bloque compartido** de dos restos es válido.
- **D-3 → A.** Mismo día, turno y segmento; el evento lo rompe.
- **D-4 → A.** Cada materia ocupa **bloques enteros** (4 h → 2+2; 5 h → 2+2+1) y su resto comparte bloque
  con el de otra materia impar; **un bloque por materia y día** (ahora exigido también por la separación).
- **N-1 → Dura.** Desvío máximo por curso `Σ(h mod 2)`; si no se alcanza, **no factible con causa nueva**.
- **N-2 → Misma aula.** Las dos horas del bloque compartido, en la misma aula (solo se juntan restos del
  mismo tipo).

### Dudas abiertas (once)

#### D-5. ¿Qué pasa con S3, la agrupación?

- **A) (recomendada) S3 queda subsumida.** La agrupación se mide con el desvío de esta spec; la 009 se
  anota superada en su métrica. *Consecuencias:* una sola noción; desaparece el peso 10 como árbitro.
- **B) S3 se mantiene y se añade aparte.** *Consecuencias:* doble conteo del bloque de 1; más constantes.
- **C) Se elimina S3.** *Consecuencias:* la 009 queda superada entera; se conserva su noción de segmento.

#### D-6. ¿Las reglas duras mandan sobre la preferencia horaria?

- **A) (recomendada) Sí:** S1 > S2 > reglas duras > preferencia. La preferencia solo elige dónde caen los
  bloques ya formados; el cumplimiento se informa como hoy. *Consecuencias:* algunas preferencias que hoy
  se respetan pasarán a incumplirse; con las reglas duras ya no hay margen para preferirlas.
- **B) No:** la preferencia queda por encima y las reglas duras podrían romperse. *Incompatible con que
  sean duras*, salvo que se degraden a blandas.
- **C) Preferencia solo como desempate final.** Equivale a A con menos efecto; el cumplimiento se
  informa.

#### D-7. ¿El bloque simple usa la misma aula en sus dos franjas?

- **A) (recomendada) Sí**, igual que el compartido (N-2). *Consecuencias:* el bloque es real; en modo de
  aula fija, un bloque de materia especializada ocupa el aula especializada dos franjas y la referencia
  queda vacía y bloqueada las dos (regla de la 010); con un solo laboratorio, exige tenerlo libre dos
  franjas seguidas.
- **B) No: puede cambiar de aula.** *Consecuencias:* más libertad con aulas escasas; deja de ser un
  bloque para los estudiantes.
- **C) Solo en modo de aula fija.** *Consecuencias:* híbrido; más difícil de explicar y verificar.

#### D-8. ¿Cómo se declara la causa de no factible?

*(la existencia de la causa está decidida en N-1; queda su forma y su orden)*

- **A) (recomendada) Quinta categoría «(5) conflicto de bloques y separación»**, después de las cuatro
  actuales, que nombre el curso, la materia, el día o el segmento y el motivo (resto sin pareja, materia
  de 7 h o más, día forzado sin disponibilidad). *Consecuencias:* la 010 y la constitución §7.4/§7.5
  recogen una quinta categoría y su orden; el usuario recibe un diagnóstico útil.
- **B) Dentro de la (3) conflicto de persona**, como un motivo más del curso. *Consecuencias:* no cambia
  el catálogo; el diagnóstico queda menos específico.
- **C) Como causa «no atribuible».** *Consecuencias:* no se nombra la entidad; el usuario no sabe qué
  corregir.
- **Pregunta adicional:** ¿la causa nueva cubre también la separación (N-5) o solo el emparejamiento y la
  alineación?

#### D-9. ¿A qué cursos y materias se aplica?

- **A) (recomendada) A todos los cursos y a todas las materias**, incluidas las de 1 h y las de aulas
  especializadas; sin excepciones.
- **B) Con excepciones por materia** (educación física en la cancha, talleres). *Consecuencias:* exige un
  dato nuevo en el modelo o en el contrato (esquema y presets).
- **C) Excluir los cursos sin margen.** *Consecuencias:* dejaría fuera los 16 cursos de 40 h; absurdo
  para el centro.

#### D-10. ¿Las reglas se configuran o solo se informan? ¿Cambia el contrato?

- **A) (recomendada)** El **contrato no cambia** (`2.0`, sin secciones ni campos nuevos): las reglas son
  del motor. El **resultado de la generación** informa bloques, compartidos, bloques de 1, desvío, tope,
  días de cada materia y ubicación; la salida del solver no cambia.
- **B) Solo exigir que el horario cumpla, sin informe.** *Consecuencias:* no se puede comprobar ni
  explicar por qué un curso no genera.
- **C) Llevar las reglas al contrato** (campos o secciones, por ejemplo «bloques: 2», «días entre
  clases»). *Consecuencias:* versión nueva del contrato, presets y frontend; nadie lo ha pedido.

#### N-3. Paridad por segmento: ¿qué se hace si la disponibilidad impide la paridad?

- **A) (recomendada) Exigirla:** con D-4 y N-1 duras, un curso sin restos no admite bloques de 1; si no
  se logra la paridad, **no factible con la causa de D-8**. En los 8 cursos de 38 h basta con poner las 2
  franjas libres juntas.
- **B) Permitir el bloque de 1 en ese curso**, relajando la alineación solo ahí e informándolo.
  *Consecuencias:* contradice D-4 A y N-1 para ese curso; requiere una excepción documentada.
- **C) Blanda.** *Consecuencias:* incompatible con que las reglas sean duras.

#### N-4. ¿Un tramo de 4 franjas consecutivas es válido?

*(reducida por la separación: 4 h de la misma materia en un día ya son imposibles)*

- **A) (recomendada) Sí:** son dos bloques de 2 h adyacentes, normalmente de materias distintas; la
  rejilla no distingue y no hay nada que penalizar.
- **B) Se penaliza el tramo de 4** por cualquier motivo (por ejemplo, para forzar un cambio de materia a
  media mañana). *Consecuencias:* restricción nueva sin petición; con margen 0 puede volver infactible.
- **C) Se permite solo si no hay alternativa.** *Consecuencias:* intermedio; más difícil de explicar.

#### N-5. ¿La separación de un día es dura o blanda?

- **A) (recomendada) Dura**, con no factibilidad declarada si no se logra. *Consecuencias:* es la lectura
  literal de la petición y es coherente con N-1; con los planes reales es factible (todos los cursos
  caben, con margen 0 o 2 h), pero **el conjunto sintético de pruebas (13 cursos, cuatro materias de 5 h
  en 1º–3º) se vuelve infactible** si el resto cuenta (N-6 = A): el escenario de verificación debe pasar
  a los planes reales o ajustarse.
- **B) Blanda con prioridad alta**, por encima del emparejamiento. *Consecuencias:* nunca se pierde un
  horario; el «no se puede ver dos días seguidos» no se garantiza.
- **C) Dura solo si el curso tiene margen.** *Consecuencias:* dos reglas según el curso; difícil de
  explicar y de verificar.

#### N-6. ¿El bloque de 1 (resto impar) cuenta como aparición para la separación?

- **A) (recomendada) Sí, cualquier clase cuenta.** Es la lectura literal («una clase de la misma
  materia»). *Consecuencias:* una materia de 5 h (2+2+1) tiene 3 apariciones y queda forzada a
  lunes/miércoles/viernes; el conjunto sintético de pruebas (cuatro materias de 5 h en 1º–3º) no cabe en
  8 h/día y es infactible; en los planes reales no hay materias de 5 h, así que no afecta.
- **B) Solo cuentan los bloques de 2 h**; el resto puede caer el día siguiente. *Consecuencias:* más
  libertad; una materia de 5 h podría ser 2 h el lunes, 2 h el miércoles y 1 h el martes. El conjunto
  sintético seguiría siendo factible. Menos fiel a la petición.

#### N-7. ¿La separación alcanza solo al curso o también al docente?

- **A) (recomendada) Solo dentro del curso:** la misma materia en dos cursos distintos puede caer en días
  consecutivos; la regla es lo que ve el estudiante.
- **B) También el docente:** si el mismo docente dicta la misma materia en dos cursos, no podría hacerlo
  en días consecutivos. *Consecuencias:* mucho más restrictivo (GHC e ITP se dictan a varias secciones
  con los mismos docentes); puede volver infactible el centro real.
- **C) Las dos cosas:** máxima coherencia pedagógica, máximo riesgo.

### Trazabilidad de las dudas a los requisitos

| Duda | RF afectado | Qué decide |
|---|---|---|
| D-5 | RF-7 | Subsunción, continuidad o eliminación de S3 |
| D-6 | RF-8 | Lugar de la preferencia frente a las reglas duras |
| D-7 | RF-9 | Misma aula en el bloque simple |
| D-8 | RF-10 | Forma y orden de la causa nueva |
| D-9 | RF-11 | Alcance y excepciones |
| D-10 | RF-12 | Contrato y observabilidad |
| N-3 | RF-4 | Paridad cuando la disponibilidad no la permite |
| N-4 | RF-1 | Tramos de 4 franjas |
| N-5 | RF-3 | Dureza de la separación |
| N-6 | RF-3 | Si el bloque de 1 cuenta como aparición |
| N-7 | RF-3 | Alcance de la separación (curso o docente) |
