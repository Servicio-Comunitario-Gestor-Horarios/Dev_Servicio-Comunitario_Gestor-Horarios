# Estado 011

Fase: implementación completa; pendiente de validación RF por RF
Aprobaciones: spec 2026-10-23 · plan/tareas 2026-10-27 · implementación 2026-10-29
Base: 8e0602c (base de la implementación; HEAD de `feat/Base-datos`: `c62653c`, suite 52/52). Las dependencias **009** y **010** están **cerradas e implementadas**: el esquema v3, el contrato 2.0 y sus pruebas ya están en la base.
Dudas abiertas: ninguna
Siguiente paso: validación RF por RF contra `Base: 8e0602c` por el reviewer; tras `VEREDICTO: APROBADO`, marcar la spec `implementada` y cerrar.
Veredicto: pendiente (validación de la implementación).
QA: spec: CAMBIOS NECESARIOS resueltos el 2026-10-23 (H1–H8); QA del plan: dos rondas de CAMBIOS NECESARIOS (B1, M1–M10) cerradas el 2026-10-27; plan y tareas aprobados el 2026-10-27.

## Decisión nueva del usuario (2026-10-23)

- **La configuración global manda** sobre la duración de la hora académica; el **turno es un dato
  externo del plantel y se valida**.
- **Default 35 minutos** (hora académica de Educación Básica, Gaceta 42.505; antes 45), rango **30–90**
  para el cambio normativo y variantes regionales.
- **La franja de clase se deriva de D**, no de repartir la ventana del turno. Esto supersede la
  estimación de la 009 y el inventario de turnos de la 008.
- **Invariante de encaje:** `nº franjas × D + tiempo de eventos ≤ ventana`; si no, rechazo con turno, D
  y déficit. El **número de franjas utilizable** se calcula con ventana, eventos y D.
- **Bloques de 80 minutos de Media:** duda diferida, fuera de alcance; hoy 1 hora = 1 bloque = 1 franja.
- **Duración del preset (cerrada).** La duración deja de ser un dato del usuario en el preset: la
  construcción la **sobrescribe siempre** con la de la configuración global. El preset puede seguir
  conteniendo el campo porque comparte formato con el contrato, pero su valor **nunca** se usa; un
  cambio de configuración desde que se guardó el preset usa el valor nuevo, sin error ni aviso.

## Decisiones de modelado para revisar

- **Franjas declaradas frente a utilizables.** El turno conserva su **número de franjas declarado**
  (dato del plantel) y se valida contra el **máximo utilizable**
  `floor((ventana − eventos) / D)`. El máximo es diagnóstico; la generación usa el declarado, que no
  puede superarlo. La alternativa (derivar siempre el número y dejar de declararlo) cambiaría el modelo
  de turnos y el contrato; no se elige.
- **Configuración y seed.** La configuración **no es dato de dominio** para la política de base vacía:
  la siembra la asegura en 35 de forma **idempotente** (una fila), no la cuenta como dato de dominio,
  no bloquea una base recién creada y no duplica filas.

## Hallazgos del QA resueltos

- **H1:** default 35, franja = D y supersesión de la estimación de la 009 y del inventario de 40 de la
  008; ya no conviven tres duraciones normativas.
- **H2:** resuelto por la decisión del usuario: la duración deja de ser un dato del preset; la
  construcción la **sobrescribe siempre** con la de la configuración y no hay aviso.
- **H3:** la validación del contrato se estrecha a 30–90; supersede V2 de la 005/010 y usa el mismo
  punto único que la configuración.
- **H4:** la configuración no es dato de dominio del seed; se asegura de forma idempotente, sin
  duplicar ni bloquear la siembra.
- **H5:** estado de entrega y verificación declarado; las pruebas del constructor con 45 y del seed con
  40 se reescriben a 35 y a la invariante, y la construcción sin interfaz debe pasar.
- **H6:** «un solo punto» se acota a horas académicas → minutos; la conversión de rangos a franjas del
  constructor sigue; la terminología es del backend y el `horasAula` del front es deuda.
- **H7:** seed verificable: valor efectivo 35 y recuento de configuraciones 1; sin «recuento o resumen»
  ni afirmaciones sobre generaciones no persistidas.
- **H8:** la 011 **extiende** la definición de huella de la 004 para incluir D.

## Supersesiones que declara

- **009:** definición de «duración estimada de la franja de clase» y cálculo
  `(ventana − eventos) / nº franjas` (RF-3 y RF-11); la franja pasa a durar D y el sobrante no se
  reparte.
- **008:** inventario de 8 franjas de 40 minutos y rejilla `8 × 40 + 10 = 330`; los turnos se revalidan
  con D = 35.
- **005/010:** V2 de `duracion_minutos` solo exige `> 0`; la 011 la estrecha a 30–90.

## Notas

- **Orden de implementación.** Después de la 010: la 010 deja el contrato en 2.0; la 011 solo cambia la
  procedencia y el rango del campo de duración.
- **Tope de 40.** No se toca; si el usuario quisiera cambiarlo, sería otro requisito.
- **Plan y tareas aprobados (2026-10-27).** `plan.md` fija la base `8e0602c` y `tasks.md` la lista
  T1–T10 con su trazabilidad; lo que queda pendiente es la **implementación**, que arranca el
  **2026-10-28**.

## QA del plan (2026-10-27)

Dos rondas de re-QA: **CAMBIOS NECESARIOS** (B1 y M1–M10), cerradas el 2026-10-27 con el reparto
aditivo T4→T9 y la aritmética de la rejilla (clases **11:40/17:20**; `fin` **11:50/17:30** = +10 min
del evento). **Plan y tareas aprobados el 2026-10-27.** Correcciones aplicadas sin tocar la spec:

- **B1 (bloqueante):** la migración de `calculo_eventos` va en **dos pasos**. T4 **añade** las funciones
  nuevas y conserva `duracionFranjaEstimada` y las firmas antiguas como envoltorios (nadie queda con el
  build roto); T5, T6 y T8 migran sus llamadores a las firmas con D; **T9** borra el legado con el
  `grep` de cierre. La tabla de trazabilidad y el orden de dependencias lo reflejan.
- **M1:** `docs/interfaz-frontend.md:106` (`VERSION_ESQUEMA_ACTUAL = 3`) → **4**, en §7 y en la
  verificación de T10.
- **M2:** las dos menciones de `duracionFranjaEstimada` (`:346` y `:732`) se sustituyen y entran en el
  `grep` de cierre.
- **M3:** `docs/guias/datos-prueba-frontend.md:56-57,70-71` (turnos/recesos de 40, pasos y `fin`
  12:30/18:10) se añaden a §7 y al `grep` de T10.
- **M4/M5:** `datos-prueba.md:151` («Los 40 minutos…») e `interfaz-frontend.md:658` («40 min…») entran
  en los `grep` con `40 min|40 minutos`.
- **M6:** supersesiones de 009/008/005/010 anotadas al cerrar por el **planner** (§7.2 del plan), con
  textos exactos; no es una tarea del implementer.
- **M7:** estimación de 20–30 min por tarea en `tasks.md` y §9 del plan (T2, T5, T6, T7 y T8 en el
  techo de 30; T9 en **25** desde que migra los casos antiguos de `test_calculo_eventos`).
- **M8:** corregido: el punto único es la conversión **horas académicas → minutos**; multiplicar por D en
  el encaje o en el ancho de franja no es esa conversión.
- **M9:** comentarios desfasados asignados a T4 (`calculo_eventos.hpp:34-36`, `.cpp:59-63`), T6
  (`solver_config.cpp:424-425`) y T8 (`perfil.cpp:282-285`).
- **M10:** `errorDeRango` y el mensaje de turno sin ventana incluyen la **acción disponible** (RNF-1).

### Correcciones finales (2026-10-27)

- **B1 (aritmética):** la rejilla de clases es **07:00–11:40 / 12:40–17:20** (`8 × 35 = 280`); `Turnos.fin`
  **11:50/17:30** añade los 10 min del evento (mismo desfase que la 008). Corregido en §6, §7.1 y en el
  `grep` de T10 (restos viejos `12:20|18:00|12:30|18:10` vacíos; `11:40|17:20` en positivo).
- **M1:** T9 también migra o retira los casos de `ventanaDePosicion` (2 args) y `eventoDentroDeVentana`
  (3 args) de `test_calculo_eventos.cpp:111-203`, con build y `ctest -R '^test_calculo_eventos$'`; T9
  pasa a 25 min (sigue en 10 tareas).
- **M2:** el criterio 1 de la 008 está en 541 y el 3 en 549-553; cita corregida en §7.2.
- **M3:** el error de `consultar()` incluye la acción («Compruebe que la base esté disponible e intente
  de nuevo»).
- **M4:** `test_backend_nucleo_datos` (T5) y `test_validador_entrada` (T7) se anotan como **regresión**,
  no como tests nuevos de esas tareas.
- **M5:** la comprobación de aridad ya no se hace con `grep`: el build es la prueba (sin sobrecargas, una
  llamada vieja no compila) y el `grep` se revisa a mano; `ctest -R test_calculo_eventos` verifica el
  resultado.

## Implementación (2026-10-29)

Ejecutada sobre `Base: 8e0602c`. El orden real de commits fue T1→T4, **T8**, T5→T7, T9, T10; T8 se
adelantó porque T5 dejaba `test_seed_aplicacion` en rojo hasta que el perfil pasara a 35 (el servicio
empezaba a validar el encaje con D = 35 sobre turnos aún dimensionados para 40). La regla «ninguna
tarea cierra con el árbol sin compilar» exigió ejecutarla antes.

| Tarea | Commit | Qué dejó |
|---|---|---|
| T1 | `fc86368` | Punto único `duracion_horaria`: rango 30–90 (con acción), horas académicas → minutos, franjas utilizables y déficit. |
| T2 | `11077f8` | Esquema **v4**: `ConfiguracionHoraria` (una fila, 35 por defecto), catálogo esperado y huella v4. |
| T3 | `4262b49` | `ServicioConfiguracionHoraria` (consulta/edición con *upsert*) y fachada `NucleoDatos`. |
| T4 | `36c28b2` | Franja = D y encaje: `franjasUtilizables`, `deficitDeEncaje`, `errorDeEncaje` y sobrecargas con D; legado temporal. |
| **T8** | `c913d87` | **Adelantada.** Seed con D = 35: turnos 07:00–11:50 / 12:40–17:30, recesos 08:10–08:20 / 13:50–14:00, configuración asegurada en 35 y fuera de `kTablasDominio`. |
| T5 | `bc7122b` | `ServicioTurnosEventos` con la D efectiva: encaje en alta/actualización de turnos y en eventos; seis llamadas migradas. |
| T6 | `fbb1f68` | Contrato: V2 30–90 con el punto único y encaje de todos los turnos; V1 rechaza no enteros. |
| T7 | `15db0d2` | `construirJsonEntrada(datos, preset, D, ahora)` (sobrescritura incondicional y rejilla con D) y `huellaDatos(datos, D)`. |
| T9 | `c62653c` | Retirada del legado: `duracionFranjaEstimada` y las firmas de 2/3 argumentos; casos de `test_calculo_eventos` migrados/retirados. |
| T10 | este cierre | Docs de §7 (más el ejemplo de `consola-datos.md`), anotaciones de supersesión en 009/008/005/010 y verificación de cierre. |

- **Suite:** `ctest --preset backend` → **52/52** (100 %), sin `test_frontend_conversion` ni
  `test_generacion_seed`.
- **Sin cambios** en el modelo del solver, el tope de 40 horas académicas ni la interfaz (deuda
  documentada en `docs/interfaz-frontend.md` §6).
- **Documentación:** los `12:30/18:10` que quedan en `docs/solver/Motor-Solver-Plan-Completo.md` son
  históricos y la constitución los declara no vinculantes; el otro resto es un `archivo:línea` de un
  log de auditoría. No se tocan.

## Tamaño previsto

Nueve RF agrupables en **≤10 tareas**: configuración y servicios, punto único, franja y encaje del
turno, contrato, huella, esquema/versión, seed y guía de API.
