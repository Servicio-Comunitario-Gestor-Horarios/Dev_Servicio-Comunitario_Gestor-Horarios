# src/frontend/CMakeLists.txt
# ===
# Módulo frontend: interfaz de usuario con Qt6.
# Contiene ventanas, formularios, tablas, y toda la interacción visual.
#
# Dependencias externas:
#   - Qt6::Core     → eventos, señales, slots
#   - Qt6::Sql      → conexión a base de datos SQLite
#   - Qt6::Widgets  → widgets de ventana (QMainWindow, etc.)
#
# Dependencias internas:
#   - middleware → comunicación entre frontend y backend
#
# Qt6 requiere AUTOMOC para procesar macros Q_OBJECT.

add_library(frontend STATIC)




target_sources(frontend PRIVATE
    main_window.cpp
    main_window.hpp
)

target_link_libraries(frontend PUBLIC
    Qt6::Core
    Qt6::Sql
    Qt6::Widgets
)

target_include_directories(frontend PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(frontend PROPERTIES AUTOMOC ON)

set_project_warnings(frontend)
