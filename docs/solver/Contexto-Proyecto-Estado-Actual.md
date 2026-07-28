# Contexto del Proyecto — Estado actual (T1.3 Solver)

> Última actualización: 2026-07-25

## Resumen del Proyecto

**Gestor de Horarios** es una aplicación de escritorio para el Liceo Nacional Robert Serra que genera horarios académicos automáticamente usando un motor de optimización CP-SAT (Google OR-Tools).

**Stack**: C++17, Qt6, CMake, OR-Tools CP-SAT, SQLite, Google Test

## Arquitectura

```
src/backend/
├── include/backend/
│   ├── data/                    ← Structs de datos del dominio
│   │   ├── horario_salida.hpp   ← HorarioSalida (output del solver)
│   │   ├── horario.hpp          ← Horario (modelo interno)
│   │   ├── aula.hpp, materia.hpp, profesor.hpp, etc.
│   ├── solver/
│   │   ├── config/
│   │   │   └── solver_config.hpp  ← Configuración de entrada (13 secciones)
│   │   ├── solver.hpp            ← Clase Solver (orquestador)
│   │   └── variables.hpp         ← Variables de decisión BoolVar
│   ├── services/
│   │   ├── ServicioHorarioSalida.hpp  ← CRUD de horarios de salida
│   │   └── ServicioConsultaHorario.hpp
│   └── resultado.hpp            ← Template Resultado<T>
```

## Struct HorarioSalida (JSON de salida del solver)

Este es el struct principal que representa un horario ya generado. Cada horario contiene metadata y un mapa de cursos con sus asignaciones por día.

### Jerarquía de structs

```
HorarioSalida
├── MetadataSalida
│   ├── fecha_generacion (QString)    ← ISO datetime
│   ├── configuracion (QString)       ← Nombre del preset
│   ├── tiempo_ejecucion_ms (int)
│   ├── total_asignaciones (int)
│   ├── cursos_generados (int)
│   ├── profesores_asignados (int)
│   └── conflictos (int)              ← 0 = solución válida
│
└── horarios (QMap<QString, CursoOutput>)  ← Key = nombre del curso
    └── CursoOutput
        ├── turno (QString)           ← "manana" | "tarde"
        └── dias (QVector<DiaOutput>)
            └── DiaOutput
                ├── dia (int)         ← 0=lunes, 4=viernes
                └── asignaciones (QVector<AsignacionOutput>)
                    └── AsignacionOutput
                        ├── slot (int)      ← 0-11 (0-5=mañana, 6-11=tarde)
                        ├── materia (int)   ← Índice en array de materias
                        ├── profesor (int)  ← Índice en array de profesores
                        └── aula (int)      ← Índice en array de aulas
```

### Ejemplo JSON de salida

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
            { "slot": 3, "materia": 2, "profesor": 2, "aula": 0 }
          ]
        }
      ]
    },
    "2B": {
      "turno": "tarde",
      "dias": [...]
    }
  }
}
```

### Notas importantes

- Todos los campos de `AsignacionOutput` son **índices enteros** que referencian arrays en el `SolverConfig` de entrada
- El mapa `horarios` usa el **nombre del curso** como key (QString)
- El JSON de salida tiene el **mismo formato** que el campo `generacion.horarios_existente` de entrada (round-trip)
- Los índices de slot van de 0-11: 0-5 = turno mañana, 6-11 = turno tarde

## Otro struct relevante: Horario (modelo interno)

Existe también un struct `Horario` que es el modelo interno con objetos completos (no índices):

```cpp
struct Asignacion {
    Aula aula;
    FranjaHoraria franja_horaria;
    Materia materia;
    Profesor profesor;
};

struct Horario {
    QVector<Asignacion> asignaciones;
};
```

Este struct se usa para el CRUD interno, pero el output del solver usa `HorarioSalida` con índices.

## Servicio existente: ServicioHorarioSalida

Ya existe un servicio CRUD para horarios de salida:
- `cargarHorario(nombreArchivo)` → `Resultado<HorarioSalida>`
- `guardarHorario(horario, nombreArchivo)` → `Resultado<bool>`
- `obtenerCurso(nombre)` → `Resultado<CursoOutput>`
- `listarCursos()` → `Resultado<QStringList>`
- `agregarAsignacion(curso, dia, asignacion)` → `Resultado<AsignacionOutput>`
- `eliminarAsignacion(curso, dia, slot)` → `Resultado<bool>`
- `modificarAsignacion(curso, dia, slot, nueva)` → `Resultado<AsignacionOutput>`

## Referencias

- Documento maestro: `docs/solver/Motor-Solver-Plan-Completo.md`
- Tareas de implementación: `docs/solver/Tareas-Implementacion.md`
- Referencia OR-Tools: `Research/OR-Tools CP-SAT - Referencia.md` (en bóveda Obsidian)
