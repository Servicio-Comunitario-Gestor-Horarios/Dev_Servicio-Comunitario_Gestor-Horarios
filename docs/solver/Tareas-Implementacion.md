# Tareas de Implementacion — Solver CP-SAT (Issues #25 + #32)

> **Rol del agente**: Explica sintesis, da ejemplos, responde dudas.
> **Rol del usuario**: Implementa el codigo.

---

## Contexto General

El motor CP-SAT recibe un JSON de configuracion (13 secciones) y produce un JSON de salida con asignaciones agrupadas por curso y dia.

### Arquitectura de Variables

```
BoolVar 5D: assignment[profesor][materia][curso][dia][slot]
  - Solo se crean donde prof enseña materia Y curso la necesita (pruning)

BoolVarAula 4D: aula_assignment[curso][dia][slot][aula]
  - Solo para cursos moviles (aula_fija == -1)
```

### Modalidades de Aula

| Modalidad | `aula_fija` | Comportamiento |
|-----------|-------------|----------------|
| Fija | Indice | Aula fija, no variable |
| Solver asigna | Omitido | DEFERIDO (discriminante TBD) |
| Movil | `-1` | BoolVarAula 4D |

### Restricciones Duras (Sprint 4+5)

1. Docente no solapado (NoOverlap per professor)
2. Horas semanales cumplidas (Sum = required)
3. Aula no solapada (NoOverlap per classroom)
4. Capacidad aula (hardcoded)
5. Materia unica por aula/slot (deriva de NoOverlap)
6. Turno del curso (manual/automatico)
7. Asignacion slot por curso (cada curso recibe N slots)
8. **Un docente por materia-curso** (sum_p <= 1)
9. **Suplente reemplaza completo** (corolario de #8)

### Arquitectura Modular del Solver

```
solver/
├── config/
│   ├── solver_config.hpp    ← Structs + fromJson() (12 validaciones)
│   └── solver_config.cpp    ← Parser JSON completo
├── variables.hpp            ← VariablesSolver struct (assignment 5D + aulaAssignment 4D)
├── variables.cpp            ← crearVariables() con pruning (65 lineas)
├── solver.hpp               ← Solver class (ResultadoSolver, resolver(), etc.)
├── solver.cpp               ← Orquestador (resolver, agregarRestricciones, extraerSolucion)
├── restricciones_aulas.cpp  ← NoOverlap aulas + capacidad + movil
├── restricciones_turnos.cpp ← Fijar slots + sum = bloques
└── restricciones_profesores.cpp ← Un-profesor-por-MC + horas (Sprint 5)
```

**Separacion modular**: `variables.cpp` es un archivo independiente de `solver.cpp`. La creacion de variablesBoolVar esta desacoplada del orquestador de resolucion.

### Archivos a Crear

| Archivo | Descripcion | Estado |
|---------|-------------|--------|
| `src/backend/include/backend/solver/config/solver_config.hpp` | Structs de configuracion | ✅ Completado |
| `src/backend/src/solver/config/solver_config.cpp` | Parser JSON (538 lineas, 12 validaciones) | ✅ Completado |
| `src/backend/include/backend/solver/variables.hpp` | VariablesSolver struct | ✅ Completado |
| `src/backend/src/solver/variables.cpp` | crearVariables() con pruning | ✅ Completado |
| `src/backend/include/backend/solver/solver.hpp` | Solver class + ResultadoSolver | ✅ Completado |
| `src/backend/src/solver/solver.cpp` | Orquestador (resolver, agregarRestricciones, extraerSolucion) | 🔲 Pendiente |
| `src/backend/src/solver/restricciones_aulas.cpp` | NoOverlap aulas + capacidad + movil | 🔲 Pendiente |
| `src/backend/src/solver/restricciones_turnos.cpp` | Fijar slots + sum = bloques | 🔲 Pendiente |
| `src/backend/src/solver/restricciones_profesores.cpp` | Un-profesor-por-MC + horas (Sprint 5) | 🔲 Pendiente |
| `test/backend/test_variables_cp_sat.cpp` | Tests GTest para crearVariables() | ✅ Completado (21/21) |
| `test/backend/test_solver_horarios.cpp` | Tests GTest para solver completo | 🔲 Pendiente |

### Archivos a Modificar

| Archivo | Cambio | Estado |
|---------|--------|--------|
| `src/backend/CMakeLists.txt` | Agregar archivos fuente solver | ✅ variables.cpp agregado |
| `test/CMakeLists.txt` | Registrar tests GTest | ✅ test_variables_cp_sat registrado |

### Convenciones

- **C++17**, Qt6, CMake, OR-Tools CP-SAT
- Naming: `camelCase` para variables/funciones, `PascalCase` para clases/structs
- Variable naming en CP-SAT: `QString::asprintf` (no fmt)
- Tests: Google Test (no QTest)

---

## Sprint 4 — Issue #25

### T1.1 — Config Structs ✅

**Objetivo**: Definir las estructuras C++ que mapean el JSON de configuracion de 13 secciones.

**Archivos creados**:
- `src/backend/include/backend/solver/config/solver_config.hpp` (194 lineas)
- `src/backend/src/solver/config/solver_config.cpp` (538 lineas, parser con 12 validaciones)

**Implementado**:
- `SolverConfig` (root) con todas las secciones del JSON
- `CursoSolverConfig`: nombre, turno, aula_fija, num_estudiantes, plan, materias
- `ProfesorSolverConfig`: nombre, horas_requeridas, horas_aula, materias_asignadas, materias_suplente, disponibilidad
- `MateriaSolverConfig`: nombre
- `AulaSolverConfig`: nombre, capacidad
- Parser `SolverConfig::fromJson()` con 12 validaciones (V1-V12)

**Estado**: ✅ Completado (2026-07-24)

---

### T1.2 — Parser JSON ✅

**Objetivo**: Funcion que lee un QJsonObject y produce un `SolverConfig` completo, validando cada campo.

**Archivo creado**:
- `src/backend/src/solver/config/solver_config.cpp` — `SolverConfig::fromJson(const QJsonObject&)`

**Implementado**:
- Parseo de las 13 secciones del JSON
- 12 reglas de validacion (V1-V12) implementadas
- Errores claros con formato: `"Error de validacion: {descripcion}"`

**Estado**: ✅ Completado (2026-07-24)

---

### T1.3 — Header solver.hpp ✅

**Objetivo**: Definir la interfaz publica del solver.

**Archivos creados**:
- `src/backend/include/backend/solver/solver.hpp`

**Implementado**:
```cpp
class Solver {
public:
    struct ResultadoSolver {
        bool exitoso;
        HorarioSalida resultado;
        QStringList errores;
        double tiempo_ms;
    };

    ResultadoSolver resolver(const SolverConfig& config);

private:
    void agregarRestricciones(const SolverConfig& config);
    HorarioSalida extraerSolucion(const SolverConfig& config);
};
```

**Estado**: ✅ Completado (2026-07-24)

---

### T1.3.5 — Variables Modulares ✅

**Objetivo**: Separar la creacion de variables del orquestador del solver en un modulo independiente.

**Archivos creados**:
- `src/backend/include/backend/solver/variables.hpp` — `VariablesSolver` struct
- `src/backend/src/solver/variables.cpp` — `crearVariables()` (65 lineas)

**Implementado**:
```cpp
// variables.hpp
struct VariablesSolver {
    // assignment[profesor][materia][curso][dia][slot] = BoolVar
    QMap<QVector<int>, operations_research::BoolVar> assignment;
    // aula_assignment[curso][dia][slot][aula] = BoolVar (solo aulas moviles)
    QMap<QVector<int>, operations_research::BoolVar> aulaAssignment;
};

// variables.cpp
VariablesSolver crearVariables(
    const SolverConfig& config,
    operations_research::sat::CpModelBuilder& model
);
```

**Pruning implementado**:
1. Titular: profesor tiene materia en `materias_asignadas`
2. Suplente: profesor tiene materia en `materias_suplente`
3. Disponibilidad: profesor disponible en (dia, slot)
4. Turno: slot dentro del turno del curso

**Separacion modular**: `variables.cpp` es un archivo independiente de `solver.cpp`. El orquestador solo llama a `crearVariables()`.

**Estado**: ✅ Completado (2026-07-24)

---

### T1.4 — Tests Variables ✅

**Objetivo**: Tests GTest para validar crearVariables() con 21 escenarios.

**Archivo creado**:
- `test/backend/test_variables_cp_sat.cpp` — 21 tests GTest

**Tests implementados**:
1. Config vacio → 0 variables
2. Happy path (1 prof, 1 curso, 1 materia) → 30 variables
3. Key correctness (prof, materia, curso, dia, slot)
4. Pruning: profesor no ensena materia
5. Pruning: profesor no disponible
6. Disponibilidad parcial (2 dias → 12 variables)
7. Slot fuera de turno no creado
8. Turno tarde crea slots 6-11
9. Profesor suplente crea variables
10. Dos profesores misma materia → 60 variables
11. Dos profesores disponibilidad distinta → pruning correcto
12. Aula fija → sin aulaAssignment
13. Aula movil → crea aulaAssignment (1 aula → 30, 3 aulas → 90)
14. Key correctness aulaAssignment
15. Multiples cursos turnos distintos → 60 variables
16. Multiples materias por curso → 60 variables
17. Profesor ensena 1 de 2 materias → pruning funciona
18. Mixto fija y movil → solo movil en aulaAssignment
19. Sin aulas → sin aulaAssignment
20. Profesor disponible en slots parciales → 10 variables
21. Todos los tests pasan

**Estado**: ✅ Completado (2026-07-24) — 21/21 pasando

---

### T1.4 — Skeleton solver.cpp (Proxima)

**Objetivo**: Implementar la logica core del solver: resolver(), agregarRestricciones(), extraerSolucion().

**Archivos a crear**:
- `src/backend/src/solver/solver.cpp`

**Que hay que hacer**:
- Implementar `Solver::resolver()`:
  1. Crear `CpModelBuilder model`
  2. Llamar `crearVariables(config, model)` (ya implementado en variables.cpp)
  3. Llamar `agregarRestricciones(config, model)` (vacio por ahora)
  4. Crear `CpSolver`, llamar `Solve()`, verificar status
  5. Si OPTIMAL/FEASIBLE: llamar `extraerSolucion()`
  6. Si INFEASIBLE: retornar ResultadoSolver con exitoso=false
- Implementar `agregarRestricciones()` basico (stub vacio por ahora)
- Implementar `extraerSolucion()` basico (stub que retorna HorarioSalida vacio)

**Archivos a modificar**:
- `src/backend/CMakeLists.txt` — agregar `src/solver/solver.cpp` a target_sources

**Depende de**: T1.3, T1.3.5

**Verificacion**: Compila. Test basico puede instanciar Solver y llamar resolver() sin crash.

---

### T1.6 — restricciones_turnos.cpp

**Objetivo**: Fijar slots segun turno del curso y garantizar que cada curso reciba la cantidad correcta de bloques.

**Archivos a crear**:
- `src/backend/src/solver/restricciones_turnos.cpp`

**Que hay que hacer**:
- **Fijar slots por turno**: Si turno="manana", solo slots 0-5. Si "tarde", solo slots 6-11. Si automatico, solver decide.
- **Sum = bloques**: Para cada (curso, materia): `sum(d,s) assignment[p][m][c][d][s] = horas_semanales / (duracion_slot/60)` para el profesor asignado
- **Maximo uno por slot**: Para cada (curso, dia, slot): `sum(m, p) assignment[p][m][c][d][s] <= 1` (un curso no puede ver 2 materias al mismo tiempo)

**Funciones esperadas**:
```cpp
void agregarRestriccionesTurnos(
    CpModel& model,
    const SolverConfig& config,
    QMap<...>& assignment
);
```

**Depende de**: T1.4

**Verificacion**: Test con 2 cursos en turnos distintos, verificar que respetan manana/tarde.

---

### T1.7 — CMake Wiring

**Objetivo**: Agregar los archivos solver al build.

**Archivos a modificar**:
- `src/backend/CMakeLists.txt`

**Que hay que hacer**:
Agregar al `target_sources`:
```cmake
src/solver/solver_horarios.cpp
src/solver/restricciones_aulas.cpp
src/solver/restricciones_turnos.cpp
```

**Depende de**: T1.4, T1.5, T1.6

**Verificacion**: `cmake --build build` compila sin errores.

---

### T1.8 — Tests GTest

**Objetivo**: Escribir tests para Milestones 1-4.

**Archivos a crear**:
- `test/backend/test_solver_horarios.cpp`

**Que hay que hacer**:
- Fixture con JSON de config pequeno (5 prof, 3 aulas, 10 materias)
- **Milestone 1**: Parser retorna config valida
- **Milestone 2**: Solver encuentra solucion con NoOverlap basico
- **Milestone 3**: Turnos manana/tarde respetados
- **Milestone 4**: Aulas no solapadas + capacidad

**Depende de**: T1.5, T1.6

**Verificacion**: Todos los tests pasan.

---

### T1.9 — Registrar Test

**Objetivo**: Habilitar el test en el build.

**Archivos a modificar**:
- `test/CMakeLists.txt`

**Que hay que hacer**:
Descomentar o agregar:
```cmake
add_gtest(test_backend_solver
    backend/test_solver_horarios.cpp
    # ... source files needed
)
```

**Depende de**: T1.8

**Verificacion**: `ctest --test-dir build` ejecuta el test.

---

## Sprint 5 — Issue #32

### T2.1 — restricciones_profesores.cpp

**Objetivo**: Implementar "un docente por materia-curso" y "horas semanales".

**Archivos a crear**:
- `src/backend/src/solver/restricciones_profesores.cpp`

**Que hay que hacer**:
- **Un docente por MC**: Para cada (m, c, d, s): `sum(p) assignment[p][m][c][d][s] <= 1`
- **Horas semanales**: Para cada (p, m, c): `sum(d,s) assignment[p][m][c][d][s] = horas_semanales`
- **Auxiliary BoolVars** `assigned[p][m][c]`: indican si p ensena m a c. Linked: si assigned=0 then all assignment[p][m][c][*][*]=0

**Depende de**: T1.4

**Verificacion**: Test con 2 profesores para misma materia-curso -> solo uno ensena.

---

### T2.2 — V13 Validacion Pre-Solver

**Objetivo**: Verificar que cada (materia, curso) tenga al menos 1 profesor disponible.

**Archivos a modificar**:
- Parser (T1.2)

**Que hay que hacer**:
- Para cada materia en cada curso, verificar que al menos 1 profesor la tiene en `materias_asignadas` o `materias_suplente`
- Si no hay ninguno, error: "Materia {X} en curso {Y} no tiene profesor disponible"

**Depende de**: T1.2

**Verificacion**: Test con materia sin profesor retorna error.

---

### T2.3 — Wiring en Orchestrator

**Objetivo**: Conectar todas las funciones de restricciones en `agregarRestricciones()`.

**Archivos a modificar**:
- `solver_horarios.cpp`

**Que hay que hacer**:
En `agregarRestricciones()`:
```cpp
agregarRestriccionesAulas(model, config, assignment, aula_assignment);
agregarRestriccionesTurnos(model, config, assignment);
agregarRestriccionesProfesores(model, config, assignment);
```

**Depende de**: T2.1

**Verificacion**: Compila. Solver ejecuta todas las restricciones.

---

### T2.4 — extraerSolucion Completa

**Objetivo**: Mapear la solucion CP-SAT a los structs HorarioSalida existentes.

**Archivos a modificar**:
- `solver_horarios.cpp`

**Que hay que hacer**:
- Leer valores de BoolVars (solver.Value(var) == 1)
- Para cada asignacion, poblar `AsignacionOutput` con: slot, materia, profesor, aula
- Resolver aula: si fija -> indice; si movil -> leer aula_assignment
- Agrupar por curso -> dia -> asignaciones
- Agregar metadata (tiempo, estado, estadisticas)

**Depende de**: T1.5, T1.6

**Verificacion**: Test con solucion conocida, verificar output matches expected.

---

### T2.5 — Feasibility Reporting

**Objetivo**: Detectar INFEASIBLE y construir mensaje de error util.

**Archivos a modificar**:
- `solver_horarios.cpp`

**Que hay que hacer**:
- Si status == INFEASIBLE, intentar identificar constraint conflictivo
- Retornar `ResultadoSolver` con `exitoso = false` y `errores` populated
- Incluir: que constraint fallo, que datos causaron el conflicto

**Depende de**: T2.4

**Verificacion**: Test con config imposible retorna error descriptivo.

---

### T2.6 — Tests Expandidos (Milestones 5-9)

**Objetivo**: Cubrir todos los milestones del Sprint 5.

**Archivos a modificar**:
- `test/backend/test_solver_horarios.cpp`

**Que hay que hacer**:
- **Milestone 5**: Solver completo con todas las restricciones
- **Milestone 6**: INFEASIBLE con datos imposibles
- **Milestone 7**: Un docente por materia-curso (A XOR B ensena M en C)
- **Milestone 8**: Distribucion de horas (4h -> 4 slots en >=2 dias)
- **Milestone 9**: Suplente reemplaza completo (A=0, B=3)

**Depende de**: T2.3, T2.4, T2.5

**Verificacion**: Todos los tests pasan.

---

### T2.7 — Benchmark Integracion

**Objetivo**: Validar con 10+ profesores, 5+ aulas, 15+ materias en <60s.

**Que hay que hacer**:
- Fixture grande con datos realistas
- Medir tiempo de resolucion
- Verificar que es < 60 segundos

**Depende de**: T2.6

**Verificacion**: Test pasa con tiempo < 60s.

---

## Progreso

| Tarea | Estado | Fecha |
|-------|--------|-------|
| T1.1 Config structs | [x] Completada | 2026-07-24 |
| T1.2 Parser JSON | [x] Completada | 2026-07-24 |
| T1.3 Header | [x] Completada | 2026-07-24 |
| T1.4 Skeleton | [x] Completada | 2026-07-25 |
| T1.5 Aulas constraints | [x] Completada | 2026-07-25 |
| T1.6 Turnos constraints | [x] Completada | 2026-07-25 |
| T1.7 CMake | [x] Completada | 2026-07-24 |
| T1.8 Tests | [ ] Pendiente | |
| T1.9 Register test | [ ] Pendiente | |
| T2.1 Profesores constraints | [x] Completada | 2026-07-25 |
| T2.2 V13 validation | [x] Completada | 2026-07-26 |
| T2.3 Wire orchestrator | [ ] Pendiente | |
| T2.4 Extract solution | [x] Completada | 2026-07-26 |
| T2.5 Feasibility | [x] Completada | 2026-07-26 |
| T2.6 Tests expanded | [ ] Pendiente | |
| T2.7 Benchmark | [ ] Pendiente | |
