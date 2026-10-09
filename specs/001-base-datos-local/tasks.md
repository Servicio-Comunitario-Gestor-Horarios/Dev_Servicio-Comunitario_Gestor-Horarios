# Tareas 001 — Base de datos local: arranque, persistencia de dominios e instancia única

- Spec: `specs/001-base-datos-local/spec.md`
- Plan: `specs/001-base-datos-local/plan.md`
- Comando de tests: `cmake --preset full && cmake --build build && ctest --preset full`
- Regla: **tests primero** (rojo → código → verde → marcar → parar). Una tarea cada vez.

---

- [x] **T1. Esquema versionado y decisión de apertura.** RF-1
    - Tests (rojo): `test/backend/test_version_esquema.cpp` — `decidirApertura` en inexistente/ausente/anterior/igual/posterior; la migración fija `user_version`; un fallo deja la versión previa intacta.
    - Código: `version_esquema.{hpp,cpp}`; `migracion.cpp` versionado y transaccional (baseline v1 = esquema actual).
    - Hecho cuando: `ctest` pasa el test y una base v1 migra a v2 sin cambios parciales ante fallo simulado.

- [ ] **T2. Respaldo de la base.** RF-1, RF-6
    - Tests (rojo): `test/backend/test_respaldo.cpp` — `construirNombreRespaldo` determinista con `ahora`; `crearRespaldo`/`validarRespaldo`/`restaurarRespaldo`; respaldo imposible se reporta como tal.
    - Código: `respaldo.{hpp,cpp}` (`VACUUM INTO`, `integrity_check` + versión).
    - Hecho cuando: `ctest` pasa; un respaldo creado y restaurado reproduce la base y uno corrupto se rechaza.

- [ ] **T3. Apertura con migración y respaldo obligatorio.** RF-1, RF-4
    - Tests (rojo): `test/backend/test_apertura_base_datos.cpp` — inexistente → crea; ausente/posterior → fallo; migración exige respaldo; sin respaldo → fallo; migración fallida → estado conocido.
    - Código: `apertura_base_datos.{hpp,cpp}`; `DatabaseManager` delega versión/migración.
    - Hecho cuando: `ctest` pasa y ninguna rama aplica cambios parciales.

- [ ] **T4. Diálogo de fallo y flujo de recuperación.** RF-4, RF-6
    - Tests (rojo): `test/frontend/test_database_failure_dialog.cpp` — 4 opciones; "Crear base nueva" deshabilitada sin respaldo; reapertura del diálogo tras fallo.
    - Código: `database_failure_dialog.{hpp,cpp}` + enlace con `apertura_base_datos` (Reintentar/Restaurar/Crear/Salir).
    - Hecho cuando: `ctest` pasa; con base corrupta el diálogo ofrece las 4 rutas y "Crear base nueva" exige respaldo.

- [ ] **T5. Núcleo de datos: CRUD de dominios.** RF-2
    - Tests (rojo): `test/backend/test_nucleo_datos.cpp` — alta/modificación/baja persisten y se recuperan tras reapertura; CRUD de Cursos y Turnos/Recesos.
    - Código: `NucleoDatos.{hpp,cpp}`; `ServicioCursos`, `ServicioTurnosRecesos` nuevos; migración v2 con las tablas de dominio.
    - Hecho cuando: `ctest` pasa; los datos de todos los dominios se recuperan tras cerrar y reabrir la base.

- [ ] **T6. Eliminación en cascada atómica.** RF-2
    - Tests (rojo): `test/backend/test_cascada.cpp` — `dependenciasDe` lista dependientes; borrado con cascada es atómico (fallo → nada aplicado); cancelación no toca datos.
    - Código: consulta de dependientes en `ServicioMaterias/Profesor/PlanesEstudio/Aula` + `eliminarConCascada` en `NucleoDatos`.
    - Hecho cuando: `ctest` pasa; un fallo a mitad de cascada no deja ninguna parte aplicada.

- [ ] **T7. Cambios pendientes y reintento.** RF-3, RNF-3, RNF-4
    - Tests (rojo): `test/backend/test_gestor_pendientes.cpp` — estado guardado/pendiente-guardar/pendiente-eliminar; reintento con éxito y sin éxito; sin perder de pantalla.
    - Código: `GestorPendientes.{hpp,cpp}` + `estadoPendienteDe` (pura).
    - Hecho cuando: `ctest` pasa; tras fallo de escritura el registro queda pendiente y el reintento lo guarda.

- [ ] **T8. Instancia única.** RF-5
    - Tests (rojo): `test/test_instancia_unica.cpp` — segundo arranque enfoca al primero; sin acuse en 10 s → avisa y no arranca; detección activa durante migración/diálogo.
    - Código: `instancia_unica.{hpp,cpp}` + enganche en `main.cpp` y `aplicacion_frontend.cpp`.
    - Hecho cuando: `ctest` pasa; dos arranques simultáneos resultan en una sola instancia operativa.

- [ ] **T9. Arranque cliente y listas desde la base.** RF-2, RF-5
    - Tests (rojo): `test/frontend/test_listas_desde_bd.cpp` — las listas se pueblan desde la base al abrir y reflejan el estado guardado/pendiente.
    - Código: `aplicacion_frontend.cpp` (instancia única → apertura → `NucleoDatos` → `MainWindow`); `main_window.*` y `*_list_widget.*` contra `NucleoDatos`.
    - Hecho cuando: `ctest` pasa y al arrancar las vistas muestran datos persistidos (no de prueba).

- [ ] **T10. Guardia de cierre por cambios pendientes.** RF-6
    - Tests (rojo): `test/frontend/test_guardia_cierre.cpp` — `closeEvent` cancelable con cambios pendientes; sin pendientes cierra normal.
    - Código: guardia de cierre en `MainWindow`/ventana principal.
    - Hecho cuando: `ctest` pasa; cerrar con pendientes pregunta y permite cancelar.

## Cobertura

- RF-1 → T1, T2, T3; RF-2 → T5, T6, T9; RF-3 → T7; RF-4 → T4; RF-5 → T8, T9; RF-6 → T2, T4, T10.
- RNF-1 (mensajes) transversal; RNF-2 → T3; RNF-3/RNF-4 → T7, T9; RNF-5 → T1, T3.
