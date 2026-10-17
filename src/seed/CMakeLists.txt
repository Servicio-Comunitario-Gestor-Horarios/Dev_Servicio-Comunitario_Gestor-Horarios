# src/seed/CMakeLists.txt
# ===
# Módulo seed: datos de prueba del centro (Plan de Estudio 31059).
#
# Biblioteca estática con el perfil, las variantes de docentes y el aplicador.
# No depende de OR-Tools ni de Qt Widgets: enlaza `datos` (que arrastra
# `backend` + Qt6 Core/Sql) para persistir el centro en la base.
#
# Dependencias:
#   - datos     → ciclo de vida de la base y fachada de dominios
#   - Qt6::Core → QString/QTime/QVector
#   - Qt6::Sql  → SQL (aplicador, 008-T5a)

add_library(seed STATIC)

target_sources(seed PRIVATE
				src/perfil.cpp
				src/docentes.cpp
				src/sembrador.cpp
				include/seed/perfil.hpp
				include/seed/docentes.hpp
				include/seed/sembrador.hpp
)

target_include_directories(seed PUBLIC
				${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(seed PUBLIC
				datos
				Qt6::Core
				Qt6::Sql
)

set_project_warnings(seed)

# ── Ejecutable gestor-horarios-seed (herramienta de desarrollo, plan 008 §1/§7) ──
# CLI de la aplicación del conjunto de datos de prueba. Enlaza `seed` (que arrastra
# `datos` + Qt6 Core/Sql); `PROJECT_VERSION` para `--version`.
add_executable(gestor-horarios-seed
				src/main.cpp
)

target_include_directories(gestor-horarios-seed PRIVATE
				${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(gestor-horarios-seed PRIVATE
				seed
)

target_compile_definitions(gestor-horarios-seed PRIVATE
				PROJECT_VERSION="${PROJECT_VERSION}"
)

set_project_warnings(gestor-horarios-seed)
