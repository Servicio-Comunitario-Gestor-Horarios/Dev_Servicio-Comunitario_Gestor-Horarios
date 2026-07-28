# Motor de Solver CP-SAT — Plan Completo

> Documento maestro del motor de generación de horarios. Cubre: esquema de entrada, esquema de salida, sistema de presets, restricciones, y validaciones.
>
> Última actualización: Jul 2026

---

## 1. Visión General

El motor CP-SAT genera horarios académicos automáticamente. Recibe un JSON de configuración con las entidades del plan de estudio, profesores, aulas y restricciones. Produce un JSON de salida con las asignaciones agrupadas por curso y día.

**Flujo de datos:**

```
Frontend (Qt6)                    Backend (C++)                   Solver CP-SAT
┌──────────────┐   IPC (JSON)    ┌──────────────┐    internally   ┌──────────────┐
│ Configurador │ ──────────────→ │  Validación  │ ──────────────→ │  CP-SAT      │
│   + Presets  │                 │  + Parseo    │                 │  Model       │
└──────────────┘                 └──────────────┘                 └──────┬───────┘
                                                                         │
                                                               ┌─────────▼─────────┐
                                                               │  Output JSON      │
                                                               │  (agrupado por    │
                                                               │   curso/día)      │
                                                               └───────────────────┘
```

**Archivos involucrados:**

| Archivo | Propósito |
|---------|-----------|
| `config/Solver.json` | Template por defecto (factory defaults) |
| `config/presets/*.json` | Configuraciones guardadas por el usuario |
| `output/*.json` | Resultados de corridas del solver |
| C++ | Sin cambios en esta fase |

---

## 2. Arquitectura del Sistema

### 2.1 Capas

```
┌─────────────────────────────────────────────────┐
│  FRONTEND (Qt6 Widgets)                         │
│                                                 │
│  ┌──────────────┐  ┌──────────────────────────┐ │
│  │  Configurador│  │  Visualizador de Horario │ │
│  │  de Solver   │  │  (resultado)             │ │
│  └──────┬───────┘  └──────────────────────────┘ │
│         │                                       │
│  ┌──────▼───────┐                               │
│  │  Gestor de   │  Lee/guarda presets de        │
│  │  Presets     │  config/presets/              │
│  └──────┬───────┘                               │
└─────────┼───────────────────────────────────────┘
          │ IPC (JSON blob)
┌─────────▼───────────────────────────────────────┐
│  BACKEND (C++)                                  │
│                                                 │
│  1. Recibe JSON de configuración                │
│  2. Valida schema (13 secciones, tipos, rangos) │
│  3. Convierte a modelo CP-SAT                   │
│  4. Ejecuta solver                              │
│  5. Convierte resultado a JSON de salida        │
│  6. Retorna vía IPC                             │
└─────────────────────────────────────────────────┘
```

### 2.2 Ciclo de Vida de una Configuración

1. **Crear**: Frontend carga template (`config/Solver.json`), ajusta parámetros
2. **Guardar**: Serializa a `config/presets/nombre-preset.json`
3. **Cargar**: Lee preset desde directorio, popula el Configurador
4. **Generar**: Envía JSON al backend → solver produce resultado
5. **Guardar resultado**: Opcionalmente guarda output en `output/`
6. **Generación parcial**: Carga output anterior como `horarios_existente`, reoptimiza subset de cursos

---

## 3. JSON de Entrada (Configuración)

### 3.1 Schema Completo

#### Top-level

| Campo | Tipo | Requerido | Descripción |
|-------|------|-----------|-------------|
| `version` | string | Sí | Versión del schema (semver) |
| `meta` | object | Sí | Metadata de la configuración |
| `dimensiones` | object | Sí | Conteos de entidades para allocación fija |
| `franja_horaria` | object | Sí | Configuración de duración de slots |
| `turnos` | object | Sí | Definiciones de turno mañana/tarde |
| `receso` | object | Sí | Períodos de receso por turno |
| `cursos` | array | Sí | Definiciones de cursos (no vacío) |
| `profesores` | array | Sí | Definiciones de profesores (no vacío) |
| `materias` | array | Sí | Pool global de materias (no vacío) |
| `aulas` | array | Sí | Definiciones de aulas (no vacío) |
| `planificacion` | object | Sí | Configuración de planificación por profesor |
| `generacion` | object | Sí | Configuración de generación parcial |
| `penalizaciones` | object | Sí | Pesos de penalización (hardcoded por ahora) |

#### `meta`

| Campo | Tipo | Ejemplo |
|-------|------|---------|
| `descripcion` | string | `"Configuración del solver CP-SAT para horarios académicos"` |
| `fecha_modificacion` | string (ISO) | `"2026-07-19"` |

#### `dimensiones`

| Campo | Tipo | Ejemplo | Restricción |
|-------|------|---------|-------------|
| `num_profesores` | int ≥ 0 | `3` | Debe coincidir con `profesores.length` |
| `num_materias` | int ≥ 0 | `4` | Debe coincidir con `materias.length` |
| `num_aulas` | int ≥ 0 | `2` | Debe coincidir con `aulas.length` |
| `num_cursos` | int ≥ 0 | `2` | Debe coincidir con `cursos.length` |
| `num_dias` | int > 0 | `5` | Días laborables por semana |
| `num_slots_por_dia` | int > 0 | `12` | Total slots (6 manana + 6 tarde) |

#### `franja_horaria`

| Campo | Tipo | Ejemplo | Descripción |
|-------|------|---------|-------------|
| `duracion_minutos` | int > 0 | `40` | Duración de cada bloque de clase |
| `slots_por_turno` | int > 0 | `6` | Cantidad de slots por turno |

#### `turnos`

| Campo | Tipo | Ejemplo | Descripción |
|-------|------|---------|-------------|
| `manana.inicio` | string (HH:mm) | `"07:00"` | Hora de inicio |
| `manana.fin` | string (HH:mm) | `"12:30"` | Hora de fin |
| `manana.slots` | array of int | `[0,1,2,3,4,5]` | Índices de slots asignados |
| `tarde.inicio` | string (HH:mm) | `"12:35"` | Hora de inicio |
| `tarde.fin` | string (HH:mm) | `"18:10"` | Hora de fin |
| `tarde.slots` | array of int | `[6,7,8,9,10,11]` | Índices de slots asignados |

#### `receso`

| Campo | Tipo | Ejemplo | Descripción |
|-------|------|---------|-------------|
| `manana.despues_de_slot` | int | `1` | Último bloque antes del receso |
| `manana.duracion_minutos` | int | `10` | Duración del receso |
| `manana.inicio` | string (HH:mm) | `"08:20"` | Hora de inicio |
| `manana.fin` | string (HH:mm) | `"08:30"` | Hora de fin |
| `tarde.despues_de_slot` | int | `8` | Último bloque antes del receso |
| `tarde.duracion_minutos` | int | `10` | Duración del receso |
| `tarde.inicio` | string (HH:mm) | `"14:00"` | Hora de inicio |
| `tarde.fin` | string (HH:mm) | `"14:10"` | Hora de fin |

> **Nota sobre el receso**: El receso NO ocupa un slot. Es un gap de tiempo entre bloques. `despues_de_slot = 1` significa que el receso ocurre después del slot 1 (entre slot 1 y slot 2). El solver ignora el receso — solo la UI lo muestra.

#### `cursos[]`

| Campo | Tipo | Ejemplo | Restricción |
|-------|------|---------|-------------|
| `nombre` | string | `"1A"` | Identificador del curso (año + sección) |
| `turno` | string | `"manana"` | `"manana"`, `"tarde"`, o omitido (solver optimiza) |
| `aula_fija` | int | `-1` | -1 = móvil, índice = fija, omitido = solver asigna |
| `num_estudiantes` | int ≥ 0 | `30` | **Debe ser > 0** |
| `plan` | string | `"Plan Ciencias"` | Nombre del plan de estudio |
| `materias` | array | ver abajo | Materias del curso con horas semanales |

#### `cursos[].materias[]`

| Campo | Tipo | Ejemplo | Descripción |
|-------|------|---------|-------------|
| `materia` | int | `0` | Índice en el pool global `materias[]` |
| `horas_semanales` | int > 0 | `5` | Horas semanales que el curso debe ver |

> **Nota**: El campo se llama `horas_semanales` (no `bloques_por_semana`). El plan de estudio define horas semanales. El solver calcula bloques: `horas_semanales / (duracion_slot / 60)`.

#### `profesores[]`

| Campo | Tipo | Ejemplo | Descripción |
|-------|------|---------|-------------|
| `nombre` | string | `"Prof. García"` | Nombre del profesor |
| `horas_requeridas` | int > 0 | `40` | Total de horas semanales (aula + planificación) |
| `horas_aula` | int > 0 | `36` | Horas en aula. Diferencia = planificación |
| `materias_asignadas` | array of int | `[0, 1]` | Índices de materias que dicta fijamente |
| `materias_suplente` | array | ver abajo | Materias que puede dar como suplente |
| `disponibilidad` | array | ver abajo | Días y slots disponibles |

#### `profesores[].materias_suplente[]`

| Campo | Tipo | Ejemplo | Descripción |
|-------|------|---------|-------------|
| `materia` | int | `2` | Índice en `materias[]` |
| `peso` | float | `1.0` | Prioridad relativa (opcional, default 1.0) |

> **Lógica suplente**: Solo se activa si no hay profesor fijo disponible para esa materia. El solver prefiere al suplente con mayor peso.

#### `profesores[].disponibilidad[]`

| Campo | Tipo | Ejemplo | Descripción |
|-------|------|---------|-------------|
| `dia` | int (0-4) | `0` | 0=Lunes, 1=Martes, ..., 4=Viernes |
| `slots` | array of int | `[0,1,2,3,4,5]` | Slots disponibles ese día |

> **Preparado para futuro**: Formato de pares `(día, slots[])` listo para restricción blanda de preferencias horarias.

#### `materias[]`

| Campo | Tipo | Ejemplo | Descripción |
|-------|------|---------|-------------|
| `nombre` | string | `"Matematica"` | Nombre único de la materia |

> **Pool global**: Una sola entrada por materia. Los cursos referencian por índice.

#### `aulas[]`

| Campo | Tipo | Ejemplo | Descripción |
|-------|------|---------|-------------|
| `nombre` | string | `"Aula 101"` | Nombre del aula |
| `capacidad` | int > 0 | `35` | Capacidad máxima de estudiantes |

#### `planificacion`

| Campo | Tipo | Ejemplo | Descripción |
|-------|------|---------|-------------|
| `activa` | bool | `true` | Si está activa la planificación |

> **Nota**: Las horas de planificación se calculan por profesor: `horas_requeridas - horas_aula`. No hay flag global — es personalizable por profesor.

#### `generacion`

| Campo | Tipo | Ejemplo | Descripción |
|-------|------|---------|-------------|
| `cursos_a_generar` | array of int | `[0, 1]` | Índices de cursos a optimizar. Vacío = todos |
| `horarios_existente` | object | ver abajo | Asignaciones fijas de corridos previos |

#### `generacion.horarios_existente`

Mismo formato que el JSON de salida (ver Sección 4). El output de una corrida se reutiliza como input para generación parcial.

#### `penalizaciones`

| Campo | Tipo | Ejemplo | Descripción |
|-------|------|---------|-------------|
| `capacidad_aula` | int ≥ 0 | `100` | Penalización por exceder capacidad (hardcoded por ahora) |
| `emergencia_profesor` | int ≥ 0 | `50` | Penalización por cobertura de emergencia (hardcoded por ahora) |

> **Hardcoded**: Estos valores están en el JSON pero se implementan como constantes en C++. Se moverán a configuración dinámica si se necesita.

---

### 3.2 Ejemplo Completo — `config/Solver.json`

```json
{
  "version": "1.0",
  "meta": {
    "descripcion": "Configuracion del solver CP-SAT para horarios academicos - Liceo Nacional Robert Serra",
    "fecha_modificacion": "2026-07-19"
  },
  "dimensiones": {
    "num_profesores": 3,
    "num_materias": 4,
    "num_aulas": 2,
    "num_cursos": 2,
    "num_dias": 5,
    "num_slots_por_dia": 12
  },
  "franja_horaria": {
    "duracion_minutos": 40,
    "slots_por_turno": 6
  },
  "turnos": {
    "manana": {
      "inicio": "07:00",
      "fin": "12:30",
      "slots": [0, 1, 2, 3, 4, 5]
    },
    "tarde": {
      "inicio": "12:35",
      "fin": "18:10",
      "slots": [6, 7, 8, 9, 10, 11]
    }
  },
  "receso": {
    "manana": {
      "despues_de_slot": 1,
      "duracion_minutos": 10,
      "inicio": "08:20",
      "fin": "08:30"
    },
    "tarde": {
      "despues_de_slot": 8,
      "duracion_minutos": 10,
      "inicio": "14:00",
      "fin": "14:10"
    }
  },
  "cursos": [
    {
      "nombre": "1A",
      "turno": "manana",
      "aula_fija": 0,
      "num_estudiantes": 30,
      "plan": "Plan Ciencias",
      "materias": [
        { "materia": 0, "horas_semanales": 5 },
        { "materia": 1, "horas_semanales": 4 },
        { "materia": 2, "horas_semanales": 5 },
        { "materia": 3, "horas_semanales": 3 }
      ]
    },
    {
      "nombre": "2B",
      "turno": "tarde",
      "aula_fija": -1,
      "num_estudiantes": 28,
      "plan": "Plan Humanidades",
      "materias": [
        { "materia": 0, "horas_semanales": 4 },
        { "materia": 2, "horas_semanales": 5 },
        { "materia": 3, "horas_semanales": 4 }
      ]
    }
  ],
  "profesores": [
    {
      "nombre": "Prof. Garcia",
      "horas_requeridas": 40,
      "horas_aula": 36,
      "materias_asignadas": [0, 1],
      "materias_suplente": [
        { "materia": 2, "peso": 1.0 }
      ],
      "disponibilidad": [
        { "dia": 0, "slots": [0, 1, 2, 3, 4, 5] },
        { "dia": 1, "slots": [0, 1, 2, 3, 4, 5] },
        { "dia": 2, "slots": [0, 1, 2, 3, 4, 5] },
        { "dia": 3, "slots": [0, 1, 2, 3, 4, 5] },
        { "dia": 4, "slots": [0, 1, 2, 3, 4, 5] }
      ]
    },
    {
      "nombre": "Prof. Lopez",
      "horas_requeridas": 40,
      "horas_aula": 40,
      "materias_asignadas": [2],
      "materias_suplente": [],
      "disponibilidad": [
        { "dia": 0, "slots": [6, 7, 8, 9, 10, 11] },
        { "dia": 1, "slots": [6, 7, 8, 9, 10, 11] },
        { "dia": 2, "slots": [6, 7, 8, 9, 10, 11] },
        { "dia": 3, "slots": [6, 7, 8, 9, 10, 11] },
        { "dia": 4, "slots": [6, 7, 8, 9, 10, 11] }
      ]
    },
    {
      "nombre": "Prof. Martinez",
      "horas_requeridas": 36,
      "horas_aula": 32,
      "materias_asignadas": [3],
      "materias_suplente": [
        { "materia": 0, "peso": 0.5 },
        { "materia": 1, "peso": 0.8 }
      ],
      "disponibilidad": [
        { "dia": 0, "slots": [0, 1, 2, 3, 4, 5] },
        { "dia": 1, "slots": [0, 1, 2, 3, 4, 5] },
        { "dia": 2, "slots": [0, 1, 2, 3, 4, 5] },
        { "dia": 3, "slots": [0, 1, 2, 3, 4, 5] },
        { "dia": 4, "slots": [0, 1, 2, 3, 4, 5] }
      ]
    }
  ],
  "materias": [
    { "nombre": "Matematica" },
    { "nombre": "Fisica" },
    { "nombre": "Espanol" },
    { "nombre": "Historia" }
  ],
  "aulas": [
    { "nombre": "Aula 101", "capacidad": 35 },
    { "nombre": "Aula 102", "capacidad": 30 }
  ],
  "planificacion": {
    "activa": true
  },
  "generacion": {
    "cursos_a_generar": [],
    "horarios_existente": {}
  },
  "penalizaciones": {
    "capacidad_aula": 100,
    "emergencia_profesor": 50
  }
}
```

### 3.3 Ejemplo — Generación Parcial

Para reoptimizar solo cursos específicos manteniendo los demás fijos:

```json
{
  "version": "1.0",
  "meta": {
    "descripcion": "Generacion parcial - solo reoptimizar curso 2B",
    "fecha_modificacion": "2026-07-19"
  },
  "dimensiones": {
    "num_profesores": 3,
    "num_materias": 4,
    "num_aulas": 2,
    "num_cursos": 2,
    "num_dias": 5,
    "num_slots_por_dia": 12
  },
  "franja_horaria": {
    "duracion_minutos": 40,
    "slots_por_turno": 6
  },
  "turnos": {
    "manana": {
      "inicio": "07:00",
      "fin": "12:30",
      "slots": [0, 1, 2, 3, 4, 5]
    },
    "tarde": {
      "inicio": "12:35",
      "fin": "18:10",
      "slots": [6, 7, 8, 9, 10, 11]
    }
  },
  "receso": {
    "manana": {
      "despues_de_slot": 1,
      "duracion_minutos": 10,
      "inicio": "08:20",
      "fin": "08:30"
    },
    "tarde": {
      "despues_de_slot": 8,
      "duracion_minutos": 10,
      "inicio": "14:00",
      "fin": "14:10"
    }
  },
  "cursos": [
    {
      "nombre": "1A",
      "turno": "manana",
      "aula_fija": 0,
      "num_estudiantes": 30,
      "plan": "Plan Ciencias",
      "materias": [
        { "materia": 0, "horas_semanales": 5 },
        { "materia": 1, "horas_semanales": 4 },
        { "materia": 2, "horas_semanales": 5 },
        { "materia": 3, "horas_semanales": 3 }
      ]
    },
    {
      "nombre": "2B",
      "turno": "tarde",
      "aula_fija": -1,
      "num_estudiantes": 28,
      "plan": "Plan Humanidades",
      "materias": [
        { "materia": 0, "horas_semanales": 4 },
        { "materia": 2, "horas_semanales": 5 },
        { "materia": 3, "horas_semanales": 4 }
      ]
    }
  ],
  "profesores": [
    {
      "nombre": "Prof. Garcia",
      "horas_requeridas": 40,
      "horas_aula": 36,
      "materias_asignadas": [0, 1],
      "materias_suplente": [
        { "materia": 2, "peso": 1.0 }
      ],
      "disponibilidad": [
        { "dia": 0, "slots": [0, 1, 2, 3, 4, 5] },
        { "dia": 1, "slots": [0, 1, 2, 3, 4, 5] },
        { "dia": 2, "slots": [0, 1, 2, 3, 4, 5] },
        { "dia": 3, "slots": [0, 1, 2, 3, 4, 5] },
        { "dia": 4, "slots": [0, 1, 2, 3, 4, 5] }
      ]
    },
    {
      "nombre": "Prof. Lopez",
      "horas_requeridas": 40,
      "horas_aula": 40,
      "materias_asignadas": [2],
      "materias_suplente": [],
      "disponibilidad": [
        { "dia": 0, "slots": [6, 7, 8, 9, 10, 11] },
        { "dia": 1, "slots": [6, 7, 8, 9, 10, 11] },
        { "dia": 2, "slots": [6, 7, 8, 9, 10, 11] },
        { "dia": 3, "slots": [6, 7, 8, 9, 10, 11] },
        { "dia": 4, "slots": [6, 7, 8, 9, 10, 11] }
      ]
    },
    {
      "nombre": "Prof. Martinez",
      "horas_requeridas": 36,
      "horas_aula": 32,
      "materias_asignadas": [3],
      "materias_suplente": [
        { "materia": 0, "peso": 0.5 },
        { "materia": 1, "peso": 0.8 }
      ],
      "disponibilidad": [
        { "dia": 0, "slots": [0, 1, 2, 3, 4, 5] },
        { "dia": 1, "slots": [0, 1, 2, 3, 4, 5] },
        { "dia": 2, "slots": [0, 1, 2, 3, 4, 5] },
        { "dia": 3, "slots": [0, 1, 2, 3, 4, 5] },
        { "dia": 4, "slots": [0, 1, 2, 3, 4, 5] }
      ]
    }
  ],
  "materias": [
    { "nombre": "Matematica" },
    { "nombre": "Fisica" },
    { "nombre": "Espanol" },
    { "nombre": "Historia" }
  ],
  "aulas": [
    { "nombre": "Aula 101", "capacidad": 35 },
    { "nombre": "Aula 102", "capacidad": 30 }
  ],
  "planificacion": {
    "activa": true
  },
  "generacion": {
    "cursos_a_generar": [1],
    "horarios_existente": {
      "1A": {
        "turno": "manana",
        "dias": [
          {
            "dia": 0,
            "asignaciones": [
              { "slot": 0, "materia": 0, "profesor": 0, "aula": 0 },
              { "slot": 1, "materia": 1, "profesor": 0, "aula": 0 },
              { "slot": 3, "materia": 2, "profesor": 2, "aula": 0 },
              { "slot": 4, "materia": 3, "profesor": 2, "aula": 0 },
              { "slot": 5, "materia": 0, "profesor": 0, "aula": 0 }
            ]
          },
          {
            "dia": 1,
            "asignaciones": [
              { "slot": 0, "materia": 2, "profesor": 2, "aula": 0 },
              { "slot": 1, "materia": 0, "profesor": 0, "aula": 0 },
              { "slot": 3, "materia": 1, "profesor": 0, "aula": 0 },
              { "slot": 4, "materia": 3, "profesor": 2, "aula": 0 },
              { "slot": 5, "materia": 2, "profesor": 2, "aula": 0 }
            ]
          }
        ]
      }
    }
  },
  "penalizaciones": {
    "capacidad_aula": 100,
    "emergencia_profesor": 50
  }
}
```

> **Nota**: Solo el curso `2B` (índice 1) está en `cursos_a_generar`. El curso `1A` tiene sus asignaciones fijas en `horarios_existente` — el solver las trata como restricciones duras.

---

## 4. JSON de Salida (Resultados)

### 4.1 Schema de Salida

| Campo | Tipo | Descripción |
|-------|------|-------------|
| `metadata` | object | Información de la corrida |
| `metadata.fecha_generacion` | string (ISO datetime) | Cuándo se ejecutó |
| `metadata.configuracion` | string | Nombre del preset utilizado |
| `metadata.tiempo_ejecucion_ms` | int | Tiempo del solver en milisegundos |
| `metadata.total_asignaciones` | int | Total de slots asignados |
| `metadata.cursos_generados` | int | Cursos que el solver optimizó |
| `metadata.profesores_asignados` | int | Profesores con al menos una asignación |
| `metadata.conflictos` | int | Conflictos detectados (0 = solución válida) |
| `horarios` | object | Resultado agrupado por curso |
| `horarios.{curso}.turno` | string | Turno del curso (`"manana"` / `"tarde"`) |
| `horarios.{curso}.dias[]` | array | Días con asignaciones |
| `horarios.{curso}.dias[].dia` | int (0-4) | Día de la semana |
| `horarios.{curso}.dias[].asignaciones[]` | array | Slots asignados ese día |
| `horarios.{curso}.dias[].asignaciones[].slot` | int | Índice del slot |
| `horarios.{curso}.dias[].asignaciones[].materia` | int | Índice en `materias[]` |
| `horarios.{curso}.dias[].asignaciones[].profesor` | int | Índice en `profesores[]` |
| `horarios.{curso}.dias[].asignaciones[].aula` | int | Índice en `aulas[]` |

### 4.2 Ejemplo de Salida

```json
{
  "metadata": {
    "fecha_generacion": "2026-07-19T15:30:00",
    "configuracion": "horario-q1",
    "tiempo_ejecucion_ms": 1247,
    "total_asignaciones": 48,
    "cursos_generados": 2,
    "profesores_asignados": 3,
    "conflictos": 0
  },
  "horarios": {
    "1A": {
      "turno": "manana",
      "dias": [
        {
          "dia": 0,
          "asignaciones": [
            { "slot": 0, "materia": 0, "profesor": 0, "aula": 0 },
            { "slot": 1, "materia": 1, "profesor": 0, "aula": 0 },
            { "slot": 3, "materia": 2, "profesor": 2, "aula": 0 },
            { "slot": 4, "materia": 3, "profesor": 2, "aula": 0 },
            { "slot": 5, "materia": 0, "profesor": 0, "aula": 0 }
          ]
        },
        {
          "dia": 1,
          "asignaciones": [
            { "slot": 0, "materia": 2, "profesor": 2, "aula": 0 },
            { "slot": 1, "materia": 0, "profesor": 0, "aula": 0 },
            { "slot": 3, "materia": 1, "profesor": 0, "aula": 0 },
            { "slot": 4, "materia": 3, "profesor": 2, "aula": 0 },
            { "slot": 5, "materia": 2, "profesor": 2, "aula": 0 }
          ]
        },
        {
          "dia": 2,
          "asignaciones": [
            { "slot": 0, "materia": 0, "profesor": 0, "aula": 0 },
            { "slot": 1, "materia": 3, "profesor": 2, "aula": 0 },
            { "slot": 3, "materia": 2, "profesor": 2, "aula": 0 },
            { "slot": 4, "materia": 0, "profesor": 0, "aula": 0 },
            { "slot": 5, "materia": 1, "profesor": 0, "aula": 0 }
          ]
        },
        {
          "dia": 3,
          "asignaciones": [
            { "slot": 0, "materia": 2, "profesor": 2, "aula": 0 },
            { "slot": 1, "materia": 0, "profesor": 0, "aula": 0 },
            { "slot": 3, "materia": 3, "profesor": 2, "aula": 0 },
            { "slot": 4, "materia": 1, "profesor": 0, "aula": 0 },
            { "slot": 5, "materia": 0, "profesor": 0, "aula": 0 }
          ]
        },
        {
          "dia": 4,
          "asignaciones": [
            { "slot": 0, "materia": 1, "profesor": 0, "aula": 0 },
            { "slot": 1, "materia": 2, "profesor": 2, "aula": 0 },
            { "slot": 3, "materia": 0, "profesor": 0, "aula": 0 },
            { "slot": 4, "materia": 3, "profesor": 2, "aula": 0 },
            { "slot": 5, "materia": 2, "profesor": 2, "aula": 0 }
          ]
        }
      ]
    },
    "2B": {
      "turno": "tarde",
      "dias": [
        {
          "dia": 0,
          "asignaciones": [
            { "slot": 6, "materia": 0, "profesor": 0, "aula": 1 },
            { "slot": 7, "materia": 2, "profesor": 1, "aula": 1 },
            { "slot": 9, "materia": 3, "profesor": 2, "aula": 1 },
            { "slot": 10, "materia": 0, "profesor": 0, "aula": 1 },
            { "slot": 11, "materia": 2, "profesor": 1, "aula": 1 }
          ]
        },
        {
          "dia": 1,
          "asignaciones": [
            { "slot": 6, "materia": 3, "profesor": 2, "aula": 1 },
            { "slot": 7, "materia": 0, "profesor": 0, "aula": 1 },
            { "slot": 9, "materia": 2, "profesor": 1, "aula": 1 },
            { "slot": 10, "materia": 3, "profesor": 2, "aula": 1 },
            { "slot": 11, "materia": 0, "profesor": 0, "aula": 1 }
          ]
        },
        {
          "dia": 2,
          "asignaciones": [
            { "slot": 6, "materia": 2, "profesor": 1, "aula": 1 },
            { "slot": 7, "materia": 0, "profesor": 0, "aula": 1 },
            { "slot": 9, "materia": 3, "profesor": 2, "aula": 1 },
            { "slot": 10, "materia": 0, "profesor": 0, "aula": 1 },
            { "slot": 11, "materia": 2, "profesor": 1, "aula": 1 }
          ]
        },
        {
          "dia": 3,
          "asignaciones": [
            { "slot": 6, "materia": 0, "profesor": 0, "aula": 1 },
            { "slot": 7, "materia": 3, "profesor": 2, "aula": 1 },
            { "slot": 9, "materia": 2, "profesor": 1, "aula": 1 },
            { "slot": 10, "materia": 0, "profesor": 0, "aula": 1 },
            { "slot": 11, "materia": 3, "profesor": 2, "aula": 1 }
          ]
        },
        {
          "dia": 4,
          "asignaciones": [
            { "slot": 6, "materia": 2, "profesor": 1, "aula": 1 },
            { "slot": 7, "materia": 0, "profesor": 0, "aula": 1 },
            { "slot": 9, "materia": 3, "profesor": 2, "aula": 1 },
            { "slot": 10, "materia": 2, "profesor": 1, "aula": 1 },
            { "slot": 11, "materia": 0, "profesor": 0, "aula": 1 }
          ]
        }
      ]
    }
  }
}
```

---

## 5. Sistema de Presets

### 5.1 Estructura de Directorios

```
config/
  Solver.json                  ← Template por defecto (factory defaults)
  presets/
    horario-q1.json            ← Preset guardado
    horario-q2-seccion-a.json  ← Otro preset
output/
  horario-q1-2026-07-19.json   ← Resultado de una corrida
  horario-q1-2026-07-20.json   ← Otro resultado
```

### 5.2 Flujo de Presets

| Acción | Descripción |
|--------|-------------|
| **Crear preset** | Frontend carga `config/Solver.json`, usuario ajusta, guarda en `config/presets/nombre.json` |
| **Cargar preset** | Frontend lee `config/presets/nombre.json`, popula el Configurador |
| **Listar presets** | Frontend lista archivos en `config/presets/` |
| **Eliminar preset** | Frontend elimina archivo de `config/presets/` |
| **Generar** | Frontend envía JSON al backend via IPC |
| **Guardar resultado** | Frontend guarda output en `output/` (opcional) |
| **Generación parcial** | Frontend carga output anterior, lo pone en `horarios_existente`, reoptimiza subset |

### 5.3 Formato de Preset

Un preset es idéntico al JSON de entrada completo (Sección 3.2). No tiene campos adicionales — es el mismo schema que `config/Solver.json` pero con valores personalizados.

---

## 6. Clasificación de Restricciones (28)

| # | Restricción | Naturaleza | Ubicación |
|---|-------------|------------|-----------|
| 1 | Docente no solapado | Hard (CP-SAT) | C++ |
| 2 | Disponibilidad docente | JSON constante | `profesores[].disponibilidad` |
| 3 | Horas semanales cumplidas | Hard (CP-SAT) | `profesores[].horas_requeridas` |
| 4 | Planificación personalizable | JSON constante | `profesores[].horas_aula` |
| 5 | Cobertura emergencia | Hardcodeada | C++ |
| 6 | Aula no solapada | Hard (CP-SAT) | C++ |
| 7 | Aula fija del curso | JSON constante | `cursos[].aula_fija` (-1/índice/vacío) |
| 8 | Capacidad aula | Hardcodeada | C++ |
| 9 | Materia única por aula/slot | Hard (CP-SAT) | C++ |
| 10 | Profesor-materia: asignado | JSON constante | `profesores[].materias_asignadas` |
| 11 | Profesor-materia: suplente | JSON constante | `profesores[].materias_suplente` |
| 12 | Materia-curso: plan de estudio | JSON constante | `cursos[].materias[]` |
| 13 | Horas materia por curso | JSON constante | `cursos[].materias[].horas_semanales` |
| 14 | Horas profesor requeridas | JSON constante | `profesores[].horas_requeridas` |
| 15 | Un solo plan por curso | Pre-validación | C++ |
| 16 | Turno del curso | JSON constante | `cursos[].turno` (manual/auto) |
| 17 | División exacta turno/franja | Pre-validación | C++ |
| 18 | Receso: gap entre bloques | Metadata JSON | `receso[].despues_de_slot` |
| 19 | No solapamiento agregado | Hard (CP-SAT) | C++ |
| 20 | Asignación slot por curso | Hard (CP-SAT) | C++ |
| 21 | Horario profesor derivado | Post-procesamiento | C++ |
| 22 | Generación parcial | JSON constante | `generacion.cursos_a_generar` |
| 23 | Horarios existentes fijos | JSON constante | `generacion.horarios_existente` |
| 24 | Estudiantes > 0 | Validación | C++ |
| 25 | Materias > 0 | Validación | C++ |
| 26 | Profesores > 0 | Validación | C++ |
| 27 | Un solo plan por curso | Pre-validación | C++ |
| 28 | Turno manual o automático | Config JSON | `cursos[].turno` |

---

## 7. Validaciones

### 7.1 Pre-Solver (Backend)

| # | Validación | Error si falla |
|---|------------|----------------|
| V1 | JSON parseable | "JSON inválido" |
| V2 | 13 secciones presentes | "Falta sección: {nombre}" |
| V3 | Tipos correctos en cada campo | "Tipo incorrecto en {campo}: esperado {tipo}" |
| V4 | `dimensiones` coincide con longitudes de arrays | "dimensiones.num_profesores={n}, pero profesores.length={m}" |
| V5 | Índices en rango (materias, profesores, aulas) | "Índice {n} fuera de rango para {array}" |
| V6 | `turno` en {"manana", "tarde"} o ausente | "Turno inválido: {valor}" |
| V7 | `aula_fija` = -1, o índice válido, o ausente | "aula_fija inválido: {valor}" |
| V8 | Sin claves desconocidas en top-level | "Clave desconocida: {clave}" |
| V9 | `num_estudiantes` > 0 para cada curso | "Curso {nombre} tiene 0 estudiantes" |
| V10 | `materias[]` no vacío | "Pool de materias vacío" |
| V11 | `profesores[]` no vacío | "Pool de profesores vacío" |
| V12 | Un solo plan por curso (no duplicados) | "Curso {nombre} aparece en múltiples planes" |

### 7.2 Post-Solver (Backend)

| # | Validación | Acción si falla |
|---|------------|-----------------|
| P1 | Todas las horas_semanales cubiertas por curso | Warning en metadata |
| P2 | Ningún profesor excede horas_requeridas | Warning en metadata |
| P3 | Ningún aula excede capacidad | Warning en metadata (penalización aplicada) |
| P4 | Sin conflictos de solapamiento | Error (solver falló) |

---

## 8. Decisiones de Diseño

| Decisión | Elección | Justificación |
|----------|----------|---------------|
| Pool de materias | Híbrido (global + por curso) | Unicidad de nombres + horas específicas por curso |
| Receso | Gap entre slots (`despues_de_slot`) | No ocupa slot; el solver lo ignora |
| Profesor-materia | Dual (asignadas + suplente) | Flexibilidad para suplencias con pesos |
| Planificación | Personalizable por profesor | Cada profesor tiene `horas_aula` propio |
| Penalizaciones | Hardcoded en C++ | Por ahora; mover a JSON si se necesita |
| Aulas fijas | 3 casos (-1/índice/vacío) | Flexibilidad para móvil, fija, o auto |
| Turnos | Solver optimiza si se omite | Automático por defecto; manual si se define |
| Presets | Archivos JSON en directorio | Portabilidad; sin dependencia de BD |
| Output | Agrupado por curso → día | Legible por humanos; reutilizable como input |
| Input parcial | Mismo formato que output | Round-trip: output → input sin transformación |
