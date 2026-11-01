# Migración del frontend tras las specs 009–012

> **Para:** equipo de `src/frontend`.
> **Fecha:** 2026-10-29 · **Rama:** `feat/Base-datos` · **Suite del backend:** 54/54.
> **Contrato vigente:** `2.0` (`docs/contrato-solver-2.0.md`). **Esquema de base:** `v5`.

Este documento dice **qué hay que cambiar en la interfaz para que vuelva a compilar** y
**qué cambió en el backend** en las specs 009, 010, 011 y 012 más las correcciones del
2026-10-29. El contrato detallado sigue en `docs/interfaz-frontend.md` y
`docs/contrato-solver-2.0.md`; aquí está solo la guía de migración.

---

## 1. Qué cambió y por qué (resumen)

| Spec | Qué aportó | Impacto en la interfaz |
|---|---|---|
| **009** (2026-10-24) | Catálogo de **tipos de aula**, **eventos** del turno (sustituyen a los recesos), esquema v3, servicios y huella | `TurnoDTO.recesos` → **`TurnoDTO.eventos`**; aulas y materias con tipo |
| **010** (2026-10-27) | **Contrato 2.0**, regla de **tipo de aula**, **aula de referencia** con desplazamiento y vuelta, salida con el **aula real**, V16–V18 | Presets 1.0 se rechazan; la salida trae el **aula real por clase** (índice); el aula del curso debe ser del **tipo general** |
| **011** (2026-10-29) | **Duración global** de la hora académica (35 min por defecto, 30–90), **franja = D**, constructor y huella **con D**, esquema v4 | `construirJsonEntrada` **exige D**; el preset ya no manda la duración; «datos anteriores» depende de D |
| **012** (2026-10-29) | **Preferencias horarias** del docente (persistencia y API; **sin UI**) | Solo aditivo: `ProfesorDTO.preferencias` y métodos nuevos en `NucleoDatos`; esquema v5 |
| **Correcciones** (2026-10-29) | Avisos P1/P3 retirados, `metadata` real, reinicio del solver, test del middleware | `AnalisisSalida.avisos` ya solo trae **P2**; `metadata` con cifras reales |

---

## 2. Lo que rompe la compilación

**Solo en el preset `full`** (backend + interfaz). Con `dev-frontend` (`BUILD_BACKEND=OFF`) el código de
generación está compilado fuera por `GESTOR_TIENE_BACKEND` y **la interfaz compila**, mostrando el aviso
de «sin backend». Para reproducir los fallos hay que compilar con backend:

```bash
cmake --preset full && cmake --build build    # requiere OR-Tools
```

### 2.1 `construirJsonEntrada` exige la duración efectiva (spec 011)

`src/frontend/src/services/entrada_solver_builder.cpp:165`

```cpp
// ANTES
comp.entrada = construirJsonEntrada(datos, preset, QDateTime::currentDateTime());

// DESPUÉS
const int D = m_nucleo->configuracionHoraria().valor.duracionMinutos;  // 35 por defecto
comp.entrada = construirJsonEntrada(datos, preset, D, QDateTime::currentDateTime());
```

- Firma nueva: `construirJsonEntrada(const DatosDominio&, const SolverConfig&, int duracionMinutos, const QDateTime&)`.
  Sin sobrecarga antigua.
- La fachada equivalente, `ConstructorEntradaSolver::construir`, recibe también `duracionMinutos`.
- **El valor del preset se ignora**: la construcción **sobrescribe siempre** `franja_horaria.duracion_minutos`
  con D antes de validar. Un preset con 0, 5 o 45 no bloquea la construcción.
- Si D está fuera de **30–90**, la construcción devuelve `Resultado` con error y **no emite JSON**.

### 2.2 `TurnoDTO.recesos` ya no existe (spec 009)

`src/frontend/src/views/schedule_generation_widget.cpp:158`

```cpp
// ANTES
int recesoMin = 0;
for (const RecesoDTO& receso : turno.recesos)
    recesoMin += qMax(0, receso.duracion);
const int spanMin = turno.inicio.secsTo(turno.fin) / 60;
const int duracion = (spanMin - recesoMin) / turno.numSlots;

// DESPUÉS (y mejor: la duración ya NO se deriva del turno)
// 1) la duración vigente es la configuración global:
const int duracion = m_nucleo->configuracionHoraria().valor.duracionMinutos;
// 2) si necesitas el tiempo de eventos para algo visual:
for (const EventoDTO& evento : turno.eventos)
    eventosMin += qMax(0, evento.duracion);
```

`TurnoDTO` ahora es `{ nombre, inicio, fin, numSlots, QVector<EventoDTO> eventos }`, con
`EventoDTO { id, nombre, despuesDeSlot, orden, duracion, inicio, fin }`.

**No derive la duración de la franja del turno**: desde la 011 la franja **dura D** y el sobrante de la
ventana queda al final. Derivarla del turno da otro número.

### 2.3 Qué **no** rompe

- `NucleoDatos` y los servicios solo **añaden** métodos (configuración horaria y preferencias):
  lo existente sigue compilando.
- `ProfesorDTO` gana `preferencias` **al final**: los agregados existentes no cambian de orden.
- `AulaDTO.tipoAula`/`idTipoAula` y `MateriaDTO` idem ya existían desde la 009.
- `CursoDTO` sigue con `anio`, `seccion`, `turno`, `aula` (el nombre se deriva en la vista).

---

## 3. Cambios de comportamiento que se ven en pantalla

1. **La duración es global.** Se consulta con `NucleoDatos::configuracionHoraria()` →
   `{ duracionMinutos, porDefecto }` y se edita con `guardarConfiguracionHoraria(int)`
   (rango **30–90**). El **selector de duración** del editor de presets y de la vista de generación
   **deja de ofrecerse**.
2. **Bases anteriores no abren.** El esquema pasó a **v5**; una base v4 o anterior se rechaza al abrir
   (política sin migración, spec 009). El arranque ya tiene el diálogo de recuperación
   (reintentar / restaurar respaldo / crear base nueva): no hay que programar nada, pero hay que
   **crear base nueva con respaldo** para probar.
3. **Avisos de la validación posterior.** `AnalisisSalida.avisos` trae **solo P2** (exceso de horas del
   docente). Los avisos de **P1 (horas no cubiertas)** y **P3 (capacidad de aula)** están **retirados**
   por la 005 (decisión I2). Si la vista los esperaba, hay que dejar de pintarlos.
4. **`metadata` con cifras reales**: `total_asignaciones` cuenta **clases** (no variables),
   `profesores_asignados` cuenta docentes distintos con al menos una clase y `tiempo_ejecucion_ms`
   trae el tiempo del motor.
5. **Salida con el aula real** por clase (`asignacion.aula` = índice de aula), también en modo de aula
   fija (spec 010). El aula de referencia **no** es el aula de todas las clases.
6. **Presets 1.0 no se cargan**: hay que regenerarlos como 2.0 desde los datos.
7. **«Datos anteriores».** La huella incluye D desde la 011. La vista construye el servicio con los
   callbacks vacíos (`schedule_generation_widget.cpp:111`) y llama a `generar(..., QString())`
   (`:769`), así que el mecanismo **no se ve**: hay que cablearlo con `huellaDatos(datos, D)`.

---

## 4. Lista de cambios a hacer

- [ ] **Builder con D** (§2.1): leer la duración efectiva de `NucleoDatos::configuracionHoraria()` y
      pasarla como tercer argumento. Dejar de leer `opciones.duracionMinutos` y
      `preset.franja_horaria.duracion_minutos` para el cálculo.
- [ ] **Recesos → eventos** (§2.2): sustituir `RecesoDTO`/`turno.recesos`; dejar de derivar la duración
      de la franja del turno.
- [ ] **Cablear `huellaActual`** con D para que «datos anteriores» funcione.
- [ ] **Retirar el selector de duración**; si se quiere ofrecer, que edite la configuración global
      (`consultar`/`guardar`), no el preset.
- [ ] **Panel de avisos**: dejar solo P2.
- [ ] **`test/frontend/test_generacion_seed.cpp`**: fija `duracion_minutos == 40`; al migrar el builder,
      reescribirlo a **35** y a la rejilla nueva del seed.
- [ ] **Opcional**: mostrar `ProfesorDTO.preferencias` (API nueva de la 012, sin UI todavía).

---

## 5. Cómo verificar

```bash
# interfaz sola (sin backend): compila y la vista de generación muestra el aviso
cmake --preset dev-frontend && cmake --build build-fe

# interfaz + backend (es la que hoy falla): requiere OR-Tools
cmake --preset full && cmake --build build

# backend
cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend   # 54/54
```

Datos de prueba del seed vigente: turnos **Mañana 07:00–11:50** y **Tarde 12:40–17:30**, 8 franjas de
**35 min**, eventos «Receso» en **08:10–08:20** y **13:50–14:00** (`8 × 35 + 10 = 290`). La rejilla de
clases va de 07:00 a **11:40** y de 12:40 a **17:20**; el `fin` del turno añade los 10 min del evento.

---

## 6. Resumen de lo implementado (para contexto)

- **009 · Tipos de aula y eventos** (2026-10-24): catálogo `TiposAula` con clave normalizada, `Eventos`
  con identidad y orden, esquema v3, servicios, cascada, huella y guía.
- **010 · Tipos y eventos en el solver** (2026-10-27): contrato **2.0**, regla de **tipo exacto**,
  **aula de referencia** con desplazamiento y vuelta (y reserva), salida con **aula real**,
  **V16–V18**, diagnóstico de causas por **conteo de capacidad** y «no atribuible».
- **011 · Duración de la hora académica** (2026-10-29): configuración global (35, 30–90), punto único
  `duracion_horaria`, franja = D con encaje del turno, constructor y huella con D, esquema v4.
- **012 · Preferencias del docente** (2026-10-29): modelo que replica la disponibilidad con **sentido**,
  validación de solapes opuestos, servicios y `ProfesorDTO.preferencias`, cascada y seed sin
  preferencias; esquema **v5**.
- **Correcciones** (2026-10-29): avisos P1/P3 retirados, `metadata` real, el solver reinicia su estado
  en cada resolución y el test del middleware dejó de ser intermitente.

---

## 7. Deuda que sigue siendo del equipo de frontend

Documentada también en `docs/interfaz-frontend.md` §5 y §6:

- Migración del builder y de la vista de generación (§2 y §4 de este documento).
- `horasAula` y la aritmética en minutos de la vista: la conversión de presentación es
  `duracion_horaria::minutosDeHorasAcademicas(horas, D)`.
- `test_generacion_seed` con la rejilla de 35.
- UI de preferencias (la 012 solo entrega servicios y API).
