# src/middleware/CMakeLists.txt
# ===
# Módulo middleware: capa de comunicación entre frontend y backend.
# Traduce peticiones de la UI en llamadas al backend y viceversa.
# También maneja validación de datos, transformación de formatos,
# y orquestación de flujos de trabajo.
#
# API pública → include/middleware/ (mensajes IPC, tipos compartidos)
# Implementación → src/ (servidor IPC, cliente, validación)
#
# Dependencias externas:
#   - (ninguna por ahora — solo STL + Qt Core para IPC)
#
# Nota: middleware NO linkea frontend ni backend directamente.
#   app (el ejecutable) es quien conecta todo.

add_library(middleware STATIC)

target_sources(middleware PRIVATE
    src/server/middleware_placeholder.cpp
    # TODO: Reemplazar placeholder por fuentes reales:
    # src/server/ipc_server.cpp
    # src/client/ipc_client.cpp
    # src/validation/data_validator.cpp
)

target_include_directories(middleware PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(middleware PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(middleware PUBLIC
    Qt6::Core
)

set_project_warnings(middleware)
