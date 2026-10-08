> **ADVERTENCIA — INFORME DE FECHAS, NO DE ESFUERZO.** Este documento responde una sola pregunta: **¿qué días dejó Luis Rojas (`LuisRojas260305`) un registro verificable en GitHub o en git?** **NO es** una medición de horas, de rendimiento ni de calidad del trabajo. **NO cuenta** horas, duración, complejidad ni calidad. Una fecha significa «hay evidencia de un evento en ese día», no «trabajó N horas ese día». Corte de datos: **2026-09-29**; las marcas de tiempo de GitHub son en UTC y se normalizan a **UTC−04:00** (offset del repositorio). Generado por `scripts/fechas-pr-revision-correccion.py` — reproducible: mismo estado del repo + misma data de GitHub => mismo informe byte a byte. Regenerar con `python3 scripts/fechas-pr-revision-correccion.py`.

# Fechas de PR, revisión y corrección — 2026-09-29

Informe read-only de la actividad de Luis Rojas en el repositorio `Servicio-Comunitario-Gestor-Horarios/Dev_Servicio-Comunitario_Gestor-Horarios` (público). Cubre tres hechos verificables e independientes: la apertura de pull requests, la revisión del código de terceros y la corrección del código de compañeros. La fuente primaria es la API de GitHub vía `gh`; el historial local `git` corrobora autoría y trailers. No se hizo `git fetch`: el clon local es idéntico a `origin/`.

## Resumen

Zona horaria de todas las fechas de este informe: **UTC−04:00**. Las marcas de tiempo de la API de GitHub llegan en UTC (`Z`); se convierten antes de agrupar por día. Ver *Nota de zona horaria* más abajo para el detalle de los eventos que cambian de día.

| Métrica | Valor | Proveniencia |
|---|---|---|
| PRs del repositorio (todos los estados) | 32 | API |
| PRs abiertos por Luis | 12 | API |
| Fechas distintas de creación de PR | 8 | API |
| Fechas distintas de merge de PR | 8 | API |
| Revisiones formales de Luis | 17 | API |
| Fechas distintas — solo formales | 6 | API |
| Revisiones basadas en comentarios (humanas) | 18 | API |
| Revisiones automatizadas (herramienta) | 3 | API |
| Fechas distintas — formales + comentarios | 11 | API |
| Revisiones sobre PRs propios (auto-revisión) | 0 | API |
| PRs de compañeros con corrección atribuida | 15 | API + git (mixta, ver sección 3) |

**Las tres listas de fechas definitivas:**

1. **PRs abiertos** — 2026-07-01, 2026-07-03, 2026-07-04, 2026-07-05, 2026-07-07, 2026-07-11, 2026-07-13, 2026-07-26 (8 fechas).
2. **Revisiones, solo formales** — 2026-07-03, 2026-07-04, 2026-07-05, 2026-07-13, 2026-07-20, 2026-07-24 (6 fechas).
   **Revisiones, formales + comentarios** — 2026-07-03, 2026-07-04, 2026-07-05, 2026-07-11, 2026-07-12, 2026-07-13, 2026-07-17, 2026-07-20, 2026-07-23, 2026-07-24, 2026-07-26 (11 fechas).
3. **Correcciones sobre código de compañeros** — 2026-07-04, 2026-07-05, 2026-07-11, 2026-07-12, 2026-07-13, 2026-07-17, 2026-07-20, 2026-07-23, 2026-07-24, 2026-07-26 (10 fechas).

> **Nota de zona horaria.** Los tres eventos cuyo día cambia según se lea en UTC o en UTC−04:00 son: la creación del PR #75 (2026-07-27T00:14:32Z UTC → 2026-07-26 local) y las revisiones de Luis en los PRs #68 y #71 (2026-07-25T02:15–02:16Z UTC → 2026-07-24 local). Ningún otro evento del informe cambia de día. Quien lea las marcas en UTC sin convertir atribuirá esas tres actividades a un día posterior.

> **Por qué las tres secciones NO se entrelazan.** Cada sección se apoya en una fuente y una granularidad distintas: la apertura de un PR es un hecho de API; una revisión es un evento sobre un PR ajeno; una corrección es, en varios casos, una inferencia a partir de commits y archivos. Colocarlas en una única línea temporal insinuaría una cadena causal —«este día abrió un PR, por lo tanto este otro día corrigió el código de alguien»— que los datos no sostienen: la API no registra la intención. Mantenerlas separadas preserva la evidencia y evita fabricar una narrativa.

## 1. PRs abiertos

12 pull requests abiertos por `LuisRojas260305`. La fecha de creación y la fecha de merge son **columnas separadas a propósito**: en el PR #75 difieren en dos días, y colapsarlas ocultaría ese intervalo. Todas las fechas en UTC−04:00.

| # | Título | Autor | Creado (UTC−04:00) | Merge (UTC−04:00) | Estado | head → base |
|---|---|---|---|---|---|---|
| #42 | chore: reorganizar docs, actualizar rulesets y guias de workflow | Luis Rojas | 2026-07-01 | 2026-07-01 | MERGED | `chore/reorganizar-docs` → `main` |
| #44 | Feature/backend/#11 or tools estructuras de datos | Luis Rojas | 2026-07-03 | 2026-07-03 | MERGED | `feature/backend/#11-OR-Tools-Estructuras-de-datos` → `main` |
| #46 | Test: Workflow auto-move project cards | Luis Rojas | 2026-07-03 | 2026-07-03 | MERGED | `test/auto-move-workflow` → `develop` |
| #47 | Test: Verify auto-move workflow #2 | Luis Rojas | 2026-07-03 | 2026-07-03 | MERGED | `test/auto-move-verify` → `develop` |
| #48 | Test: Final workflow verification | Luis Rojas | 2026-07-03 | 2026-07-03 | MERGED | `test/final-workflow-verify` → `develop` |
| #49 | fix(ci): apuntar board-screenshot y pr-project-status al proyecto de la org | Luis Rojas | 2026-07-03 | 2026-07-03 | MERGED | `develop` → `main` |
| #50 | fix: restore board-screenshot script reverted by PR #44 | Luis Rojas | 2026-07-04 | 2026-07-04 | MERGED | `fix/restore-board-screenshot` → `main` |
| #56 | refactor: binario único + compilación condicional + nomenclatura en español | Luis Rojas | 2026-07-05 | 2026-07-05 | MERGED | `feature/core/#55-refactor-cmake` → `develop` |
| #57 | feat(backend): modelo de datos entidades + diagrama ER (#20) | Luis Rojas | 2026-07-07 | 2026-07-07 | MERGED | `feature/backend/#20-modelo-datos` → `develop` |
| #60 | feat(middleware): CRUD handler stubs for teachers (Tarea 16) | Luis Rojas | 2026-07-11 | 2026-07-11 | MERGED | `feature/middleware/16-profesores-crud-handlers` → `develop` |
| #63 | fix(backend): correccion del PR #62 — build, convenciones, tests y performance | Luis Rojas | 2026-07-13 | 2026-07-13 | MERGED | `pr-62-review` → `develop` |
| #75 | feat(solver): Motor CP-SAT completo — restricciones, validacion, tests, benchmark | Luis Rojas | 2026-07-26 | 2026-07-28 | MERGED | `feature/backend/#25-#32-Modelo-CP-SAT` → `develop` |

**Fechas de creación (8 distintas):** 2026-07-01, 2026-07-03, 2026-07-04, 2026-07-05, 2026-07-07, 2026-07-11, 2026-07-13, 2026-07-26

**Fechas de merge (8 distintas):** 2026-07-01, 2026-07-03, 2026-07-04, 2026-07-05, 2026-07-07, 2026-07-11, 2026-07-13, 2026-07-28

Comandos: `gh pr list --state all --limit 100 --json ... --author LuisRojas260305`, y para el total `gh pr list --state all --limit 100 --json number --jq 'length'`.

## 2. Revisiones

Las revisiones se dividen en dos clases que la API separa de forma verificable y que **no deben sumarse entre sí sin etiquetarlas**: una *revisión formal* es un envío con estado (`APPROVED`, `CHANGES_REQUESTED`, `COMMENTED`, `DISMISSED`); un *comentario* es una nota en la conversación del PR sin estado de revisión. Muchas revisiones de este equipo son del segundo tipo.

### 2.1 Revisiones formales

17 revisiones formales sobre 10 PRs. Todas son sobre PRs de compañeros: **no hay auto-revisión** (0 revisiones sobre los 12 PRs propios).

| # | PR | Autor del PR | Fecha (UTC−04:00) | Estado | Commit revisado |
|---|---|---|---|---|---|
| 1 | #43 | Daniel Reyna | 2026-07-03 | `APPROVED` | `5b47bb06d52a` |
| 2 | #51 | Manuel García | 2026-07-04 | `CHANGES_REQUESTED` | `67c627b1d8d6` |
| 3 | #51 | Manuel García | 2026-07-04 | `APPROVED` | `048f1cb3f840` |
| 4 | #53 | Paola Peña | 2026-07-05 | `APPROVED` | `605a610b702c` |
| 5 | #54 | Nicole Sereno | 2026-07-05 | `COMMENTED` | `b2cf6650273f` |
| 6 | #54 | Nicole Sereno | 2026-07-05 | `APPROVED` | `b2cf6650273f` |
| 7 | #62 | Nicole Sereno | 2026-07-13 | `COMMENTED` | `eb05d3dba3c8` |
| 8 | #62 | Nicole Sereno | 2026-07-13 | `COMMENTED` | `eb05d3dba3c8` |
| 9 | #62 | Nicole Sereno | 2026-07-13 | `COMMENTED` | `eb05d3dba3c8` |
| 10 | #62 | Nicole Sereno | 2026-07-13 | `COMMENTED` | `eb05d3dba3c8` |
| 11 | #62 | Nicole Sereno | 2026-07-13 | `COMMENTED` | `eb05d3dba3c8` |
| 12 | #65 | Daniel Reyna | 2026-07-20 | `APPROVED` | `5090fd15f329` |
| 13 | #66 | Paola Peña | 2026-07-20 | `DISMISSED` | `96f87c4ad692` |
| 14 | #66 | Paola Peña | 2026-07-20 | `APPROVED` | `f312f1686869` |
| 15 | #67 | Nicole Sereno | 2026-07-20 | `APPROVED` | `075308a42ed8` |
| 16 | #68 | Manuel García | 2026-07-24 | `APPROVED` | `a608c9d82450` |
| 17 | #71 | Manuel García | 2026-07-24 | `DISMISSED` | `3de2c081b0e5` |

**Fechas de revisión formal (6 fechas distintas):**

- **2026-07-03** — 1 revisión (#43 APPROVED)
- **2026-07-04** — 2 revisiones (#51 APPROVED, #51 CHANGES_REQUESTED)
- **2026-07-05** — 3 revisiones (#53 APPROVED, #54 APPROVED, #54 COMMENTED)
- **2026-07-13** — 5 revisiones (#62 COMMENTED, #62 COMMENTED, #62 COMMENTED, #62 COMMENTED, #62 COMMENTED)
- **2026-07-20** — 4 revisiones (#65 APPROVED, #66 APPROVED, #66 DISMISSED, #67 APPROVED)
- **2026-07-24** — 2 revisiones (#68 APPROVED, #71 DISMISSED)

> Un día con tres revisiones cuenta como **un** día. El recuento de revisiones y el de fechas son deliberadamente distintos: el PR #62 concentra 5 revisiones formales el mismo día, y el PR #54 tiene 2 separadas por menos de un minuto.

### 2.2 Revisiones basadas en comentarios

La API devuelve además **comentarios** de conversación, sin estado de revisión. De los 21 comentarios de Luis, 18 son humanos y 3 automatizados (cubo C más abajo). De los 18 humanos, **9 caen en 7 PRs sin ninguna revisión formal registrada** (#58, #59, #61, #64, #69, #70, #72); los otros 9 complementan PRs que sí tienen revisión formal. Contar *solo* las revisiones formales perdería esta mitad de la actividad de revisión.

| PR | Autor del PR | Fecha (UTC−04:00) | Título del comentario |
|---|---|---|---|
| #51 | Manuel García | 2026-07-04 | Segunda Revisión — Cambios detectados |
| #51 | Manuel García | 2026-07-04 | ✅ Correcciones aplicadas |
| #58 | Manuel García | 2026-07-11 | Review — Tu PR fue integrado en uno nuevo |
| #58 | Manuel García | 2026-07-11 | Cerrado — reemplazado por PR #60 con la implementación correcta. |
| #59 | Paola Peña | 2026-07-11 | Review — Correcciones aplicadas |
| #61 | Daniel Reyna | 2026-07-12 | Revision y correccion del PR #61 |
| #62 | Nicole Sereno | 2026-07-13 | Revisión del PR — Comentario General |
| #62 | Nicole Sereno | 2026-07-13 | Revisión del PR — Resumen para tu aprendizaje |
| #62 | Nicole Sereno | 2026-07-13 | Cerrando este PR ya que los cambios fueron corregidos y mergeados en PR #63. Los |
| #64 | Manuel García | 2026-07-17 | Revisión Completa del PR #64 — Middleware |
| #64 | Manuel García | 2026-07-26 | Cerrado por solicitud del equipo. |
| #65 | Daniel Reyna | 2026-07-20 | Revision automatica — Checklist `pr-review` *(automatizada)* |
| #66 | Paola Peña | 2026-07-20 | Revision automatica — Checklist `pr-review` *(automatizada)* |
| #66 | Paola Peña | 2026-07-20 | Hola Paola! El PR está muy bien estructurado, sigue el patrón del módulo de doce |
| #67 | Nicole Sereno | 2026-07-20 | Revision automatica — Checklist `pr-review` *(automatizada)* |
| #68 | Manuel García | 2026-07-23 | Revisión PR #68 — QHash Route Map + Timeout + Tests |
| #68 | Manuel García | 2026-07-24 | Hola magrmanuel25! Encontre algunos aspectos que podemos mejorar en este PR: |
| #69 | Manuel García | 2026-07-23 | Revisión PR #69 — Tests Edge Cases |
| #70 | Paola Peña | 2026-07-24 | Code Review — Análisis Spec-Driven |
| #71 | Manuel García | 2026-07-24 | Hola magrmanuel25! Encontre algunos aspectos que podemos mejorar: |
| #72 | Nicole Sereno | 2026-07-26 | Code review |

### 2.3 Fechas consolidadas de revisión

| Criterio | Fechas distintas | Lista |
|---|---|---|
| Solo revisiones formales | 6 | 2026-07-03, 2026-07-04, 2026-07-05, 2026-07-13, 2026-07-20, 2026-07-24 |
| Formales + comentarios (incluyente) | 11 | 2026-07-03, 2026-07-04, 2026-07-05, 2026-07-11, 2026-07-12, 2026-07-13, 2026-07-17, 2026-07-20, 2026-07-23, 2026-07-24, 2026-07-26 |

Se reportan **ambos** criterios y la diferencia es el hallazgo: la lista «solo formales» (6 fechas) oculta 5 días en los que sí hubo revisión documentada. Ningún total de este informe usa uno u otro sin nombrarlo.

**Revisiones sobre PRs de compañeros vs. sobre PRs propios:** las 38 revisiones (formales y comentarios) recaen todas sobre PRs de terceros. Los 12 PRs de Luis no tienen ni una sola revisión suya. Esto es consistente con la política del equipo y descarta la doble contabilidad por auto-revisión.

Comandos: `gh pr view <n> --json reviews,comments` sobre los 32 PRs, filtrando `author.login == "LuisRojas260305"`.

## 3. Correcciones sobre código de compañeros

15 pull requests sobre los que hay evidencia de que Luis corrigió, bloqueó o resolvió el trabajo de otra persona. Cada fila declara **qué interpretación satisface**; las interpretaciones no se fusionan porque no tienen la misma solidez probatoria.

| # | PR | Autor | Fecha (UTC−04:00) | Interpretación | Evidencia | Proveniencia |
|---|---|---|---|---|---|---|
| 1 | #51 | Manuel García | 2026-07-04 | `revert` | revisión CHANGES_REQUESTED con 6 issues bloqueantes; revert explícito en 3d875a8 | API |
| 2 | #54 | Nicole Sereno | 2026-07-05 | `review-driven fix` | revisión COMMENTED y APPROVED; correcciones sobre la rama de la compañera | API |
| 3 | #58 | Manuel García | 2026-07-11 | `review-driven fix` | comentarios de revisión; cerrado y reemplazado por el PR #60 de Luis | API |
| 4 | #59 | Paola Peña | 2026-07-11 | `review-driven fix` | comentario «Review — Correcciones aplicadas» | API |
| 5 | #61 | Daniel Reyna | 2026-07-12 | `review-driven fix` | comentario «Revision y correccion del PR #61»; commit ba2a919 | API |
| 6 | #62→#63 | Nicole Sereno | 2026-07-13 | `review-driven fix` | 5 revisiones formales; corregido en el PR #63 de Luis (commit 6781930) | API |
| 7 | #64 | Manuel García | 2026-07-17 | `review-driven fix` | comentario «Revisión Completa del PR #64 — Middleware» | API |
| 8 | #65 | Daniel Reyna | 2026-07-20 | `review-driven fix` | revisión formal APPROVED + revisión automatizada | API |
| 9 | #66 | Paola Peña | 2026-07-20 | `review-driven fix` | revisiones DISMISSED y APPROVED + revisión automatizada | API |
| 10 | #67 | Nicole Sereno | 2026-07-20 | `review-driven fix` | revisión formal APPROVED + revisión automatizada | API |
| 11 | #68 | Manuel García | 2026-07-23 | `review-driven fix` | revisión formal APPROVED + 2 comentarios | API |
| 12 | #69 | Manuel García | 2026-07-23 | `review-driven fix` | comentario «Revisión PR #69 — Tests Edge Cases»; cerrado sin merge | API |
| 13 | #70 | Paola Peña | 2026-07-24 | `review-driven fix` | comentario «Code Review — Análisis Spec-Driven» | API |
| 14 | #71 | Manuel García | 2026-07-24 | `review-driven fix` | revisión formal DISMISSED + comentario | API |
| 15 | #72 | Nicole Sereno | 2026-07-26 | `review-driven fix` | comentario «Code review» con 7 issues (3 críticos); cerrado, reemplazado por #74 | API |

**Interpretaciones, por separado:**

- **`revert`** (1) — el trabajo del compañero fue revertido explícitamente por Luis
  - #51
- **`review-driven fix`** (14) — existe una revisión de Luis que motivó la corrección, y el resultado se materializó en un commit o PR propio
  - #54, #58, #59, #61, #62→#63, #64, #65, #66, #67, #68, #69, #70, #71, #72
- **`conflict-resolution`** (0) — la corrección resolvió un conflicto de integración, no una revisión
- **`file-last-touched`** (0) — corrección inferida por los archivos que Luis tocó después; no hay un campo de API que la confirme

### 3.1 Correcciones post-merge sin ninguna revisión

Categoría aparte, deliberadamente **no** fusionada con «corrección dirigida por revisión». Los commits `1332ab1`, `42bb963`, `b17755f` (2026-07-26) corrigen el código de los PRs #73 y #74 **y no tienen asociada ninguna revisión, formal ni comentario**. Son correcciones posteriores al merge hechas sin revisión asociada; clasificarlas como «corrección dirigida por revisión» sería afirmar una causa que la API no registra.

| Commit | Autor | Fecha (UTC−04:00) | Asunto |
|---|---|---|---|
| `1332ab1` | Luis Rojas | 2026-07-26 | fix(backend,middleware): code review fixes for PR #74 |
| `42bb963` | Luis Rojas | 2026-07-26 | Merge PR #73 (Daniel — Frontend+Middleware) into develop |
| `b17755f` | Luis Rojas | 2026-07-26 | fix(middleware): add SolicitudPendiente doc + trailing newline |

Comandos: `git show -s --format='%H|%an|%ad|%s' --date=iso-strict <sha>`, y la correlación con `gh pr view --json reviews,comments` de los PRs #73 y #74 (0 revisiones de Luis).

## Cubos deliberadamente excluidos de todo total

Los cuatro cubos siguientes se cuentan y se muestran, pero **no se suman a ninguna cifra de las secciones 1–3**. Mezclarlos con la actividad humana inflaría los totales.

### A. `luis@opencode.ai` — identidad de agente de IA

Esta dirección no aparece en `.mailmap` ni en la lista de autores canónicos. Aparece únicamente como trailer `Co-authored-by`, no como autor del commit. **El comando `git log --all --format='%H|%an|%ae|%s' | grep -c 'luis@opencode.ai'` devuelve 0** porque el formato solo cubre autor y asunto, no el cuerpo. Contando en el cuerpo completo:

| Métrica | Valor |
|---|---|
| Commits que mencionan la dirección en el cuerpo | 1 |
| Ocurrencias del trailer | 2 |

- `ba2a919` — autor **Daniel Reyna**, 2026-07-12 — feat(frontend): Implementacion de dashboard y navegacion (#61)

### B. Crédito de colaboración

Trailer `Co-authored-by: Luis <luisalexanderrojasguevara@gmail.com>` en commits **cuya autoría es de otra persona**. Es crédito recolectado, no autoría: por eso no suma a los totales de las secciones 1–3.

**8 commits**, en 8 PRs distintos.

| Commit | Autor real | PR | Fecha (UTC−04:00) |
|---|---|---|---|
| `05366ac` | Paola Peña | #59 | 2026-07-11 |
| `ba2a919` | Daniel Reyna | #61 | 2026-07-12 |
| `bac45c3` | Daniel Reyna | #43 | 2026-07-03 |
| `bc778a2` | Paola Peña | #66 | 2026-07-20 |
| `c668eb4` | Paola Peña | #53 | 2026-07-05 |
| `e7cc73e` | Nicole Sereno | #54 | 2026-07-05 |
| `e919050` | Manuel García | #71 | 2026-07-26 |
| `f16335c` | Manuel García | #68 | 2026-07-25 |

Comando: `git log --all --format='%B' | grep -ci 'co-authored-by: Luis'` devuelve **12 apariciones**; el recuento de *commits* distintos es **8**, porque `ba2a919` repite el trailer y dos commits de la lista tienen a Luis como autor.

### C. Revisiones automatizadas

Comentarios titulados *«Revision automatica — Checklist `pr-review`»*. Los genera una herramienta, no una persona: **no cuentan como juicio humano** y quedan fuera de los totales de la sección 2.

| PR | Autor del PR | Fecha (UTC−04:00) | Confianza |
|---|---|---|---|
| #65 | Daniel Reyna | 2026-07-20 | 82/100 |
| #66 | Paola Peña | 2026-07-20 | 95/100 |
| #67 | Nicole Sereno | 2026-07-20 | 85/100 |

### D. Revisiones `DISMISSED`

2 de las 17 revisiones formales tienen estado `DISMISSED` (el autor la anuló). Se cuentan porque el trabajo ocurrió, pero se etiquetan para que nadie los sume sin conocer su estado.

| PR | Fecha (UTC−04:00) | Estado |
|---|---|---|
| #66 | 2026-07-20 | `DISMISSED` |
| #71 | 2026-07-24 | `DISMISSED` |

**Totales con y sin `DISMISSED`:** revisiones formales **con** = 17, **sin** = 15. La lista de fechas de la sección 2.1 incluye las 2: ninguna revisión de este informe cambia de día al excluirlas.

## Dos hallazgos que se reportan sin interpretar

Ambos se consignan como hecho. La interpretación corresponde a Luis, no a este informe.

### 1. El commit `6781930` lleva un trailer ajeno

| Campo | Valor |
|---|---|
| Commit | `678193081fe4b0a15e4dc2c033cd3d4081526320` |
| Autor | Luis Rojas |
| Asunto | fix(backend): correccion del PR #62 — build, convenciones, tests y performance (#63) |
| Trailer | `Co-authored-by: Niko <serenonicole122@gmail.com>` |

Es el commit de la propia corrección del PR #62 y ese commit acredita a otra persona. El informe no afirma si fue cortesía deliberada ni un error de copiado: ambos son indistinguibles con los datos disponibles.

Comando: `git show --format='%H%n%an%n%B' 6781930 | grep -i 'co-authored-by'`.

### 2. Discrepancia de nombre en el PR #58

El primer comentario de revisión de Luis en el PR #58 abre *«Hola Emmanuel»*, pero la autoría del PR es `magrmanuel25` (Manuel García). O bien es un error en el texto, o bien existe una identidad que el historial de git no contiene: **ningún commit del repositorio pertenece a un autor llamado Emmanuel** (autores canónicos: Luis Rojas, Daniel Reyna, Manuel García, Nicole Sereno, Paola Peña). Se consigna como **bandera de calidad de datos abierta**, sin resolver y sin conjeturar.

Comandos: `gh pr view 58 --json author` y `gh pr view 58 --json comments`.

## Discrepancias con la exploración previa

Este informe re-derivó cada número con comandos propios. Seis afirmaciones previas **no** resistieron la verificación; se corrigen aquí y en las secciones correspondientes.

| Afirmación previa | Verificación propia | Comando de prueba |
|---|---|---|
| **18 revisiones formales** | **17** (10 PRs) | `gh pr view <n> --json reviews` por cada PR y cruce con `gh api .../pulls/<n>/reviews` (ambos coinciden en 17) |
| **22 comentarios de revisión, todos en PRs sin revisión formal** | **21 comentarios** en total (18 humanos + 3 automatizados); de esos 21, solo **9** caen en los 7 PRs sin revisión formal (#58, #59, #61, #64, #69, #70, #72) | `gh pr view <n> --json comments` por PR |
| La cifra **22** | El comentario #21-extra es el de la **issue #45**, que **no es un PR** («Test: Workflow auto-move project cards»); no es una revisión y no cuenta | `gh api .../issues/45/comments` vs `gh pr view 45` (GraphQL: no existe el PR 45) |
| Confianza de revisión automatizada **(82/100, 85/100)** | **(82/100, 95/100, 85/100)** — el PR #66 tiene **95/100**, no 85 | `gh pr view 66 --json comments` |
| `grep -c 'luis@opencode.ai'` devuelve la cantidad de commits | Devuelve **0**: la dirección es un trailer `Co-authored-by` (2 apariciones en 1 commit), nunca un email de autor | `git log --all --format='%H|%an|%ae|%s' \| grep -c 'luis@opencode.ai'` |
| Crédito de colaboración: **9 commits** en PRs #43, #44, #49, #53, #54, #59, #66, #68, #71 | **8 commits** con autor distinto de Luis, en PRs #43, #53, #54, #59, #61, #66, #68, #71. Los commits de #44 y #49 los firma el propio Luis (no son crédito ajeno) y `ba2a919` (#61) sí lleva el trailer | `git log --all --format='%B' \| grep -i 'co-authored-by: Luis'` |

Se confirman sin cambios: 32 PRs; 12 PRs de Luis; 8 fechas de creación; 8 fechas de merge; cero auto-revisiones; 2 `DISMISSED`; 11 fechas de revisión (6 formales + 5 de comentarios); 15 PRs con corrección atribuida; y los tres eventos que cambian de día en UTC (creación del PR #75, revisiones en #68 y #71).

## Apéndice — Metodología y reproducibilidad

**Reproducibilidad.** El informe se genera con `python3 scripts/fechas-pr-revision-correccion.py` y no lleva marca de tiempo: dos ejecuciones consecutivas sobre el mismo estado del repositorio y la misma data de GitHub producen un archivo idéntico byte a byte. No se ejecutó `git fetch`; el clon local es idéntico a `origin/main` (`3d875a80…`) y `origin/develop` (`9916c935…`). No se usó SSH: `git ls-remote` por SSH falla en esta máquina por ausencia de clave, de modo que toda la evidencia remota proviene de la API REST/GraphQL vía `gh`.

**Fuentes.** GitHub (`gh pr list`, `gh pr view`, API REST) para PRs, revisiones y comentarios; `git log --all` para autoría, trailers y commits. La API de GitHub es la **única** fuente posible para el estado de revisión: al fusionar con squash no queda rastro de revisiones en ningún objeto de git, y los refs `pull/*`, las notas y los trailers `Reviewed-by:` estaban vacíos.

**Zona horaria.** Git devuelve marcas con offset; la API devuelve UTC. Todo se convierte a epoch y se agrupa por día en **UTC−04:00**, el offset del repositorio. Agrupar en UTC desplazaría tres eventos al día siguiente (creación del PR #75; revisiones en los PRs #68 y #71).

**Proveniencia por fila.** Cada dato del informe lleva una de estas etiquetas: `API` (respuesta literal de GitHub), `git` (salida de `git log`/`git show`), `inferencia` (deducción a partir de varios hechos, marcada fila a fila) o `estimado`. Ninguna inferencia se presenta como hecho de API.

**Guardas de truncado.** El script reimprime la forma de la advertencia del script hermano: si `gh pr list` devolviera 100 registros o más, emitiría `AVISO: gh pr list alcanzó el límite de 100 — posible truncado` y el informe lo declararía. En esta ejecución devolvió 32, muy por debajo del límite; el total se contrastó además con `search/issues` (`total_count` = 32), que coincide.

**Qué NO hace este informe.** No estima horas ni duración. No atribuye intención. No convierte una fecha de revisión en una fecha de corrección: la sección 2 y la sección 3 son independientes y no se cruzan. No atribuye a Luis actividad de sus compañeros salvo el crédito de colaboración, que se reporta por separado y no suma.

### Comandos de verificación

| Comando | Resultado observado en esta ejecución |
|---|---|
| `gh auth status` | `LuisRojas260305`, scopes `repo`, `read:org` |
| `gh pr list --state all --limit 100 --json number --jq 'length'` | `32` |
| `gh api .../pulls?state=all&per_page=100` + `.../pulls/<n>/reviews` (cruce REST) | `17` revisiones formales de Luis — coincide con `gh pr view` |
| `gh pr list --state all --limit 100 --author LuisRojas260305 --json number --jq 'length'` | `12` |
| `git log --all --format='%H|%an|%ae|%s' \| grep -c 'luis@opencode.ai'` | `0` — la dirección solo aparece en el cuerpo, nunca en el campo de autor |
| `git log --all --format='%B' \| grep -ci 'co-authored-by: Luis'` | `12` (apariciones, no commits) |
| `git show --format='%H%n%an%n%B' 6781930 \| head -20` | trailer `Co-authored-by: Niko <serenonicole122@gmail.com>` confirmado |
| `git status --short` | solo aparecen los dos archivos de este informe (script + markdown) sobre la suciedad preexistente declarada |

