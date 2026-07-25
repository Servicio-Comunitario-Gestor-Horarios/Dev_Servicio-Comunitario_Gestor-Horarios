# Exploration: Sprint 4 — Gestor-Horarios

## Current State

### Architecture
- **4 capas**: `common`, `backend`, `frontend`, `middleware`
- **Backend** (`src/backend/`): 6 data models (value objects sin IDs de persistencia), 5 servicios CRUD (con DTOs), database SQLite con 6 tablas, directorios solver vacíos
- **Frontend** (`src/frontend/`): MainWindow con sidebar, TeacherListWidget, TeacherFormDialog, DashboardWidget, ViewPlaceholder para aulas/asignaturas/generación/visualización
- **Middleware** (`src/middleware/`): InternalClient/Server con IPC via QLocalSocket, messages.h con teacher CRUD ops (stubs)
- **Tests** (`test/`): QTest framework, 13 archivos de test en test/backend/

### Database Schema (migracion.cpp)
Tablas existentes: `Aulas`, `Profesores`, `Materias`, `Profesor_Materia`, `Disponibilidad_Profesor`, `PlanEstudio`, `PlanEstudio_Materia`

**NO existen**: `Secciones`, `ScheduleEntries`, `Seccion_Materia`

### Service Pattern
Cada servicio sigue:
- `struct XxxDTO` con `toXxx()` para conversión al solver
- `class ServicioXxx` con CRUD (crear/obtener/listar/actualizar/eliminar) + extracción para solver
- Constructor: `explicit ServicioXxx(QSqlDatabase& db)`
- Validación interna, mapeo `QSqlRecord -> DTO`, retorno `Resultado<T>`

### Solver State
Directorios `solver/core/` y `solver/constraints/` están vacíos. OR-Tools linkeado pero sin implementación.

---

## Per-Issue Technical Analysis

### Issue #25: [Backend] Modelo CP-SAT + restricciones (aulas, turnos)

**Depende de**: #20 (Sprint 3 — data models existentes)

**Archivos a crear:**
- `src/backend/include/backend/solver/solver_horarios.hpp` — Clase principal del solver
- `src/backend/src/solver/core/solver_horarios.cpp` — Implementación del modelo CP-SAT
- `src/backend/src/solver/constraints/restricciones_aulas.cpp` — Restricciones de capacidad/aula
- `src/backend/src/solver/constraints/restricciones_turnos.cpp` — Restricciones de franja horaria

**Archivos a modificar:**
- `src/backend/CMakeLists.txt` — Agregar nuevos archivos .cpp al target
- `test/CMakeLists.txt` — Agregar test del solver
- `test/backend/test_solver.cpp` — Test unitario del solver (nuevo)

**Enfoque de implementación:**
1. Clase `SolverHorarios` que recibe `QVector<Profesor>`, `QVector<Aula>`, `QVector<Materia>`, `QVector<FranjaHoraria>` como input
2. Crear variables binarias `x[p,a,m,f]` = profesor p asignado a materia m en aula a en franja f
3. Restricciones:
   - Cada materia debe asignarse exactamente `horas_semanales` veces
   - Un profesor no puede estar en dos aulas simultáneamente
   - Un aula no puede tener dos materias simultáneamente
   - Respetar disponibilidad del profesor
4. Método `resolver()` retorna `Resultado<Horario>` con timeout configurable (default 30s)
5. Método `generarReporteInfeasibilidad()` para cuando no hay solución
6. Serialización a JSON del resultado

**Por qué este enfoque**: CP-SAT es el solver recomendado para problemas de satisfacción de restricciones. Las variables binarias son el patrón estándar para asignación. El timeout evita bloqueos.

**Contexto del proyecto necesario**:
- `backend/data/horario.hpp` — structs `Asignacion` y `Horario` (ya existen)
- `backend/data/profesor.hpp` — struct `Profesor` con `disponibilidad` y `materias`
- `backend/data/aula.hpp` — struct `Aula` con `capacidad`
- `backend/data/franja_horaria.hpp` — struct `FranjaHoraria` con `dia`, `inicio`, `fin`
- `backend/data/materia.hpp` — struct `Materia` con `horas_semanales`
- `backend/resultado.hpp` — template `Resultado<T>`

---

### Issue #28: [Backend] CRUD horarios (asignación manual)

**Depende de**: #23 (ServicioAula ✅ existe), #21 (ServicioMaterias ✅ existe)

**Archivos a crear:**
- `src/backend/include/backend/services/ServicioHorario.hpp` — Servicio CRUD para schedule_entries
- `src/backend/src/services/ServicioHorario.cpp` — Implementación
- `test/backend/test_servicio_horario.cpp` — Tests unitarios

**Archivos a modificar:**
- `src/backend/src/database/migracion.cpp` — Agregar tabla `ScheduleEntries`
- `src/backend/CMakeLists.txt` — Agregar ServicioHorario.cpp
- `test/CMakeLists.txt` — Agregar test_servicio_horario

**Nueva tabla SQL:**
```sql
CREATE TABLE IF NOT EXISTS ScheduleEntries (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    id_Profesor TEXT NOT NULL REFERENCES Profesores(id),
    id_Aula INTEGER NOT NULL REFERENCES Aulas(id),
    id_Materia INTEGER NOT NULL REFERENCES Materias(id),
    dia INTEGER NOT NULL CHECK(dia BETWEEN 0 AND 6),
    hora_inicio TEXT NOT NULL,
    hora_fin TEXT NOT NULL,
    tipo TEXT NOT NULL DEFAULT 'manual' CHECK(tipo IN ('manual', 'auto')),
    fecha_creacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    fecha_modificacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(id_Profesor, dia, hora_inicio),
    UNIQUE(id_Aula, dia, hora_inicio)
)
```

**ServicioHorario DTO y API:**
```cpp
struct HorarioEntradaDTO {
    int id = -1;
    QString idProfesor;
    int idAula = -1;
    int idMateria = -1;
    int dia = 0;
    QString horaInicio;
    QString horaFin;
    QString tipo;
};

class ServicioHorario {
    Resultado<HorarioEntradaDTO> crearEntrada(...);
    Resultado<HorarioEntradaDTO> obtenerEntrada(int id) const;
    QVector<HorarioEntradaDTO> listarEntradas() const;
    Resultado<HorarioEntradaDTO> actualizarEntrada(int id, ...);
    bool eliminarEntrada(int id);
    
    // Validación de conflictos
    Resultado<void> validarConflictos(int idProfesor, int idAula, int dia, 
                                       const QString& horaInicio, const QString& horaFin,
                                       int excludeId = -1) const;
    
    // Para el solver
    QVector<Asignacion> obtenerEntradasParaSolver() const;
};
```

**Por qué**: La tabla con UNIQUE constraints en `(profesor, dia, hora_inicio)` y `(aula, dia, hora_inicio)` previene conflictos a nivel de DB. La validación en el servicio da mensajes de error claros.

---

### Issue #30: [Backend] Servicio de consultas de horarios

**Depende de**: #28 (ServicioHorario — dentro del mismo sprint)

**Archivos a crear:**
- `src/backend/include/backend/services/ServicioConsultaHorarios.hpp` — Consultas específicas para UI
- `src/backend/src/services/ServicioConsultaHorarios.cpp` — Implementación
- `test/backend/test_servicio_consulta_horarios.cpp` — Tests

**Archivos a modificar:**
- `src/backend/CMakeLists.txt` — Agregar nuevos archivos
- `test/CMakeLists.txt` — Agregar test

**API del servicio:**
```cpp
class ServicioConsultaHorarios {
public:
    explicit ServicioConsultaHorarios(QSqlDatabase& db);
    
    // GET /schedules/teacher/{id} — Horario semanal del profesor
    QVector<HorarioEntradaDTO> obtenerHorarioProfesor(const QString& idProfesor) const;
    
    // GET /schedules/classroom/{id} — Ocupación del aula
    QVector<HorarioEntradaDTO> obtenerOcupacionAula(int idAula) const;
    
    // GET /schedules/day/{day} — Horario de un día específico
    QVector<HorarioEntradaDTO> obtenerHorarioPorDia(int dia) const;
    
    // Serialización JSON para UI
    QJsonObject horarioProfesorAJson(const QString& idProfesor) const;
    QJsonObject ocupacionAulaAJson(int idAula) const;
    QJsonObject horarioDiaAJson(int dia) const;
};
```

**Por qué servicio separado**: Las consultas de visualización tienen patrones de acceso diferentes al CRUD. Un servicio dedicado permite optimizar queries (JOINs, índices) sin contaminar el servicio CRUD.

---

### Issue #26: [Middleware/QA] Tests unitarios modelo de datos

**Depende de**: #20 (Sprint 3 — data models)

**Estado actual**: Ya existen tests para cada entity:
- `test/backend/test_aula.cpp` ✅
- `test/backend/test_franja_horaria.cpp` ✅
- `test/backend/test_materia.cpp` ✅
- `test/backend/test_profesor.cpp` ✅
- `test/backend/test_horario.cpp` ✅
- `test/backend/test_plan_estudio.cpp` ✅
- `test/backend/test_entity_graph.cpp` ✅ (round-trip completo)

**Archivos a crear/modify:**
- `test/backend/test_edge_cases.cpp` — Tests de edge cases faltantes

**Edge cases a agregar:**
- Strings vacíos en campos obligatorios
- JSON con campos faltantes en `fromJson()`
- `FranjaHoraria` con `inicio >= fin`
- `Materia` con `horas_semanales = 0`
- `Profesor` con lista vacía de materias/disponibilidad
- `Asignacion` con objetos parcialmente llenos
- `Horario` con asignaciones duplicadas

**Por qué**: Los tests existentes cubren el happy path. Los edge cases faltantes son los que causan bugs en producción.

---

### Issue #24: [Frontend] Formularios entrada docente y sección (Qt)

**Depende de**: #18 (Sprint 3 — TeacherFormDialog ya existe parcialmente)

**Archivos a crear:**
- `src/frontend/src/forms/section_form_dialog.hpp` — Formulario de sección
- `src/frontend/src/forms/section_form_dialog.cpp` — Implementación

**Archivos a modificar:**
- `src/frontend/src/views/main_window.cpp` — Conectar vista de aulas (reemplazar placeholder)
- `src/frontend/src/views/main_window.hpp` — Agregar slot mostrarSecciones si aplica
- `src/frontend/CMakeLists.txt` — Agregar section_form_dialog
- `src/middleware/include/middleware/messages.h` — Agregar OPs de secciones

**SectionFormDialog:**
```cpp
namespace gestor::frontend::forms {

class SectionFormDialog : public QDialog {
    Q_OBJECT
public:
    explicit SectionFormDialog(QWidget *parent = nullptr);
    void cargarDatos(const QString& grado, const QString& seccion, 
                     const QString& turno, const QString& materias);
signals:
    void seccionGuardada(const QString& grado, const QString& seccion,
                         const QString& turno, const QString& materias);
private slots:
    void guardarSeccion();
private:
    void configurarValidadores();
    QComboBox *campoGrado;    // 1°-6°
    QLineEdit *campoSeccion;  // A, B, C...
    QComboBox *campoTurno;    // Mañana/Tarde
    QLineEdit *campoMaterias;
};
}
```

**Por qué QComboBox para grado/turno**: Validación visual, imposible ingresar datos inválidos. Consistente con el patrón de TeacherFormDialog que usa validadores.

---

### Issue #22: [Frontend] Formularios entrada docente/sección

**Depende de**: #17 (Sprint 3 —TeacherFormDialog existe)

**NOTA**: Issue #22 y #24 se superponen significativamente. #22 se enfoca en la sección con validación frontend, #24 incluye ambos formularios. La implementación recomendada es unificar en un solo esfuerzo.

**Archivos a crear (compartidos con #24):**
- `src/frontend/src/forms/section_form_dialog.hpp` — Mismo archivo
- `src/frontend/src/forms/section_form_dialog.cpp` — Mismo archivo

**Archivos a modificar (compartidos con #24):**
- `src/frontend/src/views/section_list_widget.hpp` — Widget de listado de secciones
- `src/frontend/src/views/section_list_widget.cpp` — Implementación (patrón similar a TeacherListWidget)
- `src/frontend/src/views/main_window.cpp` — Reemplazar ViewPlaceholder de Aulas con SectionListWidget

**Validación frontend:**
- Grado: obligatorio, seleccionar de lista
- Sección: obligatorio, 1-3 caracteres, alfanumérico
- Turno: obligatorio, Mañana/Tarde
- Materias: opcional, formato libre
- Mensajes de error específicos por campo

---

## Dependency Graph (Sprint 4)

```
Sprint 3 (completado):
  #20 (data models) ──┬──→ #25 (CP-SAT solver)
                      └──→ #26 (edge case tests)
  #23 (ServicioAula) ──┐
  #21 (ServicioMaterias) └──→ #28 (CRUD horarios) ──→ #30 (consultas horarios)
  #18 (TeacherFormDialog) ──→ #24 (formularios docente+sección)
  #17 (formularios) ──→ #22 (sección + validación)

Sprint 4 (mismo sprint):
  #25 y #26 son INDEPENDIENTES (no dependen de otros Sprint 4)
  #28 es PREREQUISITO de #30 (dependencia directa dentro del sprint)
  #22 y #24 se SUPERPONEN — recommend unificar implementación
```

### Orden de implementación recomendado:
1. **#26** (edge cases tests) — Sin dependencias, rápido
2. **#25** (CP-SAT solver) — Sin dependencias de Sprint 4, complejo
3. **#28** (CRUD horarios) — Requiere #23 y #21 (ya existen)
4. **#30** (consultas horarios) — Requiere #28
5. **#24 + #22** (formularios) — Unificar, reemplazar placeholders

---

## Risk Assessment

### Riesgos Altos
1. **Solver CP-SAT (#25)**: Variables binarias para 5 profesores × 3 aulas × 10 materias × 20 franjas = 3000 variables. Riesgo de performance > 30s. **Mitigación**: Empezar con caso reducido, usar `SetTimeLimit()`.
2. **Dependencia #28 → #30**: Si #28 se retrasa, #30 queda bloqueado. **Mitigación**: Implementar #28 primero en el sprint.

### Riesgos Medios
3. **Unificación #22/#24**: Ambos issues cubren formularios de sección. **Mitigación**: Unificar en un solo PR, cerrar ambos issues.
4. **Foreign keys en ScheduleEntries**: La tabla Profesores usa `id TEXT` mientras Aulas usa `id INTEGER`. **Mitigación**: Usar tipos correctos en la FK.

### Riesgos Bajos
5. **Testing framework**: Ya configurado con QTest, funciona. Sin riesgo.
6. **Middleware stubs**: Los handlers de teacher CRUD en InternalServer son stubs. **Mitigación**: Conectar cuando ServicioProfesor exista (no es Sprint 4).

---

## Ready for Proposal

**Sí** — La exploración es completa. Todos los issues tienen:
- Archivos exactos a crear/modificar
- Enfoque de implementación claro
- Dependencias identificadas
- Patrones existentes en el codebase para seguir

**Recomendación al orquestador**: Unificar #22 y #24 en un solo deliverable. El #25 (solver) es el más complejo y debería empezar primero si hay recursos disponibles.
