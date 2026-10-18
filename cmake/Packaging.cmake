# cmake/Packaging.cmake
# ===
# Empaquetado portable del ejecutable con TODAS sus dependencias (Windows).
#
# `cmake --install` deja una carpeta autocontenida —ejecutable, DLL y plugins de
# Qt, runtime de Visual C++ y DLL de OR-Tools— y CPack la comprime en un ZIP que
# se puede descomprimir y ejecutar sin instalar nada ni pedir permisos de
# administrador.
#
# Los datos del usuario NO forman parte del paquete: la aplicación los guarda por
# usuario en %APPDATA% (QStandardPaths::AppDataLocation, ver
# src/datos/src/ContextoBaseDatos.cpp).
#
# Uso, desde un build Release y con el entorno de Visual Studio activado
# (vcvars64.bat), porque se necesitan windeployqt y las DLL del CRT:
#   cmake --build build-rel --target package        # genera el ZIP en build-rel/
#   cmake --install build-rel --prefix dist         # solo la carpeta, sin ZIP
#
# Solo se genera en Windows: las dependencias que se copian (windeployqt y el
# runtime de MSVC) son específicas de esa plataforma. En Linux/Docker el build no
# se ve afectado.

if(NOT WIN32)
    message(STATUS "Empaquetado portable: omitido (solo se genera en Windows)")
    return()
endif()

if(NOT TARGET gestor-horarios)
    return()
endif()

# ── windeployqt: DLL y plugins de Qt ──
# Vive en el mismo directorio `bin` que los ejecutables de Qt
# (Qt6_DIR apunta a <Qt>/lib/cmake/Qt6).
get_filename_component(_gestor_qt_bin_dir "${Qt6_DIR}/../../../bin" ABSOLUTE)
find_program(GESTOR_WINDEPLOYQT
    NAMES windeployqt
    HINTS "${_gestor_qt_bin_dir}" "${QT6_INSTALL_PREFIX}/bin"
)

# ── DLL de OR-Tools ──
# ortools_DIR apunta a <raiz>/lib/cmake/ortools y las DLL viven en <raiz>/bin.
# windeployqt no las conoce, así que se copian aparte.
set(GESTOR_DLLS_ORTOOLS "")
if(TARGET backend_solver AND ortools_DIR)
    foreach(_gestor_cand
            "${ortools_DIR}/../../../bin"   # paquete prebuilt (<raiz>/lib/cmake/ortools)
            "${ortools_DIR}/../../bin")     # paquete instalado en <raiz>/cmake/ortools
        if(EXISTS "${_gestor_cand}")
            file(GLOB _gestor_ortools_dlls "${_gestor_cand}/*.dll")
            if(_gestor_ortools_dlls)
                set(GESTOR_DLLS_ORTOOLS ${_gestor_ortools_dlls})
                break()
            endif()
        endif()
    endforeach()

    list(LENGTH GESTOR_DLLS_ORTOOLS _gestor_n_ortools)
    if(_gestor_n_ortools EQUAL 0)
        message(WARNING
            "No se encontraron DLL de OR-Tools junto a '${ortools_DIR}': "
            "el paquete se generará sin el solver.")
    else()
        message(STATUS
            "Empaquetado: ${_gestor_n_ortools} DLL de OR-Tools incluidas")
    endif()
endif()

# ── Runtime de Visual C++ (app-local) ──
# Se copian las DLL del CRT en vez de `vc_redist.x64.exe`, para que el paquete
# funcione sin instalar nada. `VCToolsRedistDir` lo define vcvars64.bat.
set(GESTOR_DLLS_CRT "")
if(DEFINED ENV{VCToolsRedistDir})
    file(TO_CMAKE_PATH "$ENV{VCToolsRedistDir}" _gestor_redist)
    file(GLOB _gestor_crt_dirs "${_gestor_redist}/x64/Microsoft.VC*.CRT")
    foreach(_gestor_dir IN LISTS _gestor_crt_dirs)
        file(GLOB _gestor_crt_dlls "${_gestor_dir}/*.dll")
        if(_gestor_crt_dlls)
            set(GESTOR_DLLS_CRT ${_gestor_crt_dlls})
            break()
        endif()
    endforeach()
endif()

# ── Opciones de windeployqt ──
# El frontend solo usa Widgets/Network/Sql (sin Qt Quick ni OpenGL), por lo que
# se descartan las DLL que Qt trae para Quick/D3D11: ahorran ~35 MB y no se
# necesitan. Para volver a incluirlas, quita aquí `--no-opengl-sw` y
# `--no-system-dxc-compiler`.
set(_gestor_windeployqt_flags
    --release                 # nunca desplegar las DLL de depuración
    --no-translations         # la interfaz no usa archivos .qm
    --no-system-d3d-compiler
    --no-system-dxc-compiler  # dxcompiler.dll / dxil.dll (solo Qt Quick)
    --no-opengl-sw            # opengl32sw.dll (solo Qt Quick)
    --exclude-plugins qsqlibase,qsqlmimer,qsqloci,qsqlodbc,qsqlpsql
)

if(GESTOR_DLLS_CRT)
    list(APPEND _gestor_windeployqt_flags --no-compiler-runtime)
else()
    # Sin las DLL del CRT hay que dejar el instalador del redistribuible.
    list(APPEND _gestor_windeployqt_flags --compiler-runtime)
    message(WARNING
        "No se encontraron las DLL del runtime de Visual C++ "
        "($ENV{VCToolsRedistDir}/x64/Microsoft.VC*.CRT): el paquete incluirá "
        "vc_redist.x64.exe y habrá que ejecutarlo una vez en el equipo destino.")
endif()

string(REPLACE ";" " " GESTOR_WINDEPLOYQT_FLAGS "${_gestor_windeployqt_flags}")

# ── Reglas de instalación: todo queda junto al ejecutable ──
# Windows busca las DLL primero en el directorio del ejecutable, así que el
# paquete funciona sin tocar el PATH. Es necesario porque la aplicación se
# relanza a sí misma con --backend (ver src/app/src/gestor_proceso_backend.cpp).
install(TARGETS gestor-horarios RUNTIME DESTINATION .)

if(GESTOR_DLLS_ORTOOLS)
    install(FILES ${GESTOR_DLLS_ORTOOLS} DESTINATION .)
endif()

if(GESTOR_DLLS_CRT)
    install(FILES ${GESTOR_DLLS_CRT} DESTINATION .)
endif()

install(FILES "${CMAKE_SOURCE_DIR}/packaging/LEEME.txt" DESTINATION .)

if(GESTOR_WINDEPLOYQT)
    # Se ejecuta al instalar y, por tanto, también al empaquetar con CPack: las
    # dependencias de Qt se copian al lado del ejecutable ya instalado.
    # `${GESTOR_WINDEPLOYQT}` y las opciones se resuelven aquí (al configurar);
    # lo escapado con \${...} se resuelve al instalar.
    install(CODE "
        message(STATUS \"Empaquetado: desplegando dependencias de Qt (windeployqt)...\")
        execute_process(
            COMMAND \"${GESTOR_WINDEPLOYQT}\" ${GESTOR_WINDEPLOYQT_FLAGS}
                    \"\${CMAKE_INSTALL_PREFIX}/gestor-horarios.exe\"
            WORKING_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}\"
            RESULT_VARIABLE _gestor_rc)
        if(NOT _gestor_rc EQUAL 0)
            message(FATAL_ERROR \"windeployqt falló con código \${_gestor_rc}\")
        endif()
    ")
else()
    message(WARNING
        "windeployqt no encontrado: el paquete no incluirá las DLL de Qt. "
        "Configura la ruta de Qt6 (CMAKE_PREFIX_PATH) para que se localice.")
endif()

# ── CPack: ZIP portable ──
set(CPACK_GENERATOR "ZIP")
set(CPACK_PACKAGE_NAME "GestorHorarios")
set(CPACK_PACKAGE_VENDOR "Liceo Nacional Robert Serra")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "Gestor de horarios del Liceo Nacional Robert Serra (Qt6 + OR-Tools)")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "GestorHorarios-${PROJECT_VERSION}-win64-portable")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
set(CPACK_PACKAGE_EXECUTABLES "gestor-horarios" "Gestor de Horarios")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

include(CPack)
