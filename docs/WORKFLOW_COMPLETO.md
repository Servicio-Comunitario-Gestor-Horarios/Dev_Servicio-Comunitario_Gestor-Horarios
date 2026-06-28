# 🏗️ Workflow Completo — Gestor Horarios

> Guía para navegar el día a día del proyecto. Primera experiencia trabajando con
> este flujo, así que está pensada para cubrir dudas comunes, casos borde, y
> situaciones que seguro nos vamos a encontrar.

---

## 📍 El Repo

```
Organización: Servicio-Comunitario-Gestor-Horarios
Repo:         Dev_Servicio-Comunitario_Gestor-Horarios
URL:          https://github.com/Servicio-Comunitario-Gestor-Horarios/Dev_Servicio-Comunitario_Gestor-Horarios
Projecto:     @LuisRojas260305's Gestor-Horarios (https://github.com/orgs/Servicio-Comunitario-Gestor-Horarios/projects/1)
```

---

## 👥 Roles y responsabilidades

| Rol | Personas | En el repo | ¿Qué pueden hacer? |
|---|---|---|---|
| **Tech-Lead** | Luis, Daniel | Admin | Todo: mergear, crear rulesets, aprobar, bypass |
| **QA** | Manuel | Push | Crear ramas, PRs, aprobar PRs (calidad) |
| **Dev** | Paola, Nicole | Push | Crear ramas, PRs |

### Regla de oro

> **Tech-Leads** son los únicos que mergean. Devs y QA programan, crean PRs,
> revisan, pero el botón de "Squash and merge" solo lo aprietan Luis o Daniel.

---

## 🏷️ Labels y cuándo usarlas

| Label | Color | ¿Cuándo se usa? |
|---|---|---|
| `backend` | 🔵 | Todo lo que sea lógica de negocio, OR-Tools, base de datos |
| `frontend` | 🟢 | Todo lo que sea interfaz gráfica, widgets, ventanas |
| `bug` | 🔴 | Algo que no funciona como debería. Urgente por definición |
| `testing` | 🟡 | Crear tests, ejecutar pruebas, verificar cobertura |
| `middleware` | 🟠 | Comunicación frontend ↔ backend, adaptadores, validación |
| `documentación` | ⚪ | Escribir o actualizar docs, README, diagramas |
| `enhancement` | 🟣 | Mejora que suma pero no es crítica. Opcional |
| `refactor` | ⚫ | Reestructurar código sin cambiar comportamiento |
| `clean` | ⚪ | Formatear, limpiar, ordenar código |

### Combinaciones comunes

```
backend + bug        → bug en backend (urgente para Luis/Nicole)
frontend + bug       → bug en frontend (urgente para Daniel/Paola)
frontend + enhancement → mejora en UI (opcional)
testing + middleware → tests de comunicación (para Manuel)
refactor + clean    → ordenar deuda técnica
```

### ⚠️ Atención: diferencia entre `bug` y `enhancement`

Puede ser difícil a veces. Una regla simple:

- **Bug**: algo que **está roto** o no funciona como se espera. El usuario final se ve afectado.
- **Enhancement**: algo que **podría ser mejor**. No está roto, pero se puede mejorar.

*Ejemplo:* "La ventana de profesores no carga los datos" → `bug` / "La ventana de profesores podría tener búsqueda" → `enhancement`

---

## 🌿 Estructura de ramas

```
main                     ← Producción. Bloqueada.
  └── develop            ← Integración. Bloqueada.
       ├── feature/backend/schema-base-de-datos
       ├── feature/frontend/ventana-vista-profesores
       ├── bugfix/horario-mal-generado
       └── hotfix/parche-urgente   ← Solo desde main, para producción
```

### Nomenclatura

```
feature/[area]/[descripción-breve]
bugfix/[descripción]
hotfix/[descripción]
```

### ¿De dónde nace cada rama?

| Rama | Nace de | Se fusiona en |
|---|---|---|
| `feature/*` | `develop` | `develop` |
| `bugfix/*` | `develop` | `develop` |
| `hotfix/*` | `main` | `main` y `develop` |

---

## 🔄 Flujo normal (día a día)

### Para Devs (Paola, Nicole)

```
 1. Elegir un issue del Project Board (columna To-Do)
 2. Asignarse el issue
 3. Moverlo a "In Progress"
 4. git checkout develop
 5. git pull origin develop
 6. git checkout -b feature/area/mi-cambio
 7. Programar, commitear, pushear
 8. gh pr create --base develop --title "..." --body "Closes #N"
 9. Esperar revisiones
10. Tech-Lead + QA aprueban → lo mergean
11. Listo 🎉
```

### Para QA (Manuel)

```
 1. Revisar los PRs abiertos buscando `testing` en labels
 2. Code review + tests de la funcionalidad
 3. Aprobar si está todo bien
 4. Si algo falla, pedir cambios
```

### Para Tech-Leads (Luis, Daniel)

```
 1. Revisar PRs del área que lideran
 2. Verificar que cumplan los requisitos del PR
 3. Aprobar
 4. Cuando QA también apruebe → Squash and merge
 5. Eliminar la rama (automático si configuramos)
```

---

## 🔴 Flujo de emergencia (hotfix)

Cuando algo en **producción** está roto y no puede esperar:

```
 1. Crear rama desde main: git checkout -b hotfix/descripcion main
 2. Programar la corrección
 3. Hacer PR a main (no a develop)
 4. Tech-Lead aprueba + QA aprueba → merge a main
 5. Inmediatamente después, mergear main a develop para no perder el fix
```

### ⚠️ Ojo con hotfix

Un hotfix necesita incluso **más** revisión que un feature normal, porque está tocando
producción. No es "como un feature pero más rápido". Es más riesgo, no menos.

---

## 🛡️ Bypass de Tech-Leads (válvula de escape)

Los Tech-Leads tienen **bypass** en los rulesets. Esto significa que pueden:

- Pushear directo a `main` o `develop` sin PR
- Mergear PRs aunque falten aprobaciones

### ¿Cuándo usarlo?

- Emergencia real (producción caída, bug crítico)
- Algo que necesita salir **ya** y no hay tiempo de esperar QA

### ¿Cuándo NO usarlo?

- "Es que tengo fiaca esperar a Manuel"
- "Es un cambio chiquito, no pasa nada"
- "Solo es una línea de código"

> Si el bypass se vuelve el flujo normal, **no sirve tener QA**. El bypass es para
> emergencias, no para atajos. Úsenlo con criterio.

---

## 📋 Requisitos para que un PR sea aprobado

Checklist para el que revisa:

- [ ] ¿Las variables están en español y siguen convenciones?
- [ ] ¿Las funciones complejas tienen su Doxygen header?
- [ ] ¿Si se cambió un flujo lógico, se actualizó el diagrama Mermaid?
- [ ] ¿Se deshabilitaron `print` / `console.log` de depuración?
- [ ] ¿La funcionalidad está bien implementada?
- [ ] ¿Pasan los tests?

---

## 🔗 Commits con referencia a issues

Siempre vincular commits y PRs a su issue:

```bash
git commit -m "feat(backend): agregar cálculo de carga horaria

Closes #12"
```

Keywords que GitHub reconoce: `Closes`, `Fixes`, `Resolves`

---

## 📝 Convención de commits

Del documento "Flujo de trabajo":

```
tipo(alcance): descripción breve

cuerpo opcional

referencia a issue #ID
```

### Tipos

| Tipo | Cuándo |
|---|---|
| `feat` | Nueva funcionalidad |
| `fix` | Corrección de errores |
| `docs` | Cambio en documentación |
| `style` | Formateo, sin cambio lógico |
| `refactor` | Reestructurar código |
| `test` | Agregar/modificar tests |

### Alcances

| Alcance | Área |
|---|---|
| `backend` | Lógica de negocio, OR-Tools |
| `frontend` | UI, widgets, ventanas |
| `middleware` | Comunicación front-back |
| `qa` | Tests, calidad |
| `core` | Afecta todo el proyecto |

---

## 🧩 El Project Board

### Columnas

| Columna | ¿Qué hay ahí? |
|---|---|
| **Backlog** | Ideas sueltas, sin priorizar. No hay compromiso de hacerlas |
| **To-Do** | Issues listas, priorizadas, esperando que alguien las agarre |
| **In Progress** | Alguien está codeando esto AHORA |
| **In Review** | PR abierto. Se mueve solo cuando vinculás un PR |
| **Done** | Terminado. Se mueve solo cuando se mergea el PR |

### Workflows automáticos (ya configurados)

| Trigger | Pasa a |
|---|---|
| Issue creado y agregado al proyecto | → **To-Do** |
| PR vinculado a un issue (`Closes #N`) | → **In Review** |
| PR mergeado | → **Done** |

### Vista recomendada

El proyecto tiene vista **Board** (kanban) y vista **Por Asignado** (tabla).
Si querés ver solo tus issues, filtrá por "Assigneed to me".

---

## 📌 Cosas que probablemente vamos a aprender sobre la marcha

### 1. `git pull --rebase` en develop

Siempre que traigas cambios de `develop`, usá rebase para evitar commits de merge:

```bash
git checkout develop
git pull --rebase origin develop
```

### 2. Si tu rama se atrasó respecto a develop

```bash
git checkout feature/mi-rama
git rebase develop
# Si hay conflictos, resolvelos con:
#   git mergetool  (o a mano)
#   git rebase --continue
git push --force-with-lease
```

`--force-with-lease` es más seguro que `--force`. Git te avisa si alguien más
pusheó a tu rama.

### 3. Un PR debe tener un solo commit (por el Squash)

No importa cuántos commits hagas mientras codeás. Al mergear con **Squash** se
comprime todo a uno solo. Hacé commits chicos mientras trabajás (más fácil para
volver atrás), total se aplastan al mergear.

### 4. ¿Y si el PR tiene conflictos?

GitHub te avisa. Los resuelve quien mergea (Tech-Lead), o podés pedirle al Dev
que resuelva desde su rama con `git rebase develop` y `git push --force-with-lease`.

### 5. ¿Qué pasa si mergeamos algo que rompe develop?

Pasa. Todos rompen develop alguna vez. Se revierte:

```bash
git revert <commit-hash>
git push origin develop
```

O si el fix es rápido, se arregla en un nuevo PR. No es el fin del mundo.

### 6. Issues con referencias cruzadas

Si un issue depende de otro, mencionalo:

```
Depende de: #12
Bloquea: #15
```

### 7. No borrar branches de otros

Cada quien borra sus branches después del merge. Tech-Leads pueden borrar
cualquiera, pero por respeto, avisen antes de borrar la branch de otro.

### 8. El tamaño del PR importa

Un PR de 5 archivos y 200 líneas se revisa bien. Un PR de 30 archivos y 3000
líneas es una pesadilla. Si un cambio es muy grande, fraccionarlo en issues más
chicos.

---

## 🚨 Qué hacer si...

### ...no sé a quién asignarle un issue

- `backend` → Luis o Nicole
- `frontend` → Daniel o Paola
- `middleware` / `testing` → Manuel
- `documentación` → cualquiera

### ...necesito ayuda con algo

Creá un issue con label del área correspondiente y asignalo a la persona que
pueda ayudar. O hablalo directamente en el grupo.

### ...no estoy seguro de si algo es bug o enhancement

Preguntá en el grupo o asignalo a un Tech-Lead para que decida.

### ...me equivoqué de rama y pusheé a develop

```bash
git push origin --delete develop  # SOLO UN TECH-LEAD
git push origin develop           # desde la develop correcta
```

Avisá altiro si pasa. No es grave si se arregla rápido.

### ...el ruleset no me deja pushear

```bash
# Error típico: Updates were rejected because the remote contains work that you do not have locally
git pull --rebase origin develop
git push
```

Si el ruleset te bloquea, probablemente es porque estás intentando pushear a
main o develop directo. Usá una feature branch y PR.

---

## 🧪 Alias útiles (en la terminal)

### Crear issue directo al proyecto

```bash
ghi "Título" "label1,label2" "Descripción" [asignado]
```

### Ver miembros y permisos

```bash
ghm
```

### Ver tus issues abiertos

```bash
gh issue list --assignee @me -R Servicio-Comunitario-Gestor-Horarios/Dev_Servicio-Comunitario_Gestor-Horarios
```

### Ver PRs pendientes de revisión

```bash
gh pr list -R Servicio-Comunitario-Gestor-Horarios/Dev_Servicio-Comunitario_Gestor-Horarios
```

---

*Este archivo no está trackeado por git. Es una guía viva — si encuentran algo
que faltó o algo que aprendieron, agréguenlo sin miedo.*
