> ADVERTENCIA: ESTIMACIÓN — no horas reales. Modelo span−gaps (umbral 60 min, piso 1h, techo 12h), incertidumbre ±30–50%. Corte 2026-08-01.
> Los valores de horas son RANGOS estimados con escenarios de 45/60/90 min (conservador/medio/optimista). Prohibido citarlos como horas reales.
> Generado por `scripts/traqueo-horas-por-sprint.py` — reproducible: mismo estado del repo + misma data de GitHub => mismo informe byte a byte.

# Traqueo de horas por sprint — 2026-08-01

Informe read-only de actividad del equipo (git `--all` + GitHub PRs/issues + informes institucionales). Estimación de días activos y horas/día en RANGO por persona y sprint, con vistas calendario, validación cruzada y apéndice metodológico.

## Tabla canónica de sprints

| Sprint | Inicio | Fin |
|---|---|---|
| S1 | – | 2026-06-28 |
| S2 | 2026-06-29 | 2026-07-05 |
| S3 | 2026-07-06 | 2026-07-12 |
| S4 | 2026-07-13 | 2026-07-19 |
| S5 | 2026-07-20 | 2026-07-26 |
| S6 | 2026-07-27 | 2026-08-01 |

Nota: S1 = fase de bootstrap previa a la planificación (commits del 05-10 al 06-28). S6 termina en el corte del análisis (2026-08-01).

## Resumen por persona/sprint

Columnas: días activos (cualquier fuente: commit, PR, issue, revisión); horas/día RANGO (solo fuentes de horas: commit, PR, issue); commits; archivos creados (primer commit que añade el archivo); PRs (crédito completo, `Nm/Nc` = merged / cerrado-sin-merge); issues (asignados/resueltos).

| Persona | Sprint | Días activos | Horas/día (rango) | Commits | Arch. creados | PRs | Issues |
|---|---|---|---|---|---|---|---|
| Daniel Reyna | S1 | 1 | 1h (±0%) | 1 | 0 | — | — |
| Daniel Reyna | S2 | 4 | 1h (±0%) | 1 | 7 | 1 | 1/1 |
| Daniel Reyna | S3 | 4 | 1h (±0%) | 3 | 4 | 1 | 1/1 |
| Daniel Reyna | S4 | 1 | 1h (±0%) | 0 | 0 | 1 | 1/0 |
| Daniel Reyna | S5 | 3 | 1h (±0%) | 2 | 5 | 1 | 1/1 |
| Daniel Reyna | S6 | 1 | 1h (±0%) | 0 | 0 | — | 2/1 |
| Luis Rojas | S1 | 3 | 1–2h (±35%) | 12 | 66 | — | — |
| Luis Rojas | S2 | 6 | 1–2h (±28%) | 57 | 58 | 8 | 1/1 |
| Luis Rojas | S3 | 4 | 1–2h (±19%) | 11 | 6 | 2 | 1/1 |
| Luis Rojas | S4 | 2 | 1h (±0%) | 2 | 0 | 1 | 1/0 |
| Luis Rojas | S5 | 5 | 1h (±0%) | 25 | 47 | 1 | 2/0 |
| Luis Rojas | S6 | 3 | 1h (±0%) | 6 | 36 | — | 3/3 |
| Manuel García | S1 | — | datos insuficientes | — | — | — | — |
| Manuel García | S2 | 2 | 1h (±0%) | 2 | 6 | 1 | 2/1 |
| Manuel García | S3 | 2 | 1h (±0%) | 1 | 2 | 1 (0m/1c) | 1/1 |
| Manuel García | S4 | 2 | 1h (±0%) | 6 | 3 | 1 (0m/1c) | 1/0 |
| Manuel García | S5 | 4 | 1h (±0%) | 6 | 3 | 3 (2m/1c) | 1/0 |
| Manuel García | S6 | 1 | 1h (±0%) | 0 | 0 | — | 1/3 |
| Nicole Sereno | S1 | — | datos insuficientes | — | — | — | — |
| Nicole Sereno | S2 | 2 | 1h (±0%) | 2 | 42 | 1 | 1/1 |
| Nicole Sereno | S3 | 1 | 1–2h (±47%) | 2 | 21 | 1 (0m/1c) | 2/0 |
| Nicole Sereno | S4 | 1 | 1h (±0%) | 0 | 0 | — | 2/2 |
| Nicole Sereno | S5 | 3 | 1h (±0%) | 2 | 12 | 3 (2m/1c) | 1/3 |
| Nicole Sereno | S6 | — | datos insuficientes | — | — | — | — |
| Paola Peña | S1 | — | datos insuficientes | — | — | — | — |
| Paola Peña | S2 | 3 | 1–2h (±12%) | 5 | 22 | 2 (1m/1c) | 1/1 |
| Paola Peña | S3 | 2 | 1h (±0%) | 3 | 5 | 1 | 1/1 |
| Paola Peña | S4 | 1 | 1h (±0%) | 1 | 4 | 1 | 1/0 |
| Paola Peña | S5 | 3 | 1h (±0%) | 4 | 0 | 1 | 1/2 |
| Paola Peña | S6 | — | datos insuficientes | — | — | — | — |

* = día(s) con actividad solo de revisión (no alimenta el modelo de horas; se aplica el piso de 1h).
** = las variantes de escenario difieren >50%: ver apéndice B.

## Calendario — qué días trabajó

`X` = actividad (commit/PR/issue), `R` = solo revisión, `–` = sin actividad. Fechas en tz del repo (UTC-04:00).

| Persona | Sprint | L | M | X | J | V | S | D |
|---|---|---|---|---|---|---|---|---|
| Daniel Reyna | S1 | – | – | – | – | – | – | X |
| Daniel Reyna | S2 | X | – | R | – | X | – | R |
| Daniel Reyna | S3 | X | R | – | – | – | X | X |
| Daniel Reyna | S4 | – | – | – | – | – | – | X |
| Daniel Reyna | S5 | X | – | – | – | – | R | X |
| Daniel Reyna | S6 | X | – | – | – | – | – | – |
| Luis Rojas | S1 | – | – | – | – | – | – | X |
| Luis Rojas | S2 | X | – | X | X | X | X | X |
| Luis Rojas | S3 | – | X | – | – | X | X | X |
| Luis Rojas | S4 | X | – | – | – | R | – | – |
| Luis Rojas | S5 | X | – | – | R | X | X | X |
| Luis Rojas | S6 | X | X | – | – | X | – | – |
| Manuel García | S1 | – | – | – | – | – | – | – |
| Manuel García | S2 | X | – | – | – | – | X | – |
| Manuel García | S3 | – | – | – | X | – | X | – |
| Manuel García | S4 | – | – | – | X | – | X | – |
| Manuel García | S5 | – | – | – | X | X | X | X |
| Manuel García | S6 | X | – | – | – | – | – | – |
| Nicole Sereno | S1 | – | – | – | – | – | – | – |
| Nicole Sereno | S2 | X | – | – | – | – | – | X |
| Nicole Sereno | S3 | – | – | – | – | – | – | X |
| Nicole Sereno | S4 | X | – | – | – | – | – | – |
| Nicole Sereno | S5 | X | – | – | – | – | X | X |
| Nicole Sereno | S6 | – | – | – | – | – | – | – |
| Paola Peña | S1 | – | – | – | – | – | – | – |
| Paola Peña | S2 | X | – | – | – | – | X | X |
| Paola Peña | S3 | – | – | – | – | X | X | – |
| Paola Peña | S4 | – | – | – | – | – | – | X |
| Paola Peña | S5 | X | – | – | X | X | – | – |
| Paola Peña | S6 | – | – | – | – | – | – | – |

Nota: el calendario de S1 muestra la semana previa al corte de S1 (22–28 jun); actividad anterior (bootstrap 05-10/05-12, solo Luis Rojas) queda fuera de la rejilla.

## Actividad de revisión de PRs

Comentarios y reviews de PR cuentan como evidencia de días activos del revisor, pero NO alimentan el modelo de horas (fuente demasiado dispersa).

| Persona | Sprint | Eventos de revisión |
|---|---|---|
| Daniel Reyna | S2 | 4 |
| Daniel Reyna | S3 | 2 |
| Daniel Reyna | S5 | 4 |
| Luis Rojas | S2 | 8 |
| Luis Rojas | S3 | 4 |
| Luis Rojas | S4 | 9 |
| Luis Rojas | S5 | 17 |

## Validación cruzada con informes institucionales

Comparación de las contribuciones declaradas en `docs/informes/informe_sprint_institucional_semana_{2..5}.md` contra los días activos evidenciados en git/GitHub (mismo sprint por fechas).

| Sprint | Persona | Informe (contribuciones) | Evidencia (días activos) |
|---|---|---|---|
| S2 | Daniel Reyna | 1 | 4 |
| S2 | Luis Rojas | 4 | 6 |
| S2 | Manuel García | 4 | 2 |
| S2 | Nicole Sereno | 2 | 2 |
| S2 | Paola Peña | 1 | 3 |
| S3 | Daniel Reyna | 1 | 4 |
| S3 | Luis Rojas | 3 | 4 |
| S3 | Manuel García | 0 | 2 |
| S3 | Nicole Sereno | 2 | 1 |
| S3 | Paola Peña | 1 | 2 |
| S4 | Daniel Reyna | 1 | 1 |
| S4 | Luis Rojas | 1 | 2 |
| S4 | Manuel García | 2 | 2 |
| S4 | Nicole Sereno | 2 | 1 |
| S4 | Paola Peña | 1 | 1 |
| S5 | Daniel Reyna | 1 | 3 |
| S5 | Luis Rojas | 3 | 5 |
| S5 | Manuel García | 2 | 4 |
| S5 | Nicole Sereno | 1 | 3 |
| S5 | Paola Peña | 1 | 3 |

Las discrepancias se documentan en la sección Notas; nunca se ocultan.

## Corroboración local (mtime) y timelapses

El mtime de archivos en el árbol de trabajo solo corrobora sesiones locales en esta máquina (Luis Rojas); nunca se atribuye a otros miembros y no alimenta el modelo de horas.

Días con actividad local de archivos (mtime, tz UTC-04:00): 2026-05-12, 2026-06-28, 2026-07-03, 2026-07-04, 2026-07-05, 2026-07-10, 2026-07-13, 2026-07-14, 2026-07-16, 2026-07-20, 2026-07-25, 2026-07-26, 2026-07-28, 2026-07-31, 2026-08-01

Anclas de timelapses:
- `docs/timelapses/timelapse-2026-07-05.mp4` (S2, existe): sesión local esperada de Luis
- `docs/timelapses/timelapse-2026-07-14.mp4` (S4, existe): sesión local esperada de Luis
- `docs/timelapses/timelapse-2026-07-20.mp4` (S5, existe): sesión local esperada de Luis
- `docs/timelapses/timelapse-2026-07-28.mp4` (S6, existe): sesión local esperada de Luis

## Projects v2

Intento de lectura del tablero de la org (Servicio-Comunitario-Gestor-Horarios): Projects v2 excluido — token sin scope read:project

## Notas y discrepancias

- Discrepancia S3: el informe reporta 'Manuel García: 0 contribuciones' pero la evidencia git/gh muestra 2 día(s) activo(s).
- PRs cerrados sin merge (cuentan como trabajo completo, marcados en la columna PRs): #72, #69, #64, #62, #58, #52.
- 2 commits alcanzables solo vía `refs/stash` (WIP/index no mergeados) excluidos de la actividad: 034a830f59e0d263f654aeb011d1e9fac8d8c48b, 32dc720040cf5f4bfb444f7363abf0bbe014a8d8.

## Apéndice A — Metodología

**Modelo de horas (por persona y día):**
1. Eventos de horas = commits (fecha de autor), eventos de PR (creación, merge o cierre) y eventos de issue (creación, cierre), atribuidos a la persona correspondiente. Los eventos de revisión NO alimentan el modelo.
2. Para cada día: span = primer a último evento; las sesiones se dividen cuando el gap entre eventos supera el umbral (default 60 min).
3. Horas/día = span del día menos los gaps entre sesiones (= suma de duraciones de sesiones). Piso 1h, techo 12h.
4. Escenarios de umbral: conservador (45 min), medio (60 min), optimista (90 min). El rango reportado = [mín, máx] entre escenarios; incertidumbre = (máx−mín)/máx, típicamente 30–50%.
5. Si las variantes difieren >50%, el detalle por escenario se muestra abajo (apéndice B).

**Fuentes y zonas horarias:** git (`--all`, `.mailmap`) usa la tz del repo (UTC-04:00); GitHub devuelve timestamps en UTC (Z). Todo se normaliza a epoch y se agrupa por día en la tz del repo.

**Atribución:** `.mailmap` canonicaliza las 8 variantes de autor a 5 nombres canónicos (Luis Rojas, Daniel Reyna, Manuel García, Paola Peña, Nicole Sereno). Usuarios de GitHub mapeados por tabla fija. Los PRs cerrados sin merge cuentan como trabajo completo del autor. El `mergedAt` se atribuye al autor del PR (simplificación: el merge lo ejecuta a veces el mantenedor). Los issues se atribuyen a sus asignados; si el issue no tiene label de sprint, se ubica por su fecha de creación.

**Exclusiones:** `build*/`, `.agents_backup/`, binarios (no cuentan como archivos creados ni líneas), commits alcanzables solo vía `refs/stash`, y cualquier evento posterior al corte (2026-08-01).

**Proxy de creación de archivos:** primer commit (más antiguo, sobre `--all`) que añade el path.

**Comandos clave:** `git log --all --use-mailmap --date=iso-strict`, `git log --all --diff-filter=A`, `git log --all --numstat`, `git shortlog -sne --all`, `gh pr list`, `gh issue list`, `gh pr view --json reviews,comments`.

## Apéndice B — Horas por escenario (promedio de horas/día por sprint)

| Persona | Sprint | 45 min | 60 min | 90 min | Días con fuente de horas |
|---|---|---|---|---|---|
| Daniel Reyna | S1 | 1.0 | 1.0 | 1.0 | 1 |
| Daniel Reyna | S2 | 1.1 | 1.1 | 1.1 | 2 |
| Daniel Reyna | S3 | 1.0 | 1.0 | 1.0 | 3 |
| Daniel Reyna | S4 | 1.0 | 1.0 | 1.0 | 1 |
| Daniel Reyna | S5 | 1.0 | 1.0 | 1.0 | 2 |
| Daniel Reyna | S6 | 1.0 | 1.0 | 1.0 | 1 |
| Luis Rojas | S1 | 1.1 | 1.4 | 1.7 | 3 |
| Luis Rojas | S2 | 1.2 | 1.3 | 1.7 | 6 |
| Luis Rojas | S3 | 1.0 | 1.0 | 1.2 | 4 |
| Luis Rojas | S4 | 1.0 | 1.0 | 1.0 | 1 |
| Luis Rojas | S5 | 1.1 | 1.1 | 1.1 | 4 |
| Luis Rojas | S6 | 1.0 | 1.0 | 1.0 | 3 |
| Manuel García | S2 | 1.0 | 1.0 | 1.0 | 2 |
| Manuel García | S3 | 1.0 | 1.0 | 1.0 | 2 |
| Manuel García | S4 | 1.4 | 1.4 | 1.4 | 2 |
| Manuel García | S5 | 1.0 | 1.0 | 1.0 | 4 |
| Manuel García | S6 | 1.0 | 1.0 | 1.0 | 1 |
| Nicole Sereno | S2 | 1.1 | 1.1 | 1.1 | 2 |
| Nicole Sereno | S3 | 1.0 | 1.9 | 1.9 | 1 |
| Nicole Sereno | S4 | 1.0 | 1.0 | 1.0 | 1 |
| Nicole Sereno | S5 | 1.0 | 1.0 | 1.0 | 3 |
| Paola Peña | S2 | 1.1 | 1.1 | 1.2 | 3 |
| Paola Peña | S3 | 1.0 | 1.0 | 1.0 | 2 |
| Paola Peña | S4 | 1.0 | 1.0 | 1.0 | 1 |
| Paola Peña | S5 | 1.0 | 1.0 | 1.0 | 3 |

Horas totales estimadas por persona y sprint (rango entre escenarios):

| Persona | Sprint | Horas totales (rango) |
|---|---|---|
| Daniel Reyna | S1 | 1h |
| Daniel Reyna | S2 | 2h |
| Daniel Reyna | S3 | 3h |
| Daniel Reyna | S4 | 1h |
| Daniel Reyna | S5 | 2h |
| Daniel Reyna | S6 | 1h |
| Luis Rojas | S1 | 3–6h |
| Luis Rojas | S2 | 7–11h |
| Luis Rojas | S3 | 4–5h |
| Luis Rojas | S4 | 1h |
| Luis Rojas | S5 | 4h |
| Luis Rojas | S6 | 3h |
| Manuel García | S2 | 2h |
| Manuel García | S3 | 2h |
| Manuel García | S4 | 3h |
| Manuel García | S5 | 4h |
| Manuel García | S6 | 1h |
| Nicole Sereno | S2 | 2h |
| Nicole Sereno | S3 | 1–2h |
| Nicole Sereno | S4 | 1h |
| Nicole Sereno | S5 | 3h |
| Paola Peña | S2 | 3–4h |
| Paola Peña | S3 | 2h |
| Paola Peña | S4 | 1h |
| Paola Peña | S5 | 3h |

## Apéndice C — Volumen de líneas de código (texto, excluye build*/binarios)

| Persona | Sprint | + líneas | − líneas |
|---|---|---|---|
| Daniel Reyna | S1 | 3 | 0 |
| Daniel Reyna | S2 | 1085 | 54 |
| Daniel Reyna | S3 | 1016 | 99 |
| Daniel Reyna | S5 | 1052 | 508 |
| Luis Rojas | S1 | 13687 | 2300 |
| Luis Rojas | S2 | 9795 | 3185 |
| Luis Rojas | S3 | 4642 | 928 |
| Luis Rojas | S4 | 2675 | 286 |
| Luis Rojas | S5 | 12472 | 643 |
| Luis Rojas | S6 | 17772 | 37 |
| Manuel García | S2 | 256 | 53 |
| Manuel García | S3 | 126 | 0 |
| Manuel García | S4 | 408 | 164 |
| Manuel García | S5 | 2492 | 913 |
| Nicole Sereno | S2 | 2379 | 0 |
| Nicole Sereno | S3 | 2548 | 1 |
| Nicole Sereno | S5 | 1971 | 0 |
| Paola Peña | S2 | 51142 | 202 |
| Paola Peña | S3 | 1649 | 506 |
| Paola Peña | S4 | 536 | 1 |
| Paola Peña | S5 | 1864 | 505 |

## Limitaciones (caveats)

- **No son horas reales**: modelo estadístico de estimación con incertidumbre 30–50%; sirve para comparar intensidad relativa, no para reportes oficiales.
- El mtime local es solo corroboración de la máquina de Luis Rojas; el trabajo sin eventos (reuniones, diseño en pizarra, lectura) es invisible para el modelo.
- La actividad de Luis puede estar sobre-representada (commits de CI/build/AI tooling, merges de mantenedor atribuidos a autores de PR).
- S1 (≤ 06-28) solo tiene actividad de bootstrap: Luis Rojas y Daniel Reyna; para el resto no hay datos (no se estima horas).
- Issues #45 y #55 quedaron fuera del rango #6–41 del diseño (no tienen labels de sprint); su actividad no se refleja en las métricas de issues.
- Projects v2: excluido (token sin scope `read:project`)

## Verificación y reproducibilidad

- Commits (`git rev-list --all --count`): 156
- Autores canónicos (`.mailmap`, `git shortlog -sne --all`): 5 (Daniel Reyna, Luis Rojas, Manuel García, Nicole Sereno, Paola Peña)
- PRs merged: 26 | PRs cerrados sin merge: 6
- Reproducibilidad: `python3 scripts/traqueo-horas-por-sprint.py` dos veces consecutivas debe producir `diff` vacío (el informe no lleva marca de tiempo).
