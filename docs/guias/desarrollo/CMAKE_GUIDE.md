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
