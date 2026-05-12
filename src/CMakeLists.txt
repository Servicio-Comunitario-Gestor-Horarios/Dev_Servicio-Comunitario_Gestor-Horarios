# src/CMakeLists.txt
# ===
# Orquestador de los submódulos de src/.
# Cada subdirectorio representa un componente del sistema con su propio
# CMakeLists.txt que declara sus fuentes y dependencias.
#
# Arquitectura de dependencias:
#   app (ejecutable)
#     ├── frontend (Qt6)
#     ├── backend  (OR-Tools)
#     └── middleware
#
# ¿Cómo agregar un nuevo módulo?
#   1. Crear directorio en src/
#   2. Crear src/<modulo>/CMakeLists.txt
#   3. Agregar acá: add_subdirectory(<modulo>)
#   4. Si otro módulo lo necesita, linkearlo desde su CMakeLists.txt

add_subdirectory(backend)
add_subdirectory(frontend)
add_subdirectory(middleware)
add_subdirectory(app)
