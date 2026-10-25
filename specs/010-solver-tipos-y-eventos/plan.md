# Plan 010 - Solver: tipos de aula y eventos del turno

Base: e72c930

> **Alcance.** Implementa la spec 010 (10 RF) sobre el cierre de la 009. La 009 ya dejó el modelo
> persistido: `Eventos` con identidad/orden, `TiposAula` con clave normalizada, `id_tipo_aula`
> obligatorio, `ServicioTiposAula`/`ServicioTurnosEventos`, huella con tipos y eventos y el puente
> `ConstructorEntradaSolver::recesosDesdeEventos` hacia la sección `recesos` del contrato 1.0. La 010
> sustituye ese puente por el **contrato 2.0**, introduce la **regla dura de tipo** con **aula de
> referencia**, las validaciones **V16/V17**, la salida con **aula real**, la supersesión de la 005,
> la enmienda de la constitución, el MER completo y la documentación del contrato. El frontend sigue
> fuera; se verifica con `cmake --build --preset backend --parallel` y `ctest --preset backend`.
>
> **Estado de partida.** La suite de la 009 está en verde (49/49). El solver actual usa
> `aulaAssignment[c][d][s][a]` solo para cursos móviles, la salida de un curso con aula fija informa
> siempre su aula, el parser acepta solo `1.0`/`recesos`, y no existen tipos en el contrato. La 011
> (hora académica) viene después y es la que estrecha V2 a 30–90 y cambia la procedencia de
> `duracion_minutos`: **no se adelanta aquí**.

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
  validación V1–V17 vive en `fromJson`.
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
  clases (la `y`), no la fija por defecto; `causaConflictoDeAula`/`causaEntradaInvalida` amplían la
  saturación por tipo y la falta de tipos.
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
  V17.
- `test/backend/test_solver_horarios.cpp`: referencia, desplazamiento y vuelta, bloqueo del aula de
  referencia, exclusividad de desplazadas, móvil, salida con aula real, saturación de laboratorio.
- `test/backend/test_variables_cp_sat.cpp`: compila sin `RecesoConfig` (T1) y comprueba la `y` de
  todos los cursos (T4). **Ojo con el nombre:** el test se registra en CTest como
  **`test_backend_variables_cp_sat`** (`test/CMakeLists.txt:118`), así que `-R 'test_variables_cp_sat'`
  **no casa**; usar `-R 'variables_cp_sat'` o el nombre completo.
- `test/backend/test_solver_benchmark.cpp`: el escenario grande con tipos (estrategia fija en el
  apartado siguiente).
- `test/backend/test_validador_salida.cpp`: salida con aulas reales; **P3 es solo pruebas**, pero
  **P4 cambia** por la desviación autorizada de §1.2 (claves por docente y por aula, `06fc70e`).
- No se tocan `test/frontend/*` ni `test/CMakeLists.txt:120-133`: siguen fuera con
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

### 2.3 Validaciones (V1, V2, V16, V17)

- **V1 — estructura y versión:** la versión se comprueba **antes** que las secciones. Ausente,
  `1.0` o desconocida → error de versión; se espera `2.0`. Las secciones requeridas son
  `version`, `dimensiones`, `franja_horaria`, `turnos`, `eventos`, `planes`, `cursos`,
  `profesores`, `materias`, `aulas`.
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
- Orden: V1 → V2…V15 → V16 → V17, como en la 005/009.
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
- `causaConflictoDeAula` amplía la capacidad por tipo: para cada tipo, bloques de las materias que lo
  requieren frente a `días × franjas × aulas de ese tipo`; la referencia bloqueada no se cuenta como
  disponible.
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

- V16/V17 → entrada inválida (antes de resolver).
- Saturación por tipo o por referencia bloqueada sin solución → **conflicto de aula**.
- El solver no libera la reserva ni reasigna a otro tipo para hacer factible.

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
9. **Causa de saturación por tipo en `causaConflictoDeAula`.** Alternativa descartada: confiar solo
   en el estado INFEASIBLE sin categoría; RF-6 exige la causa.
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

## 6. Supersesión enumerada de la 005

La 010 supersede: la definición «Curso con aula fija: usa siempre su aula»; **D10**; el **criterio 2**
(«D1–D11 y D13») **salvo D10**; la salida «con aula fija, su aula fija»; **RF-7** de la 005; la
versión **1.0** (definición, RF-10 y criterio); y **V1** (sección de recesos). **D9 se amplía** (un
aula por clase, del tipo requerido), **D8 se extiende** a las desplazadas y **V8 se mantiene**. El
resto de la 005 sigue vigente. La anotación en el texto de la 005 la hace el planner (T7).

## 7. Enmienda de la constitución (la ejecuta el planner)

Texto exacto (sustitución literal; las líneas son las de `docs/constitution.md` en `e72c930`):

- **§5, línea 34 — protección de datos.** Sustituir la línea completa por:
  > - Los datos del usuario en SQLite (planes de estudio, docentes, aulas, tipos de aula, asignaturas,
  >   cursos, turnos, eventos y configuración del solver) nunca se borran ni se sobrescriben sin
  >   confirmación explícita.

  (Añade «tipos de aula»; sustituye «recesos» por «eventos».)
- **§7.1.5, línea 64 — invariante del aula.** Sustituir la línea completa por:
  > 5. **Aula.** Toda clase se imparte en un aula cuyo **tipo** coincide con el tipo de aula requerido
  >    por su materia. Con el modo de aula fija **inactivo**, el solver elige un aula del tipo
  >    requerido en cada franja en que el curso tiene clase; con el modo **activo**, cada curso declara
  >    un **aula de referencia**: la usa cuando su tipo coincide, y las clases cuya materia requiere
  >    otro tipo se imparten en un aula de ese tipo y el curso **vuelve** a la referencia en las demás.
  >    Mientras un curso tiene una clase desplazada en una franja, su aula de referencia queda **vacía
  >    y bloqueada** en esa franja. La **exclusividad de aula** rige también las clases desplazadas.
- **§7.3, líneas 75–84 — contrato de configuración.** Tres sustituciones y una viñeta nueva:
  1. En **Cursos**, sustituir «activo → cada curso usa su aula fija; inactivo → aulas móviles que
     decide el solver» por «activo → el aula del curso es su **aula de referencia** (§7.1.5:
     desplazamiento y vuelta); inactivo → el solver elige el aula de cada clase».
  2. En **Aulas**, añadir tras «no por nombre; la interfaz y el CSV traducen índice→nombre»:
     «Cada aula declara su **tipo** y cada materia el **tipo de aula requerido**; la igualdad es
     exacta sobre la **clave normalizada** (sin espacios extremos y sin distinguir mayúsculas),
     aunque el nombre visible del tipo se conserva.»
  3. Viñeta nueva después de **Aulas**:
     «- **Eventos:** la entrada lleva los eventos de cada turno (nombre, posición, orden, duración y
     horario opcional), que el solver ignora; sustituyen a la sección de recesos. La **identidad**
     del evento no viaja en el contrato.»
  4. Al final de la sección: «La versión del contrato es **2.0**; una configuración `1.0` o sin
     versión se rechaza.»
- **§7.4, línea 87 — validación previa.** Solo dos cosas: (1) sustituir el rango «V1–V15» por
  «**V1–V17**»; (2) añadir al final del párrafo: «**V1** espera la sección de **eventos** (no la de
  recesos); el contenido de V1, V2, V16 y V17 vive en la spec 005 y en la 010.» Se mantienen íntegras
  las frases de V13, V7, V10/V11, V14 y V15. **No se duplica** el detalle de V2/V16/V17: la constitución
  fija el rango y la remisión (RF-8), y el detalle normativo es de la 005, que la 010 anota en T7.
- **§7.5, línea 91 — fuera de alcance.** Sustituir «El receso es metadata que el solver ignora» por
  «El **evento** de turno es metadata que el solver ignora». El resto de la línea no cambia.
- **005:** anotar en `specs/005-nucleo-solver/spec.md` que V1 pasa a eventos, el catálogo a V1–V17 y
  la supersesión de D10/definición/salida/RF-7/versión 1.0. El detalle de V1/V16/V17 vive en la 005
  y la 010; la constitución solo fija el rango y la remisión.

## 8. Verificación por tarea

Comandos desde la raíz. Compilación siempre: `cmake --build --preset backend --parallel`.

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

## 9. Trazabilidad RF → tarea → verificación

| RF | Tarea(s) | Verificación |
|----|----------|--------------|
| RF-1 | T1, T2 | `test_constructor_entrada` (2.0, tipos, eventos, `inicio/fin`, ida y vuelta; **aula/materia sin tipo → `ok == false` con mensaje que nombra la entidad**), `test_contrato_solver` (forma y sellado 2.0 en T1), `test_cargador_configuracion` (2.0 válido en T2) |
| RF-2 | T2 | `test_cargador_configuracion` (1.0/sin versión, fixture versionado, archivo, versión, acción, bytes intactos) |
| RF-3 | T4 | `test_solver_horarios` (referencia, desplazamiento/vuelta, bloqueo, exclusividad, móvil) |
| RF-4 | T4, T5 | `test_solver_horarios` (aula real) y `test_validador_salida` (P3 sin cambios y P4 con claves por docente/aula, `06fc70e`) |
| RF-5 | T3 | `test_validador_entrada` (V1/V2/V16/V17) |
| RF-6 | T6 | `test_solver_horarios` (saturación → conflicto de aula; tipo sin aulas → entrada inválida) |
| RF-7 | T7 | `grep` de la 005 (V1 eventos/V1–V17/D10/salida/RF-7/1.0) |
| RF-8 | T7 | `grep` de la constitución (§5, §7.1.5, §7.3, §7.4, §7.5) |
| RF-9 | T8 | `grep` del MER (catálogo, tipos, turnos, cursos, eventos) |
| RF-10 | T9 | `grep` de `docs/contrato-solver-2.0.md` (2.0, eventos, aula de referencia, 1.0, aula real) |
| RNF-2 | T10 | `test_solver_benchmark` factible y < 60 s |
| RNF-5 | T10 | `ctest --preset backend` sin tests de frontend |
| RNF-6 | Todas | Esta tabla y el `Hecho cuando` de cada tarea |
| RNF-1/3/4 | T1, T3, T4, T6 | Reproducibilidad del solver, mensajes y no regresión fuera de la regla |

## 10. Riesgos y condiciones

- **El modelo con `y` para todos los cursos** aumenta las variables (cursos × días × franjas ×
  aulas); el benchmark de T10 debe seguir <60 s. Si se degrada, la alternativa es podar `y` por los
  tipos de las materias del curso, no volver a la lógica de aulas fijas.
- **La referencia bloqueada** es una restricción por par de cursos; con muchos cursos puede crecer
  como O(cursos² × franjas), pero es lineal en franjas y pequeña frente a `x`.
- **La 011** cambiará `duracion_minutos` (30–90) y la procedencia de la configuración; aquí no se
  toca; V2 solo mantiene `>0`.
- **La constitución y la 005** las edita el planner; el implementer no. **Ya aplicadas** en
  `f054b2e` (§5, §7.1.5, §7.3, §7.4 y §7.5 más la anotación de la 005), así que T7 no bloquea el
  cierre.
- **El frontend** no compila; `full`/`dev-frontend` siguen rojos; la verificación es el preset
  `backend`, como en la 009.
- **Rotura conocida del builder del frontend.** `src/frontend/src/services/entrada_solver_builder.cpp`
  asigna el retorno de `construirJsonEntrada` a un `QJsonObject`; con `Resultado<QJsonObject>` deja de
  compilar. Es deuda del equipo del frontend y **no se toca** en esta spec (el preset `backend` no lo
  compila). El día que el frontend vuelva, basta con `if (!r.ok) …; comp.entrada = r.valor;`.
- **`Resultado<QJsonObject>` en los consumidores backend.** Además de la definición y la fachada, hay
  cuatro llamadores en pruebas: `test_constructor_entrada`, `test_validador_entrada`,
  `test_servicio_generacion` y `test_ipc_aislamiento_bd`. T1 los adapta todos; si se olvida uno, el
  preset `backend` no compila y la tarea no se marca.
- **El seed no es generable con la regla dura (documentado, no se intenta).** La demanda de ciencias
  ronda los **104 bloques** frente a **1 laboratorio × 40 franjas**: no hay horario con la regla de
  tipo para el centro sembrado. La 009 ya retiró la promesa de generabilidad del seed; la 010 lo deja
  escrito en la guía de datos de prueba y **no** lo verifica con el solver (`test_generacion_seed`
  sigue fuera de alcance).
- **El bloqueo solo recorre `y` existentes.** Ver §4.2: recorrer `c'` a ciegas rompería con turnos
  mixtos (mañana/tarde), que es justo el caso del seed.
