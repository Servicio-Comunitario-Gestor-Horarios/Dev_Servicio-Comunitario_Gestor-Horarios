# Guía de Timelapses — Gestor de Horarios

Tres formas de visualizar la evolución del proyecto:

1. **Gource** — Animación 3D del árbol git (archivos, commits, desarrolladores)
2. **Board Screenshot** — Capturas diarias del Project Board (issues, avance)
3. **Git Graph** — Árbol de commits interactivo desde VSCode

---

## 1. Gource — Timelapse del Repositorio

### Requisitos

```bash
# Arch/EndeavourOS
sudo pacman -S gource ffmpeg

# Debian/Ubuntu
sudo apt install gource ffmpeg

# macOS
brew install gource ffmpeg
```

### Uso rápido

```bash
# Desde la raíz del repo
gource --highlight-users --viewport 1280x720 \
       --stop-at-end --seconds-per-day 0.5 \
       --start-date "2026-05-01"
```

### Script automatizado

```bash
# Dar permiso (solo la primera vez)
chmod +x scripts/gource-timelapse.sh

# Ejecutar con opciones
./scripts/gource-timelapse.sh --start-date "2026-05-01"

# Personalizar salida
./scripts/gource-timelapse.sh \
  -o ~/Videos/mi-timelapse.mp4 \
  --viewport 1920x1080 \
  -f 60 \
  -s 0.3 \
  --start-date "2026-05-01" \
  --stop-date "2026-12-31"
```

Parámetros del script:

| Flag | Default | Descripción |
|------|---------|-------------|
| `-o, --output PATH` | `timelapse-YYYY-MM-DD.mp4` | Ruta del video MP4 |
| `-r, --viewport WxH` | `1920x1080` | Resolución |
| `-f, --fps N` | `60` | Framerate |
| `-s, --seconds-per-day N` | `0.5` | Velocidad (segundos por día) |
| `--start-date DATE` | — | Fecha inicio (ej: `2026-05-01`) |
| `--stop-date DATE` | — | Fecha fin (ej: `2026-12-31`) |
| `-p, --path DIR` | todo el repo | Solo rastrea archivos bajo DIR. Usar varias veces: `-p src/ -p include/` |

### Excluir carpetas no-código del timelapse

Por defecto Gource muestra **todo** el repo, incluyendo carpetas de configuración (`.github/`, `.agents/`, `docs/`, etc.) que agregan ruido visual.

**Opción 1 — Limitar a `src/` (recomendado):**

```bash
# Solo muestra el código fuente
./scripts/gource-timelapse.sh --path src/ --start-date "2026-05-01"

# Manualmente
gource src/ --highlight-users --viewport 1280x720 \
       --stop-at-end --seconds-per-day 0.5
```

**Opción 2 — Incluir varias carpetas específicas:**

```bash
./scripts/gource-timelapse.sh \
  -p src/ -p include/ -p scripts/ \
  --start-date "2026-05-01"
```

**Opción 3 — Usar git log con exclusión de patrones:**

```bash
git log --all --pretty=format:"%ai|%an|%s" --name-only \
  -- . ':!.agents' ':!.github' ':!.opencode' ':!docs/guias' \
  > /tmp/gource-filtrado.log
gource --log-format custom --highlight-users \
  --viewport 1280x720 --seconds-per-day 0.5 \
  /tmp/gource-filtrado.log
```

Esto excluye carpetas de skills, plantillas de issues, guías internas, etc. — dejando solo el código y documentos relevantes. |

### Personalización visual

```bash
# Leyenda de tipos de archivo
gource --key

# Ocultar elementos específicos
gource --hide "bloom,date,files,filenames,mouse,progress"

# Fondo y colores
gource --background-colour 222222 --font-colour FFFFFF

# Destacar desarrolladores
gource --highlight-users

# Destacar un usuario específico
gource --highlight-user "Luis Rojas"

# Elasticidad de nodos (movimiento más suave/brusco)
gource --elasticity 0.3

# Zoom de cámara
gource --padding 1.2

# Título en pantalla
gource --title "Gestor de Horarios - Sprint 2"

# Logo superpuesto
gource --logo assets/logo.png --logo-offset 20x20

# Fondo transparente (para overlays)
gource --transparent
```

### Avatares personalizados

```bash
# 1. Crear carpeta
mkdir -p assets/avatars

# 2. Agregar imágenes con el nombre exacto del contributor
#    assets/avatars/Luis Rojas.png
#    assets/avatars/Daniel Reyna.png

# 3. Usarlos
gource --user-image-dir assets/avatars \
       --default-user-image assets/avatars/default.png \
       --fixed-user-size
```

### Unificar nombres de autores (`.mailmap`)

Si un mismo desarrollador aparece con nombres distintos (ej: `Luis` y `Luis Rojas`), Gource los muestra como personas separadas.

**Solución:** archivo `.mailmap` en la raíz del repo.

```
# .mailmap
Luis Rojas <luisalexanderrojasguevara@gmail.com>
Daniel Reyna <dan.a.reyna2018@gmail.com>
```

Esto unifica automáticamente nombres duplicados y muestra el nombre canónico.

### Controles durante la reproducción

| Tecla | Acción |
|-------|--------|
| `F1` | Ayuda en pantalla |
| `+` / `-` | Zoom in/out |
| Mouse drag | Mover cámara |
| `Space` | Pausa |
| `Esc` | Salir |

### Exportar a MP4 sin interfaz gráfica

```bash
gource --highlight-users --viewport 1280x720 \
       --stop-at-end --seconds-per-day 0.5 \
       --start-date "2026-05-01" \
       --output-framerate 60 \
       -o - | ffmpeg -y -r 60 -f image2pipe -vcodec ppm \
       -i - -vcodec libx264 -preset ultrafast -pix_fmt yuv420p \
       timelapse-manual.mp4
```

### ⚠️ Limitaciones de Gource (respuesta a la duda de Daniel)

Gource NO muestra:

| Lo que NO hace | Por qué |
|----------------|---------|
| **Distinguir ramas** | Mezcla TODO el historial en una sola línea de tiempo. No diferencia `main` vs `develop` vs `feature/*`. |
| **Identificar módulos** | No entiende "esto es el backend" o "esto es el frontend". Solo muestra directorios y archivos. |
| **Descripciones de archivos** | Solo muestra nombres de archivo y autor del commit, no su propósito o funcionalidad. |

**Alternativas para entender mejor la estructura:**

```bash
# 1. Ver el árbol de ramas (branch topology)
git log --graph --all --oneline --decorate --simplify-by-decoration

# 2. Árbol del proyecto (módulos)
tree -d -L 3 --filelimit 10

# 3. Quién tocó qué (por archivo)
git log --all --name-only --pretty=format:"%an: %s" --since="2026-05-01"

# 4. Commits por módulo (ej: backend)
git log --all --oneline -- src/backend/

# 5. Commits por rama
git log --oneline origin/develop --not origin/main
git log --oneline origin/main --not origin/develop
```

---

## 2. Board Screenshot — Timelapse del Project Board

### ¿Cómo funciona?

Un workflow de GitHub Actions (`board-screenshot.yml`) toma una captura del Project Board cada día a las 6am usando Playwright (navegador headless).

### Activar manualmente

```
https://github.com/Servicio-Comunitario-Gestor-Horarios/Dev_Servicio-Comunitario_Gestor-Horarios/actions
→ Board Screenshot → Run workflow
```

### ¿Dónde se guardan?

Cada captura se almacena como **artifact** de Actions (`board-YYYY-MM-DD.png`) con 90 días de retención.

**Para no perderlos:** descargar periódicamente o modificar el workflow para pushear a una rama (opción más permanente).

### Crear el video desde las capturas acumuladas

```bash
# Descargar todos los artifacts → misma carpeta
ffmpeg -framerate 2 -pattern_type glob -i 'board-*.png' \
  -c:v libx264 -pix_fmt yuv420p board-timelapse.mp4
```

---

## 3. Git Graph (VSCode) — Capturas rápidas

Alternativa ligera para informes escritos. Ver guía dedicada:

```
docs/git-graph-guide.md
```

Atajo rápido: `Ctrl+Shift+P` → `Git Graph: View Git Graph`

---

## Resumen de herramientas

| Herramienta | Para qué | Automático |
|-------------|----------|-----------|
| **Gource** | Video animado del árbol git | Script local |
| **Board Screenshot** | Evolución del Project Board | Actions (diario) |
| **Git Graph** | Capturas estáticas para informes | Manual (VSCode) |
| **`git log --graph`** | Topología de ramas en terminal | Manual |
