# Informe Tecnico — Motor Solver CP-SAT

## 1. Vision General

El motor solver resuelve el problema de **asignacion de horarios academicos**: dado un conjunto de cursos, profesores, materias y aulas, encuentra una asignacion factible que cumpla todas las restricciones duras.

**Stack**: C++17, Qt6 (contenedores), OR-Tools CP-SAT (solver de constraint programming), QTest (tests).

**Enfoque**: Constraint Programming (CP), no Programacion Lineal. CP-SAT modela el problema como variables discretas con restricciones y busca asignaciones que las satisfagan simultaneamente.

---

## 2. Arquitectura del Pipeline

```
JSON config → SolverConfig (V1-V13) → VariablesSolver → Restricciones → CP-SAT Solve → HorarioSalida
```

### Flujo completo en `Solver::resolver()`:

```
1. crearVariables(model, config)    →  VariablesSolver (BoolVars)
2. agregarRestricciones(config)     →  8 restricciones al model
3. SolveCpModel(model.Build())     →  CpSolverResponse
4. Si OPTIMAL/FEASIBLE:
      extraerSolucion(response)     →  HorarioSalida
   Si no:
      analizarFallo(response)       →  QStringList errores
```

---

## 3. Estructuras de Configuracion (solver_config.hpp)

### 3.1 SolverConfig — Raiz

```cpp
struct SolverConfig {
    QString version;
    DimensionesConfig dimensiones;      // num_profesores, num_materias, num_aulas, num_cursos, num_dias, num_slots_dia
    FranjaHorariaConfig franja_horaria; // duracion_minutos, slots_por_turno
    TurnosConfig turnos;                // QVector<TurnoConfig> con nombre + slots
    QVector<RecesoConfig> recesos;
    QVector<CursoSolverConfig> cursos;
    QVector<ProfesorSolverConfig> profesores;
    QVector<MateriaSolverConfig> materias;
    QVector<AulaSolverConfig> aulas;
    PlanificacionConfig planificacion;
    GeneracionConfig generacion;
    PenalizacionConfig penalizaciones;
};
```

### 3.2 CursoSolverConfig

```cpp
struct CursoSolverConfig {
    QString nombre;          // "1ro A"
    QString turno;           // "manana" o "tarde"
    int aula_fija = -1;     // -1 = aula movil, indice = aula fija
    int num_estudiantes;
    QString plan;            // plan de estudios
    QVector<MateriaCurso> materias;  // {materiaIDx, horasSemanales}
};
```

### 3.3 ProfesorSolverConfig

```cpp
struct ProfesorSolverConfig {
    QString nombre;
    int horas_requeridas;              // horas totales del contrato
    int horas_aula;                    // horas reales en aula (las que el solver asigna)
    QString turno;                     // "" = any, "manana"/"tarde" = filtrado
    QString plan;                      // "" = any, "Plan X" = filtrado
    QVector<int> materias_asignadas;   // indices de materias titulares
    QVector<MateriaSuplente> materias_suplente;  // {materiaIDx, peso}
    QVector<Disponibilidad> disponibilidad;      // [{dia, [slots]}]
};
```

### 3.4 DimensionesConfig

```cpp
struct DimensionesConfig {
    int num_profesores;   // N
    int num_materias;     // M
    int num_aulas;        // A
    int num_cursos;       // C
    int num_dias;         // D (tipicamente 5)
    int num_slots_dia;    // S (tipicamente 4-6)
};
```

**Clave**: `dimensiones` define los limites de los loops. Los arrays (`cursos`, `profesores`, etc.) deben tener exactamente el tamano indicado. Esto se valida en V3.

---

## 4. Variables de Decision (variables.hpp/cpp)

### 4.1 Estructura VariablesSolver

```cpp
struct VariablesSolver {
    QMap<std::tuple<int,int,int,int,int>, BoolVar> assignment;   // [profesor][materia][curso][dia][slot]
    QMap<std::tuple<int,int,int,int>, BoolVar> aulaAssignment;   // [curso][dia][slot][aula]
};
```

### 4.2 Que es una BoolVar

Una `BoolVar` es una variable binaria de OR-Tools: vale **0** (falso) o **1** (verdadero). El solver decide automaticamente el valor de cada variable.

- `assignment[{p,m,c,d,s}] = 1` significa: "el profesor `p` ensena la materia `m` al curso `c` en el dia `d`, slot `s`"
- `aulaAssignment[{c,d,s,a}] = 1` significa: "el curso `c` usa el aula `a` en el dia `d`, slot `s`"

### 4.3 Funcion crearVariables()

**Archivo**: `variables.cpp` (65 lineas)

**Algoritmo**:

```
Para cada curso c:
  Para cada materia m del curso:
    Para cada profesor p:
      Si p es titular de m O suplente de m:
        Para cada dia d:
          Para cada slot s en turno del curso:
            Si p esta disponible en (d,s):
              Crear BoolVar assignment[p,m,c,d,s]

Para cada curso c con aula movil (aula_fija == -1):
  Para cada dia d:
    Para cada slot s en turno del curso:
      Para cada aula a:
        Crear BoolVar aulaAssignment[c,d,s,a]
```

**Filtrado**: Solo se crean variables donde hay compatibilidad real. Esto reduce dramaticamente el espacio de busqueda.

**Patron de iteracion** (QMap con std::tuple como key):

```cpp
for (auto it = vars.assignment.constBegin(); it != vars.assignment.constEnd(); ++it) {
    auto [p, m, c, d, s] = it.key();    // C++17 structured binding
    BoolVar var = it.value();             // la variable BoolVar
}
```

**NOTA**: `QMap::constBegin()/constEnd()` son read-only. Para modificar, usar `begin()/end()`.

---

## 5. Restricciones (8 implementadas)

### 5.1 Metodos CP-SAT Disponibles

| Metodo | Significado | Parametros |
|--------|-------------|------------|
| `model.AddAtMostOne(vars)` | A lo sumo 1 variable en 1 | `std::vector<BoolVar>` |
| `model.AddExactlyOne(vars)` | Exactamente 1 variable en 1 | `std::vector<BoolVar>` |
| `model.AddAtLeastOne(vars)` | Al menos 1 variable en 1 | `std::vector<BoolVar>` |
| `model.AddEquality(expr, val)` | Expresion lineal == valor | `LinearExpr, int` |
| `model.AddLessOrEqual(expr, val)` | Expresion lineal <= valor | `LinearExpr, int` |

**IMPORTANTE**: `LinearExpr::Sum(vars)` espera `absl::Span<const BoolVar>`, NO `QVector`. Usar `std::vector<BoolVar>`.

### 5.2 Restricciones de Turnos (restricciones_turnos.cpp, 92 lineas)

#### R1: restriccionUnoPorTurno

**Que hace**: En cualquier (curso, dia, slot), a lo sumo 1 profesor ensena.

**Logica**: Para cada (c, d, s), recoge todas las BoolVar de `assignment` donde `curso==c && dia==d && slot==s`. Si hay >1, aplica `AddAtMostOne`.

**Por que importa**: Sin esto, el solver podria asignar 2 profesores al mismo curso al mismo tiempo.

```cpp
std::vector<BoolVar> varsEnSlots;
for (auto it = vars.assignment.constBegin(); ...) {
    auto [p, m, curso, dia, slot] = it.key();
    if (curso == c && dia == d && slot == s)
        varsEnSlots.push_back(it.value());
}
if (varsEnSlots.size() > 1)
    model.AddAtMostOne(varsEnSlots);
```

#### R2: restriccionBloquesRequeridos

**Que hace**: Cada materia en cada curso recibe exactamente N bloques (horas / duracion_slot, redondeo ceiling).

**Logica**: Para cada (curso, materia), recoge todas las BoolVar donde `curso==c && materia==m`. Aplica `AddEquality(Sum(vars), bloques)`.

**Formula**: `bloques = ceil(horasSemanales * 60 / duracion_slot)`

```cpp
int bloques = (totalMinutos + duracionSlot - 1) / duracionSlot;  // ceiling division
model.AddEquality(LinearExpr::Sum(varsMaterias), bloques);
```

**Por que AddEquality**: La materia DEBE tener exactamente esas horas. Ni mas ni menos.

#### R3: restriccionHorasSemanalesProfesor

**Que hace**: Cada profesor enseña exactamente `ceil(horas_aula * 60 / duracion_slot)` bloques en total (todas las materias juntas).

**Logica**: Para cada profesor `p`, recoge todas las BoolVar donde `profesor==p`. Aplica `AddEquality(Sum(vars), bloques)`.

**Relacion con R2**: R2 controla horas por materia-curso. R3 controla horas totales por profesor. Ambas deben ser consistentes (si un curso pide 4h de Matematica y el profesor tiene 4h de aula, el profesor enseñara solo esa materia).

### 5.3 Restricciones de Aulas (restricciones_aulas.cpp, 66 lineas)

#### R4: restriccionAulaNoSolapada

**Que hace**: En cualquier (aula, dia, slot), a lo sumo 1 curso usa esa aula.

**Logica**: Para cada (a, d, s), recoge BoolVar de `aulaAssignment` donde `aula==a && dia==d && slot==s`. Aplica `AddAtMostOne`.

**Solo aplica a aulas moviles**: Los cursos con `aula_fija` no crean variables en `aulaAssignment`, asi que esta restriccion no los afecta.

#### R5: restriccionMateriaUnicaPorAula

**Que hace**: En cualquier (aula, dia, slot), no pueden haber 2 cursos de la MATERIA misma usando la misma aula.

**Logica**: Para cada (a, d, s), agrupa las BoolVar por materia (usando `config.cursos[curso].materias`). Para cada materia con >1 variable, aplica `AddAtMostOne`.

**Diferencia con R4**: R4 dice "1 curso por aula". R5 dice "2 cursos de la MISMA materia no pueden estar en la misma aula al mismo tiempo". Esto evita que, por ejemplo, "Matematica 1ro A" y "Matematica 2do A" esten en la misma aula simultaneamente (conflicto de laboratorio/examenes).

### 5.4 Restricciones de Profesores (restricciones_profesores.cpp, 84 lineas)

#### R6: restriccionDocenteNoSolapado

**Que hace**: En cualquier (profesor, dia, slot), a lo sumo 1 clase.

**Logica**: Para cada (p, d, s), recoge BoolVar donde `profesor==p && dia==d && slot==s`. Aplica `AddAtMostOne`.

**Similar a R1 pero por profesor**: R1 controla que el curso no tenga 2 profesores. R6 controla que el profesor no tenga 2 cursos.

#### R7: restriccionProfesorPorTurno

**Que hace**: Si un profesor tiene turno "manana", todas sus BoolVar en slots de "tarde" se fuerzan a 0.

**Logica**: Para cada profesor con turno definido, obtiene los slots validos de ese turno. Para cada BoolVar del profesor, si el slot NO esta en los slots validos: `AddEquality(var, 0)`.

```cpp
if (profesor == p && !slotsValidos.contains(slot)) {
    model.AddEquality(it.value(), 0);  // forzar a 0
}
```

**Patron "fijar variable"**: `AddEquality(var, 0)` es la forma de decirle al solver "esta variable siempre vale 0". Equivale a eliminar esa posibilidad.

#### R8: restriccionProfesorPorPlan

**Que hace**: Si un profesor tiene plan "Ciencias" y un curso tiene plan "General", el profesor no puede ensenar en ese curso.

**Logica**: Para cada BoolVar donde `profesor==p && config.cursos[curso].plan != prof.plan`: `AddEquality(var, 0)`.

**Mismo patron que R7**: Fijar variables a 0 para invalidar combinaciones.

---

## 6. Pipeline de Resolucion (solver.cpp, 143 lineas)

### 6.1 Solver::resolver()

```cpp
ResultadoSolver Solver::resolver(const SolverConfig& config) {
    QElapsedTimer timer;
    timer.start();

    ResultadoSolver resultado;
    resultado.exito = false;

    // 1. Crear variables
    vars = crearVariables(model, config);

    // 2. Agregar restricciones
    agregarRestricciones(config);

    // 3. Construir y resolver
    Model sat_model;
    const CpSolverResponse response = SolveCpModel(model.Build(), &sat_model);

    // 4. Evaluar resultado
    if (response.status() == OPTIMAL || response.status() == FEASIBLE) {
        resultado.exito = true;
        resultado.resultado = extraerSolucion(response, config);
    } else {
        resultado.errores = analizarFallo(response, config);
    }

    resultado.tiempo_ms = timer.elapsed();
    return resultado;
}
```

**Miembros de clase**: `vars` (VariablesSolver) y `model` (CpModelBuilder) son miembros privados. Esto permite que las funciones de restricciones los reciban por referencia y modifiquen el modelo directamente.

### 6.2 Solver::agregarRestricciones()

```cpp
void Solver::agregarRestricciones(const SolverConfig& config) {
    agregarRestriccionesTurnos(model, config, vars);
    agregarRestriccionesAulas(model, config, vars);
    agregarRestriccionProfesores(model, config, vars);
}
```

**Orden importa**: Las restricciones se agregan secuencialmente. CP-SAT las evalua todas juntas al hacer `model.Build()`, pero el orden afecta la eficiencia del solver.

### 6.3 Solver::extraerSolucion()

**Archivo**: `solver.cpp` lineas 50-103

**Que hace**: Convierte la solucion CP-SAT (valores de BoolVar) en la estructura `HorarioSalida`.

**Algoritmo**:

```
1. Iterar todas las BoolVar de assignment
2. Para cada una con SolutionBooleanValue == true:
   a. Extraer [p, m, c, d, s]
   b. Resolver aula: si fija → usar indice; si movil → buscar en aulaAssignment
   c. Crear AsignacionOutput{s, m, p, aula}
   d. Agrupar en porCursoDia[c][d]
3. Convertir porCursoDia a HorarioSalida
4. Agregar metadata
```

**API de extraccion de valores**:

```cpp
// Para BoolVar (binaria)
SolutionBooleanValue(response, var) → bool

// Para LinearExpr (entero)
SolutionIntegerValue(response, expr) → int64_t
```

**NOTA**: La funcion es `SolutionBooleanValue`, NO `Value`. Qt's `QJsonPrivate::Value` sombrea el nombre `Value` en el scope.

### 6.4 Solver::analizarFallo()

**Archivo**: `solver.cpp` lineas 105-143

**Que hace**: Genera mensajes de error descriptivos segun el status del solver.

| Status | Mensaje |
|--------|---------|
| INFEASIBLE | Lista causas probables + estadisticas del config + conteo de variables/conflictos |
| MODEL_INVALID | Sugiere revisar formacion de restricciones |
| UNKNOWN | Muestra wall_time para diagnostico de timeout |
| Otro | Nombre del status via `CpSolverStatus_Name()` |

** Campos utiles de CpSolverResponse**:

```cpp
response.num_booleans()    // cantidad de BoolVars creadas
response.num_integers()    // cantidad de enteros auxiliares
response.num_conflicts()   // conflictos encontrados durante busqueda
response.wall_time()       // tiempo real de resolucion (segundos, double)
```

**NOTA**: NO existe `num_variables()`. Usar `num_booleans()`.

---

## 7. Validaciones Pre-Solver (V1-V13)

Se ejecutan en `SolverConfig::fromJson()` antes de llegar al solver.

| V# | Que valida | Funcion |
|----|-----------|---------|
| V1 | JSON parseable + secciones requeridas | `fromJson()` |
| V2 | Tipos correctos | `fromJson()` |
| V3 | dimensiones = longitudes de arrays | `validarDimensiones()` |
| V4 | Indices de materias en rango | `validarIndicesMaterias()` |
| V5 | Indices de profesores en rango | (parte de V4) |
| V6 | Indices de aulas en rango | (parte de V4) |
| V7 | turno en {"manana","tarde"} | `validarTurnosCursos()` |
| V8 | aula_fija = -1 o indice valido | `validarAulasCursos()` |
| V9 | num_estudiantes > 0 | `validarCursosNoVacios()` |
| V10 | materias y profesores no vacios | `validarCursosNoVacios()` + `validarProfesoresNoVacios()` |
| V11 | Un solo plan por curso | `validarUnSoloPlanPorCurso()` |
| V12 | Disponibilidad en rango de slots | `validarDisponibilidadProfesores()` |
| V13 | Cada (materia, curso) tiene >=1 profesor | `validarCoberturaMaterias()` |

**Retorno**: `Resultado<SolverConfig>` — si alguna validacion falla, se retorna `Resultado::error(mensaje)` y el solver nunca se ejecuta.

---

## 8. Estructuras de Salida (horario_salida.hpp)

```cpp
struct AsignacionOutput {
    int slot;       // indice del slot (0-5)
    int materia;    // indice de la materia
    int profesor;   // indice del profesor
    int aula;       // indice del aula
};

struct DiaOutput {
    int dia;                          // indice del dia (0-4)
    QVector<AsignacionOutput> asignaciones;
};

struct CursoOutput {
    QString turno;                    // "manana" o "tarde"
    QVector<DiaOutput> dias;
};

struct HorarioSalida {
    MetadataSalida metadata;
    QMap<QString, CursoOutput> horarios;  // key = nombre del curso
};

struct MetadataSalida {
    int total_asignaciones;
    int cursos_generados;
    // ... otros campos
};
```

---

## 9. Tipo Resultado<T> (resultado.hpp)

```cpp
template<typename T>
struct Resultado {
    bool ok;
    T valor;
    QString mensajeError;
    int codigoError;

    static Resultado<T> exito(const T& val);
    static Resultado<T> error(const QString& msg, int codigo = -1);
};
```

**Patron**: Todas las operaciones que pueden fallar retornan `Resultado<T>`. El caller verifica `ok` antes de usar `valor`. Usado en `SolverConfig::fromJson()`.

---

## 10. Sintaxis y Patrones Clave de OR-Tools CP-SAT

### 10.1 Crear variables

```cpp
BoolVar var = model.NewBoolVar();  // variable binaria
```

### 10.2 Restricciones con vectores

```cpp
std::vector<BoolVar> vars = {var1, var2, var3};
model.AddAtMostOne(vars);         // sum <= 1
model.AddExactlyOne(vars);        // sum == 1
model.AddAtLeastOne(vars);        // sum >= 1
```

### 10.3 Restricciones con expresiones lineales

```cpp
model.AddEquality(LinearExpr::Sum(vars), N);   // sum == N
model.AddLessOrEqual(LinearExpr::Sum(vars), N); // sum <= N
```

### 10.4 Fijar una variable a un valor

```cpp
model.AddEquality(var, 0);  // forzar a 0 (eliminar opcion)
model.AddEquality(var, 1);  // forzar a 1 (obligar opcion)
```

### 10.5 Iterar QMap con tuple keys

```cpp
for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
    auto [a, b, c, d, e] = it.key();   // C++17 structured binding
    BoolVar var = it.value();
}
```

### 10.6 Extraer valores de la solucion

```cpp
bool valor = SolutionBooleanValue(response, var);        // para BoolVar
int64_t valor = SolutionIntegerValue(response, expr);    // para LinearExpr
```

---

## 11. Test Suite

### 11.1 Tests Unitarios (test_solver_horarios.cpp, 10 tests)

| Test | Milestone | Que verifica |
|------|-----------|-------------|
| `configValida_noTieneErrores` | 1 | Struct creation sanity |
| `solver_encuentraSolucion` | 2 | Solver finds feasible solution |
| `solver_respetaTurnoManana` | 3 | All assignments in slots 0-3 |
| `solver_docenteNoSolapado` | 4 | No prof teaches 2 classes at same (dia, slot) |
| `solver_aulasNoSolapadas` | 4 | No aula used by 2 classes at same (dia, slot) |
| `solver_infeasible_retornaError` | 6 | Impossible config returns INFEASIBLE error |
| `solver_xorUnDocentePorMateria` | 7 | Exactly 1 prof teaches materia in curso |
| `solver_distribucionHorasMultiplesDias` | 8 | 4h forced across >=2 days (limited availability) |
| `solver_suplenteReemplaza` | 9 | Suplente fills when titular has plan mismatch |

### 11.2 Benchmark (test_solver_benchmark.cpp)

| Test | Que verifica |
|------|-------------|
| `benchmark_solverGrandeResuelve` | 10 prof, 15 materias, 5 aulas, 8 cursos resuelve en <60s |
| `benchmark_verificaConsistencia` | Misma config: no solapamiento de profesores ni aulas |

**Configuracion grande**: 10 professores, 15 materias, 5 aulas, 8 cursos (4 manana, 4 tarde), 5 dias, 6 slots/dia. Cada curso tiene 3-4 materias. Cada profesor tiene 10-16 horas_aula.

---

## 12. Archivos del Motor Solver

| Archivo | Lineas | Funcion |
|---------|--------|---------|
| `solver_config.hpp` | 196 | Structs de configuracion + fromJson() |
| `solver_config.cpp` | ~550 | Parser JSON + validaciones V1-V13 |
| `variables.hpp` | 50 | Struct VariablesSolver + declaracion crearVariables() |
| `variables.cpp` | 65 | Implementacion crearVariables() |
| `solver.hpp` | 32 | Clase Solver + ResultadoSolver |
| `solver.cpp` | 143 | resolver(), extraerSolucion(), analizarFallo() |
| `restricciones_turnos.hpp` | 31 | Declaraciones R1, R2, R3 |
| `restricciones_turnos.cpp` | 92 | Implementacion R1, R2, R3 |
| `restricciones_aulas.hpp` | 25 | Declaraciones R4, R5 |
| `restricciones_aulas.cpp` | 66 | Implementacion R4, R5 |
| `restricciones_profesores.hpp` | 31 | Declaraciones R6, R7, R8 |
| `restricciones_profesores.cpp` | 84 | Implementacion R6, R7, R8 |
| `horario_salida.hpp` | 68 | Structs de salida |
| `resultado.hpp` | 38 | Template Resultado<T> |

**Total**: ~1,440 lineas de implementacion del motor.

---

## 13. Gotchas y Aprendizajes

1. **`SolutionBooleanValue` NO `Value`**: Qt sombrea `Value` con `QJsonPrivate::Value`. Usar siempre `SolutionBooleanValue`.

2. **`LinearExpr::Sum` espera `absl::Span<const BoolVar>`**: NO funciona con `QVector`. Usar `std::vector<BoolVar>`.

3. **`CpModelBuilder` NO tiene `Add()`**: Usar metodos especificos (`AddAtMostOne`, `AddEquality`, etc.).

4. **`num_booleans()` NO `num_variables()`**: El campo no existe en `CpSolverResponse`.

5. **`horas_aula` debe ser consistente con las restricciones**: Si `restriccionHorasSemanalesProfesor` pide `sum(vars) == bloques` pero la suma de horas de materias no coincide, el solver es INFEASIBLE.

6. **`dimensiones` debe coincidir con los arrays**: Si `num_profesores = 2` pero `profesores.size() = 1`, hay SIGSEGV en los loops.

7. **`constBegin()/constEnd()` para QMap**: Solo lectura. Para modificacion, usar `begin()/end()`.

8. **C++17 structured binding**: `auto [p, m, c, d, s] = it.key()` funciona con `std::tuple` en QMap.

9. **Ceiling division**: `(a + b - 1) / b` en vez de `a / b` para redondeo hacia arriba.

10. **`[[maybe_unused]]`**: Necesario en todas las restricciones porque los parametros `model`, `config`, `vars` pueden no usarse en constraints simples.
