# 🎯 Issues Cheatsheet — Gestor Horarios

Atajo rápido para crear issues desde la terminal. Complementa `GITHUB_WORKFLOW.md`.

---

## 👥 Miembros del proyecto

Correr `ghm` en la terminal para ver la lista actualizada:

```
USUARIO             EQUIPO           PERMISO
LuisRojas260305     Tech-Leads       admin
IWinterI            Tech-Leads       admin
magrmanuel25        QA               push
KPrusita            Devs             push
Niko0510            Devs             push
```

| Equipo | ¿Qué puede hacer? |
|--------|-------------------|
| **Tech-Leads** | Todo: merge, rulesets, aprobar PRs, crear issues |
| **QA** | Pushear ramas, crear PRs, aprobar PRs |
| **Devs** | Pushear ramas, crear PRs |

---

## 🏷️ Labels disponibles

| Label | Color | Para qué se usa |
|-------|-------|-----------------|
| `backend` | 🔵 | Tarea exclusiva del equipo de backend |
| `frontend` | 🟢 | Tarea exclusiva del equipo de frontend |
| `bug` | 🔴 | Error encontrado en cualquier entorno |
| `testing` | 🟡 | Relacionado con pruebas o QA |
| `middleware` | 🟠 | Comunicación entre front y back |
| `documentación` | ⚪ | Cambio en la documentación |
| `enhancement` | 🟣 | Mejora opcional |
| `refactor` | ⚫ | Refactorizar una funcionalidad |
| `clean` | ⚪ | Limpiar y formatear código |

Se pueden combinar: `"bug,backend"`, `"frontend,testing"`, etc.

---

## 🚀 Crear issues con `ghi`

### Uso básico

```bash
ghi "titulo" "label1,label2" "descripción" [asignado]
```

### Ejemplos

```bash
# Te asignás a vos mismo
ghi "Módulo de profesores" "backend" "CRUD de profesores con OR-Tools"

# Con labels combinadas
ghi "Login de usuarios" "backend,frontend" "Pantalla de login y validación"

# Asignado a alguien específico
ghi "Bug al generar horarios" "bug,backend" "Falla con más de 30 aulas" "Niko0510"

# Para QA/testing
ghi "Testear módulo de horarios" "testing" "Probar generación con datos de罗伯特Serra" "magrmanuel25"

# Documentación
ghi "Guía de API" "documentación" "Documentar endpoints del middleware"

# Refactor
ghi "Limpiar validación" "refactor,clean" "Simplificar lógica de validación de horarios"
```

### Lo que hace `ghi` automáticamente

1. Crea el issue en el repo
2. Le asigna las labels que le pasaste
3. Te asigna a vos (o a quien le digas)
4. Lo agrega directo al **Project Board** → cae en **To-Do**

### Ver el resultado

```bash
gh issue list --assignee @me
```

---

## 📋 Flujo completo (de principio a fin)

```
1. ghi "Título" "label" "descripción"           ← creás el issue (cae en To-Do)
2. Te asignás (o ya lo hiciste en el comando)
3. Lo movés a "In Progress" desde el board       ← arrastrando en la UI
4. git checkout -b feature/mi-cambio develop
5. Programás, commit, push
6. Abrís PR → se mueve solo a "In Review"
7. Esperás approvals (Tech-Lead + QA)
8. Squash and merge → se mueve solo a "Done"
```

---

## 🧩 Alias disponibles

| Comando | Qué hace |
|---------|----------|
| `ghi "título" "labels" "body" [user]` | Crear issue y agregarlo al proyecto |
| `ghm` | Listar miembros de la org con equipos y permisos |

Los alias están definidos en `~/.zshrc`. Si cerrás la terminal, corre `source ~/.zshrc` para recargarlos.
