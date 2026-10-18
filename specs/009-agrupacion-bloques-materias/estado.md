# Estado de la spec 009 - Agrupación de bloques contiguos por materia-curso

## Fase actual
**implementada** (código + tests + validación). Pendiente: veredicto formal del reviewer.

## Historial
- 2026-10-18: Creación de la spec (borrador) con RF en EARS, RNF, criterios de aceptación y 5 dudas abiertas D-1 a D-5.
- 2026-10-18: **Dudas resueltas por el usuario** (decisiones definitivas):
  - **D-1 → B**: el receso **SÍ rompe** la contigüidad. Mañana {0,1} y {2..7}; Tarde {8,9} y {10..15}.
  - **D-2 → A**: solo 2–3 son buenos; 1 penaliza (salvo horas/día = 1) y ≥4 penaliza `tamaño − 3`.
  - **D-3 → A**: `PESO_AGRUPACION` es constante interna; sin cambios de contrato JSON.
  - **D-4 → B**: grupo de 1 no penaliza si las horas de esa materia-curso en el día son exactamente 1.
  - **D-5 → A**: contigüidad dentro del turno del curso.
- 2026-10-18: Reescritura de `spec.md` con RF-1..RF-8 y AC-1..AC-9; dudas = «Ninguna».
- 2026-10-18: Correcciones QA aplicadas (AC-1 aritmética, nota a apéndice, AC-9 cross-segmento).
- 2026-10-18: **Aprobada por el usuario**. Generados `plan.md` (Base `7e2688674ac587a808aa94fb76cd991b02db3bda`) y `tasks.md` (T1–T8).

## Progreso de implementación
- **T1** ✅: `PESO_AGRUPACION = 10`; `segmentosTurno`, `esInicioSegmento`, `calcularPenalizacionGrupo`.
- **T2** ✅: variables `start` por (materia, curso, día, segmento, franja).
- **T3** ✅: restricción `start ⇔ x` (arranque de grupo) con corte en receso.
- **T4** ✅: objetivo S3 `PESO_AGRUPACION · Σ start` (proxy de fragmentación).
- **T5** ✅: tests de agrupación (2 h contiguas, receso rompe, 4 h un grupo).
- **T6** ✅: `ValidadorSalidaSolver` analiza grupos y penalización RF-2; `AnalisisSalida` expone `grupos` y `penalizacionAgrupacion`.
- **T7** ✅: límite determinista `max_deterministic_time(2.0)`.
- **T8** ✅: suite completa 49/49 verde.

## Validación (2026-10-18)
- **Bug corregido**: `despues_de_slot` es **1-based relativo al turno** (el seed guarda `2`), pero `segmentosTurno` lo trataba como índice absoluto. Los tests pasaban con valores artificiales. Corregido y tests actualizados.
- **RF-6 corregido**: el límite por tiempo de pared (`max_time_in_seconds`) introducía no-determinismo. Sustituido por `max_deterministic_time(2.0)`: dos ejecuciones dan la misma distribución.
- Verificado con el preset real (13 cursos, 520 clases): D1–D13 intactas, P1–P4 sin conflictos, `RESULTADO: VALIDO (0 fallas)`.

## Resultado medido (preset real, 13 cursos)
| Grupos | Sin S3 | Con S3 |
|---|---|---|
| Tamaño 1 | 459 | 391 |
| Tamaño 2 | 29 | 52 |
| Tamaño 3 | 1 | 7 |
| Tamaño 4 | 0 | 1 |
| Tiempo | ~1,3 s | ~14 s |

## Deuda técnica
- **RNF-1**: ~14 s frente al objetivo ≤5 s. El modelo con variables de arranque es duro de optimizar; falta reducir variables o reformular la penalización.
- **Penalización exacta `f(len, horasDia)` en el solver**: el motor usa `Σ start` como proxy; el validador sí calcula la penalización exacta. Requiere una formulación eficiente de `len`.
- **Veredicto formal del reviewer** pendiente.

## Próximo paso
Validación QA final (reviewer) y cierre definitivo de la spec.
