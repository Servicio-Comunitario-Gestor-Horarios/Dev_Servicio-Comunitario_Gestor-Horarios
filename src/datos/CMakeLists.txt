# src/datos/CMakeLists.txt
# ===
# Módulo datos: ciclo de vida de la base de datos del lado cliente.
#
# Encapsula la apertura/migración/respaldo de la base (`AperturaBaseDatos`), la
# conexión persistente del cliente y la fachada de dominios (`NucleoDatos`) que
# consume la interfaz. No depende de Qt Widgets: es testeable sin UI.
#
# Dependencias:
#   - backend     → persistencia y fachada de dominios (la expone en su header)
#   - Qt6::Core   → rutas y utilidades
#   - Qt6::Sql    → conexión SQLite

add_library(datos STATIC)

target_sources(datos PRIVATE
				src/ContextoBaseDatos.cpp
				include/datos/ContextoBaseDatos.hpp
)

target_include_directories(datos PUBLIC
				${CMAKE_CURRENT_SOURCE_DIR}/include
)

# El header público expone tipos de `backend` (AperturaBaseDatos, NucleoDatos),
# por eso la dependencia es PUBLIC.
target_link_libraries(datos PUBLIC
				backend
				Qt6::Core
				Qt6::Sql
)

set_project_warnings(datos)
