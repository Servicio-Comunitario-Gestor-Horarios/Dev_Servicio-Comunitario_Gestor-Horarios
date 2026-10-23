# Estado 012

Fase: plan
Aprobaciones: spec 2026-10-23
Base: 5d6b1da77c9da645441a0bebb38e37753f0f7198 (HEAD). **Condición:** el esquema WIP de la 009 está sin commitear; la base efectiva será el commit que incluya la 009 (y, según el orden propuesto, la 011). El diff del árbol no se imputa a la implementación.
Dudas abiertas: ninguna
Siguiente paso: el planner escribirá `plan.md` y `tasks.md` de la 009 cuando el usuario confirme la base de commit.
Veredicto: pendiente
QA: CAMBIOS NECESARIOS resueltos el 2026-10-23 (H1–H5); spec aprobada por el usuario el 2026-10-23.

## Decisiones cerradas (2026-10-23)

- **Modelo replicado.** Cada preferencia tiene docente, día, hora de inicio, hora de fin y **sentido**
  (preferida o evitada), con identidad propia. Se replica el modelo de disponibilidad, sin mezclarse
  con él.
- **Solo persistir y exponer.** No hay solver, ni contrato, ni efecto en la generación en esta spec.
- **Día.** **0 = domingo … 6 = sábado**, el mismo rango de la disponibilidad. Se admiten días fuera de
  lunes–viernes: se guardan y se exponen, pero no intervienen en la generación actual.
- **Validación.** Docente existente; día 0–6; inicio anterior al fin; sentido válido; sin solapes de
  **sentidos opuestos** en el mismo docente y día; sin duplicados exactos.
- **Solapes del mismo sentido.** Se permiten (no se contradicen y no se fusionan); la fusión, si hace
  falta, es de quien presenta.
- **Duplicado exacto.** Mismo docente, día, hora de inicio, hora de fin y sentido; no implica
  exclusividad sobre otros rangos.
- **Cambio de sentido.** Se revalida contra los solapes opuestos; si aparece una contradicción, se
  rechaza y se conserva el sentido anterior.
- **Cascada.** La baja del docente borra preferencias y disponibilidad en una operación atómica y las
  muestra como dependencias antes de confirmar.
- **Conmutación de variante.** Se borran las preferencias antes que los docentes, junto con
  disponibilidad y materias; **supersede RF-9 y la definición de reemplazo de docentes de la 008** en lo
  que toca a las preferencias. Si falla a mitad, no se borra nada.
- **Huella de datos.** Las preferencias **no** entran en la huella mientras el solver no las consuma,
  porque no intervienen en la generación; se deja anotado que deberán incluirse cuando una spec del
  solver las use.
- **Seed.** No se siembran preferencias (conjunto vacío, recuento 0); no hay una política decidida y el
  conjunto se sustituirá por los datos reales. El vaciado las borra antes que a los docentes.
- **Esquema.** Registro nuevo; incremento de la marca de versión, catálogo, huella de esquema y política
  sin migración de la 009.
- **Frontend fuera de alcance.** Se entregan servicios y guía de API.

## Hallazgos del QA resueltos

- **H1:** la conmutación de variante borra preferencias antes que docentes y supersede la 008; no falla
  por claves foráneas.
- **H2:** el rango de día queda fijado en la spec (0–6 con significado) y se declara que los días fuera
  de lunes–viernes se guardan sin efecto en la generación.
- **H3:** desempate de orden por hora de fin y, si persiste, por identidad.
- **H4:** casos límite añadidos: actualizar/eliminar preferencia inexistente, docente sin preferencias
  y día válido fuera de la semana de generación.
- **H5:** el duplicado se define como **exacto**, sin sugerir exclusividad; los solapes del mismo
  sentido se admiten.

## Notas

- **Orden de implementación.** Después de la 011 para no pisar el incremento de versión de esquema; en
  contenido solo depende de la 009 y podría adelantarse, pero no aporta ventaja.
- **Preferencias futuras.** El modelo no incluye recurrencias, excepciones por fecha ni pesos; si el
  solver las necesita, será otra spec.
- **No hay plan ni tareas todavía.** Esta fase termina con la aprobación de la spec.

## Tamaño previsto

Ocho RF agrupables en **≤10 tareas**: modelo, validación, servicios, cascada, huella, esquema/versión,
seed/conmutación y guía de API.
