# Contexto Técnico — Sprint 4 + Mejoras Skill pr-review

> **Estado**: Pendiente de aprobación
> **Fecha**: 2026-07-13
> **Proyecto**: Gestor-Horarios
> **Stack**: C++17, Qt6, CMake, OR-Tools (CP-SAT), SQLite

---

## Tabla de Contenidos

1. [Resumen Ejecutivo](#1-resumen-ejecutivo)
2. [Skill pr-review: 12 Mejoras](#2-skill-pr-review-12-mejoras)
3. [Dependencias Within-Sprint](#3-dependencias-within-sprint)
4. [Issue #25: Modelo CP-SAT + restricciones](#4-issue-25-modelo-cp-sat--restricciones)
5. [Issue #28: CRUD horarios (asignación manual)](#5-issue-28-crud-horarios-asignación-manual)
6. [Issue #30: Servicio de consultas de horarios](#6-issue-30-servicio-de-consultas-de-horarios)
7. [Issue #26: Tests unitarios modelo de datos](#7-issue-26-tests-unitarios-modelo-de-datos)
8. [Issues #22 + #24: Formularios entrada sección](#8-issues-22--24-formularios-entrada-sección)
9. [Matriz de Riesgos](#9-matriz-de-riesgos)
10. [Orden de Ejecución](#10-orden-de-ejecución)
11. [Resumen de Archivos](#11-resumen-de-archivos)
12. [Preguntas Abiertas](#12-preguntas-abiertas)

---

## 1. Resumen Ejecutivo

### Cambio: sprint4-technical-context

**Intención**: Completar el contexto técnico de las 6 issues en la columna "To-Do" del Sprint 4, proporcionando detalle de implementación (archivos, clases, flujo de datos, dependencias) para que el equipo pueda empezar a trabajar.

**Alcance**: 
- Detalle técnico para 6 issues Sprint 4
- 12 mejoras a la skill pr-review
- Grafo de dependencias within-sprint
- Matriz de riesgos

**Resultado esperado**: Documento MD aprobado por el usuario que sirva como guía de implementación.

### Estado Actual del Proyecto

| Capa | Estado |
|------|--------|
| **Backend** | 6 data models (value objects), 5 services CRUD, SQLite 6 tablas, directorios solver vacíos |
| **Frontend** | MainWindow con sidebar, TeacherFormDialog + TeacherListWidget, 4 ViewPlaceholders |
| **Middleware** | IPC stubs para teacher CRUD, messages.h con operaciones definidas |
| **Tests** | QTest framework, 13 archivos de test existentes |

---

## 2. Skill pr-review: 12 Mejoras

### 2.1. Decisión de Arquitectura

**Opción elegida**: Insertar 3 sub-secciones nuevas (5.8 Frontend, 5.9 Middleware, 5.10 Backend) después de la sección 5 existente.

**Alternativas descartadas**:
- Agregar items inline en tablas existentes → Rechazado: mezclar concerns cross-cutting con module-specific confunde al revisor
- Archivo separado → Rechazado: fragmenta el proceso de revisión

**Razón**: Las 7 tablas existentes (B, C, A, T, S, E, G) son concerns cross-cutting. Los 12 nuevos items son module-specific. Agrupar por módulo permite escaneo rápido: "estoy revisando un PR de frontend → ver sección 5.8".

### 2.2. Sección 5.8: Frontend Checklist (5 items)

| # | Item | Severidad | Qué buscar |
|---|------|-----------|-----------|
| F1 | `tr()` i18n check | HIGH | Strings de UI hardcodeadas sin `tr()` — botones, labels, mensajes, placeholders |
| F2 | AUTOUIC/AUTORCC | HIGH | `#include "archivo.ui"` sin `CMAKE_AUTOUIC ON`, o `.qrc` sin `CMAKE_AUTORCC ON` |
| F3 | Form validation | MEDIUM | QDialog con campos sin validadores, sin feedback de error al usuario |
| F4 | Signal/slot patterns | MEDIUM | `SLOT()` macro legacy en vez de `&Class::method` pointer-to-member syntax |
| F5 | Widget file registration | HIGH | Archivo `.cpp`/`.hpp` nuevo en `src/frontend/` sin agregar a `FRONTEND_SOURCES` o `FRONTEND_HEADERS` en CMakeLists.txt |

### 2.3. Sección 5.9: Middleware Checklist (2 items)

| # | Item | Severidad | Qué buscar |
|---|------|-----------|-----------|
| M1 | IPC op registration | HIGH | Operación nueva sin agregar constante en `messages.h` (OP_* pattern) |
| M2 | IPC request validation | MEDIUM | `onReadyRead()` sin validación de campos requeridos antes de procesar |

### 2.4. Sección 5.10: Backend Checklist (5 items)

| # | Item | Severidad | Qué buscar |
|---|------|-----------|-----------|
| K1 | CP-SAT model validation | HIGH | Variable/constraint sin verificar status del modelo, sin manejo de `UNKNOWN`/`INFEASIBLE` |
| K2 | Export format BOM | MEDIUM | CSV export sin BOM UTF-8 (`\xEF\xBB\xBF`) para compatibilidad Excel |
| K3 | Migration patterns | HIGH | Tabla nueva sin `fecha_creacion`/`fecha_modificacion`, sin CHECK constraints, sin migración en `migracion.cpp` |
| K4 | Template method services | MEDIUM | Servicio nuevo que no sigue patrón `Resultado<T>` — usando bool/QString como retorno |
| K5 | Data struct serialization | MEDIUM | Struct nuevo sin `toJson()`/`fromJson()`, o sin round-trip test |

### 2.5. Impacto en Tamaño

- **Actual**: 706 líneas
- **Con mejoras**: ~946 líneas (~240 líneas nuevas)
- **Aceptable**: Sí — la skill es para consumo de agentes, no para browseo humano

### 2.6. Archivo a Modificar

| Archivo | Acción | Descripción |
|---------|--------|-------------|
| `.agents/skills/pr-review/SKILL.md` | Modificar | Insertar secciones 5.8, 5.9, 5.10 después de línea 256, actualizar sección 1 |

---

## 3. Dependencias Within-Sprint

### Grafo de Dependencias

```
Sprint 3 (Completado)
├── #17  Formulario profesores ────────────────┐
├── #18  Prototipos dashboard ─────────────────┤→ Formularios Sprint 4
├── #20  Modelo de datos entidades ────────────┤→ Solver + Tests Sprint 4
├── #21  CRUD de materias ─────────────────────┤→ CRUD horarios Sprint 4
└── #23  CRUD de aulas ────────────────────────┘

Sprint 4 (Actual)
├── #25 Modelo CP-SAT + restricciones ← #20 (Modelo datos)
├── #28 CRUD horarios (asignación manual) ← #23, #21 (CRUDs Sprint 3)
├── #30 Servicio de consultas de horarios ← #28 (CRUD horarios)
├── #26 Tests unitarios modelo de datos ← #20 (Modelo datos)
├── #22 Formularios entrada docente/sección ← #17 (Formulario profesores)
└── #24 Formularios entrada docente y sección (Qt) ← #18 (Prototipos)
```

### Dependencias Críticas Within-Sprint

```
#28 (CRUD horarios) ──→ #30 (Servicio consultas)
       ↑
   Sprint 3 CRUDs (#21, #23)
```

**#30 depende de #28**: No puede empezar sin la tabla `EntradasHorario` y el servicio `ServicioHorario`.

### Fases de Ejecución Paralela

| Fase | Issues | Dependencias |
|------|--------|--------------|
| **Fase 1** (paralela) | #25, #26, #22+#24 | Sin mutuas dependencias |
| **Fase 2** (después de Fase 1) | #28 | Requiere Sprint 3 completado |
| **Fase 3** (después de #28) | #30 | Espera #28 |

**Ruta crítica**: #28 → #30 (2 issues, secuenciales)

---

## 4. Issue #25: Modelo CP-SAT + restricciones

### 4.1. Objetivo

Implementar el modelo CP-SAT básico con restricciones de aulas y turnos para resolver la asignación de horarios.

### 4.2. Problema

OR-Tools está integrado pero no hay un algoritmo que resuelva la asignación de horarios. El modelo de datos (S3-I1) ya definió las entidades y relaciones, ahora es momento de implementar el solver CP-SAT.

### 4.3. Archivos a Crear

| Archivo | Descripción |
|---------|-------------|
| `src/backend/include/backend/solver/solver_horarios.hpp` | Declaración de la clase principal del solver |
| `src/backend/src/solver/solver_horarios.cpp` | Construcción del modelo, creación de variables, Solve() |
| `src/backend/src/solver/restricciones_aulas.cpp` | Restricción de una aula a la vez |
| `src/backend/src/solver/restricciones_turnos.cpp` | Restricciones de disponibilidad horaria |
| `test/test_solver_horarios.cpp` | Tests de respuesta conocida para el solver |

### 4.4. Archivos a Modificar

| Archivo | Cambio |
|---------|--------|
| `src/backend/CMakeLists.txt` | Agregar archivos solver a `target_sources` |
| `test/CMakeLists.txt` | Registrar `test_solver_horarios` |

### 4.5. Diseño de Clases

```cpp
// solver_horarios.hpp
class SolverHorarios {
public:
    struct ResultadoSolver {
        bool satisfactible = false;
        Horario horario;
        std::string mensaje;
    };

    explicit SolverHorarios(const QVector<Aula>& aulas,
                            const QVector<Profesor>& profesores,
                            const QVector<Materia>& materias,
                            const QVector<FranjaHoraria>& franjas);

    ResultadoSolver resolver(int tiempo_limite_seg = 30);

private:
    // CP-SAT variables: bool[(profesor, materia, aula, franja)]
    std::vector<std::vector<operations_research::BoolVar>> m_variables;

    void crearVariables();
    void agregarRestriccionesAulas();    // restricciones_aulas.cpp
    void agregarRestriccionesTurnos();   // restricciones_turnos.cpp
    Horario extraerSolucion() const;
};
```

### 4.6. Flujo de Datos

```
ServicioAula → QVector<Aula>     ─┐
ServicioMaterias → QVector<Materia> ─┤→ SolverHorarios → Horario
ServicioPlanes → QVector<Profesor> ─┤         │
DB Disponibilidad → QVector<FranjaHoraria> ─┘    ↓
                                           Horario.toJson() → persist
```

### 4.7. Decisiones de Diseño

**Decisión: Archivos de restricciones separados**
- **Opción**: Dividir en `restricciones_aulas.cpp` y `restricciones_turnos.cpp`
- **Alternativas**: Archivo solver monolítico
- **Razón**: SRP — cada tipo de restricción es testeable y reemplazable independientemente. Agregar `restricciones_profesores.cpp` en Sprint 5 no tocará archivos existentes.

**Decisión: Matriz BoolVar 4D**
- **Opción**: `BoolVar[profesor][materia][aula][franja]` — matriz de asignación 4D
- **Alternativas**: IntegerVar con dominio; BoolVar plano con hash-map
- **Razón**: BoolVar es natural para CP-SAT; matriz 4D mapea directamente a lógica de restricciones; memoria aceptable para escala escolar (≤20 prof, ≤30 materias, ≤20 aulas, ≤50 franjas = 600K vars worst case, pero CP-SAT maneja sparse).

**Decisión: Solve con timeout**
- **Opción**: Timeout configurable por usuario (default 30s), retornar mejor factible o INFEASIBLE
- **Alternativas**: Sin timeout (podría colgar); 60s fijo
- **Razón**: Previene freeze de UI; 30s es razonable para problemas de escala escolar

### 4.8. Criterios de Aceptación

- [ ] El solver encuentra solución para 5 profesores, 3 aulas, 10 materias en <30s
- [ ] Restricciones básicas se cumplen (sin choques horarios)
- [ ] Solución respeta capacidad de aulas y turnos asignados
- [ ] Si no hay solución posible, solver retorna "infeasible" con reporte
- [ ] La solución se serializa a JSON correctamente
- [ ] Test unitario con caso conocido valida el resultado

### 4.9. Riesgo

| Riesgo | Probabilidad | Impacto | Mitigación |
|--------|-------------|---------|------------|
| Complejidad CP-SAT alta | Media | Alto | TDD con tests de respuesta conocida; archivos de restricciones separados |
| ~3000 variables binarias exceden 30s | Media | Medio | Usar `SetTimeLimit()`; empezar con restricciones mínimas |

---

## 5. Issue #28: CRUD horarios (asignación manual)

### 5.1. Objetivo

Implementar CRUD de horarios con asignaciones manuales como alternativa a la generación automática.

### 5.2. Problema

No hay forma de crear/editar horarios manualmente como alternativa a la generación automática.

### 5.3. Archivos a Crear

| Archivo | Descripción |
|---------|-------------|
| `src/backend/include/backend/services/ServicioHorario.hpp` | Declaración del servicio + HorarioEntradaDTO |
| `src/backend/src/services/ServicioHorario.cpp` | Operaciones CRUD en tabla EntradasHorario |
| `test/test_servicio_horario.cpp` | Suite QTest siguiendo patrón ServicioAula |

### 5.4. Archivos a Modificar

| Archivo | Cambio |
|---------|--------|
| `src/backend/src/database/migracion.cpp` | Agregar bloque `CREATE TABLE EntradasHorario` |
| `src/backend/CMakeLists.txt` | Agregar `src/services/ServicioHorario.cpp` a sources |
| `test/CMakeLists.txt` | Agregar `add_qtest(test_servicio_horario ...)` |

### 5.5. Nueva Tabla DB: EntradasHorario

```sql
CREATE TABLE IF NOT EXISTS EntradasHorario (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    id_Profesor TEXT NOT NULL,
    id_Aula INTEGER NOT NULL,
    id_Materia INTEGER NOT NULL,
    dia INTEGER NOT NULL CHECK(dia BETWEEN 0 AND 6),
    hora_inicio TEXT NOT NULL,
    hora_fin TEXT NOT NULL,
    tipo TEXT NOT NULL DEFAULT 'regular' CHECK(tipo IN ('regular','extra','lab')),
    fecha_creacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    fecha_modificacion TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(dia, hora_inicio, hora_fin, id_Aula)
);
CREATE INDEX IF NOT EXISTS idx_entrada_profesor ON EntradasHorario(id_Profesor);
CREATE INDEX IF NOT EXISTS idx_entrada_aula ON EntradasHorario(id_Aula);
CREATE INDEX IF NOT EXISTS idx_entrada_materia ON EntradasHorario(id_Materia);
CREATE INDEX IF NOT EXISTS idx_entrada_dia ON EntradasHorario(dia);
```

### 5.6. Diseño de Clases

```cpp
// ServicioHorario.hpp
struct HorarioEntradaDTO {
    int id = -1;
    QString idProfesor;
    int idAula = -1;
    int idMateria = -1;
    int dia = 0;           // 0=dom, 1=lun...6=sab
    QString horaInicio;    // "HH:mm"
    QString horaFin;       // "HH:mm"
    QString tipo;          // "regular" | "extra" | "lab"

    Asignacion toAsignacion() const;  // Bridge to solver struct
};

class ServicioHorario {
public:
    explicit ServicioHorario(QSqlDatabase& db);

    Resultado<HorarioEntradaDTO> crearEntrada(int idAula, const QString& idProfesor,
                                               int idMateria, int dia,
                                               const QString& horaInicio, const QString& horaFin,
                                               const QString& tipo = "regular");
    Resultado<HorarioEntradaDTO> obtenerEntrada(int id) const;
    QVector<HorarioEntradaDTO> listarEntradas() const;
    QVector<HorarioEntradaDTO> listarPorProfesor(const QString& idProfesor) const;
    QVector<HorarioEntradaDTO> listarPorAula(int idAula) const;
    QVector<HorarioEntradaDTO> listarPorDia(int dia) const;
    Resultado<HorarioEntradaDTO> actualizarEntrada(int id, ...);
    bool eliminarEntrada(int id);

    // Batch operations for solver persistence
    bool guardarHorarioCompleto(const Horario& horario);
    bool limpiarHorario();

private:
    QSqlDatabase& m_db;
    bool validarEntrada(...) const;
    bool hayConflictoAula(int idAula, int dia, const QString& inicio,
                          const QString& fin, int excludeId = -1) const;
    bool hayConflictoProfesor(const QString& idProfesor, int dia,
                              const QString& inicio, const QString& fin,
                              int excludeId = -1) const;
    HorarioEntradaDTO mapearARecord(const QSqlRecord& record) const;
};
```

### 5.7. Flujo de Datos (CRUD)

```
Frontend ──IPC──→ Backend ──→ ServicioHorario ──→ QSqlQuery ──→ SQLite
                    │
                    ├─ crearEntrada(): validates → checks conflicts → INSERT
                    ├─ guardarHorarioCompleto(): transaction → DELETE all → bulk INSERT
                    └─ limpiarHorario(): DELETE FROM EntradasHorario
```

### 5.8. Decisiones de Diseño

**Decisión: UNIQUE constraint a nivel DB**
- **Opción**: `UNIQUE(dia, hora_inicio, hora_fin, id_Aula)` en EntradasHorario
- **Alternativas**: Solo validación a nivel de aplicación
- **Razón**: Defense-in-depth — previene race conditions; validación a nivel app provee mensajes de error amigables

**Decisión: Operaciones batch para solver**
- **Opción**: `guardarHorarioCompleto()` hace DELETE all + bulk INSERT en transacción
- **Alternativas**: Update basado en diff (delete removed, insert new, update changed)
- **Razón**: Más simple; solver genera horarios completos; updates parciales agregan complejidad sin beneficio a esta escala

### 5.9. Criterios de Aceptación

- [ ] CRUD horarios funcional
- [ ] Validación de conflictos: mismo profesor en dos lugares
- [ ] Validación de conflictos: misma aula dos veces
- [ ] Datos en SQLite correctamente persistidos

### 5.10. Riesgo

| Riesgo | Probabilidad | Impacto | Mitigación |
|--------|-------------|---------|------------|
| Conflictos de concurrencia | Baja | Medio | UNIQUE constraint en DB + validación en servicio |
| Complejidad batch operations | Baja | Bajo | Seguir patrón ServicioAula exactamente |

---

## 6. Issue #30: Servicio de consultas de horarios

### 6.1. Objetivo

Implementar servicio de consultas que abstraiga las consultas de horarios del frontend.

### 6.2. Problema

No hay una capa de servicio que abstraiga las consultas de horarios del frontend.

### 6.3. Archivos a Crear

| Archivo | Descripción |
|---------|-------------|
| `src/backend/include/backend/services/ServicioConsultaHorarios.hpp` | Declaración del servicio de consultas |
| `src/backend/src/services/ServicioConsultaHorarios.cpp` | Consultas complejas con JOINs |
| `test/test_servicio_consulta_horarios.cpp` | Suite QTest |

### 6.4. Archivos a Modificar

| Archivo | Cambio |
|---------|--------|
| `src/backend/CMakeLists.txt` | Agregar servicio a `target_sources` |
| `test/CMakeLists.txt` | Registrar test |

### 6.5. Diseño de Clases

```cpp
// ServicioConsultaHorarios.hpp

struct EntradaConDetalles {
    int id;
    QString nombreProfesor;
    QString nombreAula;
    QString nombreMateria;
    int dia;
    QString horaInicio;
    QString horaFin;
    QString tipo;
};

struct VistaSemanal {
    // indexed by [dia][hora] → list of entries
    QMap<int, QMap<QString, QVector<EntradaConDetalles>>> grilla;
};

class ServicioConsultaHorarios {
public:
    explicit ServicioConsultaHorarios(QSqlDatabase& db);

    QVector<EntradaConDetalles> consultarPorProfesor(const QString& idProfesor) const;
    QVector<EntradaConDetalles> consultarPorAula(int idAula) const;
    QVector<EntradaConDetalles> consultarPorDia(int dia) const;
    QVector<EntradaConDetalles> consultarPorMateria(int idMateria) const;
    VistaSemanal obtenerVistaSemanal() const;

    // Conflict detection
    QVector<EntradaConDetalles> detectarConflictosProfesor(const QString& idProfesor) const;
    QVector<EntradaConDetalles> detectarConflictosAula(int idAula) const;

    // Statistics
    int contarHorasProfesor(const QString& idProfesor) const;
    int contarHorasAula(int idAula) const;
    float利用率Aula(int idAula) const;

private:
    QSqlDatabase& m_db;
    QVector<EntradaConDetalles> ejecutarConsultaConJOIN(const QString& sql,
                                                         const QVariantMap& params) const;
};
```

### 6.6. Flujo de Datos (Consultas)

```
Frontend ──IPC──→ Backend ──→ ServicioConsultaHorarios
                                    │
                                    ├─ JOIN EntradasHorario + Aulas + Profesores + Materias
                                    ├─ Filter by dia/profesor/aula/materia
                                    ├─ GROUP BY for statistics
                                    └─ Return QVector<EntradaConDetalles> or VistaSemanal
```

### 6.7. Decisiones de Diseño

**Decisión: Servicio de consultas separado**
- **Opción**: Servicio separado `ServicioConsultaHorarios` de `ServicioHorario`
- **Alternativas**: Unir en ServicioHorario
- **Razón**: Patrones de consulta (JOINs, agregación) son fundamentalmente diferentes de CRUD (INSERT/UPDATE/DELETE). Archivos separados mantienen cada uno bajo 200 líneas; más fácil optimizar consultas independientemente.

**Decisión: VistaSemanal como estructura QMap**
- **Opción**: `QMap<int, QMap<QString, QVector<...>>>` indexado por día luego hora
- **Alternativas**: Lista plana; clase grid personalizada
- **Razón**: Mapea directamente a UI de grilla semanal (5 días x 12 horas); lookup O(10 para cualquier celda

### 6.8. Criterios de Aceptación

- [ ] Consultas por profesor devuelven horario semanal
- [ ] Consultas por aula devuelven ocupación
- [ ] Resultados en JSON listo para UI
- [ ] Consultas optimizadas (índices SQL)

### 6.9. Dependencia

**#30 depende de #28**: No puede empezar sin la tabla `EntradasHorario` y el servicio `ServicioHorario`.

### 6.10. Riesgo

| Riesgo | Probabilidad | Impacto | Mitigación |
|--------|-------------|---------|------------|
| JOINs complejos lentos | Baja | Medio | Índices en FKs; test con volúmenes de datos realistas |
| Bloqueado por #28 | Alta | Alto | Priorizar #28 en Fase 2 |

---

## 7. Issue #26: Tests unitarios modelo de datos

### 7.1. Objetivo

Implementar tests unitarios para el modelo de datos C++ existente.

### 7.2. Problema

El modelo de datos C++ (S3-I1) no tiene tests unitarios. No hay garantía de que las entidades, relaciones y serialización funcionen correctamente.

### 7.3. Archivos a Crear

| Archivo | Descripción |
|---------|-------------|
| `test/test_edge_cases.cpp` | Tests de casos borde para los 6 data models |

### 7.4. Archivos a Modificar

| Archivo | Cambio |
|---------|--------|
| `test/CMakeLists.txt` | Registrar `test_edge_cases` |

### 7.5. Casos de Prueba

```
FranjaHoraria: dia=0(dom), dia=6(sab), inicio==fin, inicio>fin(invalid)
Aula: capacidad=1(min), capacidad=500(max), nombre=1char, nombre=255char
Materia: horas_semanales=1, horas_semanales=40, requerimientos vacío
Profesor: disponibilidad vacía, materias vacía, nombre con espacios
Asignacion: round-trip through all models, toJson/fromJson consistency
Horario: 0 asignaciones, 100 asignaciones, asignaciones duplicadas
PlanEstudio: codigo vacío, nombre largo, descripcion nula
```

### 7.6. Patrón de Test

```cpp
// test_edge_cases.cpp
class TestEdgeCases : public QObject {
    Q_OBJECT
private slots:
    void franjaHoraria_diaLimite();
    void franjaHoraria_inicioIgualFin();
    void aula_capacidadMinima();
    void materia_requerimientosVacios();
    // ... 15-20 test methods total
};
```

### 7.7. Criterios de Aceptación

- [ ] Tests unitarios para cada entidad del modelo
- [ ] Tests de serialización JSON (round-trip)
- [ ] Tests de relaciones entre entidades
- [ ] Tests de casos borde (datos vacíos, valores extremos)
- [ ] Tests se ejecutan automáticamente en CI

### 7.8. Riesgo

| Riesgo | Probabilidad | Impacto | Mitigación |
|--------|-------------|---------|------------|
| Bajo | Bajo | Bajo | Código de test puro; sin impacto en producción |

---

## 8. Issues #22 + #24: Formularios entrada sección

### 8.1. Objetivo

Crear formularios Qt para entrada de datos de secciones y reemplazar ViewPlaceholders existentes.

### 8.2. Problema

Los prototipos del dashboard y navegación existen (S3-I6) pero no hay formularios Qt funcionales para entrada de datos de sección. Tampoco hay validación de datos de entrada en el frontend.

### 8.3. Nota: Unificación de Issues

**#22 y #24 se unifican en un solo deliverable** porque cubren el mismo dominio (formularios de sección) y tienen dependencias similares.

### 8.4. Archivos a Crear

| Archivo | Descripción |
|---------|-------------|
| `src/frontend/src/forms/section_form_dialog.hpp` | QDialog para crear/editar secciones |
| `src/frontend/src/forms/section_form_dialog.cpp` | Implementación del formulario |
| `src/frontend/src/widgets/section_list_widget.hpp` | QWidget con tabla + panel de detalles |
| `src/frontend/src/widgets/section_list_widget.cpp` | Implementación del widget de lista |

### 8.5. Archivos a Modificar

| Archivo | Cambio |
|---------|--------|
| `src/frontend/src/views/main_window.cpp` | Reemplazar ViewPlaceholder(4) con SectionListWidget |
| `src/frontend/CMakeLists.txt` | Agregar nuevos archivos de source/header |
| `src/middleware/include/middleware/messages.h` | Agregar constantes OP_SECTION_* |

### 8.6. Diseño de Clases

```cpp
// section_form_dialog.hpp — espeja teacher_form_dialog.hpp exactamente
namespace gestor::frontend::forms {
class SectionFormDialog : public QDialog {
    Q_OBJECT
public:
    explicit SectionFormDialog(QWidget *parent = nullptr);
    void cargarDatos(const QString& codigo, const QString& nombre,
                     const QString& descripcion);
signals:
    void seccionGuardada(const QString& codigo, const QString& nombre,
                         const QString& descripcion);
private slots:
    void guardarSeccion();
private:
    QLineEdit *campoCodigo;
    QLineEdit *campoNombre;
    QLineEdit *campoDescripcion;
    QPushButton *botonGuardar;
    QPushButton *botonCancelar;
};
}

// section_list_widget.hpp — espeja teacher_list_widget.hpp
class SectionListWidget : public QWidget {
    Q_OBJECT
public:
    explicit SectionListWidget(QWidget *parent = nullptr);
private slots:
    void abrirFormularioNueva();
    void agregarSeccionATabla(const QString& codigo, const QString& nombre,
                              const QString& descripcion);
private:
    QTableWidget *m_table;
    QPushButton *m_registerButton;
    void setupUi();
};
```

### 8.7. Mensajes Middleware a Agregar

```cpp
// messages.h additions
inline const QString OP_LISTA_SECCIONES   = QStringLiteral("section_list");
inline const QString OP_OBTENER_SECCION   = QStringLiteral("section_get");
inline const QString OP_CREAR_SECCION     = QStringLiteral("section_create");
inline const QString OP_ACTUALIZAR_SECCION = QStringLiteral("section_update");
inline const QString OP_ELIMINAR_SECCION  = QStringLiteral("section_delete");
```

### 8.8. Decisión de Diseño

**Decisión: Espejar patrones existentes exactamente**
- **Opción**: Seguir patrón TeacherFormDialog + TeacherListWidget 1:1
- **Alternativas**: Usar QML; crear clase base abstracta de formularios
- **Razón**: Velocidad del equipo — mismo desarrollador puede mantener; UX consistente; no se necesitan nuevas abstracciones para 2 formularios

### 8.9. Criterios de Aceptación

- [ ] Formulario sección Qt funcional con validación
- [ ] Datos se envían correctamente vía middleware
- [ ] Diseño visual alineado con prototipos
- [ ] Mensajes de error claros para el usuario

### 8.10. Riesgo

| Riesgo | Probabilidad | Impacto | Mitigación |
|--------|-------------|---------|------------|
| Errores CMakeLists | Baja | Medio | Seguir patrón existente exacto: agregar a FRONTEND_SOURCES/FRONTEND_HEADERS |
| ViewPlaceholder replacement | Baja | Bajo | Reemplazar índice 4 en QStackedWidget |

---

## 9. Matriz de Riesgos

| Issue | Riesgo Técnico | Complejidad | Dependencias Bloqueantes | Mitigación |
|-------|---------------|-------------|-------------------------|------------|
| **#25** CP-SAT solver | **ALTO** | ALTO | #20 (Modelo datos Sprint 3) | TDD con tests de respuesta conocida; archivos de restricciones separados |
| **#28** CRUD horarios | **MEDIO** | MEDIO | #23, #21 (CRUDs Sprint 3) | Seguir patrón ServicioAula exactamente; constraint UNIQUE en DB |
| **#30** Consultas horarios | **MEDIO** | MEDIO | #28 (CRUD horarios) | Índices en FKs; test con volúmenes realistas |
| **#26** Tests edge cases | **BAJO** | BAJO | #20 (Modelo datos Sprint 3) | Código de test puro |
| **#22+#24** Formularios | **BAJO** | MEDIO | #17, #18 (Sprint 3) | Espejar TeacherFormDialog 1:1 |

### Resumen de Riesgos

| Riesgo | Probabilidad | Impacto | Mitigación |
|--------|-------------|---------|------------|
| #25 CP-SAT modelo muy complejo | Media | Alto (afecta calidad de schedules) | TDD con known-answer tests; separate constraint files |
| #30 bloqueado por #28 | Alta | Alto (retrasa entrega) | Priorizar #28 en Fase 2 |
| Frontend CMakeLists estructura plana | Baja | Medio | Seguir patrón existente exacto |
| Formularios no alineados con prototipos | Baja | Bajo | Colaboración con Paola para alinear diseño |

---

## 10. Orden de Ejecución

### Fase 1 (Paralela — sin dependencias mutuas)

| Issue | Asignado | Archivos | Esfuerzo Estimado |
|-------|----------|----------|-------------------|
| **#25** CP-SAT solver | Luis | 5 crear + 2 modificar | ALTO |
| **#26** Tests edge cases | Manuel | 1 crear + 1 modificar | BAJO |
| **#22+#24** Formularios | Dani/Paola | 4 crear + 3 modificar | MEDIO |

### Fase 2 (Después de Fase 1)

| Issue | Asignado | Espera | Archivos | Esfuerzo Estimado |
|-------|----------|--------|----------|-------------------|
| **#28** CRUD horarios | Nicole | Sprint 3 completado | 3 crear + 3 modificar | MEDIO |

### Fase 3 (Después de #28)

| Issue | Asignado | Espera | Archivos | Esfuerzo Estimado |
|-------|----------|--------|----------|-------------------|
| **#30** Consultas horarios | Nicole | #28 | 3 crear + 2 modificar | MEDIO |

### Ruta Crítica

```
#28 (CRUD horarios) → #30 (Consultas horarios)
         ↓                    ↓
    Fase 2, Issue 1      Fase 3, Issue 1
```

**Duración mínima**: 3 fases secuenciales. Si #28 se retrasa, #30 se retrasa.

---

## 11. Resumen de Archivos

### Nuevos Archivos de Producción (12)

| Path | Issue |
|------|-------|
| `src/backend/include/backend/solver/solver_horarios.hpp` | #25 |
| `src/backend/src/solver/solver_horarios.cpp` | #25 |
| `src/backend/src/solver/restricciones_aulas.cpp` | #25 |
| `src/backend/src/solver/restricciones_turnos.cpp` | #25 |
| `src/backend/include/backend/services/ServicioHorario.hpp` | #28 |
| `src/backend/src/services/ServicioHorario.cpp` | #28 |
| `src/backend/include/backend/services/ServicioConsultaHorarios.hpp` | #30 |
| `src/backend/src/services/ServicioConsultaHorarios.cpp` | #30 |
| `src/frontend/src/forms/section_form_dialog.hpp` | #22+#24 |
| `src/frontend/src/forms/section_form_dialog.cpp` | #22+#24 |
| `src/frontend/src/widgets/section_list_widget.hpp` | #22+#24 |
| `src/frontend/src/widgets/section_list_widget.cpp` | #22+#24 |

### Nuevos Archivos de Test (4)

| Path | Issue |
|------|-------|
| `test/test_solver_horarios.cpp` | #25 |
| `test/test_servicio_horario.cpp` | #28 |
| `test/test_servicio_consulta_horarios.cpp` | #30 |
| `test/test_edge_cases.cpp` | #26 |

### Archivos Modificados (6)

| Path | Cambios | Issue |
|------|---------|-------|
| `src/backend/src/database/migracion.cpp` | Agregar CREATE TABLE EntradasHorario + indexes | #28 |
| `src/backend/CMakeLists.txt` | Agregar solver + ServicioHorario + ServicioConsultaHorarios | #25, #28, #30 |
| `test/CMakeLists.txt` | Agregar 4 llamadas add_qtest() | todos |
| `src/frontend/CMakeLists.txt` | Agregar section_form_dialog + section_list_widget | #22+#24 |
| `src/frontend/src/views/main_window.cpp` | Reemplazar ViewPlaceholder índice 4 con SectionListWidget | #22+#24 |
| `src/middleware/include/middleware/messages.h` | Agregar constantes OP_SECTION_* | #22+#24 |

---

## 12. Preguntas Abiertas

Antes de implementar, necesito clarificación sobre:

1. **¿El concepto "sección" mapea a la entidad PlanEstudio (ya existe) o es una entidad nueva?** Necesito clarificación del product owner.
2. **Solver timeout** — ¿debería ser configurable desde UI o hardcodeado en 30s?
3. **`guardarHorarioCompleto()`** — ¿debería preservar edits manuales o reemplazar completamente?

---

## Aprobación

Para aprobar este documento, responde con:
- **"Aprobado"** para proceder a implementación
- **"Modifico X"** para indicar cambios necesarios
- **"Rechazo"** para detener el proceso

---

*Documento generado por SDD Orchestrator — Gestor-Horarios*
*Última actualización: 2026-07-13*
