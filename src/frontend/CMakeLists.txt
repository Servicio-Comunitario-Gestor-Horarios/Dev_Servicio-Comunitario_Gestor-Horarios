set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# === FUENTES DEL FRONTEND ===
set(FRONTEND_SOURCES
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
				src/views/schedule_visualization_widget.cpp
				src/dialogs/schedule_grid_dialog.cpp
				src/dialogs/teacher_schedule_dialog.cpp
				src/dialogs/student_schedule_dialog.cpp
				src/forms/settings_dialog.cpp
)

# === CABECERAS DEL FRONTEND ===
set(FRONTEND_HEADERS
				include/frontend/toggle_switch.hpp
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
				src/views/schedule_visualization_widget.hpp
				src/dialogs/teacher_schedule_dialog.hpp
				src/dialogs/student_schedule_dialog.hpp
				src/dialogs/schedule_grid_dialog.hpp
				src/dialogs/schedule_model.hpp
				src/dialogs/schedule_theme.hpp
				src/forms/settings_dialog.hpp
)

# === RUTAS DE INCLUSIÓN ===
set(FRONTEND_INCLUDE_DIRS
				${CMAKE_CURRENT_SOURCE_DIR}/include
				${CMAKE_CURRENT_SOURCE_DIR}/src
				${CMAKE_CURRENT_SOURCE_DIR}/src/views
				${CMAKE_CURRENT_SOURCE_DIR}/src/dialogs
				${CMAKE_CURRENT_SOURCE_DIR}/src/forms
)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Sql)

# === LIBRERÍA (modo integrado) ===
add_library(frontend STATIC)
target_sources(frontend PRIVATE ${FRONTEND_SOURCES} ${FRONTEND_HEADERS} resources/resources.qrc)

# AQUÍ ESTÁ LA CLAVE: Hacemos PUBLIC la ruta del backend
target_include_directories(frontend PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src/backend/include
)

target_include_directories(frontend PRIVATE
    ${FRONTEND_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(frontend PUBLIC
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Sql
    middleware
)

target_compile_definitions(frontend PUBLIC PROJECT_VERSION="${PROJECT_VERSION}")

if(TARGET backend)
    target_link_libraries(frontend PUBLIC backend)
endif()

if(COMMAND set_project_warnings)
    set_project_warnings(frontend)
endif()