# Tarea: Exportación de Horarios a CSV

> Referencia: [Issue #35](https://github.com/Servicio-Comunitario-Gestor-Horarios/Dev_Servicio-Comunitario_Gestor-Horarios/issues/35)
> Asignado: Nicole (Niko0510)
> Sprint: 5
> Estado: Propuesta actualizada

## Problema original (Issue #35)

La exportación del solver (S5-I2) cubre horarios generados automáticamente. No hay exportación de horarios creados manualmente.

## Propuesta original

Endpoint de exportación para horarios manuales:
- `GET /schedules/{id}/export?format=json|csv`
- Mismo formato que S5-I2 pero desde datos persistidos en SQLite
- Reusa serializadores de S5-I1

## Nueva propuesta (actualización)

Crear una **función genérica de exportación a CSV** que:

1. Acepta un `HorarioSalida` (struct C++) o un `QJsonObject` (JSON del horario)
2. Exporta a CSV en **dos formatos**:
   - **Archivo 1: Horarios por curso** — Una hoja por curso, todo en un solo archivo CSV
   - **Archivo 2: Horarios por profesor** — Un archivo con los horarios de cada profesor

## Especificación del formato CSV

### Archivo 1: Horarios por curso

Un solo archivo CSV con secciones separadas por curso. Cada curso tiene su "hoja" (bloque de filas).

```csv
Curso: 1A (Turno: manana)
Dia,Slot 0,Slot 1,Slot 2,Slot 3,Slot 4,Slot 5
Lunes,Matematica (Prof. Garcia / Aula 101),Fisica (Prof. Garcia / Aula 101),,Espanol (Prof. Martinez / Aula 101),Historia (Prof. Martinez / Aula 101),Matematica (Prof. Garcia / Aula 101)
Martes,...
Miercoles,...
Jueves,...
Viernes,...

Curso: 2B (Turno: tarde)
Dia,Slot 6,Slot 7,Slot 8,Slot 9,Slot 10,Slot 11
Lunes,...
```

**Notas:**
- Los índices de `materia`, `profesor` y `aula` se resuelven a nombres legibles
- Se necesita el `SolverConfig` original para resolver índices → nombres
- Los slots vacíos quedan en blanco
- Se incluye profesor y aula entre paréntesis

### Archivo 2: Horarios por profesor

```csv
Profesor: Prof. Garcia
Dia,Slot 0,Slot 1,Slot 2,Slot 3,Slot 4,Slot 5,Slot 6,Slot 7,Slot 8,Slot 9,Slot 10,Slot 11
Lunes,1A - Matematica,1A - Fisica,,,,,,,,
Martes,,,,1A - Matematica,,,,,,,,
Miercoles,...,1A - Matematica,...,,,,,,
Jueves,...
Viernes,...

Profesor: Prof. Lopez
Dia,Slot 6,Slot 7,Slot 8,Slot 9,Slot 10,Slot 11
Lunes,2B - Espanol,...,,,,,
```

**Notas:**
- Se muestran TODOS los slots (0-11) para cada profesor
- Cada celda indica: `Curso - Materia`
- Los slots vacíos quedan en blanco

## Funciones a implementar

### `exportarCsvCursos(horario, config) → Resultado<QString>`

```cpp
/**
 * @brief Exporta horario a CSV con una sección por curso.
 * @param horario Struct HorarioSalida con las asignaciones
 * @param config SolverConfig original para resolver índices → nombres
 * @return Contenido del CSV como string, o error
 */
Resultado<QString> exportarCsvCursos(
    const HorarioSalida& horario,
    const SolverConfig& config
);
```

### `exportarCsvProfesores(horario, config) → Resultado<QString>`

```cpp
/**
 * @brief Exporta horario a CSV con una sección por profesor.
 * @param horario Struct HorarioSalida con las asignaciones
 * @param config SolverConfig original para resolver índices → nombres
 * @return Contenido del CSV como string, o error
 */
Resultado<QString> exportarCsvProfesores(
    const HorarioSalida& horario,
    const SolverConfig& config
);
```

### `exportarJson(horario) → QJsonObject`

```cpp
/**
 * @brief Exporta horario a JSON (wrapper de toJson).
 * @param horario Struct HorarioSalida
 * @return Objeto JSON listo para serializar
 */
QJsonObject exportarJson(const HorarioSalida& horario);
```

## Archivos a crear

| Archivo | Descripción |
|---------|-------------|
| `src/backend/include/backend/services/ServicioExportacion.hpp` | Header con las 3 funciones |
| `src/backend/src/services/ServicioExportacion.cpp` | Implementación |
| `test/backend/test_servicio_exportacion.cpp` | Tests GTest |

## Dependencias

- `horario_salida.hpp` — Struct `HorarioSalida`, `CursoOutput`, `DiaOutput`, `AsignacionOutput`
- `solver_config.hpp` — `SolverConfig` con arrays de materias, profesores, aulas
- `resultado.hpp` — Template `Resultado<T>`

## Resolución de índices → nombres

El CSV necesita nombres legibles. La función recibe `SolverConfig` que contiene:

```cpp
// Para resolver materia (int) → nombre:
config.materias[indice].nombre  // "Matematica", "Fisica", etc.

// Para resolver profesor (int) → nombre:
config.profesores[indice].nombre  // "Prof. Garcia", etc.

// Para resolver aula (int) → nombre:
config.aulas[indice].nombre  // "Aula 101", etc.
```

## Criterios de aceptación

- [ ] Exportación de horarios manuales a JSON (wrapper de `toJson()`)
- [ ] Exportación a CSV con una sección por curso (todo en un solo archivo)
- [ ] Exportación a CSV con una sección por profesor
- [ ] Resolución de índices a nombres legibles usando `SolverConfig`
- [ ] Slots vacíos se muestran en blanco
- [ ] Manejo de errores con `Resultado<T>`
- [ ] Tests unitarios con `HorarioSalida` de prueba

## Notas para Nicole

- El `HorarioSalida` ya tiene `toJson()` implementado — la exportación JSON es un wrapper
- Los índices en `AsignacionOutput` son enteros que referencian arrays en `SolverConfig`
- El turno del curso determina qué slots aparecen (0-5 mañana, 6-11 tarde)
- Para profesores, hay que recorrer TODOS los cursos y agrupar por profesor
- Usar `QString::asprintf` o `QStringList` para construir las filas CSV
- Considerar usar `,` como separador y escapar campos que contengan comas
