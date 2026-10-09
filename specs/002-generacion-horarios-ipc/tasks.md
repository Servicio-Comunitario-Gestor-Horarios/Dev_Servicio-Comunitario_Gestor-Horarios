# Tareas 002 — Generación de horarios y comunicación con el proceso de cálculo

- Spec: `specs/002-generacion-horarios-ipc/spec.md`
- Plan: `specs/002-generacion-horarios-ipc/plan.md`
- Depende de: `specs/001-base-datos-local` (base de datos y núcleo de datos del cliente)
- Comando de tests: `cmake --preset full && cmake --build build && ctest --preset full`
- Regla: **tests primero** (rojo → código → verde → marcar → parar). Una tarea cada vez.

---

- [ ] **T1. Cargador de configuración del solver y presets (JSON).** RF-1, RF-4
    - Tests (rojo): `test/backend/test_cargador_configuracion.cpp` — lee/escribe config y presets como JSON; archivo inexistente/corrupto → error informado.
    - Código: `CargadorConfiguracionSolver.{hpp,cpp}` (sin base de datos).
    - Hecho cuando: `ctest` pasa; la configuración se guarda y se vuelve a leer como JSON sin tocar la base.

- [ ] **T2. Constructor del JSON de entrada (13 secciones).** RF-1
    - Tests (rojo): `test/backend/test_constructor_entrada.cpp` — JSON con 13 secciones; `dimensiones` derivadas; `meta.fecha_modificacion` desde `ahora` inyectado.
    - Código: `ConstructorEntradaSolver.{hpp,cpp}` + `construirJsonEntrada` (pura).
    - Hecho cuando: `ctest` pasa; con datos de los 8 dominios y una config JSON se genera un JSON estructuralmente completo.

- [ ] **T3. Validación previa V1–V12.** RF-1
    - Tests (rojo): `test/backend/test_validador_entrada.cpp` — sin materias/profesores o curso con 0 estudiantes → error y no envía; JSON válido → pasa.
    - Código: `validarEntradaSolver` sobre `SolverConfig::fromJson`.
    - Hecho cuando: `ctest` pasa; cada regla V1–V12 tiene su caso y el fallo muestra el motivo.

- [ ] **T4. Validador de salida P1–P4.** RF-3
    - Tests (rojo): `test/backend/test_validador_salida.cpp` — P1–P3 generan avisos; P4 (solapamiento) → no presentable.
    - Código: `ValidadorSalidaSolver.{hpp,cpp}` + `analizarSalidaSolver` (pura).
    - Hecho cuando: `ctest` pasa; un resultado con solapamiento se marca no presentable y uno correcto expone solo avisos.

- [ ] **T5. Servicio de generación: envío, timeout e "datos anteriores".** RF-1
    - Tests (rojo): `test/backend/test_servicio_generacion.cpp` — timeout 60 s → abandona e ignora resultado tardío; no-factible; contrato inválido; huella distinta → "datos anteriores".
    - Código: `ServicioGeneracion.{hpp,cpp}` + `huellaDatos`/`estamparFechaGeneracion` (puras).
    - Hecho cuando: `ctest` pasa; cada desenlace (ok/no-factible/inválido/timeout/obsoleto) se comporta como exige RF-1.

- [ ] **T6. Middleware: socket fiable y rutas del solver.** RF-1, RF-2
    - Tests (rojo): `test/test_middleware_transport.cpp` — `solver_resolve`, salud y apagado ordenado; socket reutilizable sin tormenta de reintentos (F-CRASH1).
    - Código: `internalclient.cpp` (estado del socket), `messages.h`, `internalserver.cpp`.
    - Hecho cuando: `ctest` pasa; dos operaciones seguidas no disparan reconexiones espurias.

- [ ] **T7. Modo `--backend`: resolver sin base de datos.** RF-1, RF-2
    - Tests (rojo): `test/test_ipc_aislamiento_bd.cpp` — el proceso de cálculo arranca y resuelve sin crear ni abrir archivo de base; rechaza operaciones de negocio.
    - Código: `aplicacion_backend.cpp` (ruta `solver_resolve`), `gestor_proceso_backend.cpp`.
    - Hecho cuando: `ctest --preset full` (Docker, OR-Tools) pasa; no se abre ningún archivo de base en el proceso de cálculo.

- [ ] **T8. Guardado de horario y preset como archivo.** RF-4
    - Tests (rojo): `test/frontend/test_save_schedule_dialog.cpp` — guarda JSON en el destino elegido; fallo de escritura → aviso + reintento sin perder el contenido.
    - Código: `save_schedule_dialog.{hpp,cpp}`.
    - Hecho cuando: `ctest` pasa; un fallo simulado de disco deja el contenido en pantalla y permite reintentar.

- [ ] **T9. Vista de generación y visualización con avisos.** RF-1, RF-3
    - Tests (rojo): `test/frontend/test_generation_widget.cpp` — estados (validando/calculando/no solución/inválido/timeout/listo); bloquea segunda generación; confirmación "datos anteriores".
    - Código: `generation_widget.*`; `schedule_visualization_widget.*` (rejilla + panel de avisos P1–P3).
    - Hecho cuando: `ctest` pasa; el horario se pinta y un resultado con conflictos no se muestra como válido.

- [ ] **T10. Guardia de cierre por horario/generación.** RF-5
    - Tests (rojo): `test/frontend/test_guardia_cierre_generacion.cpp` — `closeEvent` cancelable con horario sin guardar o generación en curso.
    - Código: guardia de cierre en `main_window.*`; apagado del proceso con plazo 5 s.
    - Hecho cuando: `ctest` pasa; cerrar con trabajo sin guardar pregunta y permite cancelar.

## Cobertura

- RF-1 → T1, T2, T3, T5, T6, T7, T9; RF-2 → T6, T7; RF-3 → T4, T9; RF-4 → T1, T8; RF-5 → T10.
- RNF-1 (mensajes) transversal.
