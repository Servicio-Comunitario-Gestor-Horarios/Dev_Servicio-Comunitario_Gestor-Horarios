# Arquitectura de Compilación — Gestor-Horarios

## Contexto

Actualmente el proyecto tiene dos ejecutables que se comunican por IPC (QLocalSocket):

- `gestor-horarios-frontend` — UI con Qt6 Widgets (login dialog, ventanas)
- `gestor-horarios` — backend (OR-Tools) + middleware (IPC server)

El equipo frontend no puede usar Docker (sin OR-Tools), necesita compilar su UI standalone.

### Restricciones del equipo

- Somos estudiantes universitarios con poca experiencia en proyectos de esta magnitud
- Es la primera vez gestionando un proyecto de esta escala
- Ya tenemos issues detalladas hasta el sprint 6 (vamos por el sprint 2)
- Queremos algo sencillo de mantener y para que la institución lo use sin fricción

---

## Opciones Analizadas

### Opción 1 — Un solo ejecutable

Todo se compila en un solo binary. Frontend pasa de ejecutable a librería estática.

```
gestor-horarios (único ejecutable)
  ├── frontend/   (librería estática — Qt6)
  ├── backend/    (librería estática — OR-Tools)
  ├── middleware/ (librería estática — validación)
  └── common/     (header-only)
```

| Ventajas | Desventajas |
|----------|-------------|
| Un solo binary para distribuir | Requiere refactor middleware (sacar IPC) |
| Sin overhead de serialización JSON | Frontend como librería necesita separar main() |
| Llamadas directas entre capas (rápido) | UI crash → todo crash |
| Middleware más simple (solo validación) | Equipo frontend necesita Docker para compilar completo |

### Opción 2 — Dos ejecutables con auto-launch

Se mantienen los dos ejecutables actuales. El frontend detecta si el backend no está corriendo y lo lanza como subproceso.

```
Frontend (UI) ←– IPC (QLocalSocket) –→ Backend (OR-Tools)
  Lanza al backend si no está corriendo
```

| Ventajas | Desventajas |
|----------|-------------|
| Separación total de responsabilidades | Gestión de procesos: esperar inicio, reintentar, crashes |
| Frontend standalone sin Docker (ya funciona) | Frontend necesita saber ruta del backend binary |
| Si un proceso crashea, el otro sobrevive | Overhead IPC para datos en la misma máquina |
| Middleware IPC ya implementado | Más complejo de mantener — código de orquestación de procesos |

### Opción 3 — Híbrida: single executable + build modular (recomendación anterior)

```
┌─── Docker (BUILD_BACKEND=ON) ───────────────────────┐
│                                                       │
│  gestor-horarios (único ejecutable)                   │
│    ├── frontend/   (librería — UI Qt6)                │
│    ├── backend/    (librería — OR-Tools)              │
│    ├── middleware/ (librería — validación de datos)   │
│    └── common/     (header-only)                      │
│                                                       │
└───────────────────────────────────────────────────────┘

┌─── Sin Docker (BUILD_BACKEND=OFF) ───────────────────┐
│                                                       │
│  gestor-horarios-frontend (ejecutable standalone)      │
│    └── Solo UI Qt6 — desarrollo frontend              │
│                                                       │
└───────────────────────────────────────────────────────┘
```

| Ventajas | Desventajas |
|----------|-------------|
| Un solo binary para distribuir | Requiere refactor middleware (sacar IPC) |
| Frontend standalone sin Docker | Frontend como librería necesita separar main() |
| Sin overhead IPC | UI crash → todo crash |
| Build modular y predecible | Esfuerzo medio (~5-7 hrs) |

### Opción 4 ⭐ — Dos ejecutables + launcher script (NUEVA)

Se mantienen los dos ejecutables actuales. Se agrega un **script lanzador** como único punto de entrada para el usuario final.

```
┌─── Instalación ─────────────────────────────────┐
│                                                   │
│  gestor-horarios/                                 │
│  ├── gestor-horarios          ← script lanzador   │
│  ├── bin/                                          │
│  │   ├── gestor-horarios-frontend  (UI Qt6)       │
│  │   └── gestor-horarios-backend   (OR-Tools)     │
│  └── ...                                           │
│                                                   │
└───────────────────────────────────────────────────┘
```

**El script lanzador:**
1. Verifica si el backend está corriendo (puerto IPC)
2. Si no, lo inicia como proceso hijo
3. Espera confirmación de que el backend está listo
4. Inicia el frontend
5. Monitorea ambos procesos
6. Si uno crashea, mata al otro y reinicia ambos
7. Atrapa SIGTERM/SIGINT para apagado limpio

**Ejemplo conceptual del script (bash):**

```bash
#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BACKEND="$SCRIPT_DIR/bin/gestor-horarios-backend"
FRONTEND="$SCRIPT_DIR/bin/gestor-horarios-frontend"
PID_FILE="/tmp/gestor-horarios.pid"

cleanup() {
    echo "Apagando Gestor-Horarios..."
    kill $BACKEND_PID $FRONTEND_PID 2>/dev/null
    wait
    echo "Apagado completo."
}
trap cleanup SIGTERM SIGINT EXIT

# Iniciar backend
"$BACKEND" &
BACKEND_PID=$!
sleep 1  # esperar a que el backend abra el socket

# Verificar que el backend está vivo
if ! kill -0 $BACKEND_PID 2>/dev/null; then
    echo "Error: El backend no pudo iniciar"
    exit 1
fi

# Iniciar frontend
"$FRONTEND" &
FRONTEND_PID=$!

# Monitorear
while true; do
    if ! kill -0 $FRONTEND_PID 2>/dev/null; then
        echo "Frontend caído — reiniciando..."
        "$FRONTEND" &
        FRONTEND_PID=$!
    fi
    if ! kill -0 $BACKEND_PID 2>/dev/null; then
        echo "Backend caído — reiniciando..."
        "$BACKEND" &
        BACKEND_PID=$!
        sleep 1
    fi
    sleep 2
done
```

| Ventajas | Desventajas |
|----------|-------------|
| ✅ **Sin refactor del middleware** — IPC ya funciona, no se toca | ❌ Dos procesos en memoria (más RAM) |
| ✅ **Frontend standalone sin Docker** — sin cambios | ❌ Overhead IPC (JSON) para datos locales |
| ✅ **Un solo punto de entrada** — el usuario corre `gestor-horarios` | ❌ Si el script muere, no hay supervisor |
| ✅ **Auto-restart en crashes** — si un proceso falla, se reinicia | |
| ✅ **Bajo esfuerzo** — ~2-3 hrs, script simple de ~50 líneas | |
| ✅ **No toca las issues existentes** — no hay que re-planificar sprints | |
| ✅ **Sencillo de mantener** — bash script, lo entiende cualquier dev | |
| ✅ **Fácil de debuggear** — logs en consola, procesos separados | |

---

## Cuadro Comparativo Final

| Aspecto | Opc 1: Un ejecutable | Opc 2: Auto-launch | Opc 3: Híbrida | **Opc 4 ⭐: Launcher** |
|---------|:--------------------:|:------------------:|:--------------:|:----------------------:|
| **Binarios generados** | 1 | 2 | 1 | 2 |
| **Refactor middleware** | Sí (sacar IPC) | No | Sí (sacar IPC) | **No** |
| **Frontend standalone** | Con `BUILD_BACKEND=OFF` | Sí (ya funciona) | Sí (entry point) | **Sí (ya funciona)** |
| **Punto de entrada único** | Sí (el binary) | No | Sí (el binary) | **Sí (el script)** |
| **Auto-recovery en crash** | No | Sí | No | **Sí** |
| **Complejidad runtime** | Baja | Alta | Baja | **Media** |
| **Esfuerzo estimado** | ~4-6 hrs | ~6-8 hrs | ~5-7 hrs | **~2-3 hrs** |
| **Riesgo de romper algo** | Medio | Bajo | Medio | **Muy bajo** |
| **Mantenibilidad** | Alta | Media | Alta | **Alta** |
| **Impacto en sprints actuales** | Alto (replanificar) | Bajo | Medio | **Mínimo** |
| **Curva de aprendizaje** | Media | Alta | Media | **Baja** |

---

## Veredicto Final

### Recomendación: Opción 4 — Launcher script

**Para un equipo de estudiantes con poca experiencia, la Opción 4 es claramente la mejor:**

| Razón | Explicación |
|-------|-------------|
| **No toca el código existente** | El middleware IPC ya funciona, no se refactoriza. El frontend y backend se mantienen igual. |
| **Bajo riesgo** | Es solo agregar un script + un target CMake. Si algo sale mal, el equipo sigue compilando como antes. |
| **Bajo esfuerzo** | ~2-3 horas contra ~5-8 de las otras opciones. |
| **Fácil de entender** | Un script bash de ~50 líneas. Cualquier miembro del equipo lo puede leer y modificar. |
| **No replanifica sprints** | Las issues del tablero hasta sprint 6 no se tocan. El launcher es una tarea adicional chica. |
| **Auto-recovery** | Si el backend crashea, el script lo reinicia. La institución no tiene que saber de procesos. |
| **Un solo punto de entrada** | El usuario final corre `./gestor-horarios` y ya. Igual de simple que un solo binary. |

### Cuándo re-evaluar

Si en el futuro el middleware IPC se vuelve un cuello de botella de rendimiento, siempre se puede migrar a la Opción 1 o 3. Pero para el alcance actual del proyecto (gestión de horarios académicos), la latencia de QLocalSocket es perfectamente aceptable.

---

*Documento generado el 4 de julio de 2026*
