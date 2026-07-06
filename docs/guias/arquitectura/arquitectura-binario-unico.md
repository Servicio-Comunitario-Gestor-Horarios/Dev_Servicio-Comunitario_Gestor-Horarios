# Arquitectura Binario Único — Gestor-Horarios

> **Documento**: Arquitectura destino del cambio `arquitectura-binario-unico`
> **Fecha**: 5 de julio de 2026
> **Autor**: Luis Rojas - Teach-lead
> **Estado**: Propuesta técnica

---

## Índice

1. [Resumen Ejecutivo](#1-resumen-ejecutivo)
2. [Arquitectura Actual](#2-arquitectura-actual)
3. [Problemas Detectados](#3-problemas-detectados)
4. [Arquitectura Destino](#4-arquitectura-destino)
5. [Alternativa Elegida: QProcess](#5-alternativa-elegida-qprocess)
6. [Diagrama de Componentes](#6-diagrama-de-componentes)
7. [Flujo de Arranque](#7-flujo-de-arranque)
8. [Mecanismo de Crash Recovery](#8-mecanismo-de-crash-recovery)
9. [Cambios Archivo por Archivo](#9-cambios-archivo-por-archivo)
10. [Especificación CMake](#10-especificación-cmake)
11. [Compilación Condicional](#11-compilación-condicional)
12. [Plan de Implementación (5 Fases)](#12-plan-de-implementación-5-fases)
13. [Evaluación de Riesgos](#13-evaluación-de-riesgos)
14. [Comandos de Compilación](#14-comandos-de-compilación)
15. [Impacto en Issues del Backlog](#15-impacto-en-issues-del-backlog)
16. [Archivos de Referencia](#16-archivos-de-referencia)

---

## 1. Resumen Ejecutivo

El proyecto Gestor-Horarios genera actualmente **2 ejecutables independientes** que se comunican por IPC (QLocalSocket). Esto obliga a todo desarrollador a tener Docker (con OR-Tools y Google Test) para compilar, incluso si solo trabajan en la interfaz gráfica.

**Este cambio propone consolidar el sistema en un ÚNICO binario distribuible**, manteniendo el aislamiento de procesos (backend y frontend en procesos separados) y habilitando la compilación condicional para que el equipo de frontend pueda compilar sin Docker.

### ¿Qué cambia?

| Aspecto | Hoy | Después |
|---------|:---:|:-------:|
| **Binarios generados** | 2 | **1** |
| **Entry point** | Cada ejecutable tiene su `main()` | Un solo `main()` con dual-mode |
| **Aislamiento procesos** | Manual (el usuario lanza ambos) | Auto (QProcess lanza backend) |
| **Frontend standalone** | Sí (ya funciona) | Sí + flag `FRONTEND_STANDALONE` |
| **OR-Tools** | Requerido siempre | Opcional (`BUILD_BACKEND`) |
| **Middleware IPC** | Solo entre procesos | **Sin cambios** (se reutiliza) |

---

## 2. Arquitectura Actual

```
┌──────────────────────────┐      QLocalSocket       ┌──────────────────────────┐
│  gestor-horarios-        │  ────────────────────→   │  gestor-horarios         │
│  frontend                │  ←────────────────────   │  (backend)               │
│                          │      JSON over IPC       │                          │
│  ┌────────────────────┐  │                          │  ┌────────────────────┐  │
│  │ QApplication       │  │                          │  │ InternalServer     │  │
│  │  ├─ LoginDialog    │  │                          │  │  (QLocalServer)   │  │
│  │  └─ MainWindow     │  │                          │  └────────────────────┘  │
│  └────────────────────┘  │                          │  ┌────────────────────┐  │
│  ┌────────────────────┐  │                          │  │ backend/           │  │
│  │ InternalClient     │──│──────────────────────────│─→│  (OR-Tools)        │  │
│  │  (QLocalSocket)    │  │                          │  └────────────────────┘  │
│  └────────────────────┘  │                          │  ┌────────────────────┐  │
│                          │                          │  │ middleware/        │  │
│  Dependencias:           │                          │  │  (IPC server)     │  │
│  ├── Qt6::Core           │                          │  └────────────────────┘  │
│  └── Qt6::Widgets        │                          │                          │
│                          │                          │  Dependencias:           │
│  (SIN backend)           │                          │  ├── ortools::ortools    │
│                          │                          │  ├── Qt6::Core           │
│                          │                          │  └── Qt6::Network        │
└──────────────────────────┘                          └──────────────────────────┘
```

### Módulos del proyecto

| Módulo | Ruta | Tipo | Propósito |
|--------|------|------|-----------|
| `app/` | `src/app/` | Ejecutable | Punto de entrada (hoy placeholder) |
| `frontend/` | `src/frontend/` | Ejecutable (standalone) | UI Qt6 Widgets |
| `backend/` | `src/backend/` | Librería estática | OR-Tools + datos |
| `middleware/` | `src/middleware/` | Librería estática | IPC QLocalSocket |
| `common/` | `src/common/` | Header-only | Tipos compartidos |

### IPC actual

- **Mecanismo**: QLocalServer / QLocalSocket (named pipes en Unix, pipes en Windows)
- **Formato**: JSON (QJsonDocument / QJsonObject)
- **Mensajes implementados**: Solo `health_check` (envía `{"op":"health_check"}`, recibe `{"status":"ok"}`)
- **Código**: `src/middleware/` — servidor en `internalserver.cpp`, cliente en `internalclient.cpp`

---

## 3. Problemas Detectados

### P1: OR-Tools requerido incondicionalmente

```cmake
# Root CMakeLists.txt — LÍNEA PROBLEMÁTICA
find_package(ortools CONFIG REQUIRED)
```

Esto forza a **cualquier persona** que quiera compilar el proyecto a tener OR-Tools instalado. OR-Tools solo vive dentro del Docker. Sin Docker → no se puede compilar ni siquiera el frontend.

### P2: BUILD_BACKEND no tiene `option()`

La variable `BUILD_BACKEND` existe en `src/CMakeLists.txt` pero nunca se declara con `option()`. No aparece en `cmake -L` y nadie sabe que existe.

### P3: Frontend tiene `project()` anidado

```cmake
# src/frontend/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(gestor-horarios-frontend VERSION 1.0.0 LANGUAGES CXX)
```

Tener un `project()` dentro de otro `project()` es un anti-patrón en CMake. El hijo no hereda bien las variables del padre y crea un scope separado.

### P4: OR-Tools linkeado como PUBLIC

```cmake
# src/backend/CMakeLists.txt
target_link_libraries(backend PUBLIC ortools::ortools)
```

Cualquier target que linkee `backend` se ve forzado a tener OR-Tools en su ruta de include, incluso si no lo usa directamente.

### P5: `common/` está vacío

El módulo `common/` existe estructuralmente como `INTERFACE` library pero no tiene ningún archivo. No hay códigos de error, constantes ni tipos compartidos.

---

## 4. Arquitectura Destino

```
┌──────────────────────────────────────────────────────────────────┐
│                   gestor-horarios (ÚNICO binario)                 │
│                                                                   │
│  ┌──────────────────────────────────┐  ┌────────────────────────┐ │
│  │  Modo Frontend (default)          │  │  Modo Backend           │ │
│  │  ┌────────────────────────────┐   │  │  (--backend argv)       │ │
│  │  │ main() dispatch            │   │  │                         │ │
│  │  │  ├── QApplication (UI)     │   │  │  ┌──────────────────┐  │ │
│  │  │  ├── BackendProcessManager │   │  │  │ InternalServer   │  │ │
│  │  │  │   ├── QProcess          │──│──│──│  (QLocalServer)   │  │ │
│  │  │  │   ├── health QTimer     │   │  │  └──────────────────┘  │ │
│  │  │  │   └── restart backoff   │   │  │  ┌──────────────────┐  │ │
│  │  │  ├── InternalClient        │──│──│──│  SolverManager    │  │ │
│  │  │  │   (QLocalSocket)        │   │  │  │  (OR-Tools)      │  │ │
│  │  │  └── LoginDialog / MainWin │   │  │  └──────────────────┘  │ │
│  │  └────────────────────────────┘   │  │  ┌──────────────────┐  │ │
│  │                                    │  │  │ DatabaseManager  │  │ │
│  │  frontend/ (STATIC library)        │  │  │  (Qt6::Sql)      │  │ │
│  │  middleware/ (STATIC library)      │  │  └──────────────────┘  │ │
│  │  common/ (INTERFACE library)       │  │                         │ │
│  └──────────────────────────────────┘  └────────────────────────┘ │
│                                                                   │
│  backend/ (STATIC library — solo si BUILD_BACKEND=ON)             │
└──────────────────────────────────────────────────────────────────┘
```

### Principios de diseño

1. **Un solo binario, dos modos**: El ejecutable decide su rol según `argv`
2. **Aislamiento por procesos**: Frontend y backend son procesos separados (QProcess)
3. **IPC reutilizado**: El middleware QLocalSocket no se refactoriza
4. **Compilación condicional**: `BUILD_BACKEND` controla dependencias pesadas
5. **Retrocompatibilidad**: `FRONTEND_STANDALONE` preserva el flujo Qt Creator

---

## 5. Alternativa Elegida: QProcess

Se evaluaron 4 alternativas técnicas. La elegida es la **Alternativa A: QProcess**.

| Alternativa | Técnica | Aislamiento | Esfuerzo | Veredicto |
|:-----------:|---------|:-----------:|:--------:|:---------:|
| **A** ⭐ | **QProcess — auto-lanzamiento** | ✅ Sí (procesos separados) | **6-8h** | **Elegida** |
| B | Thread isolation (QThread) | ❌ Crash mata todo | 4-5h | Descartada |
| C | fork+exec self | ✅ Sí | ~6h | Descartada (QProcess > fork) |
| D | Signal isolation (sigaction) | ❌ Undefined behavior | N/A | Inviable |

### ¿Por qué QProcess?

| Razón | Explicación |
|-------|-------------|
| **Aislamiento real** | Si la UI crashea (segfault en Qt event loop), el backend child sigue corriendo |
| **Cross-platform** | QProcess funciona en Linux, Windows (CreateProcess), macOS |
| **IPC reutilizado** | El middleware QLocalSocket/JSON se mantiene exactamente igual |
| **Mínimo riesgo** | No se refactoriza el middleware existente, solo se agrega código nuevo |
| **Qt nativo** | El equipo conoce Qt, QProcess es parte de Qt6::Core |

### ¿Por qué se descartaron las otras?

- **Thread isolation**: Cualquier segfault en la UI mata el proceso completo. No cumple el requisito.
- **fork+exec**: QProcess ya encapsula fork+exec UNIX y CreateProcess Windows. No reinventar.
- **Signal isolation**: Comportamiento indefinido post-SIGSEGV según el estándar C++. Inviable.

---

## 6. Diagrama de Componentes

### Arquitectura estática

```
┌─────────────────────────────────────────────────────────────┐
│                    gestor-horarios (binario)                 │
│                                                              │
│  ┌──────────────────────────┐  ┌──────────────────────────┐ │
│  │      Frontend Mode       │  │      Backend Mode        │ │
│  │                          │  │                          │ │
│  │ ┌────────────────────┐   │  │ ┌────────────────────┐   │ │
│  │ │ QApplication       │   │  │ │ QCoreApplication   │   │ │
│  │ │  ├─ LoginDialog    │   │  │ │  ├─ InternalServer │   │ │
│  │ │  ├─ MainWindow     │   │  │ │  ├─ SolverManager  │   │ │
│  │ │  └─ InternalClient │   │  │ │  └─ DBManager      │   │ │
│  │ └────────────────────┘   │  │ └────────────────────┘   │ │
│  │ ┌────────────────────┐   │  └──────────────────────────┘ │
│  │ │ BackendProcManager │   │                               │
│  │ │  ├─ QProcess       │───│── QLocalSocket IPC ──────────│ │
│  │ │  ├─ QTimer(health) │   │                               │
│  │ │  └─ restart logic  │   │                               │
│  │ └────────────────────┘   │                               │
│  └──────────────────────────┘                               │
│                                                              │
│  Libs: [frontend.a] [backend.a] [middleware.a] [common]     │
└─────────────────────────────────────────────────────────────┘
```

### Dependencias entre módulos

```
gestor-horarios (ejecutable)
  ├── common/        → header-only (INTERFACE)
  ├── frontend/      → Qt6::Core, Qt6::Widgets (STATIC)
  ├── middleware/    → Qt6::Core, Qt6::Network (STATIC)
  └── backend/       → ortools::ortools, Qt6::Core (STATIC, solo BUILD_BACKEND=ON)
```

---

## 7. Flujo de Arranque

### Modo Frontend (default)

```
Usuario: ./gestor-horarios

  main()
    ├── argv sin "--backend" → run_frontend_app()
    │
    ├── [1] BackendProcessManager::start()
    │     ├── QProcess("gestor-horarios --backend")
    │     ├── wait 1.5s (esperar a que abra el socket)
    │     └── health_check → recibir {"status":"ok"} → emit backendReady
    │
    ├── [2] LoginDialog.exec()
    │     ├── usuario ingresa credenciales
    │     └── aceptado → MainWindow.show()
    │
    ├── [3] InternalClient conecta al socket
    │     └── Envía/recibe operaciones IPC según la UI
    │
    └── [4] QTimer cada 5s → health-check automático
          └── Si falla → detectar crash → reiniciar backend
```

### Modo Backend (--backend)

```
Usuario: ./gestor-horarios --backend

  main()
    ├── argv contiene "--backend" → run_backend_app()
    │
    ├── [1] QCoreApplication (sin UI, sin QApplication)
    │
    ├── [2] InternalServer::start()
    │     ├── QLocalServer::removeServer("GestorHorarios_Middleware")
    │     ├── m_server->listen("GestorHorarios_Middleware")
    │     └── connect(newConnection → onNewConnection)
    │
    ├── [3] SolverManager::ready()  (TODO: implementar)
    │
    ├── [4] DatabaseManager::initialize()  (TODO: implementar)
    │
    └── [5] QCoreApplication::exec() — event loop
          └── Espera conexiones IPC, procesa operaciones
```

### Shutdown

```
Usuario: Ctrl+C o cierra ventana

  BackendProcessManager::stop()
    ├── [1] Enviar OP_SHUTDOWN vía IPC al backend
    ├── [2] QProcess::terminate()  (SIGTERM en Unix)
    ├── [3] waitForFinished(5000ms)
    ├── [4] Si no responde → QProcess::kill() (SIGKILL)
    └── [5] app.exit()
```

---

## 8. Mecanismo de Crash Recovery

### Backend crash → Frontend reinicia

```
                     ┌─ health-check OK (cada 5s)
                     │
  BackendProcessManager
    ├── QTimer cada 5s → InternalClient::sendHealthCheck()
    │
    ├── Si health-check RESPONDE → ok, seguir
    │
    ├── Si health-check NO RESPONDE (timeout 10s)
    │     ├── QProcess::CrashExit signal?
    │     │     ├── Sí → crash detectado
    │     │     └── No → QProcess::kill() (proceso colgado)
    │     │
    │     ├── emit backendCrashed()
    │     │
    │     └── restart con backoff exponencial:
    │           ├── Intento 1: esperar 2s
    │           ├── Intento 2: esperar 4s
    │           ├── Intento 3: esperar 6s
    │           └── Si 3 intentos fallan → emit backendCrashed(final)
    │
    └── UI muestra mensaje: "Reconectando con el servidor..."
```

### Frontend crash → Backend sigue vivo

```
  Procesos separados → el backend ni se entera
    ├── BackendProcessManager (en proceso frontend) muere con la UI
    ├── El backend child process sigue ejecutándose
    ├── QLocalSocket se cierra (el peer desapareció)
    └── Backend detecta disconnected → cleanup resources
```

### Health-check

```
  Frontend                          Backend
    │                                  │
    │  {"op":"health_check"}           │
    │─────────────────────────────────→│
    │                                  │
    │  {"status":"ok","ts":"... "}    │
    │←─────────────────────────────────│
    │                                  │
    └── Si no responde en 10s → crash  │
```

### Configuración del BackendProcessManager

| Parámetro | Valor | Descripción |
|-----------|-------|-------------|
| `HEALTH_CHECK_INTERVAL_MS` | 5000 | Cada 5s se envía health-check |
| `INITIAL_WAIT_MS` | 1500 | Espera inicial tras lanzar backend |
| `BACKOFF_BASE_MS` | 2000 | Backoff base para reintentos |
| `MAX_RESTART_ATTEMPTS` | 3 | Máximo de reintentos antes de rendirse |

---

## 9. Cambios Archivo por Archivo

### Archivos NUEVOS (8)

| Archivo | Propósito | Líneas estimadas |
|---------|-----------|:----------------:|
| `src/app/main.cpp` | Entry point dual-mode: parsea `--backend` | ~30 |
| `src/app/include/app/backend_process_manager.hpp` | Header BackendProcessManager | ~40 |
| `src/app/src/backend_process_manager.cpp` | QProcess lifecycle + health-check + backoff | ~120 |
| `src/app/include/app/frontend_application.hpp` | Declara `run_frontend_app()` | ~10 |
| `src/app/src/frontend_application.cpp` | Orquesta UI + BPM + Login | ~40 |
| `src/app/include/app/backend_application.hpp` | Declara `run_backend_app()` | ~10 |
| `src/app/src/backend_application.cpp` | Modo servidor backend | ~30 |
| `src/common/include/common/errors.hpp` | Códigos de error del sistema | ~30 |
| `src/common/include/common/version.hpp` | Constantes de versión | ~10 |

### Archivos MODIFICADOS (5)

| Archivo | Cambio |
|---------|--------|
| `CMakeLists.txt` (root) | `option(BUILD_BACKEND)`, `find_package(ortools)` condicional, `option(BUILD_TESTING)` |
| `src/CMakeLists.txt` | `middleware` se compila siempre (no solo con BUILD_BACKEND) |
| `src/app/CMakeLists.txt` | Linkea `frontend`, `backend` (condicional), `middleware`. Nuevas fuentes. |
| `src/frontend/CMakeLists.txt` | De `project()` → `add_library(frontend STATIC)`. Flag `FRONTEND_STANDALONE`. |
| `src/middleware/include/middleware/messages.h` | Agrega `OP_SHUTDOWN`, `OP_READY`, códigos de respuesta |
| `src/middleware/include/middleware/internalclient.h` | Agrega `sendRequest()` genérico |
| `src/middleware/src/client/internalclient.cpp` | Implementa `sendRequest()`, refactoriza `sendHealthCheck()` |

### Archivos ELIMINADOS (2)

| Archivo | Razón |
|---------|-------|
| `src/app/main_placeholder.cpp` | Reemplazado por `main.cpp` real |
| `src/frontend/src/main.cpp` | Contenido movido a `frontend_application.cpp` (se mantiene para standalone) |

### Archivos SIN CAMBIOS (la mayoría)

| Módulo | Archivos | Estado |
|--------|----------|:------:|
| `backend/` | `CMakeLists.txt`, `include/backend/data/*.hpp`, `src/data/*.cpp` | ✅ Sin cambios |
| `frontend/` | `src/logindialog.*`, `src/views/*` | ✅ Sin cambios |
| `middleware/` | `internalserver.*` | ✅ Mínimos cambios (solo routing) |
| `common/` | (antes vacío) | ✅ Se puebla |
| `test/` | tests existentes | ✅ Sin cambios estructurales |
| `docker/` | `Dockerfile.dev`, `docker-compose.yml` | ✅ Sin cambios |
| `cmake/` | `CompilerWarnings.cmake` | ✅ Sin cambios |

---

## 10. Especificación CMake

### Root `CMakeLists.txt` (post-cambio)

```cmake
cmake_minimum_required(VERSION 3.24)
project(Gestor-Horarios
    VERSION 0.1.0
    DESCRIPTION "Sistema de gestión de horarios con optimización (OR-Tools)"
    LANGUAGES CXX
)

# ── Opciones de compilación ──
option(BUILD_BACKEND "Build backend, middleware, and solver (requires OR-Tools)" ON)
option(BUILD_TESTING "Build tests" ON)

# ── Estándar C++ ──
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Dependencias externas ──
find_package(Qt6 REQUIRED COMPONENTS Core Sql Widgets Network Test)

# OR-Tools: solo si BUILD_BACKEND=ON
if(BUILD_BACKEND)
    find_package(ortools CONFIG REQUIRED)
    message(STATUS "OR-Tools found — backend and solver enabled")
else()
    message(STATUS "BUILD_BACKEND=OFF — backend and OR-Tools disabled")
endif()

# ── Warnings ──
include(cmake/CompilerWarnings.cmake)

# ── Módulos ──
add_subdirectory(src)

# ── Tests ──
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()
```

### `src/CMakeLists.txt` (post-cambio)

```cmake
add_subdirectory(common)
add_subdirectory(frontend)
add_subdirectory(middleware)  # middleware siempre necesario para IPC

if(BUILD_BACKEND)
    add_subdirectory(backend)
endif()

add_subdirectory(app)
```

### `src/app/CMakeLists.txt` (post-cambio)

```cmake
add_executable(gestor-horarios)

target_sources(gestor-horarios PRIVATE
    main.cpp
    src/backend_process_manager.cpp
    src/frontend_application.cpp
    src/backend_application.cpp
)

target_link_libraries(gestor-horarios PRIVATE
    common
    frontend
)

if(TARGET backend)
    target_link_libraries(gestor-horarios PRIVATE backend)
endif()
if(TARGET middleware)
    target_link_libraries(gestor-horarios PRIVATE middleware)
endif()

set_project_warnings(gestor-horarios)
```

### `src/frontend/CMakeLists.txt` (post-cambio)

```cmake
# Opción para desarrollo frontend standalone (Qt Creator)
option(FRONTEND_STANDALONE "Build as standalone executable for Qt Creator" OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(FRONTEND_SOURCES
    src/frontend_application.cpp
    src/logindialog.cpp
)

set(FRONTEND_HEADERS
    include/frontend/frontend_application.hpp
    src/logindialog.h
)

if(FRONTEND_STANDALONE)
    # === MODO Qt CREATOR ===
    # Ejecutable independiente con main() legacy
    qt6_add_executable(gestor-horarios-frontend
        ${FRONTEND_SOURCES}
        ${FRONTEND_HEADERS}
        src/main.cpp
    )
    target_link_libraries(gestor-horarios-frontend PRIVATE
        Qt6::Core
        Qt6::Widgets
    )
    target_compile_definitions(gestor-horarios-frontend PRIVATE
        PROJECT_VERSION="${PROJECT_VERSION}"
    )
else()
    # === MODO INTEGRADO ===
    # Librería estática para linkear en el binario único
    add_library(frontend STATIC)
    target_sources(frontend PRIVATE
        ${FRONTEND_SOURCES}
        ${FRONTEND_HEADERS}
    )
    target_include_directories(frontend PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_include_directories(frontend PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(frontend PUBLIC
        Qt6::Core
        Qt6::Widgets
    )
    target_compile_definitions(frontend PUBLIC
        PROJECT_VERSION="${PROJECT_VERSION}"
    )
endif()
```

---

## 11. Compilación Condicional

### Flags disponibles

| Flag | Valores | Default | Descripción |
|------|---------|:-------:|-------------|
| `BUILD_BACKEND` | ON / OFF | ON | Compila backend + solver (requiere OR-Tools) |
| `BUILD_TESTING` | ON / OFF | ON | Compila y ejecuta tests |
| `FRONTEND_STANDALONE` | ON / OFF | OFF | Compila frontend como ejecutable legacy (Qt Creator) |

### Matriz de configuraciones

| Configuración | Comando | ¿Requiere Docker? | ¿Qué compila? | Para quién |
|:-------------|---------|:-----------------:|--------------|:----------:|
| **Full system** | `cmake -S . -B build` | ✅ Sí | Binario único completo + tests | CI / Release |
| **Frontend-only** | `cmake -S . -B build -DBUILD_BACKEND=OFF` | ❌ No | Binario único (solo modo frontend) | Desarrolladores sin Docker |
| **Qt Creator** | Abrir `src/frontend/CMakeLists.txt` + `-DFRONTEND_STANDALONE=ON` | ❌ No | Ejecutable frontend legacy | Dani / Paola |
| **Sin tests** | `cmake -S . -B build -DBUILD_TESTING=OFF` | ✅/❌ | Sin tests (compilación más rápida) | Desarrollo rápido |

### Dependencias por flag

| Flag activo | Qt6 | OR-Tools | Google Test |
|:------------|:---:|:--------:|:-----------:|
| `BUILD_BACKEND=ON` + `BUILD_TESTING=ON` | ✅ | ✅ | ✅ |
| `BUILD_BACKEND=ON` + `BUILD_TESTING=OFF` | ✅ | ✅ | ❌ |
| `BUILD_BACKEND=OFF` + `BUILD_TESTING=ON` | ✅ | ❌ | ❌ |
| `BUILD_BACKEND=OFF` + `BUILD_TESTING=OFF` | ✅ | ❌ | ❌ |
| `FRONTEND_STANDALONE=ON` | ✅ (solo Core+Widgets) | ❌ | ❌ |

---

## 12. Plan de Implementación (5 Fases)

### Fase 1 — Foundation (REFAC-1) ~2h
**Hacer OR-Tools condicional + opciones CMake**

| # | Tarea | Archivo |
|:-:|-------|---------|
| 1 | Agregar `option(BUILD_BACKEND ON)` | `CMakeLists.txt` root |
| 2 | Mover `find_package(ortools)` dentro de `if(BUILD_BACKEND)` | `CMakeLists.txt` root |
| 3 | Agregar `option(BUILD_TESTING ON)` + tests condicionales | `CMakeLists.txt` root |
| 4 | Sacar `middleware` del `if(BUILD_BACKEND)` en src/ | `src/CMakeLists.txt` |

### Fase 2a — Frontend library (REFAC-2) ~3h
**Convertir frontend de ejecutable a librería estática**

| # | Tarea | Archivo |
|:-:|-------|---------|
| 1 | Eliminar `project()` y `qt6_add_executable()` | `src/frontend/CMakeLists.txt` |
| 2 | Agregar `option(FRONTEND_STANDALONE)` con modo dual | `src/frontend/CMakeLists.txt` |
| 3 | Crear `frontend_application.hpp` | `src/frontend/include/frontend/` |
| 4 | Crear `frontend_application.cpp` con lógica migrada | `src/frontend/src/` |
| 5 | Mantener `main.cpp` legacy solo para standalone | `src/frontend/src/main.cpp` |

### Fase 2b — main dual-mode (REFAC-3) ~4h
**Crear entry point único con parseo de argv**

| # | Tarea | Archivo |
|:-:|-------|---------|
| 1 | Crear `main.cpp` con parseo de argv | `src/app/main.cpp` |
| 2 | Crear `frontend_application.hpp/.cpp` (orquestador) | `src/app/src/` |
| 3 | Crear `backend_application.hpp/.cpp` (servidor) | `src/app/src/` |
| 4 | Actualizar `src/app/CMakeLists.txt` | `src/app/CMakeLists.txt` |
| 5 | Eliminar `main_placeholder.cpp` | `src/app/` |

### Fase 3 — BackendProcessManager (REFAC-4) ~5h
**Clase QProcess con health-check + crash recovery**

| # | Tarea | Archivo |
|:-:|-------|---------|
| 1 | Crear header con API pública | `src/app/include/app/backend_process_manager.hpp` |
| 2 | Implementar `start()` → QProcess | `src/app/src/backend_process_manager.cpp` |
| 3 | Implementar health-check periódico (QTimer 5s) | `src/app/src/backend_process_manager.cpp` |
| 4 | Implementar crash detection + backoff (2s/4s/6s) | `src/app/src/backend_process_manager.cpp` |
| 5 | Implementar `stop()` graceful + force kill | `src/app/src/backend_process_manager.cpp` |
| 6 | Integrar BPM en `frontend_application.cpp` | `src/app/src/frontend_application.cpp` |

### Fase 4 — Middleware expansion (REFAC-5) ~3h
**Extender protocolo IPC con nuevas operaciones**

| # | Tarea | Archivo |
|:-:|-------|---------|
| 1 | Agregar OP_SHUTDOWN, OP_READY, códigos respuesta | `src/middleware/include/middleware/messages.h` |
| 2 | Agregar `sendRequest()` genérico en InternalClient | `src/middleware/src/client/internalclient.cpp` |
| 3 | Agregar `responseReceived` signal | `src/middleware/include/middleware/internalclient.h` |
| 4 | Manejar OP_SHUTDOWN, OP_READY en InternalServer | `src/middleware/src/server/internalserver.cpp` |

### Fase 5 — Tests (REFAC-6) ~4h
**Tests de integración del launcher y build condicional**

| # | Tarea | Archivo |
|:-:|-------|---------|
| 1 | Test: BUILD_BACKEND=OFF compila sin OR-Tools | test/ |
| 2 | Test: lanzar `--backend`, health-check respuesta "ok" | test/ |
| 3 | Test: BPM detecta crash e intenta restart | test/ |
| 4 | Verificar tests existentes siguen pasando | `ctest` |

### Fase 6 — Documentación (REFAC-7) ~3h
**Documentar arquitectura + actualizar guías**

| # | Tarea | Archivo |
|:-:|-------|---------|
| 1 | Crear `docs/arquitectura-binario-unico.md` (este documento) | `docs/` |
| 2 | Generar PDF | `docs/` |
| 3 | Actualizar `README.md` con flags de compilación | raíz |
| 4 | Revisar `docs/guias/` | `docs/guias/` |
| 5 | Revisar `.github/workflows/` | `.github/` |

---

## 13. Evaluación de Riesgos

| # | Riesgo | Prob. | Impacto | Mitigación |
|:-:|--------|:-----:|:-------:|------------|
| R1 | **Qt Creator del equipo frontend deja de funcionar** | Alta | Alto | Mantener `FRONTEND_STANDALONE=ON` para desarrollo standalone. Probar en Qt Creator antes de mergear. |
| R2 | **Regresión en middleware IPC** | Media | Alto | Tests existentes (`test_health_check.cpp`) deben seguir pasando. Los tests de health-check se ejecutan en CI. |
| R3 | **QProcess no detecta correctamente crash** | Media | Medio | Usar `QProcess::waitForFinished()` con timeout + health-check periódico como doble verificación. |
| R4 | **BUILD_BACKEND=OFF falla en CI** | Baja | Alto | Agregar workflow de CI que compile con `-DBUILD_BACKEND=OFF`. |
| R5 | **Windows: ruta del ejecutable en QProcess** | Baja | Bajo | `QCoreApplication::applicationFilePath()` funciona cross-platform. |
| R6 | **Backend lento en arrancar** | Media | Medio | `INITIAL_WAIT_MS` configurable (1500ms). Reintento de health-check si timeout. |
| R7 | **Dos instancias del backend (race condition)** | Baja | Alto | `QLocalServer::removeServer()` en `InternalServer::start()` previene esto. |

### Plan de rollback

| Paso | Acción |
|:----:|--------|
| 1 | `git tag pre-unified-binary HEAD` antes de empezar |
| 2 | Si Fase 1 falla → `git checkout pre-unified-binary` |
| 3 | Si Fase 2 falla → `git revert <commit-fase2>` |
| 4 | Si Fase 3 falla → no mergear sin test verde |
| 5 | Rollback total → `git revert --no-commit HEAD~N..HEAD` + commit único |

---

## 14. Comandos de Compilación

### Prerrequisitos

- CMake 3.24+
- Ninja (opcional, recomendado)
- Qt6 (Core, Sql, Widgets, Network, Test)
- OR-Tools 9.15+ (solo para BUILD_BACKEND=ON)
- Compilador C++17 (GCC 9+, Clang 10+, MSVC 2019+)

### Build completo (con OR-Tools)

```bash
# Configurar
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Compilar
cmake --build build

# Ejecutar tests
ctest --test-dir build --output-on-failure

# Ejecutar binario único
./build/src/app/gestor-horarios
```

### Build frontend-only (sin OR-Tools, sin Docker)

```bash
# Configurar (NO requiere OR-Tools ni Docker)
cmake -S . -B build -G Ninja -DBUILD_BACKEND=OFF -DBUILD_TESTING=OFF

# Compilar
cmake --build build

# Ejecutar (solo modo frontend, sin backend real)
./build/src/app/gestor-horarios
```

### Build frontend standalone (Qt Creator)

```bash
# Desde la terminal:
cmake -S src/frontend -B build-frontend -DFRONTEND_STANDALONE=ON
cmake --build build-frontend
./build-frontend/gestor-horarios-frontend

# O desde Qt Creator:
#   1. File → Open File or Project → src/frontend/CMakeLists.txt
#   2. En CMake configuration, agregar: -DFRONTEND_STANDALONE=ON
#   3. Build & Run
```

### Ejecución en modo backend

```bash
# Iniciar solo el servidor backend (headless)
./build/src/app/gestor-horarios --backend

# Ver versión
./build/src/app/gestor-horarios --version

# Ver ayuda
./build/src/app/gestor-horarios --help
```

### Docker (sin cambios)

```bash
# El Docker actual sigue funcionando exactamente igual
docker compose -f docker/docker-compose.yml up --build
```

### CI / Verificación

```bash
# Verificar ambas configuraciones de build
cmake -S . -B build-full -G Ninja -DBUILD_BACKEND=ON && cmake --build build-full
cmake -S . -B build-lite -G Ninja -DBUILD_BACKEND=OFF && cmake --build build-lite

# Verificar frontend standalone
cmake -S src/frontend -B build-fe -DFRONTEND_STANDALONE=ON && cmake --build build-fe
```

---

## 15. Impacto en Issues del Backlog

### Sprint 2 (completado, 22-28 Jun)

| Issue | Título | Impacto |
|:----:|--------|:-------:|
| S2-I1 | OR-Tools + estructuras de datos | ✅ Sin cambios |
| S2-I2 | Endpoints CRUD middleware | ✅ Sin cambios |
| S2-I3 | Maqueta login/dashboard | ✅ Sin cambios (solo cambia cómo se compila) |
| S2-I4 | Setup middleware + health-check | ✅ Sin cambios (se expande en Fase 4) |
| S2-I5 | SQLite schema | ✅ Sin cambios |
| S2-I6 | Scaffold Qt + capacitación | ⚠️ CMake de frontend cambia: de ejecutable a librería. Mitigado por `FRONTEND_STANDALONE`. |

### Sprint 3 (en curso, 29 Jun-5 Jul)

| Issue | Título | Impacto |
|:----:|--------|:-------:|
| S3-I1 | Modelo de datos + diagrama ER | ✅ Sin cambios |
| S3-I2 | CRUD materias | ✅ Sin cambios |
| S3-I3 | Formulario profesores | ✅ Sin cambios |
| S3-I4 | Enrutamiento middleware | ⚠️ Afectado indirectamente: routing por `op` se expande en Fase 4 |
| S3-I5 | CRUD aulas | ✅ Sin cambios |
| S3-I6 | Prototipos dashboard + navegación | ✅ Sin cambios |

### Sprint 4 (6-12 Jul)

| Issue | Título | Impacto |
|:----:|--------|:-------:|
| S4-I1 | CP-SAT solver | ✅ **Bloqueante resuelto**: BUILD_BACKEND condicional |
| S4-I2 | CRUD horarios | ✅ Sin cambios |
| S4-I3 | Formularios docente/sección | ✅ Sin cambios |
| S4-I4 | Tests unitarios modelo | ✅ Sin cambios |
| S4-I5 | Servicio consultas | ✅ Sin cambios |
| S4-I6 | Formularios Qt docente/sección | ✅ Sin cambios |

### Nuevas tareas en el backlog

| ID | Tarea | Fase | Esfuerzo | Asignado sugerido |
|:--:|-------|:----:|:--------:|:-----------------:|
| REFAC-1 | CMake condicional (OR-Tools opcional) | 1 | ~2h | Luis |
| REFAC-2 | Frontend como librería estática | 2a | ~3h | Dani / Paola |
| REFAC-3 | main.cpp dual-mode + parseo argv | 2b | ~4h | Luis |
| REFAC-4 | BackendProcessManager (QProcess) | 3 | ~5h | Manuel |
| REFAC-5 | Middleware expansion | 4 | ~3h | Manuel |
| REFAC-6 | Tests de integración | 5 | ~4h | Manuel / Luis |
| REFAC-7 | Documentación técnica | 6 | ~3h | Luis |

**Total esfuerzo: ~24 horas**

---

## 16. Archivos de Referencia

| Archivo | Propósito |
|---------|-----------|
| `CMakeLists.txt` (root) | Punto de entrada del build system |
| `src/CMakeLists.txt` | Orquestador de submódulos |
| `src/app/CMakeLists.txt` | Build del binario único |
| `src/frontend/CMakeLists.txt` | Build del frontend (library + standalone) |
| `src/app/main.cpp` | Entry point dual-mode |
| `src/app/src/backend_process_manager.cpp` | Gestor QProcess |
| `src/app/src/frontend_application.cpp` | Orquestador modo frontend |
| `src/app/src/backend_application.cpp` | Orquestador modo backend |
| `src/middleware/include/middleware/messages.h` | Constantes IPC |
| `src/middleware/src/client/internalclient.cpp` | Cliente IPC |
| `src/middleware/src/server/internalserver.cpp` | Servidor IPC |
| `docker/Dockerfile.dev` | Entorno Docker (sin cambios) |
| `.github/workflows/` | CI (puede necesitar actualización) |
| `README.md` | Instrucciones del proyecto |
| `docs/guias/` | Guías de desarrollo |

---

*Documento generado el 5 de julio de 2026 para el cambio `arquitectura-binario-unico`*
