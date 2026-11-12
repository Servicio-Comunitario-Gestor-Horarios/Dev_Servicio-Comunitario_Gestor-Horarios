# Estado 020

Fase: **cerrada**
Aprobaciones: **spec 2026-11-12** · **plan/tareas 2026-11-12** · **validación 2026-11-12**
Base: `455ba7f651d59e46f633753ccbe8368f0a37c0f9` (`HEAD` de `experimento/horas-duras-plazo-1h`, creada
desde `feat/Motor`; `455ba7f` = «fix(test): no fallar la configuracion si falta Google Test»)
Dudas abiertas: **ninguna** (D-1…D-5 cerradas; la unidad del plazo y la premisa del health-check quedaron
corregidas en la fase 2)
Siguiente paso: **nada pendiente**; el experimento queda **aislado en la rama** y **no se fusiona sin
decisión**.
Veredicto: **APROBADO (2026-11-12)**
Notas: experimento aislado. La rama **no** se fusiona ni toca `feat/Motor`. La spec fija el plazo en
**tiempo de reloj** (60 min de proceso, 55 min de solver **en total**, 5 min de reserva fija para
extracción y comunicación). **Las supersesiones propuestas están aprobadas como propuestas pero NO se
aplican** a 005/018/002/014/013 ni a `docs/constitution.md`: la rama es un experimento y aplicarlas sería
otra decisión. **Commit antes de implementar:** el `sha` de la spec/plan se anota en `journal.md`; `git
status` no lo prueba. No hay dudas abiertas.

## Rama y aislamiento

- **Rama:** `experimento/horas-duras-plazo-1h`, creada desde `feat/Motor` en `455ba7f`.
- **No** se toca `feat/Motor`, **no** se hace merge y **no** se ejecuta el solver lento.
- El cambio es reversible revirtiendo la rama; no deja estado persistente ni datos convertidos.
- Todo lo que aquí se implemente queda **fuera** de la rama de producto hasta una decisión posterior.
- **Proceso:** la spec se commitea **antes** de implementar y el diff de la implementación se limita a
  los archivos del plan.

## Resultado medido (2026-11-12)

Corrida manual del usuario, con el centro real y el tope duro de 36 h:

- **876 de 1188 h** declaradas (**73,7 %** de la capacidad). Quedan **240 h de holgura efectiva** si se
  descuentan las **72 h** de los 2 docentes sin carga.
- **31 docentes con clases:** **9 a 36/36 h** y **22 entre 12 y 34 h**.
- **2 docentes sin clases:** **P031 Ramón Ñáñez** y **P032 Ángela Ñáñez**, por **materias sin carga del
  seed**.
- **El tope duro es factible con 36 h** y la corrida **terminó dentro de la hora**.

## Notas y limitaciones de la validación

- **La hora real no se cronometró:** se sabe que terminó dentro del plazo, no el tiempo exacto.
- **No se ejecutaron** `test_solver_centro_real` ni `test_solver_centro_sin_ef` (etiqueta `slow`).
- **`PESO_HORAS` y su `static_assert`** quedan **históricos** (S1 retirado en el experimento).
- **Desfase documental:** `plan.md` habla de **7 tareas** y `tasks.md`/este estado de **8** (T1b de C3).
- **El texto de `TiempoAgotado` aún no dice «una hora»** (deuda menor del mensaje).

## Decisiones cerradas (2026-11-12)

| # | Resolución |
|---|---|
| D-1 | **Tope duro para todos los docentes, sin exceso.** `Σ clases ≤ horas_aula` para titulares y suplentes. Un docente con `horas_aula = 0` **no puede dar clase**. No hay válvula de exceso ni relajación automática. |
| D-2 | **Plazo (corregida):** **60 min de reloj de proceso** y **55 min de reloj de solver en total** (referencia y fases blandas). La referencia puede consumir los 55; las blandas usan **lo que quede de esos 55**. Los **5 min** hasta los 60 son **reserva fija de extracción y comunicación** y **no** se usan para fases blandas. El servicio **no abandona antes de los 60 min**. El cálculo lento lo ejecuta **el usuario a mano**. |
| D-3 | **Si con 36 h el modelo es NoFactible, se acepta y se muestra la causa** (categorías (1)–(5) o «no atribuible»). No se sube el tope ni se cae al comportamiento blando. |
| D-4 | **Rama aislada** `experimento/horas-duras-plazo-1h`; nada se fusiona en `feat/Motor` sin decisión posterior. |
| D-5 | **Causa del NoFactible:** se incluye la **propagación mínima** del diagnóstico del motor, como **excepción parcial y declarada a la 019** (solo el mínimo de H3). El resto de la 019 sigue fuera. |

## Lo comprobado en el sistema (para el plan)

Lectura del código y de las specs vigentes el **2026-11-12**, sin ejecutar el solver.

| Tema | Situación actual | Referencia |
|---|---|---|
| Tope de horas | **Blando**: `Σ x ≤ exceso + tope`; el exceso es variable y se penaliza en el objetivo con `PESO_HORAS = 1000`. | `src/backend/src/solver/restricciones/restricciones_turnos.cpp` (`restriccionTopeHorasProfesor`), `src/backend/src/solver/solver.cpp` (objetivo) |
| Pesos | `PESO_HORAS = 1000 > COSTE_SUPLENTE = 100 > PESO_PREFERENCIA = 25`; hay `static_assert` de la jerarquía. **S1 se retira** en el experimento; S2 y la preferencia se mantienen. | `src/backend/include/backend/solver/config/contrato_solver.hpp` |
| Exceso en el ancla | `AnclajeEstructural` guarda `z*` **y `e*`** y la fase 2 fija ambos por igualdad. **El ancla pierde `e*`** al desaparecer el exceso; conserva el reparto de docentes. | `src/backend/include/backend/solver/solver.hpp`, `src/backend/src/solver/solver.cpp` (`extraerAnclaje`, `fijarAnclaje`) |
| Presupuestos de fase | `PresupuestoFases{ referencia = 60.0, separacionHuecos = 20.0, preferencias = 10.0, workers = 8 }`. | `src/backend/include/backend/solver/solver.hpp` |
| Unidad del presupuesto | `ejecutar()` usa **`max_deterministic_time`**, no segundos de reloj. **La 020 fija reloj**: el plan debe usar el límite de tiempo de reloj del motor (o un corte equivalente) para los 55 min y garantizar que la respuesta llega dentro de los 60. | `src/backend/src/solver/solver.cpp` (`ejecutar`) |
| Solución al límite | `esFactible()` acepta **`FEASIBLE` y `OPTIMAL`**; una solución factible sin prueba de optimalidad ya se devuelve como éxito. La 020 lo hace explícito y **acota la marca de no optimalidad a la fase** cuyo presupuesto se agotó (la referencia o una blanda), no al horario entero. | `src/backend/src/solver/solver.cpp` (`esFactible`, `resolver`) |
| `UNKNOWN` | `analizarFallo` produce texto propio («no se puede afirmar que no exista solución»), pero `resolver()` lo devuelve como `exito=false` y `resolverEntradaSolver` como `factible=false`: **hoy se confunde con NoFactible**. Hay que separar el desenlace. | `src/backend/src/solver/solver.cpp` (`analizarFallo`), `src/backend/src/solver/servicio_solver.cpp` |
| Errores internos y de contrato | `MODEL_INVALID` y otros errores internos también salen como `exito=false`; un socket caído emite `RESP_ERROR` y `PuertoSolverIpc` lo entrega como `ok=false`. El servicio los presenta como **`ContratoInvalido`** (o los mezcla con NoFactible). La 020 exige un camino propio de **error de entrada/contrato**, con su causa. | `src/backend/src/solver/solver.cpp` (`analizarFallo`), `src/middleware/src/client/internalclient.cpp` (`onErrorOccurred`), `src/frontend/src/services/puerto_solver_ipc.cpp` |
| Causa del NoFactible | El servicio **descarta `respuesta.error`**: ante `!factible` escribe «No existe una solución factible…». **Es el punto de la propagación mínima (D-5/B1).** `ContratoInvalido` sí usa `respuesta.error`. | `src/backend/src/services/ServicioGeneracion.cpp` (`alResponder`) |
| Plazo del servicio | Por defecto **300 000 ms**; al vencer abandona y muestra el mensaje de plazo. **La pérdida de comunicación no produce hoy `TiempoAgotado`:** el socket caído emite una respuesta de error (`ok=false`) que llega como `ContratoInvalido`. La 020 exige un desenlace **«sin respuesta»** distinto de «respuesta inválida». La prueba automática puede inyectar un `plazoMs` corto. | `src/backend/include/backend/services/ServicioGeneracion.hpp` (enum `Estado`), `src/backend/src/services/ServicioGeneracion.cpp`, `src/middleware/src/client/internalclient.cpp` |
| Plazo en la vista | Instancia el servicio con **300000 ms** y anuncia «Generando horarios… puede tardar unos segundos.». | `src/frontend/src/views/schedule_generation_widget.cpp` (206-217 y 1066) |
| Presupuesto en producción | `resolverEntradaSolver` llama a `solver.resolver(config.valor)` → **usa `PresupuestoFases{}` por defecto**; el presupuesto del experimento debe llegar por ahí. | `src/backend/src/solver/servicio_solver.cpp` |
| Manejador síncrono | El backend atiende `solver_resolve` en el hilo de la aplicación: mientras resuelve, **no atiende health-checks**. | `src/app/src/aplicacion_backend.cpp` |
| Verificación de salud | **Corregido:** `InternalClient` **no tiene plazo**; encola las solicitudes y envía una cada vez, así que un health-check durante el cálculo **espera** y se responde después. Los **3 intentos** solo se consumen ante respuesta de error; **no se declara caída** por lentitud ni se mata el proceso (el `kill` está solo en `detener()` al cerrar). La verificación sigue programada cada 5 s. | `src/middleware/src/client/internalclient.cpp` (`m_cola`, `enviarSiguiente`), `src/app/src/gestor_proceso_backend.cpp`, `src/app/include/app/gestor_proceso_backend.hpp` |
| Estado tras la generación | Las comprobaciones encoladas se responden al terminar; la verificación queda en estado normal (reinicia el contador al responder). Si se decidiera suspenderla, hay que reanudarla al terminar. | `src/app/src/gestor_proceso_backend.cpp` |
| Causas de NoFactible | `analizarFallo` informa (1)–(5) o «no atribuible». **Las horas no son categoría**: una infactibilidad solo por horas cae en «no atribuible» (la 020 pide añadir demanda total y suma de topes como diagnóstico). | `src/backend/src/solver/solver.cpp` (`analizarFallo`) |
| Aviso P2 | Compara contra **`horas_requeridas`** (`requeridas != -1 && asignadas > requeridas`), **no** contra `horas_aula`. Bajo tope duro **no comprueba el tope**; la 020 exige una comprobación posterior contra `horas_aula`. | `src/backend/src/services/ValidadorSalidaSolver.cpp`, `src/backend/include/backend/services/ValidadorSalidaSolver.hpp` |
| `horas_aula` ausente | El parser usa `leerEntero(o, "horas_aula")` con **defecto 0**: un dato ausente deja al docente **sin capacidad en silencio**. La 020 lo trata como 0 **y exige informarlo**. | `src/backend/src/solver/config/solver_config.cpp` (`leerEntero`, lectura de profesor) |
| Validación previa | V13 exige candidato elegible y **no** exige capacidad de horas; V14 acota bloques por franjas del turno. La 020 lo convierte en **RF-6** normativo. | specs 005/010/013, `docs/constitution.md` §7.4 |
| Contrato y salida | `horas_requeridas` es opcional; si existe, `horas_aula` no puede superarla. | `docs/contrato-solver-2.0.md`, spec 005 |
| Datos del centro | 24 cursos, 33 docentes, 876 clases, tope sembrado 36 h, 40 min, capacidad 1188 (576 + 612). **Recuentos de suites heredados** (no re-ejecutados hoy): 62/62 (`backend`), 67/67 (`full-release`), lento 2/2 con 161–264 s (una corrida 369,7 s). | 017/018, `MEMORY.md` |

## Riesgos y conflictos

| # | Riesgo | Efecto | Tratamiento |
|---|---|---|---|
| 1 | **Infactibilidad con 36 h** | Un tope duro puede no tener solución con los datos reales (demanda frente a Σ topes = 1188) o ser muy difícil de encontrar. | Es un **resultado válido** del experimento: NoFactible con causa. No se sube el tope ni se cae a blando (D-3). |
| 2 | **La causa puede ser «no atribuible»** | Las horas no son categoría del catálogo (1)–(5). | Declarado: se informa «no atribuible» **con la demanda total y la suma de topes** (RF-2), sin inventar categoría. |
| 3 | **Una hora de espera** | La interfaz puede parecer colgada durante minutos. | RF-3: texto «hasta una hora», y desenlaces distintos de plazo agotado y no concluyente. |
| 4 | **Reparto de la hora** | Si el presupuesto del solver igualara el plazo del servicio, se abandonaría un cálculo que termina dentro. | **Resuelto en D-2/N1:** 60 min de proceso, **55 de solver en total** (referencia + blandas) y **5 de reserva fija** para extracción y comunicación; el servicio no abandona antes de los 60. |
| 5 | **Unidad del presupuesto** | El motor usa tiempo determinista; 60 unidades rindieron ~172 s. Un valor mal calibrado puede tardar mucho más o menos de una hora. | **Resuelto en la spec (B5):** el presupuesto es **tiempo de reloj**. El plan implementa el límite de reloj y verifica que el desenlace llega dentro de los 60 min. |
| 6 | **Solución factible al límite** | Descartarla por no probar optimalidad tiraría un horario válido. | RF-3: se presenta la **mejor solución encontrada**; la marca de no optimalidad se refiere **a la fase** que agotó su presupuesto. |
| 7 | **`UNKNOWN` confundido con NoFactible** | Hoy `exito=false` + `factible=false` lo convierte en «no existe solución», que es falso. | RF-3: desenlace **no concluyente** con el texto del motor; RF-2 prohíbe declarar NoFactible sin prueba. |
| 8 | **`FEASIBLE` y `OPTIMAL` en el mismo camino** | La distinción «no óptima» necesita un indicador observable y **acotado a la fase**. | RF-3: marca por fase; si la referencia es óptima y una blanda no, no se califica de no óptima la asignación de docentes. El plan decide el campo o el mensaje, sin cambiar el contrato de configuración. |
| 9 | **Pérdida de comunicación** | Hoy el socket caído emite `RESP_ERROR` y llega como `ContratoInvalido`, no como plazo agotado. | RF-3: desenlace **«sin respuesta»** con su motivo, distinto de respuesta inválida y de NoFactible; el plan debe propagarlo. |
| 10 | **Errores internos/contrato** | `MODEL_INVALID`, respuesta fuera de contrato y errores internos no son infactibilidad ni falta de prueba. | RF-2/RF-3: se informan como **error de entrada/contrato** con su causa; no se presentan como NoFactible ni como no concluyente. |
| 11 | **Las fases blandas pueden no ejecutarse** | Si la referencia consume los 55 min, se devuelve la referencia **sin** separación ni preferencia horaria. | Aceptado por D-2 y declarado en RF-3; el horario sigue siendo válido. |
| 12 | **Health-check** | **Premisa corregida (B12):** el cliente no tiene plazo y las comprobaciones se encolan; no se consumen intentos ni se declara caída. | RNF-4: prohibido matar o declarar colapso; si se suspende la verificación, **se reanuda al terminar la generación** y queda en estado normal. |
| 13 | **Conflicto con la 019** | La 019 (aprobada en `feat/Motor`, sin implementar) fija RF-4 con horas **flexibles** y P-5 «sin tope fijo». | Declarado. **Excepción parcial:** solo el mínimo de H3 (propagar la causa). No se implementa el resto; **no se fusiona la 020** sin reconciliar. |
| 14 | **Conflicto con 018 RF-4** | Dice literalmente que el tratamiento de horas es **blando** y que el exceso **no se convierte en infactibilidad**. | Supersesión propuesta (no aplicada); la aritmética de capacidad y el aforo no cambian. |
| 15 | **Constitución §7.2 y specs 002/005/013/014** | S1, el plazo de 300 s, los presupuestos 60/20/10 y el ancla con `e*` quedan contradichos. | Solo **notas de supersesión propuestas**; no se edita `docs/constitution.md` ni specs ajenas sin aprobación. |
| 16 | **`horas_aula` ausente** | El parser lo lee como 0 y deshabilitaría al docente en silencio. | RF-1: ausente = 0, sin clases y **se informa** qué docentes quedan sin capacidad. |
| 17 | **Aviso P2 mal leído** | P2 mide contra `horas_requeridas`; bajo tope duro no comprueba el tope. | RF-5: comprobación posterior **contra `horas_aula`**; si alguna asignación supera el tope, el resultado **no es presentable**; P2 se mantiene separado. |
| 18 | **Cambios colaterales** | El tope duro afecta a la variable de exceso, al objetivo, al ancla, al aviso y a la validación de salida. | El plan retira el exceso de forma coherente y confirma con tests que ningún resultado supera el tope. |
| 19 | **Línea base y recuentos** | Cambian los tests que afirman el comportamiento blando (exceso, objetivo, ancla, presupuestos 60/20/10, plazo 300 s, P2). | RNF-1: «línea base» = pruebas **no ligadas al comportamiento superseded**; las demás se reescriben y no son regresión. |
| 20 | **Cálculo lento** | El centro real tarda minutos; la verificación automática no debe lanzarlo. | RNF-2: lo ejecuta el usuario a mano; la prueba automática del plazo usa un **plazo corto inyectado**. |
| 21 | **Recuentos de suites** | No se han vuelto a ejecutar hoy. | **Heredados** de `MEMORY.md`/018 (62/62, 67/67, 2/2); se confirman en la implementación, no se afirman como actuales. |

## Notas para el plan (no son dudas de la spec)

1. **Presupuesto de reloj.** Sustituir/parametrizar el límite de tiempo determinista por un límite de
   **tiempo de reloj**: **55 min en total** para el solver (la referencia puede consumirlos; las blandas
   usan lo que quede) y **5 min de reserva fija** para extracción y comunicación, de modo que la
   respuesta total cabe en los 60 min.
2. **Separación de desenlaces.** Nombrar **cuatro desenlaces bien formados** más el camino de error:
   (a) horario presentado, con la optimalidad declarada por fase; (b) **NoFactible** con causa (solo
   `INFEASIBLE`); (c) **no concluyente** (`UNKNOWN`, presupuesto agotado sin prueba); (d) **sin
   respuesta** (pérdida de comunicación) o **plazo agotado** (sin respuesta en 60 min); y (e) **error de
   entrada o de contrato** (`MODEL_INVALID`, respuesta fuera de contrato, errores internos), que **no**
   es NoFactible ni no concluyente. No reutilizar `NoFactible` para `UNKNOWN` ni `ContratoInvalido` para
   una pérdida de comunicación.
3. **Propagación mínima de la causa.** Usar el diagnóstico del motor (`respuesta.error`) en el mensaje
   de NoFactible; conservar el mensaje propio de `ContratoInvalido` y el de la pérdida de comunicación.
4. **Comprobación posterior contra `horas_aula`.** Añadirla sin confundirla con P2 (que mide contra
   `horas_requeridas`): si alguna asignación supera el tope, el resultado no es presentable.
5. **Reparto observable.** Una línea por docente (horas asignadas y tope) y una línea de totales (horas
   del centro frente a la suma de topes), presentado en la vista o en el resumen del resultado.
6. **Optimalidad por fase.** La marca de no optimalidad se refiere a la fase cuyo presupuesto se agotó:
   la referencia (asignación de docentes) o una blanda concreta (separación y huecos, o preferencia). Si
   la referencia es óptima, no se califica de no óptima la asignación de docentes.
7. **Verificación de salud.** No matar ni declarar colapso; si se suspende, reanudar al terminar la
   generación. No hace falta tocar el cliente (no tiene plazo), salvo que se opte por suspender.
8. **Alcance mínimo del código.** Exceso fuera del modelo, objetivo y ancla (`e*`); presupuesto de
   reloj; plazo del servicio; propagación de la causa; desenlaces separados; comprobación de tope y
   reparto; textos de la vista. Sin tocar contrato, esquema, presets ni datos.

## Supersesiones: aprobadas como propuestas, **no aplicadas**

El usuario aprobó la spec y las supersesiones **como propuestas** el **2026-11-12**. **No se han escrito**
en 002/005/013/014/018/019 ni en `docs/constitution.md`: la rama es un **experimento aislado** y aplicarlas
sería **otra decisión**. Mientras dure la rama, se leen como excepciones temporales del experimento.

Detalle y redacción en `spec.md` → «Propuesta de supersesiones». Resumen:

- **005**: el exceso deja de ser último recurso; el tope es duro y `horas_aula = 0` no da clase; V13 se
  mantiene y pasa a ser normativa en RF-6.
- **018 RF-4**: conflicto declarado (afirma el tratamiento blando y que el exceso no es infactibilidad).
- **002 / 014b**: el plazo de 300 s pasa a 60 min de reloj (55 de solver en total + 5 de reserva fija).
- **014**: S1 se retira del objetivo; la referencia sube a 55 min de reloj; 8 workers se mantienen.
- **013**: el ancla pierde `e*`; las fases 2a/2b usan solo lo que quede de los 55 min del solver.
- **`docs/constitution.md` §7.2** (y §7.1/§7.5): tope como invariante dura, S1 fuera del orden blando,
  el aviso de exceso como dato de coherencia; **§7.4 sin cambios**.
- **019**: conflicto declarado (horas flexibles, sin tope fijo) con **excepción parcial: solo el mínimo
  de H3**; no se fusiona sin reconciliar.

## Historial

- **2026-11-12 — spec redactada (borrador).** Se leen `AGENTS.md`, `MEMORY.md`, las specs 005, 010, 013,
  014, 018 y 019, la constitución §7, el contrato 2.0 y el código del tope, el objetivo, los
  presupuestos de fase, el plazo del servicio, la vista de generación y la verificación de salud. Se
  confirman: el tope es blando con `PESO_HORAS`, el presupuesto de la referencia es 60 (tiempo
  determinista), el servicio espera 300 s y la vista anuncia «unos segundos». Se crean `spec.md` y este
  `estado.md`. **No** se escribe plan ni tareas, **no** se toca código y **no** se ejecuta el solver.
- **2026-11-12 — fase 2: correcciones del QA (B1–B21).** Se corrige `spec.md` y este estado.
  **B1:** se incluye la propagación mínima de la causa (`respuesta.error` → mensaje de NoFactible) como
  **excepción parcial a la 019** (D-5), con RF-2/RF-5/HU-2/criterios y el aviso explícito en «Fuera de
  alcance» y en las supersesiones. **B2/B5:** D-2 corregida a **60 min de reloj de proceso y 55 de
  solver** (precisado después en N1: 55 en total + 5 de reserva); la unidad es **reloj**, no tiempo
  determinista; definiciones, RF-3, casos y criterios actualizados; la duda de la equivalencia queda
  cerrada. **B3:** una solución `FEASIBLE` al límite **se presenta** (acotado después en N4: la no
  optimalidad se declara por fase); «plazo agotado» es solo la falta de respuesta dentro
  de los 60 min. **B4:** `UNKNOWN` produce el desenlace **no concluyente**, distinto de NoFactible, con
  el texto del motor y prueba que los distingue. **B6:** RNF-5 separa lo determinista (reglas, formato,
  diagnóstico) de lo variable (horario, reparto, desenlace temporal). **B7:** RNF-1 acota «línea base» a
  las pruebas no ligadas al comportamiento superseded y enumera las que se reescriben sin ser regresión.
  **B8:** RF-6 nuevo, con V13 normativa y prueba. **B9:** RF-5 exige comprobación posterior contra
  `horas_aula` (P2 mide contra `horas_requeridas` y no la sustituye) y define el reparto observable.
  **B10:** conflicto declarado con **018 RF-4**; 013 (el ancla pierde `e*`) y 014 (S1 se retira)
  corregidas en la tabla. **B11:** `horas_aula` ausente = 0, sin clases y **se informa**; caso y prueba
  añadidos. **B12:** se corrige la premisa del health-check (`InternalClient` no tiene plazo y encola;
  no se consumen intentos ni se declara caída); RNF-4 exige no matar ni declarar colapso y reanudar la
  verificación si se suspende. **B13:** «ningún camino devuelve exceso» se refiere a los resultados de
  generación; los horarios guardados no se revalidan. **B14:** plazo agotado incluye la pérdida de
  comunicación y el mensaje no afirma que no exista solución. **B15:** estado de la verificación tras la
  generación declarado. **B16:** RF-5 y sus criterios en forma EARS y con superficie observable.
  **B17:** la spec se commitea antes de implementar (RNF-3 y siguiente paso). **B18:** un NoFactible solo
  por horas informa «no atribuible» **con la demanda total y la suma de topes**. **B19:** el criterio de
  las 10 tareas pasa a RNF-6 (proceso), no a los criterios de aceptación. **B20:** la prueba automática
  del plazo usa un **plazo corto inyectado** y la hora real es evidencia manual. **B21:** los recuentos
  de suites se marcan como **heredados** (no re-ejecutados hoy). **Estado: borrador**, **dudas
  abiertas: ninguna**; sin plan, sin tareas, sin código y sin ejecutar el solver.
- **2026-11-12 — fase 3: N1–N4 y B16.** **N1:** regla única de margen: el **solver tiene 55 min de reloj
  en total**; la referencia puede consumirlos y las **blandas usan lo que quede de esos 55**; los **5 min**
  hasta los 60 son **reserva fija de extracción y comunicación** y no se usan para fases blandas.
  Actualizados D-2, definiciones, RF-3, casos, criterios y supersesiones (002/014/013). **N2:** la
  **pérdida de comunicación** es un desenlace **«sin respuesta»**, distinto de una respuesta inválida;
  `estado.md` corregido: hoy el socket caído emite `RESP_ERROR` y llega como `ContratoInvalido`, no como
  `TiempoAgotado`. **N3:** `MODEL_INVALID`, contrato incumplido y errores internos se informan como
  **error de entrada/contrato**, no como NoFactible ni no concluyente; añadido a RF-2/RF-3, casos,
  criterios y notas del plan (cuatro desenlaces bien formados + el camino de error). **N4:** la marca de
  **no optimalidad se declara por fase** (la referencia/asignación de docentes o la blanda concreta); si
  la referencia es óptima y una blanda no, no se califica de no óptima la asignación. **B16:** RF-3 dice
  «EL SISTEMA informará» y RF-5 concreta el observable (una línea por docente y una de totales).
  Sin cambios de alcance, sin plan, sin tareas, sin código y sin ejecutar el solver.
- **2026-11-12 — spec aprobada; plan y tareas escritos.** El usuario aprueba la spec y las supersesiones
  **como propuestas** (que **no se aplican**). `spec.md` pasa a **`aprobada (2026-11-12)`**; este estado a
  **fase plan · tareas**, con `Aprobaciones: spec 2026-11-12`. Se escriben `plan.md` (tope duro sin
  exceso, presupuesto de **reloj** con deadline compartido de 55 min y reserva de 5, desenlaces
  separados, causa mínima H3, comprobación posterior contra `horas_aula` y reparto observable) y
  `tasks.md` con **7 tareas** en orden de dependencia. **No** se ha tocado código ni ejecutado el solver.
- **2026-11-12 — plan y tareas aprobados; fase de implementación.** El usuario aprueba el plan y las
  tareas **tras el QA del plan** (P1–P16), que dejó **8 tareas** al dividir la antigua T2 en T2 (solver) y
  T3 (IPC/servicio). El estado pasa a **`Fase: implementacion`**, con
  `Aprobaciones: spec 2026-11-12 · plan/tareas 2026-11-12`. Siguiente paso: **implementar T1–T8 con el
  implementer**; antes de T1 el coordinator commitea la spec y el plan/tareas y el `sha` se registra en
  `journal.md` (T8). El cálculo lento lo ejecuta el usuario. **No** se ha tocado `spec.md`, `plan.md`,
  `tasks.md` ni código en este paso.
- **2026-11-12 — CIERRE: validación APROBADA.** El reviewer emite **`VEREDICTO: APROBADO
  (2026-11-12)`** con la corrida manual del usuario. `spec.md` pasa a **`implementada (2026-11-12)`**;
  este estado a **`Fase: cerrada`**, con `Aprobaciones: spec 2026-11-12 · plan/tareas 2026-11-12 ·
  validación 2026-11-12`. **Resultado medido:** **876/1188 h (73,7 %)**, **240 h de holgura efectiva**
  descontando las 72 h de los 2 docentes sin carga; **31 docentes con clases** (9 a 36/36 y 22 entre 12 y
  34) y **2 sin clases** (**P031 Ramón Ñáñez** y **P032 Ángela Ñáñez**) por materias sin carga del seed.
  **El tope duro es factible con 36 h** y la corrida terminó **dentro de la hora**. **Limitaciones
  declaradas:** la hora real no se cronometró; los tests lentos (`test_solver_centro_real`,
  `test_solver_centro_sin_ef`) no se ejecutaron; `PESO_HORAS`/`static_assert` quedan históricos;
  `plan.md` dice 7 tareas y `tasks.md`/estado 8 (T1b de C3); el texto de `TiempoAgotado` aún no dice
  «una hora». **Las supersesiones siguen aprobadas como propuestas y NO aplicadas**: el experimento queda
  **aislado en la rama** y **no se fusiona sin decisión**.
