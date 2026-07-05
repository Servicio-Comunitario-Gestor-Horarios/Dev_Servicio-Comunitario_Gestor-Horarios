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
#   - Qt6::Core    → eventos, hilos
#   - Qt6::Network → IPC (QLocalServer / QLocalSocket)
#
# Nota: middleware NO linkea frontend ni backend directamente.
#   app (el ejecutable) es quien conecta todo.

add_library(middleware STATIC)

target_sources(middleware PRIVATE
    src/server/internalserver.cpp
    src/client/internalclient.cpp
    include/middleware/internalserver.h
    include/middleware/internalclient.h
    include/middleware/messages.h
)

target_include_directories(middleware PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(middleware PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(middleware PUBLIC
    Qt6::Core
    Qt6::Network
)

set_target_properties(middleware PROPERTIES AUTOMOC ON)

set_project_warnings(middleware)
