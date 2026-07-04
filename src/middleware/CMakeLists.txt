# Objetivo central: Crear la librería estática 'middleware' para el equipo
add_library(middleware STATIC)

# Definir directorios de inclusión públicos para la estructura del proyecto
target_include_directories(middleware PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)


target_sources(middleware PRIVATE
    src/server/internalserver.cpp
    src/server/internalserver.h
    src/client/internalclient.cpp
    src/client/internalclient.h
)

# Requerir los componentes Network y Sql de Qt6
find_package(Qt6 REQUIRED COMPONENTS Network Sql)
target_link_libraries(middleware PUBLIC Qt6::Network Qt6::Sql)

set_target_properties(middleware PROPERTIES
    AUTOMOC ON
)
