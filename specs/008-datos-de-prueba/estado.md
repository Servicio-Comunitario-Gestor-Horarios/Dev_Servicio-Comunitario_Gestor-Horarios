# Estado 008

Fase: cerrada · **superada en datos por la 017 (2026-11-08)**
Aprobaciones: spec 2026-10-17 (rectificada) · plan/tareas 2026-10-17 · implementación 2026-10-17
Base: f94acba455a04ae80364c82e1f07ae16d9f156ec
Dudas abiertas: ninguna
Veredicto: APROBADO (2026-10-17)
Siguiente paso: **nada pendiente en esta spec**; su **conjunto de datos** lo sustituye la **017** (ver
«Superación»). Los pendientes menores de abajo se abordan en una spec futura o quedaron resueltos en la
005-T10.

## Implementado

- **Seed `gestor-horarios-seed` + biblioteca `seed`:** ejecutable de desarrollo y API reutilizable por
  los tests; solo puebla la base (no genera presets).
- **Plan General 31059 adaptado** a 40/40/40/40/40 horas académicas (1 hora = 1 bloque) y 3 planes
  derivados (EXA/NAT/SOC) con 2 materias propias cada uno, todos los años en 40; 57 materias, 210 filas
  plan-materia; 13 cursos sin turno, 16 aulas, 2 turnos con receso tras la franja 2.
- **4 variantes de docentes:** holgada (65; tope 30), suplentes (43; tope = ceil(carga/1,15), Σ topes
  461 = 88,65 % de la demanda 520), sin profesorado (64) y sin disponibilidad (65).
- **Seguridad:** falla si la base tiene datos; reemplazo total o solo docentes con confirmación y
  respaldo obligatorio; transacción única y rollback. Las **210 cargas** se insertan por **SQL directo**
  (no `asignarCarga`, que abre su propia transacción).
- **CLI** con `--db`, `--variante`, `--reemplazar`, `--solo-docentes`, `--si`, `--ayuda`, `--version` y
  códigos 0/1/2/3/4/5/6/7/8; **tarjeta de recuentos** (entidades, materias por año, totales por plan y
  ruta del respaldo).
- Suite **46/46** en verde en el contenedor, con `test_seed_perfil` y `test_seed_aplicacion`.

## Pendientes menores

- **Convención de `despues_de_slot`:** se documenta como **asunción 1-based** (valor 2 = receso tras la
  franja 2, 08:20/14:00). El esquema admite 0 y no la fija; si se confirma 0-based, usar 1.
- **005-T10 (cerrada el 2026-10-29):** resueltos allí la cobertura de tests por regla, RF-13 y los
  residuales del solver.

## Superación (2026-11-08)

La **017-datos-reales-del-centro** sustituye el **conjunto de datos** de esta spec por el **centro real**
(Liceo Nacional Robert Serra, 2026/2027). Lo que cambia:

| 008 | 017 (centro real) |
|---|---|
| 4 planes (General + EXA/NAT/SOC) | **2 planes** (31059 Media General y 31060 Ciencias y Tecnología) |
| Plan General adaptado a 40 h/año | Planes reales: 31059 **34/34/34/36/34** y 31060 **40×5** |
| 57 materias, 210 cargas | **85 materias** (79 con carga + 6 sin carga), **95 cargas** |
| 13 cursos **sin turno** | **24 cursos con turno** (12 mañana + 12 tarde), con matrícula (**889**) |
| 16 aulas, «un aula propia y distinta por curso» | **13 aulas** (12 generales de 44 **compartidas entre turnos** + 1 cancha de 88) |
| 4 variantes de docentes y su conmutación | **Una sola plantilla**: **32 docentes**, tope **36 h**, **272** asignaciones |
| Turnos de 35 min (011), mañana 07:00–11:50 | **40 min**, mañana **07:00–12:40** y tarde **12:40–18:10** |
| — | **No genera** hasta el **aforo simultáneo** de la cancha (**48** clases de EF por turno vs **40** franjas) |

**Sigue vigente** de la 008: la **seguridad** de la aplicación (base de destino obligatoria, fallo con
datos, confirmación explícita), el **respaldo previo obligatorio**, el **reemplazo total atómico** en una
**transacción única**, el **determinismo** y los recuentos leídos de la base tras el commit. La nota
completa está también al principio de `spec.md`.

## Historial

- **2026-10-17 — cerrada (APROBADO).** Conjunto sintético del 31059 adaptado, 4 planes, 4 variantes y
  46/46 en el contenedor.
- **2026-11-08 — superada en datos por la 017.** Se añade la nota de sustitución al principio de `spec.md`
  y esta sección; no se reescribe nada de lo histórico. El conjunto nuevo está en
  `specs/017-datos-reales-del-centro/` (implementación en curso: T1–T5 y T8 de 10).
