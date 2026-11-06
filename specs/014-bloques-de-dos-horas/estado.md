# Estado 014

Fase: spec
Aprobaciones: — (spec en **borrador**; D-1 provisional, D-2 y D-3 cerradas; **11 dudas abiertas**)
Base: **`2eb45ec00b755ad862da74a0d3a09dafac96b3ed`** (HEAD del 2026-11-06, tras el cierre y commit de la 013). El plan fijará su `Base` definitivo al abrirse.
Dudas abiertas: **D-4, D-5, D-6, D-7, D-8, D-9, D-10, N-1, N-2, N-3, N-4**. Cerradas: **D-1** (blanda-alta, provisional), **D-2** (emparejamiento de la rejilla + bloque compartido), **D-3** (A: mismo día, turno y segmento). Detalle, alternativas y recomendación en `spec.md` → «Dudas abiertas».
Siguiente paso: **el usuario resuelve las once dudas abiertas**, empezando por **N-1** (dureza) y **D-4** (alineación por materia), que son las que cambian el alcance. Después el planner reescribe `spec.md` sin condicionalidad, el usuario la aprueba y se escriben `plan.md` y `tasks.md`. **No se implementa nada todavía.**
Veredicto: pendiente
Notas: spec **solo del QUÉ y el POR QUÉ**; no se ha tocado código, contrato, esquema ni documentación. `seed.md` es del usuario y **no se commitea**.

## Corrección de alcance (2026-11-06, tras leer `seed.md`)

La primera versión de la spec contaba **sueltas por materia** (una por materia impar). Con los datos
reales del centro el modelo correcto es **la rejilla del curso**: en cada (curso, día, segmento) las
franjas ocupadas se agrupan en **pares adyacentes**, y un par puede ser **2 h de la misma materia**
(bloque simple) o **1 h + 1 h de dos materias** (bloque compartido). Los dos restos de 5º de 31059
(Castellano 3 e Inglés 3) se juntan en un bloque compartido y **no queda ningún bloque de 1**. La spec se
reescribió entera con esta lectura.

## Hechos verificados (2026-11-06)

**Datos reales del centro (`seed.md`, del usuario, no versionado):**

- **Dos planes:** **31059** (Media General) con **38 h** en 1º–3º y **40 h** en 4º–5º; **31060** (Ciencias y Tecnología) con **40 h** en los cinco años.
- **24 cursos** (15 de 31059 y 9 de 31060), **889 alumnos**, **12 cursos por turno** (mañana y tarde).
- **12 aulas de uso general y 1 cancha**; **15 docentes por turno** (30 en total).
- **Solo dos materias impares en todo el centro:** Castellano **3** e Inglés **3**, las dos en **5º de 31059**; suman 40 (par).
- Reparto por curso: **8 cursos de 38 h** (1º–3º de 31059: 2+3+3) y **16 de 40 h** (4º–5º de 31059: 3+4, y los 9 de 31060). **944 clases por semana** (8×38 + 16×40).
- Turnos de **8 franjas** (hoy 35 min en el seed sintético; el centro usa 40 en su preset) y **un evento** tras la franja 2 → **segmentos de 2 y 6 franjas**, ambos de tamaño **par**.

**Consecuencias del modelo (verificadas a mano):**

1. **El emparejamiento es alcanzable en los 24 cursos:** todos suman un número par de horas y los segmentos son pares. En 5º el 3+3 se resuelve 2+1 / 2+1 con los dos restos compartiendo un bloque.
2. **La rejilla de un curso de 40 h ya está emparejada por construcción:** ocupa sus 40 franjas, los tramos miden 6 y 2, y el desvío de rejilla es 0 sea cual sea el reparto por materias. **La regla de rejilla sola no cambia nada en 16 de los 24 cursos.** Lo que cambia el horario es exigir que **cada materia ocupe bloques enteros** (alineación, D-4).
3. **Los únicos cursos con rejilla no trivial son los 8 de 38 h:** tienen 2 franjas libres; si caen en el mismo segmento-día, todos los tramos quedan pares; si caen en segmentos distintos, aparecen **dos bloques de 1**.
4. **Cota inferior del desvío de rejilla por curso:** `1` si el total semanal es impar, `0` si es par. En el centro real, **0 en todos**.
5. **El caso 5º depende exactamente del bloque compartido:** sin él, los dos restos serían dos bloques de 1; la alineación por materia es la que hace necesario el bloque compartido.

**Resto del sistema (sin cambios desde la 013):**

- **Hora académica = bloque = franja** (`bloques(horas) = horas`); la franja mide `duracion_minutos`.
- **S3** es blanda (10 por grupo de 1 por materia; 2–3 sin penalizar; el motor **no** suma `longGroup`); la **preferencia (25)** va por encima.
- **Jerarquía vigente (013):** S1 (1000) > S2 (100) > preferencia (25) > S3 (10), por fases lexicográficas.
- **Medición de fragmentación:** 359 grupos de 1, 76 de 2 y 3 de 3 (009, presupuesto 2.0); penalización S3 de 166 (013, presupuesto 6.0, 3 laboratorios). Confirma que la rejilla está llena pero las materias no están en bloques.
- **Presupuesto:** CP-SAT determinista (1 worker, seed 0); techo de **60 s**; la 013 midió **3937 → 4676 ms** con preferencias.
- **Contrato y esquema no cambian** con esta spec, salvo que D-10 decida lo contrario.

## Alcance corregido (para aprobar o corregir)

1. **Bloque** = dos franjas adyacentes ocupadas del mismo día y segmento; **simple** (misma materia) o **compartido** (dos materias). **Bloque de 1** = franja de un tramo impar.
2. **Emparejamiento de la rejilla del curso**: cada tramo se cubre con pares; un tramo impar deja exactamente un bloque de 1.
3. **Alineación (D-4, propuesta A):** cada materia ocupa bloques enteros salvo su resto (`h mod 2`), y el resto se comparte con el de otra materia. Sin esto, la regla es vacía en los cursos llenos.
4. **Métrica:** desvío de rejilla (bloques de 1) + desvío por bloques a medias (restos no legítimos, si D-4). Cota inferior por curso = `h mod 2`; con los datos reales, **0**.
5. **Naturaleza:** blanda-alta (D-1, provisional); **N-1** decide si se sube a dura con tope `desvío ≤ h mod 2`, que en el centro real es casi gratis.
6. **Jerarquía:** S1 > S2 > emparejamiento > preferencia > (S3); garantía estructural de la 013 intacta (D-6).
7. **Aula:** misma aula en el bloque simple (D-7) y en el compartido (N-2).
8. **Sin datos nuevos ni contrato nuevo:** informe de bloques, compartidos, bloques de 1, desvío y ubicación en el resultado de la generación (D-10).
9. **Todos** los cursos y materias, sin excepciones (D-9).

## Tamaño previsto

**9–10 tareas** (en el límite): métrica y funciones puras (tramos, bloques, desvío, cota) · restricciones del emparejamiento de la rejilla · alineación por materia (D-4) · prioridad y jerarquía sobre el resultado · aula del bloque · informe del resultado · documentación y constitución · supersesiones · regresión/golden y medición · cierre.

**Riesgo de alcance:** si **D-10 = C** (regla en el contrato: versión nueva, presets y frontend) **o** si **N-1 = dura y D-8 = B** (causa nueva, quinta categoría, §7.4/§7.5) **y** además se adopta D-4, el alcance supera las 10 tareas → **se propone dividir**: `014-bloques-de-dos-horas` (motor) y una segunda spec para contrato/configuración o para el diagnóstico ampliado.

## Riesgos declarados

- **La regla de rejilla sola es vacía en los cursos llenos.** Si D-4 = C, la spec no cambia el horario de 16 de los 24 cursos y no exige el bloque compartido de 5º. Es el riesgo principal: hay que decidir D-4 antes de aprobar.
- **Alineación y disponibilidad.** Con D-4 = A, el orden de las materias dentro del día queda restringido (bloques de 2 h alineados); con 15 docentes por turno y disponibilidad amplia debería ser holgado, pero es una restricción nueva que hay que medir.
- **Los 8 cursos de 38 h.** Su desvío depende de dónde caen 2 franjas libres; con N-1 = dura, si la disponibilidad impide ponerlas en el mismo segmento-día, **no hay horario**.
- **Capacidad de la cancha (dato del centro, no de la 014).** Educación Física son 4 h en los 24 cursos = **96 clases/semana**, y una sola cancha ofrece 5 días × 16 franjas = **80**. Si la materia exige el tipo «cancha», la instancia es **INFEASIBLE por la causa (2)** de la 010 antes de aplicar esta spec. Hay que verificarlo con el usuario (¿la cancha es obligatoria para EF?, ¿hay más espacios?).
- **Aulas especializadas.** Con el bloque en la misma aula, un bloque compartido de tipos distintos es imposible (N-2); con una sola cancha, los bloques de EF compiten por dos franjas seguidas.
- **Desempeño.** La alineación y el emparejamiento añaden restricciones y una prioridad sobre el resultado; el escenario de referencia (antes 13 cursos) puede dejar de ser representativo del centro real (24 cursos, 944 clases). RNF-2 fija el doble del tiempo de la 013 y el techo de 60 s.
- **Doble conteo.** Si D-5 = B, el bloque de 1 se penaliza dos veces (S3 y emparejamiento).
- **Precio en preferencias.** Si D-6 = A, algunas preferencias que hoy se respetan pasarán a incumplirse para sostener un bloque; el cumplimiento informado lo mostrará.
- **Cambio de la referencia de la 013.** El horario **sin** preferencias cambia; su `RNF-4`/criterio 11 y el **golden del horario** quedan superados. La huella **no** cambia.
- **Seed sintético de la app (008).** Sigue siendo 13 cursos × 40 h y es el escenario de regresión; su rejilla también es trivialmente 0, pero sus 4–6 materias impares por curso hacen la **alineación** exigente. Si D-4 = A, ese escenario puede no llegar a desvío 0 dentro del presupuesto: hay que fijar una línea base y no confundirla con el criterio del centro real.

## Lo que la decisión arrastra fuera de la 014

- **`specs/009-agrupacion-bloques-materias/`:** su métrica de S3 (grupos de 1 por materia) queda superada si D-5 = A; se conservan sus segmentos y su noción de grupo.
- **013:** supersesión de la jerarquía (si D-6 = A) y de su `RNF-4`/criterio 11 (la referencia sin preferencias cambia); la garantía estructural se mantiene.
- **005:** D3 no cambia; se añade la forma del reparto y la lectura de «bloque de dos horas».
- **010:** si D-8 = B, quinta categoría de causa; si no, las cuatro se mantienen. La causa (2) sigue siendo la de capacidad de aulas (cancha).
- **Constitución §7.1.2 y §7.2:** enmienda obligatoria.
- **Contrato y frontend:** sin cambios con D-10 = A; con D-10 = C habría versión nueva, sección/campo, validación (V20), presets y frontend.
- **Seed y tests:** `seed.md` **no se commitea**; el seed sintético (008) sigue siendo el de los tests. `test_generacion_seed`, el benchmark y los goldens de la 013 se ven afectados por el nuevo objetivo.
- **MEMORY.md:** parche propuesto, **no aplicado** por falta de permiso (ver «Notas»).

## Historial

- **2026-11-06 — apertura de la spec (planner).** `spec.md` en **borrador** con 11 RF, 7 RNF, casos límite, criterios y **10 dudas abiertas** (D-1…D-10). Alcance inicial: reparto en pares **por materia** (⌊h/2⌋ pares y una suelta por materia impar). Supuestos del brief corregidos: el seed sintético tiene materias de **2–6 h** (no solo 4–5) y **4–6 impares por curso**.
- **2026-11-06 — corrección de alcance tras leer `seed.md`.** D-1, D-2 y D-3 resueltas por el usuario. El emparejamiento pasa a ser **de la rejilla del curso**, con **bloque compartido** de dos restos; la métrica pasa a **por curso** (bloques de 1 y tramos impares). Se reescribe `spec.md` entero (12 RF, nueva definición de bloque, alineación, cota, paridad, casos límite y criterios con los datos reales) y se añaden **N-1…N-4**; quedan **11 dudas abiertas**. Hallazgo clave documentado: **la rejilla de un curso lleno ya está emparejada**, así que la alineación por materia (D-4) es la que da sentido a la regla y la que hace necesario el bloque compartido de 5º.

## Notas

- **Parche de `MEMORY.md`: NO aplicado** (permiso denegado; el planner solo escribe en `specs/`). Cambios propuestos para el coordinator:
  1. «Fase actual»: la **014 está en borrador (2026-11-06)**, con **11 dudas abiertas**, y el siguiente paso es resolverlas (primero N-1 y D-4).
  2. Tabla «Specs activas», fila 014: `— (planificada) | no abierta` → `borrador (2026-11-06) | spec corregida al modelo de rejilla; 11 dudas abiertas (D-4…D-10, N-1…N-4)`.
  3. «Siguiente paso» punto 1: «Escribir la 014…» → «Resolver las dudas de la 014 (N-1 y D-4 primero) y decidir los commits de la 013».
  4. Nota del épico: la 014 deja de estar «planificada y sin escribir»; pasa a **borrador con alcance corregido**.
- **`seed.md` no se versiona** (está en la raíz, sin seguimiento). La spec no lo cita por nombre; sus datos se resumen en «Contexto».
- **Precisión de unidades:** «bloque de dos horas» = **dos franjas** (dos hotución §7.1.2 y §7.2:** enmienda obligatoria.
- **Contrato y frontend:** sin cambios con D-10 = A; con D-10 = C, versión nueva y validación.
- **MEMORY.md:** parche propuesto, **no aplicado** por falta de permiso.

## Historial

-