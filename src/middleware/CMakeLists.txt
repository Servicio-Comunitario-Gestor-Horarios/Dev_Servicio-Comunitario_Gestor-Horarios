# Configuración del proyecto para el módulo común
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Definición de la librería estática 'common'
add_library(common STATIC
    internalserver.cpp
    internalserver.h
    messages.h
)

# Inclusión de directorios para que encuentre los archivos .h
# Se incluye el directorio actual y el padre para acceder a la carpeta backend
target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../
)

# Búsqueda de los componentes necesarios de Qt
find_package(Qt6 COMPONENTS Core Network REQUIRED)

# Enlace con las librerías de Qt
target_link_libraries(common PUBLIC Qt6::Core Qt6::Network)