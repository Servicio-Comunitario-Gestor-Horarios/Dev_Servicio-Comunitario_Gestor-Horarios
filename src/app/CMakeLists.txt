# src/app/CMakeLists.txt
# ===
# Módulo app: punto de entrada del ejecutable.
# Contiene el main() dual-mode (frontend/backend) y la inicialización del sistema.

# === Librería de arranque sin UI (testeable de forma independiente) ===
# `instancia_unica` (RF-5) no depende de la interfaz ni de OR-Tools; vive aquí
# para que tanto el ejecutable como `test_instancia_unica` enlacen lo mismo.
add_library(app_core STATIC)

set_target_properties(app_core PROPERTIES AUTOMOC ON)

target_sources(app_core PRIVATE
				src/instancia_unica.cpp
				include/app/instancia_unica.hpp
)

target_include_directories(app_core PUBLIC
				${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(app_core PUBLIC
				Qt6::Core
				Qt6::Network
)

set_project_warnings(app_core)

# === Ejecutable `gestor-horarios` ===

add_executable(gestor-horarios)

set_target_properties(gestor-horarios PROPERTIES AUTOMOC ON)

target_sources(gestor-horarios PRIVATE
				main.cpp
				src/aplicacion_frontend.cpp
				src/aplicacion_backend.cpp
				src/gestor_proceso_backend.cpp
				include/app/gestor_proceso_backend.hpp
				include/app/aplicacion_frontend.hpp
				include/app/aplicacion_backend.hpp
)

target_include_directories(gestor-horarios PRIVATE
				${CMAKE_CURRENT_SOURCE_DIR}/include
				${CMAKE_SOURCE_DIR}/src/frontend/src
)

target_link_libraries(gestor-horarios PRIVATE
				common
				app_core
				frontend
)

# El solver es lo único que exige OR-Tools. `backend` (persistencia) llega
# transitivamente vía `frontend` -> `backend`.
if(TARGET backend_solver)
	target_link_libraries(gestor-horarios PRIVATE backend_solver)
endif()
if(TARGET middleware)
	target_link_libraries(gestor-horarios PRIVATE middleware)
endif()

# El arranque del cliente usa el módulo `datos` para el ciclo de vida de la base
# (ContextoBaseDatos). Si existe, el backend de persistencia está disponible.
if(TARGET datos)
	target_link_libraries(gestor-horarios PRIVATE datos)
endif()

# El arranque del cliente abre la base y construye el núcleo de datos solo si
# el backend (persistencia) está disponible en esta configuración.
if(TARGET datos)
	target_compile_definitions(gestor-horarios PRIVATE GESTOR_TIENE_BACKEND=1)
endif()

set_project_warnings(gestor-horarios)
