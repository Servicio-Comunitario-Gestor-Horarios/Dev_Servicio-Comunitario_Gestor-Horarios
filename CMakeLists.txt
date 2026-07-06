# CMakeLists.txt raíz — Gestor-Horarios
# ===
# Punto de entrada del build system.
# Define el proyecto, detecta dependencias externas (OR-Tools, Qt6)
# y delega la compilación de cada módulo a sus respectivos CMakeLists.txt.
#
# Flujo de configuración:
#   cmake -S . -B build -G Ninja
#   cmake --build build
#   ctest --test-dir build
#
# Para release:
#   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
#
# Dependencias externas requeridas (instaladas en el contenedor Docker):
#   - OR-Tools v9.15+  → find_package(ortools CONFIG)
#   - Qt6              → find_package(Qt6 COMPONENTS Core Sql)
#   - CMake 3.24+      → mínimo requerido por OR-Tools
#   - Ninja            → generador del build

cmake_minimum_required(VERSION 3.24)
project(Gestor-Horarios
    VERSION 0.1.0
    DESCRIPTION "Sistema de gestión de horarios con optimización (OR-Tools)"
    LANGUAGES CXX
)

# ── Estándar C++ ──
# OR-Tools requiere C++17 como mínimo. Se aplica globalmente a todos los targets.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Opciones de compilacion ---
option(BUILD_BACKEND "Build backend, middleware, and solver (requires OR-Tools)" ON)
option(BUILD_TESTING "Build tests" ON)

# ── Dependencias externas ──
# Qt6: framework de interfaz gráfica, base de datos e IPC
# Core (eventos/hilos), Sql (SQLite), Widgets (UI), Network (IPC/QLocalServer)
find_package(Qt6 REQUIRED COMPONENTS Core Sql Widgets Network Test)

if(BUILD_BACKEND)
    # OR-Tools: biblioteca de optimización (CP-SAT, rutas, asignación, etc.)
    # Se encuentra automáticamente porque está instalada en /usr/local/lib/cmake/ortools/
    find_package(ortools CONFIG REQUIRED)
    message(STATUS "OR-Tools found - backend and solver enabled")
else()
    message(STATUS "BUILD_BACKEND=OFF - backend and OR-Tools disabled")
endif()
    

# ── Opciones de compilación ──
include(cmake/CompilerWarnings.cmake)

# ── Módulos del proyecto ──
add_subdirectory(src)

# ── Tests ──
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()
