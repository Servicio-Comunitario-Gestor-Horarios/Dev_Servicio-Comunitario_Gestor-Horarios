# Tareas 001 — Base de datos local: arranque, persistencia de dominios e instancia única

- Spec: `specs/001-base-datos-local/spec.md`
- Plan: `specs/001-base-datos-local/plan.md`
- Interfaz de consumo (otro equipo): `docs/interfaz-frontend.md`
- Comando de tests: `cmake --preset full && cmake --build build && ctest --preset full`
- Regla: **tests primero** (rojo → código → verde → marcar → parar). Una tarea cada vez.
- Entregables: `src/backend`, `src/app` y `src/middleware`. La interfaz de `src/frontend`
  (vistas, listas, diálogos e indicadores) la implementa otro equipo; no se implementa aquí.

---

- [x] **T1. Esquema versionado y decisión de apertura.** RF-1
    - Tests (rojo): `test/backend/test_version_esquema.cpp` — `decidirApertura` en inexistente/ausente/anterior/igual/posterior; la migración fija `user_version`; un fallo deja la versión previa intacta.
    - Código: `version_esquema.{hpp,cpp}`; `migracion.cpp` versionado y transaccional (baseline v1 = esquema actual).
    - Hecho cuando: `ctest` pasa el test y una base v1 migra a v2 sin cambios parciales ante fallo simulado.

- [x] **T2. Respaldo de la base.** RF-1, RF-6
    - Tests (rojo): `test/backend/test_respaldo.cpp` — `construirNombreRespaldo` determinista con `ahora`; `crearRespaldo`/`validarRespaldo`/`restaurarRespaldo`; respaldo imposible se reporta como tal.
    - Código: `respaldo.{hpp,cpp}` (`VACUUM INTO`, `integrity_check` + versión).
    - Hecho cuando: `ctest` pasa; un respaldo creado y restaurado reproduce la base y uno corrupto se rechaza.

- [x] **T3. Apertura con migración y respaldo obligatorio.** RF-1, RF-4
    - Tests (rojo): `test/backend/test_apertura_base_datos.cpp` — inexistente → crea; ausente/posterior → fallo; migración exige respaldo; sin respaldo → fallo; migración fallida → estado conocido.
    - Código: `apertura_base_datos.{hpp,cpp}`; `DatabaseManager` delega versión/migración.
    - Hecho cuando: `ctest` pasa y ninguna rama aplica cambios parciales.

- [x] **T4. Núcleo de datos: CRUD de dominios.** RF-2
    - Tests (rojo): `test/backend/test_nucleo_datos.cpp` — alta/modificación/baja persisten y se recuperan tras reapertura; CRUD de Cursos y Turnos/Recesos.
    - Código: `NucleoDatos.{hpp,cpp}`; `ServicioCursos`, `ServicioTurnosRecesos` nuevos; migración v2 con las tablas de dominio.
    - Hecho cuando: `ctest` pasa; los datos de todos los dominios se recuperan tras cerrar y reabrir la base.

- [x] **T5. Eliminación en cascada atómica.** RF-2
    - Tests (rojo): `test/backend/test_cascada.cpp` — `dependenciasDe` lista dependientes; borrado con cascada es atómico (fallo → nada aplicado); cancelación no toca datos.
    - Código: consulta de dependientes en `ServicioMaterias/Profesor/PlanesEstudio/Aula` + `eliminarConCascada` en `NucleoDatos`.
    - Hecho cuando: `ctest` pasa; un fallo a mitad de cascada no deja ninguna parte aplicada.

- [x] **T6. Cambios pendientes y reintento.** RF-3, RNF-3, RNF-4
    - Tests (rojo): `test/backend/test_gestor_pendientes.cpp` — estado guardado/pendiente-guardar/pendiente-eliminar; reintento con éxito y sin éxito; sin perder de pantalla.
    - Código: `GestorPendientes.{hpp,cpp}` + `estadoPendienteDe` (pura); consulta de pendientes que consumirá la guardia de cierre.
    - Hecho cuando: `ctest` pasa; tras fallo de escritura el registro queda pendiente y el reintento lo guarda.

- [ ] **T7. Instancia única y arranque del cliente.** RF-5, RF-2
    - Tests (rojo): `test/test_instancia_unica.cpp` — segundo arranque enfoca al primero; sin acuse en 10 s → avisa y no arranca; detección activa durante la migración.
    - Código: `instancia_unica.{hpp,cpp}` + enganche en `main.cpp` y `aplicacion_frontend.cpp` (instancia única → apertura con `AperturaBaseDatos` → construcción de `NucleoDatos`; la interfaz la toma el frontend).
    - Hecho cuando: `ctest` pasa; dos arranques simultáneos resultan en una sola instancia y el arranque deja la base abierta y el núcleo de datos disponible para el frontend.

- [ ] **T8. Documentar la interfaz expuesta al frontend (base de datos y núcleo de datos).** RF-2, RF-3, RF-4
    - Tests (rojo): `test/backend/test_contrato_interfaz.cpp` — la semántica que consume el frontend: `AperturaBaseDatos::Resultado::ok()`/`estado`/`rutaRespaldo`, `Resultado<T>::exito/error` con código y `estadoPendienteDe`.
    - Código: ninguno de producto; se documenta `docs/interfaz-frontend.md` (sección «Base de datos local»: `VersionEsquema`, `Respaldo`, `AperturaBaseDatos`, `NucleoDatos`, dominios y estado por registro).
    - Hecho cuando: `ctest` pasa el test de contrato y `docs/interfaz-frontend.md` describe, para cada elemento expuesto, qué expone, su firma/contrato, sus estados/errores y cómo debe tratarlo el frontend.

## Cobertura

- RF-1 → T1, T2, T3; RF-2 → T4, T5, T7; RF-3 → T6; RF-4 → T3 (apertura/respaldo) + contrato de T8;
  RF-5 → T7; RF-6 → T2, T6.
- RNF-1 (mensajes) transversal; RNF-2 → T3; RNF-3/RNF-4 → T6; RNF-5 → T1, T3.
- Los diálogos y la guardia de cierre (parte de interfaz de RF-4/RF-6) los implementa y testea el
  equipo de frontend sobre el contrato documentado en T8.
