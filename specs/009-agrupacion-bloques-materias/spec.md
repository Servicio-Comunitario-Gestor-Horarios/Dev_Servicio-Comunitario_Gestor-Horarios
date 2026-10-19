# Spec 009 - Agrupación de bloques contiguos por materia-curso

Estado: aprobada (2026-10-18)

> Esta spec añade una **restricción blanda nueva (S3)** al solver para favorecer que las horas de cada materia-curso se impartan en **grupos de 2–3 franjas contiguas** dentro del mismo día, en vez de dispersarse. No toca restricciones duras D1–D13 ni las prioridades blandas actuales S1/S2.

## Contexto y problema

Hoy el solver optimiza solo dos prioridades blandas:
- **S1** — minimizar exceso de horas de aula (`PESO_HORAS = 1000`)
- **S2** — preferir titulares antes que suplentes (`COSTE_SUPLENTE = 100`)

Con los datos de prueba (Plan 31059, variante holgada: 13 cursos × 40 h académicas = 520 clases; 5 días; 16 franjas/día en dos turnos de 8 franjas de 40 min cada uno; receso de 10 min tras la franja 2 de cada turno), **cada curso llena las 8 franjas de su turno todos los días**. Eso significa que **no hay huecos** dentro del turno de un curso: la «agrupación» no consiste en compactar días, sino en **ordenar las materias dentro del día** para que sus bloques queden juntos.

Actualmente, una materia de 4 h/semana puede quedar repartida como `M _ _ M _ M _ M` (dispersa). Lo pedagógicamente deseable es `M M _ _ M M _ _` o `M M M _ M _ _ _` (grupos de 2–3 contiguos).

## Objetivo

Añadir una **prioridad blanda S3** que penalice la fragmentación de cada materia-curso dentro de un día, favoreciendo **arranques de grupos de tamaño 2–3** en franjas consecutivas del mismo día, sin violar D1–D13 ni empeorar S1/S2.

## Definiciones

- **Materia-curso**: combinación (materia, curso) con sus horas semanales (provienen del plan del curso).
- **Franja (slot)**: intervalo dentro de un día. Todas las franjas tienen la misma duración real (`duracion_minutos`).
- **Segmento contiguo del turno**: los índices de franja de un turno se dividen en **segmentos contiguos** por el receso. En los datos de prueba:
  - Turno Mañana (franjas 0–7): receso tras la franja 2 (índice 1) → segmentos **{0,1}** y **{2,3,4,5,6,7}**.
  - Turno Tarde (franjas 8–15): receso tras la franja 2 (índice 9) → segmentos **{8,9}** y **{10,11,12,13,14,15}**.
  Un grupo contiguo **nunca cruza** el receso ni el límite de segmento.
- **Grupo contiguo**: secuencia de **franjas consecutivas** (índices `s, s+1, s+2, …`) dentro de un **mismo segmento contiguo del turno**, del **mismo día**, asignadas a la **misma materia-curso**.
- **Arranque de grupo**: una franja `s` en la que la materia-curso tiene clase y **no** la tiene en `s-1` (o `s` es la primera franja de su segmento ese día).
- **Tamaño del grupo**: número de franjas consecutivas desde un arranque hasta el siguiente hueco o fin del segmento.
- **Horas del día para una materia-curso**: número total de franjas asignadas a esa materia-curso en ese día (suma sobre todos los segmentos del turno).

## Requisitos funcionales (EARS)

### RF-1. Definición de grupo contiguo y arranque (con receso como corte)

**CUANDO** se evalúe la nueva prioridad blanda, **EL SISTEMA** considerará un **grupo contiguo** de una materia-curso en un día como una secuencia máxima de franjas consecutivas (índices `s, s+1, …`) dentro de un **mismo segmento contiguo del turno declarado del curso**, asignadas a esa materia-curso. Un **arranque** es una franja `s` con clase de esa materia-curso tal que `s-1` no tiene clase de esa materia-curso **o `s` es la primera franja de su segmento ese día**.

> El receso de 10 min tras la franja 2 de cada turno **rompe la contigüidad** (decisión D-1 → B). Dos franjas a ambos lados del receso **nunca** forman parte del mismo grupo contiguo.

### RF-2. Tamaño objetivo 2–3 y penalización exacta

**SI** una materia-curso tiene un arranque en un día, **ENTONCES EL SISTEMA** penalizará según el tamaño del grupo resultante:

| Tamaño del grupo | Penalización (en unidades de `PESO_AGRUPACION`) |
|------------------|-------------------------------------------------|
| **1**            | **1** (grupo aislado: es lo que se quiere evitar). |
| **2 o 3**        | **0** (objetivo cumplido). |
| **≥ 4**          | **0 en el motor** (no se penaliza para no encarecer la búsqueda); el validador sí lo reporta como `tamaño − 3`. |

> **Cambio (2026-10-18, prioridad del usuario):** «es imperativo que prefiera los bloques de 2–3 contiguos». El motor penaliza **todo grupo de 1** (incluidos los días de 1 h) y **no** penaliza los grupos ≥4, para concentrar el esfuerzo en evitar la dispersión y mantener el tiempo de resolución acotado. El validador mantiene la métrica completa `f(len)`.

La penalización total de agrupación para una materia-curso en un día es la suma de las penalizaciones de todos sus arranques en ese día. La penalización global `Σ penalizaciones_agrupacion` es la suma sobre todas las materias-curso y todos los días.

> Ejemplos:
> - Materia de 2 h/día en un segmento → grupo de 2 → penalización 0.
> - Materia de 4 h/día en un segmento → grupo de 4 → penalización 1 (4 − 3).
> - Materia de 1 h/día → grupo de 1, horas del día = 1 → penalización 0 (D-4).
> - Materia de 3 h/día repartida como 2 + 1 en dos segmentos → grupo de 2 (pen 0) + grupo de 1 (horas día = 3 > 1 → pen 1) = 1 total.

### RF-3. Ámbito: por (curso, materia), no por docente

**EL SISTEMA** aplica la penalización **por cada par (curso, materia)**, independientemente de qué docente la imparta. Si un docente da la misma materia en varios cursos, cada curso-materia se evalúa por separado.

### RF-4. Peso de la nueva prioridad (S3) — constante interna del motor

**EL SISTEMA** usará una **constante entera del motor** `PESO_AGRUPACION` para la nueva prioridad blanda **S3**, con jerarquía estricta:

```
PESO_HORAS (1000)  >  COSTE_SUPLENTE (100)  >  PESO_AGRUPACION  >  0
```

**Orden de magnitud propuesto**: `PESO_AGRUPACION = 10` (diez veces menor que `COSTE_SUPLENTE`, cien veces menor que `PESO_HORAS`). El valor exacto es **parámetro del motor** y **no** se expone en el preset del usuario (decisión D-3 → A).

### RF-5. Jerarquía de prioridades intacta — S3 nunca canjea S1/S2 ni duras

**EL SISTEMA** garantiza que la nueva prioridad **S3 nunca puede**:
- violar ninguna restricción dura D1–D13,
- empeorar el óptimo de **S1** (exceso de horas) ni de **S2** (coste de suplentes).

La función objetivo pasa a ser:

```
Minimizar( PESO_HORAS · Σ exceso_horas_aula
         + COSTE_SUPLENTE · Σ suplentes_asignados
         + PESO_AGRUPACION · Σ penalizaciones_agrupacion )
```

donde `Σ penalizaciones_agrupacion` se define en RF-2.

**Justificación del orden de magnitud**: con `PESO_AGRUPACION = 10`, el máximo teórico de penalización de agrupación es ~8 arranques/día × 5 días × 13 cursos × (tamaño máx − 3) ≈ 5200, pero S2 (100 por suplente) y S1 (1000 por bloque de exceso) dominan siempre. Cambiar un suplente por un titular ahorra 100, lo que compensa **cualquier** mejora de agrupación posible. Un bloque de exceso cuesta 1000, muy por encima de toda la S3.

### RF-6. Reproducibilidad

**BAJO el mismo JSON de entrada y el mismo solver (versión, parámetros, semilla fija)**, **EL SISTEMA** produce el mismo horario. La nueva prioridad no introduce indeterminismo adicional.

### RF-7. Sin cambios en el contrato JSON

La entrada y la salida del solver **no cambian**. La nueva constante `PESO_AGRUPACION` vive **solo en el motor** (código). No se añade campo alguno en el preset ni se versiona el JSON (decisión D-3 → A).

### RF-8. Contigüidad evaluada dentro del turno del curso

**EL SISTEMA** evalúa la contigüidad **dentro del turno declarado del curso** (segmentos del turno, con corte en el receso según RF-1), no en el día completo 0–15 (decisión D-5 → A). D11 garantiza que un curso no tiene clases fuera de su turno.

## Requisitos no funcionales

- **RNF-1. Tiempo de resolución**: no degradar de forma inaceptable el tiempo actual (~1,3 s con los datos de prueba). La formulación debe usar **indicadores de arranque/transición por día y segmento** (variables booleanas auxiliares, no una variable por cada posible grupo), para que el modelo no explote en variables.
- **RNF-2. Determinismo**: `num_search_workers = 1`, `random_seed = 0` (ya fijados en el solver).
- **RNF-3. Trazabilidad**: la nueva regla tendrá al menos una comprobación verificable en las tareas (política de tests).

## Criterios de aceptación verificables

| ID | Qué se verifica | Cómo |
|----|----------------|------|
| AC-1 | Una materia-curso de 4 h/semana en 5 días (ej. 1 h/día 2 días + 2 h/día 1 día = 4 h) tiende a agruparse en bloques de 2 cuando toca 2 h/día; los días de 1 h no penalizan (D-4). | Test con fixture: curso con una materia de 4 h, forzar disponibilidad para permitir ambas formas; comprobar que la solución con grupos de 2 gana sobre la dispersa **si** S1/S2 son iguales. |
| AC-2 | El peso `PESO_AGRUPACION` es menor que `COSTE_SUPLENTE`: cambiar de suplente a titular (ahorro 100) **siempre** compensa cualquier mejora de agrupación. | Test: escenario donde usar un suplente permite mejor agrupación; verificar que el solver **prefiere el titular** (S2 gana a S3). |
| AC-3 | Una materia de 1 h/semana (1 h/día un día) **no es penalizada** (horas del día = 1 → grupo de 1 perdonado, D-4). Una materia de 3 h/día repartida 2+1 **sí penaliza** el grupo de 1 (horas del día = 3 > 1). | Test: materia de 1 h → penalización 0; materia de 3 h en 2+1 → penalización 1. |
| AC-4 | Grupos ≥ 4 penalizan el exceso sobre 3 (D-2 → A): grupo de 4 → penalización 1; grupo de 5 → penalización 2. | Test: forzar materia de 5 h en un día en un segmento; comprobar penalización 2. |
| AC-5 | Receso rompe contigüidad (D-1 → B): materia con bloques en franjas 1 y 2 (Mañana) **no** forman grupo contiguo; son dos grupos de 1 cada uno. | Test: forzar asignación en franjas 1 y 2; verificar dos arranques penalizados (si horas día > 1). |
| AC-6 | Reproducibilidad: misma entrada + mismo solver → mismo horario. | Ejecutar dos veces el solver con seed fija; comparar salidas completas. |
| AC-7 | Tiempo de resolución ≤ 5 s en el escenario de prueba (hoy ~1,3 s; margen holgado). | Test de benchmark existente (`test_solver_benchmark`). |
| AC-8 | Con los datos de prueba (13 cursos, 520 clases, variante holgada): **ningún grupo de tamaño 1 injustificado** (solo los de 1 h/día permitidos), **ningún grupo > 3 sin penalización contabilizada**, y **D1–D13 intactas**. | Test de integración: generar horario, extraer grupos por (curso, materia, día, segmento), contar tamaños, verificar penalizaciones esperadas y validar D1–D13 con `ValidadorSalidaSolver`. |
| AC-9 | Materia con bloques en **ambos segmentos** del mismo día (p. ej. 1 slot en {0,1} y 1 en {2..7} Mañana): dos grupos de 1, horas día = 2 → penalización 2. | Test: forzar asignación en dos segmentos del mismo día; verificar que se cuentan dos arranques y la penalización es 2. |

## Fuera de alcance

- Cambiar la rejilla de visualización (defecto conocido: descarta slots 12–15 y usa 12 filas de 45 min).
- Cambiar restricciones duras D1–D13 ni las prioridades S1/S2.
- Cambiar el contrato JSON de entrada/salida.
- Desempate entre titulares equivalentes o entre suplentes equivalentes (ya diferido en spec 005).
- Uso del `peso` del par (docente, materia-suplente) en el objetivo (diferido en spec 005).
- Reparto equilibrado min-máx de carga docente.

## Dudas abiertas

**Ninguna** — resueltas por el usuario el 2026-10-18.

## Apéndice: pistas de formulación (informativo)

> **No normativo.** La spec no prescribe CÓMO implementar; esto es solo una pista para el plan.

La formulación recomendada usa **variables de arranque** `start[p,m,c,d,s] ∈ {0,1}` (1 si `x[p,m,c,d,s]=1 ∧ x[p,m,c,d,s-1]=0` dentro del mismo segmento) y **variables de tamaño de grupo** o una penalización lineal por arranque con corrección para tamaños 2–3. Esto añade O(cursos × materias × días × franjas) variables booleanas auxiliares, no O(grupos posibles). La segmentación por receso se codifica como franjas donde `start` se fuerza a 1 tras el receso (o se crea variable por segmento).