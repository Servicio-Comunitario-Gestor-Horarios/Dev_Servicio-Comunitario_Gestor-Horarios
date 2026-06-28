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
| Luis | Configurar OR-Tools + estructuras de datos | `sprint-2`, `area-backend` | Ninguna |
| Dani | Scaffold Qt + capacitación Qt Creator + wireframes login | `sprint-2`, `area-frontend` | Ninguna |
| Paola | Maqueta de login y dashboard | `sprint-2`, `area-frontend` | Ninguna |
| Manuel | Setup de Middleware + health-check + endpoints CRUD profesores | `sprint-2`, `area-middleware/qa` | Ninguna |
| Nicole | Esquema SQLite profesores + aulas | `sprint-2`, `area-backend` | Definición datos (Luis) |

---

## Sprint 3

| Persona | Issues propuestas | Labels | Dependencias |
|---------|-------------------|--------|--------------|
| Luis | Modelo de datos entidades + diagrama ER + clases C++ | `sprint-3`, `area-backend` | Datos Sprint 2 |
| Dani | Prototipos dashboard + flujo de navegación | `sprint-3`, `area-frontend` | Qt scaffold (Sprint 2) |
| Paola | Formulario registro de profesores | `sprint-3`, `area-frontend` | CRUD profesores (Sprint 2) |
| Manuel | Enrutamiento CRUD frontend → backend | `sprint-3`, `area-middleware/qa` | Endpoints middleware (Sprint 2) |
| Nicole | CRUD de materias + CRUD de aulas | `sprint-3`, `area-backend` | SQLite (Sprint 2) |

---

## Sprint 4

| Persona | Issues propuestas | Labels | Dependencias |
|---------|-------------------|--------|--------------|
| Luis | Modelo CP-SAT + restricciones (aulas, turnos) | `sprint-4`, `area-backend` | Modelo datos (Sprint 3) |
| Dani | Formularios entrada docente y sección (Qt) | `sprint-4`, `area-frontend` | Prototipos dashboard (Sprint 3) |
| Paola | Formularios entrada docente/sección + validación | `sprint-4`, `area-frontend` | Formulario profesores (Sprint 3) |
| Manuel | Tests unitarios modelo de datos | `sprint-4`, `area-middleware/qa` | Modelo datos (Sprint 3) |
| Nicole | CRUD horarios + servicio consultas | `sprint-4`, `area-backend` | CRUDs (Sprint 3) |

---

## Sprint 5

| Persona | Issues propuestas | Labels | Dependencias |
|---------|-------------------|--------|--------------|
| Luis | Restricciones horas semanales + integrar constraints + validar data simulada + exportación solver | `sprint-5`, `area-backend` | CP-SAT (Sprint 4) |
| Dani | Refinamiento formularios + conexión a middleware | `sprint-5`, `area-frontend` | Formularios (Sprint 4) |
| Paola | Formularios aula/materia + tabla datos básica | `sprint-5`, `area-frontend` | Qt scaffold (Sprint 2) |
| Manuel | Validador de datos + tests de restricciones automatizados | `sprint-5`, `area-middleware/qa` | Tests unitarios (Sprint 4) |
| Nicole | Exportación horarios manuales JSON/CSV | `sprint-5`, `area-backend` | CRUD horarios (Sprint 4) |

---

## Sprint 6

| Persona | Issues propuestas | Labels | Dependencias |
|---------|-------------------|--------|--------------|
| Luis | Preferencias profesores (restricciones blandas CP-SAT) | `sprint-6`, `area-backend` | Constraints duras (Sprint 5) |
| Dani | Integración formularios → middleware + refinamiento UX | `sprint-6`, `area-frontend` | Conexión middleware (Sprint 5) |
| Paola | Calendario visual disponibilidad profesor | `sprint-6`, `area-frontend` | Disponibilidad (Sprint 6) |
| Manuel | Pruebas integración front→middle→back + reporte | `sprint-6`, `area-middleware/qa` | Enrutamiento CRUD (Sprint 3) |
| Nicole | CRUD disponibilidad profesores + validación conflictos solver | `sprint-6`, `area-backend` | CRUDs (Sprint 5) |

---

## Sprint 7

| Persona | Issues propuestas | Labels | Dependencias |
|---------|-------------------|--------|--------------|
| Luis | Performance tuning CP-SAT + edge cases + optimización | `sprint-7`, `area-backend` | Preferencias (Sprint 6) |
| Dani | Pantallas administración Qt (gestión usuarios + configuración) | `sprint-7`, `area-frontend` | Integración (Sprint 6) |
| Paola | Módulos administración UI (gestión usuarios + config) | `sprint-7`, `area-frontend` | Formularios (Sprint 5) |
| Manuel | Tests con datos reales + casos de conflicto | `sprint-7`, `area-middleware/qa` | Integración (Sprint 6) |
| Nicole | Validación con datos reales + optimización SQLite | `sprint-7`, `area-backend` | Disponibilidad (Sprint 6) |

---

## Sprint 8

| Persona | Issues propuestas | Labels | Dependencias |
|---------|-------------------|--------|--------------|
| Luis | Validar consistencia solver (hard+soft) + generar horarios prueba | `sprint-8`, `area-backend` | Performance (Sprint 7) |
| Dani | Dashboard administrador Qt + reportes visuales | `sprint-8`, `area-frontend` | Admin screens (Sprint 7) |
| Paola | Dashboard administrador + estadísticas visuales | `sprint-8`, `area-frontend` | Admin módulos (Sprint 7) |
| Manuel | Tests de carga + validación tiempos de respuesta | `sprint-8`, `area-middleware/qa` | Tests data real (Sprint 7) |
| Nicole | Servicio reportes internos + ajustes finales solver | `sprint-8`, `area-backend` | Validación (Sprint 7) |

---

## Sprint 9

| Persona | Issues propuestas | Labels | Dependencias |
|---------|-------------------|--------|--------------|
| Luis | Pulir solver/persistencia + bug fixes + ajustes middleware | `sprint-9`, `area-backend` | Validación (Sprint 8) |
| Dani | Smoke tests + validación funcionalidad completa UI | `sprint-9`, `area-frontend` | Dashboard (Sprint 8) |
| Paola | Integración completa frontend→backend vía middleware + smoke tests | `sprint-9`, `area-frontend` | Dashboard (Sprint 8) |
| Manuel | Tests integración completos + reporte estabilidad | `sprint-9`, `area-middleware/qa` | Tests carga (Sprint 8) |
| Nicole | Reportes sistema + pruebas aceptación interna | `sprint-9`, `area-backend` | Reportes (Sprint 8) |

---

## Sprint 10

| Persona | Issues propuestas | Labels | Dependencias |
|---------|-------------------|--------|--------------|
| Luis | Validación con data real Liceo Robert Serra + ajustes | `sprint-10`, `area-backend` | Pulido (Sprint 9) |
| Dani | Pruebas UI con usuarios simulados + correcciones | `sprint-10`, `area-frontend` | Integración (Sprint 9) |
| Paola | Pruebas UI + correcciones usabilidad | `sprint-10`, `area-frontend` | Integración (Sprint 9) |
| Manuel | Tests exhaustivos con data real + issue tracking | `sprint-10`, `area-middleware/qa` | Tests integración (Sprint 9) |
| Nicole | Procesamiento/normalización data real + ajustes backend | `sprint-10`, `area-backend` | Validación (Sprint 10) |

---

## Sprint 11

| Persona | Issues propuestas | Labels | Dependencias |
|---------|-------------------|--------|--------------|
| Luis | Documentación arquitectura backend + API interna | `sprint-11`, `area-docs` | Sprint 10 |
| Dani | Documentación componentes UI + flujo navegación + borrador manual usuario | `sprint-11`, `area-docs` | Sprint 10 |
| Paola | Documentación componentes de interfaz | `sprint-11`, `area-docs` | Sprint 10 |
| Manuel | Guía técnica instalación/config/despliegue + protocolo QA | `sprint-11`, `area-docs` | Sprint 10 |
| Nicole | Documentación servicios backend + contratos | `sprint-11`, `area-docs` | Sprint 10 |

---

## Sprint 12

| Persona | Issues propuestas | Labels | Dependencias |
|---------|-------------------|--------|--------------|
| Luis | Revisión técnica manual de usuario | `sprint-12`, `area-docs` | Sprint 11 |
| Dani | Manual de usuario (final) + guías rápidas + presentaciones capacitación | `sprint-12`, `area-docs` | Sprint 11 |
| Paola | Guías rápidas + material visual capacitación | `sprint-12`, `area-docs` | Sprint 11 |
| Manuel | Validación manual de usuario + consistencia técnica | `sprint-12`, `area-docs` | Sprint 11 |
| Nicole | Revisión documentación técnica | `sprint-12`, `area-docs` | Sprint 11 |

---

## Sprint 13

| Persona | Issues propuestas | Labels | Dependencias |
|---------|-------------------|--------|--------------|
| Luis | Consultoría técnica + soporte in-situ | `sprint-13`, `area-backend` | Sprint 12 |
| Dani | Capacitación práctica administradores (datos, generación, exportación) | `sprint-13`, `area-frontend` | Sprint 12 |
| Paola | Capacitación ingreso datos y operación | `sprint-13`, `area-frontend` | Sprint 12 |
| Manuel | Sesión teórica + demo del sistema + demo formal | `sprint-13`, `area-middleware/qa` | Sprint 12 |
| Nicole | Documento de cierre + evaluación final + firma | `sprint-13`, `area-docs` | Sprint 12 |
| ALL | Ceremonia de entrega + firma de cierre | `sprint-13`, `area-docs` | Sprint 12 |
