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
# Orden de configuración: `backend` (y su persistencia) se declara antes que
# `frontend` para que la interfaz pueda usar, en tiempo de compilación, la
# lógica de dominio ya existente (p. ej. la composición de la entrada del
# solver). `datos` encapsula el ciclo de vida de la base y depende de `backend`;
# `app` depende de `frontend` y `datos`.
#
# `frontend` solo existe con BUILD_FRONTEND=ON; `app` se añade siempre porque
# `app_core` (instancia única) no depende de la UI (ver src/app/CMakeLists.txt).
#
# ¿Cómo agregar un nuevo módulo?
#   1. Crear directorio en src/
#   2. Crear src/<modulo>/CMakeLists.txt
#   3. Agregar acá: add_subdirectory(<modulo>)
#   4. Si otro módulo lo necesita, linkearlo desde su CMakeLists.txt

add_subdirectory(common)
add_subdirectory(middleware)

if(BUILD_BACKEND)
    add_subdirectory(backend)
endif()

if(BUILD_FRONTEND)
    add_subdirectory(frontend)
endif()

if(BUILD_BACKEND)
    # `datos` encapsula el ciclo de vida de la base del lado cliente y depende
    # de `backend`; solo existe cuando hay persistencia (BUILD_BACKEND=ON).
    add_subdirectory(datos)

    # Datos de prueba del centro (Plan 31059) — requiere `datos`.
    if(BUILD_SEED)
        add_subdirectory(seed)
    endif()

    # Consola de prueba de la base de datos (herramienta de desarrollo).
    if(BUILD_CONSOLA)
        add_subdirectory(consola)
    endif()
endif()

add_subdirectory(app)
