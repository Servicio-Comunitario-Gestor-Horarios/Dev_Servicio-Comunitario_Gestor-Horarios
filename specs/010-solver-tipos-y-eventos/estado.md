# Estado 010

Fase: plan
Aprobaciones: spec 2026-10-23
Base: 5d6b1da77c9da645441a0bebb38e37753f0f7198 (HEAD). **Condición:** el cambio de esquema está **sin commitear** en el árbol; el diff no forma parte de la base y la validación futura no debe imputarlo a la implementación. La base efectiva será el commit que incluya el esquema inicial.
Dudas abiertas: ninguna
Siguiente paso: el planner escribirá `plan.md` y `tasks.md` de la 009 cuando el usuario confirme la base de commit.
Veredicto: pendiente
QA: APROBADA con 5 notas menores, aplicadas el 2026-10-23; spec aprobada por el usuario el 2026-10-23.

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
- **N19 — Base.** Anotada la condición del esquema WIP sin commitear (ver cabecera).

## Supersesión de la 005 (declarada en RF-7)

- Definición «Curso con aula fija: usa siempre su aula».
- Invariante **D10**.
- **Criterio de finalización 2** (005:745-748, «D1–D11 y D13»), salvo D10.
- Salida del horario «con aula fija, su aula fija».
- **RF-7** de la 005.
- Versión **1.0** (definición, RF-10 y criterio de finalización).
- **V1** que exige la sección de recesos.
- **D9 se amplía** y **D8 se extiende**; **V8 se mantiene**.
- El resto de la 005 sigue vigente. La anotación en el texto de la 005 la hará el planner en la fase
  de plan; el implementer no edita specs.

## Notas de proceso

- **Autoría de la enmienda constitucional.** La skill sdd dice que el implementer no edita
  `docs/constitution.md`; la 010 la exige como tarea propia del plan. El plan debe asignarla al
  planner/usuario o contar con autorización explícita para que la ejecute otro agente.
- **No hay plan ni tareas todavía.** Esta fase termina con la aprobación de la spec.

## Tamaño previsto

Diez RF agrupables en **≤10 tareas**: contrato y versión, rechazo de configuraciones anteriores, regla
de tipo, entrada/salida, validaciones, causas, supersesión, constitución, diagrama y documentación del
contrato.
