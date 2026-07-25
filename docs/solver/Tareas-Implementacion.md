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

### Archivos a Crear

| Archivo | Descripcion |
|---------|-------------|
| `src/backend/include/backend/solver/solver_horarios.hpp` | Header principal |
| `src/backend/src/solver/solver_horarios.cpp` | Orquestador + variables |
| `src/backend/src/solver/restricciones_aulas.cpp` | NoOverlap aulas + capacidad + movil |
| `src/backend/src/solver/restricciones_turnos.cpp` | Fijar slots + sum = bloques |
| `src/backend/src/solver/restricciones_profesores.cpp` | Un-profesor-por-MC + horas (Sprint 5) |
| `test/backend/test_solver_horarios.cpp` | Tests GTest |

### Archivos a Modificar

| Archivo | Cambio |
|---------|--------|
| `src/backend/CMakeLists.txt` | Agregar 4-5 archivos fuente solver |
| `test/CMakeLists.txt` | Registrar test_solver_horarios |

### Convenciones

- **C++17**, Qt6, CMake, OR-Tools CP-SAT
- Naming: `camelCase` para variables/funciones, `PascalCase` para clases/structs
- Variable naming en CP-SAT: `QString::asprintf` (no fmt)
- Tests: Google Test (no QTest)

---

## Sprint 4 — Issue #25

### T1.1 — Config Structs

**Objetivo**: Definir las estructuras C++ que mapean el JSON de configuracion de 13 secciones.

**Archivos a crear**:
- `src/backend/include/backend/solver/config/solver_config.hpp` (o en el mismo header del solver)

**Que hay que hacer**:
Crear structs anidados que representen cada seccion del JSON:
- `SolverConfig` (root) con: `version`, `DimensionesConfig`, `FranjaHorariaConfig`, `TurnosConfig`, `RecesoConfig`, `QVector<CursoSolverConfig>`, `QVector<ProfesorSolverConfig>`, `QVector<MateriaSolverConfig>`, `QVector<AulaSolverConfig>`, `PlanificacionConfig`, `GeneracionConfig`, `PenalizacionesConfig`
- `CursoSolverConfig`: nombre, turno, aula_fija, num_estudiantes, plan, materias (QVector con materia_idx + horas_semanales)
- `ProfesorSolverConfig`: nombre, horas_requeridas, horas_aula, materias_asignadas (QVector<int>), materias_suplente (QVector con materia_idx + peso), disponibilidad (QVector con dia + slots)
- `MateriaSolverConfig`: nombre
- `AulaSolverConfig`: nombre, capacidad

**Depende de**: Nada (es el primero)

**Verificacion**: El header compila sin errores. Los structs tienen campos con tipos correctos.

---

### T1.2 — Parser JSON

**Objetivo**: Funcion que lee un QJsonObject y produce un `SolverConfig` completo, validando cada campo.

**Archivos a crear**:
- Funcion `SolverConfig::fromJson(const QJsonObject&)` o free function `parsearSolverConfig(const QJsonObject&)`

**Que hay que hacer**:
- Parsear las 13 secciones del JSON
- Aplicar 12+ reglas de validacion (V1-V12):
  - JSON parseable, 13 secciones presentes, tipos correctos
  - dimensiones coincide con longitudes de arrays
  - Indices en rango (materias, profesores, aulas)
  - turno en {"manana", "tarde"} o ausente
  - aula_fija = -1, o indice valido, o ausente
  - num_estudiantes > 0, materias/profesores no vacios
  - Un solo plan por curso
- Retornar errores claros con formato: `"Error de validacion: {descripcion}"`

**Depende de**: T1.1

**Verificacion**: Test con JSON completo parsea sin errores. Test con JSON invalido retorna error descriptivo.

---

### T1.3 — Header solver_horarios.hpp

**Objetivo**: Definir la interfaz publica del solver.

**Archivos a crear**:
- `src/backend/include/backend/solver/solver_horarios.hpp`

**Que hay que hacer**:
```cpp
class SolverHorarios {
public:
    struct ResultadoSolver {
        bool exitoso;
        HorarioSalida horario;
        QStringList errores;
        double tiempo_ms;
    };

    ResultadoSolver resolver(const SolverConfig& config);

private:
    void crearVariables(const SolverConfig& config);
    void agregarRestricciones(const SolverConfig& config);
    void extraerSolucion(const SolverConfig& config, ...);
};
```

**Depende de**: T1.1

**Verificacion**: Header compila. Clase es instanciable.

---

### T1.4 — Skeleton solver_horarios.cpp

**Objetivo**: Implementar la creacion de variables BoolVar 5D con pruning.

**Archivos a crear**:
- `src/backend/src/solver/solver_horarios.cpp`

**Que hay que hacer**:
- Implementar `crearVariables()`:
  - Recorrer cursos -> materias -> profesores
  - Para cada (p, m, c, d, s): crear BoolVar SOLO si:
    1. p tiene m en `materias_asignadas` O `materias_suplente`
    2. c tiene m en `cursos[c].materias`
    3. p esta disponible en (d, s)
    4. (d, s) esta dentro del turno del curso
  - Guardar variables en un mapa `QMap<QTuple<int,int,int,int,int>, operations_research::BoolVar>`
- Implementar `resolver()` basico:
  - Llamar crearVariables
  - Llamar agregarRestricciones (vacio por ahora)
  - Crear CpSolver, Solve, verificar status
- Para aulas moviles: crear `aula_assignment[c][d][s][a]` solo si `aula_fija == -1`

**Depende de**: T1.3

**Verificacion**: Compila. Test con config basico crea variables sin crash.

---

### T1.5 — restricciones_aulas.cpp

**Objetivo**: Implementar NoOverlap por aula, capacidad, y BoolVarAula para cursos moviles.

**Archivos a crear**:
- `src/backend/src/solver/restricciones_aulas.cpp`

**Que hay que hacer**:
- **NoOverlap por aula**: Para cada aula, recopilar todas las BoolVars de assignment que usan esa aula, crear IntervalVars, agregar `model.AddNoOverlap()`
- **Capacidad**: Para cada aula, si `num_estudiantes[curso] > capacidad[aula]`, impedir asignacion
- **Aula movil**: Para cursos con `aula_fija == -1`, usar `aula_assignment[c][d][s][a]` y agregar NoOverlap sobre esas variables
- **Aula fija**: Para cursos con `aula_fija = idx`, fijar `aula = idx` en todas sus asignaciones

**Funciones esperadas**:
```cpp
void agregarRestriccionesAulas(
    CpModel& model,
    const SolverConfig& config,
    QMap<...>& assignment,
    QMap<...>& aula_assignment
);
```

**Depende de**: T1.4

**Verificacion**: Test con 2 aulas, 2 cursos moviles, verificar que no se solapan.

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
| T1.2 Parser JSON | [ ] Pendiente | |
| T1.3 Header | [ ] Pendiente | |
| T1.4 Skeleton | [ ] Pendiente | |
| T1.5 Aulas constraints | [ ] Pendiente | |
| T1.6 Turnos constraints | [ ] Pendiente | |
| T1.7 CMake | [ ] Pendiente | |
| T1.8 Tests | [ ] Pendiente | |
| T1.9 Register test | [ ] Pendiente | |
| T2.1 Profesores constraints | [ ] Pendiente | |
| T2.2 V13 validation | [ ] Pendiente | |
| T2.3 Wire orchestrator | [ ] Pendiente | |
| T2.4 Extract solution | [ ] Pendiente | |
| T2.5 Feasibility | [ ] Pendiente | |
| T2.6 Tests expanded | [ ] Pendiente | |
| T2.7 Benchmark | [ ] Pendiente | |
