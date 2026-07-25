# Exploration: Sprint 5 — Gestor-Horarios

## Current State

### Architecture
- **4 capas**: `common`, `backend`, `frontend`, `middleware`
- **Backend** (`src/backend/`): 6 data models (value objects sin IDs de persistencia), 7 servicios CRUD (con DTOs), database SQLite con 6 tablas, directorios solver vacíos
- **Frontend** (`src/frontend/`): MainWindow con sidebar, TeacherListWidget, TeacherFormDialog, DashboardWidget, ViewPlaceholder para aulas/asignaturas/generación/visualización
- **Middleware** (`src/middleware/`): InternalClient/Server con IPC via QLocalSocket, messages.h con teacher CRUD ops (stubs)
- **Tests** (`test/`): QTest framework, 16 archivos de test en test/backend/

### Database Schema (migracion.cpp)
Tablas existentes: `Aulas`, `Profesores`, `Materias`, `Profesor_Materia`, `Disponibilidad_Profesor`, `PlanEstudio`, `PlanEstudio_Materia`

**NO existen**: `EntradasHorario` (manual schedules) — Issue #28 (Sprint 4) está pendiente

### Service Pattern
Cada servicio sigue:
- `struct XxxDTO` con `toXxx()` para conversión al solver
- `class ServicioXxx` con CRUD (crear/obtener/listar/actualizar/eliminar) + extracción para solver
- Constructor: `explicit ServicioXxx(QSqlDatabase& db)`
- Validación interna, mapeo `QSqlRecord -> DTO`, retorno `Resultado<T>`

### Solver State
Directorios `solver/` no existen. OR-Tools linkeado pero sin implementación. Issue #32 creará la estructura completa.

### Sprint 5 Issues (6 issues)
1. **#32**: Restricciones CP-SAT (Luis) — ALTO
2. **#34**: Exportar solución solver (Luis) — MEDIO
3. **#35**: Exportar horarios manuales (Nicole) — BAJO
4. **#31**: Validador de datos (Manuel) — MEDIO
5. **#29**: Refinamiento formularios (Dani) — MEDIO
6. **#27**: Formularios aula + materia (Paola) — MEDIO

---

## Issue Template Format

El proyecto usa **GitHub Issue Templates** en `.github/ISSUE_TEMPLATE/`. Para Sprint 5, los issues siguen el template `feature_request.yml` con campos:
- **Problema**: Descripción del problema actual
- **Solución Propuesta**: Cómo debería funcionar
- **Área Afectada**: area-backend, area-frontend, area-middleware/qa
- **Criterios de Aceptación**: Lista de condiciones medibles
- **Contexto Técnico**: Referencias de arquitectura (opcional)
- **Dependencias**: Números de issues de los que depende
- **Asignado Sugerido**: Luis, Dani, Paola, Nicole, Manuel
- **Sprint**: Sprint 5
- **Alternativas Consideradas**: Otras soluciones descartadas (opcional)

---

## Sprint 5 Task Analysis

### Issue #32: Restricciones CP-SAT
**Asignado**: Luis | **Complejidad**: ALTO | **Depende de**: #25 (Sprint 4)

**Archivos a crear:**
- `src/backend/include/backend/solver/IConstraint.hpp` — Interfaz abstracta
- `src/backend/include/backend/solver/HorasSemanalesConstraint.hpp` — Restricción horas semanales
- `src/backend/src/solver/HorasSemanalesConstraint.cpp` — Implementación CP-SAT
- `src/backend/include/backend/solver/RestriccionManager.hpp` — Factory + agregador
- `src/backend/src/solver/RestriccionManager.cpp` — Aplica todas las restricciones
- `src/backend/include/backend/solver/SolverEngine.hpp` — Orquestador
- `src/backend/src/solver/SolverEngine.cpp` — Ensamblaje modelo + solve
- `test/test_solver_constraints.cpp` — Tests QTest

**Archivos a modificar:**
- `src/backend/CMakeLists.txt` — Agregar nuevos archivos solver
- `test/CMakeLists.txt` — Registrar test

**Diseño clave:**
- `IConstraint` interface: Principio Abierto/Cerrado
- `DataContext` struct: Desacopla carga de datos de lógica solver
- `SolverVariables` struct: Variables internas CP-SAT (BoolVar, IntVar)
- `Resultado<Horario>`: Sigue patrón existente

**Ruta crítica**: #32 → #34 → #35

### Issue #34: Exportar solución solver
**Asignado**: Luis | **Complejidad**: MEDIO | **Depende de**: #32

**Archivos a crear:**
- `src/backend/include/backend/solver/SolucionExporter.hpp` — Métodos estáticos
- `src/backend/src/solver/SolucionExporter.cpp` — Serialización JSON/CSV
- `test/test_solucion_exporter.cpp` — Tests round-trip

**Archivos a modificar:**
- `src/backend/CMakeLists.txt` — Agregar SolucionExporter
- `test/CMakeLists.txt` — Registrar test

**Diseño clave:**
- Delega a `toJson()`/`fromJson()` existentes en data structs
- CSV con BOM UTF-8 (`\xEF\xBB\xBF`) para Excel
- Métodos estáticos (sin estado)

**Dependencia**: Necesita struct `Horario` de #32

### Issue #35: Exportar horarios manuales
**Asignado**: Nicole | **Complejidad**: BAJO | **Depende de**: #34

**Archivos a crear:**
- `src/backend/include/backend/services/ServicioExportHorarios.hpp` — Servicio wrapper
- `src/backend/src/services/ServicioExportHorarios.cpp` — Carga DB + delega a SolucionExporter
- `test/test_servicio_export_horarios.cpp` — Tests con QTemporaryDir

**Archivos a modificar:**
- `src/backend/CMakeLists.txt` — Agregar servicio
- `test/CMakeLists.txt` — Registrar test

**Diseño clave:**
- Wrapper delgado sobre `SolucionExporter`
- Carga `Horario` desde DB (necesita tabla `EntradasHorario` de #28)
- Separación: transformación pura (#34) vs servicio+DB (#35)

**Dependencia**: Necesita `SolucionExporter` de #34 Y tabla `EntradasHorario` de #28

### Issue #31: Validador de datos
**Asignado**: Manuel | **Complejidad**: MEDIO | **Depende de**: #26 (Sprint 4)

**Archivos a crear:**
- `src/middleware/include/middleware/validation/DataValidator.hpp` — Interfaz validación
- `src/middleware/src/validation/DataValidator.cpp` — Implementaciones
- `test/test_data_validator.cpp` — Tests validación

**Archivos a modificar:**
- `src/middleware/CMakeLists.txt` — Agregar archivos validación
- `test/CMakeLists.txt` — Registrar test

**Diseño clave:**
- Validación en middleware (no backend) — feedback rápido al frontend
- Patrón `Resultado<bool>` para retorno
- Reglas desde schema DB (Aulas, Profesores, Materias, PlanEstudio)
- QJsonObject como input (middleware es Qt-only)

### Issue #29: Refinamiento formularios
**Asignado**: Dani | **Complejidad**: MEDIO | **Depende de**: #31, IPC Sprint 4

**Archivos a modificar:**
- `src/frontend/src/forms/teacher_form_dialog.cpp` — Conectar IPC real
- `src/frontend/src/forms/teacher_form_dialog.hpp` — Agregar InternalClient, spinner
- `src/frontend/src/views/teacher_list_widget.cpp` — Conectar respuestas IPC
- `src/frontend/src/views/teacher_list_widget.hpp` — Agregar InternalClient

**Diseño clave:**
- Reemplazar stubs con `InternalClient` real
- Patrón de conexión IPC existente (ver `InternalClient::enviarSolicitud`)
- UX: spinner, mensajes error, atajos teclado, auto-focus
- No crea nuevas clases — mejora existentes

### Issue #27: Formularios aula + materia
**Asignado**: Paola | **Complejidad**: MEDIO | **Depende de**: IPC Sprint 4

**Archivos a crear:**
- `src/frontend/src/dialogs/AulaFormDialog.hpp` — Formulario aula
- `src/frontend/src/dialogs/AulaFormDialog.cpp` — Implementación
- `src/frontend/src/dialogs/MateriaFormDialog.hpp` — Formulario materia
- `src/frontend/src/dialogs/MateriaFormDialog.cpp` — Implementación
- `src/frontend/src/widgets/AulaListWidget.hpp` — Lista aulas
- `src/frontend/src/widgets/AulaListWidget.cpp` — Implementación
- `src/frontend/src/widgets/MateriaListWidget.hpp` — Lista materias
- `src/frontend/src/widgets/MateriaListWidget.cpp` — Implementación

**Archivos a modificar:**
- `src/frontend/CMakeLists.txt` — Agregar nuevos archivos
- `src/frontend/src/views/main_window.cpp` — Conectar en QStackedWidget

**Diseño clave:**
- Sigue patrón `TeacherFormDialog` exactamente
- Headers en `src/` (no `include/frontend/`)
- `QSpinBox` para capacidad (validación numérica)
- `QLabel` para feedback de error
- Integración en `MainWindow::setupCentralArea()` con índices 2 y 3

---

## Dependencies Graph (Sprint 5)

```
Sprint 4 (completado):
├── #25  ServicioMaterias ─────────────────────┐
├── #26  ServicioPlanesEstudio ────────────────┤
├── #28  ServicioCargaHoraria ─────────────────┤→ DataContext
├── #18  ServicioComposicionPlan ──────────────┤   (carga de datos)
├── #22  DatabaseManager/migracion ────────────┤
└── #13  Data structs (Aula, Materia, etc.) ───┘

Sprint 5 (Actual):
├── #32 Restricciones horas + integrar ← DataContext (Sprint 4)
├── #34 Exportar solver a JSON/CSV ← #32 (necesita output Horario)
├── #35 Exportar horarios manuales ← #34 (reutiliza SolucionExporter)
├── #31 Validador de datos ← Schema DB Sprint 4
├── #29 Refinamiento formularios ← #31 (validación) + IPC Sprint 4
└── #27 Formularios aula + materia ← IPC Sprint 4
```

### Critical Within-Sprint Dependencies
```
#32 (Restricciones) ──→ #34 (Export solver) ──→ #35 (Export manuales)
       ↑                       ↑                       ↑
   Sprint 4 data          #32 output              #34 exporter
```

### Execution Phases
| Phase | Issues | Dependencies |
|-------|--------|--------------|
| **Phase 1** (parallel) | #32, #31, #27 | No mutual dependencies |
| **Phase 2** (after Phase 1) | #34, #29 | #34 waits #32, #29 waits #31 |
| **Phase 3** (after #34) | #35 | Waits #34 |

**Critical path**: #32 → #34 → #35 (3 issues, sequential)

---

## Affected Files Summary

### New Files (23)
| Path | Issue |
|------|-------|
| `src/backend/include/backend/solver/IConstraint.hpp` | #32 |
| `src/backend/include/backend/solver/HorasSemanalesConstraint.hpp` | #32 |
| `src/backend/src/solver/HorasSemanalesConstraint.cpp` | #32 |
| `src/backend/include/backend/solver/RestriccionManager.hpp` | #32 |
| `src/backend/src/solver/RestriccionManager.cpp` | #32 |
| `src/backend/include/backend/solver/SolverEngine.hpp` | #32 |
| `src/backend/src/solver/SolverEngine.cpp` | #32 |
| `test/test_solver_constraints.cpp` | #32 |
| `src/backend/include/backend/solver/SolucionExporter.hpp` | #34 |
| `src/backend/src/solver/SolucionExporter.cpp` | #34 |
| `test/test_solucion_exporter.cpp` | #34 |
| `src/backend/include/backend/services/ServicioExportHorarios.hpp` | #35 |
| `src/backend/src/services/ServicioExportHorarios.cpp` | #35 |
| `test/test_servicio_export_horarios.cpp` | #35 |
| `src/middleware/include/middleware/validation/DataValidator.hpp` | #31 |
| `src/middleware/src/validation/DataValidator.cpp` | #31 |
| `test/test_data_validator.cpp` | #31 |
| `src/frontend/src/dialogs/AulaFormDialog.hpp` | #27 |
| `src/frontend/src/dialogs/AulaFormDialog.cpp` | #27 |
| `src/frontend/src/dialogs/MateriaFormDialog.hpp` | #27 |
| `src/frontend/src/dialogs/MateriaFormDialog.cpp` | #27 |
| `src/frontend/src/widgets/AulaListWidget.hpp` | #27 |
| `src/frontend/src/widgets/AulaListWidget.cpp` | #27 |

### Modified Files (8)
| Path | Changes | Issue |
|------|---------|-------|
| `src/backend/CMakeLists.txt` | Add solver + export + service files | #32, #34, #35 |
| `test/CMakeLists.txt` | Register 4 new tests | #32, #34, #35, #31 |
| `src/middleware/CMakeLists.txt` | Add validation files | #31 |
| `src/frontend/CMakeLists.txt` | Add AulaFormDialog + MateriaListWidget | #27 |
| `src/frontend/src/views/main_window.cpp` | Connect new widgets in QStackedWidget | #27 |
| `src/frontend/src/forms/teacher_form_dialog.cpp` | Connect IPC real | #29 |
| `src/frontend/src/forms/teacher_form_dialog.hpp` | Add InternalClient, spinner | #29 |
| `src/frontend/src/views/teacher_list_widget.cpp` | Connect IPC responses | #29 |

---

## Risk Assessment

### High Risk
1. **#32 CP-SAT Complexity**: Variables binarias para 10+ profesores, 5+ aulas, 15+ materias = potentially thousands of variables. Risk of performance > 60s. **Mitigation**: Start with minimal constraints, use `SetTimeLimit()`, skill ortools for examples.

### Medium Risk
2. **#34/#35 Dependency Chain**: #34 waits #32, #35 waits #34. Any delay cascades. **Mitigation**: Start #32 first, define `Horario` struct early.
3. **#29 IPC Race Conditions**: Connecting forms to IPC may expose timing issues. **Mitigation**: Reuse existing `InternalClient` pattern.

### Low Risk
4. **#27 Boilerplate**: New forms follow existing pattern exactly. **Mitigation**: Copy `TeacherFormDialog` structure.
5. **#31 Validation Rules**: Well-defined from DB schema. **Mitigation**: Follow existing `Resultado<T>` pattern.

---

## Ready for Proposal

**Sí** — La exploración es completa. Todos los issues tienen:
- Archivos exactos a crear/modificar
- Enfoque de implementación claro
- Dependencias identificadas
- Patrones existentes en el codebase para seguir

**Recomendación al orquestador**: 
1. Empezar con #32 (solver) ya que es la ruta crítica
2. #31 y #27 pueden ejecutarse en paralelo con #32
3. #34 y #29 esperan Fase 2
4. #35 es el último en la cadena

**Next step**: Create proposal for `sprint5-contexto-tecnico` change.