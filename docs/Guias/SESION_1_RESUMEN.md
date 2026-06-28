# Sesión 1 — Docker Dev Environment + Estructura CMake

## Contenedor Docker (docker/)

### Problema original
OR-Tools se compilaba desde fuente con `-DBUILD_DEPS=ON`, lo que requiere ~4GB RAM y 30+ minutos. Fallaba en Docker por límite de recursos.

### Solución
Migrar al binario pre-compilado oficial de OR-Tools v9.15 para Debian 12.

### Archivos
- **`docker/Dockerfile.dev`** — Contenedor con Debian 12 slim, OR-Tools v9.15 (tarball), Qt6 (Core + Sql), CMake 3.25, Ninja, build-essential.
- **`docker/docker-compose.yml`** — Servicio `dev` con bind mount `..:/workspace`, volumen persistente `build_cache`, modo interactivo.

### Detalles técnicos
- OR-Tools se descarga de `https://github.com/google/or-tools/releases/download/v9.15/or-tools_amd64_debian-12_cpp_v9.15.6755.tar.gz`
- Se extrae con `--strip-components=1` directo a `/usr/local/` (el directorio interno del tarball usa `x86_64_Debian-12`, no `amd64_debian-12`)
- `ldconfig` actualiza la caché de bibliotecas compartidas
- Capa única `RUN` para atomicidad

### Comandos de uso
```bash
docker-start          # arrancar Docker daemon
docker-build          # construir imagen
docker-up             # arrancar contenedor
docker-shell          # entrar al contenedor
docker-run <cmd>      # ejecutar comando y salir
docker-down           # apagar
```

### Aliases en ~/.zshrc
`docker-start`, `docker-up`, `docker-down`, `docker-build`, `docker-shell`, `docker-run`, `docker-logs`

### Verificación
- `cmake --find-package -DNAME=ortools` ✅
- `qmake6 --version` → Qt 6.4.2 ✅
- `cmake --version` → 3.25.1 ✅
- `ninja --version` → 1.11.1 ✅

---

## Estructura CMake

### Arquitectura modular

```
CMakeLists.txt  ← raíz: C++17, find_package(ortools), find_package(Qt6)
├── cmake/CompilerWarnings.cmake
├── src/
│   ├── CMakeLists.txt  ← orquestador
│   ├── app/            ← ejecutable gestor-horarios
│   ├── backend/        ← librería (linkea ortools::ortools)
│   ├── frontend/       ← librería (linkea Qt6::Core, Qt6::Sql)
│   └── middleware/      ← librería (solo STL)
└── test/
    └── CMakeLists.txt  ← CTest listo para agregar tests
```

### Árbol de dependencias
```
gestor-horarios
  ├── frontend → Qt6::Core, Qt6::Sql
  ├── backend  → ortools::ortools
  └── middleware
```

### Placeholders
Cada módulo tiene un `.cpp` placeholder para que CMake pueda configurar sin código real. Cuando empieces a codificar:
1. Reemplazá el placeholder por tu archivo real
2. Actualizá `target_sources()` en el `CMakeLists.txt` del módulo

### Comandos de compilación
```bash
cmake -S . -B build -G Ninja      # configurar
cmake --build build                # compilar
ctest --test-dir build             # tests
```

---

## Archivos de documentación local (no trackeados)

| Archivo | Contenido |
|---------|-----------|
| `DOCKER_WORKFLOW.md` | Guía completa de Docker: conceptos, comandos, troubleshooting |
| `CMAKE_GUIDE.md` | Guía completa de CMake: arquitectura, módulos, cómo agregar código |
| `SESION_1_RESUMEN.md` | Este archivo — resumen de la sesión |
