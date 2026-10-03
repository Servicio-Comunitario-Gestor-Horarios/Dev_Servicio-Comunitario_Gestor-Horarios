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
				src/forms/classroom_form_dialog.cpp
				src/views/classroom_list_widget.cpp
				src/forms/subject_form_dialog.cpp
				src/views/subject_list_widget.cpp
)

set(FRONTEND_HEADERS
				include/frontend/aplicacion_frontend.hpp
				src/logindialog.h
				src/views/main_window.hpp
				src/views/teacher_list_widget.hpp
				src/forms/teacher_form_dialog.hpp
				src/views/view_placeholder.hpp
				src/views/dashboard_widget.hpp
				src/forms/classroom_form_dialog.hpp
				src/views/classroom_list_widget.hpp
				src/forms/subject_form_dialog.hpp
				src/views/subject_list_widget.hpp
)

# === LIBRERIA (modo integrado) ===
add_library(frontend STATIC)
target_sources(frontend PRIVATE ${FRONTEND_SOURCES} ${FRONTEND_HEADERS} resources/resources.qrc)
target_include_directories(frontend PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(frontend PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(frontend PUBLIC Qt6::Core Qt6::Widgets Qt6::Network middleware)
target_compile_definitions(frontend PUBLIC PROJECT_VERSION="${PROJECT_VERSION}")

# Con D2 el frontend usa la BD directo. Guard por si se compila sin backend.
if(TARGET backend)
    target_link_libraries(frontend PUBLIC backend)
endif()
set_project_warnings(frontend)

# === EJECUTABLE STANDALONE (desarrollo UI) ===
option(BUILD_FRONTEND_STANDALONE "Ejecutable standalone para desarrollo UI" ON)
if(BUILD_FRONTEND_STANDALONE)
    qt6_add_executable(gestor-horarios-frontend
        ${FRONTEND_SOURCES}
        ${FRONTEND_HEADERS}
        src/main.cpp
        resources/resources.qrc
    )
    target_include_directories(gestor-horarios-frontend PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(gestor-horarios-frontend PRIVATE frontend middleware)
    set_project_warnings(gestor-horarios-frontend)
endif()
