# src/middleware/CMakeLists.txt
# ===
# Módulo middleware: capa de comunicación entre frontend y backend.
# Traduce peticiones de la UI en llamadas al backend y viceversa.
# También maneja validación de datos, transformación de formatos,
# y orquestación de flujos de trabajo.
#
# Dependencias externas:
#   - (ninguna por ahora — solo STL)
#
# Dependencias internas:
#   - backend    → llama a los modelos de optimización
#   - frontend   → expone interfaces para la UI (opcional, por ahora separado)
#
# ¿Cómo usar?
#   Crear tus .hpp/.cpp en src/middleware/ y agregarlos acá.
#
# Nota: middleware NO linkea frontend ni backend directamente.
#   app (el ejecutable) es quien conecta todo.
#   Si middleware necesita tipos de backend/frontend,
#   agregar target_link_libraries acá y sacarlos de app.

add_library(middleware STATIC)

target_sources(middleware PRIVATE
    middleware_placeholder.cpp
    # TODO: Reemplazar placeholder por fuentes reales:
    # schedule_service.cpp
    # data_validator.cpp
)

target_include_directories(middleware PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_project_warnings(middleware)
