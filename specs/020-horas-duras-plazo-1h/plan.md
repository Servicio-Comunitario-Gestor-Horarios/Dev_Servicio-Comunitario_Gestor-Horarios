# Plan 020 - Experimento: tope duro de horas y plazo de una hora

Base: `455ba7f651d59e46f633753ccbe8368f0a37c0f9`

> **Rama:** `experimento/horas-duras-plazo-1h`. **No** se toca `feat/Motor`, **no** se fusiona y **no** se
> aplican las supersesiones (solo se referencian desde `spec.md`). El cálculo lento **no lo lanza ningún
> agente**: lo ejecuta el usuario a mano.
>
> **Commit antes de implementar:** el coordinator **commitea la spec (y este plan/tareas) antes de la
> primera tarea**; el `sha` de ese commit es la base de implementación y se registra en `journal.md` en
> el cierre (T8). `git status` no sustituye ese registro.

## 1. Alcance del plan y qué RF cubre cada parte

| Parte | Qué resuelve | RF |
|---|---|---|
| A. Tope duro | Exceso fuera del modelo y del objetivo; ancla sin `e*`; tope 0 y ausente; V13 intacta | RF-1, RF-4, RF-6, RNF-8 |
| B. Desenlaces (solver) | `FEASIBLE`/`INFEASIBLE`/`UNKNOWN`/`MODEL_INVALID`, optimalidad por fase, camino de 0 s | RF-2, RF-3, RNF-5 |
| C. Desenlaces (IPC/servicio) | Códigos, puerto, `RespuestaSolver`, `Estado`, «sin respuesta» | RF-2, RF-3 |
| D. Causa mínima | Diagnóstico del motor en NoFactible; demanda vs topes; docentes sin capacidad | RF-1, RF-2, RNF-5 |
| E. Plazo de reloj | 60 min de proceso, 55 de solver en total, 5 de reserva; textos | RF-3, RNF-2 |
| F. Tope posterior y reparto | P5 contra `horas_aula` (separado de P4), reparto observable | RF-5, RF-1 |
| G. Suites y cierre | Tests superseded, alcance del diff, `sha`, evidencia | RNF-1, RNF-3, RNF-4, RNF-7, RNF-10 |

## 2. Archivos y responsabilidades

| Archivo | Responsabilidad en este plan |
|---|---|
| `src/backend/include/backend/solver/variables.hpp` | Quitar el miembro `exceso` de `VariablesSolver`. |
| `src/backend/src/solver/variables.cpp` | Quitar la creación de `e[p]` y `cotaExceso`. |
| `src/backend/src/solver/restricciones/restricciones_turnos.cpp` | `restriccionTopeHorasProfesor`: `AddLessOrEqual(Sum(x), tope)` sin exceso. |
| `src/backend/src/solver/solver.cpp` | Objetivo sin S1; ancla sin `e*`; `ejecutar` con `max_time_in_seconds`; deadline compartido y guarda de 0 s; optimalidad por fase; `analizarFallo` con demanda/topes. |
| `src/backend/include/backend/solver/solver.hpp` | `PresupuestoFases` en **reloj**; `ResultadoSolver.desenlace` y `fasesNoOptimas`; `AnclajeEstructural` sin `e*`. |
| `src/backend/include/backend/solver/desenlace_solver.hpp` *(nuevo)* | **Solo el enum** `DesenlaceSolver`. **Sin OR-Tools** (lo incluyen `ServicioGeneracion.hpp` y el frontend). |
| `src/backend/include/backend/solver/desenlace_solver_backend.hpp` *(nuevo, solo backend)* | `inline DesenlaceSolver desenlaceDesdeEstado(CpSolverStatus)`; incluye OR-Tools y **solo lo compilan el backend y sus tests**. |
| `src/backend/include/backend/solver/config/contrato_solver.hpp` / `.cpp` | `PESO_HORAS` y su `static_assert` quedan **históricos** (comentario); funciones puras `demandaTotalHoras` y `sumaTopesHorasAula`. |
| `src/backend/src/solver/servicio_solver.cpp` / `.hpp` | Mapear `ResultadoSolver` a `RespuestaSolver`; inyectar `metadata.fases_sin_optimalidad`. |
| `src/backend/include/backend/services/ServicioGeneracion.hpp` | `RespuestaSolver` con `DesenlaceSolver`; `Estado::NoConcluyente`/`SinRespuesta`; `PLAZO_MS_POR_DEFECTO`. |
| `src/backend/src/services/ServicioGeneracion.cpp` | Mapear desenlaces; causa mínima (H3); plazo por defecto; P4 y P5 con mensajes distintos; reparto y no optimalidad. |
| `src/backend/include/backend/services/ValidadorSalidaSolver.hpp` / `.cpp` | `LineaHorasDocente`, reparto, totales, `excesosTopeHoras` (P5), `docentesSinCapacidad`, `presentable()` y `repartoTexto()`; **se conserva `hayConflictoSolapamiento()`**. |
| `src/middleware/include/middleware/messages.h` | **Solo** `RESP_NO_CONCLUYENTE` (aditivo). **No** se añade un código global de «sin respuesta». |
| `src/middleware/src/client/internalclient.cpp` | **Sin cambios**: el error de socket sigue emitiendo `RESP_ERROR`. |
| `src/app/src/aplicacion_backend.cpp` | Mapear `DesenlaceSolver` a `RESP_EXITO`/`RESP_SIN_SOLUCION`/`RESP_NO_CONCLUYENTE`/`RESP_INVALIDO`. |
| `src/frontend/src/services/puerto_solver_ipc.cpp` / `.hpp` | `RESP_ERROR` de **la operación del solver** → `SinRespuesta`; `!m_cliente` → `SinRespuesta`; `desenlaceParaCodigo` puro. No cambia el comportamiento de otras operaciones. |
| `src/frontend/src/views/schedule_generation_widget.cpp` | Plazo de 60 min y texto «hasta una hora»; reparto y no optimalidad en el diálogo existente. |
| Consumidores revisados, **sin cambios** | `schedule_visualization_widget`, `subject_list_widget`, `teacher_list_widget`: siguen tratando cualquier respuesta no exitosa como error; no dependen de un código nuevo. |
| `test/backend/*`, `test/frontend/*`, `test/test_middleware_transport.cpp` | Tests primero por tarea (detalle en `tasks.md`). |

**Sin tocar:** `src/backend/src/database/`, `src/datos/`, `src/middleware/src/server/`,
`gestor_proceso_backend`, presets, contrato 2.0, esquema, huella y datos.

## 3. Decisiones de implementación

| # | Decisión | Alternativa descartada | Por qué |
|---|---|---|---|
| D1 | **Retirar** la variable `exceso` y el término S1; `PESO_HORAS` y su `static_assert` quedan **históricos** (comentario) | Fijar `e[p] = 0`; eliminar la constante | Menos variables y sin objetivo inerte. La constante se conserva documentada como histórica para no romper referencias; el test deja de tratarla como comportamiento activo. |
| D2 | **`max_time_in_seconds`** | `max_deterministic_time` | La spec fija **reloj** (B5). |
| D3 | **Deadline único** de 55 min; cada fase recibe `restante` | Tres presupuestos independientes | Tres presupuestos podían sumar más de 55. |
| D4 | `PresupuestoFases{ totalSegundos, workers, ejecutarSeparacionHuecos, ejecutarPreferencias }` | Conservar `referencia/separacionHuecos/preferencias` | Los nombres viejos sugerían presupuestos separados; los flags prueban la escalera sin reloj. |
| D5 | **Enum en header limpio** (`desenlace_solver.hpp`, sin OR-Tools) + **función inline en header de backend** (`desenlace_solver_backend.hpp`) | Poner el enum junto a la función con `CpSolverStatus` | `ServicioGeneracion.hpp` y el frontend incluyen el enum; no deben arrastrar OR-Tools. Inline evita un símbolo sin definir y un nuevo target. |
| D6 | **`RESP_SIN_RESPUESTA` no se crea**: en `PuertoSolverIpc`, `code == RESP_ERROR` (error de socket) o `RESP_TIEMPO_AGOTADO` → `SinRespuesta`, **solo** para `OP_RESOLVER_HORARIO`; una respuesta **sin `code`** → `ErrorContrato` | Código global nuevo en `InternalClient` | `InternalClient` lo usan otras vistas; limitar el cambio al puerto del solver no altera el resto del IPC. |
| D7 | `MODEL_INVALID` y contrato → `ErrorContrato` | Tratarlos como NoFactible | NoFactible exige prueba de infactibilidad. |
| D8 | Optimalidad por fase en `metadata.fases_sin_optimalidad`, inyectada por `servicio_solver` | Campos nuevos en `MetadataSalida` | No toca el modelo de datos ni el ida y vuelta. |
| D9 | Comprobación posterior **dentro** de `analizarSalidaSolver`; `presentable()` exige `excesosTopeHoras` vacío | Comprobación aparte en el servicio | Una sola regla, testeable sin interfaz. |
| D10 | Reparto en la misma pasada de `analizarSalidaSolver` | Pantalla o informe nuevo | RF-9/RNF-9 prohíben pantallas nuevas. |
| D11 | `horas_aula` ausente = **0**, sin clases y **se informa** | Rechazar la entrada | La spec lo define así (RF-1). |
| D12 | Timeout automático con **`plazoMs` corto inyectado** | Esperar la hora real | `ServicioGeneracion` ya acepta `plazoMs`. |
| D13 | Health-check **sin cambios** | Suspender la verificación | `InternalClient` encola y no declara caída. |
| D14 | Supersesiones **no aplicadas** | Editarlas en esta rama | Aprobadas como propuestas; aplicarlas es otra decisión. |
| D15 | Si `restante <= 0`, **no se llama a `ejecutar`**: la referencia devuelve `NoConcluyente` y las fases blandas se saltan | Llamar con 0 s | En CP-SAT `max_time_in_seconds(0)` significa **sin límite**, no «sin tiempo». |
| D16 | La spec se **commitea antes de implementar**; el `sha` se anota en `journal.md` | Confiar en `git status` | `git status` no prueba que la base esté commiteada. |

## 4. Tipos e interfaz

```cpp
// backend/solver/desenlace_solver.hpp (nuevo, SIN OR-Tools)
enum class DesenlaceSolver { Exito, Infactible, NoConcluyente, ErrorContrato, SinRespuesta };

// backend/solver/desenlace_solver_backend.hpp (nuevo, solo backend)
#include "backend/solver/desenlace_solver.hpp"
#include "ortools/sat/cp_model.pb.h"
inline DesenlaceSolver desenlaceDesdeEstado(CpSolverStatus estado);

// solver.hpp
struct PresupuestoFases {
    double totalSegundos = 3300.0;        // 55 min de reloj para todo el solver
    int workers = 8;
    bool ejecutarSeparacionHuecos = true;
    bool ejecutarPreferencias = true;
};
struct ResultadoSolver {
    bool exito;
    DesenlaceSolver desenlace = DesenlaceSolver::SinRespuesta;
    QStringList fasesNoOptimas;           // "referencia" | "separacion" | "preferencias"
    HorarioSalida resultado;
    QStringList errores;
    double tiempo_ms = 0.0;
};
struct AnclajeEstructural { QMap<std::tuple<int,int,int>, int> docentePorMateria; }; // sin e*

// ServicioGeneracion.hpp
struct RespuestaSolver {
    DesenlaceSolver desenlace = DesenlaceSolver::SinRespuesta;
    QJsonObject salida;
    QString error;
};
enum class Estado { ..., NoConcluyente, SinRespuesta };
static constexpr int PLAZO_MS_POR_DEFECTO = 3'600'000;

// ValidadorSalidaSolver.hpp
struct LineaHorasDocente { int profesor = -1; QString nombre; int horasAsignadas = 0; int horasAula = 0; };
struct AnalisisSalida {
    QStringList conflictos;            // P4
    QStringList avisos;                // P2 (horas_requeridas)
    QVector<LineaHorasDocente> reparto;
    int totalHorasAsignadas = 0;
    int totalHorasAula = 0;
    QStringList excesosTopeHoras;      // P5 (horas_aula)
    QStringList docentesSinCapacidad;  // tope 0 declarado O ausente
    bool presentable() const { return conflictos.isEmpty() && excesosTopeHoras.isEmpty(); }
    bool hayConflictoSolapamiento() const { return !conflictos.isEmpty(); }  // se conserva
    QString repartoTexto() const;
};
```

## 5. Funciones puras y pseudocódigo

### 5.1 Tope duro (RF-1, RF-6)

```
restriccionTopeHorasProfesor(model, config, vars):
  para cada docente p con clases:
    AddLessOrEqual(Sum(x[p,*,*,*,*]), bloquesAula(config.profesores[p].horas_aula))
```

- No se crea `e[p]`; el objetivo solo suma S2 (suplentes).
- `extraerAnclaje`/`fijarAnclaje` solo con `z*`.
- **V13** no se toca: sigue sin mirar la capacidad de horas.

### 5.2 Presupuesto de reloj (RF-3)

```
restanteMs(deadlineMs, ahoraMs) = max(0, deadlineMs - ahoraMs)          // puro
restanteSegundos(deadlineMs, ahoraMs) = restanteMs(...) / 1000.0        // puro

resolver(config, presupuesto):
  timer.start(); deadline = timer.elapsed() + presupuesto.totalSegundos * 1000
  si restanteSegundos(deadline, timer.elapsed()) <= 0:
     return NoConcluyente("No se ejecutó el cálculo: presupuesto de reloj agotado (0 s).")
  prepararModelo; construirObjetivo
  ref = ejecutar(restanteSegundos(...), workers)          // > 0
  registrar "referencia" (OPTIMAL/FEASIBLE)
  si ref no factible: return fallo(desenlaceDesdeEstado(ref.status()), analizarFallo(ref))
  horario = extraer(ref); anclaje = extraerAnclaje(ref)
  si presupuesto.ejecutarSeparacionHuecos y restanteSegundos(...) > 0:
     prepararModelo; fijarAnclaje(anclaje); Minimize(separacionViernes)
     r2a = ejecutar(restanteSegundos(...), workers)       // > 0
     si FEASIBLE/OPTIMAL: horario = extraer; valor2a = objetivo; registrar "separacion"
     si UNKNOWN/INFEASIBLE: quedarse con la referencia
     si MODEL_INVALID: return ErrorContrato
  si hay2a y config.preferencias no vacío y presupuesto.ejecutarPreferencias
     y restanteSegundos(...) > 0:
     prepararModelo; fijarAnclaje(anclaje); AddEquality(separacion, valor2a)
     Minimize(preferencias)
     r2b = ejecutar(restanteSegundos(...), workers)       // > 0
     si FEASIBLE/OPTIMAL: horario = extraer; registrar "preferencias"
     si UNKNOWN/INFEASIBLE: quedarse con la 2a
     si MODEL_INVALID: return ErrorContrato
  return Exito(horario, fasesNoOptimas)

ejecutar(segundos, workers):
  Q_ASSERT(segundos > 0.0)          // nunca se llama con 0
  params.set_num_search_workers(workers); params.set_random_seed(0)
  params.set_max_time_in_seconds(segundos)
```

- **Nunca** se llama a `ejecutar` con `0` (en CP-SAT, 0 = sin límite); el camino de 0 s se resuelve
  **antes** de construir el modelo o saltando la fase.
- Los **5 min** de reserva quedan **fuera** del deadline (extracción + IPC); el servicio espera 60 min.
- `fasesNoOptimas` añade una fase solo si su respuesta fue `FEASIBLE` (no `OPTIMAL`).

### 5.3 Desenlaces y causa (RF-2, RF-3)

`desenlaceDesdeEstado` (inline, backend): `OPTIMAL|FEASIBLE → Exito`, `INFEASIBLE → Infactible`,
`UNKNOWN → NoConcluyente`, `MODEL_INVALID|otro → ErrorContrato`.

`ServicioGeneracion::alResponder` (switch exhaustivo):

| `DesenlaceSolver` | `Estado` | Mensaje |
|---|---|---|
| `Exito` | `Listo` / `DatosAnteriores` / `CalculoFallido` | Reparto, no optimalidad por fase y docentes sin capacidad |
| `Infactible` | `NoFactible` | `respuesta.error`; si viene vacío, **«no hay causa atribuible»** |
| `NoConcluyente` | `NoConcluyente` | `respuesta.error` o «presupuesto agotado sin prueba» |
| `ErrorContrato` | `ContratoInvalido` | `respuesta.error` o «respuesta/modelo inválido» |
| `SinRespuesta` | `SinRespuesta` | «No hubo respuesta del proceso…» (solo si el puerto lo marca) |

- `TiempoAgotado` sigue siendo el vencimiento del plazo (60 min o inyectado).
- **P4 y P5 se distinguen:** `!presentable()` ramifica por `hayConflictoSolapamiento()` (P4) y, si no,
  por `!excesosTopeHoras.isEmpty()` (P5), con mensajes distintos.
- **Sin cambios globales de IPC:** el puerto del solver traduce `RESP_ERROR` (socket) y
  `RESP_TIEMPO_AGOTADO` a `SinRespuesta`, y una respuesta **sin `code`** a `ErrorContrato`; el resto de
  operaciones conserva su comportamiento. `messages.h` solo añade `RESP_NO_CONCLUYENTE`.

### 5.4 Comprobación posterior y reparto (RF-5, RF-1)

```
analizarSalidaSolver(horario, config):
  ... P4 y P2 actuales ...
  reparto = []
  para cada docente p con clases:
     asignadas = nº de asignaciones con profesor == p
     reparto += {p, nombre, asignadas, config.profesores[p].horas_aula}
     si asignadas > horas_aula: excesosTopeHoras += "docente p: X de Y h de aula"
  totales...
  docentesSinCapacidad = nombres con horas_aula <= 0   // declarado 0 o ausente
  presentable() = conflictos vacío && excesosTopeHoras vacío

repartoTexto(analisis):
  una línea por docente: "Nombre: X/Y h"
  "Total del centro: A/S h"
  si docentesSinCapacidad no vacío: "Sin capacidad (tope 0): N1, N2…"
```

- **P2** sigue contra `horas_requeridas`; **no** se usa como tope.
- `hayConflictoSolapamiento()` se conserva (lo usa `test_validador_salida`).
- El widget añade `analisis.repartoTexto()` y la fase no óptima al diálogo existente.

### 5.5 `horas_aula` ausente (RF-1)

El parser ya usa defecto 0. **No se cambia**; se prueba que un docente sin el campo queda con tope 0, no
recibe clases y aparece en `docentesSinCapacidad` (igual que un 0 declarado) y en el mensaje del
NoFactible.

## 6. Verificación

- **C1 (backend):** `cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend`
- **C2 (completo Release):** `cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release`
- **C3 (frontend sin OR-Tools):** `cmake --preset dev-frontend && cmake --build build-fe --parallel`
  — debe **compilar** con `BUILD_BACKEND=OFF`; demuestra que el enum y la vista no arrastran OR-Tools.
- **C4 (dirigido):** `ctest --preset backend -R "<test>"` o `ctest --preset full-release -R "<test>"`.
- **Prohibido:** `ctest --preset slow-release` y cualquier ejecución del centro real. Lo lanza el usuario.

Resultado esperado: **0 fallos** en C1/C2 y **compilación** en C3. Los recuentos pueden cambiar.

## 7. Trazabilidad

| RF | Tarea(s) | Verificación |
|---|---|---|
| RF-1 | T1, T4, T6 | C1: `test_solver_horarios` (tope duro, tope 0, ausente), `test_servicio_generacion` (docentes sin capacidad), `test_validador_salida` (reparto) |
| RF-2 | T2, T3, T4 | C1: `test_solver_horarios` (mapeo de estados, 0 s), `test_servicio_generacion` (`NoFactible` con texto, sin texto y demanda/topes) |
| RF-3 | T2, T3, T5, T7 | C1: `test_servicio_generacion` (`NoConcluyente`, `SinRespuesta`, 0 s, plazo corto), `test_contrato_generacion` (estados), `test_solver_bloques` (reloj), C3 |
| RF-4 | T1, T7, T8 | C1/C2 completos; sin cambios de contrato/esquema/presets/datos |
| RF-5 | T6 | C1: `test_validador_salida` (reparto, P5 no presentable, P2 separado, P4/P5 distintos), `test_servicio_generacion` (reparto y fases no óptimas) |
| RF-6 | T1 | C1: `test_validador_entrada` (V13 no filtra por capacidad) y `test_solver_horarios` (infactibilidad al resolver) |
| RNF-1 | T1–T8 | C1 y C2 verdes; tests superseded reescritos |
| RNF-2 | T5, T8 | `plazoMs` corto; **sin** slow-release; evidencia manual en `journal.md` |
| RNF-3 | T8 | `git status --short -- src/ test/` limitado al plan; rama sin fusionar; **sha de la spec en `journal.md`** |
| RNF-4 | T3, T8 | C1: middleware sin cambios (`test_middleware_transport`, `test_middleware_health_check`, `test_ipc_aislamiento_bd`); no se mata ni se declara colapso |
| RNF-5 | T2, T6 | C1: diagnóstico/mapeo y formato deterministas; horario/reparto variables documentados |
| RNF-7 | T8 | C1/C2/C3: sin cambios de esquema/contrato/presets; **frontend compila sin OR-Tools** |
| RNF-8 | T1, T6 | C1: tope y comprobación posterior en funciones puras |
| RNF-10 | T8 | Nota de reversión y `sha` en `journal.md` |

**Ningún RF queda sin tarea ni verificación.** RNF-6 se cumple con **8 tareas** (≤10).

## 8. Riesgos

| # | Riesgo | Tratamiento |
|---|---|---|
| 1 | **Infactibilidad con 36 h** | Resultado válido: NoFactible con causa. **La expectativa «876 clases factible» de `test/frontend/test_solver_centro_real.cpp` puede quedar obsoleta**; es lento, no lo lanza el agente y se decide tras la corrida manual. |
| 2 | **`UNKNOWN` real** | «No concluyente» con el texto del motor; prueba pura del mapeo y prueba de servicio. |
| 3 | **Espera de 55/60 min** | Texto «hasta una hora»; plazo corto en pruebas; el usuario ejecuta el cálculo real. |
| 4 | **Reloj del motor** | Los 5 min de reserva absorben el desbordamiento; el servicio espera 60 min. |
| 5 | **`restante == 0`** | No se llama a `ejecutar` (0 = sin límite); camino de 0 s con prueba. |
| 6 | **Tests superseded** | Se reescriben en la misma tarea (exceso, objetivo, ancla, presupuestos, P2, estados, `test_solver_horarios`); no son regresión. |
| 7 | **Conflicto con 019** | Solo mínimo H3; no se implementa el resto ni se fusiona. |
| 8 | **`RespuestaSolver` cambia** | Sustitución mecánica guiada por el compilador. |
| 9 | **IPC global** | `InternalClient` no cambia; el nuevo mapeo vive en el puerto del solver; suites de middleware sin cambios. |
| 10 | **OR-Tools en el frontend** | Enum en header limpio y función inline en header de backend; **C3 dev-frontend** lo verifica. |
| 11 | **Health-check** | No se toca; si se tocara, se reanudaría al terminar la generación. |
| 12 | **Recuentos de suites** | La línea base son las pruebas no ligadas al comportamiento sustituido. |

## 9. Supersesiones

Aprobadas **como propuestas** y **no aplicadas**. Referencia y redacción en `spec.md` → «Supersesiones
(aprobadas como propuestas; no aplicadas)»; resumen en `estado.md`. **No** se edita 002/005/013/014/018/019
ni `docs/constitution.md` en esta rama.

## 10. Procedimiento, commits y evidencia

1. **Commit de la spec** (y plan/tareas) **antes de implementar**; el coordinator lo hace tras esta
   corrección. El `sha` se anota en `journal.md` (T8).
2. Una tarea cada vez: tests en rojo → código → tests en verde → marcar → parar.
3. Evidencia por tarea en `journal.md` (comandos C1–C4, resultados, archivos).
4. **No** se ejecuta `slow-release` ni el cálculo real del centro; si el usuario lo ejecuta, su desenlace
   (incluido un NoFactible con 36 h) se registra en `journal.md`.
