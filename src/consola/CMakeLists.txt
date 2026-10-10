# src/consola/CMakeLists.txt
# ===
# Consola de terminal para probar la base de datos (herramienta de desarrollo).
#
# Usa el mismo contrato que consumirá el frontend (ContextoBaseDatos + NucleoDatos).
# Solo depende del módulo `datos` (que arrastra backend + Qt6 Core/Sql): no añade
# dependencias nuevas y no forma parte del binario de producto.

add_executable(gestor-horarios-consola
				src/main.cpp
				src/ConsolaDatos.cpp
				include/consola/ConsolaDatos.hpp
)

target_include_directories(gestor-horarios-consola PRIVATE
				${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gestor-horarios-consola PRIVATE
				datos
)

target_compile_definitions(gestor-horarios-consola PRIVATE
				PROJECT_VERSION="${PROJECT_VERSION}"
)

set_project_warnings(gestor-horarios-consola)
