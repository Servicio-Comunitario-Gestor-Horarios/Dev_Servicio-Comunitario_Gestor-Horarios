# Design: Classroom & Subject Entry Forms

## Technical Approach

Replicate the established TeacherFormDialog + TeacherListWidget pattern for two new entity types (Aula, Materia) and wire them into the existing MainWindow navigation. IPC stubs follow the same handler-per-operation pattern in InternalServer, with new op codes in messages.h.

## Architecture Decisions

### Decision: Follow existing inline-styling pattern (no stylesheet files)

| Option | Tradeoff | Decision |
|--------|----------|----------|
| Inline stylesheets | Consistent with TeacherFormDialog; self-contained per widget; harder to theme globally | **Chosen** — matches existing codebase |
| External .qss file | Cleaner separation; not used anywhere in project | Rejected |

### Decision: QDialog for forms (not embedded panels)

| Option | Tradeoff | Decision |
|--------|----------|----------|
| Modal QDialog | Blocks parent interaction; forces explicit save/cancel; matches TeacherFormDialog | **Chosen** |
| Embedded QWidget panel | No context switch; requires complex state management for edit mode | Rejected |

### Decision: QSpinBox for capacidad (not QLineEdit + QIntValidator)

| Option | Tradeoff | Decision |
|--------|----------|----------|
| QSpinBox | Built-in min/max enforcement; arrow buttons; visual spinner | **Chosen** |
| QLineEdit + QIntValidator | Consistent with other fields; requires manual validation in save | Rejected |

### Decision: IPC stubs with same handler pattern

| Option | Tradeoff | Decision |
|--------|----------|----------|
| Add handleAula*/handleMateria* stubs + onReadyRead branches | Consistent; easy to wire to backend later | **Chosen** |
| Generic handler with operation routing table | Cleaner but different from existing pattern | Rejected |

## Data Flow

```
User clicks "Registrar Aula"
  └─→ ClassroomListWidget::abrirFormularioNuevo()
        └─→ ClassroomFormDialog (modal)
              ├── validates fields (QSpinBox enforces 1-500)
              ├── emit aulaGuardada(nombre, capacidad, edificio, piso)
              └── accept()

ClassroomListWidget receives signal
  └─→ agregarAulaATabla() → inserts row in QTableWidget

(Same flow for SubjectFormDialog / SubjectListWidget)
```

IPC path (stub):
```
ListWidget → InternalClient::sendRequest(OP_CREAR_AULA, data) [future]
          → InternalServer::onReadyRead()
            → handleAulaCreate() → sendResponse()
```

## File Changes

| File | Action | Description |
|------|--------|-------------|
| `src/frontend/src/forms/classroom_form_dialog.hpp` | Create | ClassroomFormDialog class declaration |
| `src/frontend/src/forms/classroom_form_dialog.cpp` | Create | Form implementation: fields, layout, validation, save signal |
| `src/frontend/src/forms/subject_form_dialog.hpp` | Create | SubjectFormDialog class declaration |
| `src/frontend/src/forms/subject_form_dialog.cpp` | Create | Form implementation: fields, layout, save signal |
| `src/frontend/src/views/classroom_list_widget.hpp` | Create | ClassroomListWidget class declaration |
| `src/frontend/src/views/classroom_list_widget.cpp` | Create | List view with table, detail panel, CRUD buttons |
| `src/frontend/src/views/subject_list_widget.hpp` | Create | SubjectListWidget class declaration |
| `src/frontend/src/views/subject_list_widget.cpp` | Create | List view with table, detail panel, CRUD buttons |
| `src/middleware/include/middleware/messages.h` | Modify | Add OP_*_AULA and OP_*_MATERIA constants |
| `src/middleware/include/middleware/internalserver.h` | Modify | Add handleAula*/handleMateria* declarations |
| `src/middleware/src/server/internalserver.cpp` | Modify | Add stub handlers + onReadyRead branches |
| `src/frontend/src/views/main_window.cpp` | Modify | Replace ViewPlaceholder indices 2,3 with real widgets |
| `src/frontend/src/views/main_window.hpp` | Modify | Add forward declarations |
| `src/frontend/CMakeLists.txt` | Modify | Add 4 new .cpp and 4 new .hpp files |

## Interfaces / Contracts

### ClassroomFormDialog

```cpp
namespace gestor::frontend::forms {

class ClassroomFormDialog : public QDialog {
    Q_OBJECT
public:
    explicit ClassroomFormDialog(QWidget *parent = nullptr);
    ~ClassroomFormDialog() override = default;
    void cargarDatos(const QString& nombre, int capacidad,
                     const QString& edificio, const QString& piso);

signals:
    void aulaGuardada(const QString& nombre, int capacidad,
                      const QString& edificio, const QString& piso);

private slots:
    void guardarAula();

private:
    void configurarUI();
    void configurarValidadores();

    QLineEdit   *campoNombre;
    QSpinBox    *campoCapacidad;
    QLineEdit   *campoEdificio;
    QLineEdit   *campoPiso;
    QPushButton *botonGuardar;
    QPushButton *botonCancelar;
};

} // namespace gestor::frontend::forms
```

**Key validation rules:**
- `campoNombre`: required, non-empty, maxLength 100
- `campoCapacidad`: QSpinBox(1, 500, 1), default 1
- `campoEdificio`: optional, maxLength 50
- `campoPiso`: optional, maxLength 10

### ClassroomListWidget

```cpp
class ClassroomListWidget : public QWidget {
    Q_OBJECT
public:
    explicit ClassroomListWidget(QWidget *parent = nullptr);

private slots:
    void abrirFormularioNuevo();
    void agregarAulaATabla(const QString& nombre, int capacidad,
                           const QString& edificio, const QString& piso);

private:
    void setupUi();
    QTableWidget *m_table;
    QPushButton  *m_registerButton;
};
```

**Table columns:** Nombre | Capacidad | Edificio | Piso | Acciones

### SubjectFormDialog

```cpp
namespace gestor::frontend::forms {

class SubjectFormDialog : public QDialog {
    Q_OBJECT
public:
    explicit SubjectFormDialog(QWidget *parent = nullptr);
    ~SubjectFormDialog() override = default;
    void cargarDatos(const QString& nombre, const QString& requisitos);

signals:
    void materiaGuardada(const QString& nombre, const QString& requisitos);

private slots:
    void guardarMateria();

private:
    void configurarUI();

    QLineEdit   *campoNombre;
    QLineEdit   *campoRequisitos;
    QPushButton *botonGuardar;
    QPushButton *botonCancelar;
};

} // namespace gestor::frontend::forms
```

**Key validation rules:**
- `campoNombre`: required, non-empty, maxLength 100
- `campoRequisitos`: optional, placeholder "Ej. Pre-cálculo, Química I"

### SubjectListWidget

```cpp
class SubjectListWidget : public QWidget {
    Q_OBJECT
public:
    explicit SubjectListWidget(QWidget *parent = nullptr);

private slots:
    void abrirFormularioNuevo();
    void agregarMateriaATabla(const QString& nombre, const QString& requisitos);

private:
    void setupUi();
    QTableWidget *m_table;
    QPushButton  *m_registerButton;
};
```

**Table columns:** Nombre | Requisitos | Acciones

### IPC: messages.h additions

```cpp
// ─── Códigos de operación Aulas ─────────────────────────────────
inline const QString OP_LISTA_AULAS        = QStringLiteral("classroom_list");
inline const QString OP_OBTENER_AULA       = QStringLiteral("classroom_get");
inline const QString OP_CREAR_AULA         = QStringLiteral("classroom_create");
inline const QString OP_ACTUALIZAR_AULA    = QStringLiteral("classroom_update");
inline const QString OP_ELIMINAR_AULA      = QStringLiteral("classroom_delete");

// ─── Códigos de operación Asignaturas ──────────────────────────
inline const QString OP_LISTA_MATERIAS     = QStringLiteral("subject_list");
inline const QString OP_OBTENER_MATERIA    = QStringLiteral("subject_get");
inline const QString OP_CREAR_MATERIA      = QStringLiteral("subject_create");
inline const QString OP_ACTUALIZAR_MATERIA = QStringLiteral("subject_update");
inline const QString OP_ELIMINAR_MATERIA   = QStringLiteral("subject_delete");
```

### InternalServer handler declarations (additions)

```cpp
// ── CRUD Aulas (stubs) ──
void handleAulaList(QLocalSocket *clienteSocket);
void handleAulaGet(const QJsonObject &data, QLocalSocket *clienteSocket);
void handleAulaCreate(const QJsonObject &data, QLocalSocket *clienteSocket);
void handleAulaUpdate(const QJsonObject &data, QLocalSocket *clienteSocket);
void handleAulaDelete(const QJsonObject &data, QLocalSocket *clienteSocket);

// ── CRUD Asignaturas (stubs) ──
void handleMateriaList(QLocalSocket *clienteSocket);
void handleMateriaGet(const QJsonObject &data, QLocalSocket *clienteSocket);
void handleMateriaCreate(const QJsonObject &data, QLocalSocket *clienteSocket);
void handleMateriaUpdate(const QJsonObject &data, QLocalSocket *clienteSocket);
void handleMateriaDelete(const QJsonObject &data, QLocalSocket *clienteSocket);
```

### MainWindow changes (in setupCentralArea)

Replace:
```cpp
m_contenedorVistas->addWidget(new ViewPlaceholder("Gestión de Aulas", this));
m_contenedorVistas->addWidget(new ViewPlaceholder("Gestión de Asignaturas", this));
```
With:
```cpp
#include "classroom_list_widget.hpp"
#include "subject_list_widget.hpp"
// ...
m_contenedorVistas->addWidget(new ClassroomListWidget(this));
m_contenedorVistas->addWidget(new SubjectListWidget(this));
```

### CMakeLists.txt additions

```cmake
# FRONTEND_SOURCES: add after teacher_form_dialog.cpp
src/forms/classroom_form_dialog.cpp
src/forms/subject_form_dialog.cpp
src/views/classroom_list_widget.cpp
src/views/subject_list_widget.cpp

# FRONTEND_HEADERS: add after teacher_form_dialog.hpp
src/forms/classroom_form_dialog.hpp
src/forms/subject_form_dialog.hpp
src/views/classroom_list_widget.hpp
src/views/subject_list_widget.hpp
```

## Styling Reference

All styles follow the existing TeacherFormDialog palette:

| Element | Style |
|---------|-------|
| Dialog background | `background-color: white` |
| Input fields | `border: 1px solid #d1d5db; border-radius: 6px; padding: 8px; background-color: white; color: #374151; font-size: 13px;` |
| Labels (QFormLayout) | `<span style='font-weight:bold; color:#111827; font-size: 13px;'>` |
| Primary button | `background-color: #1a237e; color: white; font-weight: bold; padding: 10px 20px; border-radius: 6px;` |
| Primary button hover | `background-color: #283593` |
| Cancel button | `background-color: white; border: 1px solid #d1d5db; color: #374151;` |
| Cancel button hover | `background-color: #f3f4f6` |
| Table header | `background-color: white; font-weight: bold; color: #374151; border-bottom: 1px solid #e0e0e0;` |
| Register button | Same as primary button with cursor PointingHandCursor |
| Delete button | `color: #dc2626; background: transparent; border: none;` |
| Edit button | `color: #2563eb; background: transparent; border: none;` |
| Dialog size | `resize(400, 250)` for Subject; `resize(450, 300)` for Classroom |
| Right panel (detail) | `background-color: white; border-radius: 12px; border: 1px solid #e0e0e0; min-width: 320; max-width: 400` |

## Testing Strategy

| Layer | What to Test | Approach |
|-------|-------------|----------|
| Unit | Field validation (empty name rejected, capacity bounds) | Manual modal testing |
| Integration | Signal emission on save, table row insertion | Qt signal/slot connect verification |
| UI | Dialog opens, fields populate in edit, cancel discards | Visual manual testing |

No automated test infrastructure exists in this project. Testing is manual/visual.

## Migration / Rollout

No data migration required. The IPC handlers are stubs returning empty/success responses — identical to the teacher CRUD stubs already in place. When backend services are ready, the stub implementations get replaced with real logic.

## Implementation Order

1. **messages.h** — Add all 10 new op code constants (foundation for IPC)
2. **ClassroomFormDialog** (.hpp + .cpp) — New file, self-contained, follows TeacherFormDialog exactly
3. **ClassroomListWidget** (.hpp + .cpp) — New file, follows TeacherListWidget pattern
4. **SubjectFormDialog** (.hpp + .cpp) — Simpler than classroom (fewer fields)
5. **SubjectListWidget** (.hpp + .cpp) — Same pattern as classroom list
6. **InternalServer** (.h + .cpp) — Add handler declarations and stub implementations
7. **MainWindow** (.cpp + .hpp) — Replace ViewPlaceholder with real widgets, add includes
8. **CMakeLists.txt** — Register all 8 new files
9. **Build & verify** — cmake + make, visual check of all navigation paths

## Open Questions

- [ ] Should the right-side detail panel show entity-specific data (e.g., classroom capacity, subject prerequisites) or remain a generic placeholder?
- [ ] Is there a maximum number of aulas/materias the system should handle? (affects table virtualization decisions)
- [ ] Should the edit flow for aulas also allow editing via double-click on table row, or only via the edit button?
