# Reunión — Guía de Middleware y QA

> Rol de Manuel: middleware + QA. Este documento explica qué hace,
> cómo hacerlo, y qué herramientas usar (incluyendo OpenCode con gentle-ia).

---

## 1. ¿Qué es el Middleware?

El middleware es la **capa de comunicación** entre frontend y backend.

### Estructura

```
src/middleware/
├── common/           ← Tipos compartidos (estructuras, serialización JSON)
│   ├── types.hpp        → DatosEntrada, ResultadoHorario, Docente, Aula...
│   └── serialization.hpp → Conversión a/desde QJsonObject
│
├── server/           ← Lado del backend
│   ├── api_handler.hpp  → Recibe JSON, llama al backend, devuelve JSON
│   └── server_main.cpp  → Punto de entrada del servicio
│
└── client/           ← Lado del frontend
│    ├── backend_client.hpp → QLocalSocket wrapper
│    └── request_builder.hpp → Construye peticiones tipadas
```

### Cómo funciona la comunicación

```
Frontend (Qt6 GUI)                Backend (OR-Tools)
      │                                 │
      │  JSON por QLocalSocket           │
      │────────────────────────────────▶│
      │  { "docentes": [...],           │
      │    "aulas": [...],              │
      │    "secciones": [...] }         │
      │                                 │
      │  JSON respuesta                  │
      │◀────────────────────────────────│
      │  { "horario": [...],            │
      │    "conflictos": 0,             │
      │    "exito": true }              │
      │                                 │
```

**Protocolo:** JSON sobre named pipes (QLocalSocket).
**Native:** no HTTP, no TCP — es IPC del sistema operativo.

---

## 2. ¿Qué es QA en este proyecto?

QA (Quality Assurance) no es solo "encontrar bugs". Es **garantizar que todo funcione** antes de que llegue a producción.

### Responsabilidades de QA

| Área | Qué hacer |
|------|-----------|
| **Code Review** | Revisar PRs: lógica, estilo, buenas prácticas |
| **Testing** | Probar que la funcionalidad funciona |
| **Integración** | Verificar que frontend + backend + middleware hablan bien |
| **Regresión** | Asegurar que cambios nuevos no rompen lo que ya funcionaba |
| **Documentación** | Asegurar que los cambios están documentados |

---

## 3. Paso a Paso para QA

### Al inicio de un sprint

1. Revisar las issues en **To-Do** del Project Board
2. Identificar qué issues van a necesitar más testing
3. Planificar qué probar y cómo

### Cuando hay un PR para revisar

```
1. Ver qué cambió
   └── gh pr view NUMERO
   └── gh pr diff NUMERO

2. Code review
   └── ¿Sigue las convenciones del proyecto?
   └── ¿Las variables están en español?
   └── ¿Hay prints de depuración olvidados?
   └── ¿La lógica es correcta?

3. Build y test
   └── cmake --build build          (compila?)
   └── ctest --test-dir build       (pasan los tests?)

4. Prueba funcional (si aplica)
   └── Backend: correr el servicio y enviar peticiones
   └── Frontend: abrir la app, probar la funcionalidad

5. Aprobar o pedir cambios
   └── gh pr review NUMERO --approve
   └── gh pr review NUMERO --request-changes --body "razón"
```

### Cuando hay que hacer tests

```
1. Identificar qué probar (según la issue)
2. Escribir tests unitarios (CTest + Google Test)
3. Escribir tests de integración (backend + middleware)
4. Ejecutar todo: ctest --test-dir build -V
5. Reportar resultados en el PR
```

### Para probar el backend manualmente

Si el backend expone su API por el named pipe:

```bash
# Con un script Python (recomendado)
#   - Conectarse al pipe
#   - Enviar JSON
#   - Verificar respuesta

# También se puede probar con un script C++ mínimo
#   - QLocalSocket, connectToServer, write/read
```

---

## 4. Herramientas Recomendadas

### Esenciales

| Herramienta | Para qué |
|-------------|----------|
| **gh CLI** | Ver PRs, issues, revisar código desde terminal |
| **CMake + Ninja** | Compilar el proyecto |
| **CTest** | Ejecutar tests |
| **Qt6** | Compilar frontend y middleware (requiere Qt6::Core) |

### OpenCode con gentle-ia

OpenCode es una herramienta de IA generativa que funciona como **asistente de código en la terminal**. Puede ayudarte con:

**¿Qué es gentle-ia?**
Es un modo de OpenCode que usa el modelo `big-pickle` (Claude) para asistencia de código. Piensa en él como un compañero de programación que:
- Lee y entiende el código del proyecto
- Puede escribir, revisar y modificar código
- Te explica lo que no entiendas
- Es especialmente útil para QA y middleware

**Para QA — cómo te ayuda OpenCode:**

| Situación | Qué decirle a OpenCode |
|-----------|----------------------|
| Revisar un PR | "Revisa este PR, dime si hay problemas de lógica o estilo" |
| Crear tests | "Crea tests unitarios para la función X" |
| Entender código | "Explícame cómo funciona la función X" |
| Depurar errores | "Este código falla con este error, ¿qué está mal?" |
| Escribir scripts de prueba | "Ayúdame a escribir un script que pruebe el middleware" |

**Para Middleware — cómo te ayuda OpenCode:**

| Situación | Qué decirle |
|-----------|------------|
| Entender tipos | "Muéstrame cómo se serializa esto a JSON" |
| Implementar API | "Ayúdame a implementar el handler para la ruta X" |
| Depurar IPC | "La conexión falla, ¿qué puede estar mal?" |

**Cómo empezar con OpenCode:**

```bash
# Desde la terminal, en el directorio del proyecto
opencode
```

Esto abre OpenCode en modo interactivo. Le podés pedir cualquier cosa en lenguaje natural.

### Otras herramientas útiles

| Herramienta | Para qué |
|-------------|----------|
| **Qt Creator** | IDE para Qt, útil para depurar visualmente |
| **gdb** | Depurador de C++ |
| **valgrind** | Detectar fugas de memoria |
| **Docker** | Para correr el backend sin instalar dependencias |

---

## 5. Flujo de Trabajo de Manuel (Middleware + QA)

### Día típico

```
1. Revisar PRs pendientes
   └── gh pr list
   └── Revisar cada PR: diff, build, test

2. Code review de PRs de backend y frontend
   └── ¿Cumple las convenciones?
   └── ¿Los tests pasan?
   └── ¿La implementación es correcta?

3. Trabajar en middleware
   └── Implementar serialización JSON
   └── Implementar handlers del API
   └── Implementar cliente para frontend

4. Hacer pruebas de integración
   └── Conectar frontend + middleware + backend
   └── Verificar que los mensajes fluyen correctamente

5. Reportar issues encontrados
   └── Crear issues con label area-middleware/qa
```

### Para testear sin Docker

No necesitás Docker para todo:
- **Frontend**: se compila nativo con Qt6 en Windows
- **Middleware**: se compila nativo (solo STL + Qt6::Core)
- **Solo backend necesita Docker** (OR-Tools)

```
Si querés probar la comunicación:
┌──────────────────────────────────────────┐
│  Tu máquina (sin Docker)                 │
│                                          │
│  Frontend ─────QLocalSocket────→ Backend │
│  (nativo Qt6)      JSON          (Docker)│
│                                          │
│  O con un script de prueba:              │
│  Script ───────QLocalSocket────→ Backend │
│  (Python/C++)     JSON          (Docker) │
└──────────────────────────────────────────┘
```

---

## 6. Para la Reunión — Puntos a discutir

1. ✅ **Middleware = capa de comunicación** entre frontend y backend
2. ✅ **QA = code review + tests + integración**
3. **¿Tiene sentido usar OpenCode con gentle-ia?** Manuel, ¿te gustaría probarlo?
4. **¿Cómo testear sin backend real?** ¿Necesita un mock del backend para desarrollar el middleware?
5. **¿Scripts de prueba?** ¿Python o C++ para probar el named pipe?
6. **¿Frecuencia de PRs?** ¿PRs chicos y frecuentes o grandes y espaciados?
