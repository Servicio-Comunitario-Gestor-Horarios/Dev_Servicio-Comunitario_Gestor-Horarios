cmake_minimum_required(VERSION 3.16)
project(gestor-horarios-frontend VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

qt6_standard_project_setup()

set(SOURCES
    src/main.cpp
    src/logindialog.cpp
)

set(HEADERS
    src/logindialog.h
)

qt6_add_executable(gestor-horarios-frontend
    ${SOURCES}
    ${HEADERS}
)

# ¡Importante: usar PRIVATE!
target_link_libraries(gestor-horarios-frontend PRIVATE
    Qt6::Core
    Qt6::Widgets
)

target_compile_definitions(gestor-horarios-frontend PRIVATE
    PROJECT_VERSION="${PROJECT_VERSION}"
)