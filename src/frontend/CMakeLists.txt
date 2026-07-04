cmake_minimum_required(VERSION 3.16)
project(gestor-horarios-frontend VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

set(SOURCES
    src/dialogs/login_dialog.cpp
    src/views/main_window.cpp
)

set(HEADERS
    src/dialogs/login_dialog.hpp
    src/views/main_window.hpp
)

# CAMBIO CLAVE: Ahora es una librería estática, no un ejecutable
qt6_add_library(frontend STATIC
    ${SOURCES}
    ${HEADERS}
)

# Permitimos que otros módulos encuentren las carpetas 'dialogs' y 'views'
target_include_directories(frontend PUBLIC src)

# PUBLIC asegura que quien use el frontend (como nuestra app) herede Qt6 automáticamente
target_link_libraries(frontend PUBLIC
    Qt6::Core
    Qt6::Widgets
)