# Estado 009

Fase: implementacion
Aprobaciones: spec 2026-10-23 · plan/tareas 2026-10-23
Base: 471311c (commit docs(specs): plan y tareas de la 009 aprobados; contiene el esquema de 059168f). HEAD = 471311c.
Dudas abiertas: ninguna
Siguiente paso: implementar **T1** (build sin frontend) en una sola tarea y una sola pasada. Tras cada tarea, el coordinator ejecuta la verificación del plan y `git diff --stat`; no se commitea sin permiso del usuario.
Veredicto: pendiente
QA: APROBADA con 5 notas menores, aplicadas el 2026-10-23; spec aprobada por el usuario el 2026-10-23.
QA del plan: CAMBIOS NECESARIOS corregidos y `plan.md`/`tasks.md` aprobados por el usuario el 2026-10-23.

## Decisiones cerradas (2026-10-23)

- **Reparto en dos specs.** La 009 se queda con **persistencia y servicios** (esquema, marca de
  versión, catálogo de esquema, servicios, cascada, huella, seed y documentación de la API). Todo lo
  del **solver** pasa a la **010**: contrato 2.0, regla de tipo, validaciones, supersesión de la 005,
  constitución, documentación del contrato y diagrama entidad-relación. Las dos specs se enlazan y la
  009 **precede** a la 010.
- **Eventos.** Varios eventos por franja con **identidad propia**; el **orden persistente** dentro de
  la posición es una consecuencia derivada de la presentación estable, no una decisión literal del
  usuario. Se queda en la 009.
- **Frontend fuera de alcance.** La 001 ya declara la interfaz fuera de alcance; esta spec mantiene la
  frontera. En su lugar exige **documentar la API** (`docs/interfaz-frontend.md`). Las firmas cambian
  **en limpio**; se acepta como condición conocida que el frontend no compilará hasta que su equipo lo
  adapte. La consola **no** es frontend: sus comandos se actualizan aquí. Las heurísticas de la
  interfaz pasan a ser deuda documentada de su equipo.
- **Seed.** Debe seguir aplicándose de forma **determinista** con tipos y eventos y recuentos
  coherentes, pero **no** se exige generabilidad con la regla dura de tipo; eso **supersede la
  promesa de generabilidad de la 008**. El conjunto se sustituirá por los datos reales del liceo. No
  se inventan aulas nuevas.

## Decisiones de modelado del re-QA (2026-10-23)

- **Clave normalizada almacenada (decisión del usuario, 2026-10-23).** El catálogo de tipos de aula
  **guarda la clave normalizada** (recorte de espacios extremos + `toCaseFolded()` Unicode, calculada
  por el servicio) y el **índice único va sobre esa clave**. Corrige la definición de «clave de tipo» y
  RF-5 de la spec, que sigue **aprobada**. Motivo: `lower()`/`trim()` de SQLite solo pliegan ASCII y no
  frenan «Física»/«FÍSICA» en escrituras directas.
- **N1 — Regla medible del seed.** Cada materia sembrada recibe el tipo especializado de su área
  (ciencias experimentales → laboratorio; informática/taller → taller; Educación Física → cancha) **si
  el conjunto tiene al menos un aula de ese tipo**; en caso contrario, el tipo genérico. Así se cumple
  «todo tipo requerido tiene al menos un aula» sin inventar una cancha, y el criterio es comprobable a
  nivel de datos.
- **N2 — Puente de la entrada del solver.** Se elige **(a)**: la 009 mantiene la emisión de la sección
  de **recesos** del contrato 1.0 **derivada de los eventos** hasta que la 010 la sustituya, para que
  el backend y sus pruebas sigan compilando y pasando. No añade tipos ni eventos al contrato. El
  puente es temporal y la 010 lo elimina. «Contrato observable» se acota a servicios, consola y
  documentación de API.
- **N3 — Entrega y verificación.** Dentro: backend, datos, seed, consola y sus pruebas, que compilan y
  pasan. Fuera: la interfaz y los tests ligados a ella. La construcción debe poder **excluir la
  interfaz** para que la política de pruebas se cumpla con backend/datos/seed/consola (RNF-6); el
  detalle técnico es del plan.
- **N4 — Posición del evento.** Se reduce a **1..número de franjas** (convención 1-based de la 008:
  tras la k-ésima franja). Se descarta la posición 0 porque el turno empieza en la primera franja y no
  hay tiempo antes; la contradicción con «intervalo dentro de la ventana del turno» desaparece.
- **N5 — Ventanas y resta de tiempo.** La ventana de la posición k va del fin de la franja k al inicio
  de la k+1 (última: hasta el fin del turno), de modo que las ventanas de posiciones distintas son
  disjuntas. Dentro de una posición el orden es cronológico y no hay solapes; entre posiciones tampoco
  se admite cruce. La duración estimada del bloque se obtiene restando la **suma** de las duraciones
  (no la unión) y dividiendo entre el número de franjas; no se admiten solapes internos, así que suma
  y unión coinciden. La fórmula es la misma que documenta RF-11 y no altera la rejilla del solver.
- **N6 — Vaciado y reemplazo.** Se borra en orden de dependencia con las claves foráneas activas:
  eventos antes que turnos; aulas y materias antes que el catálogo de tipos.
- **N18 — Catálogo vacío en criterios.** El criterio del catálogo cubre que el catálogo vacío impide
  crear aulas y materias.
- **N19 — Base.** Fijada en **`471311c`**, que contiene el esquema de `059168f`; HEAD = `471311c`. No
  hay diff de esquema sin commitear que imputar a la implementación.

## Supersesión

- **008:** la noción de receso en los turnos, los recuentos por entidad de su RF-12 (008:429-431), su
  RF-9 (008:390) y sus criterios 1 y 3; además de la promesa de generabilidad, ya retirada.
- **001:** se mantiene y se apoya la frontera de 001:132: interfaz de usuario fuera de alcance, solo
  contrato de datos.

## Notas

- **Cierre del plan (2026-10-23).** `plan.md` y `tasks.md` aprobados por el usuario el 2026-10-23 y
  **commiteados en `471311c`** (junto con este `estado.md`). La implementación empieza el
  **2026-10-24**. Riesgos declarados al usuario: **T6 y T8 en el límite de 30 min** y **el frontend no
  compila** hasta que su equipo migre.
- **El esquema del commit Base ya tiene `TiposAula` y `Eventos`**, pero no la identidad/orden del
  evento, la clave normalizada/índice único de tipos ni la marca de versión; la 009 los añade. El
  ajuste es del plan (T2).
- **Sin migración.** Una base con el esquema anterior se rechaza al abrir; no se convierten recesos en
  eventos ni se suponen tipos. El usuario crea una base nueva con respaldo (specs 001 y 004).
- **La constitución no se toca aquí.** Su enmienda es requisito y tarea de la 010.
- **La 005 no se toca aquí.** Su supersesión se declara en la 010; la anotación del texto de la 005 la
  hará el planner en la fase de plan.

## Tamaño previsto

Doce RF en **diez tareas** (T1 10-15 min; T2-T8 25-30 min; T9 25 min; T10 25 min), con build sin
frontend y compatibilidad transitoria de eventos hasta el cierre.
