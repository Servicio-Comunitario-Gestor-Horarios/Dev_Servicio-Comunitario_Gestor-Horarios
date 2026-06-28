# src/frontend/CMakeLists.txt
# ===
# Módulo frontend: interfaz de usuario con Qt6.
# Contiene ventanas, formularios, tablas, y toda la interacción visual.
#
# API pública → include/frontend/ (si otros módulos necesitan tipos de UI)
# Implementación → src/ (vistas, diálogos, widgets, modelos)
# Recursos → resources/ (iconos, QRC, estilos)
#
# Dependencias externas:
#   - Qt6::Core     → eventos, señales, slots
#   - Qt6::Sql      → conexión a base de datos SQLite
#   - Qt6::Widgets  → widgets de ventana (QMainWindow, etc.)
#
# Qt6 requiere AUTOMOC para procesar macros Q_OBJECT.

add_library(frontend STATIC)

target_sources(frontend PRIVATE
    src/views/main_window.cpp
    src/views/main_window.hpp
    # TODO: Agregar más fuentes:
    # src/views/dashboard_view.cpp
    # src/dialogs/login_dialog.cpp
    # src/widgets/schedule_table.cpp
    # src/models/schedule_model.cpp
)

target_include_directories(frontend PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(frontend PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(frontend PUBLIC
    Qt6::Core
    Qt6::Sql
    Qt6::Widgets
)

set_target_properties(frontend PROPERTIES AUTOMOC ON)

set_project_warnings(frontend)
