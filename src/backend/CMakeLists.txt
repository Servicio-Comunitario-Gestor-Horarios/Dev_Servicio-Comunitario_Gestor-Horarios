# src/backend/CMakeLists.txt
# ===
# Módulo backend: lógica de optimización y procesamiento.
# Aquí vive todo lo relacionado con OR-Tools: modelos CP-SAT,
# algoritmos de asignación de horarios, restricciones, etc.
#
# API pública → include/backend/ (visible para otros módulos)
# Implementación → src/ (privada del módulo)
#
# Dependencias externas:
#   - ortools::ortools  → optimización (CP-SAT, routing, etc.)

add_library(backend STATIC)

target_sources(backend PRIVATE
    # Data structs
    src/data/aula.cpp
    src/data/franja_horaria.cpp
    src/data/horario.cpp
    src/data/materia.cpp
    src/data/plan_estudio.cpp
    src/data/profesor.cpp

    # Database
    src/database/DatabaseManager.cpp
    src/database/migracion.cpp

    # Services - Issue #23 (Aulas)
    src/services/ServicioAula.cpp

    # Services - Issue #21 (Materias - usando tablas existentes)
    src/services/ServicioMaterias.cpp
    src/services/ServicioPlanesEstudio.cpp
    src/services/ServicioCargaHoraria.cpp
    src/services/ServicioComposicionPlan.cpp
)

target_include_directories(backend PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(backend PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(backend PUBLIC
    ortools::ortools
    Qt6::Core
    Qt6::Sql
)

set_project_warnings(backend)
