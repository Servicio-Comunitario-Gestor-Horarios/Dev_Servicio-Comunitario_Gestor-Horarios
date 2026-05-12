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

# ── Dependencias externas ──

# OR-Tools: biblioteca de optimización (CP-SAT, rutas, asignación, etc.)
# Se encuentra automáticamente porque está instalada en /usr/local/lib/cmake/ortools/
find_package(ortools CONFIG REQUIRED)

# Qt6: framework de interfaz gráfica y base de datos
# Se usa Core (eventos, hilos) y Sql (SQLite).
find_package(Qt6 REQUIRED COMPONENTS Core Sql)

# ── Opciones de compilación ──
include(cmake/CompilerWarnings.cmake)

# ── Módulos del proyecto ──
add_subdirectory(src)

# ── Tests ──
add_subdirectory(test)
