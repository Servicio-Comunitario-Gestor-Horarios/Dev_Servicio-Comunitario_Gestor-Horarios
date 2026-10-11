@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
set PATH=C:\ninja;%PATH%
cd /d "C:\Users\Daniel\Documents\GitHub\Dev_Servicio-Comunitario_Gestor-Horarios"
if exist build rmdir /s /q build
"E:\Programas\Tools\CMake_64\bin\cmake.exe" -S . -B build -G Ninja ^
  -DCMAKE_PREFIX_PATH="E:/Programas/6.11.1/msvc2022_64;C:/ortools-v9.15-vs2022/or-tools_x64_VisualStudio2022_cpp_v9.15.6755;C:/eigen3-install" ^
  -DCMAKE_BUILD_TYPE=Debug
if errorlevel 1 (
  echo CMake configure failed
  exit /b 1
)
echo Configure successful, building...
"E:\Programas\Tools\CMake_64\bin\cmake.exe" --build build --config Debug
if errorlevel 1 (
  echo Build failed
  exit /b 1
)
echo Build successful!