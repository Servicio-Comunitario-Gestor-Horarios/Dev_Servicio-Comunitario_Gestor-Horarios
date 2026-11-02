@echo off
setlocal
title Datos de demostracion - Gestor de Horarios

set "ORIGEN=%~dp0datos-demo\centro-31059-holgada.db"
set "CARPETA=%APPDATA%\LiceoRobertSerra\GestorHorarios"
set "DESTINO=%CARPETA%\gestor_horarios.db"

echo.
echo   Gestor de Horarios - Datos de demostracion
echo   ==========================================
echo.

if not exist "%ORIGEN%" goto sin_origen
if exist "%DESTINO%" goto ya_existe

if not exist "%CARPETA%" mkdir "%CARPETA%"
copy /y "%ORIGEN%" "%DESTINO%" >nul
if errorlevel 1 goto fallo_copia

echo   Listo. La base de datos de demostracion quedo instalada en:
echo.
echo     %DESTINO%
echo.
echo   Abre "gestor-horarios.exe": ya trae los datos del centro (13 cursos,
echo   65 docentes, 57 materias, 16 aulas, 4 planes y los 2 turnos).
echo.
echo   El preset de configuracion del generador esta en:
echo.
echo     %~dp0datos-demo\preset-centro-31059-holgada.json
echo.
echo   Cargalo en la ventana "Generar horarios" con el boton "Cargar preset...".
echo.
goto fin

:ya_existe
echo   Ya hay una base de datos en:
echo.
echo     %DESTINO%
echo.
echo   No se ha tocado nada, para no borrar tus datos.
echo   Si quieres los datos de demostracion, renombra o mueve ese archivo y
echo   vuelve a ejecutar este instalador.
echo.
goto fin

:sin_origen
echo   No se encontro la base de datos de demostracion:
echo.
echo     %ORIGEN%
echo.
echo   Descomprime el paquete completo (la carpeta "datos-demo" debe quedar
echo   junto a este archivo) y vuelve a intentarlo.
echo.
goto fin

:fallo_copia
echo   No se pudo copiar la base de datos. Comprueba que hay espacio en disco
echo   y permisos sobre la carpeta:
echo.
echo     %CARPETA%
echo.
goto fin

:fin
echo.
pause
endlocal
