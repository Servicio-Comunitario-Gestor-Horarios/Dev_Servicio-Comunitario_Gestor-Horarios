# Diseno: Skill de Revision de PR

> Change: `crear-skill-revision-pr`
> Fecha: 2026-07-13
> Estado: Diseno tecnico

---

## Enfoque Tecnico

Un solo archivo `SKILL.md` en `.agents/skills/pr-review/`, con frontmatter YAML y 9 secciones principales en Markdown. La skill se activa cuando el agente realiza review de un PR del proyecto, proporcionando contexto especifico del stack, checklist de revision, y guias de correccion.

---

## Decisiones de Arquitectura

### Decision 1: Formato — SKILL.md unico

**Eleccion**: Un solo archivo `SKILL.md` con todas las secciones.

**Alternativas**:
- Multiples archivos (SKILL.md + references/*.md)
- Dividir por capa (skill-backend.md, skill-frontend.md)

**Razon**: El proyecto tiene ~15 archivos por modulo y 4 capas. Una skill unica con 9 secciones es suficiente para cubrir todo el contexto sin fragmentacion. Las skills existentes en el proyecto (`ortools`, `sqlite-database-expert`, `qt-cpp-docs`) siguen este patron de archivo unico.

---

### Decision 2: Idioma — Espanol

**Eleccion**: Todo el contenido de la skill en espanol.

**Razon**: El equipo es hispanohablante. Los commits, branches, PR template, workflow, y documentacion del proyecto estan en espanol. La skill debe ser coherente con el contexto del proyecto.

---

### Decision 3: Separacion de concerns con skills genericas

**Eleccion**: Esta skill es **especifica del proyecto**. No reemplaza `code-review-checklist`, `github-review-pr`, ni `frontend-design`.

**Razon**:
- `code-review-checklist` -> checklist generico de calidad (se usa como referencia)
- `github-review-pr` -> flujo de review en GitHub (se usa para la mecanica del PR)
- `pr-review` (esta) -> contexto del stack C++17/Qt6/CMake/OR-Tools, errores recurrentes del proyecto, convenciones especificas

La skill se activa **junto con** las skills genericas, no las reemplaza.

---

### Decision 4: Frontmatter YAML con trigger claro

**Eleccion**: Frontmatter con `description` que incluye triggers en espanol.

**Razon**: El agente debe poder activar la skill automaticamente cuando se pide review de un PR. La descripcion debe ser lo suficientemente especifica para no activarse en reviews genericas.

---

### Decision 5: Tablas para checklists

**Eleccion**: Checklists en formato de tabla Markdown con columnas: Categoria, Item, Severidad, Como verificar.

**Razon**: Las tablas son mas escaneables que listas anidadas para un checklist largo. El proyecto ya usa tablas extensamente (WORKFLOW_COMPLETO.md, skill-registry.md).

---

### Decision 6: Ejemplos reales del proyecto

**Eleccion**: Incluir ejemplos concretos de archivos del proyecto (no codigo generico).

**Razon**: La propuesta identifica que skills genericas no aportan valor suficiente. Los ejemplos reales (paths de CMakeLists.txt, estructura de carpetas, helpers de test) hacen la skill util desde el primer uso.

---

## Estructura del SKILL.md

### Frontmatter

```yaml
---
name: pr-review
description: >-
  Revision de PR especifica del proyecto Gestor-Horarios (C++17, Qt6, CMake, OR-Tools).
  Activa al revisar un PR del repo. Cubre: build, codigo C++/Qt, CMakeLists,
  tests (QTest/GTest), arquitectura de capas, convenciones, y errores recurrentes.
  NO reemplaza code-review-checklist ni github-review-pr — se usa en conjunto.
version: 1.0.0
tags: [review, pr, c++, qt6, cmake, code-review, project-specific]
---
```

### Seccion 1: Proposito

```markdown
# Skill: Revision de PR — Gestor-Horarios

## Proposito

Esta skill proporciona un checklist de revision especifico para el proyecto
Gestor-Horarios. Se activa cuando un agente revisa un PR del repo,
complementando las skills genericas de review con contexto del stack real.

**Cundo se activa**: Al hacer review de un PR en
`Servicio-Comunitario-Gestor-Horarios/Dev_Servicio-Comunitario_Gestor-Horarios`

**Que NO cubre** (usar otras skills):
- Revision de UI/UX -> `frontend-design`, `web-design-guidelines`
- Auditoria de seguridad completa -> `vulnerability-scanner`
- Performance profiling -> `performance-profiling`
- Flujo de review en GitHub -> `github-review-pr`
```

### Seccion 2: Contexto del Proyecto

```markdown
## Contexto del Proyecto

### Arquitectura (4 capas)

app (ejecutable)
  |-- frontend  (Qt6 Widgets — UI, ventanas, widgets)
  |-- middleware (comunicacion frontend <-> backend, validacion, IPC)
  |-- backend   (OR-Tools, logica de negocio, SQLite via Qt6::Sql)
  -- common    (tipos compartidos entre capas)

### Stack

| Componente | Version | Uso |
|------------|---------|-----|
| C++ | 17 | Estandar del proyecto |
| Qt6 | 6.x | Core, Sql, Widgets, Network, Test |
| CMake | 3.24+ | Build system |
| Ninja | - | Generador de build |
| OR-Tools | 9.15 | Optimizacion (CP-SAT, routing) |
| SQLite | - | Base de datos (via Qt6::Sql) |
| QTest | Qt6::Test | Tests de codigo Qt |
| GTest | Opcional | Tests de logica general |

### Dependencias CMake

CMakeLists.txt raiz
  |-- cmake/CompilerWarnings.cmake  (-Wall -Wextra -Wpedantic)
  |-- src/CMakeLists.txt
  |     |-- src/common/
  |     |-- src/frontend/    (Qt6::Widgets)
  |     |-- src/middleware/   (Qt6::Network)
  |     |-- src/backend/     (ortools::ortools, Qt6::Core, Qt6::Sql)
  |     -- src/app/         (ejecutable principal)
  -- test/CMakeLists.txt    (QTest + GTest helpers)

### Directorios de Build

| Directorio | Proposito |
|------------|-----------|
| `build/` | Build principal |
| `build-test/` | Tests (alternativo) |
```

### Seccion 3: Configuracion del Entorno

```markdown
## Configuracion del Entorno

### Docker

- Archivo: `docker/Dockerfile.dev` (Debian 12-slim)
- Comando: `docker compose -f docker/docker-compose.yml exec dev bash`
- **GTest NO esta instalado** — solo `find_package(GTest QUIET)` lo detecta
  - Si el PR agrega tests GTest, verificar que `libgtest-dev` este en el Dockerfile
  - Si no esta, el `add_gtest()` hace skip silencioso con warning

### Compilacion

```bash
# Configurar
cmake -S . -B build -G Ninja

# Compilar
cmake --build build

# Tests
ctest --test-dir build

# Test especifico
ctest --test-dir build -R franja
```

### Warnings del Compilador

`cmake/CompilerWarnings.cmake` aplica `-Wall -Wextra -Wpedantic` a todos los targets.
Un PR que genere warnings **no debe mergearse** sin justificacion.
```

### Seccion 4: Convenciones

```markdown
## Convenciones

### Ramas

| Tipo | Formato | Ejemplo |
|------|---------|---------|
| Feature | `feature/[area]/[descripcion]` | `feature/backend/#23-crud-aulas` |
| Bugfix | `bugfix/[descripcion]` | `bugfix/horario-mal-generado` |
| Hotfix | `hotfix/[descripcion]` | `hotfix/parche-urgente` |

**Areas**: `backend`, `frontend`, `middleware`, `core`

### Commits

```
tipo(alcance): descripcion breve

cuerpo opcional

Closes #N
```

**Tipos**: `feat`, `fix`, `docs`, `style`, `refactor`, `test`
**Alcances**: `backend`, `frontend`, `middleware`, `qa`, `core`

### Naming

| Elemento | Convencion | Ejemplo |
|----------|------------|---------|
| Clases | PascalCase | `ServicioAula`, `DatabaseManager` |
| Funciones | camelCase | `agregarAula()`, `getById()` |
| Archivos | kebab-case | `servicio-aula.cpp` |
| Variables | camelCase | `cantidadAulas`, `planActual` |
| Constants | UPPER_SNAKE | `MAX_CAPACITY` |
| Headers | `#pragma once` | (no include guards) |

### Estructura de Archivos

- Una clase por archivo
- Headers en `include/<modulo>/`
- Implementacion en `src/<modulo>/`
- Tests en `test/<modulo>/`
```

### Seccion 5: Checklist de Revision

```markdown
## Checklist de Revision

### Build

| # | Item | Severidad | Como verificar |
|---|------|-----------|----------------|
| B1 | Compila sin errores en Ninja | BLOCKER | `cmake --build build` sin errores |
| B2 | Sin warnings nuevos | MAJOR | Revisar output de compilacion |
| B3 | `ctest --test-dir build` pasa | BLOCKER | Todos los tests pasan |
| B4 | No hay targets duplicados en CMakeLists | MAJOR | Revisar `add_executable` y `target_link_libraries` |
| B5 | Todos los archivos .cpp/.h estan en un CMakeLists | MAJOR | Comparar archivos en disco vs referenciados |

### Codigo C++/Qt

| # | Item | Severidad | Como verificar |
|---|------|-----------|----------------|
| C1 | Includes correctos tras reubicacion | BLOCKER | Buscar `#include` de archivos movidos |
| C2 | No hay codigo muerto (comentado o inalcanzable) | MINOR | grep por bloques comentados grandes |
| C3 | No hay variables no usadas | MINOR | Warning `-Wunused-variable` |
| C4 | Doxygen en funciones complejas | MINOR | Verificar `///` o `/** */` |
| C5 | Variables en espanol | MINOR | Revisar identificadores |
| C6 | Sin print/console.log de depuracion | MAJOR | Buscar `qDebug()`, `std::cout`, `printf` |

### CMakeLists.txt

| # | Item | Severidad | Como verificar |
|---|------|-----------|----------------|
| M1 | No hay `add_executable` duplicado | BLOCKER | Buscar nombre del target duplicado |
| M2 | No hay `target_link_libraries` duplicado | MAJOR | Verificar que cada lib solo se linkea una vez |
| M3 | Fuentes referenciadas existen en disco | BLOCKER | `ls` de cada path en `target_sources` |
| M4 | No hay archivos .cpp/.h huerfanos | MAJOR | Comparar `find src/ -name "*.cpp"` vs CMakeLists |
| M5 | `target_include_directories` correcto | MAJOR | Verificar paths PUBLIC vs PRIVATE |
| M6 | `set_project_warnings()` aplicado | MINOR | Verificar al final del CMakeLists del modulo |

### Tests

| # | Item | Severidad | Como verificar |
|---|------|-----------|----------------|
| T1 | Tests pasan en CI/local | BLOCKER | `ctest --test-dir build` |
| T2 | Framework correcto (QTest vs GTest) | MAJOR | Ver seccion Estrategia de Tests |
| T3 | Path del test en CMakeLists coincide | BLOCKER | Verificar `add_qtest()` o `add_gtest()` |
| T4 | Test tiene assertions validas | MAJOR | No solo `QVERIFY(true)` |
| T5 | Test limpia recursos (DB, archivos) | MINOR | Verificar cleanup en `cleanup()` o RAII |

### Arquitectura

| # | Item | Severidad | Como verificar |
|---|------|-----------|----------------|
| A1 | Respeta la separacion de capas | BLOCKER | Backend no incluye frontend, etc. |
| A2 | No hay dependencias circulares | MAJOR | Revisar `target_link_libraries` |
| A3 | OR-Tools solo en backend | BLOCKER | `grep -r "ortools" src/` — solo en backend/ |
| A4 | SQLite solo via Qt6::Sql | MAJOR | No usar sqlite3.h directo |
| A5 | Common no depende de otros modulos | MAJOR | Verificar src/common/CMakeLists |

### Seguridad

| # | Item | Severidad | Como verificar |
|---|------|-----------|----------------|
| S1 | Queries parametrizadas (sin concatenacion) | BLOCKER | Buscar `QString("...%1...").arg()` en SQL |
| S2 | No hay secrets hardcoded | BLOCKER | Buscar tokens, passwords, keys |
| S3 | Inputs validados antes de usar | MAJOR | Verificar puntos de entrada |

### GitHub

| # | Item | Severidad | Como verificar |
|---|------|-----------|----------------|
| G1 | PR apunta a `develop` (no a `main`) | BLOCKER | Verificar base branch |
| G2 | PR vinculado a issue (`Closes #N`) | MAJOR | Verificar descripcion del PR |
| G3 | PR tiene descripcion clara | MINOR | Verificar body del PR |
| G4 | CODEOWNERS aprobado | BLOCKER | Verificar status checks |
| G5 | Merge es Squash | INFO | Recordatorio para el reviewer |
```

### Seccion 6: Estrategia de Tests

```markdown
## Estrategia de Tests

### Cundo usar QTest vs GTest

| Escenario | Framework | Helper en CMake | Ejemplo |
|-----------|-----------|-----------------|---------|
| Servicios con Qt (SQL, signals/slots) | QTest | `add_qtest()` | `test_servicio_aula.cpp` |
| Datos con serializacion Qt | QTest | `add_qtest()` | `test_franja_horaria.cpp` |
| Logica pura, utilidades, sin Qt | GTest | `add_gtest()` | `test_solver.cpp` (futuro) |
| Mocks complejos | GTest | `add_gtest()` | (futuro) |

### Convencion de nombres de tests

```
test_<modulo>_<nombre>.cpp     -> en test/backend/ o test/frontend/
test_<servicio>.cpp            -> tests de servicios del backend
```

### Helpers en test/CMakeLists.txt

```cmake
# QTest — para codigo que usa Qt
add_qtest(test_backend_aula
    backend/test_aula.cpp
    backend          # target a testear
)

# GTest — para logica general (opcional)
add_gtest(test_backend_solver
    backend/test_solver.cpp
    backend
)
```

### Si el PR agrega tests GTest

1. Verificar que `libgtest-dev` este en `docker/Dockerfile.dev`
2. Si no esta, agregarlo: `libgtest-dev \`
3. El `find_package(GTest QUIET)` ya maneja el caso de no encontrado
4. Verificar que el test compila: `cmake --build build -t test_<nombre>`
```

### Seccion 7: Instrucciones de Correccion

```markdown
## Instrucciones de Correccion

### Reglas Generales

1. **Commentar con sugerencia concreta** — no solo "esto esta mal", sino como corregirlo
2. **Usar formato de diff en el comentario** — el reviewer puede aplicar el cambio directo
3. **Priorizar BLOCKER sobre MINOR** — un BLOCKER detiene el merge

### Correcciones Comunes

#### Archivo huerfano (no esta en CMakeLists.txt)

```bash
# Detectar
find src/<modulo>/ -name "*.cpp" | while read f; do
  grep -q "$f" src/<modulo>/CMakeLists.txt || echo "HUERFANO: $f"
done
```

**Fix**: Agregar `src/<ruta>/archivo.cpp` al `target_sources()` correspondiente.

#### Target duplicado en CMakeLists.txt

**Fix**: Verificar que cada `add_executable` y `add_library` aparezca solo una vez.
Si un archivo se movio de modulo, eliminar la entrada vieja.

#### Include faltante tras reubicacion

**Fix**: Actualizar el `#include` para apuntar al nuevo path.
Verificar `target_include_directories` del modulo destino.

#### Path de test incorrecto

**Fix**: Verificar que el path en `add_qtest()` o `add_gtest()` coincide con
la ubicacion real del archivo de test en `test/`.

#### Test GTest sin GTest disponible

**Fix**: Agregar `libgtest-dev` al `docker/Dockerfile.dev`:
```dockerfile
RUN apt-get update && apt-get install -y --no-install-recommends \
    ...
    libgtest-dev \
    && apt-get clean
```

### Formato de Comentarios en el PR

```markdown
**[BLOCKER]** Archivo `src/backend/src/nuevo_servicio.cpp` no esta referenciado en
`src/backend/CMakeLists.txt`. Agregar al `target_sources(backend PRIVATE ...)`.

```diff
 target_sources(backend PRIVATE
     ...
+    src/services/NuevoServicio.cpp
 )
```

**[MAJOR]** Query potencialmente N+1 en `ServicioAula::obtenerTodas()`.
Verificar que se usa una sola query con JOIN en lugar de N queries individuales.
```
```

### Seccion 8: Verificacion del Estado

```markdown
## Verificacion del Estado

### Repo Remoto

```bash
# Verificar que la rama remota existe
git remote -v
git branch -r | grep <nombre-rama>

# Verificar status del PR
gh pr view <PR-number> --json statusCheckRollup,reviewDecision,mergeable

# Verificar que no hay conflictos
gh pr view <PR-number> --json mergeable
```

### Project Board

| Columna | Trigger | Verificar |
|---------|---------|-----------|
| In Review | PR vinculado a issue (`Closes #N`) | Issue en columna "In Review" |
| Done | PR mergeado | Issue en columna "Done" |

### Auto-move workflow

- `Closes #N` en el PR -> issue se mueve a "In Review"
- PR mergeado -> issue se mueve a "Done"
- Verificar: `.github/workflows/auto-move-project.yml`

### CODEOWNERS

```
* @Servicio-Comunitario-Gestor-Horarios/Tech-Leads
```

Todos los PRs necesitan aprobacion de ambos Tech-Leads (Luis y Daniel).
```

### Seccion 9: Errores Comunes del Proyecto

```markdown
## Errores Comunes

Basado en PRs pasados (PR #62 y otros):

| Error | Frecuencia | Severidad | Deteccion |
|-------|-----------|-----------|-----------|
| Archivo .cpp/.h sin referencia en CMakeLists.txt | Alta | BLOCKER | `find` vs `grep` en CMakeLists |
| Target duplicado en `add_executable` | Media | BLOCKER | Buscar nombre de target duplicado |
| Include faltante tras mover archivo | Alta | BLOCKER | Compilacion falla |
| Query N+1 en acceso a SQLite | Media | MAJOR | Revisar patrones de loop + query |
| Path de test incorrecto | Media | BLOCKER | `ctest` falla |
| Rama no sigue convencion `feature/` | Baja | MAJOR | Verificar nombre de rama |
| Commits sin lint o con warnings | Media | MAJOR | Output de compilacion |
| `libgtest-dev` no en Dockerfile | Baja | MAJOR | `find_package(GTest)` falla |
```

---

## Cambios en Archivos

| Archivo | Accion | Descripcion |
|---------|--------|-------------|
| `.agents/skills/pr-review/SKILL.md` | **Crear** | Skill principal con las 9 secciones |
| `docker/Dockerfile.dev` | **Modificar** | Agregar `libgtest-dev` a la lista de paquetes apt |
| `.atl/skill-registry.md` | **Actualizar** | Registrar `pr-review` en la tabla de User Skills |

---

## Estrategia de Testing

La skill en si no tiene tests unitarios (es un archivo Markdown). La verificacion es:

1. **Carga exitosa**: El agente carga la skill sin errores
2. **Activacion correcta**: Se activa al revisar un PR del proyecto
3. **Contenido completo**: Las 9 secciones estan presentes y con contenido real
4. **Dockerfile funcional**: `docker compose build --no-cache` pasa con `libgtest-dev`
5. **Skill registry actualizado**: `.atl/skill-registry.md` incluye `pr-review`

---

## Flujo de Ejecucion

```
Agente recibe solicitud de review de PR
         |
         v
Cargar skills: pr-review + code-review-checklist + github-review-pr
         |
         v
Leer PR: diff, descripcion, estado, checks
         |
         v
Aplicar checklist de pr-review:
  |-- Build: B1-B5
  |-- Codigo: C1-C6
  |-- CMakeLists: M1-M6
  |-- Tests: T1-T5
  |-- Arquitectura: A1-A5
  |-- Seguridad: S1-S3
  -- GitHub: G1-G5
         |
         v
Hay BLOCKERs?
  |-- Si -> Commentar en PR con sugerencia concreta -> Request changes
  -- No -> Hay MAJORs?
              |-- Si -> Commentar con sugerencias -> Request changes
              -- No -> Commentar OK -> Approve
         |
         v
Verificar estado:
  |-- CODEOWNERS aprobado
  |-- Project board: issue en "In Review"
  -- No hay conflictos
```

---

## Riesgos

| Riesgo | Probabilidad | Mitigacion |
|--------|-------------|------------|
| Skill demasiado generica | Media | Incluir ejemplos concretos del proyecto, no solo buenas practicas |
| Checklist incompleto | Baja | Revisar con PRs reales del proyecto despues de crear la skill |
| GTest no disponible en Docker | Baja | Verificar con `docker compose build --no-cache` antes de merge |
| Conflictos con skills existentes | Media | Documentar que esta skill ESPECIFICA del proyecto, las otras son genericas |
| Contenido del SKILL.md muy largo | Baja | Usar tablas compactas, evitar repeticiones, referenciar secciones |

---

## Criterios de Exito

- [ ] `.agents/skills/pr-review/SKILL.md` creado con las 9 secciones completas
- [ ] `docker/Dockerfile.dev` incluye `libgtest-dev` explicitamente
- [ ] `.atl/skill-registry.md` registra la nueva skill
- [ ] La skill se activa correctamente al hacer review de un PR del proyecto
- [ ] El checklist cubre los 5 errores comunes identificados en la exploracion
- [ ] La estrategia de tests (QTest vs GTest) esta clara con ejemplos
