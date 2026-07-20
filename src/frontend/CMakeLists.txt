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
    src/views/main_window.cpp
    src/views/teacher_list_widget.cpp
    src/forms/teacher_form_dialog.cpp
    src/views/view_placeholder.cpp
    src/views/dashboard_widget.cpp
    # Nuevos:
    src/forms/subject_form_dialog.cpp
    src/views/subject_list_widget.cpp
)

set(FRONTEND_HEADERS
    include/frontend/aplicacion_frontend.hpp
    src/logindialog.h
    src/views/main_window.hpp
    src/views/teacher_list_widget.hpp
    src/forms/teacher_form_dialog.hpp
    src/views/dashboard_widget.hpp
    src/views/view_placeholder.hpp
    # Nuevos:
    src/forms/subject_form_dialog.hpp
    src/views/subject_list_widget.hpp
)

if(FRONTEND_STANDALONE)
    # === MODO Qt CREATOR ===
    # Ejecutable independiente para desarrollo frontend puro
    find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)
    qt6_add_executable(gestor-horarios-frontend
        ${FRONTEND_SOURCES}
        ${FRONTEND_HEADERS}
        src/main.cpp
        src/forms/subject_form_dialog.hpp
        src/forms/subject_form_dialog.cpp
        src/views/subject_list_widget.hpp
        src/views/subject_list_widget.cpp
        resources/resources.qrc
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
    target_sources(frontend PRIVATE ${FRONTEND_SOURCES} ${FRONTEND_HEADERS} resources/resources.qrc)
    target_include_directories(frontend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_include_directories(frontend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(frontend PUBLIC Qt6::Core Qt6::Widgets Qt6::Network)
    target_compile_definitions(frontend PUBLIC PROJECT_VERSION="${PROJECT_VERSION}")
endif()
