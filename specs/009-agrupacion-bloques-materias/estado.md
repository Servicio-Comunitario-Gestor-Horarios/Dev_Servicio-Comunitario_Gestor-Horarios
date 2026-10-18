# Estado de la spec 009 - Agrupación de bloques contiguos por materia-curso

## Fase actual
**spec: aprobada (2026-10-18)**

## Historial
- 2026-10-18: Creación de la spec (borrador) con RF en EARS, RNF, criterios de aceptación y 5 dudas abiertas numeradas D-1 a D-5.
- 2026-10-18: **Dudas resueltas por el usuario** (decisiones definitivas):
  - **D-1 → B**: el receso de 10 min **SÍ rompe** la contigüidad. Segmentos contiguos en Mañana: {0,1} y {2,3,4,5,6,7}; en Tarde: {8,9} y {10,11,12,13,14,15}. Los grupos nunca cruzan receso ni turnos.
  - **D-2 → A**: solo grupos de tamaño **2 o 3** son «buenos». Grupo de **1** penaliza (salvo horas del día = 1, ver D-4). Grupo de **≥ 4** penaliza **tamaño − 3** (cada franja extra sobre 3 cuenta 1).
  - **D-3 → A**: `PESO_AGRUPACION` es **constante interna del motor**. NO se expone en preset, NO cambia contrato JSON ni versión.
  - **D-4 → B**: grupo de 1 **no penaliza** cuando horas del día para esa materia-curso = **1** (inevitable). En cualquier otro caso, grupo de 1 sí penaliza.
  - **D-5 → A**: contigüidad se evalúa **dentro del turno del curso** (segmentos del turno con corte en receso según D-1).
- 2026-10-18: Reescritura de `spec.md` incorporando decisiones como RF verificables (RF-1 a RF-8), criterios de aceptación medibles (AC-1 a AC-8), y sección de dudas abierta marcada como «Ninguna».
- 2026-10-18: **Correcciones QA aplicadas**:
  - AC-1: corregido error aritmético (4 h/semana = 1 h/día 2 días + 2 h/día 1 día).
  - Nota final movida a **Apéndice: pistas de formulación (informativo)**, fuera de la parte normativa.
  - Añadido **AC-9**: cross-segmento (bloques en dos segmentos del mismo día → dos grupos de 1, horas día = 2 → penalización 2).
- 2026-10-18: **Aprobada por el usuario**. Estado → **aprobada (2026-10-18)**. Fase → planificación.

## Próximo paso
Generar `plan.md` y `tasks.md` (esta acción).

## Base de código (para el plan futuro)
- `specs/005-nucleo-solver/spec.md` (aprobada, sin dudas)
- `specs/005-nucleo-solver/plan.md` (Base: f94acba455a04ae80364c82e1f07ae16d9f156ec)
- Commit HEAD actual: **pendiente de confirmar** (se fijará al generar el plan).