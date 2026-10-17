# Estado 008

Fase: cerrada
Aprobaciones: spec 2026-10-17 (rectificada) · plan/tareas 2026-10-17 · implementación 2026-10-17
Base: f94acba455a04ae80364c82e1f07ae16d9f156ec
Dudas abiertas: ninguna
Veredicto: APROBADO (2026-10-17)
Siguiente paso: nada pendiente en esta spec. Los pendientes menores (ver abajo) se abordan en una spec futura o en la 005-T10; el usuario decide si se abre PR.

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

- **`.clangd` sin ignorar** en `.gitignore`.
- **Convención de `despues_de_slot`:** se documenta como **asunción 1-based** (valor 2 = receso tras la
  franja 2, 08:20/14:00). El esquema admite 0 y no la fija; si se confirma 0-based, usar 1.
- **005-T10** sigue abierta con sus pendientes conocidos (cobertura de tests por regla, RF-13 y
  residuales del solver).
