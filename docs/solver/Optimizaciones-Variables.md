# Optimizaciones Pendientes — Variables CP-SAT

> **Estado**: Pendiente para tarea futura
> **Archivo actual**: `src/backend/src/solver/variables.cpp`

---

## 1. QList → QSet para lookups de disponibilidad

**Problema**: `QVector<int>::contains()` es O(n). Se llama en el loop más interno.

**Actual**:
```cpp
QVector<int> slotsDisponibles;
for (const auto& disp : prof.disponibilidad) {
    if (disp.dia == d) { slotsDisponibles = disp.slot; break; }
}
for (int s : slotsTurno) {
    if (!slotsDisponibles.contains(s)) continue;  // O(S_d)
```

**Optimizado**:
```cpp
QSet<int> slotsDisponibles;
for (const auto& disp : prof.disponibilidad) {
    if (disp.dia == d) { slotsDisponibles = disp.slot.begin(), disp.slot.end()); break; }
}
for (int s : slotsTurno) {
    if (!slotsDisponibles.contains(s)) continue;  // O(1) amortizado
```

**Impacto**: Reduce factor `(M_p + D × S_d)` a O(1) por lookup.
**Complejidad resultante**: O(C × M_c × P × D × S_t)

---

## 2. Pre-computar slots del turno como QSet

**Problema**: `slotsPorTurno.value()` retorna `QVector<int>`, que se itera en cada curso.

**Optimizado**:
```cpp
// Pre-computar una vez
QMap<QString, QSet<int>> slotsPorTurnoSet;
for (const auto& t : config.turnos.turnos)
    slotsPorTurnoSet[t.nombre] = QSet<int>(t.slot.begin(), t.slot.end());
```

**Impacto**: Menor, pero consistente con la optimización #1.

---

## 3. Cache de disponibilidad por profesor

**Problema**: Para cada (curso, materia, profesor), se recorre `prof.disponibilidad` para buscar el día actual. Con muchos cursos/materias, el mismo profesor se busca repetidamente.

**Optimizado**:
```cpp
// Pre-computar QMap<int, QSet<int>> por profesor
 QVector<QMap<int, QSet<int>>> dispPorProfesor(config.dimensiones.num_profesores);
for (int p = 0; p < config.dimensiones.num_profesores; ++p) {
    for (const auto& d : config.profesores[p].disponibilidad) {
        dispPorProfesor[p][d.dia] = QSet<int>(d.slot.begin(), d.slot.end());
    }
}

// En el loop:
QSet<int> slotsDisponibles = dispPorProfesor[p].value(d, {});
```

**Impacto**: Elimina la búsqueda lineal en disponibilidad. O(1) en vez de O(D).

---

## 4. Iterar materias del curso en vez de todas las materias

**Problema**: El loop actual itera `curso.materias` (correcto), pero si se iterara `config.materias` sería O(M) en vez de O(M_c). Ya está bien — confirmar que no se cambia.

**Estado**: ✅ Ya optimizado (itera `curso.materias`, no `config.materias`).

---

## 5. Bitset para titularidad/suplencia

**Problema**: `materias_asignadas.contains(m)` es O(M_p). Con muchas materias, usar un bitset sería O(1).

**Optimizado**:
```cpp
// Pre-computar por profesor
 QVector<QBitArray> esTitularPorProf(config.dimensiones.num_profesores);
for (int p = 0; p < config.dimensiones.num_profesores; ++p) {
    esTitularPorProf[p].resize(config.dimensiones.num_materias);
    for (int m : config.profesores[p].materias_asignadas)
        esTitularPorProf[p].setBit(m);
}

// En el loop:
if (!esTitularPorProf[p].at(m) && !esSuplente) continue;
```

**Impacto**: O(1) en vez de O(M_p). Relevante si >20 materias.

---

## 6. Estructura de datos unificada para disponibilidad

**Problema**: La disponibilidad se almacena como `QVector<Disponibilidad>` donde cada `Disponibilidad` tiene un día y un vector de slots. Para buscar un día específico, se recorre todo.

**Optimizado**: Cambiar `Disponibilidad` a un `QMap<int, QSet<int>>` directamente en el config:
```cpp
// En vez de QVector<Disponibilidad>
QMap<int, QSet<int>> disponibilidad;  // día → slots
```

**Impacto**: Elimina la búsqueda del día. O(1) en vez de O(D).
**Nota**: Requiere cambiar el parser JSON también.

---

## 7. Mover aulasAssignment al loop de assignment

**Problema**: Los dos loops (assignment y aulaAssignment) son independientes pero recorren los mismos días y slots. Se podrían fusionar.

**Optimizado**:
```cpp
for (int c = 0; c < dim.num_cursos; ++c) {
    // ... assignment ...
    
    // Aula assignment en el mismo loop de días/slots
    if (config.cursos[c].aula_fija == -1) {
        for (int d = 0; d < dim.num_dias; ++d) {
            for (int s : slotsTurno) {
                for (int a = 0; a < dim.num_aulas; ++a) {
                    vars.aulaAssignment[{c, d, s, a}] = model.NewBoolVar();
                }
            }
        }
    }
}
```

**Impacto**: Menor (ahorra un loop de cursos), pero más limpio.

---

## 8. Reservar capacidad en mapas

**Problema**: `QMap` redimensiona dinámicamente. Si se sabe el tamaño aproximado, reservar.

**Optimizado**:
```cpp
// Estimar tamaño
int estimated = dim.num_cursos * dim.num_materias * dim.num_profesores * dim.num_dias * 6;
vars.assignment.reserve(estimated);  // QMap no tiene reserve, pero QHash sí
```

**Nota**: `QMap` no soporta `reserve()`. Si se cambia a `QHash`, sí. `QHash` tiene O(1) amortizado vs O(log n) de `QMap`.

---

## Resumen de prioridad

| # | Optimización | Impacto | Esfuerzo | Prioridad |
|---|-------------|---------|----------|-----------|
| 1 | QList → QSet slots | Alto | Bajo | **Alta** |
| 3 | Cache disponibilidad | Alto | Bajo | **Alta** |
| 5 | Bitset titularidad | Medio | Bajo | Media |
| 6 | Disponibilidad como QMap | Alto | Alto (parser) | Media |
| 2 | QSet slotsPorTurno | Bajo | Bajo | Baja |
| 7 | Fusionar loops aulas | Bajo | Bajo | Baja |
| 8 | QMap → QHash | Bajo | Medio | Baja |
| 4 | Iterar materias del curso | — | — | ✅ Ya hecho |

**Recomendación**: Implementar #1 y #3 primero — mayor impacto con menor esfuerzo.
