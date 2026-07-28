# Validation Refinement Specification

## Purpose

Add frontend field validation with inline error indicators to all form dialogs (TeacherFormDialog, AulaFormDialog, MateriaFormDialog). Currently only email regex validation exists in TeacherFormDialog.

## Requirements

### Requirement: Inline Error Indicators

The system SHALL display inline error messages (red text QLabel) below invalid fields when the user attempts to submit. Error messages SHALL clear when the user corrects the field.

#### Scenario: Error shown on invalid submit

- GIVEN a form dialog is open with an invalid field
- WHEN the user clicks "Guardar"
- THEN a red error label appears below the invalid field
- AND the error text describes the validation failure

#### Scenario: Error clears on correction

- GIVEN a red error label is visible below a field
- WHEN the user types valid data in that field
- THEN the red error label disappears

### Requirement: TeacherFormDialog — Validation Enhancement

The system SHALL validate all required fields in TeacherFormDialog: id (non-empty), nombre (non-empty), email (regex format if non-empty), telefono (optional), materias (optional). Current email-only validation SHALL be extended.

#### Scenario: Empty id rejected

- GIVEN id field is empty
- WHEN the user clicks "Guardar"
- THEN an error appears below the id field
- AND submission is blocked

#### Scenario: Empty nombre rejected

- GIVEN nombre field is empty
- WHEN the user clicks "Guardar"
- THEN an error appears below the nombre field
- AND submission is blocked

#### Scenario: Invalid email rejected

- GIVEN email field contains "invalido"
- WHEN the user clicks "Guardar"
- THEN an error appears below the email field
- AND submission is blocked

#### Scenario: Empty email accepted

- GIVEN email field is empty
- WHEN the user clicks "Guardar"
- THEN no email error appears
- AND submission proceeds (email is optional)

### Requirement: AulaFormDialog — Validation Rules

The system SHALL validate: nombre (non-empty), capacidad (integer >= 1). edificio and piso are optional with no validation.

#### Scenario: All valid

- GIVEN nombre="Sala 101", capacidad=30
- WHEN the user clicks "Guardar"
- THEN no errors appear
- AND the form submits

#### Scenario: Nombre empty

- GIVEN nombre=""
- WHEN the user clicks "Guardar"
- THEN an error appears below nombre
- AND submission is blocked

#### Scenario: Capacidad zero

- GIVEN capacidad=0
- WHEN the user clicks "Guardar"
- THEN an error appears below capacidad
- AND submission is blocked

### Requirement: MateriaFormDialog — Validation Rules

The system SHALL validate: nombre (non-empty). requisitos is optional with no validation.

#### Scenario: Nombre empty

- GIVEN nombre=""
- WHEN the user clicks "Guardar"
- THEN an error appears below nombre
- AND submission is blocked

#### Scenario: Nombre valid, requisitos empty

- GIVEN nombre="Fisica", requisitos=""
- WHEN the user clicks "Guardar"
- THEN no errors appear
- AND the form submits

### Requirement: Server Unavailable Feedback

The system SHALL display a QMessageBox::critical when the server (InternalClient) is unavailable during any form submission.

#### Scenario: Server down on submit

- GIVEN the IPC server is not running
- WHEN the user submits any form
- THEN a QMessageBox::critical appears with connection error message
- AND the form does NOT close

### Requirement: Keyboard Shortcuts

The system SHALL support Enter to submit and Escape to cancel in all form dialogs.

#### Scenario: Enter saves

- GIVEN a form dialog is open with valid data
- WHEN the user presses Enter
- THEN the form submits (equivalent to clicking "Guardar")

#### Scenario: Escape cancels

- GIVEN a form dialog is open
- WHEN the user presses Escape
- THEN the dialog closes without saving (equivalent to "Cancelar")
