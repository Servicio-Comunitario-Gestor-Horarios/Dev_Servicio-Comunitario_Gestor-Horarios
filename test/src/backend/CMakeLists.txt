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
    src/data/aula.cpp
    src/data/franja_horaria.cpp
    src/data/horario.cpp
    src/data/materia.cpp
    src/data/plan_estudio.cpp
    src/data/profesor.cpp
    # TODO: Agregar fuentes reales:
    # src/data/schedule.cpp
    # src/data/teacher.cpp
    # src/data/classroom.cpp
    # src/data/subject.cpp
    # src/solver/core/model.cpp
    # src/solver/constraints/hard_constraints.cpp
    # src/solver/constraints/soft_constraints.cpp
    # src/services/generation_service.cpp
    # src/services/export_service.cpp
    # src/database/schema.cpp
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
)

set_project_warnings(backend)

target_sources(backend PRIVATE

    src/data/backend_placeholder.cpp

    src/database/database_manager.cpp

)

target_link_libraries(backend PUBLIC

    ortools::ortools

    Qt6::Sql

)