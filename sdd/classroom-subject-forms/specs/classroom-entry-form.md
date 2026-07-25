# Classroom Entry Form Specification

## Purpose

Replace the ViewPlaceholder at stacked widget index 2 with a functional CRUD form for managing Aulas (classrooms), following the TeacherFormDialog/TeacherListWidget pattern.

## Requirements

### Requirement: ClassroomFormDialog — Data Entry

The system SHALL provide a modal dialog (`AulaFormDialog`) for creating and editing aulas with fields: nombre (required), capacidad (required integer, min=1), edificio (optional), piso (optional).

#### Scenario: Open form for new classroom

- GIVEN the user is on the Aulas view
- WHEN the user clicks "+ Registrar Aula"
- THEN a modal dialog opens with empty fields
- AND nombre and capacidad fields are focused/required

#### Scenario: Open form for edit

- GIVEN a classroom row exists in the table
- WHEN the user clicks the edit button on that row
- THEN the dialog opens pre-filled with existing values
- AND the window title reads "Editar Aula"

#### Scenario: Submit valid classroom (create)

- GIVEN the dialog is open with nombre="Sala 101", capacidad=30, edificio="A", piso="2"
- WHEN the user clicks "Guardar"
- THEN signal `aulaGuardada(nombre, capacidad, edificio, piso)` is emitted
- AND the dialog closes

#### Scenario: Submit empty nombre

- GIVEN the dialog is open with nombre="" and capacidad=20
- WHEN the user clicks "Guardar"
- THEN a validation message is displayed
- AND the dialog does NOT close

### Requirement: ClassroomFormDialog — Validation

The system SHALL validate form fields before submission. nombre MUST be non-empty (trimmed). capacidad MUST be an integer >= 1.

#### Scenario: Empty nombre rejected

- GIVEN nombre field is empty or whitespace-only
- WHEN the user clicks "Guardar"
- THEN an error indicator appears on the nombre field
- AND submission is blocked

#### Scenario: Zero or negative capacidad rejected

- GIVEN capacidad field has value 0 or negative
- WHEN the user clicks "Guardar"
- THEN an error indicator appears on the capacidad field
- AND submission is blocked

#### Scenario: Non-numeric capacidad rejected

- GIVEN capacidad field contains non-numeric text
- WHEN the user clicks "Guardar"
- THEN an error indicator appears
- AND submission is blocked

### Requirement: ClassroomListWidget — Table Display

The system SHALL display classrooms in a table with columns: Nombre, Capacidad, Edificio, Piso, Acciones. Each row SHALL have edit and delete action buttons.

#### Scenario: Table shows classrooms

- GIVEN 3 classrooms exist in the backend
- WHEN the Aulas view is displayed
- THEN the table shows 3 rows with correct data in each column

#### Scenario: Empty table state

- GIVEN no classrooms exist
- WHEN the Aulas view is displayed
- THEN the table shows 0 rows
- AND the register button is still visible and functional

### Requirement: ClassroomListWidget — Delete Confirmation

The system SHALL prompt confirmation before deleting a classroom. On confirm, the row SHALL be removed from the table.

#### Scenario: Delete with confirmation

- GIVEN a classroom row exists
- WHEN the user clicks the delete button
- THEN a confirmation dialog asks "Are you sure?"
- AND on "Yes", the row is removed from the table

#### Scenario: Delete cancelled

- GIVEN a classroom row exists
- WHEN the user clicks delete and then "No"
- THEN the row remains in the table unchanged

### Requirement: ClassroomListWidget — Edit Flow

The system SHALL allow editing existing classrooms by opening the form dialog pre-filled with current data, and updating the table row on save.

#### Scenario: Edit and save

- GIVEN a classroom row exists with nombre="Sala 101"
- WHEN the user clicks edit, changes nombre to "Sala 102", and saves
- THEN the table row updates to show "Sala 102"

### Requirement: IPC Integration — Classroom Operations

The system SHALL define IPC operations: OP_LISTA_AULAS, OP_CREAR_AULA, OP_ACTUALIZAR_AULA, OP_ELIMINAR_AULA in messages.h. Server handlers SHALL be stub implementations returning success with empty data.

#### Scenario: IPC op constants exist

- GIVEN the messages.h header is compiled
- WHEN the middleware is built
- THEN OP_LISTA_AULAS, OP_CREAR_AULA, OP_ACTUALIZAR_AULA, OP_ELIMINAR_AULA are defined

#### Scenario: Stub handler for create

- GIVEN a OP_CREAR_AULA request arrives at InternalServer
- WHEN the server processes it
- THEN a success response is returned with the request data echoed

### Requirement: MainWindow Integration — Aulas View

The system SHALL replace the ViewPlaceholder at stacked widget index 2 with ClassroomListWidget. Navigation via the sidebar "Aulas" button SHALL switch to this view.

#### Scenario: Sidebar navigates to Aulas

- GIVEN the app is on any view
- WHEN the user clicks "Aulas" in the sidebar
- THEN the stacked widget shows the ClassroomListWidget (index 2)
- AND the sidebar highlight moves to "Aulas"

#### Scenario: Placeholder is removed

- GIVEN the change is implemented
- WHEN MainWindow::setupCentralArea() runs
- THEN ViewPlaceholder("Gestion de Aulas") is no longer instantiated
