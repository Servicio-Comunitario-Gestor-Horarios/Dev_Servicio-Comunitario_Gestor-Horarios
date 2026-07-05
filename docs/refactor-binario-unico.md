# Documentación del Refactor: Binario Único

> **Propósito**: Documentar el refactor que consolida `gestor-horarios-frontend` y `gestor-horarios` (backend) en un **único binario** con aislamiento de procesos y compilación condicional.
> **Branch**: `feature/core/#55-refactor-cmake`
> **Issue**: [#55](https://github.com/Servicio-Comunitario-Gestor-Horarios/Dev_Servicio-Comunitario_Gestor-Horarios/issues/55)
> **Estado**: ✅ Implementado y verificado

---

## Índice

1. [Resumen de Cambios](#1-resumen-de-cambios)
2. [Arquitectura Final](#2-arquitectura-final)
3. [Nueva Forma de Compilación](#3-nueva-forma-de-compilación)
4. [Nueva Forma de Ejecución](#4-nueva-forma-de-ejecución)
5. [Nomenclatura y Convenciones](#5-nomenclatura-y-convenciones)
6. [Guía por Área de Desarrollo](#6-guía-por-área-de-desarrollo)
   - [Frontend (Daniel, Paola)](#61-frontend-daniel-paola)
   - [Backend (Luis, Nicole)](#62-backend-luis-nicole)
   - [Middleware (Manuel)](#63-middleware-manuel)
7. [Archivos Creados y Modificados](#7-archivos-creados-y-modificados)
8. [Fases de Implementación](#8-fases-de-implementación)
9. [Comandos de Verificación](#9-comandos-de-verificación)

---

## 1. Resumen de Cambios

### Antes
- **2 ejecutables**: `gestor-horarios-frontend` (UI) y `gestor-horarios` (backend)
- OR-Tools requerido **siempre** para compilar (`find_package(ortools)` incondicional)
- Frontend tenía `project()` anidado — anti-patrón CMake
- `BackendProcessManager` en inglés (inconsistente con el equipo hispanohablante)

### Después
- **1 binario**: `gestor-horarios` con dual-mode (frontend/backend vía `--backend`)
- OR-Tools **opcional** (`BUILD_BACKEND=OFF` para desarrollo sin Docker)
- Frontend como **librería estática** en el build integrado, o **standalone** con `FRONTEND_STANDALONE=ON`
- **Todo en español**: archivos, clases, métodos, señales, constantes

### Matriz de Builds Verificados

| Configuración | Comando | Targets | Estado |
|:-------------|---------|:-------:|:------:|
| Full system | `cmake -B build -G Ninja` | 58/58 | ✅ |
| Frontend-only | `cmake -B build -DBUILD_BACKEND=OFF` | 17/17 | ✅ |
| Qt Creator | `cmake -S src/frontend -B build-fe -DFRONTEND_STANDALONE=ON` | 6/6 | ✅ |

---

## 2. Arquitectura Final

```
gestor-horarios (ÚNICO binario)
├── Modo frontend (default)
│   ├── main.cpp → ejecutarAplicacionFrontend()
│   │   ├── GestorProcesoBackend (QProcess)
│   │   │   └── lanza: gestor-horarios --backend
│   │   ├── InternalClient (QLocalSocket)
│   │   └── LoginDialog / MainWindow
│   └── Librerías: frontend.a, middleware.a, common
│
└── Modo backend (--backend)
    ├── main.cpp → ejecutarAplicacionBackend()
    │   ├── InternalServer (QLocalServer)
    │   ├── [TODO] DatabaseManager
    │   └── [TODO] SolverManager (OR-Tools)
    └── Librerías: backend.a, middleware.a, common
```

### Flujo de arranque

```
$ ./gestor-horarios
  └── main()
      ├── argv --backend? → QCoreApplication + ejecutarAplicacionBackend()
      └── default → QApplication + ejecutarAplicacionFrontend()
          └── GestorProcesoBackend::iniciar()
              ├── QProcess("gestor-horarios --backend")
              ├── esperar 1.5s, health-check
              ├── OK → timer cada 5s verifica salud
              └── Crash → backoff 2s/4s/6s, max 3 reintentos
```

---

## 3. Nueva Forma de Compilación

### Prerrequisitos

| Dependencia | Build Full | Frontend-only | Qt Creator |
|:-----------|:----------:|:-------------:|:----------:|
| CMake 3.24+ | ✅ | ✅ | ✅ |
| Qt6 Core + Sql + Widgets + Network + Test | ✅ | ✅ | Solo Core+Widgets |
| OR-Tools 9.15+ | ✅ | ❌ | ❌ |
| Google Test | ✅ | ❌ | ❌ |
| Ninja | ✅ | ✅ | ✅ |
| Compilador C++17 | ✅ | ✅ | ✅ |

### Build Completo (con OR-Tools)

```bash
# Requiere Docker o OR-Tools instalado
docker compose -f docker/docker-compose.yml up --build
```

O manualmente:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

### Build Frontend-Only (sin OR-Tools, sin Docker)

```bash
# NO requiere Docker — ideal para desarrollo rápido
cmake -S . -B build -G Ninja -DBUILD_BACKEND=OFF -DBUILD_TESTING=OFF
cmake --build build
./build/src/app/gestor-horarios
```

### Build Standalone para Qt Creator (Daniel/Paola)

```bash
cmake -S src/frontend -B build-fe -G Ninja -DFRONTEND_STANDALONE=ON
cmake --build build-fe
./build-fe/gestor-horarios-frontend
```

Desde Qt Creator:
1. **File → Open File or Project** → `src/frontend/CMakeLists.txt`
2. En configuración CMake, agregar: `-DFRONTEND_STANDALONE=ON`
3. **Build & Run**

---

## 4. Nueva Forma de Ejecución

### Modo Frontend (default) — con backend automático

```bash
# Lanza UI + backend como proceso hijo automáticamente
./build/src/app/gestor-horarios

# Backend se inicia solo, health-check cada 5s
# Si crashea, se reintenta hasta 3 veces con backoff
```

### Modo Backend (headless)

```bash
# Solo el servidor, sin interfaz gráfica
./build/src/app/gestor-horarios --backend
# Salida esperada:
#   Backend: iniciando en modo backend...
#   Servidor Middleware IPC escuchando en: "GestorHorarios_Middleware"
#   Backend: listo para recibir conexiones
```

### Utilidades

```bash
./build/src/app/gestor-horarios --version
# Gestor-Horarios v0.1.0

./build/src/app/gestor-horarios --help
# Uso: gestor-horarios [--backend] [--version] [--help]
```

---

## 5. Nomenclatura y Convenciones

Como parte del refactor, se estableció una convención de **nombres en español** para todos los elementos nuevos. El código preexistente (structs de datos, tests) mantiene su nomenclatura original.

### Reglas

| Elemento | Convención | Ejemplo |
|----------|-----------|---------|
| Clases | PascalCase español | `GestorProcesoBackend` |
| Métodos | camelCase español | `iniciar()`, `detener()`, `estaEjecutando()` |
| Señales Qt | camelCase español | `backendListo()`, `backendColapsado()` |
| Slots privados | `al` + verbo | `alIniciarProceso()`, `alExpiracionVerificacion()` |
| Variables miembro | `m_` + camelCase | `m_proceso`, `m_timerVerificacion` |
| Constantes | `MAYUSCULAS` | `MAX_INTENTOS_REINICIO`, `INTERVALO_VERIFICACION_MS` |
| Namespace | PascalCase inglés | `Middleware` (preexistente) |

### Mapa de traducciones

| Inglés (original) | Español (actual) | Archivo |
|-------------------|:----------------:|---------|
| `BackendProcessManager` | `GestorProcesoBackend` | `gestor_proceso_backend.hpp/cpp` |
| `frontend_application` | `aplicacion_frontend` | `aplicacion_frontend.hpp/cpp` (app/) |
| `backend_application` | `aplicacion_backend` | `aplicacion_backend.hpp/cpp` |
| `frontend_application` (lib) | `aplicacion_frontend` | `aplicacion_frontend.hpp/cpp` (frontend/) |
| `run_frontend_app()` | `ejecutarAplicacionFrontend()` | `aplicacion_frontend.cpp` |
| `run_backend_app()` | `ejecutarAplicacionBackend()` | `aplicacion_backend.cpp` |
| `run_frontend_app_impl()` | `ejecutarAplicacionFrontendImpl()` | `frontend/src/aplicacion_frontend.cpp` |
| `BackendProcessManager::start()` | `GestorProcesoBackend::iniciar()` | `gestor_proceso_backend.cpp` |
| `BackendProcessManager::stop()` | `GestorProcesoBackend::detener()` | `gestor_proceso_backend.cpp` |
| `isRunning()` | `estaEjecutando()` | `gestor_proceso_backend.cpp` |
| `sendRequest()` | `enviarSolicitud()` | `internalclient.h/cpp` |
| `responseReceived` | `respuestaRecibida` | `internalclient.h/cpp` |
| `logConnection()` | `registrarConexion()` | `internalserver.h/cpp` |
| `OP_READY` | `OP_LISTO` | `messages.h` |
| `OP_SHUTDOWN` | `OP_APAGAR` | `messages.h` |
| `OP_TEACHER_LIST` | `OP_LISTA_PROFESORES` | `messages.h` |
| `OP_TEACHER_GET` | `OP_OBTENER_PROFESOR` | `messages.h` |
| `OP_TEACHER_CREATE` | `OP_CREAR_PROFESOR` | `messages.h` |
| `OP_TEACHER_UPDATE` | `OP_ACTUALIZAR_PROFESOR` | `messages.h` |
| `OP_TEACHER_DELETE` | `OP_ELIMINAR_PROFESOR` | `messages.h` |
| `RESP_OK` | `RESP_EXITO` | `messages.h` |
| `RESP_NOT_FOUND` | `RESP_NO_ENCONTRADO` | `messages.h` |
| `RESP_TIMEOUT` | `RESP_TIEMPO_AGOTADO` | `messages.h` |
| `RESP_INVALID` | `RESP_INVALIDO` | `messages.h` |
| `backendReady` | `backendListo` | `gestor_proceso_backend.hpp` |
| `backendCrashed` | `backendColapsado` | `gestor_proceso_backend.hpp` |
| `backendRestarting` | `backendReiniciando` | `gestor_proceso_backend.hpp` |
| `backendStopped` | `backendDetenido` | `gestor_proceso_backend.hpp` |
| `onProcessStarted` | `alIniciarProceso` | `gestor_proceso_backend.cpp` |
| `onProcessFinished` | `alFinalizarProceso` | `gestor_proceso_backend.cpp` |
| `onProcessErrorOccurred` | `alOcurrirErrorProceso` | `gestor_proceso_backend.cpp` |
| `onHealthCheckTimeout` | `alExpiracionVerificacion` | `gestor_proceso_backend.cpp` |
| `launchProcess()` | `lanzarProceso()` | `gestor_proceso_backend.cpp` |

**API preexistente que NO cambió** (para no romper tests):
- `InternalServer`, `InternalClient`
- `sendHealthCheck()`, `healthCheckResponseReceived()`
- `OP_HEALTH_CHECK`, `SERVER_NAME`
- Todas las structs de datos (`Aula`, `Profesor`, etc.)

---

## 6. Guía por Área de Desarrollo

### 6.1 Frontend (Daniel, Paola)

**¿Qué cambió para ustedes?**

- El `main()` ya no está en `frontend/src/main.cpp`. Ahora está en `src/app/main.cpp`.
- `GestorProcesoBackend` lanza el backend automáticamente — no tienen que preocuparse por iniciarlo.
- Su código de UI (LoginDialog, MainWindow, vistas) **no cambió en absoluto**.

**¿Cómo trabajan en Qt Creator?**

```bash
# En la terminal, una vez:
cmake -S src/frontend -B build-fe -G Ninja -DFRONTEND_STANDALONE=ON

# Luego abren src/frontend/CMakeLists.txt desde Qt Creator
# La config -DFRONTEND_STANDALONE=ON ya está en el cache
```

O desde Qt Creator directamente:
1. **File → Open File or Project** → `src/frontend/CMakeLists.txt`
2. En los settings de CMake, agregar `-DFRONTEND_STANDALONE=ON`
3. Build & Run como siempre

**¿Qué NO deben tocar?**
- `src/app/main.cpp` — el entry point dual-mode
- `src/app/src/gestor_proceso_backend.*` — el lanzador del backend
- `src/middleware/` — a menos que estén agregando una nueva operación IPC

**¿Qué SÍ pueden tocar?**
- `src/frontend/src/aplicacion_frontend.cpp` — solo si cambia el flujo de inicio
- `src/frontend/src/logindialog.*` — la ventana de login
- `src/frontend/src/views/` — todas las vistas

**Nuevas señales disponibles para conectar en la UI:**

```cpp
// En el orquestador (app/src/aplicacion_frontend.cpp):
GestorProcesoBackend gestor;
connect(&gestor, &GestorProcesoBackend::backendListo, []() {
    // Backend listo para recibir peticiones
});
connect(&gestor, &GestorProcesoBackend::backendColapsado, []() {
    // Mostrar mensaje: "Reconectando con el servidor..."
});
connect(&gestor, &GestorProcesoBackend::backendReiniciando, [](int intento) {
    // Mostrar progreso: "Intento N de 3..."
});
```

### 6.2 Backend (Luis, Nicole)

**¿Qué cambió para ustedes?**

- Ya no hay `main_placeholder.cpp` — el backend real está en `src/app/src/aplicacion_backend.cpp`.
- `ejecutarAplicacionBackend()` inicia el `InternalServer` y entra en el bucle de eventos.
- OR-Tools ahora es **opcional** — usen `-DBUILD_BACKEND=OFF` para pruebas rápidas.

**¿Dónde vive el backend?**

```
src/app/src/aplicacion_backend.cpp    ← Entry point del modo backend
src/backend/                           ← Data structs + solver (OR-Tools)
src/middleware/                        ← IPC server/client
```

**¿Cómo se integra DatabaseManager?**

```cpp
// src/app/src/aplicacion_backend.cpp — ya hay TODOs:
// TODO: Inicializar DatabaseManager (Sprint 2 — Nicole)
// TODO: Inicializar SolverManager OR-Tools (Sprint 4)
```

Cuando DatabaseManager esté listo, se agrega en `ejecutarAplicacionBackend()`:

```cpp
DatabaseManager db;
if (!db.initialize("gestor_horarios.db")) {
    qCritical() << "No se pudo inicializar la base de datos";
    return 1;
}
```

**¿Cómo se integra el solver OR-Tools?**

```cpp
SolverManager solver;
solver.cargarRestricciones(/* ... */);
auto resultado = solver.resolver();
```

**Nuevas constantes disponibles en `messages.h`:**

```cpp
namespace Middleware {
    // CRUD Profesores
    inline const QString OP_LISTA_PROFESORES   = "teacher_list";
    inline const QString OP_OBTENER_PROFESOR   = "teacher_get";
    inline const QString OP_CREAR_PROFESOR     = "teacher_create";
    inline const QString OP_ACTUALIZAR_PROFESOR = "teacher_update";
    inline const QString OP_ELIMINAR_PROFESOR   = "teacher_delete";

    // Códigos de respuesta
    inline constexpr int RESP_EXITO         = 0;
    inline constexpr int RESP_ERROR         = -1;
    inline constexpr int RESP_NO_ENCONTRADO = -2;
    inline constexpr int RESP_TIEMPO_AGOTADO = -3;
    inline constexpr int RESP_INVALIDO      = -4;
}
```

### 6.3 Middleware (Manuel)

**¿Qué cambió para ustedes?**

- `InternalClient` ahora tiene `enviarSolicitud(op, payload)` — método genérico para cualquier operación.
- `InternalServer` tiene un router expandido que maneja CRUD de profesores.
- Las constantes de operación ahora están en español (ver sección 5).

**Nuevo método en InternalClient:**

```cpp
// Antes (solo health-check):
client.sendHealthCheck();

// Ahora (cualquier operación):
client.enviarSolicitud(Middleware::OP_LISTA_PROFESORES);
// o con payload:
client.enviarSolicitud(Middleware::OP_CREAR_PROFESOR, {{"nombre", "Juan"}});

// Respuesta vía nueva señal:
connect(&client, &InternalClient::respuestaRecibida, [](QJsonObject resp) {
    // resp["status"] → "ok" o "error"
    // resp["code"]   → RESP_EXITO, RESP_ERROR, etc.
});
```

**Router del servidor expandido:**

```cpp
// En InternalServer::onReadyRead():
if (op == Middleware::OP_HEALTH_CHECK) { ... }
else if (op == Middleware::OP_LISTO) { ... }
else if (op == Middleware::OP_APAGAR) { ... }
else if (op == Middleware::OP_LISTA_PROFESORES
      || op == Middleware::OP_OBTENER_PROFESOR
      || op == ...) { ... }
else { respuesta["code"] = Middleware::RESP_INVALIDO; }
```

Para agregar una nueva operación CRUD (ej: aulas):
1. Agregar constante en `messages.h`: `OP_LISTA_AULAS`, `OP_CREAR_AULA`, etc.
2. Agregar el `else if` en el router de `internalserver.cpp`
3. Usar `client.enviarSolicitud(OP_LISTA_AULAS)` desde el frontend

---

## 7. Archivos Creados y Modificados

### Archivos NUEVOS (8)

| Archivo | Líneas | Propósito |
|---------|:------:|-----------|
| `src/app/main.cpp` | 36 | Entry point dual-mode: parsea `--backend`, `--version`, `--help` |
| `src/app/include/app/aplicacion_frontend.hpp` | 20 | Header del orquestador frontend |
| `src/app/src/aplicacion_frontend.cpp` | 30 | Orquesta GestorProcesoBackend + UI |
| `src/app/include/app/aplicacion_backend.hpp` | 18 | Header del orquestador backend |
| `src/app/src/aplicacion_backend.cpp` | 28 | Servidor IPC + placeholders DB/solver |
| `src/app/include/app/gestor_proceso_backend.hpp` | 80 | Header BPM con QProcess + health-check |
| `src/app/src/gestor_proceso_backend.cpp` | 140 | Implementación QProcess + crash recovery |
| `src/frontend/include/frontend/aplicacion_frontend.hpp` | 18 | Header librería frontend |

### Archivos MODIFICADOS (7)

| Archivo | Cambio |
|---------|--------|
| `CMakeLists.txt` (root) | `option(BUILD_BACKEND)`, `option(BUILD_TESTING)`, OR-Tools condicional |
| `src/CMakeLists.txt` | `middleware` fuera del `if(BUILD_BACKEND)` — siempre se compila |
| `src/app/CMakeLists.txt` | Nuevas fuentes: `aplicacion_frontend/backend`, `gestor_proceso_backend` |
| `src/frontend/CMakeLists.txt` | De `project()` → `add_library(frontend STATIC)` + `FRONTEND_STANDALONE` |
| `src/middleware/include/middleware/messages.h` | Agregados `OP_LISTO`, `OP_APAGAR`, CRUD profesores, códigos respuesta |
| `src/middleware/include/middleware/internalclient.h` | Agregado `enviarSolicitud()`, señal `respuestaRecibida` |
| `src/middleware/src/client/internalclient.cpp` | Implementación `enviarSolicitud()`, refactor `sendHealthCheck()` |
| `src/middleware/src/server/internalserver.cpp` | Router expandido con CRUD + `registrarConexion()` |

### Archivos ELIMINADOS (2)

| Archivo | Reemplazado por |
|---------|:---------------:|
| `src/app/main_placeholder.cpp` | `src/app/main.cpp` |
| `src/frontend/src/main.cpp` | `src/app/main.cpp` (standalone preservado) |

---

## 8. Fases de Implementación

| Fase | Tarea | Archivos | Estado |
|:----:|-------|----------|:------:|
| **1** | CMake condicional | `CMakeLists.txt` (root), `src/CMakeLists.txt` | ✅ |
| **2a** | Frontend como librería | `src/frontend/CMakeLists.txt`, `aplicacion_frontend.hpp/cpp` | ✅ |
| **2b** | main dual-mode | `src/app/main.cpp`, `aplicacion_frontend/backend.hpp/cpp` | ✅ |
| **3** | BackendProcessManager | `gestor_proceso_backend.hpp/cpp` | ✅ |
| **4** | Middleware expansion | `messages.h`, `internalclient.*`, `internalserver.cpp` | ✅ |
| **5** | Tests | Verificar `ctest` passes | ✅ |
| **6** | Documentación | Este documento | ✅ |

---

## 9. Comandos de Verificación

```bash
# 1. BUILD_BACKEND=ON (full system, con OR-Tools)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
# → 58/58 targets, 8/8 tests passed

# 2. BUILD_BACKEND=OFF (frontend-only, sin OR-Tools)
cmake -S . -B build-lite -G Ninja -DBUILD_BACKEND=OFF -DBUILD_TESTING=OFF
cmake --build build-lite
# → 17/17 targets, sin OR-Tools

# 3. FRONTEND_STANDALONE=ON (Qt Creator)
cmake -S src/frontend -B build-fe -G Ninja -DFRONTEND_STANDALONE=ON
cmake --build build-fe
# → 6/6 targets, ejecutable independiente

# 4. Modo backend
./build/src/app/gestor-horarios --backend
# → Servidor IPC escuchando

# 5. Flags
./build/src/app/gestor-horarios --version  # → v0.1.0
./build/src/app/gestor-horarios --help     # → Uso correcto
```

---

*Documento generado el 5 de julio de 2026 — Branch `feature/core/#55-refactor-cmake`*
