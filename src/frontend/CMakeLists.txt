# Opcion para desarrollo frontend standalone (Qt Creator)
option(FRONTEND_STANDALONE "Build frontend as standalone executable" OFF)

# En modo standalone, este archivo es el CMakeLists.txt raiz
if(FRONTEND_STANDALONE)
    cmake_minimum_required(VERSION 3.24)
    project(gestor-horarios-frontend LANGUAGES CXX)
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(FRONTEND_SOURCES
    src/aplicacion_frontend.cpp
    src/logindialog.cpp
)

set(FRONTEND_HEADERS
    include/frontend/aplicacion_frontend.hpp
    src/logindialog.h
)

if(FRONTEND_STANDALONE)
    # === MODO Qt CREATOR ===
    # Ejecutable independiente para desarrollo frontend puro
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
    qt6_add_executable(gestor-horarios-frontend
        ${FRONTEND_SOURCES}
        ${FRONTEND_HEADERS}
        src/main.cpp
    )
    target_include_directories(gestor-horarios-frontend PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(gestor-horarios-frontend PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
    )
else()
    # === MODO INTEGRADO ===
    # Libreria estatica para linkear en el binario unico
    add_library(frontend STATIC)
    target_sources(frontend PRIVATE ${FRONTEND_SOURCES} ${FRONTEND_HEADERS})
    target_include_directories(frontend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_include_directories(frontend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(frontend PUBLIC Qt6::Core Qt6::Widgets Qt6::Network)
    target_compile_definitions(frontend PUBLIC PROJECT_VERSION="${PROJECT_VERSION}")
endif()
