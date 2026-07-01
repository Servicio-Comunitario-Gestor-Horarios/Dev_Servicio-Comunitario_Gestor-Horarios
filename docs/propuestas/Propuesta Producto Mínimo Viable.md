<div style="display: flex; align-items: center; justify-content: center; gap: 20px;">
  <img src="/home/luis/Imágenes/logo.png" width="120" alt="Logo UNEG">
  <div style="text-align: center; font-weight: bold;">
    <p style="margin: 0;">UNIVERSIDAD NACIONAL EXPERIMENTAL DE GUAYANA</p>
    <p style="margin: 0;">COORDINACIÓN GENERAL DE PREGRADO</p>
    <p style="margin: 0;">COORDINACIÓN DE EDUCACIÓN COMUNITARIA</p>
    <p style="margin: 0;">PROYECTO DE CARRERA: Ingenieria en Informática</p>
    <p style="margin: 0;">SEDE: Puerto Ordaz</p>
  </div>
</div>

<div style="text-align: center; margin-top: 20px; font-weight: bold;">
  <p>PROPUESTA DE PRODUCTO MÍNIMO VIABLE</p>
  <p>PROYECTO GESTOR DE HORARIO</p>
</div>
# Responsables.

| **Nombre**        | **Rol**                       |
| ----------------- | ----------------------------- |
| **Daniel Reyna**  | Líder del desarrollo Frontend |
| **Luis Rojas**    | Líder del desarrollo Backend  |
| **Paola Peña**    | Desarrolladora Frontend       |
| **Nicole Sereno** | Desarrolladora Backend        |
| **Manuel Garcia** | Desarrollador Middleware / QA |

# Solución

El motor es un programa que, dadas sus condiciones reales, busca la mejor organización posible de clases, respetando las restricciones rígidas programadas y optimizando las restricciones blandas buscando satisfacer a todos lo mas posible.

## ¿Qué hace este motor?

- **Recibe**:
	- Lista de **docentes** con las materias que dictan y su disponibilidad horaria, preferencias de horas de clase y horas que tienen que ejercer.
	- **Grados** y **secciones** con su carga curricular.
	- **Aulas** con capacidad y tipo.

- **Aplica restricciones rígidas (no negociables)**:
	- Cada asignatura debe completar sus horas semanales.
	- Un docente no puede estar en dos sitios al mismo tiempo.
	- Cumplir con el horario de disponibilidad del docente.
	- Dos secciones no pueden ocupar la misma aula a la misma hora.
	- Cumplimiento de las horas que debe ejercer un docente.

- **Aplica restricciones blandas (negociables)**:
	- Trata de ubicar a cada docente en la franja horaria que prefiera.
	- Evita "ventanas" (horas libres entre clases) para los docentes.

- **Entrega**:
	- Horario en tabla, exportable a PDF.
	- Posibilidad de ajuste manual con re-generación.

# Decisiones Técnicas

| ¿Qué usamos?                        | ¿Por qué?                                                                                                               |
| ----------------------------------- | ----------------------------------------------------------------------------------------------------------------------- |
| Programa de escritorio              | Funciona en cualquier PC del liceo, sin conexión. Los datos no salen de la institución.                                 |
| Lenguaje C++ y biblioteca OR-Tools  | C++ es un lenguaje conocido por su velocidad y eficiencia en el uso de la memoria y los recursos del sistema.           |
| Guardado en archivo (JSON o SQLite) | No requiere instalar servidores de bases de datos ni capacitación avanzada de uso. Hacer respaldo es copiar un archivo. |
| Software completamente gratuito.    | Cero costos de licencias de uso. Pueden instalarlo en varias computadoras.                                              |
| Motor e interfaz separados          | Si más adelante deciden cambiar el diseño del programa, el motor no se toca.                                            |

# Diagrama de flujo.

```mermaid
  flowchart TD
      %% Título del diagrama
      title["SISTEMA GENERADOR Y GESTOR DE HORARIOS ACADÉMICOS\nLiceo Nacional Robert Serra"]

      %% Capa de entrada de datos
      A["Inicio del proceso (Inicio de año escolar)"] --> B["Administrador ingresa datos maestros: Docentes - Asignaturas - Secciones - Aulas"]
      B --> C{"¿Datos completos y correctos?"}
      C -- No --> B
      C -- Sí --> D["Definición de restricciones:"]
      
      %% Restricciones
      D --> D1["Restricciones duras: No solapamiento de docentes - Capacidad máxima del aula - Carga horaria por asignatura"]
      D --> D2["Restricciones blandas: - Preferencias horarias del docente - Balance de carga diaria - Evitar aulas dispersas"]
      D1 & D2 --> E["Ejecutar motor generador (OR-Tools - CP-SAT)"]
      
      %% Núcleo de solución
      E --> F["Solucionador busca asignación que satisfaga restricciones"]
      F --> G{"¿Horario factible encontrado?"}
      G -- Sí --> H["Mostrar horario en formato tabular"]
      G -- No --> I["Revisar restricciones (relajar condiciones o corregir datos)"]
      I --> D
      
      %% Capa de salida y ajustes
      H --> J["Revisión del coordinador"]
      J --> K{"¿Horario aprobado?"}
      K -- No --> I
      K -- Sí --> L["Guardar versión definitiva (JSON / SQLite)"]
      L --> M["Publicación y distribución: Impresión - Consulta docente - Exportación"]
      M --> N["Capacitación al personal en uso y mantenimiento"]
      N --> O["Fin del proceso"]

      %% Estilos para mejorar legibilidad
      style title fill:#f9f,stroke:#333,stroke-width:1px,color:#000
      style A fill:#c8e6c9,stroke:#2e7d32
      style O fill:#c8e6c9,stroke:#2e7d32
      style E fill:#fff9c4,stroke:#fbc02d
      style F fill:#fff9c4,stroke:#fbc02d
      style H fill:#bbdefb,stroke:#1565c0
      style L fill:#bbdefb,stroke:#1565c0
```
