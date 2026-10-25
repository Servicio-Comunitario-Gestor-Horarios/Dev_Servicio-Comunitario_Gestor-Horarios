# Estado 010

Fase: validacion
Aprobaciones: spec 2026-10-23 · plan/tareas 2026-10-25
Base: e72c930 (base del plan, cierre de la 009) · aa7ee74 (HEAD, base efectiva de la implementación y del diff de la re-validación)
Dudas abiertas: ninguna
Siguiente paso: re-validación del reviewer contra `aa7ee74` (suite 49/49); con `VEREDICTO: APROBADO`, el planner marca la spec `implementada (fecha)` y cierra este estado. La 011 (hora académica) va después.
Veredicto: pendiente
QA: APROBADA con las notas del re-QA aplicadas el 2026-10-23; spec aprobada por el usuario el 2026-10-23.
QA del plan: escrito el 2026-10-24, ajustado el 2026-10-25 (RF-1 con `Resultado<QJsonObject>`) y corregido tras el QA del plan (11 hallazgos, sección propia); aprobado por el usuario el 2026-10-25 junto con `tasks.md`.
Notas: T1–T10 implementadas y commiteadas (T7 = `f054b2e`, desviación autorizada de P4 = `06fc70e`); la corrección del hallazgo de validación está en `aa7ee74` y falta la re-validación.

## Validación (2026-10-25)

- **Veredicto del reviewer: CAMBIOS NECESARIOS.** Hallazgo principal (RF-6): `causaConflictoDeAula`
  contaba solo los bloques que se quedan en el aula de referencia y **no descontaba** las franjas que
  dejan bloqueadas las clases desplazadas del grupo; la saturación por tipo tampoco. Una entrada
  válida podía ser INFEASIBLE y quedarse sin categoría de causa («no se pudo identificar una
  categoría concreta»).
- **Corregido en `aa7ee74`** (solo `src/backend/src/solver/solver.cpp` y
  `test/backend/test_solver_horarios.cpp`): capacidad disponible = franjas del grupo menos un bloque
  por bloque desplazado (limitado al turno del curso) y, por tipo, se descuentan las franjas
  bloqueadas en las referencias de ese tipo. Tres casos nuevos y uno ajustado; `ctest --preset
  backend` **49/49** en `aa7ee74`.
- **Aplicado antes del veredicto**: la desviación **autorizada por el usuario fuera del plan** sobre
  P4 (`06fc70e`): `ValidadorSalidaSolver` indexa la ocupación por `(profesor, día, slot)` y
  `(aula, día, slot)` en lugar de solo por franja.
- **Falta la re-validación** del reviewer contra `aa7ee74`; todavía no hay `VEREDICTO: APROBADO`.

## Decisiones cerradas (2026-10-23)

- **Reparto en dos specs.** La 010 recibe todo lo del **solver**: contrato 2.0, regla de tipo,
  validaciones, supersesión de la 005, enmienda constitucional, documentación del contrato y diagrama
  entidad-relación. La 009 conserva persistencia, servicios, esquema, seed y documentación de la API.
- **Regla de tipo dura siempre.** Toda clase se imparte en un aula del tipo exacto requerido por su
  materia, en los dos modos. El **aula fija es el aula de referencia**: se usa cuando su tipo coincide;
  si no, la clase se desplaza a un aula del tipo requerido y el curso **vuelve**.
- **Aula de referencia vacía y bloqueada (decisión del usuario).** Mientras el curso tiene una clase
  desplazada en una franja, su aula de referencia **no aloja ninguna clase y ningún otro curso puede
  usarla** en esa franja. La reserva no se extiende a franjas en las que el curso no tiene clase.
- **Exclusividad.** Dos cursos no comparten aula en la misma franja, incluidas las clases desplazadas.
  La competición por el mismo tipo no se rechaza antes de resolver: si no hay solución, el resultado es
  **no factible con causa conflicto de aula**.
- **Contrato.** Solo se declara y acepta la versión `2.0`; 1.0, ausente o desconocida se rechaza como
  **entrada inválida**, sin conversión ni tipos por defecto. Los presets 1.0 en disco quedan intactos y
  se rechazan al usarlos.
- **Salida.** Cada clase informa el **aula real** donde se impartió, también en modo de aula fija.
- **Duda diferida (fuera de alcance): la cancha compartida.** Se deja con exclusividad; si el usuario
  decide compartirla, se abre un cambio de requisitos.

## Correcciones del QA del plan (2026-10-25)

Once hallazgos aplicados; el plan sigue con **10 tareas** (T1–T10, 30/30/30/30/20/20/20/20/20/20 min).

1. **Fixture de T2 versionado.** `datos-prueba/` está en `.gitignore:35`; se sustituye por
   `test/data/preset-1.0.json` (nuevo, versionado), cargado con `QFINDTESTDATA`, con hash de bytes
   antes/después. El preset real solo se menciona en la documentación.
2. **Bloqueo de la referencia.** El bucle recorre solo las claves de `vars.aulaAssignment` con
   `(d, s, r)` existentes (los turnos mixtos no crean `y` en todas las franjas).
3. **§7.4 sin detalle.** Solo rango `V1–V15` → `V1–V17`, constancia de que V1 habla de eventos y
   remisión a la 005/010; no se duplican V2/V16/V17.
4. **Documentación viva fuera de `docs/solver/`.** `docs/contrato-solver-2.0.md` (la constitución
   declara `docs/solver/` histórico y no obligatorio).
5. **T1/T2/T3 reequilibrados.** T1 = contrato 2.0 (incluida la sección `eventos`) + constructor +
   consumidores que compilan; T2 = versión y cargador; T3 = casos V1/V2/V16/V17 (V1 ya cambiado en
   T1, aquí solo sus pruebas); los `-R` ya no arrastran tests de otras tareas.
6. **Benchmark con estrategia fija.** 6 aulas (5 comunes + 1 laboratorio), solo `Tecnología` requiere
   laboratorio, referencias repartidas por turno; T10 exige factibilidad y < 60 s.
7. **Seed no generable.** ≈104 bloques de ciencias contra 1 laboratorio × 40 franjas; anotado en el
   plan y en T10, sin intentar generarlo.
8. **T5 es solo prueba.** `ValidadorSalidaSolver` ya usa `asignacion.aula`; no cambia producción.
9. **Un solo nombre de campo.** `SolverConfig` usa `tipo`; la guarda de RF-1 mira `tipoAula` del DTO y
   los fixtures deben fijar el nombre, no solo `idTipoAula`.
10. **`TurnoConfig.inicio/fin`** se rellenan en el constructor desde el `TurnoDTO`; sin eso V2 falla.
11. **Cita corregida:** `test/CMakeLists.txt:120-133`.

## Decisión de plan (2026-10-25)

- **RF-1: la construcción falla si falta un tipo.** `construirJsonEntrada` y la fachada
  `ConstructorEntradaSolver::construir` devuelven **`Resultado<QJsonObject>`**. Si un aula o una
  materia de la instantánea no declara tipo, la construcción devuelve `error` con un mensaje en
  español que nombra la entidad y **no emite JSON**. Se descartan «copiar el tipo vacío y dejar que
  V16 lo rechace» y «comprobación previa separada». El builder del frontend queda como **rotura
  conocida** (fuera de alcance; el preset `backend` no lo compila). Detalle en `plan.md` §1.1, §3.1 y
  decisión 12.

## Decisiones y correcciones del re-QA (2026-10-23)

- **N3 — Entrega y verificación.** Dentro: backend, datos, seed, consola y sus pruebas, que compilan y
  pasan. Fuera: la interfaz y los tests ligados a ella. La construcción debe poder excluir la interfaz
  (requisito de la 009, RNF-6); el detalle técnico es del plan. RNF-5 lo exige en esta spec.
- **N7 — V2 alineada con la 009.** Valida turno existente, nombre no vacío, posición 1..franjas,
  orden ≥ 1, duración > 0, par inicio/fin, `fin − inicio = duración`, ventana del turno y de la
  posición, orden cronológico y ausencia de solapes dentro y entre posiciones.
- **N8 — Documentación del contrato.** Se asigna a un **RF propio de la 010 (RF-10)**, no a la guía de
  servicios de la 009: forma 2.0, tipos por nombre y clave, sección de eventos, salida con aula real,
  rechazo de 1.0 y semántica del aula de referencia. La guía de la 009 sigue siendo la de servicios.
- **N9 — Dependencia.** La 009 precede a la 010; la 010 consume los tipos persistidos, los eventos con
  identidad y orden, los servicios, la huella y el esquema que la 009 deja listos.
- **N10 — Enmienda constitucional exacta.** §7.1.5 (línea 64, invariante del aula), §7.3 (línea 78,
  contrato de configuración), §7.4 (línea 87, rango V1–V15 → **V1–V17**; la constitución no define el
  contenido de V1, solo lo menciona: se actualiza el rango, se anota que V1 habla de eventos y el
  detalle de V1 se cambia en la 005), §7.5 (línea 91, «el receso es metadata» → eventos) y §5
  (línea 34, dominios protegidos: recesos → eventos y añadir tipos de aula).
- **N11 — Normalización.** La normalización es **solo para comparar**; el nombre visible se conserva
  tal cual. La equivalencia de ida y vuelta se evalúa sobre la clave de tipo.
- **N12 — RF-6 en EARS.** «Si no existe solución factible y el bloqueo es de aulas, la causa es
  conflicto de aula».
- **N13 — RF-2.** El aviso de archivo se limita a presets en disco; se elimina «abrir con una versión
  compatible» y la acción es generar una 2.0 desde los datos.
- **N14 — Duplicado.** La regla de competición por el mismo tipo queda solo en RF-3; se quitó de RF-5
  (no es validación previa).
- **N15 — Criterio 2 de la 005.** Se añade a la lista de supersesión: se mantiene salvo D10.
- **N16 — Numeración.** Se unifica «V16 y V17» en toda la spec; no queda «V16+».
- **N17 — Identidad del evento.** No viaja en el contrato ni forma parte de la equivalencia; los
  eventos se distinguen por nombre, posición y orden.
- **N19 — Base.** Fijada en **`e72c930`**, cierre de la 009; HEAD = `e72c930`. La 009 está
  implementada y el árbol limpio; no hay diff de esquema sin commitear que imputar a la
  implementación.

## Supersesión de la 005 (declarada en RF-7)

- Definición «Curso con aula fija: usa siempre su aula».
- Invariante **D10**.
- **Criterio de finalización 2** (005:745-748, «D1–D11 y D13»), salvo D10.
- Salida del horario «con aula fija, su aula fija».
- **RF-7** de la 005.
- Versión **1.0** (definición, RF-10 y criterio de finalización).
- **V1** que exige la sección de recesos.
- **D9 se amplía** y **D8 se extiende**; **V8 se mantiene**.
- El resto de la 005 sigue vigente. La anotación en el texto de la 005 la hizo el planner en T7
  (commiteada en `f054b2e`); el implementer no edita specs.

## Notas de proceso

- **La enmienda constitucional la ejecuta el planner.** Hecha en `f054b2e` (T7) junto con la
  anotación de supersesión de la 005; el implementer solo verifica.
- **Plan y tareas aprobados (2026-10-25).** `plan.md` y `tasks.md` se escribieron el 2026-10-24 y el
  usuario los aprobó el 2026-10-25; la implementación se hizo en orden T1–T10 y está commiteada.
- **RF-1 cerrado (2026-10-25).** La construcción devuelve `Resultado<QJsonObject>` y falla si un aula
  o una materia no declara tipo; la confirmación que estaba pendiente queda resuelta y ya no hay dudas
  abiertas.
- **Preset 1.0 de prueba.** `test/data/preset-1.0.json` (versionado) es el fixture de rechazo de T2,
  leído con `QFINDTESTDATA` y verificado por hash de bytes. `datos-prueba/preset-generacion-31059.json`
  no está versionado (`.gitignore:35`) y solo se cita en la documentación como ejemplo a regenerar.
- **T7 (2026-10-25), hecha y commiteada (`f054b2e`).** La enmienda de `docs/constitution.md` (§5,
  §7.1.5, §7.3, §7.4 y §7.5) **y** la anotación de supersesión de
  `specs/005-nucleo-solver/spec.md` están aplicadas: aula fija, **D8/D9 ampliadas**, **D10**, salida
  con **aula real**, **RF-7**, versión **1.0**, **V1** (eventos), **V2** (+ **011** futura en
  `duracion_minutos`), **V8 mantenida** y el catálogo a **V1–V17**. El parche del planner
  (`/tmp/opencode/constitution-t7.patch`, §5/§7.1.5/§7.3/§7.4/§7.5) lo aplicó el coordinator, porque
  el subagente planner no escribe fuera de `specs/`; el implementer lo verificó con los dos `grep` de
  la tarea. **No queda nada pendiente de la constitución ni de la 005.**
- **Desviación autorizada fuera del plan: P4 (2026-10-25, `06fc70e`).** El usuario autorizó arreglar
  el defecto preexistente de `ValidadorSalidaSolver` (la ocupación se indexaba solo por franja):
  ahora las claves son `(profesor, día, slot)` y `(aula, día, slot)`. Sin este arreglo, cualquier
  horario con dos cursos en paralelo se marcaba `CalculoFallido` en `ServicioGeneracion`. Detalle en
  `plan.md` §1.2 y en `journal.md` («P4 · claves por aula y docente»).

## Tamaño previsto

Diez RF en **diez tareas** (20-30 min), con build sin frontend y el benchmark de 60 s en T10.
