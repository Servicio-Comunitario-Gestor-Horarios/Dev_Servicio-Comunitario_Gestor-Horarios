# Reunión — Guía de Issues y Templates

> Cómo crear issues, qué template usar en cada caso, y el ciclo de vida completo.

---

## 1. Tipos de Issues

Solo existen 2 tipos de issues. Cada uno tiene su propio template YAML ya configurado en el repo.

### Reporte de Bug (`bug_report.yml`)

**¿Cuándo usarlo?**
- Algo está **roto**: no funciona como debería
- Un comportamiento esperado no ocurre
- Hay un error, crash, o malfuncionamiento

**Campos del template:**
- [ ] Pre-flight: verificar que no sea duplicado, última versión, probar en develop
- Descripción del bug
- Pasos para reproducir
- Comportamiento esperado vs actual
- Sistema Operativo (dropdown: Windows/macOS/Linux)
- Cliente/Agente (dropdown: gh CLI/Web/Desktop)
- Shell/Terminal (dropdown: bash/zsh/powershell/otro)
- Logs o capturas (opcional)
- Contexto adicional (opcional)

**Labels que se asignan automáticamente:** `bug`, `status:needs-review`

### Solicitud de Funcionalidad (`feature_request.yml`)

**¿Cuándo usarlo?**
- Queremos algo **nuevo** que no existe
- Una mejora sobre algo que ya funciona pero podría ser mejor
- Una tarea planificada en un sprint

**Campos del template:**
- [ ] Pre-flight: no duplicado, entiende el flujo de aprobación
- Descripción del problema (qué necesidad resuelve)
- Solución propuesta (cómo debería funcionar)
- Área afectada (dropdown: Frontend/Backend/Middleware/Docs/Infra)
- Alternativas consideradas (opcional)
- Contexto adicional (opcional)

**Labels que se asignan automáticamente:** `enhancement`, `status:needs-review`

### ¿Bug o Enhancement?

| Situación | Es Bug | Es Enhancement |
|-----------|--------|---------------|
| "La ventana no carga los datos" | ✅ | |
| "La ventana podría tener búsqueda" | | ✅ |
| "El programa se cierra al guardar" | ✅ | |
| "Agregar exportación a PDF" | | ✅ |
| "El cálculo de horarios da resultados incorrectos" | ✅ | |
| "Optimizar velocidad del algoritmo" | | ✅ |

**Regla simple:** si el usuario final ve algo que **no funciona** → bug.
Si **funciona pero podría mejorar** → enhancement.

---

## 2. Labels del Proyecto

### Labels de área

| Label | Color | Cuándo usarla |
|-------|-------|--------------|
| `area-backend` | 🔴 `#d73a4a` | Lógica, OR-Tools, DB |
| `area-frontend` | 🔵 `#2d9ff3` | UI, widgets, ventanas |
| `area-middleware` | 🟡 `#fbca04` | Comunicación, tests |
| `area-docs` | 🟣 `#7057ff` | Documentación |

### Labels de sprint

| Label | Sprint |
|-------|--------|
| `sprint-2` a `sprint-13` | Cada sprint tiene su label |

### Labels de estado (se asignan automáticamente)

| Label | Quién la asigna |
|-------|----------------|
| `status:needs-review` | Automático al crear la issue |
| `status:approved` | Tech-Lead cuando decide implementarla |

### Labels generales

| Label | Color | Cuándo usarla |
|-------|-------|--------------|
| `bug` | 🔴 | Algo que no funciona |
| `enhancement` | 🟣 | Nueva funcionalidad o mejora |
| `tech-debt` | 🟣 | Deuda técnica |
| `urgent` | 🔴 | Prioritario, atención inmediata |
| `good-first-issue` | 🟢 | Para quienes recién empiezan |

---

## 3. Ciclo de Vida de una Issue

```
1. CREAR la issue
   ├── Bug → template bug_report.yml (labels: bug, status:needs-review)
   └── Feature → template feature_request.yml (labels: enhancement, status:needs-review)

2. DISCUTIR en equipo
   → Las issues NO se implementan de una. Primero se hablan.
   → Se revisa si está clara, si el alcance es correcto.

3. APROBAR (Tech-Lead)
   → Agrega label status:approved
   → Se mueve al Project Board (To-Do)

4. ASIGNAR y trabajar
   → Alguien se asigna la issue
   → Mueve a "In Progress"
   → Crea branch, codea, hace PR vinculando la issue

5. REVISAR (QA + Tech-Lead)
   → PR se mueve a "In Review" automáticamente
   → QA revisa y aprueba
   → Tech-Lead revisa y aprueba

6. MERGEAR (Tech-Lead)
   → Squash and merge a develop
   → Issue se cierra y pasa a "Done" automáticamente
```

---

## 4. Cómo Crear una Issue

### Opción 1: GitHub Web

```
Repositorio → Issues → New Issue
→ Elegir "Reporte de Bug" o "Solicitud de Funcionalidad"
→ Completar el formulario
→ Submit
```

### Opción 2: gh CLI

```bash
# Bug
gh issue create --template bug_report.yml \
  --title "[Bug]: error al generar horario con 30+ aulas" \
  --label "bug,area-backend,sprint-3"

# Feature
gh issue create --template feature_request.yml \
  --title "Exportar horario a PDF" \
  --label "enhancement,area-frontend,sprint-5"
```

### Opción 3: Alias `ghi`

```bash
# Uso: ghi "título" "labels" "descripción" [asignado]
ghi "Módulo de profesores" "area-backend" "CRUD de profesores"
ghi "Login UI" "area-frontend" "Pantalla de login" "Paola"
ghi "Testear comunicación" "area-middleware,sprint-3" "Tests de integración" "magrmanuel25"
```

El alias `ghi` agrega la issue al Project Board automáticamente.

---

## 5. Pull Requests

**Template ya configurado en `.github/PULL_REQUEST_TEMPLATE.md`:**

- Resumen de cambios
- Issues relacionadas (`Closes #N`)
- Checklist de calidad:
  - [ ] Código sigue convenciones
  - [ ] Probado localmente
  - [ ] QA aprobó
  - [ ] Tech-Lead aprobó
  - [ ] Apunta a **develop**
  - [ ] Merge debe ser **Squash**

### Reglas de PRs

- Siempre apuntar a `develop`, no a `main`
- Siempre vincular al menos una issue con `Closes #N`
- PRs pequeños (< 200 líneas, < 5 archivos) se revisan mejor
- Si el cambio es grande, dividirlo en issues más chicas

---

## 6. Para la Reunión — Puntos a discutir

1. ✅ **Template de bug**: todos entienden cuándo usarlo
2. ✅ **Template de feature**: todos entienden cuándo usarlo
3. ✅ **Labels**: área + sprint se usan desde el inicio
4. **¿Quién crea las issues?** Idealmente cada quien crea las suyas ANTES de codear
5. **¿Discutir antes de implementar?** Issues se crean para discutir. No se codea directo
6. **¿Dudas con ghi?** El alias está en `~/.zshrc`, si no aparece correr `source ~/.zshrc`
