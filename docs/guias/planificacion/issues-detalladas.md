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

## Sprint 2 — MVP Base (22-28 Jun)

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

---

### S2-I2 — [Middleware] Endpoints CRUD de profesores vía middleware

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-middleware |
| **Labels** | `enhancement`, `status:needs-review`, `area-middleware`, `sprint-2` |
| **Milestone** | MVP Base |
| **Dependencias** | S2-I1 (estructuras), S2-I5 (SQLite) |

**Problema:**
No existen endpoints para gestionar profesores. El sistema necesita CRUD completo para asignarlos a materias y horarios, y la comunicación debe pasar por el middleware.

**Solución Propuesta:**
Manuel implementa el enrutamiento en middleware para operaciones CRUD de profesores:
- `GET /teachers` — listar todos
- `GET /teachers/{id}` — obtener por ID
- `POST /teachers` — crear
- `PUT /teachers/{id}` — actualizar
- `DELETE /teachers/{id}` — eliminar

El middleware expone estos endpoints, validando y enrutando hacia los servicios backend que implementa Nicole (S2-I5).

**Criterios de Aceptación:**
- [ ] `GET /teachers` retorna array JSON (vacío si no hay datos)
- [ ] `GET /teachers/{id}` retorna profesor o 404
- [ ] `POST /teachers` valida campos requeridos (name, email) → 400 si inválido
- [ ] `PUT /teachers/{id}` actualiza y retorna el registro actualizado
- [ ] `DELETE /teachers/{id}` retorna 204 o 404
- [ ] Datos persisten en SQLite correctamente
- [ ] Comunicación con frontend vía middleware (Qt Local Sockets + JSON)

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

---

### S2-I4 — [Middleware] Setup de middleware + health-check

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-middleware |
| **Labels** | `enhancement`, `status:needs-review`, `area-middleware`, `sprint-2` |
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

---

### S2-I6 — [Frontend] Scaffold Qt + capacitación Qt/Qt Creator

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-2` |
| **Milestone** | MVP Base |
| **Dependencias** | Ninguna |

**Problema:**
El proyecto no tiene interfaz gráfica. No existe estructura frontend/ ni configuración Qt6. Dani, como frontend lead, debe configurar el entorno y capacitarse en Qt Creator/Qt6 Widgets.

**Solución Propuesta:**
- Crear estructura `frontend/` con CMakeLists.txt para Qt6 Widgets
- Configurar Qt Creator project (.pro/user files opcionales)
- LoginDialog básico como punto de partida (mock, sin backend)
- Capacitación: Dani estudia Qt6 Widgets, QLocalSocket, QJsonDocument
- Documentar setup en `docs/frontend-setup.md`

**Criterios de Aceptación:**
- [ ] `frontend/CMakeLists.txt` compila con Qt6 Widgets
- [ ] LoginDialog básico funcional (mock, sin validación real)
- [ ] Proyecto abre correctamente en Qt Creator
- [ ] Documento de setup creado con instrucciones
- [ ] Dani demuestra comprensión de Qt6 Widgets básicos

---

## Sprint 3 — MVP Base (29 Jun-5 Jul)

### S3-I1 — [Backend] Modelo de datos entidades + diagrama ER

| Campo | Valor |
|-------|-------|
| **Asignado** | Luis |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-3` |
| **Milestone** | MVP Base |
| **Dependencias** | S2-I1 (OR-Tools + estructuras) |

**Problema:**
Las estructuras de datos existen (S2-I1) pero no hay un modelo relacional completo que relacione profesores, aulas, materias, turnos y horarios. El diagrama ER es necesario antes de implementar el solver.

**Solución Propuesta:**
- Definir entidades: Teacher, Classroom, Subject, TimeSlot, Schedule, Availability
- Relaciones: Teacher→Subject (N:M), Schedule→Teacher+Subject+Classroom+TimeSlot
- Diagrama ER en `docs/er-diagram.md`
- Modelo de datos C++ en `backend/include/data/` extendiendo S2-I1
- Validar que el modelo cubre todos los casos de uso del sistema

**Criterios de Aceptación:**
- [ ] Diagrama ER completo con todas las entidades y relaciones
- [ ] Modelo de datos C++ refleja el diagrama ER
- [ ] Relaciones N:M correctamente modeladas (tablas pivote)
- [ ] Serialización JSON de todas las entidades
- [ ] Test unitario que crea un grafo completo de entidades

---

### S3-I2 — [Backend] CRUD de materias

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-3` |
| **Milestone** | MVP Base |
| **Dependencias** | S2-I5 (esquema SQLite) |

**Problema:**
No hay endpoints para gestionar materias. El solver necesita materias como input y actualmente no existe forma de crearlas, consultarlas, actualizarlas o eliminarlas.

**Solución Propuesta:**
CRUD completo para `subjects`:
- `GET/POST/PUT/DELETE /subjects`
- Campos: name, weekly_hours, required_classroom_type, department
- Persistencia en SQLite + serialización JSON
- Validación de campos obligatorios y tipos

**Criterios de Aceptación:**
- [ ] CRUD completo funcional con validaciones
- [ ] Persistencia en SQLite con tabla `subjects`
- [ ] Serialización JSON middleware-compatible
- [ ] Respuestas de error claras para datos inválidos

---

### S3-I3 — [Frontend] Formulario de registro de profesores

| Campo | Valor |
|-------|-------|
| **Asignado** | Paola |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-3` |
| **Milestone** | MVP Base |
| **Dependencias** | S2-I6 (login), S2-I2 (CRUD profesores) |

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

---

### S3-I4 — [Middleware] Enrutamiento CRUD frontend → backend

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-middleware |
| **Labels** | `enhancement`, `status:needs-review`, `area-middleware`, `sprint-3` |
| **Milestone** | MVP Base |
| **Dependencias** | S2-I2 (endpoints middleware) |

**Problema:**
El middleware existe (health-check) pero no enruta peticiones CRUD. Frontend no puede comunicarse con backend para operaciones de datos.

**Solución Propuesta:**
Routing de mensajes CRUD en el middleware:
- Identificar operación por campo `"op"` en JSON
- Encaminar al servicio backend correspondiente
- Devolver respuesta al frontend
- Manejo de errores (timeout, backend caído)
- Mapa de rutas extensible para nuevas entidades

**Criterios de Aceptación:**
- [ ] Middleware enruta peticiones CRUD profesores/materias
- [ ] Frontend recibe respuestas correctas
- [ ] Timeout de 5s con mensaje de error si backend no responde
- [ ] Mapa de rutas configurable para agregar nuevas entidades

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
No hay endpoints para gestionar aulas. El solver necesita datos de aulas como input y actualmente no existen operaciones CRUD para esta entidad.

**Solución Propuesta:**
CRUD completo para `classrooms`:
- `GET/POST/PUT/DELETE /classrooms`
- Campos: name, capacity, building, floor, resources
- Persistencia SQLite + JSON
- Validación de capacidad positiva y nombre único

**Criterios de Aceptación:**
- [ ] CRUD funcional con validaciones
- [ ] Tabla `classrooms` operativa
- [ ] Compatible con middleware (contrato de mensajes)
- [ ] Validación de datos de entrada

---

### S3-I6 — [Frontend] Prototipos dashboard + flujo de navegación

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani + Paola |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-3` |
| **Milestone** | MVP Base |
| **Dependencias** | S2-I6 (Qt scaffold) |

**Problema:**
El scaffold Qt tiene un LoginDialog y MainWindow vacío. No hay prototipos de las pantallas principales ni un flujo de navegación definido.

**Solución Propuesta:**
- Dani + Paola diseñan prototipos de todas las pantallas principales:
  - Dashboard con resumen del sistema
  - Formularios de entrada (profesores, aulas, materias)
  - Vista de horarios (tabla semana)
  - Pantalla de generación/exportación
- Maquetas en papel/figma primero, luego prototipos Qt
- Flujo de navegación documentado: Login → Dashboard → (Formularios | Horarios | Generación)
- QStackedWidget con navegación por botones en sidebar

**Criterios de Aceptación:**
- [ ] Prototipos de todas las pantallas principales aprobados por el equipo
- [ ] Flujo de navegación documentado en `docs/flujo-navegacion.md`
- [ ] Sidebar con navegación funcional entre pantallas placeholder
- [ ] Cada pantalla placeholder tiene el layout base preparado

---

## Sprint 4 — MVP Base (06-12 Jul)

### S4-I1 — [Backend] Modelo CP-SAT + restricciones (aulas, turnos)

| Campo | Valor |
|-------|-------|
| **Asignado** | Luis |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-4` |
| **Milestone** | MVP Base |
| **Dependencias** | S3-I1 (data model) |

**Problema:**
OR-Tools está integrado pero no hay un algoritmo que resuelva la asignación de horarios. El modelo de datos (S3-I1) ya definió las entidades y relaciones, ahora es momento de implementar el solver CP-SAT.

**Solución Propuesta:**
Implementar el modelo CP-SAT básico con restricciones de aulas y turnos:
- Variables: asignación materia→profesor→aula→franja
- Restricciones: un profesor no puede estar en dos lugares, un aula no puede tener dos materias simultáneas
- Restricciones de capacidad: materia X requiere aula con capacidad ≥ Y
- Restricciones de turno: ciertas materias solo en turno mañana/tarde
- Tipos de turno: mañana (7-12), tarde (13-18), noche (18-22)
- Función objetivo: maximizar ocupación de aulas

**Criterios de Aceptación:**
- [ ] El solver encuentra solución para 5 profesores, 3 aulas, 10 materias en <30s
- [ ] Restricciones básicas se cumplen (sin choques horarios)
- [ ] Solución respeta capacidad de aulas y turnos asignados
- [ ] Si no hay solución posible, solver retorna "infeasible" con reporte
- [ ] La solución se serializa a JSON correctamente
- [ ] Test unitario con caso conocido valida el resultado

---

### S4-I2 — [Backend] CRUD horarios (asignación manual)

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
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
- Tabla SQLite `schedule_entries`

**Criterios de Aceptación:**
- [ ] CRUD horarios funcional
- [ ] Validación de conflictos: mismo profesor en dos lugares
- [ ] Validación de conflictos: misma aula dos veces
- [ ] Datos en SQLite correctamente persistidos

---

### S4-I3 — [Frontend] Formularios entrada docente/sección

| Campo | Valor |
|-------|-------|
| **Asignado** | Paola |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-4` |
| **Milestone** | MVP Base |
| **Dependencias** | S3-I3 (teacher form) |

**Problema:**
El formulario de profesores existe (S3-I3) pero no hay formularios para secciones, ni validación de datos de entrada en el frontend.

**Solución Propuesta:**
Paola diseña los formularios de entrada docente/sección:
- Formulario de secciones (grado, sección, turno, materias)
- Validación de datos en frontend antes de enviar al middleware
- Mensajes de error claros para cada campo
- Consistencia visual con el formulario de profesores

**Criterios de Aceptación:**
- [ ] Formulario de secciones con campos correctos
- [ ] Validación frontend (campos requeridos, formatos)
- [ ] Mensajes de error específicos por campo
- [ ] Consistencia visual con formulario de profesores
- [ ] Datos se envían correctamente vía middleware

---

### S4-I4 — [Middleware/QA] Tests unitarios modelo de datos

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-middleware |
| **Labels** | `enhancement`, `status:needs-review`, `area-middleware`, `sprint-4` |
| **Milestone** | MVP Base |
| **Dependencias** | S3-I1 (data model) |

**Problema:**
El modelo de datos C++ (S3-I1) no tiene tests unitarios. No hay garantía de que las entidades, relaciones y serialización funcionen correctamente.

**Solución Propuesta:**
Manuel implementa tests unitarios para:
- Creación de entidades y asignación de propiedades
- Relaciones entre entidades (profesor→materias, aula→horarios)
- Serialización/deserialización JSON de cada entidad
- Casos borde: campos vacíos, valores límite, datos nulos
- Adaptadores de tipos entre backend y middleware

**Criterios de Aceptación:**
- [ ] Tests unitarios para cada entidad del modelo
- [ ] Tests de serialización JSON (round-trip)
- [ ] Tests de relaciones entre entidades
- [ ] Tests de casos borde (datos vacíos, valores extremos)
- [ ] Tests se ejecutan automáticamente en CI

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
- [ ] Consultas optimizadas (índices SQL)

---

### S4-I6 — [Frontend] Formularios entrada docente y sección (Qt)

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-4` |
| **Milestone** | MVP Base |
| **Dependencias** | S3-I6 (prototypes) |

**Problema:**
Los prototipos del dashboard y navegación existen (S3-I6) pero no hay formularios Qt funcionales para entrada de datos docente y sección.

**Solución Propuesta:**
Dani implementa los formularios Qt basados en los prototipos de Paola (S4-I3):
- Formulario de datos docente (nombre, email, teléfono, materias)
- Formulario de sección (grado, sección, turno)
- Validación de campos en frontend
- Conexión a middleware para persistencia
- Colaboración con Paola para alinear diseño visual

**Criterios de Aceptación:**
- [ ] Formulario docente Qt funcional con validación
- [ ] Formulario sección Qt funcional con validación
- [ ] Datos se envían correctamente vía middleware
- [ ] Diseño visual alineado con prototipos de Paola
- [ ] Mensajes de error claros para el usuario

---

## Sprint 5 — MVP Base (13-19 Jul)

### S5-I1 — [Backend] Restricciones: horas semanales + integrar restricciones + validación simulada

| Campo | Valor |
|-------|-------|
| **Asignado** | Luis |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-5` |
| **Milestone** | MVP Base |
| **Dependencias** | S4-I1 (CP-SAT) |

**Problema:**
El solver CP-SAT maneja restricciones de aulas y turnos (S4-I1) pero no incluye restricciones de horas semanales por materia ni una integración completa de todas las restricciones duras.

**Solución Propuesta:**
- Agregar restricción de horas semanales: cada materia debe recibir N horas semanales
- Integrar todas las restricciones duras en un solo modelo: capacidad, turnos, horas semanales, sin conflictos
- Validar con datos simulados (10+ profesores, 5+ aulas, 15+ materias)
- Reporte de factibilidad: si no hay solución, identificar restricciones conflictivas
- Benchmark de tiempo de resolución con datos simulados

**Criterios de Aceptación:**
- [ ] Restricción de horas semanales implementada y validada
- [ ] Todas las restricciones duras integradas en un solo modelo
- [ ] Solver encuentra solución con 10 profesores, 5 aulas, 15 materias en <60s
- [ ] Reporte de factibilidad cuando no hay solución
- [ ] Validación con datos simulados completa

---

### S5-I2 — [Backend] Exportar solución solver a JSON/CSV

| Campo | Valor |
|-------|-------|
| **Asignado** | Luis |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-5` |
| **Milestone** | MVP Base |
| **Dependencias** | S5-I1 (constraints) |

**Problema:**
La solución generada por el solver solo existe en memoria y no puede exportarse para uso externo.

**Solución Propuesta:**
Serializar Schedule a:
- JSON: estructura completa del horario (profesor, materia, aula, día, hora)
- CSV: filas por asignación, legible en Excel/Google Sheets
- Archivos generados por el backend y servidos al frontend vía middleware
- Formato CSV con BOM para compatibilidad Excel

**Criterios de Aceptación:**
- [ ] Exportación a JSON con estructura completa
- [ ] Exportación a CSV legible en Excel/Google Sheets
- [ ] Archivos se generan en backend y se sirven al frontend
- [ ] Módulo `backend/src/export/` creado

---

### S5-I3 — [Frontend] Formularios aula + materia + tabla datos básica

| Campo | Valor |
|-------|-------|
| **Asignado** | Paola |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-5` |
| **Milestone** | MVP Base |
| **Dependencias** | S2-I6 (Qt scaffold) |

**Problema:**
Existen formularios de profesores y secciones pero no hay formularios para gestionar aulas y materias desde la UI. Tampoco hay una tabla que muestre los datos ingresados.

**Solución Propuesta:**
- Formulario de aula: nombre, capacidad, edificio, piso
- Formulario de materia: nombre, horas semanales, tipo de aula requerida
- Tabla básica de datos con QTableView para listar entidades
- Botones de acción (editar/eliminar) por fila
- Conexión a middleware para CRUD real

**Criterios de Aceptación:**
- [ ] Formulario aula funcional con validación
- [ ] Formulario materia funcional con validación
- [ ] Tabla muestra datos de todas las entidades
- [ ] Editar/eliminar desde la tabla funciona
- [ ] Datos persisten vía middleware

---

### S5-I4 — [Middleware/QA] Validador de datos + tests de restricciones automatizados

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-middleware |
| **Labels** | `enhancement`, `status:needs-review`, `area-middleware`, `sprint-5` |
| **Milestone** | MVP Base |
| **Dependencias** | S4-I4 (unit tests) |

**Problema:**
Los datos que viajan entre frontend y backend no tienen validación suficiente. El solver puede recibir datos inconsistentes. Las restricciones del solver no tienen tests automatizados.

**Solución Propuesta:**
- Validador de datos en middleware: JSON schema validation, campos requeridos, tipos
- Tests automatizados de restricciones del solver (cada restricción por separado)
- Integración temprana modelo→validador
- Reporte de validación con errores específicos por campo

**Criterios de Aceptación:**
- [ ] Validador rechaza datos malformados con error descriptivo
- [ ] Tests automatizados para cada restricción del solver
- [ ] Integración modelo→validador funcionando
- [ ] Reporte de validación claro y específico

---

### S5-I5 — [Backend] Exportación horarios manuales a JSON/CSV

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-5` |
| **Milestone** | MVP Base |
| **Dependencias** | S4-I2 (CRUD horarios) |

**Problema:**
La exportación del solver (S5-I2) cubre horarios generados automáticamente. No hay exportación de horarios creados manualmente.

**Solución Propuesta:**
Endpoint de exportación para horarios manuales:
- `GET /schedules/{id}/export?format=json|csv`
- Mismo formato que S5-I2 pero desde datos persistidos en SQLite
- Reusa serializadores de S5-I1 (Schedule → JSON/CSV)
- Persistencia de datos de horarios manuales

**Criterios de Aceptación:**
- [ ] Exportación de horarios manuales a JSON y CSV
- [ ] Mismo formato que exportación automática
- [ ] Reuso de serializadores existentes

---

### S5-I6 — [Frontend] Refinamiento formularios + conexión a middleware

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-5` |
| **Milestone** | MVP Base |
| **Dependencias** | S3-I6, S4-I6 |

**Problema:**
Los formularios Qt existen pero no están conectados al middleware, tienen problemas de UX, y no reciben feedback de validación del backend.

**Solución Propuesta:**
- Refinar formularios basados en feedback de Paola y el equipo
- Conectar todos los formularios al middleware (QLocalSocket → JSON)
- Mostrar errores de validación del backend en la UI
- Mejoras de UX: tabs, atajos de teclado, foco automático
- Feedback visual de operaciones (spinner, success/error toast)

**Criterios de Aceptación:**
- [ ] Todos los formularios conectados a middleware
- [ ] Errores del backend se muestran en UI
- [ ] Feedback visual de operaciones (guardando, éxito, error)
- [ ] UX mejorada: tabs, atajos, foco automático
- [ ] Test de integración formulario→middleware

---

## Sprint 6 — Beta (20-26 Jul)

### S6-I1 — [Backend] Preferencias de profesores (restricciones blandas CP-SAT)

| Campo | Valor |
|-------|-------|
| **Asignado** | Luis |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-6` |
| **Milestone** | Beta |
| **Dependencias** | S5-I1 (hard constraints) |

**Problema:**
El solver solo maneja restricciones duras (capacidad, turnos, horas). No considera preferencias de profesores como turno preferido, días libres o carga horaria deseada.

**Solución Propuesta:**
Modelo CP-SAT con restricciones blandas:
- Preferencia de turno (mañana > tarde > noche) como peso en función objetivo
- Días libres solicitados como restricción blanda con penalización
- Carga máxima/mínima por profesor
- Balance de carga entre profesores
- Ranking de soluciones por satisfacción de preferencias
- Reporte de preferencias no satisfechas

**Criterios de Aceptación:**
- [ ] Preferencias de turno impactan la solución
- [ ] Días libres se respetan (o se reportan si no es posible)
- [ ] Carga máxima/mínima por profesor se cumple
- [ ] Solución ranking por satisfacción de preferencias
- [ ] Reporte de preferencias no satisfechas

---

### S6-I2 — [Backend] CRUD disponibilidad de profesores

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
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
- Tabla SQLite `teacher_availability`
- Integración con solver en S6-I5

**Criterios de Aceptación:**
- [ ] CRUD disponibilidad funcional por profesor
- [ ] Formato matriz día×franja
- [ ] Validación contra horarios existentes
- [ ] Persistencia en SQLite

---

### S6-I3 — [Frontend] Calendario visual disponibilidad profesor

| Campo | Valor |
|-------|-------|
| **Asignado** | Paola |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-6` |
| **Milestone** | Beta |
| **Dependencias** | S6-I2 (availability CRUD) |

**Problema:**
No hay interfaz visual para ver/editar la disponibilidad de profesores de forma intuitiva.

**Solución Propuesta:**
Calendario semanal visual:
- Grilla día×franja con celdas cliqueables
- Verde = disponible, rojo = no disponible
- Selector de profesor para ver/editar su disponibilidad
- Persistir cambios vía middleware → S6-I2
- Diseño limpio y responsive

**Criterios de Aceptación:**
- [ ] Grilla día×franja interactiva
- [ ] Click cambia estado disponible/no disponible
- [ ] Selector de profesor funcional
- [ ] Persistencia automática o con botón guardar
- [ ] Diseño visual consistente con el resto de la UI

---

### S6-I4 — [Middleware/QA] Pruebas de integración front→middle→back + reporte

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-middleware |
| **Labels** | `enhancement`, `status:needs-review`, `area-middleware`, `sprint-6` |
| **Milestone** | Beta |
| **Dependencias** | S3-I4 (routing) |

**Problema:**
Cada componente funciona individualmente pero no hay pruebas de que el flujo completo frontend→middleware→backend funcione correctamente.

**Solución Propuesta:**
- Pruebas de integración end-to-end:
  - Frontend envía petición → middleware enruta → backend procesa → respuesta al frontend
  - CRUD completo de cada entidad
  - Flujo de generación de horarios
  - Manejo de errores en cada capa
- Reporte de integración con métricas de éxito/fallo
- Automatización de pruebas para CI

**Criterios de Aceptación:**
- [ ] Pruebas E2E para CRUD de cada entidad
- [ ] Prueba de flujo de generación de horarios
- [ ] Manejo de errores probado en cada capa
- [ ] Reporte de integración con métricas
- [ ] Pruebas automatizadas en CI

---

### S6-I5 — [Backend] Validación conflictos disponibilidad + solver

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-6` |
| **Milestone** | Beta |
| **Dependencias** | S6-I1, S6-I2 |

**Problema:**
El solver no considera disponibilidad de profesores (S6-I2), pudiendo asignar horarios en los que el profesor no está disponible.

**Solución Propuesta:**
Integrar disponibilidad como restricción en el solver:
- Validar que asignaciones respeten disponibilidad de cada profesor
- Reportar conflictos: "El profesor X no está disponible el lunes a las 8am"
- Combinar con preferencias (S6-I1): disponibilidad es restricción dura, preferencias son blandas
- Test con caso de conflicto conocido

**Criterios de Aceptación:**
- [ ] Solver respeta disponibilidad de profesores
- [ ] Reporte de conflictos claro y específico
- [ ] Disponibilidad como restricción dura, preferencias como blandas
- [ ] Test con caso de conflicto conocido

---

### S6-I6 — [Frontend] Integración formularios → middleware + refinamiento UX

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-6` |
| **Milestone** | Beta |
| **Dependencias** | S5-I6 (form refinement) |

**Problema:**
Los formularios están conectados al middleware pero la integración no es completa. Faltan pantallas, la UX necesita refinamiento y hay casos borde no manejados.

**Solución Propuesta:**
- Integrar todos los formularios restantes con el middleware
- Refinar UX basado en feedback de Paola y pruebas internas
- Manejar casos borde: backend caído, timeout, datos vacíos
- Mejorar mensajes de error y feedback visual
- Unificar estilo visual en todas las pantallas

**Criterios de Aceptación:**
- [ ] Todos los formularios integrados con middleware
- [ ] UX refinada (feedback visual, mensajes claros)
- [ ] Casos borde manejados (backed caído, timeout)
- [ ] Estilo visual unificado en todas las pantallas

---

## Sprint 7 — Beta (27-31 Jul)

### S7-I1 — [Backend] Performance tuning CP-SAT + edge cases

| Campo | Valor |
|-------|-------|
| **Asignado** | Luis |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-7` |
| **Milestone** | Beta |
| **Dependencias** | S6-I1 (preferences) |

**Problema:**
El solver funciona pero puede ser lento con conjuntos grandes de datos. Hay cuellos de botella y casos borde no manejados.

**Solución Propuesta:**
- Análisis de rendimiento del solver con perfiles de datos reales
- Optimización de variables CP-SAT (reducir cantidad, mejorar encoding)
- Ajuste de parámetros del solver (tiempo límite, búsqueda, paralelismo)
- Manejo de edge cases: 0 profesores, 0 aulas, restricciones imposibles
- Timeout configurable con respuesta graceful
- Benchmark antes/después con métricas

**Criterios de Aceptación:**
- [ ] Tiempo de resolución mejorado en 30% vs S6-I1
- [ ] Edge cases manejados sin crash
- [ ] Timeout configurable con respuesta graceful
- [ ] Benchmark documentado
- [ ] Test de rendimiento automatizado

---

### S7-I2 — [Backend] Validación con datos de escuela real + conflictos

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-7` |
| **Milestone** | Beta |
| **Dependencias** | S6-I2, S6-I5 |

**Problema:**
Todas las pruebas se han hecho con datos simulados. No se ha validado el sistema con datos reales de un liceo, ni se han probado escenarios de conflicto realistas.

**Solución Propuesta:**
- Preparar entorno de prueba con datos reales del Liceo Robert Serra
- Cargar datos reales de profesores, aulas, materias y horarios
- Identificar y documentar conflictos reales
- Validar que el solver maneja correctamente los conflictos
- Ajustar restricciones según necesidades reales

**Criterios de Aceptación:**
- [ ] Datos reales cargados en entorno de prueba
- [ ] Conflictos reales identificados y documentados
- [ ] Solver maneja correctamente los conflictos
- [ ] Ajustes documentados

---

### S7-I3 — [Frontend] Módulos de administración (gestión usuarios) + pantalla configuración

| Campo | Valor |
|-------|-------|
| **Asignado** | Paola |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-7` |
| **Milestone** | Beta |
| **Dependencias** | S4-I6 (forms) |

**Problema:**
El sistema no tiene módulos de administración. No hay gestión de usuarios ni pantalla de configuración del sistema.

**Solución Propuesta:**
Paola diseña los módulos administrativos:
- Pantalla de gestión de usuarios (lista, crear, editar, roles)
- Pantalla de configuración del sistema (turnos, franjas horarias, parámetros)
- Maquetas y prototipos para implementación posterior de Dani (S7-I6)
- Flujo de navegación para sección administrativa

**Criterios de Aceptación:**
- [ ] Prototipo de gestión de usuarios aprobado
- [ ] Prototipo de configuración del sistema aprobado
- [ ] Flujo de navegación administrativa definido
- [ ] Maquetas listas para implementación

---

### S7-I4 — [Middleware/QA] Tests con data real + casos de conflicto

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-middleware |
| **Labels** | `enhancement`, `status:needs-review`, `area-middleware`, `sprint-7` |
| **Milestone** | Beta |
| **Dependencias** | S6-I4 (integration tests) |

**Problema:**
Las pruebas de integración (S6-I4) usaron datos simulados. No hay tests con datos reales ni escenarios de conflicto realistas.

**Solución Propuesta:**
- Tests con datos reales del Liceo Robert Serra
- Escenarios de conflicto: choques horarios, disponibilidad insuficiente, capacidad de aula excedida
- Validar que el sistema reporta correctamente cada tipo de conflicto
- Tests de regresión con data real
- Reporte de cobertura de escenarios

**Criterios de Aceptación:**
- [ ] Tests con datos reales ejecutándose
- [ ] Escenarios de conflicto probados
- [ ] Reporte de conflictos validado
- [ ] Tests de regresión con data real

---

### S7-I5 — [Backend] Optimización consultas SQLite para solver

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-7` |
| **Milestone** | Beta |
| **Dependencias** | S7-I1 (solver perf) |

**Problema:**
El solver necesita consultar datos frecuentemente. Las consultas SQL pueden ser un cuello de botella con datos reales.

**Solución Propuesta:**
- Analizar consultas del solver con EXPLAIN QUERY PLAN
- Agregar índices en columnas usadas en JOINs/WHERE
- Optimizar consultas lentas
- Cache de consultas frecuentes en backend
- Benchmark de consultas antes/después

**Criterios de Aceptación:**
- [ ] Índices creados en columnas clave (teacher_id, classroom_id, subject_id)
- [ ] Tiempo de consultas <100ms con datos reales
- [ ] EXPLAIN QUERY PLAN muestra index scans
- [ ] Cache de consultas frecuentes implementado

---

### S7-I6 — [Frontend] Pantallas de administración (gestión usuarios Qt)

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-7` |
| **Milestone** | Beta |
| **Dependencias** | S7-I3 (admin design) |

**Problema:**
Los prototipos de administración existen (S7-I3) pero no hay implementación Qt de las pantallas de gestión de usuarios y configuración.

**Solución Propuesta:**
Dani implementa las pantallas de administración en Qt:
- Gestión de usuarios: lista con QTableView, formulario crear/editar, roles
- Configuración del sistema: turnos, franjas horarias, parámetros generales
- Basado en prototipos de Paola (S7-I3)
- Conexión a middleware para CRUD de usuarios
- Validación de datos en frontend

**Criterios de Aceptación:**
- [ ] Pantalla de gestión de usuarios funcional
- [ ] CRUD de usuarios operativo vía middleware
- [ ] Pantalla de configuración funcional
- [ ] Diseño alineado con prototipos de Paola
- [ ] Validación de datos en frontend

---

## Sprint 8 — Beta (24-30 Sep)

### S8-I1 — [Backend] Validar consistencia solver (hard+soft) + generar horarios prueba

| Campo | Valor |
|-------|-------|
| **Asignado** | Luis |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-8` |
| **Milestone** | Beta |
| **Dependencias** | S7-I1 (solver perf) |

**Problema:**
El solver tiene todas las restricciones implementadas pero no se ha validado la consistencia del modelo completo (hard + soft). No hay horarios de prueba generados con el modelo final.

**Solución Propuesta:**
- Validar consistencia del modelo completo: hard constraints + soft constraints
- Generar horarios de prueba con diferentes configuraciones
- Verificar que no hay contradicciones entre restricciones
- Test de regresión con casos conocidos
- Documentar límites del solver (máximo profesores, aulas, materias)

**Criterios de Aceptación:**
- [ ] Modelo completo validado (hard + soft)
- [ ] Horarios de prueba generados con diferentes configuraciones
- [ ] Sin contradicciones entre restricciones
- [ ] Tests de regresión pasan
- [ ] Límites del solver documentados

---

### S8-I2 — [Backend] Servicio de reportes internos del solver

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-8` |
| **Milestone** | Beta |
| **Dependencias** | S8-I1 |

**Problema:**
No hay reportes internos que documenten el funcionamiento del solver: estadísticas de resolución, restricciones aplicadas, métricas de la solución.

**Solución Propuesta:**
- Servicio de reportes internos del solver:
  - Estadísticas de resolución (tiempo, nodos explorados, soluciones encontradas)
  - Restricciones aplicadas y su impacto
  - Métricas de la solución (ocupación, conflictos, satisfacción de preferencias)
- Endpoint `GET /solver/report` para consumir desde frontend
- Documentación interna del algoritmo y parámetros

**Criterios de Aceptación:**
- [ ] Reporte con estadísticas de resolución
- [ ] Reporte de restricciones aplicadas
- [ ] Métricas de la solución calculadas
- [ ] Endpoint funcional

---

### S8-I3 — [Frontend] Dashboard administrador + estadísticas visuales

| Campo | Valor |
|-------|-------|
| **Asignado** | Paola |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-8` |
| **Milestone** | Beta |
| **Dependencias** | S7-I6 (admin screens) |

**Problema:**
El dashboard actual es básico sin métricas ni estadísticas visuales. Los administradores necesitan una vista general del sistema.

**Solución Propuesta:**
- Dashboard con:
  - Tarjetas de resumen: total profesores, aulas, materias, horarios generados
  - Gráfico de ocupación de aulas (barras)
  - Tabla de carga horaria por profesor
  - Estadísticas de generación (última generación, tiempo, conflictos)
- Diseño visual con colores y gráficos simples (QWidget personalizados)
- Datos desde backend vía middleware (S8-I2)

**Criterios de Aceptación:**
- [ ] Tarjetas con métricas desde backend
- [ ] Gráfico de ocupación de aulas funcional
- [ ] Tabla de carga horaria por profesor
- [ ] Estadísticas de generación visibles
- [ ] Diseño visual profesional

---

### S8-I4 — [Middleware/QA] Tests de carga + validación tiempos de respuesta

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-middleware |
| **Labels** | `enhancement`, `status:needs-review`, `area-middleware`, `sprint-8` |
| **Milestone** | Beta |
| **Dependencias** | S7-I4 (conflict tests) |

**Problema:**
No se ha probado el sistema bajo carga. No se sabe si los tiempos de respuesta son aceptables con datos reales y múltiples usuarios.

**Solución Propuesta:**
- Tests de carga con datos reales:
  - Simular múltiples requests concurrentes
  - Medir tiempos de respuesta por endpoint
  - Identificar cuellos de botella
- Validar tiempos de respuesta contra objetivos (<100ms CRUD, <5s generación)
- Reporte de performance con recomendaciones

**Criterios de Aceptación:**
- [ ] Tests de carga con datos reales ejecutados
- [ ] Tiempos de respuesta medidos y documentados
- [ ] Cuellos de botella identificados
- [ ] Objetivos de rendimiento validados
- [ ] Reporte de performance

---

### S8-I5 — [Backend] Ajustes finales solver con data real

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-8` |
| **Milestone** | Beta |
| **Dependencias** | S8-I1 |

**Problema:**
El solver funciona con datos de prueba pero puede tener problemas con datos reales: volúmenes grandes, restricciones específicas, casos no previstos.

**Solución Propuesta:**
- Ajustar solver con datos reales del Liceo Robert Serra
- Calibrar pesos de restricciones blandas
- Validar tiempos de respuesta con data real
- Ajustar parámetros del solver (tiempo límite, tolerancia)
- Documentar configuración óptima

**Criterios de Aceptación:**
- [ ] Solver calibrado con datos reales
- [ ] Pesos de restricciones blandas ajustados
- [ ] Tiempos de respuesta aceptables con data real
- [ ] Configuración óptima documentada

---

### S8-I6 — [Frontend] Dashboard administrador Qt + reportes visuales

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-8` |
| **Milestone** | Beta |
| **Dependencias** | S8-I3 |

**Problema:**
Los prototipos del dashboard existen (S8-I3) pero no hay implementación Qt funcional con datos reales del backend.

**Solución Propuesta:**
Dani implementa el dashboard administrador en Qt:
- Tarjetas de resumen con datos desde backend
- Gráficos de barras simples con QPainter o Qt Charts
- Reportes visuales de ocupación y carga horaria
- Selector de período y filtros
- Conexión a middleware para datos en tiempo real
- Basado en prototipos de Paola (S8-I3)

**Criterios de Aceptación:**
- [ ] Dashboard Qt funcional con datos reales
- [ ] Gráficos de ocupación y carga horaria
- [ ] Reportes visuales con filtros
- [ ] Conexión a middleware operativa
- [ ] Diseño alineado con prototipos de Paola

---

## Sprint 9 — Beta (31 Sep-06 Oct)

### S9-I1 — [Backend] Pulir solver + persistencia + bug fixes + ajustes middleware

| Campo | Valor |
|-------|-------|
| **Asignado** | Luis |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-9` |
| **Milestone** | Beta |
| **Dependencias** | S8-I1 (consistency) |

**Problema:**
El solver funciona pero tiene bugs menores, la persistencia no está pulida, y la interfaz con middleware necesita ajustes.

**Solución Propuesta:**
- Bug fixes del solver reportados en pruebas
- Pulir persistencia: manejo de transacciones, rollback en errores
- Ajustar interfaz middleware: contratos de mensajes, códigos de error
- Mejorar logging del solver
- Tests de regresión post-ajustes

**Criterios de Aceptación:**
- [ ] Bugs conocidos corregidos
- [ ] Persistencia con manejo de transacciones
- [ ] Interfaz middleware estable
- [ ] Tests de regresión pasan

---

### S9-I2 — [Backend] Reportes del sistema backend

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-9` |
| **Milestone** | Beta |
| **Dependencias** | S9-I1 |

**Problema:**
No hay servicios backend que generen reportes del sistema: ocupación, carga horaria, materias sin asignar.

**Solución Propuesta:**
- Servicio de reportes backend:
  - Reporte semanal de ocupación de aulas
  - Reporte de carga horaria por profesor
  - Reporte de materias sin asignar
  - Exportación a JSON para consumo frontend
- Consultas SQL optimizadas con GROUP BY y agregaciones

**Criterios de Aceptación:**
- [ ] Reporte de ocupación semanal funcional
- [ ] Reporte de carga por profesor funcional
- [ ] Reporte de materias sin asignar funcional
- [ ] Exportación JSON

---

### S9-I3 — [Frontend] Integración completa frontend → backend vía middleware

| Campo | Valor |
|-------|-------|
| **Asignado** | Paola |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-9` |
| **Milestone** | Beta |
| **Dependencias** | S9-I1 |

**Problema:**
La integración entre frontend y backend es parcial. Hay pantallas que aún no consumen datos reales del backend.

**Solución Propuesta:**
- Completar integración de todas las pantallas frontend con backend vía middleware
- Smoke tests de todas las operaciones CRUD desde la UI
- Verificar que los datos fluyen correctamente en ambos sentidos
- Manejo de errores de red en todas las pantallas
- Cache local para mejorar respuesta

**Criterios de Aceptación:**
- [ ] Todas las pantallas consumen datos reales del backend
- [ ] Smoke tests de todas las operaciones CRUD
- [ ] Manejo de errores de red en todas las pantallas
- [ ] Cache local funcionando

---

### S9-I4 — [Middleware/QA] Tests de integración completos + reporte de estabilidad

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-middleware |
| **Labels** | `enhancement`, `status:needs-review`, `area-middleware`, `sprint-9` |
| **Milestone** | Beta |
| **Dependencias** | S8-I4 (load tests) |

**Problema:**
Las pruebas de integración son parciales. No hay un reporte completo de estabilidad del sistema.

**Solución Propuesta:**
- Tests de integración completos: back + middle + front
- Pruebas de estabilidad: ejecutar el sistema por 24h sin fallos
- Reporte de estabilidad con métricas: uptime, errores, tiempos de respuesta
- Identificar y corregir inestabilidades
- Tests de regresión automatizados

**Criterios de Aceptación:**
- [ ] Tests de integración back+middle+front completos
- [ ] Prueba de estabilidad 24h sin fallos
- [ ] Reporte de estabilidad con métricas
- [ ] Inestabilidades corregidas

---

### S9-I5 — [Backend] Pruebas de aceptación interna + ajustes

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-9` |
| **Milestone** | Beta |
| **Dependencias** | S9-I2 |

**Problema:**
No se han realizado pruebas de aceptación interna del backend. Pueden haber bugs o problemas de usabilidad técnica.

**Solución Propuesta:**
- Pruebas de aceptación interna del backend:
  - Probar todos los endpoints con datos reales
  - Verificar códigos de respuesta y mensajes de error
  - Probar escenarios de error (datos inválidos, servidor caído)
- Ajustes basados en resultados de pruebas
- Documentación de resultados

**Criterios de Aceptación:**
- [ ] Todos los endpoints probados con datos reales
- [ ] Códigos de respuesta verificados
- [ ] Escenarios de error probados
- [ ] Ajustes aplicados

---

### S9-I6 — [Frontend] Smoke tests + validación funcionalidad completa UI

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-9` |
| **Milestone** | Beta |
| **Dependencias** | S9-I3 |

**Problema:**
La UI tiene todas las pantallas implementadas pero no se ha validado que la funcionalidad completa funcione correctamente.

**Solución Propuesta:**
- Smoke tests de todas las pantallas y flujos de la UI
- Validar que todas las funcionalidades están accesibles
- Verificar navegación entre pantallas
- Probar operaciones CRUD desde cada pantalla
- Reporte de issues encontrados
- Correcciones rápidas de bugs críticos

**Criterios de Aceptación:**
- [ ] Smoke tests de todas las pantallas completados
- [ ] Todas las funcionalidades accesibles
- [ ] Navegación correcta entre pantallas
- [ ] CRUD funcional desde todas las pantallas
- [ ] Issues reportados y corregidos

---

## Sprint 10 — Release 1.0 (14-20 Oct)

### S10-I1 — [Backend] Validación con data real del Liceo Robert Serra

| Campo | Valor |
|-------|-------|
| **Asignado** | Luis |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-10` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S9-I1 (polish) |

**Problema:**
El sistema se ha probado con datos simulados y parcialmente reales. Falta una validación exhaustiva con la data completa del Liceo Robert Serra.

**Solución Propuesta:**
- Cargar data completa del Liceo Robert Serra
- Validar que el solver encuentra solución con los datos reales
- Verificar que las restricciones se cumplen con datos reales
- Ajustar parámetros según necesidades específicas del liceo
- Documentar resultados de la validación
- Corregir bugs encontrados durante la validación

**Criterios de Aceptación:**
- [ ] Data completa del Liceo Robert Serra cargada
- [ ] Solver encuentra solución con datos reales
- [ ] Restricciones validadas con datos reales
- [ ] Parámetros ajustados según necesidades del liceo
- [ ] Resultados documentados

---

### S10-I2 — [Backend] Ajustes backend post-validación data real

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-10` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S10-I1 |

**Problema:**
La validación con datos reales (S10-I1) puede revelar problemas en el backend que requieren ajustes.

**Solución Propuesta:**
- Implementar ajustes post-validación identificados en S10-I1
- Optimizar consultas lentas con datos reales
- Corregir bugs de integración con datos reales
- Mejorar manejo de errores para casos reales
- Tests de regresión post-ajustes

**Criterios de Aceptación:**
- [ ] Ajustes post-validación implementados
- [ ] Consultas lentas optimizadas
- [ ] Bugs de integración corregidos
- [ ] Tests de regresión pasan

---

### S10-I3 — [Frontend] Pruebas UI + correcciones usabilidad

| Campo | Valor |
|-------|-------|
| **Asignado** | Paola |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-10` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S9-I3 (integration) |

**Problema:**
La UI es funcional pero no se han realizado pruebas formales de usabilidad. Pueden haber problemas de UX.

**Solución Propuesta:**
- Pruebas UI con usuarios simulados
- Identificar problemas de usabilidad: flujos confusos, botones difíciles de encontrar, mensajes poco claros
- Correcciones de usabilidad basadas en feedback
- Ajustes de diseño: espaciados, colores, tipografía
- Documentar guía de estilo actualizada

**Criterios de Aceptación:**
- [ ] Pruebas UI con usuarios simulados completadas
- [ ] Problemas de usabilidad identificados y corregidos
- [ ] Ajustes de diseño aplicados
- [ ] Guía de estilo actualizada

---

### S10-I4 — [Middleware/QA] Tests exhaustivos con data real + issue tracking

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-middleware |
| **Labels** | `enhancement`, `status:needs-review`, `area-middleware`, `sprint-10` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S9-I4 (integration tests) |

**Problema:**
Los tests existentes no cubren todos los escenarios con datos reales. No hay un sistema formal de seguimiento de issues.

**Solución Propuesta:**
- Tests exhaustivos con data real del Liceo Robert Serra
- Validar que los horarios generados cumplen con las normas del liceo
- Sistema de issue tracking para bugs encontrados
- Priorización y asignación de issues
- Reporte de calidad con métricas

**Criterios de Aceptación:**
- [ ] Tests exhaustivos con data real completados
- [ ] Horarios validados contra normas del liceo
- [ ] Issue tracking funcionando
- [ ] Reporte de calidad generado

---

### S10-I5 — [Backend] Procesamiento y normalización data real

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-backend |
| **Labels** | `enhancement`, `status:needs-review`, `area-backend`, `sprint-10` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S10-I1 |

**Problema:**
Los datos reales del liceo pueden tener inconsistencias: formatos variados, duplicados, datos incompletos.

**Solución Propuesta:**
- Pipeline de procesamiento de datos reales:
  - Normalizar nombres (trim, capitalize)
  - Detectar y fusionar duplicados por email/nombre
  - Validar integridad referencial
  - Log de transformaciones aplicadas
- Carga limpia y consistente en SQLite

**Criterios de Aceptación:**
- [ ] Pipeline de procesamiento funcional
- [ ] Normalización de datos aplicada
- [ ] Duplicados detectados y fusionados
- [ ] Integridad referencial validada
- [ ] Log de transformaciones

---

### S10-I6 — [Frontend] Pruebas UI con usuarios simulados + correcciones

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-10` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S10-I3 |

**Problema:**
Las pruebas UI de Paola (S10-I3) identificaron problemas. Dani debe implementar las correcciones y validar con usuarios.

**Solución Propuesta:**
- Implementar correcciones de usabilidad identificadas en S10-I3
- Pruebas con usuarios simulados (compañeros de equipo)
- Validar flujos completos: login → CRUD → generar → exportar
- Correcciones basadas en feedback adicional
- Ajustes finales de UI

**Criterios de Aceptación:**
- [ ] Correcciones de usabilidad implementadas
- [ ] Pruebas con usuarios simulados completadas
- [ ] Flujos completos validados
- [ ] Feedback adicional incorporado

---

## Sprint 11 — Release 1.0 (21-27 Oct)

### S11-I1 — [Backend] Documentación arquitectura backend + API interna

| Campo | Valor |
|-------|-------|
| **Asignado** | Luis |
| **Área** | area-docs |
| **Labels** | `enhancement`, `status:needs-review`, `area-docs`, `sprint-11` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S10-I1 (real data) |

**Problema:**
No hay documentación de la arquitectura backend ni de la API interna. Solo los desarrolladores conocen la estructura.

**Solución Propuesta:**
- Documentar arquitectura backend: módulos, dependencias, flujo de datos
- Documentar API interna: contratos de mensajes, códigos de operación, formatos JSON
- Diagrama de componentes y secuencia
- Guía de estilo para el backend
- Archivos en `docs/backend/`

**Criterios de Aceptación:**
- [ ] Documento de arquitectura backend completo
- [ ] API interna documentada con ejemplos
- [ ] Diagrama de componentes
- [ ] Guía de estilo backend

---

### S11-I2 — [Frontend] Documentación componentes UI + flujo navegación

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-docs |
| **Labels** | `enhancement`, `status:needs-review`, `area-docs`, `sprint-11` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S10-I6 (UI tests) |

**Problema:**
No hay documentación de los componentes UI, el flujo de navegación ni la estructura del frontend.

**Solución Propuesta:**
Dani documenta:
- Componentes UI: clases Qt, widgets, signals/slots
- Flujo de navegación: pantallas, transiciones, estados
- Conexiones a middleware: qué endpoints consume cada pantalla
- Guía para agregar nuevas pantallas
- Archivos en `docs/frontend/`

**Criterios de Aceptación:**
- [ ] Componentes UI documentados
- [ ] Flujo de navegación documentado
- [ ] Conexiones a middleware documentadas
- [ ] Guía para nuevas pantallas

---

### S11-I3 — [Frontend] Documentación componentes de interfaz

| Campo | Valor |
|-------|-------|
| **Asignado** | Paola |
| **Área** | area-docs |
| **Labels** | `enhancement`, `status:needs-review`, `area-docs`, `sprint-11` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S10-I3 (UI fixes) |

**Problema:**
No hay documentación de los componentes de interfaz: estilos, colores, tipografía, layout.

**Solución Propuesta:**
- Documentar sistema de diseño: paleta de colores, tipografía, espaciados
- Componentes reutilizables: botones, tablas, formularios, diálogos
- Guía de estilos para mantener consistencia visual
- Ejemplos de uso de cada componente

**Criterios de Aceptación:**
- [ ] Sistema de diseño documentado
- [ ] Componentes reutilizables documentados
- [ ] Guía de estilos creada
- [ ] Ejemplos de uso incluidos

---

### S11-I4 — [Middleware/QA] Guía técnica instalación/configuración/despliegue + protocolo QA

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-docs |
| **Labels** | `enhancement`, `status:needs-review`, `area-docs`, `sprint-11` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S10-I4 (real tests) |

**Problema:**
No hay guía técnica para instalar, configurar o desplegar el sistema. Tampoco hay un protocolo de QA documentado.

**Solución Propuesta:**
- Guía técnica de instalación: requisitos, dependencias, compilación
- Guía de configuración: variables de entorno, archivos de configuración
- Guía de despliegue: Docker, servidor, puesta en producción
- Protocolo de QA: cómo probar, qué probar, criterios de aceptación
- Checklist de go-live

**Criterios de Aceptación:**
- [ ] Guía de instalación completa
- [ ] Guía de configuración completa
- [ ] Guía de despliegue completa
- [ ] Protocolo de QA documentado
- [ ] Checklist de go-live

---

### S11-I5 — [Backend] Documentación servicios backend + contratos

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-docs |
| **Labels** | `enhancement`, `status:needs-review`, `area-docs`, `sprint-11` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S11-I1 |

**Problema:**
La documentación de arquitectura (S11-I1) cubre el backend general pero no documenta servicios específicos ni contratos detallados.

**Solución Propuesta:**
- Documentar cada servicio backend: schedule_service, query_service, export_service, etc.
- Contratos de API: endpoints, request/response JSON, códigos de error
- Ejemplos de uso de cada servicio
- Diagrama de secuencia para flujos complejos

**Criterios de Aceptación:**
- [ ] Servicios backend documentados
- [ ] Contratos de API detallados
- [ ] Ejemplos de uso
- [ ] Diagramas de secuencia

---

### S11-I6 — [Frontend] Manual de usuario (borrador)

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-docs |
| **Labels** | `enhancement`, `status:needs-review`, `area-docs`, `sprint-11` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S11-I2, S11-I3 |

**Problema:**
No hay manual de usuario. Los administradores del liceo no tienen documentación sobre cómo usar el sistema.

**Solución Propuesta:**
Dani redacta el borrador del manual de usuario:
- Introducción al sistema
- Ingreso de datos: profesores, aulas, materias
- Configuración de disponibilidad y preferencias
- Generación de horarios
- Exportación de resultados
- Capturas de pantalla
- Basado en la documentación de componentes (S11-I2, S11-I3)

**Criterios de Aceptación:**
- [ ] Borrador del manual de usuario completo
- [ ] Cubre todas las funcionalidades principales
- [ ] Capturas de pantalla incluidas
- [ ] Lenguaje claro para no-técnicos

---

## Sprint 12 — Release 1.0 (28 Oct-04 Nov)

### S12-I1 — [Backend] Revisión técnica manual de usuario

| Campo | Valor |
|-------|-------|
| **Asignado** | Luis |
| **Área** | area-docs |
| **Labels** | `enhancement`, `status:needs-review`, `area-docs`, `sprint-12` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S11-I6 (user manual draft) |

**Problema:**
El borrador del manual de usuario (S11-I6) puede contener errores técnicos, descripciones incorrectas o información desactualizada.

**Solución Propuesta:**
- Revisar manual de usuario desde perspectiva técnica
- Verificar que todas las descripciones son técnicamente correctas
- Corregir errores de terminología
- Validar que los procedimientos descritos funcionan
- Aprobar manual para versión final

**Criterios de Aceptación:**
- [ ] Revisión técnica completada
- [ ] Errores técnicos corregidos
- [ ] Terminología correcta
- [ ] Procedimientos validados
- [ ] Manual aprobado técnicamente

---

### S12-I2 — [Frontend] Manual de usuario (final) + guías rápidas + presentaciones capacitación

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-docs |
| **Labels** | `enhancement`, `status:needs-review`, `area-docs`, `sprint-12` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S11-I6 |

**Problema:**
El borrador del manual existe (S11-I6) pero no está en versión final. No hay guías rápidas ni material de capacitación.

**Solución Propuesta:**
Dani finaliza el manual de usuario:
- Incorporar correcciones de revisión técnica (S12-I1)
- Diseño profesional con portada, índice, secciones
- Guías rápidas de una página para tareas comunes
- Presentaciones de capacitación (diapositivas)
- Formato PDF listo para distribución

**Criterios de Aceptación:**
- [ ] Manual de usuario versión final
- [ ] Guías rápidas para tareas comunes
- [ ] Presentaciones de capacitación
- [ ] Formato PDF profesional

---

### S12-I3 — [Frontend] Guías rápidas + material visual capacitación

| Campo | Valor |
|-------|-------|
| **Asignado** | Paola |
| **Área** | area-docs |
| **Labels** | `enhancement`, `status:needs-review`, `area-docs`, `sprint-12` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S12-I2 |

**Problema:**
No hay material visual de apoyo para la capacitación de los administradores.

**Solución Propuesta:**
Paola diseña material visual:
- Infografías de flujos de trabajo
- Guías rápidas visuales (una página por tarea)
- Material para presentaciones: diagramas, iconos, capturas
- Formato imprimible y digital

**Criterios de Aceptación:**
- [ ] Infografías de flujos de trabajo
- [ ] Guías rápidas visuales
- [ ] Material para presentaciones
- [ ] Formato imprimible y digital

---

### S12-I4 — [Middleware/QA] Validación manual de usuario + consistencia técnica

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-docs |
| **Labels** | `enhancement`, `status:needs-review`, `area-docs`, `sprint-12` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S12-I1, S12-I2 |

**Problema:**
El manual de usuario final (S12-I2) y las guías (S12-I3) pueden tener inconsistencias técnicas o procedimientos incorrectos.

**Solución Propuesta:**
- Validar manual de usuario completo desde perspectiva QA
- Verificar consistencia técnica entre documentación y sistema real
- Probar cada procedimiento descrito en el manual
- Identificar y reportar discrepancias
- Aprobar documentación para entrega

**Criterios de Aceptación:**
- [ ] Manual validado desde perspectiva QA
- [ ] Consistencia técnica verificada
- [ ] Procedimientos probados
- [ ] Discrepancias corregidas
- [ ] Documentación aprobada

---

### S12-I5 — [Backend] Revisión documentación técnica

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-docs |
| **Labels** | `enhancement`, `status:needs-review`, `area-docs`, `sprint-12` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S11-I5 |

**Problema:**
La documentación técnica del backend (S11-I1, S11-I5) puede tener errores, omisiones o información desactualizada.

**Solución Propuesta:**
- Revisar toda la documentación técnica del backend
- Verificar que los contratos de API coinciden con la implementación
- Actualizar documentación desactualizada
- Completar secciones faltantes
- Aprobar documentación técnica

**Criterios de Aceptación:**
- [ ] Documentación técnica revisada
- [ ] Contratos de API verificados contra implementación
- [ ] Documentación actualizada
- [ ] Secciones faltantes completadas
- [ ] Documentación aprobada

---

## Sprint 13 — Release 1.0 (05-11 Nov)

### S13-I1 — [Backend] Consultoría técnica + soporte in-situ

| Campo | Valor |
|-------|-------|
| **Asignado** | Luis |
| **Área** | area-docs |
| **Labels** | `enhancement`, `status:needs-review`, `area-docs`, `sprint-13` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S12-I1 (manual review) |

**Problema:**
El sistema está completo pero pueden surgir dudas técnicas durante la capacitación y entrega. Se requiere soporte especializado del backend lead.

**Solución Propuesta:**
- Luis disponible para consultoría técnica durante la capacitación
- Soporte in-situ para resolver problemas técnicos
- Asistencia en la configuración del sistema en el liceo
- Resolución de bugs críticos que puedan surgir
- Documentación de lecciones aprendidas

**Criterios de Aceptación:**
- [ ] Consultoría técnica disponible durante capacitación
- [ ] Soporte in-situ para configuración
- [ ] Bugs críticos resueltos
- [ ] Lecciones aprendidas documentadas

---

### S13-I2 — [Frontend] Capacitación práctica administradores

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani |
| **Área** | area-frontend |
| **Labels** | `enhancement`, `status:needs-review`, `area-frontend`, `sprint-13` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S12-I2 (user manual) |

**Problema:**
Los administradores del liceo no saben usar el sistema. No ha habido capacitación práctica.

**Solución Propuesta:**
Dani conduce la capacitación práctica:
- Sesión de ingreso de datos: profesores, aulas, materias
- Configuración de disponibilidad y preferencias
- Generación de horarios
- Exportación de resultados (JSON, CSV)
- Responder preguntas y resolver dudas
- Basado en manual de usuario (S12-I2)

**Criterios de Aceptación:**
- [ ] Sesión de capacitación práctica realizada
- [ ] Administradores ingresan datos correctamente
- [ ] Administradores generan horarios
- [ ] Administradores exportan resultados
- [ ] Dudas resueltas

---

### S13-I3 — [Frontend] Capacitación ingreso datos y operación

| Campo | Valor |
|-------|-------|
| **Asignado** | Paola |
| **Área** | area-docs |
| **Labels** | `enhancement`, `status:needs-review`, `area-docs`, `sprint-13` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S12-I3 |

**Problema:**
Los usuarios no están familiarizados con la interfaz. Necesitan capacitación en el ingreso de datos y operación diaria.

**Solución Propuesta:**
Paola apoya la capacitación:
- Enfoque en ingreso de datos y operación diaria
- Demostración de flujos de trabajo comunes
- Material visual de apoyo
- Práctica guiada con datos de ejemplo
- Recopilación de feedback para mejoras

**Criterios de Aceptación:**
- [ ] Capacitación en ingreso de datos realizada
- [ ] Usuarios operan el sistema independientemente
- [ ] Material visual utilizado
- [ ] Feedback recopilado

---

### S13-I4 — [Middleware/QA] Sesión teórica + demo del sistema

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-docs |
| **Labels** | `enhancement`, `status:needs-review`, `area-docs`, `sprint-13` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S12-I4 (manual validation) |

**Problema:**
Los administradores necesitan entender el flujo completo del sistema: cómo los datos fluyen desde el ingreso hasta la generación de horarios.

**Solución Propuesta:**
Manuel prepara y conduce:
- Sesión teórica: arquitectura del sistema, flujo de datos, roles
- Demo completa del sistema: desde login hasta exportación
- Explicación de conceptos clave: restricciones, preferencias, solver
- Sesión de preguntas y respuestas

**Criterios de Aceptación:**
- [ ] Sesión teórica realizada
- [ ] Demo completa del sistema
- [ ] Conceptos clave explicados claramente
- [ ] Preguntas respondidas

---

### S13-I5 — [Backend] Documento de cierre + evaluación final

| Campo | Valor |
|-------|-------|
| **Asignado** | Nicole |
| **Área** | area-docs |
| **Labels** | `enhancement`, `status:needs-review`, `area-docs`, `sprint-13` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S13-I1 |

**Problema:**
No hay documento de cierre del proyecto ni evaluación final del sistema.

**Solución Propuesta:**
- Documento de cierre del proyecto:
  - Resumen de funcionalidades implementadas
  - Lecciones aprendidas
  - Métricas del proyecto (sprints, issues, horas)
- Evaluación final del sistema:
  - Checklist de criterios de aceptación
  - Resultados de pruebas
  - Recomendaciones para futuras versiones
- Formularios de firma de entrega

**Criterios de Aceptación:**
- [ ] Documento de cierre completo
- [ ] Evaluación final realizada
- [ ] Checklist de aceptación completado
- [ ] Formularios de firma listos

---

### S13-I6 — [Middleware/QA] Demo formal del sistema

| Campo | Valor |
|-------|-------|
| **Asignado** | Manuel |
| **Área** | area-docs |
| **Labels** | `enhancement`, `status:needs-review`, `area-docs`, `sprint-13` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S13-I4 |

**Problema:**
Se requiere una demo formal del sistema ante autoridades del liceo para validar que cumple con los requisitos.

**Solución Propuesta:**
- Demo formal con autoridades del liceo
- Recorrido completo por todas las funcionalidades
- Validación de que el sistema cumple requisitos
- Recopilación de feedback y aprobación formal
- Documentación de resultados de la demo

**Criterios de Aceptación:**
- [ ] Demo formal realizada
- [ ] Autoridades validan el sistema
- [ ] Feedback recopilado
- [ ] Aprobación formal documentada

---

### S13-I7 — [Frontend] Capacitación práctica administradores

| Campo | Valor |
|-------|-------|
| **Asignado** | Dani + Paola |
| **Área** | area-docs |
| **Labels** | `enhancement`, `status:needs-review`, `area-docs`, `sprint-13` |
| **Milestone** | Release 1.0 |
| **Dependencias** | S13-I2, S13-I3 |

**Problema:**
La capacitación inicial (S13-I2, S13-I3) puede no haber sido suficiente. Se requiere una sesión formal adicional.

**Solución Propuesta:**
Dani y Paola realizan sesión formal de capacitación:
- Repaso de todas las funcionalidades
- Práctica supervisada con datos reales
- Resolución de dudas específicas
- Evaluación de competencias
- Certificación de usuarios capacitados

**Criterios de Aceptación:**
- [ ] Sesión formal de capacitación realizada
- [ ] Todos los usuarios capacitados
- [ ] Práctica con datos reales completada
- [ ] Usuarios certificados

---

### S13-I8 — [Documentación] Ceremonia de entrega + firma de cierre

| Campo | Valor |
|-------|-------|
| **Asignado** | TODO EL EQUIPO |
| **Área** | area-docs |
| **Labels** | `enhancement`, `status:needs-review`, `area-docs`, `sprint-13` |
| **Milestone** | Release 1.0 |
| **Dependencias** | Todas S13 |

**Problema:**
No hay un acto formal de entrega del proyecto. No hay firma de cierre que valide la aceptación del cliente.

**Solución Propuesta:**
- Ceremonia de entrega con todas las partes interesadas
- Presentación del sistema completo
- Firma de formularios de aceptación
- Entrega de documentación (manual, guías, código)
- Cierre formal del proyecto

**Criterios de Aceptación:**
- [ ] Ceremonia de entrega realizada
- [ ] Formularios de aceptación firmados
- [ ] Documentación entregada
- [ ] Proyecto cerrado formalmente

---

## Resumen

| Sprint | Issues | Milestone |
|--------|:------:|-----------|
| 2 | 6 | MVP Base |
| 3 | 6 | MVP Base |
| 4 | 6 | MVP Base |
| 5 | 6 | MVP Base |
| 6 | 6 | Beta |
| 7 | 6 | Beta |
| 8 | 6 | Beta |
| 9 | 6 | Beta |
| 10 | 6 | Release 1.0 |
| 11 | 6 | Release 1.0 |
| 12 | 5 | Release 1.0 |
| 13 | 8 | Release 1.0 |
| **Total** | **73** | **3 milestones** |

---

> *Documento generado el 28 de junio de 2026.*
> *Sirve como guía para la asignación semanal de issues entre el equipo.*
> *Las issues de Sprint 2 ya fueron creadas en GitHub. Las restantes se crearán al iniciar cada sprint.*
