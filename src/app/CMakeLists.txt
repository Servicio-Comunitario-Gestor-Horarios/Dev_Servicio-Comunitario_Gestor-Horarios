# src/app/CMakeLists.txt
# ===
# Módulo app: punto de entrada del ejecutable.
# Contiene el main() y la inicialización del sistema.
#
# Dependencias internas (todas PRIVATE — no se filtran):
#   - frontend   → UI con Qt6
#   - backend    → lógica de optimización con OR-Tools
#   - middleware  → orquestación entre frontend y backend
#
# ¿Cómo usar?
#   1. Crear src/app/main.cpp con el punto de entrada
#   2. Agregarlo a target_sources(gestor-horarios PRIVATE ...)
#   3. Listo — compilás con cmake --build build

add_executable(gestor-horarios)

target_sources(gestor-horarios PRIVATE
    main_placeholder.cpp
    # TODO: Reemplazar placeholder por fuentes reales:
    # main.cpp
)

target_link_libraries(gestor-horarios PRIVATE
    frontend
    common
)

# backend y middleware existen solo si BUILD_BACKEND=ON
if(TARGET backend)
    target_link_libraries(gestor-horarios PRIVATE backend)
endif()
if(TARGET middleware)
    target_link_libraries(gestor-horarios PRIVATE middleware)
endif()

set_project_warnings(gestor-horarios)
