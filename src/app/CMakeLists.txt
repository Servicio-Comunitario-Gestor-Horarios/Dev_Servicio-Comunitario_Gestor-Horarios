# src/app/CMakeLists.txt
# ===
# Módulo app: punto de entrada del ejecutable.
# Contiene el main() dual-mode (frontend/backend) y la inicialización del sistema.

add_executable(gestor-horarios)

set_target_properties(gestor-horarios PROPERTIES AUTOMOC ON)

target_sources(gestor-horarios PRIVATE
    main.cpp
    src/frontend_application.cpp
    src/backend_application.cpp
    src/backend_process_manager.cpp
    include/app/backend_process_manager.hpp
    include/app/frontend_application.hpp
    include/app/backend_application.hpp
)

target_include_directories(gestor-horarios PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
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
