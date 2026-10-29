# Estado 012

Fase: cerrada
Aprobaciones: spec 2026-10-23 · plan/tareas 2026-10-29 · implementación 2026-10-29 · validación 2026-10-29
Base: 3007c54 (base de la implementación; último commit de código: `94647ef`; notas de la validación en `e12641b`; suite 54/54; el cierre de la implementación son estos documentos). Las dependencias **009**, **010** y **011** están **cerradas e implementadas**; el esquema vigente es **v5**.
Dudas abiertas: ninguna
Siguiente paso: nada pendiente en esta spec. Fuera de ella: los pendientes de la **005-T10**, las **deudas del frontend** de la 011 y de la 012 (`docs/interfaz-frontend.md` §6), la divergencia con `release/1.0` y el PR de `feat/Base-datos` a `develop`.
Veredicto: APROBADO (2026-10-29) contra `Base: 3007c54`; las seis notas menores (M1–M6) están corregidas en `e12641b`.
QA: spec: CAMBIOS NECESARIOS resueltos el 2026-10-23 (H1–H5); spec aprobada el 2026-10-23 · QA del plan: CAMBIOS NECESARIOS (B1, I1, I2, M1–M9) y re-QA APROBADO el 2026-10-29.

## Decisiones cerradas (2026-10-23)

- **Modelo replicado.** Cada preferencia tiene docente, día, hora de inicio, hora de fin y **sentido**
  (preferida o evitada), con identidad propia. Se replica el modelo de disponibilidad, sin mezclarse
  con él.
- **Solo persistir y exponer.** No hay solver, ni contrato, ni efecto en la generación en esta spec.
- **Día.** **0 = domingo … 6 = sábado**, el mismo rango de la disponibilidad. Se admiten días fuera de
  lunes–viernes: se guardan y se exponen, pero no intervienen en la generación actual.
- **Validación.** Docente existente; día 0–6; inicio anterior al fin; sentido válido; sin solapes de
  **sentidos opuestos** en el mismo docente y día; sin duplicados exactos.
- **Solapes del mismo sentido.** Se permiten (no se contradicen y no se fusionan); la fusión, si hace
  falta, es de quien presenta.
- **Duplicado exacto.** Mismo docente, día, hora de inicio, hora de fin y sentido; no implica
  exclusividad sobre otros rangos.
- **Cambio de sentido.** Se revalida contra los solapes opuestos; si aparece una contradicción, se
  rechaza y se conserva el sentido anterior.
- **Cascada.** La baja del docente borra preferencias y disponibilidad en una operación atómica y las
  muestra como dependencias antes de confirmar.
- **Conmutación de variante.** Se borran las preferencias antes que los docentes, junto con
  disponibilidad y materias; **supersede RF-9 y la definición de reemplazo de docentes de la 008** en lo
  que toca a las preferencias. Si falla a mitad, no se borra nada.
- **Huella de datos.** Las preferencias **no** entran en la huella mientras el solver no las consuma,
  porque no intervienen en la generación; se deja anotado que deberán incluirse cuando una spec del
  solver las use.
- **Seed.** No se siembran preferencias (conjunto vacío, recuento 0); no hay una política decidida y el
  conjunto se sustituirá por los datos reales. El vaciado las borra antes que a los docentes.
- **Esquema.** Registro nuevo; incremento de la marca de versión, catálogo, huella de esquema y política
  sin migración de la 009.
- **Frontend fuera de alcance.** Se entregan servicios y guía de API.

## Hallazgos del QA resueltos

- **H1:** la conmutación de variante borra preferencias antes que docentes y supersede la 008; no falla
  por claves foráneas.
- **H2:** el rango de día queda fijado en la spec (0–6 con significado) y se declara que los días fuera
  de lunes–viernes se guardan sin efecto en la generación.
- **H3:** desempate de orden por hora de fin y, si persiste, por identidad.
- **H4:** casos límite añadidos: actualizar/eliminar preferencia inexistente, docente sin preferencias
  y día válido fuera de la semana de generación.
- **H5:** el duplicado se define como **exacto**, sin sugerir exclusividad; los solapes del mismo
  sentido se admiten.

## Implementación (2026-10-29)

Nueve tareas, una por commit, tests en rojo antes del código. Esquema **v5**; suite **54/54**. **No**
se tocó el solver, el contrato ni la interfaz: solo persistencia, validación, servicios, cascada y seed.

| Tarea | Commit | Qué dejó |
|---|---|---|
| T1. Modelo puro y validación compartida | `4993be0` | `SentidoPreferencia`, modelo puro de preferencias y `franja_horaria::errorDeFranja` compartido con la disponibilidad |
| T2. Esquema v5 | `3871488` | `Preferencia_Profesor` (día TEXT 0–6, horas TEXT, `sentido` preferida/evitada, `CHECK` inicio<fin, `UNIQUE` del duplicado exacto), `VERSION_ESQUEMA_ACTUAL = 5` y catálogo/huellas de esquema |
| T3. Servicio de preferencias | `7405351` | `ServicioPreferencias`: `agregar`/`listar`/`actualizar`/`eliminar`/`limpiar`; solapes de sentidos opuestos rechazados, mismo sentido permitido, cambio de día revalidado |
| T4. DTO y fachada | `d2f2147` | `ProfesorDTO::preferencias` (aditivo) y cinco operaciones de preferencias en `NucleoDatos` |
| T5. Cascada y baja del docente | `8acce25` | `eliminarProfesor` y `eliminarConCascada` borran preferencias y disponibilidad en una transacción; `dependientesDe("profesor")` las lista con etiqueta |
| T6. Recuento del seed | `3b8ba90` | `Recuentos.preferencias = 0`, `Preferencia_Profesor` en `kTablasDominio` (12 tablas) y `Preferencias: 0` en el resumen |
| T7. Borrado ordenado y atomicidad | `43c222c` | `--solo-docentes` y reemplazo total borran preferencias antes que docentes y disponibilidad; rollback y determinismo cubiertos |
| T8. Huella intacta | `94647ef` | `ServicioGeneracion` sin «preferencia»: cambiar preferencias no marca «datos anteriores» |
| T9. Documentación y regresión | `98c9dbc` | `docs/interfaz-frontend.md` (§2.2 a v5, dominios, `NucleoDatos`, DTO, fila `profesor` con preferencias, §2.4.5, tabla §4), `docs/guias/datos-prueba.md` (fila 0 y ejemplo con `Hora académica` y `Preferencias: 0`) y notas de supersesión en la 008; `ctest --preset backend` **54/54** |

## Cierre (2026-10-29)

**VEREDICTO: APROBADO (2026-10-29)** contra `Base: 3007c54`, con las seis notas menores (M1–M6)
aplicadas y verificadas en `e12641b`. Suite **54/54** (`ctest --preset backend`).

| Tarea | Commit |
|---|---|
| T1. Modelo puro y validación compartida | `4993be0` |
| T2. Esquema v5 | `3871488` |
| T3. Servicio de preferencias | `7405351` |
| T4. DTO y fachada | `d2f2147` |
| T5. Cascada y baja del docente | `8acce25` |
| T6. Recuento del seed | `3b8ba90` |
| T7. Borrado ordenado y atomicidad | `43c222c` |
| T8. Huella intacta | `94647ef` |
| T9. Documentación y regresión | `98c9dbc` |
| Notas de la validación (M1–M6) | `e12641b` |

- **No se tocó** el solver, el contrato de configuración, la generación ni la interfaz: la spec
  entrega persistencia, validación, servicios, cascada, seed y documentación.
- **Deudas declaradas:**
  - Las preferencias **no tienen efecto en la generación**; cuando una spec del solver las consuma,
    deberán entrar en `DatosDominio` y en `huellaDatos`.
  - El `CHECK` de horas asume el formato `HH:mm`; es la misma dependencia que ya tenían
    disponibilidad y turnos.
  - La interfaz de usuario sigue fuera de alcance (`docs/interfaz-frontend.md` §2.4.5 y §6).

## Notas

- **Orden de implementación.** Después de la 011 para no pisar el incremento de versión de esquema; en
  contenido solo depende de la 009 y podría adelantarse, pero no aporta ventaja.
- **Preferencias futuras.** El modelo no incluye recurrencias, excepciones por fecha ni pesos; si el
  solver las necesita, será otra spec.
- **Plan y tareas (2026-10-29).** `plan.md` fija la base `3007c54` y `tasks.md` la lista T1–T9 con su
  trazabilidad; ambos aprobados tras el QA. Corregida en §8.1 la redacción del ejemplo de salida del
  seed: `Preferencias: 0` va en **línea propia**, después de `Cursos…` (así lo imprime `resumenTexto`).
- **Cierre documental (2026-10-29).** Veredicto APROBADO; `spec.md` pasa a `implementada (2026-10-29)` y
  este `estado.md` queda cerrado. El contenido completo de `MEMORY.md` lo entrega el **planner** y lo
  aplica el **coordinator** en la raíz, porque el planner no escribe fuera de `specs/`.

## Tamaño previsto

Ocho RF agrupables en **≤10 tareas**: modelo, validación, servicios, cascada, huella, esquema/versión,
seed/conmutación y guía de API.
