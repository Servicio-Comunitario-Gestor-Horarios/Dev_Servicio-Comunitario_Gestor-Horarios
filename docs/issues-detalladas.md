# Issues Detalladas — Gestor Horarios

> Documento de referencia con todas las issues planificadas por sprint.
> Cada issue sigue la estructura del template `feature_request.yml`.
> **No crear en GitHub** — usar solo como guía para la asignación semanal.

---

## Milestones

| Milestone | Sprints | Objetivo |
|-----------|---------|----------|
| **MVP Base** | 2–5 | Login, CRUD profesores/aulas, generación básica de horarios |
| **Beta** | 6–9 | Validación restricciones, exportación, dashboard, optimización |
| **Release 1.0** | 10–13 | Carga masiva, reportes, testing completo, documentación y deploy |

---

## Sprint 2 — MVP Base

### S2-I1 — [Backend] OR-Tools + Estructuras de datos

| Campo | Valor |
|-------|-------|
| **Asignado** | Luis |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-2` |
| **Milestone** | MVP Base |
| **Dependencias** | Ninguna |

**Problema:**
El proyecto no tiene OR-Tools integrado ni estructuras para representar horarios. No se puede modelar ni ejecutar el solver CP-SAT.

**Solución Propuesta:**
Integrar OR-Tools vía CMake FetchContent. Definir en `backend/include/data/`:
- `Schedule`: contenedor del horario generado
- `TimeSlot`: franja horaria (día, hora inicio, hora fin)
- `Subject`: materia (nombre, horas semanales, requisitos)
- `Teacher`: profesor (disponibilidad, materias)
- `Classroom`: aula (capacidad, ubicación)

Todas las structs con serialización JSON (`QJsonObject`/`QJsonDocument`) para middleware.

**Criterios de Aceptación:**
- [ ] OR-Tools compila y linkea correctamente vía CMake
- [ ] Test unitario que llama a `CpSolver` y retorna resultado válido
- [ ] `Schedule`, `TimeSlot`, `Subject`, `Teacher`, `Classroom` definidas en `backend/include/data/`
- [ ] Cada estructura tiene métodos `toJson()`/`fromJson()`
- [ ] Test que crea un Schedule, lo serializa y deserializa correctamente

**Contexto Técnico:**
- Módulos: `backend/` (solver), `middleware/common/` (tipos compartidos)
- OR-Tools vía `FetchContent_Declare(ortools ...)`
- Serialización JSON para IPC con Qt Local Sockets
- Referencia: `docs/arquitectura-comunicacion-frontend-backend.md`

---

### S2-I2 — [Backend] CRUD de profesores vía middleware

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-2` |
| **Milestone** | MVP Base |
| **Dependencias** | S2-I1 (estructuras), S2-I5 (SQLite) |

**Problema:**
No existen endpoints para gestionar profesores. El sistema necesita CRUD completo para asignarlos a materias y horarios.

**Solución Propuesta:**
Endpoints REST vía middleware:
- `GET /teachers` — listar todos
- `GET /teachers/{id}` — obtener por ID
- `POST /teachers` — crear
- `PUT /teachers/{id}` — actualizar
- `DELETE /teachers/{id}` — eliminar

Usar estructuras `Teacher` (S2-I1) y persistencia SQLite (S2-I5).

**Criterios de Aceptación:**
- [ ] `GET /teachers` retorna array JSON (vacío si no hay datos)
- [ ] `GET /teachers/{id}` retorna profesor o 404
- [ ] `POST /teachers` valida campos requeridos (name, email) → 400 si inválido
- [ ] `PUT /teachers/{id}` actualiza y retorna el registro actualizado
- [ ] `DELETE /teachers/{id}` retorna 204 o 404
- [ ] Datos persisten en SQLite correctamente
- [ ] Comunicación con frontend vía middleware (Qt Local Sockets + JSON)

**Contexto Técnico:**
- `backend/` (servicios), `middleware/server/` (enrutamiento), `middleware/common/` (tipos)
- JSON serializado entre frontend ↔ middleware ↔ backend
- Depende de S2-I1 (estructura `Teacher`) y S2-I5 (esquema SQLite)

---

### S2-I3 — [Frontend] Maqueta de login y dashboard

| Campo | Valor |
|-------|-------|
| **Asignado** | Paola |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-2` |
| **Milestone** | MVP Base |
| **Dependencias** | Ninguna |

**Problema:**
No existe interfaz gráfica. La app es headless, impidiendo validar visualmente cualquier funcionalidad.

**Solución Propuesta:**
Maqueta Qt6 Widgets:
- `LoginDialog` (QDialog): usuario/contraseña con validación no-vacío. Sin auth real — cualquier input válido abre el dashboard.
- `MainWindow` (QMainWindow): layout con `QStackedWidget` para vistas futuras.
- Sin conexión a backend — standalone.

**Criterios de Aceptación:**
- [ ] App inicia mostrando LoginDialog
- [ ] LoginDialog valida que usuario y contraseña no estén vacíos
- [ ] "Iniciar sesión" con campos válidos abre MainWindow
- [ ] MainWindow tiene layout preparado con QStackedWidget
- [ ] App se cierra correctamente (sin procesos colgados)

**Contexto Técnico:**
- Módulo: `frontend/`. Qt6 Widgets: `QDialog`, `QMainWindow`, `QStackedWidget`, `QLineEdit`, `QPushButton`, `QLabel`
- Mock UI — sin backend

---

### S2-I4 — [Middleware] Setup de middleware + health-check

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-middleware/qa |
| **Labels** | `enhancement`, `status:needs-review`, `area-middleware/qa`, `sprint-2` |
| **Milestone** | MVP Base |
| **Dependencias** | Ninguna (paralelizable con S2-I1 y S2-I3) |

**Problema:**
No existe middleware que conecte frontend con backend. No hay IPC, logging, ni forma de verificar que el backend esté vivo.

**Solución Propuesta:**
Estructura del middleware:
- `middleware/server/`: servidor IPC (QLocalServer)
- `middleware/client/`: cliente IPC (QLocalSocket)
- `middleware/common/`: tipos compartidos (mensajes JSON, códigos de operación)

Ruta health-check: frontend envía `{"op":"health-check"}` → middleware reenvía → backend responde `{"status":"ok"}` → respuesta al frontend.

Logging básico de conexiones (timestamp, dirección, operación).

**Criterios de Aceptación:**
- [ ] Middleware compila independientemente (CMakeLists.txt propio)
- [ ] Health-check funciona end-to-end: frontend → middleware → backend → respuesta
- [ ] Logging muestra timestamp y operación de cada conexión
- [ ] Test unitario de health-check
- [ ] Estructura `middleware/{server,client,common}/` creada

**Contexto Técnico:**
- Qt Local Sockets (QLocalServer/QLocalSocket) + JSON
- Referencia: `docs/arquitectura-comunicacion-frontend-backend.md`

---

### S2-I5 — [Backend] Esquema SQLite profesores + aulas

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-2` |
| **Milestone** | MVP Base |
| **Dependencias** | S2-I1 (coordinar campos exactos) |

**Problema:**
No hay esquema de base de datos. El backend no puede persistir datos.

**Solución Propuesta:**
SQLite con Qt6::Sql.

Tabla `teachers`:
| Columna | Tipo | Restricciones |
|---------|------|--------------|
| id | INTEGER | PRIMARY KEY AUTOINCREMENT |
| name | TEXT | NOT NULL |
| email | TEXT | NOT NULL UNIQUE |
| phone | TEXT | |
| created_at | TEXT | NOT NULL DEFAULT CURRENT_TIMESTAMP |
| updated_at | TEXT | NOT NULL DEFAULT CURRENT_TIMESTAMP |

Tabla `classrooms`:
| Columna | Tipo | Restricciones |
|---------|------|--------------|
| id | INTEGER | PRIMARY KEY AUTOINCREMENT |
| name | TEXT | NOT NULL UNIQUE |
| capacity | INTEGER | NOT NULL |
| building | TEXT | |
| floor | INTEGER | |
| created_at | TEXT | NOT NULL DEFAULT CURRENT_TIMESTAMP |
| updated_at | TEXT | NOT NULL DEFAULT CURRENT_TIMESTAMP |

Migración idempotente (CREATE TABLE IF NOT EXISTS) al iniciar backend.

**Criterios de Aceptación:**
- [ ] Tablas `teachers` y `classrooms` se crean automáticamente al iniciar
- [ ] Columnas, tipos y restricciones correctos
- [ ] Conexión Qt6::Sql funciona (QSqlDatabase abierta)
- [ ] Posible insertar/leer/actualizar/eliminar vía QSqlQuery
- [ ] Test unitario de creación de tablas
- [ ] Seed data opcional (2-3 registros de ejemplo)

**Contexto Técnico:**
- `backend/src/database/`, Qt6::Sql (`QSqlDatabase`, `QSqlQuery`, `QSqlError`)
- Migración idempotente

---

## Sprint 3 — MVP Base

### S3-I1 — [Backend] Algoritmo de asignación básico (CP-SAT)

| Campo | Valor |
|-------|-------|
| **Asignado** | Luis |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-3` |
| **Milestone** | MVP Base |
| **Dependencias** | S2-I1 (OR-Tools configurado) |

**Problema:**
OR-Tools está integrado pero no hay un algoritmo que resuelva la asignación de horarios. No se puede generar un horario automáticamente.

**Solución Propuesta:**
Implementar el modelo CP-SAT básico:
- Variables: asignación materia→profesor→aula→franja
- Restricciones: un profesor no puede estar en dos lugares, un aula no puede tener dos materias simultáneas
- Función objetivo: maximizar ocupación de aulas y preferencias
- Salida: Schedule con asignaciones resueltas

**Criterios de Aceptación:**
- [ ] El solver encuentra solución para 5 profesores, 3 aulas, 10 materias en <30s
- [ ] Restricciones básicas se cumplen (sin choques horarios)
- [ ] La solución se serializa a JSON correctamente
- [ ] Test unitario con caso conocido valida el resultado

**Contexto Técnico:**
- `backend/src/solver/` — modelo CP-SAT
- Usa estructuras de S2-I1
- Comunicación vía middleware (cuando exista)

---

### S3-I2 — [Backend] CRUD de materias

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-3` |
| **Milestone** | MVP Base |
| **Dependencias** | S2-I5 (esquema SQLite) |

**Problema:**
No hay endpoints para gestionar materias. El solver necesita materias como input.

**Solución Propuesta:**
CRUD completo para `subjects`:
- `GET/POST/PUT/DELETE /subjects`
- Campos: name, weekly_hours, required_classroom_type, department
- Persistencia en SQLite + serialización JSON

**Criterios de Aceptación:**
- [ ] CRUD completo funcional con validaciones
- [ ] Persistencia en SQLite
- [ ] Serialización JSON middleware-compatible

**Contexto Técnico:**
- Mismo patrón que CRUD profesores (S2-I2)
- Tabla SQLite `subjects` (Nicole extiende esquema)

---

### S3-I3 — [Frontend] Formulario de registro de profesores

| Campo | Valor |
|-------|-------|
| **Asignado** | Paola |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-3` |
| **Milestone** | MVP Base |
| **Dependencias** | S2-I3 (maqueta login), S2-I2 (CRUD profesores) |

**Problema:**
No hay forma de ingresar profesores desde la UI. La maqueta existe pero no tiene funcionalidad.

**Solución Propuesta:**
Formulario de registro/edición de profesores:
- Campos: nombre, email, teléfono, materias que dicta
- Validación de campos
- Conexión al backend vía middleware para CRUD real
- Lista de profesores existentes con opciones editar/eliminar

**Criterios de Aceptación:**
- [ ] Formulario muestra campos correctos y valida entrada
- [ ] Crear profesor persiste vía middleware → backend → SQLite
- [ ] Lista de profesores se actualiza después de crear/editar/eliminar
- [ ] Manejo de errores (servidor caído, validación fallida)

**Contexto Técnico:**
- `frontend/` — Qt6 Widgets: QFormLayout, QTableView, QPushButton
- Comunicación vía `middleware/client/backend_client.hpp`
- Depende de S2-I2 (endpoints) y S2-I4 (middleware)

---

### S3-I4 — [Middleware] Endpoints CRUD desde frontend hacia backend

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-middleware/qa |
| **Labels** | `enhancement`, `status:needs-review`, `area-middleware/qa`, `sprint-3` |
| **Milestone** | MVP Base |
| **Dependencias** | S2-I2/S3-I2 (CRUDs backend) |

**Problema:**
El middleware existe (health-check) pero no enruta peticiones CRUD. Frontend no puede comunicarse con backend para operaciones de datos.

**Solución Propuesta:**
Routing de mensajes CRUD en el middleware:
- Identificar operación por campo `"op"` en JSON
- Encaminar al servicio backend correspondiente
- Devolver respuesta al frontend
- Manejo de errores (timeout, backend caído)

**Criterios de Aceptación:**
- [ ] Middleware enruta peticiones CRUD profesores/materias
- [ ] Frontend recibe respuestas correctas
- [ ] Timeout de 5s con mensaje de error si backend no responde

**Contexto Técnico:**
- `middleware/server/api_handler.hpp` — routing por op
- `middleware/common/messages.hpp` — tipos de mensajes

---

### S3-I5 — [Backend] CRUD de aulas

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-3` |
| **Milestone** | MVP Base |
| **Dependencias** | S2-I5 (esquema SQLite) |

**Problema:**
No hay endpoints para gestionar aulas. El solver necesita datos de aulas como input.

**Solución Propuesta:**
CRUD completo para `classrooms`:
- `GET/POST/PUT/DELETE /classrooms`
- Campos: name, capacity, building, floor, resources
- Persistencia SQLite + JSON

**Criterios de Aceptación:**
- [ ] CRUD funcional con validaciones
- [ ] Tabla `classrooms` operativa
- [ ] Compatible con middleware

**Contexto Técnico:**
- Mismo patrón que S2-I2, tabla ya creada en S2-I5

---

## Sprint 4 — MVP Base

### S4-I1 — [Backend] Restricciones de horario (aulas, turnos)

| Campo | Valor |
|-------|-------|
| **Asignado** | Luis |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-4` |
| **Milestone** | MVP Base |
| **Dependencias** | S3-I1 (algoritmo básico) |

**Problema:**
El algoritmo básico no maneja restricciones reales como disponibilidad de aulas por turno, capacidad mínima, o restricciones de horario.

**Solución Propuesta:**
Agregar al modelo CP-SAT:
- Restricciones de capacidad: materia X requiere aula con capacidad ≥ Y
- Restricciones de turno: ciertas materias solo en turno mañana/tarde
- Restricciones de aula: aula específica para materia específica
- Tipos de turno: mañana (7-12), tarde (13-18), noche (18-22)

**Criterios de Aceptación:**
- [ ] Solución respeta capacidad de aulas
- [ ] Solución respeta turnos asignados
- [ ] Si no hay solución posible, solver retorna "infeasible" con reporte
- [ ] Test con restricciones conocidas valida cumplimiento

**Contexto Técnico:**
- `backend/src/solver/constraints/` — módulo de restricciones
- Extiende modelo CP-SAT de S3-I1

---

### S4-I2 — [Backend] CRUD horarios (asignación manual)

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-4` |
| **Milestone** | MVP Base |
| **Dependencias** | S3-I2 (CRUD materias), S3-I5 (CRUD aulas) |

**Problema:**
No hay forma de crear/editar horarios manualmente como alternativa a la generación automática.

**Solución Propuesta:**
CRUD de horarios con asignaciones manuales:
- `GET/POST/PUT/DELETE /schedules`
- Asignar profesor + materia + aula + franja manualmente
- Validar que no haya conflictos con asignaciones existentes

**Criterios de Aceptación:**
- [ ] CRUD horarios funcional
- [ ] Validación de conflictos: mismo profesor en dos lugares
- [ ] Validación de conflictos: misma aula dos veces
- [ ] Datos en SQLite

**Contexto Técnico:**
- `backend/src/services/schedule_service.hpp`
- Tabla SQLite `schedule_entries` (extensión del esquema)

---

### S4-I3 — [Frontend] Tabla de visualización de horarios

| Campo | Valor |
|-------|-------|
| **Asignado** | Paola |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-4` |
| **Milestone** | MVP Base |
| **Dependencias** | S4-I2 (CRUD horarios) |

**Problema:**
No hay forma de ver los horarios generados en la UI.

**Solución Propuesta:**
Tabla de horarios con QTableView:
- Columnas: hora, lunes a viernes
- Celdas: materia, profesor, aula
- Código de colores por profesor o materia
- Scroll y redimensionamiento

**Criterios de Aceptación:**
- [ ] Tabla muestra horarios desde backend vía middleware
- [ ] Columnas por día, filas por franja horaria
- [ ] Código de colores por materia
- [ ] Scroll horizontal/vertical
- [ ] Actualización al cambiar datos

**Contexto Técnico:**
- `frontend/` — QTableView con modelo personalizado
- Datos vía middleware → S4-I2

---

### S4-I4 — [Middleware] Validación de datos en middleware

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-middleware/qa |
| **Labels** | `enhancement`, `status:needs-review`, `area-middleware/qa`, `sprint-4` |
| **Milestone** | MVP Base |
| **Dependencias** | S3-I4 (endpoints CRUD) |

**Problema:**
Actualmente el middleware pasa datos sin validar entre frontend y backend. Datos malformados pueden causar crashes.

**Solución Propuesta:**
Capa de validación en middleware:
- Validar JSON de entrada (campos requeridos, tipos)
- Rechazar peticiones malformadas con error descriptivo
- Sanitizar strings (longitud máxima, caracteres permitidos)
- Tests unitarios de validación

**Criterios de Aceptación:**
- [ ] Middleware rechaza JSON inválido con error 400
- [ ] Middleware rechaza campos faltantes con mensaje claro
- [ ] Tests unitarios cubren casos borde

**Contexto Técnico:**
- `middleware/server/validator.hpp`
- Validación antes de routing

---

### S4-I5 — [Backend] Servicio de consultas de horarios

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-4` |
| **Milestone** | MVP Base |
| **Dependencias** | S4-I2 (CRUD horarios) |

**Problema:**
No hay una capa de servicio que abstraiga las consultas de horarios del frontend.

**Solución Propuesta:**
Servicio de consultas:
- `GET /schedules/teacher/{id}` — horario de un profesor
- `GET /schedules/classroom/{id}` — ocupación de un aula
- `GET /schedules/day/{day}` — horario de un día específico
- Formato JSON optimizado para consumo frontend

**Criterios de Aceptación:**
- [ ] Consultas por profesor devuelven horario semanal
- [ ] Consultas por aula devuelven ocupación
- [ ] Resultados en JSON listo para UI

**Contexto Técnico:**
- `backend/src/services/query_service.hpp`
- Consultas SQL optimizadas

---

## Sprint 5 — MVP Base

### S5-I1 — [Backend] Exportar solución a JSON/CSV

| Campo | Valor |
|-------|-------|
| **Asignado** | Luis |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-5` |
| **Milestone** | MVP Base |
| **Dependencias** | S4-I1 (restricciones) |

**Problema:**
La solución generada solo existe en memoria y no puede exportarse para uso externo.

**Solución Propuesta:**
Serializar Schedule a:
- JSON: estructura completa del horario
- CSV: filas por asignación (profesor, materia, aula, día, hora)
- Archivos descargables desde el frontend

**Criterios de Aceptación:**
- [ ] Exportación a JSON con estructura completa
- [ ] Exportación a CSV legible en Excel/Google Sheets
- [ ] Archivos se generan en backend y se sirven al frontend

**Contexto Técnico:**
- `backend/src/export/` — módulo de exportación
- Formato CSV con BOM para Excel

---

### S5-I2 — [Backend] Endpoints de generación automática

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-5` |
| **Milestone** | MVP Base |
| **Dependencias** | S4-I1 (restricciones) |

**Problema:**
No hay un endpoint que dispare la generación automática de horarios.

**Solución Propuesta:**
- `POST /schedules/generate` — dispara el solver con datos actuales
- `GET /schedules/generate/status` — estado de la generación
- Parámetros: tipo de optimización (velocidad vs calidad)

**Criterios de Aceptación:**
- [ ] Endpoint dispara el solver y devuelve resultado
- [ ] Endpoint de estado para generaciones largas
- [ ] Timeout configurable

**Contexto Técnico:**
- `backend/src/services/generation_service.hpp`
- Llama al solver de S4-I1

---

### S5-I3 — [Frontend] Vista de generación y exportación

| Campo | Valor |
|-------|-------|
| **Asignado** | Paola |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-5` |
| **Milestone** | MVP Base |
| **Dependencias** | S5-I1 (exportación) |

**Problema:**
No hay interfaz para iniciar la generación ni descargar resultados.

**Solución Propuesta:**
Vista de generación:
- Botón "Generar Horario" que llama a `POST /schedules/generate`
- Barra de progreso / spinner mientras el solver trabaja
- Botones "Exportar JSON" y "Exportar CSV" cuando la solución está lista
- Vista previa del resultado antes de exportar

**Criterios de Aceptación:**
- [ ] Botón "Generar Horario" dispara el proceso
- [ ] Feedback visual durante la generación
- [ ] Botones de exportación descargan archivos
- [ ] Vista previa del horario generado

**Contexto Técnico:**
- `frontend/` — QProgressBar, QPushButton, QFileDialog
- Comunicación con S5-I2 y S5-I1

---

### S5-I4 — [Middleware] Manejo de errores y respuestas HTTP estándar

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-middleware/qa |
| **Labels** | `enhancement`, `status:needs-review`, `area-middleware/qa`, `sprint-5` |
| **Milestone** | MVP Base |
| **Dependencias** | S4-I4 (validación) |

**Problema:**
Las respuestas de error del middleware son inconsistentes (a veces texto plano, a veces JSON, a veces crash).

**Solución Propuesta:**
Formato estándar de respuestas:
```json
{
  "success": true/false,
  "data": { ... },
  "error": { "code": "ERROR_CODE", "message": "Descripción" }
}
```
Códigos de error estandarizados y logging de errores.

**Criterios de Aceptación:**
- [ ] Todas las respuestas siguen el formato estándar
- [ ] Códigos de error consistentes
- [ ] Logging de errores con stack trace

**Contexto Técnico:**
- `middleware/common/response.hpp` — formato estándar
- Refactor de respuestas existentes

---

### S5-I5 — [Backend] Exportación de horarios a JSON/CSV (backend)

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-5` |
| **Milestone** | MVP Base |
| **Dependencias** | S4-I2 (CRUD horarios) |

**Problema:**
La exportación desde el solver (S5-I1) solo cubre horarios generados automáticamente. No hay exportación de horarios creados manualmente.

**Solución Propuesta:**
Endpoint de exportación para horarios manuales:
- `GET /schedules/{id}/export?format=json|csv`
- Mismo formato que S5-I1 pero desde datos persistidos

**Criterios de Aceptación:**
- [ ] Exportación de horarios manuales a JSON y CSV
- [ ] Mismo formato que exportación automática

**Contexto Técnico:**
- `backend/src/services/export_service.hpp`
- Reusa serializadores de S5-I1

---

## Sprint 6 — Beta

### S6-I1 — [Backend] Validación de restricciones duplicadas y conflictos

| Campo | Valor |
|-------|-------|
| **Asignado** | Luis |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-6` |
| **Milestone** | Beta |
| **Dependencias** | S5-I1 (exportación) |

**Problema:**
Pueden existir restricciones duplicadas o contradictorias que el solver no detecta hasta que falla.

**Solución Propuesta:**
Validador de restricciones pre-solver:
- Detectar restricciones duplicadas
- Detectar restricciones contradictorias (ej: materia X debe estar en aula A y aula B a la vez)
- Reporte de conflictos antes de ejecutar solver

**Criterios de Aceptación:**
- [ ] Validador detecta restricciones duplicadas
- [ ] Validador detecta contradicciones lógicas
- [ ] Reporte claro al usuario antes de ejecutar solver

**Contexto Técnico:**
- `backend/src/solver/validator.hpp`
- Se ejecuta antes del CP-SAT

---

### S6-I2 — [Backend] CRUD de disponibilidad de profesores

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-6` |
| **Milestone** | Beta |
| **Dependencias** | S4-I2 (CRUD horarios) |

**Problema:**
No hay forma de registrar la disponibilidad horaria de cada profesor (días y horas que puede trabajar).

**Solución Propuesta:**
CRUD de disponibilidad:
- `GET/POST/PUT/DELETE /teachers/{id}/availability`
- Formato: matriz día×franja (booleano)
- Validar que no haya conflictos con asignaciones existentes

**Criterios de Aceptación:**
- [ ] CRUD disponibilidad funcional por profesor
- [ ] Formato matriz día×franja
- [ ] Validación contra horarios existentes

**Contexto Técnico:**
- Tabla SQLite `teacher_availability`
- Integración con solver en S6-I5

---

### S6-I3 — [Frontend] Calendario interactivo por profesor

| Campo | Valor |
|-------|-------|
| **Asignado** | Paola |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-6` |
| **Milestone** | Beta |
| **Dependencias** | S6-I2 (disponibilidad) |

**Problema:**
No hay interfaz visual para ver/editar la disponibilidad de profesores.

**Solución Propuesta:**
Calendario semanal interactivo:
- Grilla día×franja con celdas cliqueables
- Verde = disponible, rojo = no disponible
- Persistir cambios vía middleware → S6-I2

**Criterios de Aceptación:**
- [ ] Grilla día×franja interactiva
- [ ] Click cambia estado disponible/no disponible
- [ ] Persistencia automática o con botón guardar

**Contexto Técnico:**
- `frontend/` — QTableWidget con colores condicionales
- Datos vía middleware

---

### S6-I4 — [Middleware] Caché de consultas frecuentes

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-middleware/qa |
| **Labels** | `enhancement`, `status:needs-review`, `area-middleware/qa`, `sprint-6` |
| **Milestone** | Beta |
| **Dependencias** | S5-I4 (respuestas estándar) |

**Problema:**
Consultas repetitivas (lista de profesores, materias) viajan al backend cada vez, generando latencia innecesaria.

**Solución Propuesta:**
Caché LRU en middleware:
- Cachear respuestas GET con TTL configurable
- Invalidar caché en operaciones POST/PUT/DELETE
- Timestamp de última actualización

**Criterios de Aceptación:**
- [ ] Caché funciona para GETs repetitivos
- [ ] Caché se invalida al modificar datos
- [ ] TTL configurable por tipo de consulta

**Contexto Técnico:**
- `middleware/server/cache.hpp` — LRU cache thread-safe
- Usar `std::unordered_map` + `std::list`

---

### S6-I5 — [Backend] Validación de conflictos de disponibilidad

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-6` |
| **Milestone** | Beta |
| **Dependencias** | S6-I2 (CRUD disponibilidad) |

**Problema:**
El solver no considera disponibilidad de profesores, pudiendo asignar horarios en los que el profesor no está disponible.

**Solución Propuesta:**
Integrar disponibilidad como restricción en el solver:
- Validar que asignaciones respeten disponibilidad
- Reportar conflictos: "El profesor X no está disponible el lunes a las 8am"

**Criterios de Aceptación:**
- [ ] Solver respeta disponibilidad de profesores
- [ ] Reporte de conflictos claro
- [ ] Test con caso de conflicto conocido

**Contexto Técnico:**
- Extiende solver de S4-I1
- Datos de S6-I2

---

## Sprint 7 — Beta

### S7-I1 — [Backend] Optimización intermedia (múltiples soluciones)

| Campo | Valor |
|-------|-------|
| **Asignado** | Luis |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-7` |
| **Milestone** | Beta |
| **Dependencias** | S6-I1 (validación) |

**Problema:**
El solver genera una sola solución. No hay forma de comparar alternativas.

**Solución Propuesta:**
Modo multi-solución en CP-SAT:
- Generar N soluciones (top N según función objetivo)
- Almacenar cada solución con métricas
- Endpoint para listar soluciones disponibles

**Criterios de Aceptación:**
- [ ] Solver genera múltiples soluciones
- [ ] Cada solución tiene métricas (ocupación, conflictos, etc.)
- [ ] Endpoint lista soluciones disponibles

**Contexto Técnico:**
- CP-SAT `SearchForAllSolutions` o similar
- `backend/src/solver/multi_solution.hpp`

---

### S7-I2 — [Backend] Endpoint para comparar soluciones

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-7` |
| **Milestone** | Beta |
| **Dependencias** | S7-I1 (múltiples soluciones) |

**Problema:**
No hay un endpoint que compare dos soluciones lado a lado.

**Solución Propuesta:**
- `GET /schedules/compare?id1=X&id2=Y` — diff estructurado
- Diferencias: asignaciones cambiadas, métricas por aula/profesor
- Formato JSON listo para UI

**Criterios de Aceptación:**
- [ ] Endpoint compara dos soluciones
- [ ] Diferencias claras por asignación
- [ ] Métricas comparativas

**Contexto Técnico:**
- `backend/src/services/compare_service.hpp`

---

### S7-I3 — [Frontend] Dashboard con métricas de horarios

| Campo | Valor |
|-------|-------|
| **Asignado** | Paola |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-7` |
| **Milestone** | Beta |
| **Dependencias** | S7-I2 (comparación) |

**Problema:**
El dashboard actual es un placeholder sin métricas reales.

**Solución Propuesta:**
Dashboard con:
- Tarjetas: total profesores, aulas, materias, horarios generados
- Gráfico de ocupación de aulas (porcentaje)
- Tabla de profesores con carga horaria
- Selector de solución activa (si hay múltiples)

**Criterios de Aceptación:**
- [ ] Tarjetas con métricas desde backend
- [ ] Gráfico de ocupación de aulas
- [ ] Tabla de carga horaria por profesor
- [ ] Selector de solución si hay múltiples

**Contexto Técnico:**
- `frontend/` — QWidget personalizados para gráficos simples
- Datos vía middleware desde S7-I2

---

### S7-I4 — [Middleware] Logging estructurado

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-middleware/qa |
| **Labels** | `enhancement`, `status:needs-review`, `area-middleware/qa`, `sprint-7` |
| **Milestone** | Beta |
| **Dependencias** | Ninguna |

**Problema:**
El logging actual es básico (solo prints). No hay formato estructurado para debugging.

**Solución Propuesta:**
Logging JSON estructurado:
```json
{"timestamp":"...", "level":"INFO", "component":"server", "message":"...", "request_id":"..."}
```
Niveles: DEBUG, INFO, WARN, ERROR. Archivos rotativos con tamaño máximo.

**Criterios de Aceptación:**
- [ ] Logs en formato JSON
- [ ] Niveles de logging configurables
- [ ] Archivos rotativos (max 10MB, mantener 5)
- [ ] Cada request tiene ID único para trazabilidad

**Contexto Técnico:**
- `middleware/server/logger.hpp` — wrapper sobre spdlog o QDebug

---

### S7-I5 — [Backend] Comparador de soluciones

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-7` |
| **Milestone** | Beta |
| **Dependencias** | S7-I1 (optimización) |

**Problema:**
No hay lógica backend para comparar soluciones más allá del endpoint básico.

**Solución Propuesta:**
Servicio de comparación detallado:
- Diferencias por profesor: qué cambió entre solución A y B
- Diferencias por aula: ocupación distinta
- Score de cada solución basado en weighted sum de métricas

**Criterios de Aceptación:**
- [ ] Comparación detallada por profesor
- [ ] Comparación por aula
- [ ] Score numérico por solución

**Contexto Técnico:**
- `backend/src/services/compare_service.hpp`
- Algoritmo de diff estructurado

---

## Sprint 8 — Beta

### S8-I1 — [Backend] Algoritmo de reasignación rápida

| Campo | Valor |
|-------|-------|
| **Asignado** | Luis |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-8` |
| **Milestone** | Beta |
| **Dependencias** | S7-I1 (optimización) |

**Problema:**
Si un profesor cambia disponibilidad, hay que regenerar el horario completo desde cero.

**Solución Propuesta:**
Algoritmo de reasignación parcial:
- Detectar qué asignaciones se ven afectadas por el cambio
- Reasignar solo las afectadas (no regenerar todo)
- Heurística greedy para cambios rápidos

**Criterios de Aceptación:**
- [ ] Reasignación parcial funciona en <5s para cambios pequeños
- [ ] Solo modifica asignaciones afectadas
- [ ] No rompe asignaciones no afectadas

**Contexto Técnico:**
- `backend/src/solver/quick_reassign.hpp`
- Algoritmo greedy con validación post-cambio

---

### S8-I2 — [Backend] Endpoints de edición masiva

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-8` |
| **Milestone** | Beta |
| **Dependencias** | Ninguna (usa CRUDs existentes) |

**Problema:**
Editar un profesor, materia o aula es individual. No hay forma de hacer cambios masivos.

**Solución Propuesta:**
- `POST /batch/teachers` — crear/actualizar múltiples profesores
- `POST /batch/classrooms` — crear/actualizar múltiples aulas
- Formato: array de objetos con operación (create/update/delete)

**Criterios de Aceptación:**
- [ ] Edición masiva para profesores
- [ ] Edición masiva para aulas
- [ ] Transaccional: todo o nada
- [ ] Reporte de errores por ítem

**Contexto Técnico:**
- `backend/src/services/batch_service.hpp`
- SQLite transactions

---

### S8-I3 — [Frontend] Filtros y búsqueda en tabla de horarios

| Campo | Valor |
|-------|-------|
| **Asignado** | Paola |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-8` |
| **Milestone** | Beta |
| **Dependencias** | S4-I3 (tabla de horarios) |

**Problema:**
La tabla de horarios tiene muchos datos y no hay forma de filtrar o buscar.

**Solución Propuesta:**
Barra de filtros:
- Por profesor (dropdown)
- Por materia (dropdown)
- Por aula (dropdown)
- Búsqueda por texto libre
- Filtros combinables

**Criterios de Aceptación:**
- [ ] Filtro por profesor funciona
- [ ] Filtro por materia funciona
- [ ] Filtro por aula funciona
- [ ] Búsqueda por texto en materia/profesor
- [ ] Filtros combinables

**Contexto Técnico:**
- `frontend/` — QSortFilterProxyModel sobre QTableView

---

### S8-I4 — [Middleware] Autenticación JWT en middleware

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-middleware/qa |
| **Labels** | `enhancement`, `status:needs-review`, `area-middleware/qa`, `sprint-8` |
| **Milestone** | Beta |
| **Dependencias** | Ninguna |

**Problema:**
No hay autenticación. Cualquiera con acceso al named pipe puede hacer peticiones.

**Solución Propuesta:**
JWT en middleware:
- Login: valida credenciales → emite JWT
- Middleware verifica JWT en cada request
- Roles: admin, editor, viewer
- Renovación automática de tokens

**Criterios de Aceptación:**
- [ ] Login emite JWT válido
- [ ] Middleware rechaza requests sin token
- [ ] Middleware rechaza token inválido/expirado
- [ ] Roles funcionan (admin puede todo, viewer solo lectura)

**Contexto Técnico:**
- `middleware/server/auth.hpp` — middleware JWT
- JWT header en mensajes IPC
- Biblioteca: jwt-cpp o implementación simple HMAC-SHA256

---

### S8-I5 — [Backend] Edición masiva de horarios (backend)

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-8` |
| **Milestone** | Beta |
| **Dependencias** | S4-I2 (CRUD horarios) |

**Problema:**
No hay edición masiva de asignaciones de horario.

**Solución Propuesta:**
- `POST /batch/schedules` — múltiples asignaciones en una request
- Validación transaccional: si alguna asignación es inválida, se rechaza todo
- Reporte detallado de errores

**Criterios de Aceptación:**
- [ ] Edición masiva de asignaciones
- [ ] Transaccional (todo o nada)
- [ ] Validación de conflictos en batch

**Contexto Técnico:**
- Reusa lógica de S8-I2
- Validación extra para conflictos horarios

---

## Sprint 9 — Beta

### S9-I1 — [Backend] Reporte de conflictos no resueltos

| Campo | Valor |
|-------|-------|
| **Asignado** | Luis |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-9` |
| **Milestone** | Beta |
| **Dependencias** | S8-I1 (reasignación rápida) |

**Problema:**
Cuando el solver no encuentra solución, no hay un reporte claro de qué restricciones causan el conflicto.

**Solución Propuesta:**
Sistema de diagnosis:
- Identificar restricciones infactibles (minimal conflict set)
- Reporte human-readable: "Conflicto entre materia X (aula A, lunes 8am) y materia Y (aula A, lunes 8am)"
- Sugerencias de resolución

**Criterios de Aceptación:**
- [ ] Reporta al menos un conjunto mínimo de conflicto
- [ ] Mensaje claro para el usuario
- [ ] Sugerencia de resolución

**Contexto Técnico:**
- `backend/src/solver/diagnosis.hpp`
- Algoritmo de búsqueda de conflictos

---

### S9-I2 — [Backend] Endpoint de estadísticas del sistema

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-9` |
| **Milestone** | Beta |
| **Dependencias** | S9-I1 (reportes) |

**Problema:**
No hay endpoint que exponga estadísticas del sistema para el dashboard.

**Solución Propuesta:**
- `GET /stats` — estadísticas generales
- `GET /stats/occupation` — ocupación de aulas
- `GET /stats/teachers/load` — carga horaria por profesor
- Formato JSON listo para gráficos

**Criterios de Aceptación:**
- [ ] Endpoint stats devuelve métricas generales
- [ ] Endpoint occupation devuelve ocupación por aula
- [ ] Endpoint teacher load devuelve carga por profesor

**Contexto Técnico:**
- `backend/src/services/stats_service.hpp`
- Consultas SQL agregadas

---

### S9-I3 — [Frontend] Exportación a PDF de horarios

| Campo | Valor |
|-------|-------|
| **Asignado** | Paola |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-9` |
| **Milestone** | Beta |
| **Dependencias** | S5-I3 (vista de exportación) |

**Problema:**
Solo se puede exportar a JSON/CSV. No hay formato PDF imprimible.

**Solución Propuesta:**
Exportar horario a PDF:
- Usar QPrinter/QPainter para generar PDF desde Qt
- Formato tabla: días como columnas, franjas como filas
- Header con nombre del profesor/aula
- Opción de exportar horario individual por profesor

**Criterios de Aceptación:**
- [ ] Exportación PDF del horario completo
- [ ] Exportación PDF por profesor
- [ ] Formato tabla legible e imprimible
- [ ] Header informativo

**Contexto Técnico:**
- `frontend/` — QPrinter, QPainter, QPdfWriter

---

### S9-I4 — [Middleware] Rate limiting y protección de endpoints

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-middleware/qa |
| **Labels** | `enhancement`, `status:needs-review`, `area-middleware/qa`, `sprint-9` |
| **Milestone** | Beta |
| **Dependencias** | S8-I4 (autenticación) |

**Problema:**
No hay límite de peticiones. Un cliente malicioso o con bug puede saturar el backend.

**Solución Propuesta:**
Rate limiter en middleware:
- Límite por IP/sesión: 100 req/min
- Token bucket algorithm
- Headers: X-RateLimit-Limit, X-RateLimit-Remaining, X-RateLimit-Reset
- Protección de endpoints sensibles (login, generación)

**Criterios de Aceptación:**
- [ ] Rate limit funcional (100 req/min)
- [ ] Headers de rate limit en respuestas
- [ ] Endpoints sensibles con límite más restrictivo (10 req/min)
- [ ] Test de rate limiting

**Contexto Técnico:**
- `middleware/server/rate_limiter.hpp`
- Token bucket thread-safe

---

### S9-I5 — [Backend] Reportes y estadísticas del sistema (backend)

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-9` |
| **Milestone** | Beta |
| **Dependencias** | S9-I1 (reporte conflictos) |

**Problema:**
S9-I1 genera reportes de conflictos. No hay reportes consolidados del sistema.

**Solución Propuesta:**
Reportes backend:
- Reporte semanal de ocupación de aulas
- Reporte de carga horaria por profesor
- Reporte de materias sin asignar
- Exportación a JSON

**Criterios de Aceptación:**
- [ ] Reporte de ocupación semanal
- [ ] Reporte de carga por profesor
- [ ] Reporte de materias sin asignar
- [ ] Exportación JSON

**Contexto Técnico:**
- `backend/src/services/report_service.hpp`
- Consultas SQL con GROUP BY y agregaciones

---

## Sprint 10 — Release 1.0

### S10-I1 — [Backend] Carga masiva desde Excel/CSV

| Campo | Valor |
|-------|-------|
| **Asignado** | Luis |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-10` |
| **Milestone** | Release 1.0 |
| **Dependencias** | Ninguna |

**Problema:**
Cargar profesores, aulas y materias uno por uno es lento para volúmenes grandes.

**Solución Propuesta:**
Parser de archivos Excel/CSV:
- `POST /import/teachers` — CSV → SQLite
- `POST /import/classrooms` — CSV → SQLite
- `POST /import/subjects` — CSV → SQLite
- Formato esperado documentado
- Validación de datos antes de insertar
- Reporte de errores por fila

**Criterios de Aceptación:**
- [ ] Carga CSV de profesores con 100+ registros
- [ ] Carga CSV de aulas
- [ ] Carga CSV de materias
- [ ] Validación por fila con reporte de errores
- [ ] Rollback si hay errores críticos

**Contexto Técnico:**
- `backend/src/import/` — módulo de importación
- Parseo CSV con validación
- Batch inserts con transacciones

---

### S10-I2 — [Backend] Validación de datos cargados masivamente

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-10` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S10-I1 (carga masiva) |

**Problema:**
Los datos cargados masivamente pueden tener errores (duplicados, referencias inválidas).

**Solución Propuesta:**
Validador post-carga:
- Detectar emails duplicados en profesores
- Detectar aulas con capacidad inválida
- Detectar materias sin profesor asignable
- Reporte consolidado de issues

**Criterios de Aceptación:**
- [ ] Validador de duplicados
- [ ] Validador de integridad referencial
- [ ] Reporte claro de errores encontrados

**Contexto Técnico:**
- `backend/src/import/validator.hpp`
- Post-procesamiento después de S10-I1

---

### S10-I3 — [Frontend] Interfaz de importación con preview

| Campo | Valor |
|-------|-------|
| **Asignado** | Paola |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-10` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S10-I1 (carga masiva) |

**Problema:**
No hay interfaz para cargar archivos CSV/Excel con preview de datos.

**Solución Propuesta:**
Ventana de importación:
- Selector de archivo (QFileDialog)
- Preview de datos en tabla antes de importar
- Checkboxes para seleccionar filas a importar
- Barra de progreso durante la importación
- Reporte de resultados: N insertados, N errores

**Criterios de Aceptación:**
- [ ] Selector de archivo CSV/Excel funcional
- [ ] Preview de datos en tabla
- [ ] Importación con barra de progreso
- [ ] Reporte post-importación claro

**Contexto Técnico:**
- `frontend/` — QFileDialog, QTableWidget, QProgressBar
- Llamadas a S10-I1 via middleware

---

### S10-I4 — [Middleware] Documentación de API (Swagger/OpenAPI)

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-docs |
| **Labels** | `enhancement`, `status:needs-review`, `area-docs`, `sprint-10` |
| **Milestone** | Release 1.0 |
| **Dependencias** | Endpoints existentes (todos los sprints previos) |

**Problema:**
No hay documentación de la API. Los endpoints y su formato son conocidos solo por quien los implementó.

**Solución Propuesta:**
Documentación OpenAPI 3.0 de todos los endpoints:
- Formato JSON/YAML estándar
- Descripción de cada endpoint, parámetros y respuestas
- Ejemplos de request/response
- Esquemas de datos (JSON Schema)

**Criterios de Aceptación:**
- [ ] Documentación de todos los endpoints CRUD
- [ ] Documentación de endpoints de generación/exportación
- [ ] Documentación de endpoints de importación
- [ ] Ejemplos de request/response

**Contexto Técnico:**
- Archivo `docs/api/openapi.yaml`
- O `middleware/common/api_docs.hpp` si se sirve desde el middleware

---

### S10-I5 — [Backend] Validación y procesamiento de carga masiva

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-10` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S10-I1 (carga masiva) |

**Problema:**
S10-I1 importa datos crudos. Falta el procesamiento (normalización, deduplicación inteligente).

**Solución Propuesta:**
Pipeline de procesamiento:
- Normalizar nombres (trim, capitalize)
- Detectar y fusionar duplicados por email/nombre
- Asignar IDs consistentes
- Log de transformaciones aplicadas

**Criterios de Aceptación:**
- [ ] Normalización de datos
- [ ] Detección de duplicados con sugerencia de fusión
- [ ] Log detallado de transformaciones

**Contexto Técnico:**
- `backend/src/import/processor.hpp`
- Pipeline post-validación

---

## Sprint 11 — Release 1.0

### S11-I1 — [Backend] Algoritmo avanzado con preferencias de profesores

| Campo | Valor |
|-------|-------|
| **Asignado** | Luis |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-11` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S10-I1 (carga masiva) |

**Problema:**
El solver asigna horarios sin considerar preferencias de profesores (turno preferido, días libres).

**Solución Propuesta:**
Modelo CP-SAT con preferencias:
- Preferencia de turno (mañana > tarde > noche) como weight en objetivo
- Días libres solicitados como restricción blanda
- Profesores con carga máxima/mínima
- Ranking de soluciones por satisfacción de preferencias

**Criterios de Aceptación:**
- [ ] Preferencias de turno impactan la solución
- [ ] Días libres se respetan (o se reportan si no es posible)
- [ ] Carga máxima/mínima por profesor se cumple
- [ ] Solución ranking por satisfacción

**Contexto Técnico:**
- `backend/src/solver/preferences.hpp` — modelo extendido
- Pesos en función objetivo

---

### S11-I2 — [Backend] Personalización de restricciones por aula

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-11` |
| **Milestone** | Release 1.0 |
| **Dependencias** | Ninguna (usa restricciones existentes) |

**Problema:**
Las restricciones de aula son fijas (capacidad). No hay personalización (equipamiento, tipo de aula).

**Solución Propuesta:**
Restricciones personalizables por aula:
- Tipo de aula (laboratorio, salón, taller)
- Equipamiento requerido (proyector, pizarra, computadoras)
- Restricción: materia X solo en aula tipo Y
- CRUD de restricciones por aula

**Criterios de Aceptación:**
- [ ] CRUD de restricciones por aula
- [ ] Solver respeta tipo de aula
- [ ] Solver respeta equipamiento requerido

**Contexto Técnico:**
- Extiende tabla `classrooms` con campos tipo/equipamiento
- Extiende solver con restricciones de tipo

---

### S11-I3 — [Frontend] Asistente paso a paso para generar horarios

| Campo | Valor |
|-------|-------|
| **Asignado** | Paola |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-11` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S11-I1 (algoritmo avanzado) |

**Problema:**
Generar un horario requiere múltiples pasos (cargar datos, configurar restricciones, ejecutar). No hay guía.

**Solución Propuesta:**
QWizard con pasos:
1. Seleccionar profesores y materias
2. Configurar restricciones (turnos, aulas)
3. Configurar preferencias
4. Ejecutar solver
5. Revisar y exportar resultado

**Criterios de Aceptación:**
- [ ] Wizard de 5 pasos funcional
- [ ] Cada paso valida antes de avanzar
- [ ] Paso 4 muestra progreso del solver
- [ ] Paso 5 permite exportar

**Contexto Técnico:**
- `frontend/` — QWizard, QWizardPage
- Llama a S11-I1 vía middleware

---

### S11-I4 — [Middleware] Webhooks para notificaciones de cambios

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-middleware/qa |
| **Labels** | `enhancement`, `status:needs-review`, `area-middleware/qa`, `sprint-11` |
| **Milestone** | Release 1.0 |
| **Dependencias** | Ninguna |

**Problema:**
Los cambios en horarios no notifican a los interesados. No hay forma de saber si un horario cambió.

**Solución Propuesta:**
Sistema de webhooks:
- Registrar webhooks URL por evento
- Eventos: horario.generado, horario.modificado, profesor.asignado
- POST HTTP a URL registrada con payload del cambio
- Reintentos con backoff

**Criterios de Aceptación:**
- [ ] Registro de webhooks funcional
- [ ] Notificación en evento horario.generado
- [ ] Reintentos con backoff (3 intentos)

**Contexto Técnico:**
- `middleware/server/webhook.hpp`
- Usar Qt Network o curl para HTTP POST

---

### S11-I5 — [Backend] Restricciones por preferencias de profesores

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-11` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S11-I1 (algoritmo avanzado) |

**Problema:**
Las preferencias de profesores existen en el solver (S11-I1) pero no hay endpoints para gestionarlas.

**Solución Propuesta:**
CRUD de preferencias de profesor:
- `GET/POST/PUT/DELETE /teachers/{id}/preferences`
- Campos: preferred_shift (morning/afternoon/evening), max_hours, min_hours, preferred_days_off
- Validación de consistencia

**Criterios de Aceptación:**
- [ ] CRUD de preferencias funcional
- [ ] Validación de consistencia con disponibilidad
- [ ] Integración con solver S11-I1

**Contexto Técnico:**
- Tabla SQLite `teacher_preferences`
- Endpoints consumidos por frontend

---

## Sprint 12 — Release 1.0

### S12-I1 — [Backend] Refactor y optimización final del solver

| Campo | Valor |
|-------|-------|
| **Asignado** | Luis |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-12` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S11-I1 (algoritmo avanzado) |

**Problema:**
El solver tiene code smell, falta de modularidad y oportunidades de optimización.

**Solución Propuesta:**
Refactor completo del solver:
- Separar modelo, restricciones y objetivo en módulos claros
- Optimizar variables CP-SAT (reducir cantidad)
- Cache de resultados intermedios
- Perf: reducir tiempo de resolución en 50%
- Documentación interna del modelo matemático

**Criterios de Aceptación:**
- [ ] Código refactorizado y modular
- [ ] Tiempo de resolución 50% menor (benchmark)
- [ ] Tests existentes siguen pasando
- [ ] Documentación del modelo matemático

**Contexto Técnico:**
- `backend/src/solver/` — refactor completo
- Benchmark con casos de prueba conocidos

---

### S12-I2 — [Backend] Historial de cambios en horarios

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-12` |
| **Milestone** | Release 1.0 |
| **Dependencias** | Ninguna |

**Problema:**
No hay trazabilidad de quién cambió qué y cuándo en los horarios.

**Solución Propuesta:**
Audit log:
- Tabla `audit_log` (id, entity_type, entity_id, action, user_id, old_values, new_values, timestamp)
- Trigger automático en modificaciones de horarios
- Endpoint `GET /audit?entity=schedules&id=X`
- Interfaz de consulta en frontend

**Criterios de Aceptación:**
- [ ] Audit log registra todas las modificaciones
- [ ] Consulta de historial por entidad
- [ ] Frontend muestra historial de cambios

**Contexto Técnico:**
- `backend/src/services/audit_service.hpp`
- Tabla SQLite `audit_log`
- JSON diff para old/new values

---

### S12-I3 — [Frontend] Modo oscuro y personalización de UI

| Campo | Valor |
|-------|-------|
| **Asignado** | Paola |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-12` |
| **Milestone** | Release 1.0 |
| **Dependencias** | Ninguna |

**Problema:**
La UI tiene un solo tema claro. No hay opciones de personalización.

**Solución Propuesta:**
- Tema oscuro completo (QPalette personalizado)
- Tema claro (default)
- Selector de tema en settings
- Personalización: tamaño de fuente, espaciado
- Persistencia de preferencias en QSettings

**Criterios de Aceptación:**
- [ ] Modo oscuro funcional en toda la UI
- [ ] Selector de tema en settings
- [ ] Preferencia persiste entre sesiones
- [ ] Consistencia visual en ambos temas

**Contexto Técnico:**
- `frontend/themes/` — QPalette dark/light
- QSettings para persistencia

---

### S12-I4 — [Middleware] Monitoreo y alertas del sistema

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-middleware/qa |
| **Labels** | `enhancement`, `status:needs-review`, `area-middleware/qa`, `sprint-12` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S11-I4 (webhooks) |

**Problema:**
No hay monitoreo del estado del sistema. Si el backend cae, no hay alerta.

**Solución Propuesta:**
Health endpoint mejorado:
- `GET /health` → estado de todos los componentes (backend, DB, cache)
- Endpoint `GET /metrics` → métricas de rendimiento
- Alerta automática si backend no responde (reintentar, loggear, notificar webhook)
- Dashboard de monitoreo en frontend

**Criterios de Aceptación:**
- [ ] Health check de todos los componentes
- [ ] Endpoint de métricas (requests/min, avg latency)
- [ ] Alerta si backend caído por >10s
- [ ] Dashboard de monitoreo en frontend

**Contexto Técnico:**
- `middleware/server/monitor.hpp`
- Integración con webhooks S11-I4

---

### S12-I5 — [Backend] Optimización de consultas e índices SQLite

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-12` |
| **Milestone** | Release 1.0 |
| **Dependencias** | Ninguna (funcionalidades completas) |

**Problema:**
Las consultas SQL se vuelven lentas con datos reales (100+ profesores, 50+ aulas).

**Solución Propuesta:**
- Índices en columnas usadas en WHERE/JOIN
- Optimizar consultas con EXPLAIN QUERY PLAN
- Vacuum periódico
- Benchmark de consultas antes/después

**Criterios de Aceptación:**
- [ ] Índices creados en columnas clave (teacher_id, classroom_id, subject_id)
- [ ] Tiempo de consultas <100ms con datos reales
- [ ] EXPLAIN QUERY PLAN muestra index scans

**Contexto Técnico:**
- Migraciones SQL para índices
- `backend/src/database/optimization.hpp`

---

## Sprint 13 — Release 1.0

### S13-I1/I2 — [Backend] Correcciones finales y estabilización (Luis + Dani)

| Campo | Valor |
|-------|-------|
| **Asignados** | Luis, Dani |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-13` |
| **Milestone** | Release 1.0 |
| **Dependencias** | Todo Sprint 12 |

**Problema:**
Bugs residuales, edge cases no cubiertos, estabilización general.

**Solución Propuesta:**
Correcciones finales:
- Bug fixes reportados en testing
- Edge cases (0 profesores, 0 aulas, datos vacíos)
- Mejora de mensajes de error
- Logging de producción (niveles ajustados)
- Última ronda de optimización

**Criterios de Aceptación:**
- [ ] Todos los bugs conocidos corregidos
- [ ] Manejo graceful de edge cases
- [ ] Logging de producción configurado
- [ ] Tests de regresión pasan

**Contexto Técnico:**
- Issues de GitHub con label `bug` y `sprint-13`

---

### S13-I3 — [Frontend] Correcciones finales de UI

| Campo | Valor |
|-------|-------|
| **Asignado** | Paola |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-13` |
| **Milestone** | Release 1.0 |
| **Dependencias** | Sprint 12 |

**Problema:**
Detalles de UI: espaciados inconsistentes, texto cortado, colores no alineados.

**Solución Propuesta:**
Pulido final:
- Revisión de espaciados y alineación
- Texto truncado: asegurar que todo el contenido es visible
- Tooltips en todos los botones
- Mensajes de error amigables en lugar de códigos
- Prueba en resoluciones 1366×768, 1920×1080, 2560×1440

**Criterios de Aceptación:**
- [ ] UI consistente en resoluciones objetivo
- [ ] Tooltips en todos los botones de acción
- [ ] Mensajes de error amigables
- [ ] Sin texto cortado o solapado

**Contexto Técnico:**
- `frontend/` — revisión visual completa
- QSizePolicy y layouts

---

### S13-I4 — [Middleware] Despliegue y configuración del servidor

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-middleware/qa |
| **Labels** | `enhancement`, `status:needs-review`, `area-middleware/qa`, `sprint-13` |
| **Milestone** | Release 1.0 |
| **Dependencias** | Sprint 12 |

**Problema:**
El sistema funciona en desarrollo pero no está listo para producción.

**Solución Propuesta:**
Preparación para despliegue:
- Script de instalación (setup.sh)
- Docker compose para producción (backend + middleware)
- Variables de entorno configurables
- Documentación de despliegue
- Checklist de go-live

**Criterios de Aceptación:**
- [ ] Script de instalación funcional
- [ ] Docker compose para producción
- [ ] Variables de entorno documentadas
- [ ] Documentación de despliegue lista

**Contexto Técnico:**
- `deploy/` — scripts, docker-compose.prod.yml
- Ver docs de despliegue

---

### S13-I5 — [Docs] Tests de aceptación y documento de cierre

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-docs |
| **Labels** | `enhancement`, `status:needs-review`, `area-docs`, `sprint-13` |
| **Milestone** | Release 1.0 |
| **Dependencias** | Sprint 12 |

**Problema:**
No hay documentación de cierre ni tests de aceptación formales.

**Solución Propuesta:**
Documentos de cierre:
- Tests de aceptación (criterios de go-live)
- Documento de cierre del proyecto
- Manual de usuario básico
- README actualizado con instrucciones de uso

**Criterios de Aceptación:**
- [ ] Tests de aceptación documentados
- [ ] Documento de cierre completo
- [ ] Manual de usuario funcional
- [ ] README actualizado

**Contexto Técnico:**
- `docs/` — documentación de cierre
- Basado en issues completadas y lecciones aprendidas

---

## Resumen

| Sprint | Issues | Milestone |
|--------|:------:|-----------|
| 2 | 5 | MVP Base |
| 3 | 5 | MVP Base |
| 4 | 5 | MVP Base |
| 5 | 5 | MVP Base |
| 6 | 5 | Beta |
| 7 | 5 | Beta |
| 8 | 5 | Beta |
| 9 | 5 | Beta |
| 10 | 5 | Release 1.0 |
| 11 | 5 | Release 1.0 |
| 12 | 5 | Release 1.0 |
| 13 | 5 | Release 1.0 |
| **Total** | **60** | **3 milestones** |

---

> *Documento generado el 28 de junio de 2026.*
> *Sirve como guía para la asignación semanal de issues entre el equipo.*
> *Las issues de Sprint 2 ya fueron creadas en GitHub. Las restantes se crearán al iniciar cada sprint.*
