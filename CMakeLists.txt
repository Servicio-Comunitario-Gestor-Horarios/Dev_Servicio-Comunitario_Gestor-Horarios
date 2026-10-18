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
option(BUILD_CONSOLA "Build the DB console tool (requires BUILD_BACKEND)" ON)
option(BUILD_SEED "Build the test-data seeder (requires BUILD_BACKEND)" ON)

# ── Dependencias externas ──
# Qt6: framework de interfaz gráfica, base de datos e IPC
# Core (eventos/hilos), Sql (SQLite), Widgets (UI), Network (IPC/QLocalServer)
# Test solo se pide si se compilan los tests. Sql es obligatorio siempre:
# con D2 el frontend accede a la base de datos directamente.
set(GESTOR_QT_COMPONENTS Core Sql Widgets Network)
if(BUILD_TESTING)
    list(APPEND GESTOR_QT_COMPONENTS Test)
endif()
find_package(Qt6 REQUIRED COMPONENTS ${GESTOR_QT_COMPONENTS})

if(BUILD_BACKEND)
    # OR-Tools: biblioteca de optimización (CP-SAT, rutas, asignación, etc.)
    # Se encuentra automáticamente porque está instalada en /usr/local/lib/cmake/ortools/
    find_package(ortools CONFIG REQUIRED)
    message(STATUS "OR-Tools found - backend and solver enabled")

    # En Windows/MSVC las DLL de OR-Tools precompiladas usan la CRT de release
    # (/MD). Un binario Debug (/MDd) mezcla el heap y el motor corrompe la memoria
    # al resolver (código 0xC0000374): las pruebas y la generación solo funcionan
    # en un build Release. Se avisa aquí para evitar la caída en tiempo de ejecución.
    if(MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(WARNING
            "BUILD_BACKEND=ON con CMAKE_BUILD_TYPE=Debug sobre MSVC: el solver "
            "colapsará con 0xC0000374 al generar horarios (CRT /MDd del proyecto "
            "frente a /MD de las DLL de OR-Tools). Configura el proyecto en Release "
            "(-DCMAKE_BUILD_TYPE=Release) para generar horarios.")
        # La vista de generación bloquea el envío y explica por qué (ver
        # `schedule_generation_widget.cpp`): así no se llega a corromper la memoria.
        add_compile_definitions(GESTOR_SOLVER_REQUIERE_RELEASE)
    endif()
else()
    message(STATUS "BUILD_BACKEND=OFF - backend and OR-Tools disabled")
endif()
    

# ── Opciones de compilación ──
include(cmake/CompilerWarnings.cmake)

# ── Módulos del proyecto ──
add_subdirectory(src)

# ── Tests ──
# Los tests de middleware no dependen de OR-Tools. Los de backend/backend_solver
# se guardan dentro de test/CMakeLists.txt con if(TARGET ...).
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()
