# Tareas 002 — Generación de horarios y comunicación con el proceso de cálculo

- Spec: `specs/002-generacion-horarios-ipc/spec.md`
- Plan: `specs/002-generacion-horarios-ipc/plan.md`
- Depende de: `specs/001-base-datos-local` (base de datos y núcleo de datos del cliente)
- Interfaz de consumo (otro equipo): `docs/interfaz-frontend.md`
- Comando de tests: `cmake --preset full && cmake --build build && ctest --preset full`
- Regla: **tests primero** (rojo → código → verde → marcar → parar). Una tarea cada vez.
- Entregables: `src/backend`, `src/app` y `src/middleware`. La interfaz de `src/frontend`
  (vista de generación, visualización y diálogos) la implementa otro equipo; no se implementa aquí.

---

- [x] **T1. Configuración del solver y presets como archivo JSON (backend).** RF-1, RF-4
    - Tests (rojo): `test/backend/test_cargador_configuracion.cpp` — lee/escribe config y presets como JSON; archivo inexistente/corrupto → error informado.
    - Código: `CargadorConfiguracionSolver.{hpp,cpp}` (sin base de datos); lectura/escritura de presets.
    - Hecho cuando: `ctest` pasa; la configuración y un preset se guardan y se vuelven a leer como JSON sin tocar la base.

- [x] **T2. Constructor del JSON de entrada (13 secciones).** RF-1
    - Tests (rojo): `test/backend/test_constructor_entrada.cpp` — JSON con 13 secciones; `dimensiones` derivadas; `meta.fecha_modificacion` desde `ahora` inyectado.
    - Código: `ConstructorEntradaSolver.{hpp,cpp}` + `construirJsonEntrada` (pura).
    - Hecho cuando: `ctest` pasa; con datos de los 8 dominios y una config JSON se genera un JSON estructuralmente completo.

- [x] **T3. Validación previa V1–V12.** RF-1
    - Tests (rojo): `test/backend/test_validador_entrada.cpp` — sin materias/profesores o curso con 0 estudiantes → error y no envía; JSON válido → pasa.
    - Código: `validarEntradaSolver` sobre `SolverConfig::fromJson`.
    - Hecho cuando: `ctest` pasa; cada regla V1–V12 tiene su caso y el fallo muestra el motivo.

- [x] **T4. Validador de salida P1–P4.** RF-3
    - Tests (rojo): `test/backend/test_validador_salida.cpp` — P1–P3 generan avisos; P4 (solapamiento) → no presentable.
    - Código: `ValidadorSalidaSolver.{hpp,cpp}` + `analizarSalidaSolver` (pura); devuelve el `AnalisisSalida` que consumirá el frontend.
    - Hecho cuando: `ctest` pasa; un resultado con solapamiento se marca no presentable y uno correcto expone solo avisos.

- [x] **T5. Servicio de generación: envío, timeout e "datos anteriores".** RF-1, RF-5
    - Tests (rojo): `test/backend/test_servicio_generacion.cpp` — timeout 60 s → abandona e ignora resultado tardío; no-factible; contrato inválido; huella distinta → "datos anteriores"; expone el estado de "generación en curso".
    - Código: `ServicioGeneracion.{hpp,cpp}` + `huellaDatos`/`estamparFechaGeneracion` (puras).
    - Hecho cuando: `ctest` pasa; cada desenlace (ok/no-factible/inválido/timeout/obsoleto) se comporta como exige RF-1 y el estado de generación es consultable por el frontend.

- [x] **T6. Middleware: socket fiable y rutas del solver.** RF-1, RF-2
    - Tests (rojo): `test/test_middleware_transport.cpp` — `solver_resolve`, salud y apagado ordenado; socket reutilizable sin tormenta de reintentos (F-CRASH1).
    - Código: `internalclient.cpp` (estado del socket), `messages.h`, `internalserver.cpp`.
    - Hecho cuando: `ctest` pasa; dos operaciones seguidas no disparan reconexiones espurias.

- [x] **T7. Modo `--backend`: resolver sin base de datos.** RF-1, RF-2
    - Tests (rojo): `test/test_ipc_aislamiento_bd.cpp` — el proceso de cálculo arranca y resuelve sin crear ni abrir archivo de base; rechaza operaciones de negocio.
    - Código: `aplicacion_backend.cpp` (ruta `solver_resolve`), `gestor_proceso_backend.cpp` (salud 5 s y apagado ordenado).
    - Hecho cuando: `ctest --preset full` (Docker, OR-Tools) pasa; no se abre ningún archivo de base en el proceso de cálculo.

- [ ] **T8. Horario generado como archivo JSON (backend).** RF-4
    - Tests (rojo): `test/backend/test_archivos_horario.cpp` — se escribe y se vuelve a leer un `HorarioSalida` como JSON en el destino elegido; fallo de escritura simulado → `Resultado` con error, sin perder el contenido en memoria.
    - Código: `ServicioHorarioSalida` (escritura/lectura del archivo). El diálogo de elección de destino es del frontend.
    - Hecho cuando: `ctest` pasa; un horario guardado se recupera idéntico y un fallo de escritura devuelve error sin retirar el contenido.

- [ ] **T9. Documentar el contrato de generación para el frontend.** RF-1, RF-3, RF-5
    - Tests (rojo): `test/backend/test_contrato_generacion.cpp` — estados de `ServicioGeneracion` (calculando/no-factible/inválido/timeout/«datos anteriores») y `Resultado<T>` con código de error.
    - Código: ninguno de producto; se amplía `docs/interfaz-frontend.md` (sección «Generación de horarios»: `ServicioGeneracion`, `CargadorConfiguracionSolver`, `HorarioSalida`/archivos, `AnalisisSalida`, salud del proceso).
    - Hecho cuando: `ctest` pasa el test de contrato y `docs/interfaz-frontend.md` describe, para cada elemento, qué expone, su firma/contrato, sus estados/errores y cómo debe tratarlo el frontend.

## Cobertura

- RF-1 → T1, T2, T3, T5, T6, T7; RF-2 → T6, T7; RF-3 → T4; RF-4 → T1, T8; RF-5 → estado de
  generación expuesto por T5 y contrato de T9.
- RNF-1 (mensajes) transversal.
- La vista de generación, la visualización y la guardia de cierre (interfaz de RF-1/RF-3/RF-5) las
  implementa y testea el equipo de frontend sobre el contrato documentado en T9.
