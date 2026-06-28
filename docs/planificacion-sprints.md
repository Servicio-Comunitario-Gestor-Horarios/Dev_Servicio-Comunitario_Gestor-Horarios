# Planificación de Sprints — Gestor Horarios

> **Aviso importante:** Las issues se crearán en GitHub solo después de discutir y aprobar en equipo.

---

## Labels del proyecto

| Label | Color | Descripción |
|-------|-------|-------------|
| `sprint-2` | `#006b75` | Sprint 2 |
| `sprint-3` | `#006b75` | Sprint 3 |
| `sprint-4` | `#006b75` | Sprint 4 |
| `sprint-5` | `#006b75` | Sprint 5 |
| `sprint-6` | `#006b75` | Sprint 6 |
| `sprint-7` | `#006b75` | Sprint 7 |
| `sprint-8` | `#006b75` | Sprint 8 |
| `sprint-9` | `#006b75` | Sprint 9 |
| `sprint-10` | `#006b75` | Sprint 10 |
| `sprint-11` | `#006b75` | Sprint 11 |
| `sprint-12` | `#006b75` | Sprint 12 |
| `sprint-13` | `#006b75` | Sprint 13 |
| `area-backend` | `#d73a4a` | Tareas del backend |
| `area-frontend` | `#2d9ff3` | Tareas del frontend |
| `area-middleware/qa` | `#fbca04` | Tareas de middleware y QA |
| `area-docs` | `#7057ff` | Tareas de documentación |

---

## Milestones propuestos

| Milestone | Sprints | Objetivo |
|-----------|---------|----------|
| MVP Base | 2–5 | Funcionalidad mínima viable: login, CRUD profesores, CRUD aulas, generación básica de horarios |
| Beta | 6–9 | Validación de restricciones, exportación, dashboard, optimización intermedia |
| Release 1.0 | 10–13 | Carga masiva, reportes avanzados, testing completo, documentación y despliegue |

---

## Sprint 2

| Persona | Issues propuestas | Labels | Dependencias |
|---------|-------------------|--------|--------------|
| Luis | Configurar OR-Tools en backend, definir estructura de datos de horarios | `sprint-2`, `area-backend` | Ninguna |
| Dani | Crear endpoints CRUD de profesores | `sprint-2`, `area-backend` | Definición de datos (Luis) |
| Paola | Maqueta de login y dashboard | `sprint-2`, `area-frontend` | Ninguna |
| Manuel | Setup de Middleware, ruta de health-check | `sprint-2`, `area-middleware/qa` | Ninguna |
| Nicole | Esquema SQLite de profesores y aulas | `sprint-2`, `area-backend` | Definición de datos (Luis) |

---

## Sprint 3

| Persona | Issues propuestas | Labels | Dependencias |
|---------|-------------------|--------|--------------|
| Luis | Algoritmo de asignación básico (CP-SAT) | `sprint-3`, `area-backend` | OR-Tools configurado (Sprint 2) |
| Dani | CRUD de materias | `sprint-3`, `area-backend` | Esquema SQLite (Sprint 2) |
| Paola | Formulario de registro de profesores | `sprint-3`, `area-frontend` | CRUD profesores (Sprint 2) |
| Manuel | Endpoints CRUD desde frontend hacia backend | `sprint-3`, `area-middleware/qa` | CRUD backend (Sprint 2) |
| Nicole | CRUD de aulas (backend) | `sprint-3`, `area-backend` | Esquema SQLite (Sprint 2) |

---

## Sprint 4

| Persona | Issues propuestas | Labels | Dependencias |
|---------|-------------------|--------|--------------|
| Luis | Restricciones de horario (aulas, turnos) | `sprint-4`, `area-backend` | Algoritmo básico (Sprint 3) |
| Dani | CRUD horarios (asignación manual) | `sprint-4`, `area-backend` | CRUD aulas y materias (Sprint 3) |
| Paola | Tabla de visualización d
e horarios | `sprint-4`, `area-frontend` | CRUD horarios (Sprint 4) |
| Manuel | Validación de datos en middleware | `sprint-4`, `area-middleware/qa` | Endpoints CRUD (Sprint 3) |
| Nicole | Servicio de consultas de horarios (backend) | `sprint-4`, `area-backend` | CRUDs implementados (Sprint 3) |

---

## Sprint 5

| Persona | Issues propuestas | Labels | Dependencias |
|---------|-------------------|--------|--------------|
| Luis | Exportar solución a JSON/CSV | `sprint-5`, `area-backend` | Restricciones (Sprint 4) |
| Dani | Endpoints de generación automática | `sprint-5`, `area-backend` | Algoritmo con restricciones (Sprint 4) |
| Paola | Vista de generación y exportación | `sprint-5`, `area-frontend` | Exportación backend (Sprint 5) |
| Manuel | Manejo de errores y respuestas HTTP estándar | `sprint-5`, `area-middleware/qa` | Validación (Sprint 4) |
| Nicole | Exportación de horarios a JSON/CSV (backend) | `sprint-5`, `area-backend` | CRUD horarios (Sprint 4) |

---

## Sprint 6

| Persona | Issues propuestas | Labels | Dependencias |
|---------|-------------------|--------|--------------|
| Luis | Validación de restricciones duplicadas y conflictos | `sprint-6`, `area-backend` | Exportación (Sprint 5) |
| Dani | CRUD de disponibilidad de profesores | `sprint-6`, `area-backend` | CRUD horarios (Sprint 4) |
| Paola | Calendario interactivo por profesor | `sprint-6`, `area-frontend` | Disponibilidad (Sprint 6) |
| Manuel | Caché de consultas frecuentes | `sprint-6`, `area-middleware/qa` | Endpoints estables (Sprint 5) |
| Nicole | Validación de conflictos de disponibilidad (backend) | `sprint-6`, `area-backend` | CRUD disponibilidad (Sprint 6) |

---

## Sprint 7

| Persona | Issues propuestas | Labels | Dependencias |
|---------|-------------------|--------|--------------|
| Luis | Optimización intermedia (múltiples soluciones) | `sprint-7`, `area-backend` | Validación (Sprint 6) |
| Dani | Endpoint para comparar soluciones | `sprint-7`, `area-backend` | Optimización (Sprint 7) |
| Paola | Dashboard con métricas de horarios | `sprint-7`, `area-frontend` | Comparación (Sprint 7) |
| Manuel | Logging estructurado en middleware | `sprint-7`, `area-middleware/qa` | Ninguna |
| Nicole | Comparador de soluciones de horarios (backend) | `sprint-7`, `area-backend` | Optimización (Sprint 7) |

---

## Sprint 8

| Persona | Issues propuestas | Labels | Dependencias |
|---------|-------------------|--------|--------------|
| Luis | Algoritmo de reasignación rápida | `sprint-8`, `area-backend` | Optimización (Sprint 7) |
| Dani | Endpoints de edición masiva | `sprint-8`, `area-backend` | CRUDs existentes |
| Paola | Filtros y búsqueda en tabla de horarios | `sprint-8`, `area-frontend` | Tabla de horarios (Sprint 4) |
| Manuel | Autenticación JWT en middleware | `sprint-8`, `area-middleware/qa` | Ninguna |
| Nicole | Edición masiva de horarios (backend) | `sprint-8`, `area-backend` | CRUDs existentes |

---

## Sprint 9

| Persona | Issues propuestas | Labels | Dependencias |
|---------|-------------------|--------|--------------|
| Luis | Reporte de conflictos no resueltos | `sprint-9`, `area-backend` | Reasignación (Sprint 8) |
| Dani | Endpoint de estadísticas del sistema | `sprint-9`, `area-backend` | Reportes (Sprint 9) |
| Paola | Exportación a PDF de horarios | `sprint-9`, `area-frontend` | Exportación (Sprint 5) |
| Manuel | Rate limiting y protección de endpoints | `sprint-9`, `area-middleware/qa` | Autenticación (Sprint 8) |
| Nicole | Reportes y estadísticas del sistema (backend) | `sprint-9`, `area-backend` | Reporte de conflictos (Sprint 9) |

---

## Sprint 10

| Persona | Issues propuestas | Labels | Dependencias |
|---------|-------------------|--------|--------------|
| Luis | Carga masiva desde Excel/CSV | `sprint-10`, `area-backend` | Ninguna |
| Dani | Validación de datos cargados masivamente | `sprint-10`, `area-backend` | Carga masiva (Sprint 10) |
| Paola | Interfaz de importación con preview | `sprint-10`, `area-frontend` | Carga masiva (Sprint 10) |
| Manuel | Documentación de API (Swagger/OpenAPI) | `sprint-10`, `area-docs` | Endpoints existentes |
| Nicole | Validación y procesamiento de carga masiva (backend) | `sprint-10`, `area-backend` | Carga masiva (Sprint 10) |

---

## Sprint 11

| Persona | Issues propuestas | Labels | Dependencias |
|---------|-------------------|--------|--------------|
| Luis | Algoritmo avanzado con preferencias de profesores | `sprint-11`, `area-backend` | Carga masiva (Sprint 10) |
| Dani | Personalización de restricciones por aula | `sprint-11`, `area-backend` | Restricciones existentes |
| Paola | Asistente paso a paso para generar horarios | `sprint-11`, `area-frontend` | Algoritmo avanzado (Sprint 11) |
| Manuel | Webhooks para notificaciones de cambios | `sprint-11`, `area-middleware/qa` | Ninguna |
| Nicole | Restricciones por preferencias de profesores (backend) | `sprint-11`, `area-backend` | Algoritmo avanzado (Sprint 11) |

---

## Sprint 12

| Persona | Issues propuestas | Labels | Dependencias |
|---------|-------------------|--------|--------------|
| Luis | Refactor y optimización final del solver | `sprint-12`, `area-backend` | Algoritmo avanzado (Sprint 11) |
| Dani | Historial de cambios en horarios | `sprint-12`, `area-backend` | Ninguna |
| Paola | Modo oscuro y personalización de UI | `sprint-12`, `area-frontend` | Ninguna |
| Manuel | Monitoreo y alertas del sistema | `sprint-12`, `area-middleware/qa` | Webhooks (Sprint 11) |
| Nicole | Optimización de consultas e índices SQLite | `sprint-12`, `area-backend` | Funcionalidades completas |

---

## Sprint 13

| Persona | Issues propuestas | Labels | Dependencias |
|---------|-------------------|--------|--------------|
| Luis | Correcciones finales y estabilización | `sprint-13`, `area-backend` | Sprint 12 |
| Dani | Correcciones finales y estabilización | `sprint-13`, `area-backend` | Sprint 12 |
| Paola | Correcciones finales de UI | `sprint-13`, `area-frontend` | Sprint 12 |
| Manuel | Despliegue y configuración del servidor | `sprint-13`, `area-middleware/qa` | Sprint 12 |
| Nicole | Tests de aceptación y documento de cierre | `sprint-13`, `area-docs` | Sprint 12 |
