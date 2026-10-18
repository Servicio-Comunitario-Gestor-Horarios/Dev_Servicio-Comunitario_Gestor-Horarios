# CMake Build System — Gestor-Horarios

## Arquitectura

El proyecto usa CMake 3.24+ con una estructura modular basada en `add_subdirectory()`.

```
Gestor-Horarios/
├── CMakeLists.txt                ← Raíz: define el proyecto y dependencias globales
├── cmake/
│   └── CompilerWarnings.cmake    ← Funciones auxiliares para warnings
├── src/
│   ├── CMakeLists.txt            ← Orquestador de módulos
│   ├── app/
│   │   └── CMakeLists.txt        ← Ejecutable principal (main)
│   ├── backend/
│   │   └── CMakeLists.txt        ← Librería: lógica de optimización (OR-Tools)
│   ├── frontend/
│   │   └── CMakeLists.txt        ← Librería: interfaz gráfica (Qt6)
│   └── middleware/
│       └── CMakeLists.txt        ← Librería: comunicación frontend-backend
└── test/
    └── CMakeLists.txt            ← Tests (CTest)
```

### Árbol de dependencias

```
gestor-horarios (ejecutable)
  ├── frontend → Qt6::Core, Qt6::Sql
  ├── backend  → ortools::ortools
  └── middleware
```

Cada módulo es una **librería estática** (`.a`). El ejecutable `gestor-horarios` linkea todas y las une en un solo binario.

## Dependencias externas

| Biblioteca | find_package | Instalación |
|-----------|-------------|-------------|
| OR-Tools v9.15 | `find_package(ortools CONFIG REQUIRED)` | `/usr/local/lib/cmake/ortools/` |
| Qt6 Core + Sql | `find_package(Qt6 REQUIRED COMPONENTS Core Sql)` | `/usr/lib/x86_64-linux-gnu/cmake/` |

Ambas vienen pre-instaladas en el contenedor Docker. Si compilás fuera del contenedor, tenés que instalarlas vos.

## Comandos básicos

```bash
# Configurar el proyecto (primera vez o cuando cambia CMakeLists.txt)
cmake -S . -B build -G Ninja

# Compilar
cmake --build build

# Compilar en release
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Ejecutar tests
ctest --test-dir build

# Limpiar compilación
cmake --build build --target clean

# Re-configurar desde cero (si algo raro pasa)
rm -rf build && cmake -S . -B build -G Ninja
```

## Presets (CMakePresets.json)

| Preset | Para qué | Directorio |
|--------|----------|------------|
| `dev-frontend` | Solo interfaz (sin OR-Tools ni tests) | `build-fe/` |
| `full` | Build completo en **Docker** (Linux) | `build/` |
| `full-release` | Build completo en **Release** para el **host** | `build-rel/` |

```bash
cmake --preset full-release
cmake --build --preset full-release
ctest --preset full-release
```

### Windows/MSVC: la generación de horarios exige Release

Las DLL de OR-Tools precompiladas usan la CRT de release (`/MD`). Un binario **Debug**
(`/MDd`) mezcla el *heap* y el proceso de cálculo (`gestor-horarios --backend`) **colapsa al
resolver** con `0xC0000374` (corrupción de memoria). Por eso:

- `CMakeLists` **avisa** al configurar MSVC + `CMAKE_BUILD_TYPE=Debug` + `BUILD_BACKEND=ON`, y
  define `GESTOR_SOLVER_REQUIERE_RELEASE`.
- Con esa macro, la vista de generación **no envía** la petición y explica que hay que compilar
  en Release (mejor que llegar a la caída). El diálogo de caída del backend también traduce el
  código `0xC0000374`.

En el host usa el preset **`full-release`**, poniendo el directorio `bin` de OR-Tools en el `PATH`
(o basta con dejar sus DLL junto al ejecutable).

### `CMakeUserPresets.json` (local, no versionado)

Las rutas de Qt, OR-Tools y Eigen son de cada máquina. En lugar de ensuciar `CMakePresets.json`
—que es del proyecto—, se declaran en `CMakeUserPresets.json` (ignorado por git), heredando del
preset del proyecto:

```json
{
  "version": 6,
  "configurePresets": [
    {
      "name": "full-release-host",
      "inherits": "full-release",
      "cacheVariables": {
        "CMAKE_PREFIX_PATH": "C:/Qt/6.11.1/msvc2022_64;C:/ortools;C:/eigen3-install",
        "ortools_DIR": "C:/ortools/lib/cmake/ortools",
        "ZLIB_LIBRARY_RELEASE": "C:/ortools/lib/zlib.lib",
        "BZIP2_LIBRARY_RELEASE": "C:/ortools/lib/bz2.lib"
      }
    }
  ]
}
```

En **Qt Creator** los presets aparecen como configuraciones de build: elige **`full-release`**
(o el que herede de él) y pulsa *Run*. El preset `full` (Debug en Windows) no sirve para generar.

## Empaquetado portable (Windows)

`cmake/Packaging.cmake` (incluido desde el `CMakeLists.txt` raíz) deja un paquete
**autocontenido**: el ejecutable, las DLL y plugins de Qt, el runtime de Visual C++ y las DLL de
OR-Tools. No requiere instalación ni permisos de administrador, y los datos del usuario siguen en
`%APPDATA%\LiceoRobertSerra\GestorHorarios` (fuera del paquete).

```bat
:: 1. Entorno de Visual Studio: hace falta para windeployqt y las DLL del CRT
call "C:\...\VC\Auxiliary\Build\vcvars64.bat"

:: 2. Solo la carpeta de distribución (sin comprimir)
cmake --install build-rel --prefix dist\GestorHorarios

:: 3. ZIP portable -> build-rel\GestorHorarios-<version>-win64-portable.zip
cmake --build build-rel --target package
```

Cosas a tener en cuenta:

- Exige un build **Release** (las DLL de OR-Tools usan `/MD`; ver arriba) y `vcvars64.bat` activo,
  porque se copian `windeployqt` y las DLL del CRT desde `VCToolsRedistDir`.
- Las dependencias van **junto al ejecutable**, que es donde Windows busca primero: el paquete
  funciona sin tocar el `PATH` (necesario, porque la aplicación se relanza con `--backend`).
- Se descartan las DLL que Qt trae para Quick/D3D11 (`opengl32sw`, `dxcompiler`, `dxil`) y los
  drivers SQL que no sean SQLite: la interfaz solo usa Widgets/Network/Sql y solo `QSQLITE`.
- **ICU no se empaqueta**: lo aporta Windows (`%SystemRoot%\System32\icuuc.dll`).
- Tamaño de referencia (Qt 6.11 + OR-Tools 9.15): ~120 MB expandido, ~38 MB el ZIP.
- Solo se genera en Windows; en Linux/Docker la regla se omite sin afectar al build.
- Un instalador (Inno Setup, NSIS…) puede envolver ese ZIP más adelante.

## Flujo de trabajo diario

```bash
docker-up                  # 1. Arrancar contenedor
docker-shell               # 2. Entrar al contenedor
cd /workspace              # 3. Ir al proyecto
cmake -S . -B build -G Ninja   # 4. Configurar
cmake --build build        # 5. Compilar
ctest --test-dir build     # 6. (opcional) Correr tests
```

O todo desde afuera sin entrar al contenedor:

```bash
docker-run cmake -S . -B build -G Ninja
docker-run cmake --build build
docker-run ctest --test-dir build
```

## Cómo agregar código nuevo

### 1. Agregar fuente a un módulo existente

Ejemplo: agregar una función de validación a middleware.

```cmake
# src/middleware/CMakeLists.txt
target_sources(middleware PRIVATE
    validator.cpp          ← agregás acá
)
```

### 2. Agregar una cabecera pública

Si otro módulo necesita incluir tus headers, se agregan al `target_include_directories` (ya configurado, usa `${CMAKE_CURRENT_SOURCE_DIR}`). Simplemente creá el `.hpp` en el directorio del módulo.

### 3. Agregar un nuevo módulo

```cmake
# 1. Crear src/nuevo_modulo/CMakeLists.txt
add_library(nuevo_modulo STATIC)
target_sources(nuevo_modulo PRIVATE archivo.cpp)
target_include_directories(nuevo_modulo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 2. Agregarlo a src/CMakeLists.txt
add_subdirectory(nuevo_modulo)

# 3. Linkearlo desde app/CMakeLists.txt
target_link_libraries(gestor-horarios PRIVATE nuevo_modulo)
```

### 4. Agregar un test

```cmake
# test/CMakeLists.txt
add_executable(test_validator test/test_validator.cpp)
target_link_libraries(test_validator PRIVATE middleware)
add_test(NAME test_validator COMMAND test_validator)
```

## Variables de configuración útiles

| Variable | Ejemplo | Efecto |
|----------|---------|--------|
| `CMAKE_BUILD_TYPE` | `Release`, `Debug`, `RelWithDebInfo` | Optimización y símbolos |
| `CMAKE_CXX_STANDARD` | `17`, `20` | Estándar de C++ |
| `CMAKE_PREFIX_PATH` | `/opt/or-tools` | Rutas adicionales para find_package |

## Troubleshooting

### "Could not find a package configuration file for OR-Tools"

```bash
# Verificar que OR-Tools está instalado
docker-run ls /usr/local/lib/cmake/ortools/

# Si no existe, reconstruir el contenedor
docker-build --no-cache
```

### "Qt6 not found"

```bash
# Verificar que Qt6 está instalado en el contenedor
docker-run dpkg -l | grep qt6

# Si falta, revisar Dockerfile.dev
```

### Error de compilación raro

```bash
# Limpiar build y reconfigurar
rm -rf build
cmake -S . -B build -G Ninja
```
