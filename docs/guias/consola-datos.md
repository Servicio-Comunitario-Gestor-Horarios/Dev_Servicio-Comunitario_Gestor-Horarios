# Consola de datos — `gestor-horarios-consola`

> Herramienta de **desarrollo** para probar la base de datos sin interfaz. Usa **el mismo
> contrato que consumirá el frontend**: `ContextoBaseDatos` (apertura/migración/respaldo) y
> `NucleoDatos` (CRUD de dominios, cascada y cambios pendientes). El módulo vive en `src/consola/`.

---

## 1. Requisitos y compilación

El ejecutable depende de `datos` → `backend` (Qt6 Core/Sql). Se compila con el preset **`full`**
(`BUILD_BACKEND=ON`, que exige OR-Tools presente en el entorno de configuración; por eso se compila
dentro del contenedor `gestor-dev`).

```bash
cmake --preset full
cmake --build build
# ejecutable: build/src/consola/gestor-horarios-consola
```

Opciones de CMake: `BUILD_CONSOLA` (por defecto `ON`). Con `BUILD_BACKEND=OFF` (preset
`dev-frontend`) la consola **no** se compila.

Dentro del contenedor:

```bash
docker exec -it gestor-dev bash -lc "cd /workspace && cmake --preset full && cmake --build build"
docker exec -it gestor-dev bash -lc "cd /workspace && ./build/src/consola/gestor-horarios-consola"
```

## 2. Acceso y argumentos

```bash
gestor-horarios-consola [--db <ruta>] [--cmd "<comando>"]... [--ayuda] [--version]
```

| Argumento | Descripción |
|---|---|
| `--db <ruta>` | Ruta del archivo de base de datos. Por defecto `./consola_datos.db` (directorio actual). |
| `--cmd "<comando>"` | Ejecuta un comando y termina (repetible). Sin `--cmd` entra en modo interactivo (REPL). |
| `--ayuda`, `--help`, `-h` | Muestra el uso y la lista de comandos. |
| `--version` | Muestra la versión. |

Al arrancar imprime el **estado de apertura** (`OkCreada`/`OkAbierta`/`OkMigrada` y los fallos),
más `detalle` y `rutaRespaldo` si los hubiera (prueba RF-1/RF-4). Si la apertura falla, sale con
código ≠ 0.

En modo interactivo, el prompt es `consola>`; `salir` (o `q`, o EOF con Ctrl+D) termina.

## 3. Comandos

Los nombres de dominio y acción no distinguen mayúsculas/minúsculas. Los argumentos con espacios van
entre comillas dobles (`"1ro A"`).

### Docentes
```
docentes listar
docentes crear      <id> "<nombre>" <email> [telefono]
docentes actualizar <id> "<nombre>" <email> [telefono]
docentes eliminar   <id>
```

### Aulas
```
aulas listar
aulas crear      "<nombre>" <capacidad> [edificio] [piso]
aulas actualizar <id> "<nombre>" <capacidad> [edificio] [piso]
aulas eliminar   <id>
```

### Materias
```
materias listar
materias crear      "<nombre>" ["<requisitos>"]
materias actualizar <id> "<nombre>" ["<requisitos>"]
materias eliminar   <id>
```

### Planes de estudio
```
planes listar
planes crear      <codigo> "<nombre>" ["<descripcion>"]
planes actualizar <codigo> "<nombre>" ["<descripcion>"]
planes eliminar   <codigo>
```

### Cursos
```
cursos listar
cursos crear      "<nombre>" [turno] [aulaFija] [numEstudiantes] [codigoPlan]
cursos actualizar <id> "<nombre>" [turno] [aulaFija] [numEstudiantes] [codigoPlan]
cursos eliminar   <id>
cursos asignar    <idCurso> <idMateria> <horasSemanales>
cursos quitar     <idCurso> <idMateria>
```

### Turnos y recesos
```
turnos listar
turnos crear      "<nombre>" <inicio HH:mm> <fin HH:mm> <numSlots>
turnos actualizar "<nombre>" <inicio HH:mm> <fin HH:mm> <numSlots>
turnos eliminar   "<nombre>"
turnos receso-agregar  "<turno>" <despuesDeSlot> <duracion>
turnos receso-eliminar "<turno>" <despuesDeSlot>
```

### Cascada, pendientes y estado
```
dependientes <dominio> <id>          Lista los registros que se eliminarían en cascada.
cascada      <dominio> <id> [--si]   Elimina en cascada (pide confirmación; --si la omite).
pendientes                           Lista los cambios pendientes de reintento.
reintentar   <id>                    Reintenta un cambio pendiente.
estado                               Muestra la base abierta y si hay cambios pendientes.
ayuda | help                         Muestra la ayuda.
salir | q                            Termina.
```

Dominios válidos para `dependientes`/`cascada`: `plan`, `materia`, `profesor`, `aula`, `curso`,
`turno`.

## 4. Ejemplos

No interactivo (script/smoke):

```bash
./build/src/consola/gestor-horarios-consola --db /tmp/consola.db \
  --cmd 'docentes crear P1 "Ana Gómez" ana@uni.edu 555' \
  --cmd "docentes listar" \
  --cmd 'materias crear "Matemática"' \
  --cmd "materias listar" \
  --cmd 'aulas crear "A1" 30 "Planta Baja"' \
  --cmd 'planes crear CIEN "Ciencias"' \
  --cmd 'turnos crear manana 07:00 12:00 6' \
  --cmd 'cursos crear "1ro A" manana -1 20 CIEN' \
  --cmd "cursos asignar 1 1 4" \
  --cmd "dependientes materia 1" \
  --cmd "cascada materia 1 --si" \
  --cmd "pendientes"
```

Interactivo:

```bash
./build/src/consola/gestor-horarios-consola --db /tmp/consola.db
consola> docentes listar
consola> dependientes curso 1
consola> cascada curso 1            # pide confirmación [s/N]
consola> salir
```

## 5. Notas

- **Resultados:** cada operación imprime `ok: …` o `error: <mensajeError>` (según el `Resultado<T>`
  real del contrato). En los `listar*`, un fallo de lectura se distingue de «sin datos».
- **Confirmación:** las operaciones destructivas en cascada piden confirmación en modo interactivo;
  en modo `--cmd` hay que añadir el token `--si`.
- **Orden:** los `listar*` muestran los registros ordenados en español (acentos, ñ y mayúsculas).
- **Pendientes:** si una escritura falla, queda como cambio pendiente reintentable (`pendientes`,
  `reintentar <id>`).

## 6. Relación con el contrato del frontend

La consola llama exactamente a los métodos documentados en `docs/interfaz-frontend.md`
(`AperturaBaseDatos`, `NucleoDatos`, `dependientesDe`, `eliminarConCascada`, `GestorPendientes`). Es
una forma práctica de verificar que la implementación de la base responde como el contrato promete.

## 7. Cómo extenderla

1. Añade un método `cmdX(const Args& args)` en `src/consola/include/consola/ConsolaDatos.hpp`.
2. Impleméntalo en `src/consola/src/ConsolaDatos.cpp` (usa `nucleo()` para llamar al contrato).
3. Regístralo en `registrarComandos()` y documenta el comando aquí.
