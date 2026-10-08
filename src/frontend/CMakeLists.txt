set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# === FUENTES DEL FRONTEND ===
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
				src/views/schedule_visualization_widget.cpp
				src/dialogs/teacher_schedule_dialog.cpp
				src/dialogs/student_schedule_dialog.cpp
				src/forms/settings_dialog.cpp
)

# === CABECERAS DEL FRONTEND ===
set(FRONTEND_HEADERS
				include/frontend/aplicacion_frontend.hpp
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

# === LIBRERÍA (modo integrado) ===
add_library(frontend STATIC)
target_sources(frontend PRIVATE ${FRONTEND_SOURCES} ${FRONTEND_HEADERS} resources/resources.qrc)

target_include_directories(frontend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(frontend PRIVATE ${FRONTEND_INCLUDE_DIRS})

target_link_libraries(frontend PUBLIC
				Qt6::Core
				Qt6::Widgets
				Qt6::Network
				middleware
)

target_compile_definitions(frontend PUBLIC PROJECT_VERSION="${PROJECT_VERSION}")

if(TARGET backend)
	target_link_libraries(frontend PUBLIC backend)
endif()

if(COMMAND set_project_warnings)
	set_project_warnings(frontend)
endif()
