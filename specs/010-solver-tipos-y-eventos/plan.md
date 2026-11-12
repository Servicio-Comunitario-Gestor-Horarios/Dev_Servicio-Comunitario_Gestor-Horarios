# Plan 010 - Solver: tipos de aula y eventos del turno

Base: 49d39dc

> **Dos entregas en una sola spec.** La primera (**T1–T10**, 2026-10-25) está **implementada y
> commiteada** y se conserva entera: contrato 2.0 y rechazo de versiones (RF-1, RF-2), V16/V17 y eventos
> (RF-5), regla de tipo, aula de referencia con desplazamiento/bloqueo y salida con aula real (RF-3,
> RF-4), P4 autorizado, supersesión de la 005 y enmienda constitucional (RF-7, RF-8), MER (RF-9),
> documentación del contrato (RF-10) y benchmark (RNF-2). **Lo único que se sustituye** es el
> diagnóstico de causas (RF-6), la validación de entrada nueva (**V18**) y los textos que los citan.
>
> **Qué se sustituye en este plan.**
> - **§1.2, §1.3 y §3:** se retiran `palomarPorTipo`, `franjasBloqueadasEnFranjasContadas` y el conteo
>   por aula de referencia; `causaSaturacionPorTipo` se **reescribe** como conteo de capacidad del tipo;
>   se añaden las funciones puras del tipo `FranjaSemana` y la constante del tipo general.
> - **§4.4:** causas nuevas (conteo por tipo y «no atribuible»).
> - **§5:** la decisión **9** queda superada; se añaden las decisiones 14–18.
> - **§7:** la enmienda constitucional se reduce a lo que este cambio **añade** (§7.1.5, §7.3, §7.4 y
>   «Comportamiento»); lo que aplicó T7 no se reescribe.
> - **§8–§10:** se añaden **T11–T16**, la trazabilidad de V18/RF-6 y los riesgos nuevos.
>
> **Lo que no se toca:** el **esquema** (no hay columna nueva: el tipo general es una convención de
> nombre), la **009** (no se reabre), el **contrato** (sigue en 2.0 y sin sección nueva), el
> **frontend**, el **seed** (solo se verifica que siembra `Aula de estudio`) y el **motor** (`x`/`y`,
> objetivo y resto de restricciones).
>
> **Cómo leer lo ya ejecutado.** Las secciones §1–§10 describen la entrega completa; las verificaciones
> de **T1–T10** (§8) se conservan como registro de lo ya hecho y **no se re-ejecutan** salvo la regresión
> global de **T16**. La evaluación de viabilidad exigida por el usuario abre el plan (§0).


## 0. Evaluación de viabilidad del conteo de capacidad del tipo

El usuario la exigió antes de implementar. Demuestra, con el código de `49d39dc`, por qué el único test
de la categoría (2) es correcto **para lo que la spec promete**: probar la infactibilidad si falla y no
concluir si no falla.

### 0.1 Qué hay hoy en el modelo (con líneas)

| Pieza | Dónde | Qué fija |
|---|---|---|
| Tipo exacto | `restricciones_aulas.cpp:89-112` (`restriccionTipoExacto`) | `y[c][d][s][a]` solo puede ser 1 si el tipo del aula coincide con el de la materia activa en esa franja |
| D8 exclusividad | `restricciones_aulas.cpp:41-56` (`restriccionAulaNoSolapada`) | `AddAtMostOne` de todas las `y` con la misma `(d, s, a)` |
| D9 una aula | `restricciones_aulas.cpp:60-85` (`restriccionCursoOcupaUnAula`) | `Σ_a y[c][d][s][a] == Σ_{p,m} x[p][m][c][d][s]` |
| D11 / `x` | `variables.cpp:17-44` | la `x` solo se crea para `s ∈ slotsDisponibles(...)`, que es subconjunto del turno del curso |
| `y` | `variables.cpp:46-68` | la `y` solo se crea para aulas cuyo tipo requiere **alguna** materia del curso, y solo en franjas del turno |
| D1/D3 | `restricciones_profesores.cpp:27-63` | un docente por materia-curso y `Σ x = bloques · z` |

### 0.2 Por qué si falla, prueba la infactibilidad

Sea `t` un tipo de aula, `A_t` su número de aulas en el centro y
`F_t = { (d, s) : d < num_dias ∧ s ∈ ⋃_{c : c requiere t} turno(c).slot }`.
La spec define **capacidad(t)** como `A_t × |F_t|`: celdas `(aula de tipo t, día, franja)`.

En cualquier solución, cada bloque de una materia que requiere `t` produce exactamente una clase (D3),
que ocupa exactamente un aula (D9) y esa aula es de tipo `t` (tipo exacto). Ninguna otra clase puede
ocupar la misma celda `(aula, día, franja)` (D8), y la celda está en `F_t` porque la `x` de esa clase
solo existe en franjas del turno de su curso y ese curso aporta demanda de `t`. Por tanto las
`demanda(t)` clases **se inyectan** en las `A_t × |F_t|` celdas de `t`; si
`demanda(t) > A_t × |F_t|`, no existe solución. No es una cota: es un conteo de celdas.

Las tres premisas son del modelo, no del diagnóstico: (i) la demanda se cuenta con `materiasDelCurso` y
`bloquesMateria`, exactamente los bloques que crean `x` y que D3 obliga a programar; (ii) `F_t` incluye
las franjas de **todos** los cursos que aportan esa demanda; (iii) `A_t` cuenta **todas** las aulas del
tipo del centro.

La **excepción de precedencia** de RF-6 (0.6) no afecta a esta prueba: si el conteo falla, sigue
probando la infactibilidad; lo único que cambia es **qué categoría se informa** cuando además un curso
no cabe en su turno.

### 0.3 Por qué si no falla, no concluye

Es necesario, no suficiente. Quedan fuera, al menos:

- **Ventanas de turno entre cursos (condición de Hall).** Fixture verificado y reproducible (es
  `solver_causaNoAtribuiblePorVentanasDeTurno` en T14):
  - `dimensiones = {3, 1, 1, 3, 1, 7}` (3 profesores, 1 materia, 1 aula, 3 cursos, **1 día**, 7
    franjas); `franja_horaria = {60, 7}`; `aula_fija = false` (modo móvil, sin referencias).
  - `materias = {{"Matemática", "Aula de estudio"}}`; `aulas = {{"A1", "Aula de estudio", 30}}`.
  - `planes = {{"PA", {{0, 1, 1}}}, {"PB", {{0, 2, 1}}}, {"PC", {{0, 3, 1}}}}`: para cada año, 1 bloque
    (`horasSemanales = 1`) de la materia 0.
  - `cursos = {{1, "A", "TB", 20, "PA", -1}, {2, "B", "TB", 20, "PB", -1},
    {3, "C", "TC", 20, "PC", -1}}`; turnos `TB = {4}` y `TC = {5,6}`.
  - `profesores` (orden `{nombre, horas_requeridas, horas_aula, turno, plan, anios,
    materias_asignadas, materias_suplente, disponibilidad}`): `Prof A` = `{"Prof A", 1, 1, "", "", {1},
    {0}, {}, {{0, {4}}}}`; `Prof B` = `{"Prof B", 1, 1, "", "", {2}, {0}, {}, {{0, {4}}}}`; `Prof C` =
    `{"Prof C", 1, 1, "", "", {3}, {0}, {}, {{0, {5,6}}}}`. **Ninguno declara `turno` ni `plan`** (vacío
    = cualquiera); `anios` es el año de su curso y `materias_asignadas = {0}`.
  - `F_t = {4} ∪ {5,6} = {4,5,6}` → **capacidad = 1 aula × 3 franjas = 3**.
  - **demanda = 1 + 1 + 1 = 3**: demanda **igual** a capacidad, así que **el conteo no falla**.
  - El modelo es **INFEASIBLE**: `1|A` y `2|B` solo pueden ir en la celda `(0,4)` y el aula es una; no
    pueden compartirla (D8). `3|C` sí tiene dos celdas disponibles.
  - Desenlace: (1) no aplica; (2) no falla; (3) cada curso cabe en su turno (1 ≤ 1 y 1 ≤ 2); (4) cada
    docente tiene al menos una franja de su ventana → **causa no atribuible**.
  El conteo mide la **unión** de las ventanas, no su composición: `{1|A, 2|B}` necesita 2 celdas y solo
  tiene 1 (Hall).
- **Referencia compartida (modo fijo).** Las clases cuyo tipo coincide **deben** ir al aula de referencia
  (RF-3), no a cualquier aula del tipo. Dos cursos que la comparten pueden necesitar más franjas de esa
  aula de las que tiene con el tipo sobrado (el fixture `configDosCursosCasiLlenosConLaboratorios`: 28
  bloques frente a 30 de capacidad del tipo, pero solo 26 franjas utilizables en la referencia).
- **Disponibilidad de docentes**, que es la categoría (4), no (2).

**Lo que no se lista, y por qué.** El «acoplamiento entre tipos dentro de un curso (D4)» **no** produce
infactibilidad por sí solo: V14 acota la suma de bloques del curso a las franjas de su turno, así que un
curso siempre puede colocar sus bloques en su ventana si existen aulas de cada tipo. El acoplamiento que
sí rompe el conteo es el de **ventanas entre cursos** (primer caso) y el de la **referencia compartida**
(segundo).

En todos esos casos el desenlace es **causa no atribuible**: RF-6 no permite inventar una categoría y el
solver no corrige la entrada.

### 0.4 Por qué antes sí podía dar falsos positivos

El código de `49d39dc` restaba del conteo una **estimación** de las franjas que las clases desplazadas
dejan bloqueadas en su referencia (`palomarPorTipo`, ⌈bloques desplazados / aulas del tipo⌉, y
`franjasBloqueadasEnFranjasContadas`), calculada en **pares día×franja** y restada de un conjunto medido
en **franjas del día**, con el descuento limitado a las franjas contadas. Con dos o más días el descuento
podía superar el bloqueo real y declarar (2) donde la causa era (4) —el contraejemplo del reviewer, que
entra como test de regresión en T14—. El conteo nuevo **no resta nada** y **no cuenta referencias**: ese
camino ya no existe.

### 0.5 Hipótesis de las que depende y qué revisar si cambian

- **Tipo exacto, D8, D9 y D11** (0.1): si alguna deja de ser dura, la prueba de necesidad cae y hay que
  rehacerla. No se prevé.
- **`num_dias` y las franjas de los turnos describen la rejilla real**: si aparecen días parciales o
  franjas fuera de turno, `F_t` deja de contener todas las celdas posibles.
- **Un solo tipo general por convención de nombre** (`aula de estudio`): de ello depende **V18**, no el
  conteo. Si se admiten varios tipos generales o cambia el nombre, se revisa V18 y la spec.
- **V14** no interviene en la prueba de necesidad (la `x` ya está restringida al turno); sí explica por
  qué (2) y (3) no compiten en el flujo real (0.6).
- **Reserva vigente del aula de referencia**: el conteo la ignora a propósito. Si se retirara, algunos
  datos pasarían de «no atribuible» a factibles; el conteo seguiría siendo necesario y sin falsos
  positivos.
- **Aforo y capacidad física** del aula: no participan (fuera de alcance, spec 005).

### 0.6 (3) sobre (2): excepción de precedencia aprobada (2026-10-26)

`causaSaturacionPorTipo` devuelve vacío si algún curso necesita más bloques que franjas de su turno
(`solver.cpp:191-194`), y `causaConflictoDeAula` hace lo mismo por grupo (`:313-315`, bloque que se
elimina). El usuario aprobó el 2026-10-26 **mantener ese guard y enmendar RF-6**: «cuando un curso no
cabe en su turno, la causa es (3) conflicto de persona aunque el conteo de capacidad del tipo también
falle, porque es el motivo concreto y comprobable». La spec ya recoge la **excepción de precedencia** en
RF-6, en la definición de «Causa», en RF-7 (punto (c)) y en el criterio 6, acotada a ese único caso. En
consecuencia:

- La versión reescrita de `causaSaturacionPorTipo` **conserva el guard** y su comentario cita RF-6.
- `solver_falloCausaConflictoDePersona` (4 bloques en un turno de 2 franjas, 1 aula → demanda 4 >
  capacidad 2) **sigue dando (3)** y se convierte en la prueba de la excepción; no se reescribe.
- Fuera de ese caso, (2) se informa antes que (3) sin cambios.

### 0.7 Inventario exacto: qué se elimina, qué se reescribe, qué se añade

Líneas de `49d39dc` en `src/backend/src/solver/solver.cpp`:

| Elemento | Líneas | Destino |
|---|---|---|
| `struct DesplazamientosDeGrupo` | 26-29 | **se elimina** (T13) |
| `aulasDelTipo` | 33-38 | **se mueve** a `contrato_solver` **y su copia anónima se borra en T11** (ambigüedad ADL, nota siguiente); las llamadas no cambian |
| `palomarPorTipo` | 40-59 | **se elimina** (T13) |
| `franjasBloqueadasEnFranjasContadas` | 61-90 | **se elimina** (T13) |
| `slotsTurno` | 92-98 | **se mueve** a `contrato_solver` **y su copia anónima se borra en T11** (misma ambigüedad); las llamadas (`114`, `204`, `249`, `286`, `327`) **no cambian** y el cuerpo es idéntico al de la función global |
| `bloquesMateria`, `bloquesCurso` | 100-110 | se conservan |
| `franjasCurso` (`num_dias × slots`) | 112-115 | **se sustituye** por `franjasSemanales` (`|franjasDelCurso|`) (T13) |
| `nombreAula` | 126-128 | **se elimina** (solo lo usaban los mensajes por aula) (T13) |
| `claveAula` | 140-144 | **se elimina** (id.) (T13) |
| `causaEntradaInvalida` | 146-184 | se conserva |
| `causaSaturacionPorTipo` | 186-274 | **se reescribe**: solo `demanda(t) > capacidad(t)`; sin `bloqueosPorReferencia`, sin descuento y sin producto manual; **conserva el guard** de la excepción de precedencia de RF-6 (T13) |
| Bloque por aula de referencia (`cursosPorAula`, `total`, `bloqueadas`, mensajes) | 295-348 | **se elimina** (T13) |
| Rama de aulas móviles | 278-293 | **se elimina** (T13): el caso «modo móvil sin aulas» lo rechaza (1) por **V15** (`solver_config.cpp:714-720`) en el flujo real, y el chequeo de tipo sin aulas (V17) cubre el resto |
| `causaConflictoDeAula` (resto) | 277-352 | **se elimina** (T13); `primeraCausaNoFactible` llama a (2) directamente |
| `causaConflictoDePersona`, `causaConflictoDeAsignacion` | 354-389 | se conservan |
| `primeraCausaNoFactible` | 391-400 | se conserva el orden (1)(2)(3)(4) |
| Comentario de `causaEntradaInvalida` | 147 | **se actualiza** «V1–V17» → «V1–V18» (I3) (T13) |
| Fallback de `analizarFallo` | 537-542 | **se reescribe**: mensaje de **causa no atribuible** (T13) |

Notas del inventario:

- **Borrado de las dos copias anónimas en T11 (corrección del 2026-10-27).** `solver.cpp` declara
  `aulasDelTipo` y `slotsTurno` en un **namespace anónimo**, que se inyecta en el ámbito global por
  *using-directive*. Al declarar las mismas firmas en `contrato_solver.hpp` (globales), ambas son
  candidatas a la vez y el compilador las declara **ambiguas**: **no hay ocultamiento posible** (la
  afirmación contraria del re-QA del plan era **falsa**). Comprobado el 2026-10-26: con las copias
  presentes, **8 errores de ambigüedad**; borrando **solo esas dos definiciones** (llamadas intactas,
  cuerpos idénticos a las funciones globales), compila y la suite queda **50/50**. Por eso el borrado de
  `solver.cpp:33-38` y `solver.cpp:92-98` es de **T11**, no de T13.
- **`claveAula`:** se elimina **solo** la de `solver.cpp` (140-144). El `claveAula` estático de
  `restricciones_aulas.cpp:26` (usado en `:98` y `:130`) es **otro helper** y **se conserva**.
- **Mensaje de (2):** usa el **nombre visible** del tipo tal como lo declara la primera materia que lo
  requiere (`tipo.trimmed()`), **no** la clave. Los tests conservados asertan `«Laboratorio»`,
  `«Aula base»`… (I6).
- **`slotsDelTurno` de `contrato_solver.cpp:54-60`:** su único consumidor es `slotsDisponibles`; en T11 se
  sustituye por la función pública `slotsTurno` y se elimina la copia privada.
- **`slots` es macro de Qt** (`QObject`): no se puede usar como nombre de variable local. En
  `franjasDelCurso` la lista local se llama **`slotsDelCurso`** (ya resuelto en el código de T11).

Otros archivos:

- `contrato_solver.hpp/.cpp` (backend, sin OR-Tools): **se añaden** `FranjaSemana` (con `==`, `<` y
  `qHash`), `slotsTurno`, `franjasDelCurso`, `franjasSemanales`, `demandaDelTipo`, `franjasDelTipo`,
  `aulasDelTipo`, `capacidadDelTipo` y `tipoGeneralAula()`; se retira el `slotsDelTurno` privado.
- `solver_config.cpp`: **se añade** `validarReferenciaGeneral` (V18) después de `validarTiposConAulas`
  (`:746`) y en la lista de errores (`:799-806`).
- Tests: `test/backend/test_solver_horarios.cpp` (causas), `test/backend/test_validador_entrada.cpp`
  (V18) y **nuevo** `test/backend/test_capacidad_tipo.cpp` con
  `add_gtest(test_capacidad_tipo backend/test_capacidad_tipo.cpp backend)` en `test/CMakeLists.txt`
  (junto a `:96`).

## 1. Archivos y responsabilidades

### 1.1 Contrato 2.0 y construcción

- `src/backend/include/backend/solver/config/solver_config.hpp`:
  - `MateriaSolverConfig { QString nombre; QString tipo; }` — **`tipo` es el único nombre** en el
    contrato (igual que `AulaSolverConfig::tipo` y la clave JSON `"tipo"`). El nombre del DTO de
    origen es `MateriaDTO::tipoAula`; se copia a `tipo` y no se usa `tipoAula` dentro de `SolverConfig`.
  - `TurnoConfig { QString nombre; QVector<int> slot; QString inicio; QString fin; }` (la ventana
    del turno hace falta para validar los eventos; las inicializaciones agregadas existentes
    (`{"manana", {0,1,2}}`) siguen compilando y dejan `inicio`/`fin` vacíos).
  - `EventoConfig { QString nombre; QString turno; int despues_de_slot = 0; int orden = 0;
    int duracion = 0; QString inicio; QString fin; }` (sin identidad).
  - `SolverConfig { QString version = "2.0"; ...; QVector<EventoConfig> eventos; ... }`; se retira
    `RecesoConfig`/`recesos`.
- `src/backend/src/solver/config/solver_config.cpp`: parseo/serialización de `eventos`, `inicio/fin`
  de turno y `tipo` de materia/aula; `V1` (secciones, versión 2.0), `V2` (forma/temporalidad de
  eventos), `V16`/`V17`; `toJson` sella siempre `2.0`.
- `src/backend/src/services/ConstructorEntradaSolver.cpp` y su cabecera:
  - `construirJsonEntrada` y `ConstructorEntradaSolver::construir` pasan a devolver
    **`Resultado<QJsonObject>`** (RF-1): si un aula o una materia de la instantánea no declara tipo
    (`datos.aulas[i].tipoAula` / `datos.materias[i].tipoAula` vacíos tras `trimmed()`), la
    construcción **falla** con un mensaje en español que nombra la entidad; no se rellena un tipo por
    defecto ni se emite JSON.
  - Copia `MateriaDTO::tipoAula` → `MateriaSolverConfig::tipo` y `AulaDTO::tipoAula` →
    `AulaSolverConfig::tipo` (se retira el `"regular"` fijo de hoy); `eventos` con
    nombre/posición/orden/duración/horario; **`TurnoConfig::inicio/fin` rellenos desde
    `TurnoDTO::inicio/fin`** (`HH:mm`), sin los cuales V2 rechaza cualquier evento por ventana
    inválida; se retira `recesosDesdeEventos` y la sección `recesos`.
  - Consumidores backend adaptados en la misma tarea: la fachada `ConstructorEntradaSolver::construir`,
    `test/backend/test_constructor_entrada.cpp`, `test/backend/test_validador_entrada.cpp`,
    `test/backend/test_servicio_generacion.cpp` y `test/test_ipc_aislamiento_bd.cpp` (estos dos
    últimos también llaman a `construirJsonEntrada` y compilan en el preset `backend`). Los
    `DatosDominio` de todos ellos deben declarar **`tipoAula` con el nombre del tipo** (no solo
    `idTipoAula`, que es la FK y no viaja en el contrato) y de forma **coherente entre aulas y
    materias**.
  - **Rotura conocida, fuera de alcance:** `src/frontend/src/services/entrada_solver_builder.cpp`
    (`comp.entrada = construirJsonEntrada(...)`) deja de compilar hasta que el equipo del frontend
    adopte `Resultado<QJsonObject>`. **No se toca**: el frontend sigue con `BUILD_FRONTEND=OFF`.
- `src/backend/src/services/CargadorConfiguracionSolver.cpp`: al rechazar por versión, añade el
  **archivo** y la **acción** («genere una configuración 2.0 desde los datos»).
- `src/backend/src/services/ValidadorEntradaSolver.cpp`: sigue siendo el punto de entrada; la
  validación V1–V18 vive en `fromJson`.
- `src/backend/include/backend/solver/config/contrato_solver.hpp` / `.cpp`: `QString claveTipo(const
  QString&)` pura (trim + `toCaseFolded`), usada por el parser/validador y el modelo.
- `src/backend/CMakeLists.txt`: sin fuentes nuevas; los cambios son en las existentes.

### 1.2 Modelo del solver

- `src/backend/include/backend/solver/variables.hpp` y `src/backend/src/solver/variables.cpp`:
  `aulaAssignment[c][d][s][a]` para **todos** los cursos (fijos y móviles), no solo móviles.
- `src/backend/src/solver/restricciones/restricciones_aulas.cpp`: exclusividad solo con `y`, una
  aula por clase para todos los cursos, **tipo exacto**, **aula de referencia** (coincidencia por
  tipo), **desplazamiento** y **aula de referencia vacía/bloqueada**.
- `src/backend/src/solver/solver.cpp`: `extraerSolucion` informa el aula real de **todas** las
  clases (la `y`), no la fija por defecto; el diagnóstico de causas (**reescrito en T13**) se queda
  solo con el conteo de capacidad del tipo, sin cotas ni descuentos, y con el desenlace **no
  atribuible** (§4.4). Las funciones de capacidad viven en `contrato_solver` (T11), no aquí.
- `src/backend/src/services/ValidadorSalidaSolver.cpp`: **P3 sin cambios de producción** (opera sobre
  `asignacion.aula`, el aula real que emite el solver). **P4 sí cambia, por la desviación autorizada
  del apartado siguiente** (claves por recurso, no por franja).

> **Desviación autorizada fuera del plan (2026-10-25, commit `06fc70e`): P4.**
> `ValidadorSalidaSolver` indexaba la ocupación **solo por `(día, slot)`**, así que dos clases
> cualesquiera en la misma franja se denunciaban como «docente en dos clases» y «aula ocupada», aunque
> el docente y el aula fueran distintos; `ServicioGeneracion` convierte un conflicto de P4 en
> `CalculoFallido`, de modo que con más de un curso (el seed tiene 13) toda generación real se
> marcaba como fallida. **El usuario autorizó arreglarlo fuera del plan de la 010.** Arreglo: clave
> `ClaveOcupacion{recurso, día, slot}` y dos mapas separados, `ocupacionProfesor` por **(profesor,
> día, slot)** y `ocupacionAula` por **(aula, día, slot)**; un recurso sin asignar (negativo) no
> ocupa franja, y un índice positivo fuera de rango se indexa por su valor bruto (dos clases con la
> misma aula inexistente sí colisionan y el mensaje la nombra «(desconocida)»). Se conservan P1, P2,
> P3 y el resto de P4 (mismo curso dos veces en la franja). Alternativa descartada: dejar el defecto
> documentado y no tocar producción; mantiene rota la generación con más de un curso y la 010 no
> podría validarse con datos reales.

### 1.3 Verificación y pruebas

- `test/data/preset-1.0.json` (**nuevo, versionado**): configuración 1.0 completa y válida para el
  contrato 1.0 (`"version": "1.0"`, sección `recesos`, sin `eventos` y sin tipos). Es el fixture de
  rechazo de T2; se localiza con `QFINDTESTDATA("../data/preset-1.0.json")` desde
  `test/backend/test_cargador_configuracion.cpp`, sin variables de CMake ni definiciones de compilación.
- `test/backend/test_contrato_solver.cpp`: `claveTipo` (trim, mayúsculas, acentos).
- `test/backend/test_constructor_entrada.cpp`: tipos de aula/materia, sección `eventos`, ausencia de
  `recesos`, identidad del evento fuera, una entrada por evento, `TurnoConfig.inicio/fin` y
  `Resultado` (fallo por aula/materia sin tipo).
- `test/backend/test_cargador_configuracion.cpp`: 2.0 válido, 1.0 rechazado con archivo/versión y
  acción (fixture versionado, bytes intactos), ida y vuelta 2.0, tipos/eventos.
- `test/backend/test_validador_entrada.cpp`: V1 (versión/eventos), V2 (forma/temporalidad), V16,
  V17 y **V18** (referencia de otro tipo, modo móvil, centro sin tipo general).
- `test/backend/test_solver_horarios.cpp`: referencia, desplazamiento y vuelta, bloqueo del aula de
  referencia, exclusividad de desplazadas, móvil, salida con aula real y **causas (T13/T14)**.
- **`test/backend/test_capacidad_tipo.cpp` (nuevo, `add_gtest`, target `backend`)**: unidades del tipo
  `FranjaSemana` y de `demandaDelTipo`/`capacidadDelTipo` (T11); no necesita OR-Tools.
- `test/backend/test_variables_cp_sat.cpp`: compila sin `RecesoConfig` (T1) y comprueba la `y` de
  todos los cursos (T4). **Ojo con el nombre:** el test se registra en CTest como
  **`test_backend_variables_cp_sat`** (`test/CMakeLists.txt:118`), así que `-R 'test_variables_cp_sat'`
  **no casa**; usar `-R 'variables_cp_sat'` o el nombre completo.
- `test/backend/test_solver_benchmark.cpp`: el escenario grande con tipos (estrategia fija en el
  apartado siguiente).
- `test/backend/test_validador_salida.cpp`: salida con aulas reales; **P3 es solo pruebas**, pero
  **P4 cambia** por la desviación autorizada de §1.2 (claves por docente y por aula, `06fc70e`).
- No se tocan `test/frontend/*` ni `test/CMakeLists.txt:127-141`: siguen fuera con
  `BUILD_FRONTEND=OFF`.

**Migración de fixtures (obligatoria al retirar `RecesoConfig` y endurecer el tipo).** Todo test que
construye `SolverConfig` en C++ debe quedar coherente: quitar `cfg.recesos = {}`, poner `version` a
`"2.0"` si la declara, y declarar **tipos coherentes** entre `materias[].tipo` y `aulas[].tipo` (si no,
el modelo no tiene aula válida y el resultado es infactible). Los `DatosDominio` de los tests declaran
`tipoAula` **con el nombre del tipo** y coherente entre aulas y materias; fijar solo `idTipoAula` (la
FK) no sirve, porque el contrato transporta el nombre. Afecta a `test_variables_cp_sat`,
`test_solver_horarios`, `test_solver_benchmark`, `test_servicio_generacion`, `test_constructor_entrada`,
`test_cargador_configuracion` y `test_validador_entrada`. `TurnoConfig` gana `inicio`/`fin`: las
inicializaciones agregadas existentes (`{"manana", {0,1,2}}`) siguen compilando y dejan ambos vacíos
(que es un error de datos, solo válido en configuraciones sin eventos). Los llamadores de
`construirJsonEntrada` (incluidos `test_servicio_generacion` y `test_ipc_aislamiento_bd`) se adaptan a
`Resultado<QJsonObject>`: `QVERIFY(r.ok)` y `r.valor`.

**Reparto de los tests entre tareas (evita arrastres).**

- **T1** reescribe `test_contrato_solver` (forma 2.0, ida y vuelta) y `test_constructor_entrada`
  (tipos, eventos, `inicio/fin`, `Resultado`), y **solo** los `p.version` de fixture que impidan
  compilar/validar (p. ej. `test_servicio_generacion`). **No** toca los casos de versión del cargador.
- **T2** reescribe los tests de versión: `test_cargador_configuracion` completo
  (`version_1_0_pasa`, `versionAusenteSeRechaza`, `configPorDefectoYaDeclaraVersion_1_0`,
  `toJsonSella`…) y el caso de sellado de `test_constructor_entrada`.
- **T3** reescribe los casos de V1 (sección `eventos`), V2, V16 y V17 en `test_validador_entrada`.

**Estrategia fija del benchmark (`test_solver_benchmark.cpp`, T10).** 6 aulas: índices 0–4 con tipo
`"Aula común"` y el índice 5 con tipo `"Laboratorio"`; `num_aulas = 6`. Materias: todas
`"Aula común"` salvo `Tecnología` (índice 14), que requiere `"Laboratorio"`. Referencias de curso
(todas de tipo común): 1A→0, 1B→1, 2A→2, 2B→0, 3A→3, 3B→1, 4A→4, 4B→2. Los cinco cursos de la
mañana tienen aula distinta y los tres de la tarde reutilizan 0, 1 y 2, así que ninguna aula se
comparte dentro del mismo turno; solo 4B (tarde, plan de ciencias de 4.º) desplaza sus 3 bloques de
Tecnología al laboratorio, y la única otra usuaria de la referencia 2 (2A) es de la mañana: el bloqueo
de la referencia no puede chocar. **Las referencias no cambian** respecto del fixture actual: lo único
que cambia es que las aulas 0–4 se tipan como comunes, el aula 4 deja de llamarse «Auditorio» (pasa a
«Aula 203»), aparece el laboratorio en el índice 5 y `num_aulas` pasa de 5 a 6. La solución existe por
construcción (la misma asignación **temporal** de hoy sigue siendo válida con las aulas reetiquetadas)
y el benchmark ya exige `resultado.exito`, además del tiempo.

### 1.4 Documentación

- `docs/constitution.md` §5, §7.1.5, §7.3, §7.4 y §7.5 (la ejecuta el planner; el implementer
  verifica).
- `specs/005-nucleo-solver/spec.md`: anotación de la supersesión (V1 eventos, V1–V17, D10,
  definición de aula fija, salida, RF-7, versión 1.0). La ejecuta el planner.
- `docs/Diagrama-MER.md`: modelo completo (catálogo, tipos en aulas/materias, turnos, cursos,
  eventos).
- `docs/contrato-solver-2.0.md` (nuevo, **en `docs/` raíz, no en `docs/solver/`**): forma del
  contrato 2.0, tipos, eventos, semántica del aula de referencia, salida con aula real y rechazo de
  1.0. `docs/constitution.md:47` declara los documentos de `docs/solver/` como **referencia histórica
  no obligatoria**; RF-10 exige documentación **viva**, así que no puede vivir ahí.

## 2. Contrato 2.0 (forma y validaciones)

### 2.1 Estructuras

```cpp
struct MateriaSolverConfig { QString nombre; QString tipo; };   // tipo obligatorio (V16)
struct AulaSolverConfig    { QString nombre; QString tipo; int capacidad = 0; };

struct TurnoConfig {
    QString nombre;
    QVector<int> slot;   // slots absolutos del día
    QString inicio;      // HH:mm, ventana del turno para V2
    QString fin;
};

struct EventoConfig {    // sin id: la identidad no viaja
    QString nombre;
    QString turno;
    int despues_de_slot = 0;   // 1..slots
    int orden = 0;             // >=1
    int duracion = 0;          // >0
    QString inicio;            // vacío = ausente
    QString fin;
};

struct SolverConfig {
    QString version = QStringLiteral("2.0");
    bool aula_fija = true;
    ...
    QVector<TurnoConfig> turnos;
    QVector<EventoConfig> eventos;
    QVector<PlanSolverConfig> planes;
    ...
};
```

### 2.2 JSON

```json
{
  "version": "2.0",
  "aula_fija": true,
  "dimensiones": { ... },
  "franja_horaria": { ... },
  "turnos": { "turnos": [ { "nombre": "Mañana", "slot": [0,1,...],
                            "inicio": "07:00", "fin": "12:30" } ] },
  "eventos": [ { "nombre": "Receso", "turno": "Mañana", "despues_de_slot": 2,
                 "orden": 1, "duracion": 10, "inicio": "08:20", "fin": "08:30" } ],
  "planes": [ ... ],
  "cursos": [ ... ],
  "profesores": [ ... ],
  "materias": [ { "nombre": "Matemática", "tipo": "Aula de estudio" } ],
  "aulas": [ { "nombre": "Aula 01", "tipo": "Aula de estudio", "capacidad": 40 } ]
}
```

La identidad del evento no viaja; los eventos se distinguen por `nombre`, `turno`, `posición` y
`orden`. Los campos desconocidos se toleran. La sección `recesos` no se acepta como sección
requerida; en un 2.0 se ignora como campo desconocido.

### 2.3 Validaciones (V1, V2, V16, V17 y V18)

- **V1 — estructura y versión:** la versión se comprueba **antes** que las secciones. Ausente,
  `1.0` o desconocida → error de versión; se espera `2.0`. Las secciones requeridas son
  `version`, `dimensiones`, `franja_horaria`, `turnos`, `eventos`, `planes`, `cursos`,
  `profesores`, `materias`, `aulas`. **No hay sección de tipos ni de marca general** (cambio del
  2026-10-26).
- **V2 — forma y temporalidad de eventos:** nombre no vacío; turno existente; posición
  `1..slots`; orden `>=1`; duración `>0`; par inicio/fin completo o ausente; `fin>inicio`;
  `fin−inicio = duración`; intervalo dentro de la ventana del turno y de su posición; orden
  cronológico y sin solapes dentro/entre posiciones. Se reutilizan las funciones puras de
  `backend/services/calculo_eventos.hpp` de la 009 convirtiendo `EventoConfig`/`TurnoConfig` a
  `EventoDTO`/`TurnoDTO`; así la semántica es la misma que la 009.
- **V16 — tipos declarados:** toda aula y toda materia declara un tipo no vacío; el error identifica
  la entidad. Una configuración sin tipos no se completa con valores por defecto.
- **V17 — tipo con aulas:** para cada materia de un curso, su tipo requerido tiene al menos un aula
  en el centro (comparación por `claveTipo`), **en los dos modos**. Error con tipo, materia y curso.
  No se rechaza por un tipo declarado que ninguna materia requiera.
- **V18 — referencia del tipo general (nuevo, T12):** con el modo de aula fija activo, el aula de
  referencia de cada curso debe ser del tipo de `tipoGeneralAula()` (`Aula de estudio`, por
  `claveTipo`); si no, error `V18` con curso, aula y tipo. Con el modo inactivo no aplica. **No hay
  dato nuevo** en el contrato ni en la base. Detalle en §3.2.
- Orden: V1 → V2…V15 → V16 → V17 → V18, como en la 005/009.
- La 011 (posterior) estrecha `duracion_minutos` a 30–90; aquí solo se mantiene `>0`.

### 2.4 Rechazo de 1.0 y presets

- `SolverConfig::fromJson` rechaza por versión con un mensaje que identifica la versión declarada.
- `CargadorConfiguracionSolver::cargar` envuelve el error con el **archivo** y la **acción**:
  «Genere una configuración 2.0 desde los datos con la versión actual». No hay conversión.
- `toJson` sella siempre `2.0` (no `1.0`); un preset 1.0 en disco no se modifica.
- **Fixture 1.0 versionado:** `test/data/preset-1.0.json`, cargado con
  `QFINDTESTDATA("../data/preset-1.0.json")`. El test comprueba que el mensaje incluye la **ruta del
  fixture**, `"1.0"` y la **acción**, y que los **bytes del fixture no cambian** (hash antes/después de
  la carga). El preset real `datos-prueba/preset-generacion-31059.json` **no se usa**: `datos-prueba/`
  está en `.gitignore` (línea 35) y no existe en un clon limpio, así que no puede ser fixture ni
  probar nada con `git status`. Se menciona en la documentación como ejemplo de preset 1.0 que el
  usuario deberá regenerar.

## 3. Funciones puras

- `claveTipo(const QString&)` en `contrato_solver`: `trimmed().toCaseFolded()`; solo para comparar;
  el nombre visible no se reescribe. Tests con `" Laboratorio "`, `"laboratorio"`, `"Física"` y
  `"FÍSICA"`.
- `validarEventos(const SolverConfig&)` (en `solver_config.cpp`) construye los `TurnoDTO` y
  `EventoDTO` de la 009 y usa `duracionFranjaEstimada`, `ventanaDePosicion`,
  `eventoDentroDeVentana` y `haySolapeCronologico`; no lee el reloj.
- **`FranjaSemana` y las funciones de capacidad (T11), en `contrato_solver` (backend, sin OR-Tools).**
  **Ámbito (M5):** el `struct` y las funciones van en **ámbito global** de
  `backend/solver/config/contrato_solver.hpp`, **junto a `claveTipo`**, que ya es global; el
  `namespace ContratoSolver` solo contiene constantes (`PESO_HORAS`, `COSTE_SUPLENTE`,
  `COSTE_MAX_SUPLENTE`) y **no** se amplía. La constante del tipo general es `tipoGeneralAula()`
  (global), no `ContratoSolver::tipoGeneralAula()`.
  - `struct FranjaSemana { int dia = 0; int slot = 0; }` con `operator==` y `operator<` (`día`, luego
    `slot`) y `qHash` (`qHashMulti`), de modo que sirve en `QSet` y en `QMap` sin ambigüedad de
    unidades: una franja es un par `(día, slot)`, no un `slot` repetido por día.
  - `QVector<int> slotsTurno(const SolverConfig&, const QString& turno)`; se retira el
    `slotsDelTurno` privado de `contrato_solver.cpp:54-60` y su consumidor pasa a `slotsTurno`, y **se
    borra la copia anónima de `solver.cpp:92-98`** en T11 (ambigüedad ADL, §0.7): sus llamadas **no
    cambian**.
  - `QSet<FranjaSemana> franjasDelCurso(const SolverConfig&, const CursoSolverConfig&)`:
    `{ (d, s) : d < num_dias ∧ s ∈ slotsDelCurso }`, donde `slotsDelCurso` es la lista local con los
    slots del turno (**`slots` es macro de Qt y no se puede usar como nombre de variable**).
  - `int franjasSemanales(...)`: `|franjasDelCurso|`; **sustituye** el producto manual
    `num_dias × slotsTurno(...).size()` (la sustitución en `solver.cpp` es de T13).
  - `QSet<FranjaSemana> franjasDelTipo(const SolverConfig&, const QString& clave)`: unión de
    `franjasDelCurso` de los cursos con al menos una materia que requiere el tipo.
  - `int demandaDelTipo(...)`: suma de `bloquesMateria(materiasDelCurso(...))` de las materias cuya
    clave de tipo coincide.
  - `int aulasDelTipo(...)` y `int capacidadDelTipo(...)` = `aulasDelTipo × |franjasDelTipo|`.
  - `QString tipoGeneralAula()` → `"Aula de estudio"` (**una sola constante** para V18 y su test).
  - Todas puras y deterministas (sin reloj, sin E/S); se prueban en `test_capacidad_tipo`.
- El constructor conserva el instante inyectado (`ahora`): no lee el reloj y sigue siendo
  determinista.

### 3.1 Construcción de la entrada (RF-1)

Firmas nuevas:

```cpp
Resultado<QJsonObject> construirJsonEntrada(const DatosDominio& datos,
                                             const SolverConfig& parametros,
                                             const QDateTime& ahora);

class ConstructorEntradaSolver {
public:
    static Resultado<QJsonObject> construir(const DatosDominio& datos,
                                             const SolverConfig& parametros,
                                             const QDateTime& ahora);
};
```

Pseudocódigo:

```
construirJsonEntrada(datos, parametros, ahora):
    # 1. Tipos obligatorios (RF-1). Orden determinista: aulas y luego materias, en el orden de la
    #    instantánea; el primer defecto detiene la construcción y no se emite JSON.
    para cada aula a de datos.aulas:
        si a.tipoAula.trimmed().isEmpty():
            return error("El aula «<a.nombre>» no tiene tipo de aula. Asígnelo en el catálogo de
                          tipos antes de generar el horario.")

    para cada materia m de datos.materias:
        si m.tipoAula.trimmed().isEmpty():
            return error("La materia «<m.nombre>» no tiene tipo de aula requerido. Asígnelo antes
                          de generar el horario.")

    # 2. Armado: versión 2.0; materias con `tipo = m.tipoAula`; aulas con `tipo = a.tipoAula`
    #    (sin el "regular" fijo); turnos con nombre, slot e `inicio`/`fin` desde el `TurnoDTO`
    #    (HH:mm); eventos del turno; cursos, planes, profesores y meta.
    para cada turno t de datos.turnos:
        turno.inicio = t.inicio.toString("HH:mm")
        turno.fin    = t.fin.toString("HH:mm")
    ...
    return exito(json)
```

- **`TurnoConfig.inicio/fin` es obligatorio:** V2 compara el intervalo de cada evento con la ventana
  del turno; si el constructor deja esos campos vacíos, **cualquier** evento con horario haría fallar
  V2 por ventana inválida.
- **Un solo nombre de campo:** `MateriaSolverConfig::tipo` / `AulaSolverConfig::tipo`; el DTO de origen
  es `tipoAula`. La guarda de RF-1 mira `datos.*.tipoAula` (el **nombre** del tipo); los fixtures deben
  fijar ese campo, no solo `idTipoAula`.

- **Dos capas coherentes, no duplicadas:** la construcción falla si la **instantánea** trae entidades
  sin tipo (RF-1, mensaje que nombra el aula o la materia); **V16** sigue rechazando una
  configuración **JSON escrita a mano** sin tipos (RF-5). La primera protege al constructor, la
  segunda al cargador.
- Los consumidores usan `resultado.ok`, propagan `mensajeError` y leen `resultado.valor`; en los tests,
  un helper `entradaValida()` hace `QVERIFY(r.ok); return r.valor;` para no repetir el desempaquetado.
- `test_constructor_entrada` compara entradas con `QCOMPARE(a.valor, b.valor)` (no compara `Resultado`,
  que no define igualdad).
- `ConstructorEntradaSolver.hpp` añade `#include "backend/resultado.hpp"`; no hay fuentes nuevas ni
  cambios en `src/backend/CMakeLists.txt`.

### 3.2 V18: aula de referencia del tipo general (T12)

Vive en **`solver_config.cpp`** (parser/validador), no en el motor ni en la base: la regla es de
**entrada**, y el modelo no cambia (la `y` y la reserva ya funcionan con cualquier aula).

```
validarReferenciaGeneral(config):                 # se ejecuta DESPUÉS de validarTiposConAulas
    si !config.aula_fija: return ""               # modo móvil: el aula del curso se ignora (V8)
    para cada curso cu en config.cursos, en orden:
        si cu.aula < 0 o cu.aula >= config.aulas.size(): continue   # V6/V8 ya lo rechazan
        aula = config.aulas[cu.aula]
        si claveTipo(aula.tipo) != claveTipo(tipoGeneralAula()):     # «Aula de estudio»
            return "V18: el aula «<aula.nombre>», referencia del curso <anio><seccion>, es del tipo
                    «<aula.tipo>»; el aula de referencia debe ser del tipo general «Aula de estudio».
                    Asigne al curso un aula del tipo general."
    return ""
```

- Se registra en la lista de errores **después** de `validarTiposConAulas` (`solver_config.cpp:799-806`),
  de modo que el orden es V16 → V17 → V18 (RF-5).
- El tipo general sale de **una sola constante**, `tipoGeneralAula()` (global en `contrato_solver.hpp`),
  no de `ContratoSolver::` (M5); no hay columna en `TiposAula`, ni sección en el contrato, ni consulta a
  la base. La comparación es la misma `claveTipo` del contrato, así que `"AULA DE ESTUDIO"` o
  `"  aula de estudio "` también valen.
- El mensaje nombra **curso, aula y tipo** (RF-5, RNF-3) y dice la **acción disponible** (asignar al
  curso un aula del tipo general), coherente con RNF-3.
- **No se duplica en el motor.** `causaEntradaInvalida` no comprueba V18: el motor es agnóstico al
  nombre del tipo (el tipo exacto ya se cumple por la `y`) y duplicarla invalidaría los fixtures
  unitarios que llaman al solver directamente con tipos de referencia arbitrarios. Alternativa
  descartada: duplicarla y renombrar todos los tipos de los fixtures a `Aula de estudio`; más ruido,
  ninguna cobertura nueva.

## 4. Modelo del solver (pseudocódigo)

### 4.1 Variables

```
para todo curso c:
  para d, s en las franjas del turno de c:
    para a en aulas:
      y[c][d][s][a] = BoolVar          # aula real de la clase (fija o móvil)
x[p][m][c][d][s], z[p][c][m], e[p]    # sin cambios
```

Alternativa descartada: `y[c][m][d][s][a]` (una variable por materia); multiplica las variables por
el número de materias del curso sin aportar nada porque D4 ya limita a una clase por (c,d,s).

### 4.2 Restricciones de aula

```
# D9 generalizada: una clase ocupa exactamente un aula (fijos y móviles).
para c,d,s: sum_a y[c][d][s][a] == sum_{p,m} x[p][m][c][d][s]

# D8: exclusividad en todas las aulas, incluidas las desplazadas.
para d,s,a: AddAtMostOne({ y[c][d][s][a] : c })

# Tipo exacto: si la clase es de la materia m, el aula debe ser de su tipo.
para c,d,s,a, m de c con clave(aula[a]) != clave(materia[m]):
    y[c][d][s][a] + sum_p x[p][m][c][d][s] <= 1

# Aula de referencia (solo modo fijo):
para c con referencia r:
  para d, s en las franjas del turno de c:          # solo donde existe y[c][...]
    # Si el tipo de m coincide con el de r, la clase va a r.
    para m de c con clave(materia[m]) == clave(aula[r]):
        sum_p x[p][m][c][d][s] <= y[c][d][s][r]

    # Desplazamiento: alguna clase de tipo distinto al de r.
    desplazado[c][d][s] = sum_{m: clave(materia[m]) != clave(aula[r])} sum_p x[p][m][c][d][s]

    # Mientras está desplazado, la referencia queda vacía y bloqueada para otros.
    # `y` solo existe para las franjas del turno de cada curso: se recorre el mapa de variables
    # ya creadas y se filtra por d, s y la referencia r (como `restriccionAulaNoSolapada`).
    para (c', dd, ss, a) en claves(vars.aulaAssignment) con c' != c y dd == d y ss == s y a == r:
        y[c'][dd][ss][a] + desplazado[c][d][s] <= 1
```

La `y` de la propia clase desplazada no puede ser `r` porque el tipo no coincide; la `y` de la clase
que coincide es `r` por la implicación. La reserva **no** se extiende a franjas sin clase: si
`desplazado=0` y el curso no tiene clase, no hay `y` que bloquee. La exclusividad con `y` cubre a los
cursos fijos que antes se trataban por `x`.

**Guarda de implementación (bloqueante):** `crearVariables` solo crea `y[c][d][s][a]` para las franjas
del turno del curso. El bucle de bloqueo **no** puede recorrer `c'` de 0 a `num_cursos` e indexar
`y[c'][d][s][r]` a ciegas: en un centro con turnos de mañana y tarde, la combinación `(c', d, s)` puede
no existir y `QMap::operator[]` crearía una BoolVar vacía o abortaría. Se recorre el mapa
`vars.aulaAssignment` ya construido (o se consulta `contains` antes de usar), igual que hace hoy
`restriccionAulaNoSolapada`.

### 4.3 Salida

```
para cada x activa de (p,m,c,d,s):
  a = la única y[c][d][s][a] activa
  AsignacionOutput{ slot=s, materia=m, profesor=p, aula=a }   # aula real
```

Se elimina el atajo `config.aula_fija ? config.cursos[c].aula : y`. El `ValidadorSalidaSolver` P3/P4
opera sobre `asig.aula` real; el curso fijo puede tener clases en aulas distintas.

### 4.4 Causas

Diagnóstico **completo** en `solver.cpp` tras el cambio (T13). Orden fijo (1)(2)(3)(4) y desenlace «no
atribuible»:

```
primeraCausaNoFactible(config):
    causa = causaEntradaInvalida(config)          # (1): V16/V17 (tipos) + turno/materias del curso
    si causa != "": return causa

    causa = causaSaturacionPorTipo(config)        # (2): único test = conteo de capacidad del tipo
    si causa != "": return causa

    causa = causaConflictoDePersona(config)       # (3): el curso no cabe en las franjas de su turno
    si causa != "": return causa

    return causaConflictoDeAsignacion(config)     # (4): materia-curso sin docente suficiente
    # "" (ninguna categoría) => analizarFallo informa «causa no atribuible»

causaSaturacionPorTipo(config):
    # Excepción de precedencia de RF-6 (aprobada el 2026-10-26): si un curso no cabe en su turno, la
    # causa es (3) y una (2) mecánica taparía el motivo concreto. Es el guard actual, ahora normativo.
    para cada curso c: si bloquesCurso(c) > franjasSemanales(c): return ""

    demanda[t] = suma de bloquesMateria de las materias (de todos los cursos) cuya clave de tipo es t
    para cada tipo t en orden de clave (determinista):
        si aulasDelTipo(t) == 0: continue         # V17 lo rechaza antes como (1)
        si franjasDelTipo(t).isEmpty(): continue  # turno inexistente => (3); idem
        si demanda[t] > capacidadDelTipo(t):
            return "Causa (2) conflicto de aula: el tipo «<nombre visible de t>» necesita <demanda>
                    bloques y sus <aulas> aulas ofrecen <capacidad> franjas."
    return ""

analizarFallo(...):                               # INFEASIBLE
    errores.append("El modelo no tiene solución factible (INFEASIBLE).")
    causa = primeraCausaNoFactible(config)
    si causa != "": errores.append(causa)
    si no:          errores.append("Causa: no atribuible. Ninguna categoría de (1) a (4) explica el
                                    fallo; revise la capacidad de los tipos de aula, las franjas de
                                    los turnos y la disponibilidad de los docentes.")
```

- **(1)** se conserva tal cual (incluye la falta de tipos y la falta de aulas del tipo requerido).
- **(2)** no descuenta bloqueos, no cuenta aulas de referencia y no usa cotas: `demanda(t)` frente a
  `capacidad(t)` (definición de la spec, §3).
- **(3)** se informa **antes que (2)** cuando un curso no cabe en su turno: es la **excepción de
  precedencia** de RF-6 (aprobada el 2026-10-26) y el guard la implementa. **(4)** no cambia.
- El mensaje de (2) nombra el **nombre visible** del tipo, la demanda y la capacidad; ya no nombra un
  aula (el conteo es por tipo). El de «no atribuible» no nombra ninguna entidad.
- El solver no libera la reserva ni reasigna a otro tipo para hacer factible; no inventa tipos.

## 5. Decisiones (con alternativa descartada)

1. **Contrato 2.0 con `eventos` y `tipo`.** Se retira `RecesoConfig`/`recesos`; la versión es 2.0.
   Alternativa descartada: mantener 1.0 y añadir campos opcionales; dejaría presets antiguos
   ambiguos y no cumpliría RF-2.
2. **Versión comprobada antes que las secciones.** Un 1.0 sin `eventos` se rechaza por versión, no
   por «falta eventos». Alternativa descartada: comprobar secciones primero; el mensaje no
   identificaría la versión.
3. **Tipo por nombre + `claveTipo`.** La igualdad es exacta sobre la clave normalizada; el nombre
   visible se conserva. Alternativa descartada: comparar cadenas crudas (rechazaría variantes de
   mayúsculas) o jerarquías (fuera de alcance).
4. **`y[c][d][s][a]` para todos los cursos.** Es la variable de aula real; las restricciones de tipo
   se condicionan a la materia activa. Alternativa descartada: `y` por materia (más variables) o
   mantener la lógica de aulas fijas por `x` (no permite desplazar).
5. **Referencia que coincide ⇒ se usa la referencia** (no otra aula del mismo tipo). Alternativa
   descartada: permitir cualquier aula del mismo tipo; rompería «el curso vuelve a su aula».
6. **Bloqueo de la referencia solo en franjas con clase desplazada.** `y[c'][d][s][r] +
   desplazado <= 1`. Alternativa descartada: reservar la referencia todo el turno; contradice la
   spec y desperdicia aulas.
7. **Salida con la `y` real.** Alternativa descartada: informar la fija del curso; es el defecto
   actual y contradice RF-4.
8. **V17 en ambos modos.** Alternativa descartada: solo móvil; la regla del usuario es dura siempre.
9. **Causa de saturación por tipo en `causaConflictoDeAula`.** ~~Alternativa descartada: confiar solo
   en el estado INFEASIBLE sin categoría; RF-6 exige la causa.~~
   > **Superada el 2026-10-26.** La categoría (2) deja de ser «saturación por tipo o referencia
   > bloqueada»: es **solo** el conteo de capacidad del tipo (decisión 15) y el fallback es «no
   > atribuible» (decisión 17). El `causaConflictoDeAula` de esta decisión **se elimina**.
10. **Constitución y 005 las edita el planner.** El implementer solo verifica; la skill se lo
    prohíbe. Alternativa descartada: que las edite el implementer; no autorizado.
11. **MER y contrato documental en documentos vivos.** `docs/Diagrama-MER.md` completo y
    `docs/contrato-solver-2.0.md` (raíz de `docs/`, **fuera de `docs/solver/`**, que
    `docs/constitution.md:47` declara histórico y no obligatorio); los documentos históricos no se
    reescriben. Alternativa descartada: dejar el contrato solo en la spec (RF-10 pide documentación
    viva) o ponerlo en `docs/solver/` (la propia constitución lo desautoriza).
12. **Tipo ausente ⇒ la construcción falla (decisión del usuario, 2026-10-25).**
    `construirJsonEntrada` y la fachada `ConstructorEntradaSolver::construir` devuelven
    **`Resultado<QJsonObject>`**; si un aula o una materia de la instantánea no declara tipo, se
    devuelve `error` con un mensaje en español que nombra la entidad y **no se emite JSON**. El
    instante sigue inyectado y la función sigue siendo pura y determinista.
    Alternativas descartadas: (a) copiar el tipo vacío y dejar que **V16** lo rechace al cargar; y
    (b) una comprobación previa separada al estilo de `ValidadorEntradaSolver`. Ambas alejan el fallo
    de su causa y permiten construir una entrada que nunca podrá validarse.
13. **P4 indexa por recurso, no por franja (desviación autorizada por el usuario, 2026-10-25,
    `06fc70e`).** `ValidadorSalidaSolver` usa una clave `(recurso, día, slot)`: `ocupacionProfesor`
    por `(profesor, día, slot)` y `ocupacionAula` por `(aula, día, slot)`. Sin esto, dos cursos en
    paralelo en aulas y con docentes distintos se denuncian como conflicto y `ServicioGeneracion`
    devuelve `CalculoFallido` con cualquier horario real. Está **fuera del plan original** (§1.2 lo
    daba por «sin cambios de producción»): lo autorizó el usuario al detectarse en T5.
    Alternativa descartada: dejarlo como defecto documentado y no tocar producción; deja la
    generación real inutilizable y la validación de la 010 no se habría podido hacer con datos reales.
14. **Excepción de precedencia de (3) sobre (2) (decisión del usuario, 2026-10-26).** La spec
    **enmienda RF-6** (y la definición de «Causa», RF-7 y el criterio 6): si un curso no cabe en su
    turno, la causa es **(3)** aunque la capacidad del tipo también falle. `causaSaturacionPorTipo`
    implementa la excepción con el guard (`solver.cpp:191-194`); el orden (1)(2)(3)(4) no cambia fuera
    de ese caso. Alternativa descartada (QA del 2026-10-26, antes de aprobar la enmienda): quitar el
    guard y aceptar (2) mecánica; se descartó porque oculta el motivo concreto y comprobable.
15. **Causa (2): solo conteo de capacidad del tipo (2026-10-26).** `demanda(t)` frente a
    `capacidad(t) = aulas(t) × días × franjas de la unión de turnos`; **sin** descuento de bloqueos y
    **sin** conteo por aula de referencia. Es el único test de (2), **salvo la excepción de (3)**.
    Alternativa descartada: conservar el conteo por referencia sin bloqueos; con varios cursos su
    cuenta exacta exige resolver el empaquetamiento (lo que la heurística aproximaba) y, con un solo
    curso, no puede fallar.
16. **Tipo `FranjaSemana` (decisión del usuario, 2026-10-26).** Un `(día, slot)` con `==`, `<` y
    `qHash` en `contrato_solver`, **en ámbito global junto a `claveTipo`**, usado por las cuentas de
    capacidad; elimina los productos manuales por `num_dias` (`solver.cpp:114, 263, 288, 330`). Se
    **acota a las cuentas**: no se refactorizan `x`/`y` ni el resto del modelo. Alternativa descartada:
    seguir multiplicando a mano (fue el origen del error de unidades) y tipar también `x`/`y` (refactor
    sin efecto en RF-6).
17. **«Causa no atribuible» como desenlace explícito (2026-10-26).** Si (1)–(4) no aplican,
    `analizarFallo` informa «Causa: no atribuible» sin nombrar entidad. Alternativa descartada:
    mantener el mensaje genérico «no se pudo identificar una categoría concreta» (no dice el desenlace
    y sugiere que el sistema falló) o inventar la categoría más probable (prohibido por RF-6).
18. **V18 en el validador, con una constante y sin tocar el motor (2026-10-26).** `validarReferenciaGeneral`
    en `fromJson`, con `tipoGeneralAula()` (global, como `claveTipo`); no hay columna ni sección nueva y
    la comprobación es la misma `claveTipo`. Alternativa descartada: duplicarla en
    `causaEntradaInvalida` (obligaría a renombrar los tipos de todos los fixtures unitarios del motor
    sin aportar cobertura).

## 6. Supersesión enumerada de la 005

La 010 supersede: la definición «Curso con aula fija: usa siempre su aula»; **D10**; el **criterio 2**
(«D1–D11 y D13») **salvo D10**; la salida «con aula fija, su aula fija»; **RF-7** de la 005; la
versión **1.0** (definición, RF-10 y criterio); y **V1** (sección de recesos). **D9 se generaliza**
(**todas** las clases —fijas y móviles— ocupan exactamente un aula, del tipo requerido), **D8 se
extiende** a las desplazadas y **V8 se mantiene**. El resto de la 005 sigue vigente. La anotación en el
texto de la 005 la hizo el planner en **T7** (`f054b2e`).

**Añadido del 2026-10-26 (lo anota el planner en T15):**

- **Definición «Causa de un resultado no factible»** (`005:314-320`): (a) la categoría (2) se decide por
  el **conteo de capacidad del tipo** y su descripción deja de citar «(invariantes D8/D9/D10)» (D10 está
  supersedida; la (2) ya no es solo exclusividad); (b) «las categorías **cubren** todas las invariantes
  duras D1–D11 y D13» pasa a «se informa la primera categoría aplicable y, si ninguna lo es, la causa se
  informa como **no atribuible**»; (c) se añade la **excepción de precedencia** de RF-6 (un curso que no
  cabe en su turno es (3) aunque la capacidad del tipo también falle).
- **RF-6 «Comportamiento y contrato del solver»** (`005:484-487`): el párrafo «la causa se informa con
  las categorías … en su orden» se sustituye por el orden y las condiciones de RF-6 de la 010
  (**incluida la excepción de precedencia de (3) sobre (2)**) y por la causa no atribuible; las cuatro
  categorías y su orden no cambian.
- **Catálogo de validaciones:** **V1–V17 → V1–V18** (la 010 añade V18).

## 7. Enmienda de la constitución (la ejecuta el planner, T15)

> **Lo que aplicó T7 (`f054b2e`) no se reescribe:** §5, §7.1.5 (aula de referencia con desplazamiento,
> vuelta y bloqueo), §7.3 (referencia, tipos por clave, eventos, versión 2.0), §7.4 (V1–V17 y V1 de
> eventos) y §7.5 (eventos) ya están en `docs/constitution.md`. **Este cambio solo añade cinco cosas.**

Líneas de `docs/constitution.md` en `49d39dc`:

- **§7.1.5, línea 64 — invariante del aula.** Añadir al final del punto 5, sin tocar el resto:
  > El **aula de referencia es siempre del tipo general/ordinario del centro** (el tipo de clave
  > normalizada `aula de estudio`); las aulas de los demás tipos **nunca** quedan reservadas.

- **§7.3, línea 78 — viñeta de Cursos.** Añadir a la frase del aula: «… su aula proviene de los datos
  del curso y **debe ser del tipo general/ordinario** (V18) …», y añadir, si no está, «La configuración
  **no** lleva ningún dato que distinga los tipos generales: la convención es el nombre del tipo.» (No
  repetir «el aula del curso es su aula de referencia»: ya está en esa línea.)

- **§7.4, línea 89 — validación previa.** Sustituir **V1–V17** por **V1–V18** y añadir V18 a la
  remisión final (hoy «el contenido de V1, V2, V16 y V17»). No se duplica el contenido de V18.

- **§7 «Comportamiento», línea 56.** «declara el problema no factible **con la causa**» pasa a
  «declara el problema no factible **con la causa cuando una categoría la explica y sin atribuirla
  cuando ninguna aplica**».

- **`specs/005-nucleo-solver/spec.md`** (lo mismo que §6): anotar la supersesión de la definición de
  causa y de RF-6 (005:314-320 y 005:484-487) y el rango **V1–V18**.

**Verificación de T15** (texto exacto, sin depender del número de línea):

```
grep -n "tipo general" docs/constitution.md
grep -n "V1–V18" docs/constitution.md specs/005-nucleo-solver/spec.md
grep -n "sin atribuirla" docs/constitution.md
grep -n "no atribuible" specs/005-nucleo-solver/spec.md
```

## 8. Verificación por tarea

Comandos desde la raíz. Compilación siempre: `cmake --build --preset backend --parallel`.

> **T1–T10: ejecutadas y commiteadas (2026-10-25, HEAD `49d39dc`, suite 49/49).** Su verificación se
> conserva como registro y **no se re-ejecuta**; la regresión final es **T16**. Las tareas nuevas de
> este cambio son **seis** (T11–T16), de **20–30 min**, en orden de dependencia y con el límite de 10
> tareas del proyecto.

### T1. Contrato 2.0, constructor con `Resultado` y consumidores (30 min)

```
cmake --build --preset backend --parallel
ctest --test-dir build-backend --output-on-failure -R 'test_contrato_solver|test_constructor_entrada|test_servicio_generacion'
```

Esperado: **todo el árbol compila** (sin `RecesoConfig`; `test_variables_cp_sat`, `test_ipc_aislamiento_bd`
y `test_servicio_generacion` incluidos, adaptados a `Resultado<QJsonObject>`) y los tests del filtro
están verdes. El contrato declara 2.0, `materias[].tipo`, `aulas[].tipo`, `turnos[].inicio/fin`, exige
la sección `eventos` (ya no `recesos`) y sella 2.0 al serializar; el constructor copia los tipos,
rellena `inicio/fin` de cada turno y emite una entrada por evento sin identidad; la ida y vuelta 2.0
reproduce tipos/eventos. Además, en `test_constructor_entrada`:

- **aula sin tipo** → `r.ok == false`, `r.mensajeError` contiene el **nombre del aula** y la palabra
  «tipo», y `r.valor` está vacío (no se emitió JSON);
- **materia sin tipo** → `r.ok == false` y `r.mensajeError` contiene el **nombre de la materia**;
- **tipos completos** → `r.ok == true` y `r.valor` trae la entrada 2.0, con `turnos[].inicio/fin`
  rellenos (mismas secciones y contenido que antes, ahora desempaquetado con `.valor`).

Los casos de **rechazo y valor por defecto** de versión (`version_1_0_pasa`,
`configPorDefectoYaDeclaraVersion_1_0`, archivo sin versión) **no** entran aquí: son de T2. El
**sellado de `toJson` en 2.0** sí es de T1 (es el cambio de contrato) y se comprueba en
`test_constructor_entrada`. La **reescritura de los casos V1/V2/V16/V17** es de T3. En T1 solo se
ajustan los `p.version` de fixture (p. ej. `test_servicio_generacion`) para que la entrada generada
sea 2.0 y pueda validarse.

### T2. Rechazo de 1.0/sin versión, cargador y tests de versión (30 min)

```
cmake --build --preset backend --parallel
ctest --test-dir build-backend --output-on-failure -R '^test_cargador_configuracion$'
```

Esperado: build y verde. Se reescriben los casos de versión de `test_cargador_configuracion`
(`version_1_0_pasa`, `configPorDefectoYaDeclaraVersion_1_0`, `toJsonSella2_0`…), sin arrastrar los de
V1/V2/V16/V17 (T3). El fixture versionado `test/data/preset-1.0.json` se localiza con `QFINDTESTDATA`
(se comprueba que la ruta no está vacía) y se rechaza como entrada inválida con la **ruta del
fixture**, la **versión** y la **acción**; el test comprueba el **hash de los bytes antes y después**
de la carga (el archivo no se modifica). Un 2.0 válido carga, `toJson` sella 2.0 y no se completan
tipos por defecto. No se usa `datos-prueba/` (ignorado por git).

### T3. Casos V1 (eventos), V2, V16 y V17 (30 min)

```
cmake --build --preset backend --parallel
ctest --test-dir build-backend --output-on-failure -R 'test_validador_entrada|test_contrato_solver'
```

Esperado: build y verde. **Casos de V1:** exigen la sección `eventos` y ya no `recesos` (el
comportamiento se cambió en T1; aquí solo se reescriben sus pruebas), incluida la versión ausente/1.0
como entrada inválida. V2 rechaza nombre
vacío, turno inexistente, posición 0/fuera, orden 0, duración 0, par incompleto, `fin<=inicio`,
diferencia distinta de la duración, intervalo fuera de la ventana de turno/posición, solapes y orden
cronológico invertido; V16 rechaza aula/materia sin tipo; V17 rechaza un tipo requerido sin aulas
(también en aula fija); `claveTipo` normaliza mayúsculas, espacios y acentos.

### T4. Modelo: tipo, referencia, desplazamiento, bloqueo y salida (30 min)

```
cmake --build --preset backend --parallel
ctest --test-dir build-backend --output-on-failure -R 'test_solver_horarios|variables_cp_sat'
```

Esperado: build y verde. Un curso fijo con materia del tipo de su referencia usa la referencia; una
materia de otro tipo se imparte en un aula de ese tipo y el curso vuelve; mientras hay una clase
desplazada, otro curso no usa la referencia en esa franja; la exclusividad cubre las desplazadas; un
curso móvil usa un aula del tipo exacto; la salida informa el aula real de cada clase;
`test_backend_variables_cp_sat` (nombre real en CTest; `-R 'test_variables_cp_sat'` no casa) ve la `y`
de todos los cursos.

### T5. Validador de salida con aulas reales (20 min; P3 solo pruebas)

```
cmake --build --preset backend --parallel
ctest --test-dir build-backend --output-on-failure -R '^test_validador_salida$'
```

Esperado: build y verde. **P3 no cambia** (ya valida sobre `asignacion.aula`, el aula real); se migran
los fixtures (tipos coherentes) y se añade el caso de un curso fijo con clases en aulas distintas: no
se marca como si todas estuvieran en su referencia y la capacidad se comprueba contra el aula real.
**Desviación autorizada (2026-10-25, `06fc70e`):** P4 **sí** cambia respecto de este plan (§1.2):
`ocupacionProfesor` y `ocupacionAula` se indexan por `(profesor, día, slot)` y `(aula, día, slot)`, no
solo por franja. Los casos que lo cubren son `dosCursosEnParalelo_conAulasYdocentesDistintos_esPresentable`,
`docenteYaulaSinAsignar_noSonConflictoEntreSi`, `aulaInexistenteRepetidaEnLaFranja_noPresentable` y los
de solape aislado por recurso.

### T6. Causas de infactibilidad (20 min)

```
cmake --build --preset backend --parallel
ctest --test-dir build-backend --output-on-failure -R 'test_solver_horarios'
```

Esperado: build y verde. Un laboratorio insuficiente para varios cursos desplazados produce
**conflicto de aula**, no una reasignación a otro tipo ni la liberación de la referencia; un tipo sin
aulas es **entrada inválida** (V16/V17); el resto de invariantes sigue reportando su categoría.

### T7. Supersesión de la 005 y enmienda constitucional (20 min; la ejecuta el planner)

```
grep -n "aula de referencia\|V1–V17\|tipos de aula\|evento.*metadata" docs/constitution.md
grep -n "eventos" specs/005-nucleo-solver/spec.md
```

Esperado: el planner ha aplicado el texto de §5, §7.1.5, §7.3, §7.4 y §7.5 y la anotación de la
005; el implementer **solo verifica** y no edita esos archivos. Sin este paso, la 005 y la
constitución seguirían describiendo el aula fija y la versión 1.0.

### T8. Diagrama entidad-relación completo (20 min)

```
grep -n "TiposAula\|Eventos\|Turnos\|Cursos\|id_tipo_aula" docs/Diagrama-MER.md
```

Esperado: el MER incluye `TiposAula`, `Aulas`/`Materias` con `id_tipo_aula`, `Turnos`, `Eventos`
con nombre/posición/orden y `Cursos` con `turno`/`aula`, con sus relaciones; no es solo «quitar
recesos» (el diagrama anterior no tenía turnos, cursos ni eventos).

### T9. Documentación viva del contrato 2.0 (20 min)

```
grep -n "2.0\|eventos\|aula de referencia\|1.0\|aula real" docs/contrato-solver-2.0.md
```

Esperado: `docs/contrato-solver-2.0.md` (raíz de `docs/`, **fuera de `docs/solver/`**) describe la
forma 2.0, tipos por nombre/clave, eventos, salida con aula real, semántica del aula de referencia
(desplazamiento, vuelta, vacía/bloqueada), rechazo de 1.0 con archivo/versión/acción y no duplica la
guía de servicios de la 009. No se escribe nada en `docs/solver/`: la constitución lo declara
referencia histórica no obligatoria.

### T10. Regresión global y desempeño con tipos (20 min)

```
cmake --build --preset backend --parallel
ctest --preset backend
ctest --test-dir build-backend -N
```

Esperado: **100 % verde**, sin `test_frontend_conversion` ni `test_generacion_seed`; el benchmark con
la estrategia fija de §1.3 (6 aulas: 5 comunes + 1 laboratorio; solo `Tecnología` requiere laboratorio;
referencias repartidas por turno) devuelve **factible** y sigue por debajo de 60 s; los tests V16/V17,
la referencia y la salida real están incluidos. **El seed no se intenta generar** con la regla dura: se
anota en `docs/guias/datos-prueba.md` §6 (límites) que la demanda de ciencias (≈104 bloques) no cabe en
1 laboratorio × 40 franjas y que la 009 ya retiró la promesa de generabilidad;
`grep -n "regla de tipo\|no es generable\|no genera horarios" docs/guias/datos-prueba.md` lo confirma.
El frontend sigue fuera hasta que su equipo migre.

### T11. `FranjaSemana` y funciones puras de capacidad y demanda (30 min)

```
cmake --build --preset backend --parallel
ctest --test-dir build-backend --output-on-failure -R 'test_capacidad_tipo|test_contrato_solver|test_backend_variables_cp_sat'
grep -n "int aulasDelTipo\|QVector<int> slotsTurno" src/backend/src/solver/solver.cpp
ctest --preset backend
```

Esperado: build y verde; el `grep` **no devuelve nada**; la **suite completa sigue verde** (50/50: los 49 existentes + `test_capacidad_tipo`). En
`contrato_solver` (target `backend`, sin OR-Tools) y **en ámbito global, junto a `claveTipo`** (el
`namespace ContratoSolver` solo lleva constantes y no se amplía): `FranjaSemana` con `==`, `<` y
`qHash`; `slotsTurno`, `franjasDelCurso`, `franjasSemanales`, `demandaDelTipo`, `franjasDelTipo`,
`aulasDelTipo`, `capacidadDelTipo` y `tipoGeneralAula()`. Se retira el `slotsDelTurno` privado de
`contrato_solver.cpp:54-60` (su consumidor pasa a `slotsTurno`) **y se borran las dos copias anónimas de
`solver.cpp` (`aulasDelTipo` 33-38 y `slotsTurno` 92-98)**: con ellas presentes, las firmas globales
nuevas hacen que el compilador vea **dos candidatas por *using-directive*** y dé **8 errores de
ambigüedad** (no hay ocultamiento posible, §0.7). **Las llamadas de `solver.cpp` no se tocan** y los
cuerpos son idénticos a las funciones globales; el resto del inventario de causas sigue en T13. En
`franjasDelCurso`, la lista local de slots se llama **`slotsDelCurso`** (`slots` es macro de Qt). En
`test_capacidad_tipo` (nuevo, `add_gtest`, target `backend`): **unidades** —2 días × 2 franjas producen 4
`FranjaSemana` distintas, no 2—; unión de turnos `{4,5} ∪ {5,6} = 3` franjas/día; `demandaDelTipo` suma
los bloques de todos los cursos y no los de un tipo ajeno; `capacidadDelTipo = aulas ×
|franjasDelTipo|`; orden/igualdad/`qHash` coherentes; `tipoGeneralAula()` es `"Aula de estudio"` y
`claveTipo` de `"  AULA DE ESTUDIO "` coincide. **Rojo primero**: el test no compila hasta que existan
las firmas.

### T12. V18 en el validador con la constante del tipo general (25 min)

```
cmake --build --preset backend --parallel
ctest --test-dir build-backend --output-on-failure -R 'test_validador_entrada'
```

Esperado: build y verde. Se añade `validarReferenciaGeneral` (§3.2) **después** de
`validarTiposConAulas`, con el mensaje que nombra **curso, aula y tipo** y la **acción** (asignar al
curso un aula del tipo general); el tipo general sale de `tipoGeneralAula()` (global, junto a
`claveTipo`). Casos nuevos: `v18_referenciaSobreLaboratorio_falla` (aula de referencia = laboratorio,
materias del tipo general → `V18:` con `1A`, nombre del aula y `Laboratorio` y la acción);
`v18_referenciaSobreTipoNoGeneral_falla` (catálogo con `Aula regular` y solo materias de laboratorio →
V17 pasa y V18 rechaza); `v18_aulasMoviles_noAplica` (`aula_fija=false`, `aula=-1` → pasa, y también
con una materia que requiere el tipo general si hay aulas de ese tipo);
`v18_sinAulaGeneralEnModoFijo_llegaPorV17` (materias del tipo general sin aulas de ese tipo → `V17:`);
`v18_sinAulaGeneralEnModoMovil_llegaPorV17` (móvil, materia que requiere el tipo general y ningún aula
de ese tipo → `V17:`, V18 no aplica); `v18_referenciaGeneralPorClave_pasa` (`"  AULA DE ESTUDIO "`).
`jsonValido_pasa` sigue en verde (su referencia ya es `Aula de estudio`).

### T13. Causa (2) por capacidad: producción y tests acoplados (30 min)

> **Por qué una sola tarea y no dos (M2).** El cambio de producción y los seis tests que fijan la
> conducta nueva no se pueden separar sin cerrar una tarea con la suite en rojo (la skill lo prohíbe).
> La división por tiempo es **T13 = producción + tests acoplados** (30 min) y **T14 = pruebas nuevas y
> renombrados** (30 min), dentro del límite de 10 tareas.

```
cmake --build --preset backend --parallel
ctest --test-dir build-backend --output-on-failure -R 'test_solver_horarios'
grep -n "palomarPorTipo\|franjasBloqueadasEnFranjasContadas\|causaConflictoDeAula\|DesplazamientosDeGrupo\|nombreAula\|claveAula" src/backend/src/solver/solver.cpp
```

**Paso 1 (rojo, al inicio).** Se escribe el test de regresión
`solver_causaDeAulaContraejemploDelReviewer_esCausaDeDocente` (M7) y **falla** contra `49d39dc`, que
devuelve (2). Fixture exacto (M6):

- `dimensiones = {2, 2, 2, 2, 2, 7}`, `franja_horaria = {60, 7}`; turnos `TB = {5,6}` y `TX = {4,5}`;
  2 días.
- Materias: `Ciencias` (tipo `Aula base`) y `Laboratorio` (tipo `Laboratorio`); aulas: `R1`
  (`Aula base`) y `L1` (`Laboratorio`).
- Planes: `Plan Y = {{0, 2, 4}}` (Ciencias, año 2, 4 bloques) y `Plan X = {{1, 1, 2}}` (Laboratorio,
  año 1, 2 bloques).
- Cursos: `2|Y` turno `TB`, referencia `R1` (índice 0); `1|X` turno `TX`, referencia `R1`.
- **Quién imparte y cuándo:** `Prof Y` (materias `{0}`, años `{2}`) disponible en `(0,{5,6})` y
  `(1,{5,6})` → 4 celdas para los 4 bloques de `2|Y`; `Prof Lab` (materias `{1}`, años `{1}`)
  disponible **solo en `(0,4)`** → 1 celda para los 2 bloques de `1|X`.
- Cifras: `Aula base` demanda 4 = capacidad 1 × 2 días × 2 franjas = 4; `Laboratorio` demanda 2 ≤
  capacidad 1 × 2 × 2 = 4; (3) 4 ≤ 4 y 2 ≤ 4; (4) `Prof Lab` no alcanza → **`Causa (4)`, sin (2)**.
  Con `49d39dc` el descuento de bloqueos da (2) (palomar ⌈2/1⌉ = 2, fuera de lo contado 1 → descuento 1
  → 4 − 1 = 3 < 4): **rojo**.

**Paso 2 (producción).** Inventario de §0.7 aplicado: se eliminan `palomarPorTipo`,
`franjasBloqueadasEnFranjasContadas`, `DesplazamientosDeGrupo`, `causaConflictoDeAula` (sus dos
bloques), `nombreAula` y la `claveAula` de `solver.cpp`; `causaSaturacionPorTipo` se reescribe (§4.4)
con `demandaDelTipo`/`capacidadDelTipo` y **conserva el guard** (excepción de RF-6); `franjasCurso` pasa
a `franjasSemanales`; `primeraCausaNoFactible` llama a (2) directamente; el fallback de `analizarFallo`
dice **no atribuible**; el comentario de `causaEntradaInvalida` pasa a «V1–V18» (I3).

**Paso 3 (tests acoplados).** Se adaptan los tests que dependen de la producción para cerrar en verde.
**El `grep` de arriba no devuelve nada**; ojo: el `claveAula` estático de
`restricciones_aulas.cpp:26` (usado en `:98` y `:130`) es **otro helper** y **se conserva** (M3).

**Inventario: 13 tests de causas = 12 `solver_causa*` + `solver_falloDocenteNoElegibleNoSeInformaComoConflictoDeAula`**
(`test_solver_horarios.cpp:983`). **Ninguno se elimina**: todos siguen cubriendo un escenario.

| Test (nombre actual) | Destino | Por qué |
|---|---|---|
| `solver_causaDeAulaConSemanaAnchaNoInventaElConflicto` | **se conserva**, comentario nuevo | ya exige `Causa (4)` y no `(2)` |
| `solver_causaDeAulaCuentaLasFranjasBloqueadasPorDesplazamientos` | **se reescribe** → `solver_causaNoAtribuibleCuandoLaReferenciaCompartidaNoCabe` | INFEASIBLE por la referencia compartida, sin categoría: `no atribuible`, sin entidad |
| `solver_causaDeAulaConCapacidadSuficienteNoInventaElConflicto` | **se conserva** | control negativo: factible |
| `solver_causaDeAulaNoDescuentaDosVecesLaMismaFranja` | **se renombra** → `...ConDosCursosDesplazadosEsFactible` | la unión ya no existe; sigue siendo factible |
| `solver_causaDeAulaCuentaLasFranjasQueNoCabenEnUnSoloLaboratorio` | **se reescribe** → `solver_causaNoAtribuibleCuandoLaReferenciaCasiLlenaNoCabe` | 28 ≤ 30 y 4 ≤ 30: el conteo por tipo pasa |
| `solver_causaDeAulaConLaboratoriosSuficientesNoInventaElConflicto` | **se conserva** | control negativo con 2 laboratorios |
| `solver_causaSaturacionPorTipoDescuentaLasFranjasBloqueadas` | **se reescribe** → `solver_causaNoAtribuibleCuandoElTipoTieneCapacidad` | 7 ≤ 12; **no se toca el fixture** (la referencia de `3|C` sigue en la cancha: los fixtures del motor no pasan por V18, §3.2) y se mantiene la infactibilidad |
| `solver_causaDeAulaNoSobreestimaConTurnosDistintos` | **se conserva**, comentario nuevo | ya exige `Causa (4)` |
| `solver_causaDeAulaConTurnosDistintosNoInventaElConflicto` | **se conserva** | control negativo |
| `solver_causaSaturacionPorTipoNoDescuentaFranjasDeOtroTurno` | **se renombra** → `solver_causaDeDocenteEnTurnoDistintoNoSeInformaComoConflictoDeAula` | ya exige `Causa (4)` |
| `solver_causaSaturacionPorTipoNoDescuentaFranjasConDocenteNoElegible` | **se renombra** → `solver_causaDeDocenteNoElegibleNoSeInformaComoConflictoDeAula` | ya exige `Causa (4)` |
| `solver_causaSaturacionPorTipoConDocenteSuficienteNoInventaElConflicto` | **se conserva** | control negativo |
| `solver_falloDocenteNoElegibleNoSeInformaComoConflictoDeAula` | **se conserva** | ya exige `Causa (4)` |
| `solver_falloCausaConflictoDeAula` (005) | **se reescribe la aserción** | **el fixture no se toca**: se asevera el **tipo declarado** (`«regular»`) en vez del nombre del aula (8 > 4 sigue siendo (2)) |
| `solver_falloCausaConflictoDeAulaPorReferenciaSaturada` (T6) | **se renombra** → `...PorCapacidadDeTipo` y **se reescribe la aserción** | 10 > 6 sigue siendo (2); el mensaje ya no nombra el aula |
| `solver_falloCausaConflictoDeAulaPorSaturacionDeTipo` (T6) | **se conserva**, comentario | 10 > 8 sigue siendo (2) |
| `solver_falloCausaConflictoDeAulaPorSaturacionDeTipoEnModoMovil` (T6) | **se conserva** | 10 > 8 en modo móvil |
| `solver_falloTipoSinAulas_esEntradaInvalida` (T6) | **se conserva** | (1) por V17 |
| `solver_falloCausaConflictoDePersona` (005) | **se conserva** | prueba la **excepción de precedencia** de RF-6 (§0.6): demanda 4 > capacidad 2 y aun así (3) |

### T14. Pruebas nuevas: ventanas, reproducibilidad, aula especializada y renombrados (30 min)

```
cmake --build --preset backend --parallel
ctest --test-dir build-backend --output-on-failure -R 'test_solver_horarios'
```

Esperado: build y verde (el build va en el «Hecho cuando» para que la tarea sea autocontenida, I9).

- **`solver_causaNoAtribuiblePorVentanasDeTurno`** (**fixture completo en §0.3**, verificado): 1 día,
  `num_slots_dia = 7`, **1 aula** `A1` (`Aula de estudio`), **modo móvil**, tres cursos de 1 bloque:
  `1|A` y `2|B` con turno `TB={4}` y `Prof A`/`Prof B` disponibles en `(0,4)`, y `3|C` con turno
  `TC={5,6}` y `Prof C` en `(0,5)` y `(0,6)`. `demanda = 3 = capacidad` (1 × 3) → **(2) no falla**;
  (3) y (4) tampoco; el modelo es **INFEASIBLE** (`1|A` y `2|B` no caben en la única celda `(0,4)`) →
  **no atribuible**, sin `Causa (`, sin `A1`, sin `1|A`.
- **`solver_esReproducible_mismaEntradaMismaSalida`** (RNF-1, M10): resolver **dos veces** el mismo
  `SolverConfig` (uno factible y el fixture INFEASIBLE de T13) y comparar `exito` y un resumen
  determinista de la salida —por curso, la lista de `(día, slot, materia, profesor, aula)` y, si no hay
  solución, `errores`—. Se apoya en que el motor fija `num_search_workers(1)` y `random_seed(0)`
  (`solver.cpp:424-425`).
- **`solver_exclusividadEntreClasesDesplazadas`**: se añade una aserción (M11) de que el laboratorio
  —tipo especializado— aloja clases de **los dos** cursos (`enLaboratorio == 4` sigue), es decir, **no
  queda reservado** a ninguno; así RF-3 («las aulas especializadas nunca quedan reservadas») tiene
  comprobación directa, además de seguirse de `solver_referenciaBloqueadaMientrasHayClaseDesplazada`
  (solo se bloquea la referencia general).
- Se actualizan los **3 renombrados** y los **2 comentarios** de la tabla de T13 (sin cambiar aserciones
  más allá de las indicadas) y se comprueba que ningún test usa datos no versionados.

### T15. Documentación, constitución y 005 (25 min; la ejecuta el planner)

```
grep -n "tipo general" docs/contrato-solver-2.0.md
grep -n "capacidad del tipo\|no atribuible" docs/contrato-solver-2.0.md
grep -n "aula de referencia\|preset" docs/contrato-solver-2.0.md
grep -n "V18" docs/contrato-solver-2.0.md
grep -n "tipo general" docs/constitution.md
grep -n "V1–V18" docs/constitution.md
grep -n "sin atribuirla" docs/constitution.md
grep -n "no atribuible" specs/005-nucleo-solver/spec.md
grep -n "excepción de precedencia\|no cabe en su turno" specs/005-nucleo-solver/spec.md
grep -n "V1–V17" specs/005-nucleo-solver/spec.md docs/contrato-solver-2.0.md
```

Esperado:

- `docs/contrato-solver-2.0.md` explica el **tipo general por convención de nombre** (`aula de
  estudio`), la **capacidad del tipo** (fórmula con días×franjas de la unión de turnos), que (2) se
  decide por ese conteo sin descontar bloqueos, que lo no explicado es **no atribuible** y que **un
  preset 2.0 anterior con una referencia de tipo especializado pasa a ser entrada inválida por V18**,
  sin conversión, con la acción «asignar al curso un aula del tipo general» (M12). Se actualiza
  `:27` («se valida el resto (V1–V17)» → V1–V18) y la fila de `cursos` de la tabla de secciones
  (`:65`) para incluir **V18**: el `grep -n "V18"` del contrato lo muestra en ambas (esa fila no cae en
  el `grep` de `V1–V17` porque hoy no lo menciona). Sin sección nueva; el contrato sigue 2.0.
- Constitución: `tipo general` en §7.1.5/§7.3, **`V1–V18`** en §7.4 y `sin atribuirla` en §7
  «Comportamiento» (texto de §7 del plan).
- `specs/005-nucleo-solver/spec.md`: `no atribuible` en la definición de causa y en RF-6, la
  **excepción de precedencia** de (3) sobre (2), la (2) descrita por capacidad **sin «(invariantes
  D8/D9/D10)»** (`005:317`, I8) y `V1–V18` en **todas** las apariciones que hoy dicen V1–V17 (`128`,
  `135`, `317`, `322`, `575`, `578`, `580`, `822` y notas), actualizando «la 010 añade V16 y V17» a
  «V16, V17 y V18».
- **El último `grep` no devuelve nada**: no queda ninguna mención a `V1–V17` en la 005 ni en el
  contrato vivo (M9).
- **El implementer solo verifica**; no edita estos archivos.

### T16. Regresión global y comprobación del seed (20 min)

```
cmake --build --preset backend --parallel
ctest --preset backend
ctest --test-dir build-backend -N
git diff --stat src/backend/src/database src/backend/include/backend/database src/seed
grep -n 'const char\* const kTipoGenerico' src/seed/src/perfil.cpp
grep -c "kTipoGenerico" src/seed/src/perfil.cpp
grep -n "Aula de estudio" src/seed/src/perfil.cpp
```

Esperado: **100 % verde** (sin `test_frontend_conversion` ni `test_generacion_seed`); el benchmark sigue
**factible** y < 60 s; `git diff --stat` de la base y del seed **vacío** (no hay cambio de esquema ni de
siembra); el seed sigue declarando `const char* const kTipoGenerico = "Aula de estudio";`
(`perfil.cpp:43`), **18** líneas mencionan `kTipoGenerico` (declaración, catálogo, las 14 aulas y los dos
retornos de `tipoFinalDeMateria`) y el curso se asigna por `Aula 01…13` (el aula de referencia no se
siembra en el seed: se elige en la app). Así la convención se cumple **sin tocar el seed** (I7).

## 9. Trazabilidad RF → tarea → verificación

| RF | Tarea(s) | Verificación |
|----|----------|--------------|
| RF-1 | T1, T2 | `test_constructor_entrada` (2.0, tipos, eventos, `inicio/fin`, ida y vuelta; **aula/materia sin tipo → `ok == false` con mensaje que nombra la entidad**), `test_contrato_solver` (forma y sellado 2.0 en T1), `test_cargador_configuracion` (2.0 válido en T2). **Sin cambio el 2026-10-26**: no hay catálogo ni marca en el contrato |
| RF-2 | T2 | `test_cargador_configuracion` (1.0/sin versión, fixture versionado, archivo, versión, acción, bytes intactos) |
| RF-3 | T4, **T12**, **T14** | `test_solver_horarios` (referencia, desplazamiento/vuelta, bloqueo, exclusividad, móvil) + **V18**: `test_validador_entrada` (`v18_*`: referencia de otro tipo → `V18:` con curso/aula/tipo; móvil no aplica; sin tipo general llega por `V17:`) + **T14**: aserción de que el laboratorio aloja clases de los dos cursos (los especializados **no** se reservan) |
| RF-4 | T4, T5 | `test_solver_horarios` (aula real) y `test_validador_salida` (P3 sin cambios y P4 con claves por docente/aula, `06fc70e`) |
| RF-5 | T3, **T12** | `test_validador_entrada` (V1/V2/V16/V17) + **V18** (`v18_*`) en el orden V16 → V17 → V18 |
| RF-6 | **T11, T13, T14** | `test_capacidad_tipo` (unidades día×franja, demanda y capacidad) + `test_solver_horarios`: (2) solo si `demanda(t) > capacidad(t)`; **excepción de precedencia**: `solver_falloCausaConflictoDePersona` da (3) con demanda 4 > capacidad 2; **contraejemplo del reviewer → `Causa (4)`**; ventanas de turno y referencia compartida → `no atribuible`; `grep` sin `palomarPorTipo`/`franjasBloqueadas...`/`causaConflictoDeAula`/`nombreAula`/`claveAula` |
| RF-7 | T7, **T15** | `grep` de la 005: V1 eventos, **V1–V18**, D10, salida, RF-7, 1.0, definición de causa (sin D8/D9/D10), **excepción de precedencia** y `no atribuible`; `grep V1–V17` **vacío** |
| RF-8 | T7, **T15** | `grep` de la constitución: `tipo general` (§7.1.5/§7.3), **`V1–V18`** (§7.4), `sin atribuirla` (§7 Comportamiento); §5 y §7.5 **sin** cambios nuevos |
| RF-9 | T8 | `grep` del MER (catálogo, tipos, turnos, cursos, eventos). **Sin cambio el 2026-10-26**: no hay columna nueva |
| RF-10 | T9, **T15** | `grep` de `docs/contrato-solver-2.0.md` (2.0, eventos, aula de referencia, 1.0, aula real) + `tipo general`, `capacidad del tipo`, `no atribuible`, V1–V18 y el aviso del preset 2.0 con referencia especializada |
| RNF-2 | T10, **T16** | `test_solver_benchmark` factible y < 60 s |
| RNF-3 | **T13, T14** | Mensaje de (2) con nombre visible del tipo, demanda y capacidad; «no atribuible» sin nombrar entidad; V18 con acción |
| RNF-4 | T1, T3, T4, T6, **T16** | Suite completa verde; invariantes y objetivo sin cambios |
| RNF-5 | T10, **T16** | `ctest --preset backend` sin tests de frontend |
| RNF-6 | Todas | Esta tabla y el `Hecho cuando` de cada tarea |
| RNF-1 | **T14**, T16 | `solver_esReproducible_mismaEntradaMismaSalida`: dos ejecuciones del mismo fixture (factible e INFEASIBLE) dan el mismo `exito` y el mismo resumen/`errores`; el motor fija `num_search_workers(1)` y `random_seed(0)` |

**Cobertura de la spec por parte del plan (no hay RF sin verificación).** RF-1/RF-2/RF-4/RF-9 quedan
como están (T1–T9, ya verdes); RF-3 y RF-5 suman **V18** (T12); RF-6 se reescribe (T11/T13/T14); RF-7,
RF-8 y RF-10 suman los textos de T15.

## 10. Riesgos y condiciones

- **Se pierde atribución, no detección (2026-10-26).** Al retirar el conteo por aula de referencia, los
  fallos que solo esa ocupación explica se informan como **no atribuible**. Es aceptado por RF-6 y está
  cubierto por tests que antes exigían (2): si el mensaje «no atribuible» desapareciera o alguien
  reintrodujera un descuento, T13/T14 fallan. No se pierde el estado INFEASIBLE ni la causa cuando es
  (1), (3) o (4).
- **El nombre del tipo general es una convención, no un dato (2026-10-26).** `tipoGeneralAula()` es la
  única fuente; si la semilla cambiara el nombre, V18 rechazaría todas las referencias. Se verifica en
  T16 con `grep -n 'const char\* const kTipoGenerico' src/seed/src/perfil.cpp` (debe decir
  `= "Aula de estudio"`) y `grep -c kTipoGenerico` (18 líneas); el mensaje de V18 nombra la convención.
- **Presets 2.0 anteriores con referencia especializada (M12).** La regla V18 es de entrada: un preset
  2.0 guardado con el aula de referencia sobre un laboratorio, taller o cancha pasa a ser **entrada
  inválida**, **sin conversión automática**. La acción es **asignar al curso un aula del tipo general**
  (`Aula de estudio`) y regenerar la entrada desde los datos. Afecta a presets escritos antes de este
  cambio; se documenta en `docs/contrato-solver-2.0.md` (T15). No hay presets en el repo que lo usen.
- **La causa (2) ya no nombra un aula.** Los mensajes por aula desaparecen con el bloque retirado; los
  tests que los esperaban se reescriben en T13. Si algún consumidor (GUI, logs) buscaba el nombre del
  aula en el mensaje, deja de encontrarlo: es coherente con que el conteo es por tipo (RNF-3 se cumple
  con tipo, demanda y capacidad).
- **Orden (2) antes que (3) con la excepción aprobada.** La **excepción de precedencia** de RF-6
  (2026-10-26) manda informar (3) cuando un curso no cabe en su turno, aunque la capacidad del tipo
  también falle. En producción V14 ya rechaza ese dato antes del solver; la excepción se prueba con
  `solver_falloCausaConflictoDePersona` (demanda 4 > capacidad 2 → (3)) y la implementa el guard de
  `causaSaturacionPorTipo`. Si se retirara el guard, habría que enmendar la spec otra vez.
- **El modelo con `y` para todos los cursos** aumenta las variables (cursos × días × franjas ×
  aulas); el benchmark de T10/T16 debe seguir <60 s. Si se degrada, la alternativa es podar `y` por los
  tipos de las materias del curso, no volver a la lógica de aulas fijas.
- **La referencia bloqueada** es una restricción por par de cursos; con muchos cursos puede crecer
  como O(cursos² × franjas), pero es lineal en franjas y pequeña frente a `x`. **Este cambio no la
  toca**: solo deja de entrar en las cuentas de causa.
- **La 011** cambiará `duracion_minutos` (30–90) y la procedencia de la configuración; aquí no se
  toca; V2 solo mantiene `>0`.
- **La constitución y la 005** las edita el planner; el implementer no. Lo de T7 ya está (`f054b2e`);
  lo de este cambio es **T15**.
- **El frontend** no compila; `full`/`dev-frontend` siguen rojos; la verificación es el preset
  `backend`, como en la 009. **Rotura conocida:** `src/frontend/src/services/entrada_solver_builder.cpp`
  asigna el retorno de `construirJsonEntrada` a un `QJsonObject`; es deuda del equipo del frontend y no
  se toca.
- **`Resultado<QJsonObject>` en los consumidores backend:** ya adaptados en T1; este cambio no añade
  llamadores.
- **El seed no es generable con la regla dura (documentado, no se intenta).** La demanda de ciencias
  ronda los **104 bloques** frente a **1 laboratorio × 40 franjas**. T16 solo verifica que el seed sigue
  sembrando `Aula de estudio` y que no se toca.
- **El bloqueo solo recorre `y` existentes.** Ver §4.2: recorrer `c'` a ciegas rompería con turnos
  mixtos (mañana/tarde), que es justo el caso del seed.

## 11. Fuera de alcance de este cambio (2026-10-26)

- **El esquema y su versión:** no hay columna nueva; `TiposAula` sigue con `nombre` y `clave` y
  `VERSION_ESQUEMA_ACTUAL` no cambia. Una base existente se abre como hoy.
- **La 009:** no se reabre; no se tocan servicios, DTOs, huella ni seed.
- **El contrato:** sigue siendo **2.0**, sin sección ni campo nuevos. Un preset 2.0 **sin referencias
  especializadas** sigue siendo válido; uno con el aula de referencia sobre un tipo especializado pasa a
  ser **entrada inválida por V18**, sin conversión (ver §10, M12).
- **El frontend:** no se toca (sigue fuera de la construcción); su deuda con `Resultado<QJsonObject>`
  es la misma.
- **El seed:** solo se verifica que siembra `Aula de estudio` (T16); no se modifica ni se intenta
  generar un horario con él.
- **El motor:** `x`/`y`, el objetivo (`PESO_HORAS`, `COSTE_SUPLENTE`), la reserva del aula de
  referencia, el tipo exacto y el resto de restricciones no cambian. `FranjaSemana` se usa **solo**
  en las cuentas de capacidad.
- **El resto de RF de la 010:** contrato 2.0, rechazo de versiones, V1/V2/V16/V17, aula de referencia,
  salida con aula real, MER, documentación general del contrato y benchmark ya están hechos y validados
  (T1–T10); este cambio no los reabre.
