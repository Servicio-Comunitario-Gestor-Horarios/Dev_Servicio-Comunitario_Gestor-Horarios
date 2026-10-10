# Plan 003 — Cierre de la spec 001: huecos detectados en la validación

Estado: propuesto.
Spec de referencia: `specs/003-cierre-001-base-datos/spec.md` (aprobada).
Constitución: `docs/constitution.md` (stack único, separación lógica/UI, tests primero).
Depende de: `specs/001-base-datos-local` (implementada).
Interfaz de consumo: `docs/interfaz-frontend.md` (documentación viva; se amplía en la tarea de cierre).

> Este documento describe el CÓMO. No introduce dependencias nuevas: C++17 · Qt6 · CTest.

---

## 1. Archivos y responsabilidades

| Archivo | Cambio | RF |
|---|---|---|
| `src/backend/include/backend/database/respaldo.hpp` + `.cpp` | Añadir `descartarBaseYCrearNueva(ruta, Opciones)` con respaldo obligatorio. | RF-1 |
| `src/backend/include/backend/database/apertura_base_datos.hpp` + `.cpp` | `Opciones::progreso` (observador por paso de migración) y mensajes en español. | RF-3, RF-5 |
| `src/backend/include/backend/services/Servicio*.hpp` + `.cpp` | `listar*` devuelven `Resultado<QVector<Dto>>` (Aula, Materias, Profesor, PlanesEstudio, Cursos, TurnosRecesos). | RF-2 |
| `src/backend/include/backend/services/NucleoDatos.hpp` + `.cpp` | `listar*` delegan y propagan el `Resultado`. | RF-2 |
| `src/app/include/app/instancia_unica.hpp` + `src/app/src/instancia_unica.cpp` | No cambia la API; se documenta el enfoque. | RF-4 |
| `src/app/src/aplicacion_frontend.cpp` (y `main.cpp`) | Pasar la `InstanciaUnica` y conectar `activarSolicitada()` al enfoque de la ventana. | RF-4 |
| `docs/interfaz-frontend.md` | §2.1 (crear base nueva, progreso), §2.3, §2.4 (listar*), §2.6 (enfoque). | RF-1–RF-5 |
| `MEMORY.md` | Registrar el cierre. | — |

---

## 2. Funciones puras / firmas nuevas

| Función | Firma (conceptual) | Qué decide | RF |
|---|---|---|---|
| Descartar y crear base nueva | `Respaldo::ResultadoDescarte descartarBaseYCrearNueva(const QString& ruta, const Opciones& = {})` | Respaldar → descartar → crear; respaldo obligatorio. | RF-1 |
| Progreso de migración | `AperturaBaseDatos::Opciones::progreso` = `std::function<void(int versionDestino)>` | Notifica cada paso de migración. | RF-3 |
| Listar (servicios y núcleo) | `Resultado<QVector<Dto>> listarX() const` | Distingue error de vacío. | RF-2 |

`Respaldo::descartarBaseYCrearNueva` devuelve un `ResultadoDescarte` con `estado`, `detalle` y
`rutaRespaldo`. Estados: `Ok`, `FalloRespaldo` (no se descartó), `FalloCreacion` (se descartó y el
respaldo sigue disponible), `FalloBase` (no había base que descartar).

---

## 3. Decisiones técnicas

| # | Decisión | Alternativa descartada | Motivo |
|---|---|---|---|
| D-1 | `descartarBaseYCrearNueva` abre la base (si existe) y usa `crearRespaldo`; el respaldo es precondición para descartar. | Descartar primero y respaldar después. | RF-1: sin respaldo no se descarta. |
| D-2 | El descarte es un `QFile::remove` de la base; la creación reusa `AperturaBaseDatos::abrir`. | `QFile::rename`. | Simplicidad; el respaldo ya conserva los datos. |
| D-3 | Progreso por callback inyectable en `Opciones`. | Estado interno consultable / señales Qt. | `backend` es UI-free y no usa QObject; el callback es testeable y determinista. |
| D-4 | `listar*` devuelve `Resultado<QVector<Dto>>`. | Vector + canal de error aparte. | Idiomático: el proyecto ya usa `Resultado<T>`; el vacío deja de ser ambiguo. |
| D-5 | El enfoque se cablea pasando la `InstanciaUnica` a `ejecutarAplicacionFrontend` y conectando a `MainWindow`. | Exponer un singleton `instanciaUnicaActiva()`. | Inyección explícita (consistente con el refactor de `src/datos`). |

---

## 4. Estrategia de tests

Comando: `cmake --preset full && cmake --build build && ctest --preset full` (en `gestor-dev`).

| Capa | Archivo de test | Qué comprueba | RF |
|---|---|---|---|
| Backend | `test/backend/test_respaldo.cpp` (extender) | Descartar respalda antes; respaldo imposible no descarta; creación fallida tras descartar conserva el respaldo. | RF-1 |
| Backend | `test/backend/test_apertura_base_datos.cpp` (extender) | El observador de progreso se invoca por paso; sin migración no se invoca. | RF-3 |
| Backend | `test/backend/test_servicio_*.cpp` + `test_backend_nucleo_datos.cpp` | Vacío → `ok` con lista vacía; lectura fallida → `!ok` con mensaje. | RF-2 |
| App | `test/test_instancia_unica.cpp` (extender) | Al pedir activación se emite `activarSolicitada()` y el receptor la recibe. | RF-4 |
| Backend | `test/backend/test_apertura_base_datos.cpp` / `test_respaldo.cpp` | Los mensajes no incluyen texto crudo de Qt y enumeran acciones. | RF-5 |

---

## 5. Trazabilidad RF → partes del plan

| RF | Partes del plan |
|---|---|
| RF-1 | §1 (`respaldo`), §2, §3 (D-1, D-2), test de respaldo |
| RF-2 | §1 (`Servicio*`, `NucleoDatos`), §3 (D-4), tests de servicios y núcleo |
| RF-3 | §1 (`apertura_base_datos`), §2, §3 (D-3), test de apertura |
| RF-4 | §1 (`aplicacion_frontend`, `main`), §3 (D-5), test de instancia única |
| RF-5 | §1 (mensajes en respaldo y apertura), test de mensajes |

---

## 6. Riesgos y supuestos

- **H2 es transversal:** cambia firmas de seis servicios y de `NucleoDatos`, y todos los tests que
  usan `listar*` (~68 usos). Se hace en una sola tarea para no dejar la compilación en rojo.
- **`backend` no usa Qt Widgets** ni QObject: el progreso va por callback, no por señales.
- **OR-Tools solo en Docker:** el cierre no toca el solver; los 3 rojos conocidos de la línea base
  siguen siendo ajenos.
