# Git Graph — Capturar árbol de commits

Guía para exportar/capturar el árbol de commits del repositorio usando
extensiones de VSCode.

## 1. Instalar Git Graph

1. Abrir VSCode
2. Ir a **Extensions** (`Ctrl+Shift+X`)
3. Buscar `Git Graph`
4. Instalar la extensión **Git Graph** (ID: `mhutchie.git-graph`)
5. Recargar la ventana si es necesario

## 2. Abrir Git Graph

| Método | Acción |
|--------|--------|
| Command Palette | `Ctrl+Shift+P` → `Git: View Git Graph` |
| Botón en Source Control | Click en el icono de Git Graph en la barra de Source Control |
| Atajo personalizado | Asignar en Keyboard Shortcuts si se usa frecuentemente |

## 3. Navegar el árbol

- **Zoom**: Scroll del mouse o gesto de pellizco en trackpad
- **Scroll horizontal**: Arrastrar la línea de tiempo o Shift+scroll
- **Seleccionar commit**: Click en cualquier nodo del árbol
- **Detalles del commit**: Click en un nodo para ver mensaje, diff, autor y fecha
- **Filtrar**: Usar el cuadro de búsqueda para filtrar por mensaje, autor o hash
- **Branches**: Las ramas se muestran como líneas de colores; las etiquetas (tags) aparecen como marcadores

## 4. Exportar como imagen

1. En la vista de Git Graph, hacer click en el botón **Export** (icono de descarga en la esquina superior derecha)
2. Seleccionar **Export to PNG** (Git Graph supports PNG, SVG, and JSON)
3. Elegir ubicación y nombre de archivo
4. Opcional: ajustar profundidad del historial en `git-graph.maxDepth` (settings.json)

## 5. Alternativa: GitLens

Si Git Graph no está disponible, GitLens ofrece funcionalidad similar:

1. Instalar **GitLens** (ID: `eamodio.gitlens`)
2. Abrir Command Palette → `GitLens: Inspect`
3. Navegar a la vista **Commits** o **Graph**
4. Usar **GitLens+** → **Share as Snapshot** para capturar el árbol como imagen
   (requiere cuenta gratuita de GitLens+)
5. También se puede hacer captura manual (ver sección 6)

## 6. Captura manual (fallback)

Si ninguna extensión permite exportar directamente:

1. Asegurarse de que el árbol esté completamente visible
2. Usar la herramienta de captura del sistema:
   - **Linux**: `gnome-screenshot -a` o `flameshot gui`
   - **macOS**: `Cmd+Shift+4`
   - **Windows**: `Win+Shift+S`
3. Seleccionar el área del árbol
4. Guardar como PNG

## 7. Verificación

- El archivo exportado debe mostrar todos los commits principales
- Los nombres de rama y etiquetas deben ser legibles
- La imagen debe tener fondo blanco o claro para buena impresión/visualización
