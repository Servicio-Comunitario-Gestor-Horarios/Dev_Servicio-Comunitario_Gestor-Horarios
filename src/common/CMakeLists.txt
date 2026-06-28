# src/common/CMakeLists.txt
# ===
# Módulo common: tipos y utilidades COMPARTIDOS entre todos los módulos.
# No contiene lógica de negocio — solo definiciones de tipos,
# códigos de error, enumeraciones, versiones, etc.
#
# Es una INTERFACE library (solo headers, sin compilación).
# Cualquier módulo que lo necesite lo linkea.
#
# ¿Cómo usar?
#   target_link_libraries(mi_modulo PUBLIC common)
#   #include <common/errors.hpp>

add_library(common INTERFACE)

target_include_directories(common INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
