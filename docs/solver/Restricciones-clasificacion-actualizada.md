# Clasificacion de Restricciones — Actualizada (Jul 2026)

> Revision del spec original de `restricciones-json-analysis` con correcciones del usuario.
> Cambios aplicados: cobertura emergencia hardcodeada, aulas fijas 3 casos, profesor-materia dual, plan de estudio por curso, turnos manuales/automaticos, planificacion personalizable, pool de materias revisado.

---

## Tabla de Clasificacion de Restricciones

| # | Entidad / Comportamiento | Naturaleza | Justificacion Tecnica | Ubicacion | Notas |
|---|---|---|---|---|---|
| 1 | Docente no solapado | Hard (CP-SAT) | NoOverlap por conjunto de variables del profesor | — (C++) | |
| 2 | Disponibilidad docente | Constante JSON | Pares `(dia, slots[])` — preparado para futura restriccion blanda de preferencias horarias | `profesores[].disponibilidad` | Formato: `[{dia: 0, slots: [0,1,2,3,4,5]}, ...]` |
| 3 | Horas semanales cumplidas | Hard (CP-SAT) | Suma de slots asignados = horas requeridas | `profesores[].horas_requeridas` | |
| 4 | Planificacion personalizable | Constante JSON | Horas de aula por profesor, diferencia = planificacion | `profesores[].horas_aula` | **CAMBIADO**: antes era flag global, ahora es por profesor |
| 5 | Cobertura emergencia | Hardcodeada | Constante en codigo C++, no en JSON | — (C++) | **CAMBIADO**: hardcodeada por ahora, mover a JSON despues |
| 6 | Aula no solapada | Hard (CP-SAT) | NoOverlap por conjunto de variables del aula | — (C++) | |
| 7 | Aula fija del curso | Constante JSON | -1 = movil; indice = fija; VACIO = solver asigna | `cursos[].aula_fija` | **CAMBIADO**: 3 casos (antes solo -1/indice) |
| 8 | Capacidad aula | Hardcodeada | Constante en codigo C++, no en JSON | — (C++) | **CAMBIADO**: hardcodeada como cobertura emergencia |
| 9 | Materia unica por aula/slot | Hard (CP-SAT) | Deriva de NoOverlap + unicidad de asignacion | — (C++) | |
| 10 | Profesor-materia: asignado y calificado | Constante JSON | Materias fijas del profesor (asignacion primaria) | `profesores[].materias_asignadas` | **CAMBIADO**: separado en dos tipos |
| 11 | Profesor-materia: suplente | Constante JSON | Materias que puede dar si no hay fijo disponible, con pesos opcionales | `profesores[].materias_suplente` | **NUEVO**: sistema de pesos opcional |
| 12 | Materia-curso: plan de estudio | Constante JSON | Cada curso tiene su plan, un curso no puede ver 2 planes simultaneamente | `cursos[].materias[]` | **CAMBIADO**: ligado al plan de estudio |
| 13 | Horas materia por curso | Constante JSON | Desde PlanEstudio, horas semanales por curso. El solver calcula bloques segun duracion del slot | `cursos[].materias[].horas_semanales` | **CAMBIADO**: de `bloques_por_semana` a `horas_semanales` |
| 14 | Horas profesor requeridas | Constante JSON | Suma de la asignacion del plan | `profesores[].horas_requeridas` | |
| 15 | Un solo plan por curso | Pre-validacion | Rechazar planes conflictivos antes de correr el solver | — (C++) | **NUEVO** |
| 16 | Turno del curso | Constante JSON | Asignacion manana/tarde: manual (usuario define) o automatica (solver optimiza si se omite) | `cursos[].turno` | **CAMBIADO**: si se omite, el solver asigna el turno que mejor le parezca |
| 17 | Division exacta turno/franja | Pre-validacion | (turno_duration - receso) % slot_duration == 0 | — (C++) | |
| 18 | Slot receso: entre bloques | Metadata JSON | Gap de tiempo entre slots, NO ocupa un slot | `receso[].despues_de_slot` | **CAMBIADO**: antes era `slot_index`, ahora es `despues_de_slot` |
| 19 | No solapamiento agregado | Hard (CP-SAT) | Union de no-solapamiento de profesor + aula | — (C++) | |
| 20 | Asignacion slot por curso | Hard (CP-SAT) | Cada curso recibe exactamente N slots por semana | — (C++) | |
| 21 | Horario profesor derivado | Post-procesamiento | Derivar de la asignacion completa despues de que el solver termina | — (C++) | |
| 22 | Generacion parcial | Constante JSON | Solo optimizar subconjunto de cursos | `generacion.cursos_a_generar` | |
| 23 | Horarios existentes fijos | Constante JSON | Tupla completa: `{curso, dia, slot, materia, profesor, aula}` como restriccion dura | `generacion.horarios_existente` | **CAMBIADO**: tupla completa, no simplificada |
| 24 | Estudiantes > 0 | Validacion | Curso no puede tener 0 estudiantes | `cursos[].num_estudiantes` | **NUEVO** |
| 25 | Materias > 0 | Validacion | Pool de materias no puede estar vacio | `materias[]` | **NUEVO** |
| 26 | Profesores > 0 | Validacion | Pool de profesores no puede estar vacio | `profesores[]` | **NUEVO** |
| 27 | Un solo plan por curso (validacion) | Pre-validacion | Un curso no puede pertenecer a dos planes de estudio | `cursos[].plan` | **NUEVO** |
| 28 | Turno manual o automatico | Config JSON | Si `cursos[].turno` se omite, el solver optimiza. Si se define, se respeta | `cursos[].turno` | **CAMBIADO**: solver optimiza por defecto, usuario puede forzar |

---

## Resumen de Cambios vs Spec Original

### Cambios en la clasificacion
- **Restriccion 5 (Cobertura emergencia)**: de Soft/penalizacion JSON → Hardcodeada en C++
- **Restriccion 8 (Capacidad aula)**: de Soft/penalizacion JSON → Hardcodeada en C++
- **Restriccion 7 (Aula fija)**: de 2 casos (-1/indice) → 3 casos (-1/indice/vacio)
- **Restriccion 10 (Profesor-materia)**: dividida en 10 (asignado) + 11 (suplente)
- **Restriccion 12 (Materia-curso)**: ligada al plan de estudio por curso
- **Restriccion 13 (Horas materia)**: de global por materia → especifico por curso
- **Restriccion 16 (Turno)**: de solo automatico → manual o automatico
- **Restriccion 18 (Receso)**: de `slot_index` → `despues_de_slot` (gap entre bloques)
- **Restriccion 4 (Planificacion)**: de flag global → personalizable por profesor

### Nuevas restricciones (24-28)
- Validaciones de entidades no vacias (estudiantes, materias, profesores)
- Un solo plan por curso
- Turno manual o automatico

### Cambios de enfoque
- **Pool de materias**: ya no es generico global, ahora es especifico por curso (ligado al plan de estudio)
- **Planificacion**: enlazada a profesores especificos, horas de aula personalizables
- **Receso**: conceptualizado como gap entre bloques, no como slot que se ocupa

---

## Nota sobre el Receso

El modelo original usaba `receso[].slot_index` asumiendo que el receso "ocupaba" un slot. Esto es conceptualmente incorrecto: si cada slot es un bloque de clase de 40 min, el receso de 10 min es un **gap** entre dos slots, no un slot en si.

**Modelo corregido**:
```json
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
}
```

Para el solver CP-SAT, el receso es irrelevante — el solver asigna clases a indices de slot (0-5 manana, 6-11 tarde) y el receso es metadata que la UI usa para mostrar el break. El solver NO necesita saber cuando hay receso.

---

## Nota sobre el Pool de Materias

**Decision adoptada: Opcion 3 (Hibrida)**

El spec original usaba un pool generico global `materias[]` con `cursos[]` como indices. El usuario senalo que esto no funciona cuando cada curso tiene su propio plan de estudio (distintas materias, distintas horas).

La solucion hibrida combina:
- Pool global `materias[]` para unicidad de nombres (una sola entrada "Matematica")
- Asignacion por curso `cursos[].materias[]` con indices al pool + `horas_semanales` especificas

```json
"materias": [
  { "nombre": "Matematica" },
  { "nombre": "Fisica" }
],
"cursos": [
  {
    "nombre": "1A",
    "plan": "Plan Ciencias",
    "materias": [
      { "materia": 0, "horas_semanales": 5 },
      { "materia": 1, "horas_semanales": 4 }
    ]
  }
]
```

El solver calcula automaticamente cuantos bloques se necesitan: `horas_semanales / (duracion_slot / 60)`.

---

## Nota sobre Profesor-Materia Dual

El usuario defino dos tipos de asignacion:

1. **Asignado y calificado** (`materias_asignadas`): Materias que el profesor dicta normalmente. Asignacion fija.
2. **Suplente** (`materias_suplente`): Materias que el profesor PUEDE dar si no hay un profesor fijo disponible. Solo se activa bajo esta condicion. Soporta sistema de pesos opcional para priorizar entre multiples suplentes.

```json
"profesores": [
  {
    "nombre": "Prof. Garcia",
    "horas_requeridas": 40,
    "horas_aula": 36,
    "materias_asignadas": [0, 1],
    "materias_suplente": [
      { "materia": 2, "peso": 1 },
      { "materia": 3, "peso": 0.5 }
    ],
    "disponibilidad": [
      { "dia": 0, "slots": [0, 1, 2, 3, 4, 5] },
      { "dia": 1, "slots": [0, 1, 2, 3, 4, 5] },
      { "dia": 2, "slots": [0, 1, 2, 3, 4, 5] },
      { "dia": 3, "slots": [0, 1, 2, 3, 4, 5] },
      { "dia": 4, "slots": [0, 1, 2, 3, 4, 5] }
    ]
  }
]
```

El peso es opcional y se usa solo si hay multiples suplentes para la misma materia. El solver prefiere al suplente con mayor peso.

La disponibilidad usa formato de pares `(dia, slots[])` preparado para futura restriccion blanda de preferencias horarias (ej. profesor prefiere manana sobre tarde).
