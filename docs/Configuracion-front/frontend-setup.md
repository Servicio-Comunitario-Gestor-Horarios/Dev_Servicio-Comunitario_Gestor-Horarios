# Configuración del Frontend (Qt6 Widgets)

Este documento explica cómo configurar el entorno de desarrollo para el frontend del proyecto Gestor-Horarios.

---

## Requisitos previos

- **Qt6** (versión 6.2 o superior) con los módulos `Core` y `Widgets`.
- **CMake** (versión 3.16 o superior).
- **Compilador C++17** (MSVC 2019/2022 en Windows, GCC 9+ en Linux, Clang en macOS).
- **Qt Creator** (opcional pero muy recomendado para desarrollo).

---

## Estructura del frontend
frontend/
├── CMakeLists.txt # Build con CMake
├── src/
│ ├── main.cpp # Punto de entrada
│ ├── logindialog.h # Diálogo de login (mock)
│ └── logindialog.cpp
└── resources/ # (opcional) imágenes, .qrc, etc.
---

## Compilación con CMake (línea de comandos)

1. Abre una terminal en la raíz del proyecto.
2. Crea un directorio para la compilación:
   mkdir build-frontend && cd build-frontend
3. Ejecuta CMake apuntando a la carpeta frontend:
    cmake ../frontend -DCMAKE_PREFIX_PATH=<ruta_a_Qt6/lib/cmake>
    En Windows (con Qt instalado en C:\Qt\6.5.3\msvc2019_64):
        cmake ../frontend -DCMAKE_PREFIX_PATH="C:/Qt/6.5.3/msvc2019_64/lib/cmake"
    En Linux (Qt instalado por el gestor de paquetes):
        cmake ../frontend -DCMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6
4. Compila:
    cmake --build . --config Release

    El ejecutable se generará en build-frontend/Release/ (Windows) o build-frontend/ (Linux).

## Uso con Qt Creator
1. Abre Qt Creator.
2. Ve a Archivo > Abrir archivo o proyecto... y selecciona frontend/CMakeLists.txt.
3. Elige el kit que tenga Qt6 (por ejemplo, "Desktop Qt 6.5.3 MSVC2019 64bit").
4. Haz clic en Configurar proyecto.
5. El proyecto se cargará. Puedes compilar y ejecutar desde Qt Creator.