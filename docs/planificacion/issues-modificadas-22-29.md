# Issues Modificadas — Gestor Horarios

> Documento con las issues #22 y #29 modificadas según nueva planificación.

---

## Issue #22 — [Frontend] Formulario de ingreso de aulas

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-5` |
| **Milestone** | MVP Base |
| **Dependencias** | S3-I6 (prototypes), S4-I6 (teacher form pattern) |

**Problema:**
El sistema no tiene formularios para gestionar aulas desde la interfaz gráfica. Actualmente existe un ViewPlaceholder en la posición del sidebar correspondiente a "Gestión de Aulas" (índice 2), pero no hay funcionalidad CRUD implementada. Los datos de aulas son necesarios para el solver CP-SAT y la generación de horarios.

**Solución Propuesta:**
Implementar formulario CRUD completo para aulas siguiendo el patrón establecido en `TeacherFormDialog`:

- **Formulario de entrada** (`ClassroomFormDialog`):
  - Campo `nombre` (QLineEdit, requerido, no vacío)
  - Campo `capacidad` (QSpinBox, requerido, min=1, max=500)
  - Campo `edificio` (QLineEdit, opcional)
  - Campo `piso` (QLineEdit, opcional)
  - Validación: nombre no vacío, capacidad > 0
  - Señal: `aulaGuardada(nombre, capacidad, edificio, piso)`

- **Vista de lista** (`ClassroomListWidget`):
  - QTableWidget con columnas: Nombre, Capacidad, Edificio, Piso, Acciones
  - Botones: Registrar, Editar, Eliminar
  - Panel de detalle a la derecha (opcional)
  - Confirmación antes de eliminar

- **Integración IPC** (stubs iniciales):
  - Agregar operaciones en `messages.h`: `OP_LISTA_AULAS`, `OP_CREAR_AULA`, `OP_ACTUALIZAR_AULA`, `OP_ELIMINAR_AULA`
  - Implementar handlers stub en `internalserver.cpp` (patrón similar a teachers)
  - Conexión real con backend diferida a cambio posterior

- **Integración en MainWindow**:
  - Reemplazar `ViewPlaceholder("Gestión de Aulas", this)` en índice 2 con `new ClassroomListWidget(this)`
  - Actualizar `CMakeLists.txt` con nuevos archivos

**Criterios de Aceptación:**
- [ ] Formulario de aula con campos correctos y validación
- [ ] QSpinBox para capacidad (solo acepta integers, rango 1-500)
- [ ] Tabla de aulas muestra datos correctamente
- [ ] Crear aula funciona (emite señal con datos correctos)
- [ ] Editar aula pre-carga datos existentes
- [ ] Eliminar aula muestra confirmación
- [ ] Validación: nombre requerido, capacidad > 0
- [ ] Mensajes de error claros por campo
- [ ] Consistencia visual con formulario de profesores
- [ ] MainWindow navega correctamente a vista de aulas
- [ ] Compila sin errores (CMakeLists.txt actualizado)

---

## Issue #29 — [Frontend] Formulario de ingreso de asignaturas + revisiones de validación de campos y refinamiento

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-5` |
| **Milestone** | MVP Base |
| **Dependencias** | S3-I6 (prototypes), S4-I6 (teacher form pattern), S5-I3 (subject forms) |

**Problema:**
El sistema carece de formularios para gestionar asignaturas. Existe un ViewPlaceholder en la posición del sidebar correspondiente a "Gestión de Asignaturas" (índice 3) sin funcionalidad. Además, la validación de campos en los formularios existentes es mínima — actualmente solo se valida el formato de email en `TeacherFormDialog`, sin validación de campos requeridos, formatos telefónicos, o indicadores visuales de error.

**Solución Propuesta:**

### Parte 1: Formulario de Asignaturas

Implementar formulario CRUD para asignaturas siguiendo el patrón `TeacherFormDialog`:

- **Formulario de entrada** (`SubjectFormDialog`):
  - Campo `nombre` (QLineEdit, requerido, no vacío)
  - Campo `requisitos` (QLineEdit, opcional — texto libre o comma-separated)
  - Validación: nombre no vacío
  - Señal: `materiaGuardada(nombre, requisitos)`

- **Vista de lista** (`SubjectListWidget`):
  - QTableWidget con columnas: Nombre, Requisitos, Acciones
  - Botones: Registrar, Editar, Eliminar
  - Confirmación antes de eliminar

- **Integración IPC** (stubs):
  - Agregar operaciones en `messages.h`: `OP_LISTA_MATERIAS`, `OP_CREAR_MATERIA`, `OP_ACTUALIZAR_MATERIA`, `OP_ELIMINAR_MATERIA`
  - Implementar handlers stub en `internalserver.cpp`
  - Conexión real con backend diferida

- **Integración en MainWindow**:
  - Reemplazar `ViewPlaceholder("Gestión de Asignaturas", this)` en índice 3 con `new SubjectListWidget(this)`
  - Actualizar `CMakeLists.txt`

### Parte 2: Refinamiento de Validación

Mejorar la validación en todos los formularios existentes y nuevos:

- **TeacherFormDialog** (mejoras):
  - Validar campo `id` (requerido, no vacío)
  - Validar campo `nombre` (requerido, no vacío)
  - Validar campo `email` (requerido + regex existente)
  - Validar campo `telefono` (opcional, pero si presente validar formato)
  - Agregar indicadores visuales de error (labels inline o cambio de color en borde)

- **ClassroomFormDialog** (nuevo):
  - Validar `nombre` (requerido, no vacío)
  - Validar `capacidad` (requerido, > 0 via QSpinBox)

- **SubjectFormDialog** (nuevo):
  - Validar `nombre` (requerido, no vacío)

- **Patrón de validación unificado**:
  - Mostrar QMessageBox con error específico por campo al intentar guardar
  - Considerar indicadores visuales (borde rojo, texto de error inline)
  - Validación antes de emitir señal de guardado
  - Prevenir envío de datos inválidos al middleware

**Criterios de Aceptación:**

*Formulario de Asignaturas:*
- [ ] Formulario de asignatura con campos correctos y validación
- [ ] Tabla de asignaturas muestra datos correctamente
- [ ] Crear asignatura funciona (emite señal con datos correctos)
- [ ] Editar asignatura pre-carga datos existentes
- [ ] Eliminar asignatura muestra confirmación
- [ ] Validación: nombre requerido
- [ ] Mensajes de error claros por campo
- [ ] Consistencia visual con otros formularios
- [ ] MainWindow navega correctamente a vista de asignaturas
- [ ] Compila sin errores

*Refinamiento de Validación:*
- [ ] TeacherFormDialog valida todos los campos requeridos (id, nombre, email)
- [ ] TeacherFormDialog valida formato de teléfono si está presente
- [ ] Todos los formularios muestran errores específicos por campo
- [ ] Indicadores visuales de error implementados (mínimo QMessageBox, ideal inline)
- [ ] No se permiten enviar datos con campos requeridos vacíos
- [ ] Validación consistente en todos los formularios (teacher, classroom, subject)

---

## Notas de Implementación

### Patrón a Seguir
Todos los nuevos formularios deben replicar la arquitectura de `TeacherFormDialog`:
- Hereda de `QDialog`
- Usa `QFormLayout` para campos
- `QRegularExpressionValidator` para formatos (donde aplique)
- Estilos inline consistentes con el proyecto
- Señales Qt para comunicación con la vista de lista

### Archivos Afectados
```
src/frontend/src/forms/
├── teacher_form_dialog.hpp/.cpp    (referencia - solo mejoras de validación)
├── classroom_form_dialog.hpp/.cpp  (NUEVO)
└── subject_form_dialog.hpp/.cpp    (NUEVO)

src/frontend/src/views/
├── teacher_list_widget.hpp/.cpp    (referencia)
├── classroom_list_widget.hpp/.cpp  (NUEVO)
└── subject_list_widget.hpp/.cpp    (NUEVO)

src/middleware/include/middleware/messages.h  (agregar op codes)
src/middleware/src/server/internalserver.cpp  (agregar handlers stub)
src/frontend/CMakeLists.txt                  (agregar nuevos archivos)
src/frontend/src/views/main_window.cpp       (reemplazar ViewPlaceholders)
```

### Orden de Implementación Recomendado
1. Crear `ClassroomFormDialog` + `ClassroomListWidget`
2. Integrar en MainWindow + CMakeLists
3. Crear `SubjectFormDialog` + `SubjectListWidget`
4. Integrar en MainWindow + CMakeLists
5. Agregar op codes en `messages.h`
6. Implementar handlers stub en `internalserver.cpp`
7. Refinar validación en `TeacherFormDialog`
8. Agregar indicadores visuales de error en todos los formularios
