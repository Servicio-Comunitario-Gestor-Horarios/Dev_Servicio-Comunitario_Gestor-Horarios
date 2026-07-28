# Subject Entry Form Specification

## Purpose

Replace the ViewPlaceholder at stacked widget index 3 with a functional CRUD form for managing Asignaturas (subjects), following the TeacherFormDialog/TeacherListWidget pattern.

## Requirements

### Requirement: SubjectFormDialog — Data Entry

The system SHALL provide a modal dialog (`MateriaFormDialog`) for creating and editing subjects with fields: nombre (required), requisitos (optional text, comma-separated).

#### Scenario: Open form for new subject

- GIVEN the user is on the Asignaturas view
- WHEN the user clicks "+ Registrar Asignatura"
- THEN a modal dialog opens with empty fields
- AND nombre field is focused/required

#### Scenario: Open form for edit

- GIVEN a subject row exists in the table
- WHEN the user clicks the edit button on that row
- THEN the dialog opens pre-filled with existing values
- AND the window title reads "Editar Asignatura"

#### Scenario: Submit valid subject (create)

- GIVEN the dialog is open with nombre="Matematicas", requisitos="Ninguno"
- WHEN the user clicks "Guardar"
- THEN signal `materiaGuardada(nombre, requisitos)` is emitted
- AND the dialog closes

#### Scenario: Submit empty nombre

- GIVEN the dialog is open with nombre=""
- WHEN the user clicks "Guardar"
- THEN a validation message is displayed
- AND the dialog does NOT close

### Requirement: SubjectFormDialog — Validation

The system SHALL validate form fields before submission. nombre MUST be non-empty (trimmed). requisitos is optional and accepts any text.

#### Scenario: Empty nombre rejected

- GIVEN nombre field is empty or whitespace-only
- WHEN the user clicks "Guardar"
- THEN an error indicator appears on the nombre field
- AND submission is blocked

#### Scenario: Empty requisitos accepted

- GIVEN nombre="Fisica", requisitos=""
- WHEN the user clicks "Guardar"
- THEN the form submits successfully
- AND requisitos is stored as empty string

### Requirement: SubjectListWidget — Table Display

The system SHALL display subjects in a table with columns: Nombre, Requisitos, Acciones. Each row SHALL have edit and delete action buttons.

#### Scenario: Table shows subjects

- GIVEN 2 subjects exist in the backend
- WHEN the Asignaturas view is displayed
- THEN the table shows 2 rows with correct data

#### Scenario: Empty table state

- GIVEN no subjects exist
- WHEN the Asignaturas view is displayed
- THEN the table shows 0 rows
- AND the register button is still visible and functional

### Requirement: SubjectListWidget — Delete Confirmation

The system SHALL prompt confirmation before deleting a subject. On confirm, the row SHALL be removed from the table.

#### Scenario: Delete with confirmation

- GIVEN a subject row exists
- WHEN the user clicks the delete button
- THEN a confirmation dialog asks "Are you sure?"
- AND on "Yes", the row is removed from the table

#### Scenario: Delete cancelled

- GIVEN a subject row exists
- WHEN the user clicks delete and then "No"
- THEN the row remains in the table unchanged

### Requirement: SubjectListWidget — Edit Flow

The system SHALL allow editing existing subjects by opening the form dialog pre-filled with current data, and updating the table row on save.

#### Scenario: Edit and save

- GIVEN a subject row exists with nombre="Matematicas"
- WHEN the user clicks edit, changes nombre to "Matematicas II", and saves
- THEN the table row updates to show "Matematicas II"

### Requirement: IPC Integration — Subject Operations

The system SHALL define IPC operations: OP_LISTA_MATERIAS, OP_CREAR_MATERIA, OP_ACTUALIZAR_MATERIA, OP_ELIMINAR_MATERIA in messages.h. Server handlers SHALL be stub implementations returning success with empty data.

#### Scenario: IPC op constants exist

- GIVEN the messages.h header is compiled
- WHEN the middleware is built
- THEN OP_LISTA_MATERIAS, OP_CREAR_MATERIA, OP_ACTUALIZAR_MATERIA, OP_ELIMINAR_MATERIA are defined

#### Scenario: Stub handler for create

- GIVEN a OP_CREAR_MATERIA request arrives at InternalServer
- WHEN the server processes it
- THEN a success response is returned with the request data echoed

### Requirement: MainWindow Integration — Asignaturas View

The system SHALL replace the ViewPlaceholder at stacked widget index 3 with SubjectListWidget. Navigation via the sidebar "Asignaturas" button SHALL switch to this view.

#### Scenario: Sidebar navigates to Asignaturas

- GIVEN the app is on any view
- WHEN the user clicks "Asignaturas" in the sidebar
- THEN the stacked widget shows the SubjectListWidget (index 3)
- AND the sidebar highlight moves to "Asignaturas"

#### Scenario: Placeholder is removed

- GIVEN the change is implemented
- WHEN MainWindow::setupCentralArea() runs
- THEN ViewPlaceholder("Gestion de Asignaturas") is no longer instantiated
