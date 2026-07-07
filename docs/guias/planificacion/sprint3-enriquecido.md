# Sprint 3 — Contexto Técnico Enriquecido

> Documento consolidado para revisión y aprobación del equipo. Contiene contexto técnico detallado, criterios de aceptación, diseño arquitectónico y desglose de tareas para los 6 issues de Sprint 3.

---

## Índice

- [Sprint 3 — Contexto Técnico Enriquecido](#sprint-3--contexto-técnico-enriquecido)
  - [Índice](#índice)
  - [1. Visión General](#1-visión-general)
    - [Equipo](#equipo)
    - [Issues del Sprint](#issues-del-sprint)
  - [2. ⛓️ Distinción Crítica: Solver vs Persistencia](#2-️-distinción-crítica-solver-vs-persistencia)
    - [Solver Structs (existentes — NO modificar)](#solver-structs-existentes--no-modificar)
    - [Persistence Entities (a crear/usar en Sprint 3)](#persistence-entities-a-crearusar-en-sprint-3)
  - [3. Orden de Ejecución](#3-orden-de-ejecución)
    - [Grafo de dependencias](#grafo-de-dependencias)
    - [Calendario sugerido](#calendario-sugerido)
  - [4. Issues Enriquecidos](#4-issues-enriquecidos)
    - [4.1 S3-I1 (#20) — Modelo de Datos ER](#41-s3-i1-20--modelo-de-datos-er)
    - [4.2 S3-I4 (#19) — Enrutamiento CRUD](#42-s3-i4-19--enrutamiento-crud)
    - [4.3 S3-I2 (#21) — CRUD Materias](#43-s3-i2-21--crud-materias)
    - [4.4 S3-I5 (#23) — CRUD Aulas](#44-s3-i5-23--crud-aulas)
    - [4.5 S3-I3 (#17) — Formulario Profesores](#45-s3-i3-17--formulario-profesores)
    - [4.6 S3-I6 (#18) — Dashboard + Navegación](#46-s3-i6-18--dashboard--navegación)
  - [5. Decisiones Arquitectónicas](#5-decisiones-arquitectónicas)
    - [Flujo de datos IPC](#flujo-de-datos-ipc)
  - [6. Desglose de Tareas](#6-desglose-de-tareas)
    - [Fase 1: Fundación (paralelo, día 1)](#fase-1-fundación-paralelo-día-1)
    - [Fase 2: Infraestructura (día 1-3, bloquea fases 3 y 4)](#fase-2-infraestructura-día-1-3-bloquea-fases-3-y-4)
    - [Fase 3: Servicios de Datos (día 3-5, depende de Fase 2)](#fase-3-servicios-de-datos-día-3-5-depende-de-fase-2)
    - [Fase 4: Integración Frontend (día 4-6, depende de Fase 2)](#fase-4-integración-frontend-día-4-6-depende-de-fase-2)
    - [Fase 5: Testing](#fase-5-testing)
  - [7. Riesgos](#7-riesgos)
    - [Estrategia de mitigación general](#estrategia-de-mitigación-general)

---

## 1. Visión General

**Sprint**: 29 Jun — 5 Jul 2026
**Repositorio**: `Servicio-Comunitario-Gestor-Horarios/Gestor-Horarios`
**Stack**: C++17 · Qt6 (Core/Sql/Widgets/Network) · SQLite · OR-Tools CP-SAT v9.15
**Arquitectura**: Binario único modular (app → frontend + backend + middleware)

### Equipo

| Rol | Persona | Issues asignados |
|-----|---------|------------------|
| Tech Lead & Backend | Luis | S3-I1 (ER Model) |
| Middleware & QA | Manuel | S3-I4 (CRUD Routing) |
| Frontend | Dani + Paola | S3-I6 (Dashboard) + S3-I3 (Teacher Form) |
| Backend | Nicole | S3-I2 (Subject CRUD) + S3-I5 (Classroom CRUD) |

### Issues del Sprint

| ID | Issue # | Título | Asignado |
|----|---------|--------|----------|
| S3-I1 | #20 | Modelo de datos entidades + diagrama ER | Luis |
| S3-I4 | #19 | Enrutamiento CRUD frontend → backend | Manuel |
| S3-I2 | #21 | CRUD de materias | Nicole |
| S3-I5 | #23 | CRUD de aulas | Nicole |
| S3-I3 | #17 | Formulario de registro de profesores | Paola |
| S3-I6 | #18 | Prototipos dashboard + flujo de navegación | Dani + Paola |

---

## 2. ⛓️ Distinción Crítica: Solver vs Persistencia

**⚠️ Esta es la decisión arquitectónica más importante del Sprint 3. NO confundir estas dos capas.**

### Solver Structs (existentes — NO modificar)

Ubicación: `src/backend/include/backend/data/`
Propósito: Modelado CP-SAT para OR-Tools. NO tienen campo `id`.

| Struct | Archivo | Campos clave |
|--------|---------|-------------|
| `Aula` | `aula.hpp` | nombre, capacidad, locacion |
| `Materia` | `materia.hpp` | nombre, horas_semanales, requerimientos |
| `Profesor` | `profesor.hpp` | nombre, disponibilidad, materias |
| `FranjaHoraria` | `franja_horaria.hpp` | dia, hora_inicio, hora_fin |
| `Horario` | `horario.hpp` | — |
| `PlanEstudio` | `plan_estudio.hpp` | — |

Tienen `toJson()` / `fromJson()` para serialización. Están registrados en `src/backend/CMakeLists.txt`.

### Persistence Entities (a crear/usar en Sprint 3)

Tablas SQLite con `id INTEGER PRIMARY KEY AUTOINCREMENT`.

| Entidad | Tabla | Estado |
|---------|-------|--------|
| Teacher | `teachers` | ✅ Existe (S2-I5) |
| Classroom | `classrooms` | ✅ Existe (S2-I5) |
| Subject | `subjects` | ❌ Crear (S3-I2) |
| TimeSlot | — | ❌ Futuro sprint |
| Schedule | — | ❌ Futuro sprint |

Las entidades de persistencia tienen su propia serialización JSON y son servidas por clases `*Service`. NO comparten memoria ni herencia con los solver structs — solo coinciden en nombres de campos informalmente.

---

## 3. Orden de Ejecución

### Grafo de dependencias

```
S3-I1 (#20) ER Model ──┬── sin bloqueados
                        │
S3-I6 (#18) Dashboard ──┘── sin bloqueados (frontend puro)

S3-I4 (#19) CRUD Routing ─── bloquea a S3-I2, S3-I5, S3-I3
                │
                ├── S3-I2 (#21) Subject CRUD (necesita op constants + routing)
                ├── S3-I5 (#23) Classroom CRUD (necesita op constants + routing)
                └── S3-I3 (#17) Teacher Form (necesita routing real)
```

### Calendario sugerido

| Día | Luis | Manuel | Dani+Paola | Nicole |
|-----|------|--------|------------|--------|
| Lun 29 | S3-I1 | S3-I4 | S3-I6 | — |
| Mar 30 | S3-I1 | S3-I4 | S3-I6 | — |
| Mié 1 | Review | S3-I4 | S3-I6 | S3-I2 |
| Jue 2 | Review | Review | S3-I3 | S3-I2+I5 |
| Vie 3 | — | — | S3-I3 | S3-I5 |
| Sáb 4 | — | — | S3-I3 | S3-I5 |
| Dom 5 | — | — | Review | Review |

**⚠️ Crítico**: Manuel debe mergear `messages.h` actualizado (con `subject_*`, `classroom_*`) **primero**, antes de que el routing completo esté listo. Nicole necesita esas constantes para compilar S3-I2 e I5.

---

## 4. Issues Enriquecidos

### 4.1 S3-I1 (#20) — Modelo de Datos ER

**Contexto técnico**: Crear diagrama entidad-relación en `docs/er-diagram.md` usando Mermaid. Documentar explícitamente la distinción solver vs persistence.

- Archivos de referencia: `DatabaseManager.cpp` (líneas 80-137), `test/backend/test_database.cpp`
- Formato: Mermaid con tipos SQL (INTEGER, TEXT, etc.)
- El ER debe capturar: Teacher (existe), Classroom (existe), Subject (crear), TimeSlot (futuro), Schedule (futuro), PlanEstudio (futuro)
- Relaciones N:M documentadas: Teacher ↔ Subject (tabla pivote `teacher_subjects`)

**Especificaciones**:

| ID | Statement | Fuerza |
|----|-----------|--------|
| ER1 | El diagrama Mermaid DEBE documentar TODAS las entidades de persistencia con tipos SQL | MUST |
| ER2 | Cada entidad DEBE tener `id INTEGER PRIMARY KEY AUTOINCREMENT` | MUST |
| ER3 | Todas las entidades DEBE pasar round-trip JSON (toJson→fromJson) | MUST |
| ER4 | Relaciones N:M DEBEN usar tablas pivote | MUST |

**Criterios de aceptación**:
- Diagrama ER renderizable (Mermaid) con todas las entidades, atributos y relaciones
- Distinción solver vs persistence DOCUMENTADA EXPLÍCITAMENTE en el ER
- Serialización JSON round-trip para TODAS las entidades

### 4.2 S3-I4 (#19) — Enrutamiento CRUD

**Contexto técnico**: Refactorizar `internalserver.cpp` para usar route map (QHash) en lugar de if-else. Agregar 10 constantes de operación nuevas a `messages.h`.

**Op constants a agregar en `messages.h`**:
```cpp
// CRUD Materias
OP_LISTA_MATERIAS = "subject_list"
OP_OBTENER_MATERIA = "subject_get"
OP_CREAR_MATERIA = "subject_create"
OP_ACTUALIZAR_MATERIA = "subject_update"
OP_ELIMINAR_MATERIA = "subject_delete"

// CRUD Aulas
OP_LISTA_AULAS = "classroom_list"
OP_OBTENER_AULA = "classroom_get"
OP_CREAR_AULA = "classroom_create"
OP_ACTUALIZAR_AULA = "classroom_update"
OP_ELIMINAR_AULA = "classroom_delete"
```

**Especificaciones**:

| ID | Statement | Fuerza |
|----|-----------|--------|
| RT1 | `messages.h` DEBE definir constantes subject_* y classroom_* | MUST |
| RT2 | InternalServer DEBE usar QHash dispatch map (no if-else) | MUST |
| RT3 | Cada op DEBE tener un handler que retorna `{"status":"ok","code":0}` | MUST |
| RT4 | Ops desconocidas DEBEN retornar RESP_INVALIDO (-4) | MUST |
| RT5 | Timeout de 5s DEBE retornar RESP_TIEMPO_AGOTADO (-3) | MUST |

**Criterios de aceptación**:
- Route map implementado con QHash (sin if-else por operación)
- Handler para cada operación existente + nuevas
- Ops inválidas → RESP_INVALIDO
- Timeout global de 5s → RESP_TIEMPO_AGOTADO
- Tests E2E para health_check, subject_list, classroom_list, invalid_op

**Archivos a modificar**:
- `src/middleware/include/middleware/messages.h`
- `src/middleware/include/middleware/internalserver.h`
- `src/middleware/src/server/internalserver.cpp`

### 4.3 S3-I2 (#21) — CRUD Materias

**Contexto técnico**: Crear tabla `materias` en `DatabaseManager::runMigrations()` y servicio `ServicioMaterias` con CRUD completo.

**DDL de la tabla**:
```sql
CREATE TABLE IF NOT EXISTS materias (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    horas_semanales INTEGER NOT NULL CHECK(weekly_hours > 0),
    fecha_modificacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    fecha_modificacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
)
```

**Especificaciones**:

| ID | Statement | Fuerza |
|----|-----------|--------|
| SC1 | `runMigrations()` DEBE crear tabla `materias` (id PK, nombre, horas_semanales) | MUST |
| SC2 | ServicioMaterias DEBE implementar CRUD con QSqlQuery+bindValue | MUST |
| SC3 | Validación: nombre no vacío ≤200, horas_semanales > 0 ≤ 40 (confirmar con la institución) | MUST |
| SC4 | Códigos IPC: RESP_EXITO (0), RESP_INVALIDO (-4) | MUST |

**Criterios de aceptación**:
- CRUD completo: create, get, list, update, remove
- SQL injection prevenido: todas las queries con `prepare()` + `bindValue()`
- fecha_actualizacion se actualiza automáticamente en UPDATE
- Tests: CRUD completo, validación, integración con QTemporaryDir

**Archivos a crear**:
- `src/backend/src/services/ServicioMateria.cpp`
- `src/backend/include/backend/services/ServicioMateria.hpp`
- `test/backend/test_servicio_materia.cpp`

**Archivos a modificar**:
- `src/backend/src/database/DatabaseManager.cpp` (agregar tabla)
- `src/backend/CMakeLists.txt` (registrar servicio)

### 4.4 S3-I5 (#23) — CRUD Aulas

**Contexto técnico**: Crear `ClassroomService` sobre la tabla `classrooms` existente (S2-I5). NO modificar el schema de la tabla.

**Tabla existente**:
```sql
CREATE TABLE IF NOT EXISTS classrooms (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    capacity INTEGER NOT NULL CHECK(capacity > 0),
    building TEXT,
    floor INTEGER,
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
)
```

**Especificaciones**:

| ID | Statement | Fuerza |
|----|-----------|--------|
| CC1 | ClassroomService DEBE usar tabla `classrooms` existente (sin cambios de schema) | MUST |
| CC2 | Service DEBE implementar CRUD con QSqlQuery+bindValue | MUST |
| CC3 | Validación: name no vacío UNIQUE, capacity >0 ≤500 | MUST |
| CC4 | Violaciones UNIQUE DEBEN retornar error JSON descriptivo | MUST |

**Criterios de aceptación**:
- CRUD completo sobre tabla existente
- NO se modifica el schema de `classrooms`
- Manejo graceful de UNIQUE constraint violation
- floor como QVariant opcional
- Tests: CRUD completo, UNIQUE violation, capacity validation, QTemporaryDir

**Archivos a crear**:
- `src/backend/src/services/ClassroomService.cpp`
- `src/backend/include/backend/services/ClassroomService.hpp`
- `test/backend/test_classroom_service.cpp`

**Archivos a modificar**:
- `src/backend/CMakeLists.txt` (registrar servicio)

### 4.5 S3-I3 (#17) — Formulario Profesores

**Contexto técnico**: Crear TeacherFormDialog y TeacherListWidget con conexión real al backend vía InternalClient (QLocalSocket).

**Formulario**: nombre (QLineEdit), email (QLineEdit + regex), teléfono (QLineEdit), materias (QListWidget + checkboxes)

**Especificaciones**:

| ID | Statement | Fuerza |
|----|-----------|--------|
| TF1 | TeacherFormDialog DEBE tener campos: nombre, email, teléfono, materias | MUST |
| TF2 | TeacherListWidget DEBE listar profesores con editar/eliminar | MUST |
| TF3 | El formulario DEBE usar InternalClient para IPC (OP_CREAR_PROFESOR) | MUST |
| TF4 | Email DEBE tener QRegularExpressionValidator en frontend | MUST |
| TF5 | Servidor no disponible DEBE mostrar QMessageBox de error | MUST |
| TF6 | frontend/CMakeLists.txt DEBE linkear Qt6::Network | MUST |

**Criterios de aceptación**:
- Conexión real al backend vía InternalClient
- Validación frontend: email con regex, nombre no vacío
- Manejo de errores: servidor caído, timeout 5s, validación backend
- Flujo post-login: LoginDialog → MainWindow → TeacherListWidget
- Tests: validación de campos, integración con InternalClient mockeado

**Archivos a crear**:
- `src/frontend/src/forms/teacher_form_dialog.hpp`
- `src/frontend/src/forms/teacher_form_dialog.cpp`
- `src/frontend/src/views/teacher_list_widget.hpp`
- `src/frontend/src/views/teacher_list_widget.cpp`

**Archivos a modificar**:
- `src/frontend/CMakeLists.txt` (nuevos archivos + Qt6::Network)

### 4.6 S3-I6 (#18) — Dashboard + Navegación

**Contexto técnico**: Implementar sidebar de navegación, 6 widgets placeholder, arreglar flujo post-login, documentar navegación.

**Layout**: QHBoxLayout con sidebar (QVBoxLayout + QPushButtons) | QStackedWidget

**6 widgets placeholder**: DashboardWidget, TeacherListWidget, SubjectListWidget, ClassroomListWidget, ScheduleWidget, GenerationWidget

**Especificaciones**:

| ID | Statement | Fuerza |
|----|-----------|--------|
| DB1 | MainWindow DEBE exponer método público `addPage(QWidget*)` | MUST |
| DB2 | Botones del sidebar DEBEN cambiar la página del QStackedWidget | MUST |
| DB3 | Post-login DEBE mostrar MainWindow (la app no termina) | MUST |
| DB4 | DEBEN existir 6 widgets placeholder para las vistas principales | MUST |
| DB5 | `docs/flujo-navegacion.md` DEBE documentar con diagrama Mermaid | MUST |

**Criterios de aceptación**:
- Sidebar con navegación funcional (6 botones)
- Flujo post-login: LoginDialog → MainWindow (no termina la app)
- MainWindow::addPage() público
- DashboardWidget con info del sistema
- StatusBar se actualiza al navegar
- Documento `docs/flujo-navegacion.md` con diagrama Mermaid
- Build exitoso sin warnings
- Tests: MainWindow sin crashes, sidebar tiene 6 botones

---

## 5. Decisiones Arquitectónicas

| Decisión | Opción elegida | Alternativa rechazada | Rationale |
|----------|---------------|----------------------|-----------|
| **Route map dispatch** | QHash\<QString, std::function\<\>\> | if-else chain | O(1) dispatch, nuevo handler = nueva entrada, sin modificar dispatch central |
| **Service layer** | Clases separadas con QSqlDatabase& injection | SQL inline en handlers | CRUD API consistente, testeable, sin duplicación de queries |
| **Solver vs Persistence** | Capas separadas, sin herencia ni casting | Unificar en una sola clase | Son dominios diferentes (optimización vs almacenamiento) |
| **Frontend IPC** | InternalClient directo | IPC vía signals personalizados | Ya existe, wrapper de QLocalSocket, emite respuestaRecibida |
| **Sidebar** | QHBoxLayout + QPushButtons | QDockWidget | Más simple, sin dependencias extra, control total del layout |
| **Timeout** | QTimer::singleShot por operación | QTimer global | Por operación es más preciso, global es más simple (decidir en S3-I4) |

### Flujo de datos IPC

```
Frontend (Qt6)              Middleware                  Backend
┌─────────────────┐   ┌──────────────────────┐   ┌──────────────────┐
│ TeacherForm     │   │ InternalServer       │   │ SubjectService   │
│ InternalClient ─┼──→│ onReadyRead()        │──→│ ClassroomService │
│ MainWindow      │   │ routeMap[op]()       │   │ DatabaseManager  │
│ QStackedWidget  │   │ QHash dispatch       │   │ (SQLite + QSql)  │
└─────────────────┘   └──────────────────────┘   └──────────────────┘
     QLocalSocket            O(1) lookup             bindValue()
     JSON over IPC                                   injection-safe
```

**Formato mensaje IPC**:
```
Request:  {"op":"subject_create","payload":{"name":"Matemáticas","weekly_hours":5}}
Response: {"status":"ok","code":0,"op":"subject_create","data":{"id":1,...}}
Error:    {"status":"error","code":-4,"op":"subject_create","error":"campo 'name' requerido"}
```

---

## 6. Desglose de Tareas

### Fase 1: Fundación (paralelo, día 1)

| # | Issue | Tarea | Archivos |
|---|-------|-------|----------|
| 1.1 | S3-I1 | Crear `docs/er-diagram.md` Mermaid ER con entidades, tipos SQL, PKs, FKs, N:M | `docs/er-diagram.md` |
| 1.2 | S3-I1 | Documentar solver vs persistence explícitamente en ER | `docs/er-diagram.md` |
| 1.3 | S3-I6 | Crear 6 widgets placeholder en `src/frontend/src/views/` | 6 archivos .hpp + 6 .cpp |
| 1.4 | S3-I6 | Agregar sidebar QPushButton nav a MainWindow | `main_window.hpp`, `.cpp` |
| 1.5 | S3-I6 | Arreglar flujo post-login (MainWindow + app.exec()) | `aplicacion_frontend.cpp` |
| 1.6 | S3-I6 | Exponer `addPage(QWidget*)` público | `main_window.hpp` |
| 1.7 | S3-I6 | Crear `docs/flujo-navegacion.md` con Mermaid | `docs/flujo-navegacion.md` |
| 1.8 | S3-I6 | Actualizar CMakeLists.txt (nuevos archivos + Qt6::Network) | `src/frontend/CMakeLists.txt` |

### Fase 2: Infraestructura (día 1-3, bloquea fases 3 y 4)

| # | Issue | Tarea | Archivos |
|---|-------|-------|----------|
| 2.1 | S3-I4 | Agregar 10 op constants a `messages.h` (subject_* + classroom_*) | `messages.h` |
| 2.2 | S3-I4 | Agregar QHash route map + handler signatures a internalserver.h | `internalserver.h` |
| 2.3 | S3-I4 | Refactorizar onReadyRead() a dispatch vía route map | `internalserver.cpp` |
| 2.4 | S3-I4 | Implementar stubs para cada handler, unknown op → RESP_INVALIDO | `internalserver.cpp` |
| 2.5 | S3-I4 | Agregar timeout 5s por operación → RESP_TIEMPO_AGOTADO | `internalserver.cpp` |

### Fase 3: Servicios de Datos (día 3-5, depende de Fase 2)

| # | Issue | Tarea | Archivos |
|---|-------|-------|----------|
| 3.1 | S3-I2 | Agregar tabla `subjects` en `runMigrations()` | `DatabaseManager.cpp` |
| 3.2 | S3-I2 | Crear SubjectService con CRUD + validación | `SubjectService.hpp/.cpp` |
| 3.3 | S3-I5 | Crear ClassroomService con CRUD + UNIQUE handling | `ClassroomService.hpp/.cpp` |
| 3.4 | Ambos | Registrar servicios en CMakeLists.txt | `src/backend/CMakeLists.txt` |

### Fase 4: Integración Frontend (día 4-6, depende de Fase 2)

| # | Issue | Tarea | Archivos |
|---|-------|-------|----------|
| 4.1 | S3-I3 | Crear TeacherFormDialog (campos + validación + IPC) | `teacher_form_dialog.hpp/.cpp` |
| 4.2 | S3-I3 | Crear TeacherListWidget (lista + editar/eliminar) | `teacher_list_widget.hpp/.cpp` |
| 4.3 | S3-I3 | Integrar vistas en MainWindow + CMakeLists | `CMakeLists.txt` |

### Fase 5: Testing

| # | Issue | Tarea | Archivos |
|---|-------|-------|----------|
| 5.1 | S3-I1 | QTest para solver structs JSON round-trip | `test/CMakeLists.txt` |
| 5.2 | S3-I4 | E2E test para rutas nuevas (subject_list, classroom_list, invalid) | `test/` |
| 5.3 | S3-I2 | QTest SubjectService CRUD + validación + QTemporaryDir | `test/backend/test_subject_service.cpp` |
| 5.4 | S3-I5 | QTest ClassroomService CRUD + UNIQUE + capacidad | `test/backend/test_classroom_service.cpp` |
| 5.5 | S3-I3 | QTest form: validación frontend + IPC mock | `test/` |
| 5.6 | S3-I6 | QTest MainWindow: sidebar 6 botones, post-login alive | `test/` |

---

## 7. Riesgos

| Riesgo | Impacto | Probabilidad | Mitigación |
|--------|---------|:-----------:|------------|
| **Contrato IPC no alineado**: Manuel define op constants pero no mergea → Nicole no puede compilar | Bloqueante | 🔴 Alta | Mergear `messages.h` como PR mínimo primero |
| **Frontend sin backend real**: Paola/Dani avanzan UI pero no pueden probar IPC | Retraso en S3-I3 | 🔴 Alta | Mockear respuestas IPC para avanzar UI |
| **MainWindow sin API pública**: No se puede agregar páginas al QStackedWidget | Bloqueante | 🔴 Alta | Agregar `addPage()` en MainWindow desde el inicio |
| **Confundir solver vs persistence**: Usar struct sin id donde va tabla con id | Retrabajo | 🟡 Media | Documentar la distinción en cada issue y en el ER |
| **CMake no actualizado**: Archivos nuevos no registrados → build breaks | Retraso | 🟡 Media | Cada asignado verifica build antes de PR |
| **Sidebar + toolbar UI sobrecargada**: Demasiados controles | UX pobre | 🟡 Media | Simplificar: toolbar puede eliminarse si sidebar tiene todo |
| **Timeout mal implementado**: Falsos positivos o timeouts que no se disparan | Bugs | 🟢 Baja | Usar QTimer::singleShot, probar con latencia artificial |

### Estrategia de mitigación general

1. **Op constants primero**: Manuel mergea `messages.h` como PR mínimo en el día 1
2. **Paralelización máxima**: S3-I1 + S3-I6 + S3-I4 empiezan el día 1 simultáneamente
3. **Mocks para frontend**: Si backend no está listo, Paola mockea respuestas IPC
4. **Review cruzado**: Luis revisa modelos, Manuel revisa IPC, Dani revisa frontend
5. **CI obligatorio**: Cada PR debe pasar `ctest --test-dir build` antes de mergear

---

> **Documento generado el 2026-07-06**. Basado en el análisis del código fuente, la documentación del proyecto, y el repositorio remoto (issues #17-#21, #23). Para revisión y aprobación por el equipo de desarrollo.
