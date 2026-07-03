# Gestor-Horarios

Sistema de gestión y optimización de horarios académicos mediante OR-Tools (CP-SAT).
Aplicación de escritorio C++17 con interfaz Qt6 para la creación y optimización de
horarios usando algoritmos de satisfacción de restricciones.

---

## Stack Tecnologico

| Componente | Tecnologia |
|---|---|
| Lenguaje | C++17 |
| UI | Qt6 (Widgets) |
| Optimizacion | OR-Tools v9.15+ (CP-SAT) |
| Base de datos | SQLite via Qt6::Sql |
| Build system | CMake 3.24+ / Ninja |
| Testing | CTest + Google Test |
| Contenedor | Docker (multi-stage) |

---

## Estructura del Proyecto

```
Gestor-Horarios/
├── CMakeLists.txt          # Entry point del build. Detecta OR-Tools, Qt6, delegates a src/
├── src/                    # Codigo fuente
│   ├── CMakeLists.txt      # Coordina los modulos (app, common, backend, middleware, frontend)
│   ├── app/                # Punto de entrada de la aplicacion (main.cpp)
│   ├── common/             # Tipos compartidos entre modulos (errors, envelopes, version)
│   │   └── include/common/
│   ├── backend/            # Logica de negocio + solver OR-Tools + persistencia
│   │   ├── include/backend/   # API publica del backend
│   │   │   ├── data/          #   Estructuras de datos del dominio (Docente, Aula, etc.)
│   │   │   └── solver/        #   Interfaces del solver
│   │   └── src/               # Implementacion
│   │       ├── data/          #   Entidades del dominio
│   │       ├── solver/        #   Optimizacion CP-SAT
│   │       │   ├── core/      #     Motor principal del solver
│   │       │   └── constraints/ #     Restricciones (duras y blandas)
│   │       ├── services/      #   Servicios de alto nivel (reportes, estadisticas)
│   │       └── database/      #   Persistencia SQLite (CRUD, consultas)
│   ├── middleware/          # Capa de comunicacion frontend <-> backend
│   │   ├── include/middleware/ # API publica del middleware
│   │   └── src/
│   │       ├── server/        #   Side que recibe peticiones del frontend
│   │       ├── client/        #   Side que envia peticiones al backend
│   │       └── validation/    #   Validacion de datos en transito
│   └── frontend/           # Interfaz grafica Qt6
│       ├── include/frontend/  # API publica del frontend
│       ├── src/
│       │   ├── views/         #   Ventanas principales (MainWindow, etc.)
│       │   ├── dialogs/       #   Dialogos modales (forms, configuracion, acerca de)
│       │   ├── widgets/       #   Widgets reutilizables (tablas, calendarios, graficos)
│       │   └── models/        #   Modelos Qt (QAbstractTableModel, etc.)
│       └── resources/         #   Archivos Qt .qrc, iconos, estilos, traducciones
├── test/                   # Tests unitarios y de integracion
│   ├── CMakeLists.txt      # Configura CTest + Google Test
│   ├── backend/            # Tests del backend
│   ├── middleware/          # Tests del middleware
│   └── frontend/           # Tests del frontend (GUI testing)
├── config/                 # Archivos de configuracion (JSON, YAML, INI)
├── cmake/                  # Modulos CMake auxiliares
│   └── CompilerWarnings.cmake
├── docker/                 # Entorno de desarrollo contenerizado
│   ├── Dockerfile.dev
│   ├── docker-compose.yml
│   └── verify-dev-env.sh
├── docs/                   # Documentacion del proyecto
│   ├── guias/              # Guias de trabajo, workflow, sprints, issues
│   ├── propuestas/         # Propuestas y anteproyectos
│   ├── timelapses/         # Videos MP4 generados
│   ├── imagenes/           # Screenshots de issues y diagramas
│   ├── manual-usuario/     # Futuro manual de usuario
│   └── manual-tecnico/     # Futura documentacion tecnica
├── build/                  # Directorio de build (generado, no committear)
├── .github/                # Configuracion de GitHub (templates, workflows)
│   └── ISSUE_TEMPLATE/     # Plantillas para crear issues
├── CMakeLists.txt          # (raiz, descrito arriba)
├── LICENSE
└── README.md
```

### Por que cada carpeta

| Carpeta | Proposito |
|---|---|
| `src/app/` | Unico punto de entrada. Inicializa Qt, configura modulos, lanza la app. No debe contener logica de negocio. |
| `src/common/` | Evita dependencias circulares entre modulos. Aqui viven los tipos que backend, middleware y frontend necesitan compartir: codigos de error, envelopes de mensajes, version del sistema. |
| `src/backend/` | El nucleo del sistema. Contiene el solver OR-Tools (CP-SAT) con sus restricciones, las entidades del dominio, los servicios de alto nivel y la capa de persistencia SQLite. Es un modulo autocontenido que no depende de frontend ni middleware. |
| `src/backend/include/backend/` | Headers publicos del backend. Separar la interfaz de la implementacion permite que otros modulos (middleware) dependan solo de las interfaces, no del codigo interno. |
| `src/backend/src/solver/core/` | Motor CP-SAT: variables, objetivos, configuracion del solver. Separado de constraints para poder probar y modificar restricciones sin tocar el motor. |
| `src/backend/src/solver/constraints/` | Cada restriccion (no solapamiento, capacidad de aulas, disponibilidad de profesores, carga horaria, preferencias) en su propio archivo. Facil de agregar, modificar o desactivar sin afectar el core. |
| `src/middleware/` | Desacopla frontend de backend. El frontend nunca habla directamente con el backend; todo pasa por el middleware. Esto permite cambiar la implementacion del backend (ej: migrar a microservicios) sin tocar el frontend. |
| `src/middleware/src/server/` | Recibe mensajes del frontend, los valida, los traduce a llamadas del backend. |
| `src/middleware/src/client/` | Envia respuestas del backend de vuelta al frontend. |
| `src/middleware/src/validation/` | Validacion de esquemas, tipos, rangos. Primera linea de defensa contra datos mal formados. |
| `src/frontend/` | Interfaz grafica Qt6. Separada por patrones de Qt: vistas (ventanas), dialogos (modales), widgets (componentes reutilizables), modelos (datos para tablas/listas). |
| `src/frontend/resources/` | Archivos .qrc, iconos, estilos CSS/QSS, traducciones. Separar recursos del codigo fuente permite cambiar la apariencia sin recompilar la logica. |
| `test/` | Los tests reflejan la estructura de `src/` para que sea obvio donde van las pruebas de cada modulo. |
| `config/` | Archivos de configuracion que aplican en cualquier entorno: params del solver, valores por defecto, config de base de datos. |
| `cmake/` | Modulos CMake reutilizables (warnings, sanitizers, helpers). Separados del CMakeLists.txt principal para mantenerlo limpio. |
| `docker/` | Entorno de desarrollo reproducible. Todos usan el mismo contenedor, no hay "en mi maquina si funciona". |
| `docs/` | Documentacion del proyecto: planificacion de sprints, guias de trabajo, decisiones de arquitectura. |

---

## Convenciones

### Commits

Formato:

```
tipo(alcance): descripcion breve

cuerpo opcional (explicar el QUE y el POR QUE, no el COMO)

Referencia a issue: Closes #ID
```

**Tipos:**

| Tipo | Cuando usarlo |
|---|---|
| `feat` | Nueva funcionalidad |
| `fix` | Correccion de errores |
| `docs` | Cambio en documentacion |
| `style` | Formateo, sin cambio logico |
| `refactor` | Reestructurar codigo sin cambiar comportamiento |
| `test` | Agregar o modificar tests |
| `chore` | Cambios en build, CI, configuracion |

**Alcances:**

| Alcance | Area |
|---|---|
| `backend` | Logica de negocio, OR-Tools, solver |
| `frontend` | UI, widgets, ventanas |
| `middleware` | Comunicacion front-back, API |
| `qa` | Tests, calidad, CI |
| `core` | Afecta todo el proyecto (CMake, estructura) |
| `docs` | README, guias, documentacion |

Ejemplos:

```
feat(backend): agregar restriccion de no solapamiento de aulas

Implementa la constraint CP-SAT que impide que dos secciones
ocupen el mismo aula en el mismo turno.

Closes #45
```

```
docs(core): agregar estructura del proyecto al README
```

### Archivos

- **Nombres**: en ingles, `snake_case` para archivos de codigo, `UPPER_CASE` para constantes y defines.
- **Headers**: `nombre.hpp` (extension .hpp para C++).
- **Implementacion**: `nombre.cpp`.
- **Headers publicos**: en `include/<modulo>/` (ej: `include/backend/data/docente.hpp`).
- **Separacion**: un archivo por clase/entidad. No juntar varias clases en un mismo archivo.
- **Codigo en español**: las variables, funciones y comentarios van en español (el dominio del problema es academico venezolano).
- **Headers de Qt**: ir al principio del archivo, antes de headers propios.

Ejemplo:
```
// Docente.hpp
#include <QString>
#include "backend/data/persona.hpp"

namespace gestor::backend::data {

class Docente : public Persona {
    QString cedula;
    QVector<QString> materiasHabilitadas;
    // ...
};

} // namespace gestor::backend::data
```

### Labels (GitHub Issues)

| Label | Uso |
|---|---|
| `area-backend` | Logica de negocio, OR-Tools, base de datos |
| `area-frontend` | Interfaz grafica, widgets, ventanas |
| `area-middleware` | Comunicacion frontend-backend, adaptadores |
| `area-docs` | Documentacion, guias, README |
| `tipo-bug` | Algo que no funciona como deberia |
| `tipo-enhancement` | Mejora que suma pero no es critica |
| `tipo-testing` | Crear o ejecutar pruebas |
| `tipo-refactor` | Reestructurar codigo sin cambiar comportamiento |
| `tipo-clean` | Formatear, limpiar, ordenar codigo |

Combinaciones comunes:
- `area-backend` + `tipo-bug` → bug en backend (urgente para Luis/Nicole)
- `area-frontend` + `tipo-bug` → bug en frontend (urgente para Daniel/Paola)
- `area-frontend` + `tipo-enhancement` → mejora en UI
- `area-middleware` + `tipo-testing` → tests de comunicacion (para Manuel)

### Ramas

```
main                     # Produccion. Bloqueada.
  └── develop            # Integracion. Bloqueada.
       ├── feature/[area]/[descripcion]
       ├── bugfix/[descripcion]
       └── hotfix/[descripcion]  # Solo desde main, para produccion
```

| Rama | Nace de | Se fusiona en |
|---|---|---|
| `feature/*` | `develop` | `develop` |
| `bugfix/*` | `develop` | `develop` |
| `hotfix/*` | `main` | `main` y `develop` |

---

## Workflow Basico

### Flujo diario (para todos)

```
 1. Elegir un issue del Project Board (columna To-Do)
 2. Asignarse el issue y moverlo a "In Progress"
 3. git checkout develop && git pull --rebase origin develop
 4. git checkout -b feature/[area]/[mi-cambio]
 5. Programar, commitear, pushear
 6. gh pr create --base develop --title "..." --body "Closes #N"
 7. Esperar revisiones (QA + Tech-Lead)
 8. Tech-Lead hace Squash and merge
 9. Listo
```

### Reglas importantes

- **Solo Tech-Leads (Luis, Daniel) mergean PRs.** Devs y QA crean PRs y revisan, pero no aprietan el boton de merge.
- **Squash and merge siempre.** Todos los commits de una rama se comprimen en uno solo al mergear.
- **Un PR debe ser pequeno.** Maximo 5 archivos y ~200 lineas. Si es mas grande, fraccionarlo en varios issues.
- **Siempre referenciar el issue** en el PR y en el commit (`Closes #N`).
- **No pushear directo a `main` ni `develop`** (protegido por rulesets). Siempre usar feature branch + PR.
- **Git pull con rebase** en develop para evitar commits de merge.
- **Si tu rama se atraso** respecto a develop: `git rebase develop` y `git push --force-with-lease`.

### Flujo de emergencia (hotfix)

Cuando algo en produccion esta roto:

```
 1. git checkout -b hotfix/[descripcion] main
 2. Programar la correccion
 3. PR a main (no a develop)
 4. Tech-Lead + QA aprueban → merge a main
 5. Inmediatamente: mergear main a develop
```

### Asignacion de issues por area

| Area | Personas |
|---|---|
| Backend (solver, OR-Tools) | Luis |
| Backend (persistencia, SQLite, servicios) | Nicole |
| Frontend (UI, formularios, navegacion) | Daniel, Paola |
| Middleware (IPC, contratos, validacion) | Manuel |
| QA / Testing | Manuel |
| Documentacion | Todos |

---

## Build

```bash
# Configurar (desde la raiz del proyecto)
cmake -S . -B build -G Ninja

# Compilar
cmake --build build

# Ejecutar tests
ctest --test-dir build

# Release
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
```

### Entorno Docker (recomendado)

```bash
docker compose -f docker/docker-compose.yml up -d
docker compose exec dev bash
# Dentro del contenedor: cmake, build, test
```

---

## Planificacion

La planificacion detallada por sprints y las issues asociadas estan en:

- `docs/guias/planificacion/planificacion-sprints.md` — que hace cada quien en cada sprint
- `docs/guias/planificacion/issues-detalladas.md` — desglose de issues por persona y sprint
- `docs/guias/workflow/WORKFLOW_COMPLETO.md` — guia extendida de trabajo en equipo
