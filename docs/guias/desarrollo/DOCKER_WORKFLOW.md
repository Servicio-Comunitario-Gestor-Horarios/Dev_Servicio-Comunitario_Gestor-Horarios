# Docker Dev Environment — Gestor-Horarios

## Arquitectura

El entorno de desarrollo usa **Docker Compose** para definir y ejecutar un contenedor
con todas las dependencias necesarias para compilar y desarrollar el proyecto.

```
┌──────────────────────────────────────────────────────┐
│                    HOST (tu PC)                      │
│  Proyecto: /home/luis/.../Gestor-Horarios/           │
│  Editor: VS Code, vim, etc.                         │
│  Shell: zsh (luis@)                                 │
│                                                      │
│  ┌──────────────────────────────────────────────┐   │
│  │           CONTENEDOR DOCKER (gestor-dev)     │   │
│  │  OS: Debian 12 slim                          │   │
│  │  Shell: bash (root@)                         │   │
│  │                                               │   │
│  │  /workspace/ ← montaje del proyecto (bind)   │   │
│  │  /usr/local/lib/cmake/ortools/ ← OR-Tools    │   │
│  │  /usr/lib/x86_64-linux-gnu/cmake/Qt6/ ← Qt6  │   │
│  │  cmake, ninja, g++ listos para compilar      │   │
│  └──────────────────────────────────────────────┘   │
│                                                      │
│  Volumen persistente: build_cache → /workspace/build │
└──────────────────────────────────────────────────────┘
```

## Conceptos clave

- **Host** = tu máquina real (`luis@`). Acá editás código.
- **Contenedor** = entorno aislado (`root@`). Acá compilás.
- **Bind mount** = `..:/workspace` en docker-compose. Espeja la raíz del proyecto
  dentro del contenedor. Los cambios se ven al instante en ambos lados.
- **Volumen** = `build_cache`. Persiste la carpeta `build/` entre reinicios del contenedor.
- **Imagen** = la receta (`Dockerfile.dev`). Se construye una vez, se ejecuta muchas veces.

## Prerrequisitos

```bash
docker-start          # arranca el daemon de Docker
```

## Flujo de trabajo diario

### 1. Construir la imagen (solo la primera vez o cuando cambies Dockerfile.dev)

```bash
docker-build
```

Construye la imagen según la receta en `docker/Dockerfile.dev`.
La primera vez tarda unos minutos porque descarga e instala todo.

### 2. Arrancar el contenedor

```bash
docker-up
```

Crea y arranca el contenedor `gestor-dev` en background.
A partir de acá podés entrar y salir cuantas veces quieras.

### 3. Entrar al contenedor

```bash
docker-shell
```

Te abre una terminal `root@` adentro del contenedor, en `/workspace/`.

### 4. Compilar

Ya adentro del contenedor:

```bash
cmake -S . -B build -G Ninja    # configurar (solo la primera vez)
cmake --build build              # compilar
ctest --test-dir build           # (opcional) correr tests
```

O desde afuera (host) sin entrar:

```bash
docker-run cmake -S . -B build -G Ninja
docker-run cmake --build build
```

La carpeta `build/` se persiste entre sesiones via el volumen `build_cache`.

### 5. Salir del contenedor

```bash
exit
```

El contenedor sigue corriendo en background.

### 6. Apagar el contenedor (cuando termines la sesión)

```bash
docker-down
```

## Referencia de comandos

### Alias del proyecto (definidos en ~/.zshrc)

| Comando | Qué hace | Cuándo usarlo |
|---------|----------|---------------|
| `docker-start` | Arranca el daemon de Docker | Si Docker no está corriendo |
| `docker-build` | Construye la imagen desde `docker/Dockerfile.dev` | Primera vez, o cuando cambies el Dockerfile |
| `docker-up` | Arranca el contenedor en background | Cada vez que empezás a trabajar |
| `docker-shell` | Abre una shell bash adentro del contenedor | Para compilar, instalar cosas, debuggear |
| `docker-run <cmd>` | Ejecuta un comando adentro y sale | Para comandos rápidos sin entrar |
| `docker-down` | Apaga y elimina el contenedor | Al finalizar la sesión |
| `docker-logs` | Muestra logs del contenedor en vivo | Para debuggear problemas de arranque |

### docker-run — ejemplos útiles

```bash
# Versiones
docker-run cmake --version
docker-run qmake6 --version
docker-run g++ --version

# Explorar dependencias instaladas
docker-run ls /usr/local/lib/
docker-run ls /usr/local/include/ortools/

# Shell rápido (ejecuta, hace lo que necesites, sale)
docker-run sh -c "echo 'hola' && ls /workspace/"
```

### Comandos nativos de Docker (sin alias)

```bash
# Listar contenedores corriendo
docker ps

# Listar imágenes disponibles
docker images

# Reconstruir desde cero (sin caché)
docker compose -f docker/docker-compose.yml build --no-cache

# Ver espacio usado por Docker
docker system df
```

## ¿Estoy en el host o en el contenedor?

```bash
which cmake
```

- Si `cmake` está en `/usr/bin/cmake` → **host** (tiene cmake del sistema)
- Si ves `root@` en el prompt → **contenedor**
- Si `/usr/local/include/ortools/` existe → **contenedor**

## Cómo modificar el entorno

Si necesitás agregar una dependencia nueva al contenedor:

1. Editá `docker/Dockerfile.dev` — agregá el `apt-get install` o la descarga
2. Reconstruí la imagen: `docker-build` (o `docker-build --no-cache` si querés forzar)
3. Arrancá de nuevo: `docker-up`

## Troubleshooting

### El contenedor no arranca

```bash
docker-logs                          # ver el error
docker-build --no-cache              # reconstruir desde cero
```

### La compilación falla con "file not found" raro

```bash
# Verificar que el contenedor tiene todo lo necesario
docker-shell
ls /usr/local/include/ortools/       # OR-Tools?
dpkg -l | grep qt6                   # Qt6?
```

### Docker dice "no space left"

```bash
docker system prune -f               # limpia contenedores/imágenes huérfanas
```
