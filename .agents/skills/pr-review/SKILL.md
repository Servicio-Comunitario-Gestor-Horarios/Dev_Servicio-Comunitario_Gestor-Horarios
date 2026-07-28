---
name: pr-review
description: >
  Revision de Pull Requests especifica del proyecto Gestor-Horarios (C++17/Qt6/CMake/OR-Tools).
  Checklist de codigo, build, tests, arquitectura y convenciones. Incluye instrucciones de correccion.
triggers:
  - "review PR"
  - "revisar PR"
  - "code review"
  - "revision de codigo"
  - "check PR"
  - "revisar codigo"
---

# Skill de Revision de PR — Gestor-Horarios

## 1. Proposito

Esta skill guia la revision de Pull Requests del proyecto Gestor-Horarios. Cubre:

- **Build y CMake**: targets duplicados, archivos huerfanos, missing AUTOMOC, paths incorrectos
- **Codigo C++**: includes faltantes, ODR violations, queries N+1, uso inconsistente de `Resultado<T>`
- **Arquitectura**: separacion de capas, inyeccion por constructor, DTOs como puentes
- **Tests**: uso correcto de `add_qtest()`/`add_gtest()`, aislamiento, edge cases
- **Seguridad y concurrencia**: FK checks, SQL injection, validacion de inputs
- **Estilo**: K&R braces, Doxygen, naming consistency, trailing newlines
- **GitHub**: PR template, branch naming, squash merge, CODEOWNERS

**No cubre:**
- Revision de UX/UI (usar `frontend-design` o `web-design-guidelines`)
- Auditoria de seguridad ofensiva (usar `red-team-tactics`)
- Optimizacion de performance (usar `performance-profiling`)
- Documentacion general (usar `documentation-templates`)

## 2. Contexto del Proyecto

### Arquitectura: Monolito Modular (4 capas)

```
app (ejecutable)
 ├── common (INTERFACE — solo headers, tipos compartidos)
 ├── frontend (STATIC — Qt6 Widgets, UI)
 ├── backend (STATIC — OR-Tools, SQLite, servicios)
 └── middleware (STATIC — IPC QLocalServer/QLocalSocket)
```

**Flujo de dependencias**: `app -> {common, frontend, backend, middleware}`

| Capa | Tipo CMake | Dependencias | Proposito |
|------|-----------|-------------|-----------|
| `common` | INTERFACE | — | Tipos compartidos, codigos de error, enums |
| `frontend` | STATIC | Qt6::Core, Qt6::Widgets, Qt6::Network | UI (QMainWindow, vistas, formularios) |
| `backend` | STATIC | ortools::ortools, Qt6::Core, Qt6::Sql | Optimizacion OR-Tools, DB services, data structs |
| `middleware` | STATIC | Qt6::Core, Qt6::Network | IPC server/client, mensajes, validacion |
| `gestor-horarios` | EXECUTABLE | common, frontend, backend, middleware | Punto de entrada dual-mode |

**Regla critica**: middleware NO linkea frontend ni backend directamente. El ejecutable `app` es quien conecta todo.

### Stack Tecnologico

| Componente | Version/Detalle |
|-----------|----------------|
| Lenguaje | C++17 |
| Framework UI | Qt6 (Core, Sql, Widgets, Network, Test) |
| Build | CMake 3.24+, Ninja generator |
| Optimizacion | OR-Tools 9.15 (CP-SAT solver) |
| Base de datos | SQLite via Qt6::Sql |
| Testing | QTest (primario), Google Test (opcional) |
| Docker | Debian 12-slim, OR-Tools pre-compilado |

### Estructura del Backend

```
src/backend/
├── include/backend/
│   ├── data/          — Aula, FranjaHoraria, Horario, Materia, PlanEstudio, Profesor
│   ├── database/      — DatabaseManager.hpp, migracion.hpp
│   ├── services/      — ServicioAula, ServicioMaterias, ServicioPlanesEstudio, etc.
│   ├── solver/        — (vacio, esperando modelos OR-Tools)
│   └── resultado.hpp  — Resultado<T> patron exito/error
└── src/
    ├── data/          — Serializacion (toJson/fromJson)
    ├── database/      — DatabaseManager.cpp, migracion.cpp (schema SQLite)
    └── services/      — Implementaciones CRUD + validacion
```

## 3. Configuracion del Entorno

### Docker

El entorno de desarrollo corre en Docker (Debian 12-slim). El Dockerfile esta en `docker/Dockerfile.dev`.

### Compilacion

```bash
# Build principal (dentro del container o localmente con dependencias):
cmake -S . -B build -G Ninja
cmake --build build

# Frontend standalone (sin Docker):
cmake -S src/frontend -B build-frontend -G Ninja -DFRONTEND_STANDALONE=ON

# Tests:
cmake --build build
ctest --test-dir build/test --output-on-failure
ctest --test-dir build/test -R aula   # filtrar por patron
```

### Directorios de Build

| Directorio | Proposito |
|-----------|-----------|
| `build/` | Build principal |
| `build-test/` | Tests (si se usa build separado) |
| `build-frontend/` | Frontend standalone (Qt Creator) |
| `build-lite/` | Build ligero |

### Compiler Warnings

Centralizado en `cmake/CompilerWarnings.cmake`. Se aplica via `set_project_warnings(target)`:

- **GCC/Clang**: `-Wall -Wextra -Wpedantic`
- **MSVC**: `/W4 /permissive-`

Todos los targets DEBEN llamar `set_project_warnings(target)` al final de su CMakeLists.txt. No agregar warnings manualmente en targets individuales.

## 4. Convenciones

### Ramas

```
feature/[area]/[descripcion-corta]
bugfix/[descripcion-corta]
hotfix/[descripcion-corta]
```

Areas validas: `backend`, `frontend`, `middleware`, `core`

**Ejemplo**: `feature/backend/servicio-carga-horaria`

### Commits

```
tipo(alcance): descripcion breve
Closes #N
```

| Tipo | Uso |
|------|-----|
| `feat` | Feature nueva |
| `fix` | Bug fix |
| `docs` | Documentacion |
| `style` | Formateo, sin cambio logico |
| `refactor` | Refactor sin cambio funcional |
| `test` | Tests |

Scopes: `backend`, `frontend`, `middleware`, `qa`, `core`

### Naming

| Elemento | Formato | Ejemplo |
|----------|---------|---------|
| Clases | PascalCase | `ServicioAula`, `DatabaseManager`, `Resultado` |
| Funciones/metodos | camelCase | `listarAulas()`, `crearAula()`, `toJson()` |
| Variables | camelCase | `nombre`, `capacidad`, `idAula` |
| Archivos .cpp/.hpp | snake_case o descriptivo | `ServicioAula.cpp`, `franja_horaria.cpp` |
| Constantes | `static constexpr` / `inline const QString` | `static constexpr int CAPACIDAD_MAX = 100` |
| Enums | PascalCase | `enum class Estado` |

### Codigo

- `#pragma once` en todos los headers
- `// --- Section ---` como divisores de secciones
- Comentarios Doxygen `/** @brief */` en API publica
- SQL con queries parametrizados (`:nombre` bindings)
- Servicios aceptan `QSqlDatabase&` por constructor
- Patron `Resultado<T>` para exito/error

### Organizacion de Archivos

- Headers publicos: `include/[modulo]/` (visible para otros modulos)
- Implementacion: `src/` (privada del modulo)
- DTOs separados de structs del dominio solver

## 5. Checklist de Revision

### Build & CMake

| # | Item | Severidad | Que buscar |
|---|------|-----------|-----------|
| B1 | Triple add_library | CRITICAL | Mismo target definido mas de una vez en CMakeLists.txt |
| B2 | Archivos huerfanos | CRITICAL | Archivo .cpp/.h sin referencia en target_sources() |
| B3 | target_link_libraries incorrecto | HIGH | Dependencia faltante o circular entre modulos |
| B4 | Missing AUTOMOC | HIGH | Target con Q_OBJECT que no tiene `set_target_properties(... AUTOMOC ON)` |
| B5 | Paths de test incorrectos | HIGH | Ruta relativa incorrecta en add_qtest/add_gtest |

### Codigo C++

| # | Item | Severidad | Que buscar |
|---|------|-----------|-----------|
| C1 | #include faltante | CRITICAL | Tras reubicar archivo: `#include <QSqlDatabase>`, `#include <QVariant>`, etc. |
| C2 | ODR violations | CRITICAL | Variable/funcion definida en header sin `inline` o `static` |
| C3 | Deteccion por string | HIGH | `if (error.contains("..."))` en vez de usar enum/codigo de error |
| C4 | Queries N+1 | HIGH | `QSqlQuery` dentro de un loop `for` sin batch |
| C5 | Missing transactions | HIGH | Multiples INSERT/UPDATE sin `db.transaction()`/`db.commit()` |
| C6 | Resultado inconsistente | MEDIUM | Servicio que retorna `bool` o `QString` en vez de `Resultado<T>` |
| C7 | Missing trailing newline | LOW | Archivo sin salto de linea al final |

### Arquitectura

| # | Item | Severidad | Que buscar |
|---|------|-----------|-----------|
| A1 | Separacion de capas | CRITICAL | Frontend incluye backend directamente, o backend incluye frontend |
| A2 | Inyeccion por constructor | HIGH | Servicio que abre DB internamente en vez de recibir `QSqlDatabase&` |
| A3 | DTOs como puentes | MEDIUM | Struct del solver usada directamente en DB sin DTO intermedio |
| A4 | UI en backend | CRITICAL | `QWidget`, `QDialog`, `QMainWindow` en src/backend/ |
| A5 | Solver en frontend | CRITICAL | `ortools` en src/frontend/ o include/frontend/ |

### Tests

| # | Item | Severidad | Que buscar |
|---|------|-----------|-----------|
| T1 | Helper correcto | HIGH | `add_qtest()` para Qt-dependent, `add_gtest()` para logica pura |
| T2 | Aislamiento | HIGH | Tests que usan DB real en vez de `QTemporaryDir` |
| T3 | Edge cases | MEDIUM | Solo happy path, sin tests para errores, vacio, duplicados |
| T4 | Includes en test | MEDIUM | `#include "test_xxx.moc"` olvidado en test con Q_OBJECT |
| T5 | Paths en CMakeLists | HIGH | Ruta incorrecta en add_qtest/add_gtest (verificar path relativo) |
| T6 | Coverage minima | LOW | Codigo nuevo sin ningun test asociado |

### Seguridad e Integridad

| # | Item | Severidad | Que buscar |
|---|------|-----------|-----------|
| S1 | FK checks | HIGH | Tabla nueva sin `FOREIGN KEY` o sin `PRAGMA foreign_keys = ON` |
| S2 | SQL injection | CRITICAL | Concatenacion de strings en queries SQL |
| S3 | Validacion de inputs | HIGH | Campo numerico sin CHECK constraint, sin validacion en servicio |
| S4 | Capacidad/limites | MEDIUM | Sin validacion de capacidad maxima en aulas, horarios, etc. |

### Estilo de Codigo

| # | Item | Severidad | Que buscar |
|---|------|-----------|-----------|
| E1 | K&R braces | LOW | Llave de apertura en linea nueva en vez de misma linea |
| E2 | Doxygen | LOW | Funcion publica compleja sin `/** @brief */` |
| E3 | Naming consistency | MEDIUM | Mezcla de ingles/espanol en nombres de variables/funciones |
| E4 | Organizacion de archivos | MEDIUM | Header en src/ o implementacion en include/ |

### GitHub

| # | Item | Severidad | Que buscar |
|---|------|-----------|-----------|
| G1 | PR template | HIGH | PR sin template o checklist incompleta |
| G2 | Branch name | MEDIUM | Rama que no sigue `feature/[area]/desc` o `bugfix/desc` |
| G3 | Squash only | HIGH | Commits multiples sin squash, o merge commit |
| G4 | CODEOWNERS | HIGH | PR sin aprobacion de ambos Tech-Leads |

## 6. Estrategia de Tests

### Decision: QTest vs Google Test

| Escenario | Framework | Helper | Razon |
|-----------|-----------|--------|-------|
| Servicios Qt (QSqlQuery, signals/slots, QIODevice) | QTest | `add_qtest()` | Necesita Qt event loop, moc, links Qt6::Test |
| Data structs con serialization (toJson/fromJson) | QTest | `add_qtest()` | Usa QVariant, QJsonDocument |
| Logica pura (algoritmos, utilidades, sin Qt) | GTest | `add_gtest()` | No depende de Qt, mas liviano |
| Solver OR-Tools (CP-SAT models) | GTest | `add_gtest()` | Logica de optimizacion sin UI |
| Tests IPC (QLocalServer/QLocalSocket) | QTest | `add_qtest()` | Usa Qt event loop y network |

### Firmas de los Helpers (test/CMakeLists.txt)

```cmake
# Helper: registrar un test con QTest
function(add_qtest NAME SOURCE)
    add_executable(${NAME} ${SOURCE})
    set_target_properties(${NAME} PROPERTIES AUTOMOC ON)
    target_link_libraries(${NAME} PRIVATE
        ${ARGN}
        Qt6::Test
        Qt6::Core
    )
    add_test(NAME ${NAME} COMMAND ${NAME})
endfunction()

# Helper: registrar un test con Google Test
function(add_gtest NAME SOURCE)
    if(NOT GTest_FOUND)
        message(WARNING "Skipping ${NAME} — Google Test not available")
        return()
    endif()
    add_executable(${NAME} ${SOURCE})
    target_link_libraries(${NAME} PRIVATE
        ${ARGN}
        GTest::GTest
        GTest::Main
    )
    add_test(NAME ${NAME} COMMAND ${NAME})
endfunction()
```

### Ejemplo de uso en test/CMakeLists.txt

```cmake
# Qt-dependent test (servicio con QSqlQuery):
add_qtest(test_servicio_aula
    backend/test_servicio_aula.cpp
    backend
    Qt6::Sql
)

# Pure C++ test (logica sin Qt):
add_gtest(test_solver_restrictions
    backend/test_solver_restrictions.cpp
    backend
)
```

**Regla**: NUNCA usar `add_executable()` directo en `test/CMakeLists.txt`. Siempre usar `add_qtest()` o `add_gtest()`.

### Patrones de Test del Proyecto

```cpp
// Patron QTest para servicios:
class TestServicioAula : public QObject
{
    Q_OBJECT

private slots:
    void init() {
        m_tempDir = std::make_unique<QTemporaryDir>();
        QVERIFY(m_tempDir->isValid());
        m_db = DatabaseManager(m_tempDir->filePath() + "/test.db");
        QVERIFY(m_db.inicializar());
        m_servicio = ServicioAula(m_db.db());
    }

    void cleanup() {
        m_servicio = ServicioAula(QSqlDatabase());
        m_db.cerrar();
    }

    // Tests con QVERIFY, QCOMPARE

private:
    std::unique_ptr<QTemporaryDir> m_tempDir;
    DatabaseManager m_db;
    ServicioAula m_servicio;
};

#include "test_servicio_aula.moc"
QTEST_MAIN(TestServicioAula)
```

## 7. Instrucciones de Correccion

> **REGLA ABSOLUTA**: Siempre hacer push de fixes a LA MISMA RAMA desde la que se abrio el PR. NUNCA crear una rama nueva ni un PR nuevo.

### Arbol de Decision

```
Error detectado en el PR
│
├── ¿Es error de build?
│   ├── SI → Bloqueante. Aplicar fix y push a la misma rama.
│   │         Ejemplo: add_library duplicado, #include faltante, path incorrecto.
│   │
│   └── NO → continuar
│
├── ¿Es solo estilo?
│   ├── SI → Sugerencia (verde). Comentar en PR, no aplicar fix automatico.
│   │         Ejemplo: trailing newline, Doxygen faltante, K&R braces.
│   │
│   └── NO → continuar
│
├── ¿Es error de logica?
│   ├── SI → Importante (amarillo). Aplicar fix y push a la misma rama.
│   │         Ejemplo: query N+1, transaction faltante, validacion omitida.
│   │
│   └── NO → continuar
│
└── ¿Es error de arquitectura?
    ├── SI → Bloqueante (rojo). Comentar en PR con explicacion detallada.
    │         Ejemplo: frontend incluye backend, solver en UI, circular dependency.
    │         NO aplicar fix automatico — requiere discusion con el autor.
    │
    └── NO → Revisar con criterio
```

### Regla de la Misma Rama

```
1. Clonar el repo (o hacer fetch)
2. Checkout a la rama del PR:
   git checkout feature/backend/nueva-funcionalidad
3. Aplicar los fixes
4. Commitear:
   git add .
   git commit -m "fix(scope): descripcion del fix"
   git push origin feature/backend/nueva-funcionalidad
5. El PR se actualiza automaticamente
```

**NUNCA hacer esto:**
- `git checkout -b fix/nuevo-nombre` (rama nueva = PR nuevo)
- `gh pr create` (PR nuevo para arreglar otro PR)
- Push a `main` o `develop` directamente

### Template de Comentario (Español, tono educativo/supportivo)

```
Hola {autor}! Encontre {N} aspectos que podemos mejorar en este PR:

### Bloqueantes (Build)
- `path/to/file.cpp:42` — {descripcion del error y como arreglarlo}

### Importantes (Calidad)
- `path/to/file.cpp:87` — {sugerencia con justificacion tecnica}

### Sugerencias (Estilo)
- {sugerencia menor}

Puedo ayudarte con los arreglos si lo necesitas. Buen trabajo!
```

### Ejemplos de Buenos Comentarios

**Comentario bueno (especifico, accionable, educativo):**

```
### Bloqueantes (Build)
- `src/backend/CMakeLists.txt:13` — El target `backend` esta definido 3 veces
  (lineas 13, 15, 20). CMake falla con "add_library cannot create target
  'backend' because another target with the same name already exists".
  Solucion: consolidar en una sola definicion con target_sources().

### Importantes (Calidad)
- `src/backend/src/services/ServicioCargaHoraria.cpp:45-60` — Query N+1:
  QSqlQuery ejecutado dentro del loop for. Para 100 materias, son 100 queries.
  Solucion: usar WHERE IN con batch:
  ```cpp
  QSqlQuery query(db);
  query.prepare("SELECT * FROM Materias WHERE id IN (:ids)");
  query.bindValue(":ids", variantList);
  ```
```

**Comentario malo (vago, no accionable, punitivo):**

```
Esto esta mal, arreglenlo. No funciona y es inaceptable.
```

### Ejemplo: Fix de Triple add_library

```cmake
# MAL (3 definiciones):
add_library(backend STATIC)
add_library(backend STATIC src/data/aula.cpp)
add_library(backend STATIC src/services/ServicioAula.cpp)

# BIEN (1 definicion, multiples target_sources):
add_library(backend STATIC)
target_sources(backend PRIVATE
    src/data/aula.cpp
    src/services/ServicioAula.cpp
)
```

### Ejemplo: Fix de Missing #include

```cpp
// MAL (falta include tras reubicar archivo):
#include "ServicioAula.hpp"
// QSqlQuery no esta definido

// BIEN:
#include "ServicioAula.hpp"
#include <QSqlDatabase>
#include <QSqlQuery>
```

### Ejemplo: Fix de Query N+1

```cpp
// MAL (N+1):
for (const auto& id : idsMaterias) {
    QSqlQuery query(db);
    query.prepare("SELECT nombre FROM Materias WHERE id = :id");
    query.bindValue(":id", id);
    query.exec();
    // ...
}

// BIEN (batch):
QSqlQuery query(db);
QString placeholders;
QStringList bindValues;
for (const auto& id : idsMaterias) {
    placeholders.append("?,");
    bindValues.append(id);
}
placeholders.chop(1); // remover ultima coma
query.prepare(QString("SELECT id, nombre FROM Materias WHERE id IN (%1)").arg(placeholders));
for (const auto& val : bindValues) {
    query.addBindValue(val);
}
query.exec();
```

### Ejemplo: Fix de Transaction Faltante

```cpp
// MAL (sin transaction):
for (const auto& aula : aulas) {
    QSqlQuery query(db);
    query.prepare("INSERT INTO Aulas (...) VALUES (...)");
    query.exec();
}

// BIEN (con transaction):
QSqlTransaction tx(db);
for (const auto& aula : aulas) {
    QSqlQuery query(db);
    query.prepare("INSERT INTO Aulas (...) VALUES (...)");
    query.exec();
}
tx.commit();
```

### Advertencias

1. **NUNCA abrir rama nueva ni PR nuevo** para corregir otro PR. Siempre push a la misma rama.
2. **NUNCA usar tono punitivo**. Siempre tono educativo y de apoyo. El autor esta aprendiendo.
3. **NUNCA aplicar fixes de arquitectura automaticamente**. Discutir primero con el autor.
4. **Si el fix es complejo**, ofrecer ayuda: "Puedo ayudarte con los arreglos si lo necesitas."
5. **Si el PR es bueno**, reconocerlo: "Buen trabajo! Encontre solo cosas menores."

## 8. Verificacion del Estado

### Estado del Repo Remoto

```bash
# Verificar estado de la rama del PR:
git fetch origin
git log origin/feature/backend/nueva-funcionalidad --oneline -5

# Verificar que no hay conflicto con develop:
git merge-base --is-ancestor origin/develop origin/feature/backend/nueva-funcionalidad
```

### Estado del PR en GitHub

```bash
# Ver detalles del PR:
gh pr view <numero>

# Ver checks (CI):
gh pr checks <numero>

# Ver diff:
gh pr diff <numero>
```

### Project Board

| Columna | Significado |
|---------|-------------|
| Backlog | Tareas pendientes |
| To-Do | Issues listas para trabajar |
| In Progress | En desarrollo |
| In Review | PR abierto, esperando aprobacion |
| Testing | En verificacion |
| Done | Completadas |

El workflow `pr-project-status.yml` mueve automaticamente el issue:
- PR abierto -> "In Review"
- PR merged -> "Done"

### Verificar CI

- Build debe pasar sin errores
- Tests deben pasar (`ctest --test-dir build/test`)
- No deben haber warnings nuevos (`-Wall -Wextra -Wpedantic`)

## 9. Errores Comunes del Proyecto

Basado en el review del PR #62 y patrones recurrentes:

| # | Error | Frecuencia | Severidad | Fix |
|---|-------|-----------|-----------|-----|
| 1 | Triple `add_library` | Alta | CRITICAL | Consolidar en una sola definicion con `target_sources()` |
| 2 | Missing `#include <QSqlDatabase>` | Alta | CRITICAL | Agregar include explicito al reubicar archivos a capa backend |
| 3 | Archivos huerfanos (.cpp sin CMakeLists ref) | Alta | CRITICAL | Agregar a `target_sources()` o eliminar |
| 4 | Queries N+1 (QSqlQuery en loop) | Media | HIGH | Usar batch query con `WHERE IN` o JOIN |
| 5 | Missing transactions (multiples INSERT sin tx) | Media | HIGH | Envolver en `QSqlTransaction` + `commit()` |
| 6 | Paths incorrectos en test CMakeLists | Media | HIGH | Verificar ruta relativa: `backend/test_xxx.cpp` no `../src/backend/test_xxx.cpp` |
| 7 | Missing trailing newline | Alta | MEDIUM | Agregar newline al final del archivo |
| 8 | No usar `add_qtest()`/`add_gtest()` helper | Baja | MEDIUM | Siempre usar helpers en vez de `add_executable()` directo |

### Detalle de Errores Frecuentes

**1. Triple add_library (CRITICAL)**

El mismo target se define multiples veces en un CMakeLists.txt. CMake falla al intentar crear un target que ya existe.

```cmake
# MAL:
add_library(backend STATIC)           # linea 13
add_library(backend STATIC src/a.cpp) # linea 15 — ERROR: target ya existe
add_library(backend STATIC src/b.cpp) # linea 20 — ERROR: target ya existe

# BIEN:
add_library(backend STATIC)
target_sources(backend PRIVATE
    src/a.cpp
    src/b.cpp
)
```

**2. Missing #include <QSqlDatabase> (CRITICAL)**

Al mover un archivo de capa o reorganizar includes, puede faltar `#include <QSqlDatabase>`. El compilador no da error claro — falla en runtime o con errores crypticos.

```cpp
// Sintoma: " QSqlDatabase: no such driver" o errores de tipo
// Fix: agregar #include <QSqlDatabase> y #include <QSqlQuery>
```

**3. Archivos huerfanos (CRITICAL)**

Un archivo .cpp/.h existe en el filesystem pero no esta referenciado en ningun `target_sources()`. No se compila ni se linkea.

```bash
# Detectar: comparar archivos .cpp en src/ con target_sources() del CMakeLists
find src/backend -name "*.cpp" | sort
# vs las lineas en target_sources del CMakeLists
```

**4. Queries N+1 (HIGH)**

```cpp
// MAL:
for (const auto& profesor : profesores) {
    QSqlQuery q(db);
    q.prepare("SELECT * FROM Profesor_Materia WHERE id_Profesor = :id");
    q.bindValue(":id", profesor.id);
    q.exec();
    // ...
}

// BIEN:
QSqlQuery q(db);
QStringList ids;
for (const auto& p : profesores) ids.append(p.id);
q.prepare("SELECT * FROM Profesor_Materia WHERE id_Profesor IN (:ids)");
q.bindValue(":ids", ids);
q.exec();
```

**5. Missing transactions (HIGH)**

```cpp
// MAL:
db.exec("INSERT INTO Aulas (nombre) VALUES ('A101')");
db.exec("INSERT INTO Aulas (nombre) VALUES ('A102')");
// Si falla la segunda, la primera queda insertada — datos inconsistentes

// BIEN:
QSqlTransaction tx(db);
db.exec("INSERT INTO Aulas (nombre) VALUES ('A101')");
db.exec("INSERT INTO Aulas (nombre) VALUES ('A102')");
tx.commit();
```

**6. Wrong test paths in CMakeLists (HIGH)**

```cmake
# MAL:
add_qtest(test_aula ../src/backend/test_aula.cpp backend)

# BIEN:
add_qtest(test_aula backend/test_aula.cpp backend)
```

**7. Missing trailing newlines (MEDIUM)**

Archivos terminan sin newline. Algunos editores y herramientas (diff, linters) esperan newline al final.

```bash
# Detectar:
for f in $(find src -name "*.cpp" -o -name "*.hpp"); do
    [ -n "$(tail -c 1 "$f")" ] && echo "Missing newline: $f"
done
```

**8. Not using add_qtest() helper (MEDIUM)**

```cmake
# MAL:
add_executable(test_servicio_aula test_servicio_aula.cpp)
target_link_libraries(test_servicio_aula PRIVATE backend Qt6::Test Qt6::Core)
set_target_properties(test_servicio_aula PROPERTIES AUTOMOC ON)
add_test(NAME test_servicio_aula COMMAND test_servicio_aula)

# BIEN:
add_qtest(test_servicio_aula test_servicio_aula.cpp backend)
```
