# Diagrama de Dependencias — Gestor Horarios

Diagramas Mermaid de issues por milestone con colores por área y flechas de dependencia.

**Leyenda:**
- 🔴 `area-backend` — Luis, Dani, Nicole
- 🔵 `area-frontend` — Paola
- 🟡 `area-middleware/qa` — Manuel
- 🟣 `area-docs` — Documentación

---

## MVP Base (Sprints 2–5)

```mermaid
flowchart TB
    classDef backend fill:#d73a4a,color:#fff,stroke:#d73a4a
    classDef frontend fill:#2d9ff3,color:#fff,stroke:#2d9ff3
    classDef middleware fill:#fbca04,color:#000,stroke:#fbca04

    subgraph S2["Sprint 2"]
        S2L["Luis: OR-Tools + estructura"]:::backend
        S2D["Dani: CRUD profesores"]:::backend
        S2P["Paola: Maqueta login"]:::frontend
        S2M["Manuel: Setup middleware"]:::middleware
        S2N["Nicole: Esquema SQLite"]:::backend
    end

    subgraph S3["Sprint 3"]
        S3L["Luis: Algoritmo CP-SAT"]:::backend
        S3D["Dani: CRUD materias"]:::backend
        S3P["Paola: Formulario profesores"]:::frontend
        S3M["Manuel: Endpoints CRUD"]:::middleware
        S3N["Nicole: CRUD aulas"]:::backend
    end

    subgraph S4["Sprint 4"]
        S4L["Luis: Restricciones"]:::backend
        S4D["Dani: CRUD horarios"]:::backend
        S4P["Paola: Tabla horarios"]:::frontend
        S4M["Manuel: Validación datos"]:::middleware
        S4N["Nicole: Servicio consultas"]:::backend
    end

    subgraph S5["Sprint 5"]
        S5L["Luis: Exportar solución"]:::backend
        S5D["Dani: Endpoints generación"]:::backend
        S5P["Paola: Vista exportación"]:::frontend
        S5M["Manuel: Manejo errores"]:::middleware
        S5N["Nicole: Exportación JSON/CSV"]:::backend
    end

    S2D --> S2L
    S2N --> S2L

    S3L --> S2L
    S3D --> S2N
    S3P --> S2D
    S3M --> S2D
    S3N --> S2N

    S4L --> S3L
    S4D --> S3D & S3N
    S4P --> S4D
    S4M --> S3M
    S4N --> S3D & S3N

    S5L --> S4L
    S5D --> S4L
    S5P --> S5L
    S5M --> S4M
    S5N --> S4D
```

---

## Beta (Sprints 6–9)

```mermaid
flowchart TB
    classDef backend fill:#d73a4a,color:#fff,stroke:#d73a4a
    classDef frontend fill:#2d9ff3,color:#fff,stroke:#2d9ff3
    classDef middleware fill:#fbca04,color:#000,stroke:#fbca04

    subgraph S6["Sprint 6"]
        S6L["Luis: Validar conflictos"]:::backend
        S6D["Dani: CRUD disponibilidad"]:::backend
        S6P["Paola: Calendario interactivo"]:::frontend
        S6M["Manuel: Caché consultas"]:::middleware
        S6N["Nicole: Validar conflictos dispon."]:::backend
    end

    subgraph S7["Sprint 7"]
        S7L["Luis: Optimización intermedia"]:::backend
        S7D["Dani: Endpoint comparar soluciones"]:::backend
        S7P["Paola: Dashboard métricas"]:::frontend
        S7M["Manuel: Logging estructurado"]:::middleware
        S7N["Nicole: Comparador soluciones"]:::backend
    end

    subgraph S8["Sprint 8"]
        S8L["Luis: Reasignación rápida"]:::backend
        S8D["Dani: Endpoints edición masiva"]:::backend
        S8P["Paola: Filtros y búsqueda"]:::frontend
        S8M["Manuel: Autenticación JWT"]:::middleware
        S8N["Nicole: Edición masiva backend"]:::backend
    end

    subgraph S9["Sprint 9"]
        S9L["Luis: Reporte conflictos"]:::backend
        S9D["Dani: Endpoint estadísticas"]:::backend
        S9P["Paola: Exportación PDF"]:::frontend
        S9M["Manuel: Rate limiting"]:::middleware
        S9N["Nicole: Reportes y estadísticas"]:::backend
    end

    S6L --> S5L
    S6D --> S4D
    S6P --> S6D
    S6M --> S5M
    S6N --> S6D

    S7L --> S6L
    S7D --> S7L
    S7P --> S7D
    S7N --> S7L

    S8L --> S7L
    S8P --> S4P
    S8M --> S7M

    S9L --> S8L
    S9D --> S9L
    S9P --> S5P
    S9M --> S8M
    S9N --> S9L
```

---

## Release 1.0 (Sprints 10–13)

```mermaid
flowchart TB
    classDef backend fill:#d73a4a,color:#fff,stroke:#d73a4a
    classDef frontend fill:#2d9ff3,color:#fff,stroke:#2d9ff3
    classDef middleware fill:#fbca04,color:#000,stroke:#fbca04
    classDef docs fill:#7057ff,color:#fff,stroke:#7057ff

    subgraph S10["Sprint 10"]
        S10L["Luis: Carga masiva"]:::backend
        S10D["Dani: Validar carga masiva"]:::backend
        S10P["Paola: Interfaz importación"]:::frontend
        S10M["Manuel: Documentación API"]:::docs
        S10N["Nicole: Procesar carga masiva"]:::backend
    end

    subgraph S11["Sprint 11"]
        S11L["Luis: Algoritmo preferencias"]:::backend
        S11D["Dani: Restricciones por aula"]:::backend
        S11P["Paola: Asistente generación"]:::frontend
        S11M["Manuel: Webhooks"]:::middleware
        S11N["Nicole: Preferencias profesores"]:::backend
    end

    subgraph S12["Sprint 12"]
        S12L["Luis: Refactor solver"]:::backend
        S12D["Dani: Historial cambios"]:::backend
        S12P["Paola: Modo oscuro"]:::frontend
        S12M["Manuel: Monitoreo"]:::middleware
        S12N["Nicole: Optimizar SQLite"]:::backend
    end

    subgraph S13["Sprint 13"]
        S13L["Luis: Correcciones finales"]:::backend
        S13D["Dani: Correcciones finales"]:::backend
        S13P["Paola: Correcciones UI"]:::frontend
        S13M["Manuel: Despliegue"]:::middleware
        S13N["Nicole: Tests aceptación"]:::docs
    end

    S10D --> S10L
    S10P --> S10L
    S10N --> S10L

    S11L --> S10L
    S11P --> S11L
    S11N --> S11L

    S12L --> S11L
    S12M --> S11M

    S13L --> S12L & S12D & S12P & S12M & S12N
    S13D --> S12L & S12D & S12P & S12M & S12N
    S13P --> S12P
    S13M --> S12M
    S13N --> S12L & S12D & S12P & S12M & S12N
```
