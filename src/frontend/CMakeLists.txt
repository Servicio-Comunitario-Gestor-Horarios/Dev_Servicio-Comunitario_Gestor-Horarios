# src/frontend/CMakeLists.txt
# ===
# Módulo frontend: interfaz de usuario con Qt6.
# Contiene ventanas, formularios, tablas, y toda la interacción visual.
#
# Dependencias externas:
#   - Qt6::Core  → eventos, señales, slots
#   - Qt6::Sql   → conexión a base de datos SQLite
#
# Dependencias internas:
#   - middleware → comunicación entre frontend y backend
#
# ¿Cómo usar?
#   Crear tus .hpp/.cpp con widgets de Qt en src/frontend/
#   y agregarlos a target_sources(frontend PRIVATE ...)
#
# Qt6 requiere AUTOMOC para procesar macros Q_OBJECT.
# Qt6::Core y Qt6::Sql ya incluyen los directorios de headers.

add_library(frontend STATIC)

target_sources(frontend PRIVATE
    frontend_placeholder.cpp
    # TODO: Reemplazar placeholder por fuentes reales:
    # main_window.cpp
    # schedule_table.cpp
)

target_link_libraries(frontend PUBLIC
    Qt6::Core
    Qt6::Sql
)

target_include_directories(frontend PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_project_warnings(frontend)
