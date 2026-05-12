# src/backend/CMakeLists.txt
# ===
# Módulo backend: lógica de optimización y procesamiento.
# Aquí vive todo lo relacionado con OR-Tools: modelos CP-SAT,
# algoritmos de asignación de horarios, restricciones, etc.
#
# Dependencias externas:
#   - ortools::ortools  → optimización (CP-SAT, routing, etc.)
#
# Dependencias internas:
#   - (ninguna por ahora)
#
# ¿Cómo usar?
#   1. Crear tus archivos .hpp y .cpp en src/backend/
#   2. Agregarlos a target_sources(backend PRIVATE ...)
#   3. Si necesitás headers públicos para otros módulos,
#      agregarlos a target_include_directories(backend PUBLIC ...)
#
# Los headers del módulo se encuentran automáticamente
# (target_include_directories PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}).

add_library(backend STATIC)

target_sources(backend PRIVATE
    backend_placeholder.cpp
    # TODO: Reemplazar placeholder por fuentes reales:
    # schedule_model.cpp
    # constraint_builder.cpp
)

target_link_libraries(backend PUBLIC
    ortools::ortools
)

target_include_directories(backend PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_project_warnings(backend)
