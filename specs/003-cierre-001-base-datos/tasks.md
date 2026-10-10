# Tareas 003 — Cierre de la spec 001: huecos detectados en la validación

- Spec: `specs/003-cierre-001-base-datos/spec.md`
- Plan: `specs/003-cierre-001-base-datos/plan.md`
- Depende de: `specs/001-base-datos-local` (implementada)
- Interfaz de consumo (otro equipo): `docs/interfaz-frontend.md`
- Comando de tests: `cmake --preset full && cmake --build build && ctest --preset full`
- Regla: **tests primero** (rojo → código → verde → marcar → parar). Una tarea cada vez.
- Entregables: `src/backend` y `src/app`. La interfaz de `src/frontend` la implementa otro equipo.

---

- [x] **T1. Respaldo antes de descartar y creación de base nueva.** RF-1
    - Tests (rojo): `test/backend/test_respaldo.cpp` — descartar respalda antes y crea base nueva; respaldo imposible → no descarta y la base sigue; creación fallida tras descartar → conserva el respaldo.
    - Código: `Respaldo::descartarBaseYCrearNueva` + `ResultadoDescarte` en `respaldo.{hpp,cpp}`.
    - Hecho cuando: `ctest` pasa; no se descarta sin respaldo y la base nueva queda en la versión actual.

- [x] **T2. Distinguir error de lectura de "sin datos" en los `listar*`.** RF-2
    - Tests (rojo): `test/backend/test_servicio_*.cpp` + `test_backend_nucleo_datos.cpp` — lectura vacía → `ok` con lista vacía; lectura fallida → `!ok` con mensaje.
    - Código: `ServicioAula/Materias/Profesor/PlanesEstudio/Cursos/TurnosRecesos` y `NucleoDatos` devuelven `Resultado<QVector<Dto>>`.
    - Hecho cuando: `ctest` pasa; error y vacío se distinguen y el contrato §2.4 queda actualizado.

- [x] **T3. Indicar la migración en marcha.** RF-3
    - Tests (rojo): `test/backend/test_apertura_base_datos.cpp` — el observador inyectado se invoca por cada paso de migración; sin migración no se invoca.
    - Código: `AperturaBaseDatos::Opciones::progreso` invocado en cada paso.
    - Hecho cuando: `ctest` pasa; hay un aviso observable por paso.

- [x] **T4. Enfocar la instancia existente.** RF-4
    - Tests (rojo): `test/test_instancia_unica.cpp` — al pedir activación se emite `activarSolicitada()` y un receptor conectado la recibe.
    - Código: pasar la `InstanciaUnica` a `ejecutarAplicacionFrontend` y conectar `activarSolicitada()` al enfoque de `MainWindow`.
    - Hecho cuando: `ctest` pasa; el enfoque se dispara al segundo arranque y sigue activo durante migración/diálogo.

- [x] **T5. Mensajes de error en español con causa y acciones (RNF-1).** RF-5
    - Tests (rojo): asserts de que los mensajes de apertura y respaldo no contienen texto crudo de Qt y enumeran acciones.
    - Código: reescribir los mensajes de `apertura_base_datos.cpp` y `respaldo.cpp`.
    - Hecho cuando: `ctest` pasa; los mensajes cumplen RNF-1.

## Cobertura

- RF-1 → T1; RF-2 → T2; RF-3 → T3; RF-4 → T4; RF-5 → T5.
- RNF-1 → T5; RNF-2 (no romper lo verde) transversal a todas las tareas.
