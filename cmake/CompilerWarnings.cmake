# CompilerWarnings.cmake
# ===
# Configuración centralizada de warnings del compilador.
# Se incluye desde el CMakeLists.txt raíz y se aplica a todos los targets.
# A medida que el proyecto crezca, podés agregar flags específicos
# por plataforma (MSVC, AppleClang, etc.) o por módulo.
#
# Uso en un target:
#   set_project_warnings(mi-target)
#
# Referencia:
#   https://cmake.org/cmake/help/latest/command/target_compile_options.html

function(set_project_warnings target)
    if(MSVC)
        # Windows / MSVC
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        # GCC / Clang (Linux, macOS)
        target_compile_options(${target} PRIVATE
            -Wall           # warnings habituales
            -Wextra         # warnings adicionales
            -Wpedantic      # código ISO C++ estricto
        )
    endif()
endfunction()
