# Journal 016 - Comprobación y cierre del tipo general de aula

Registro de la implementación de las tareas del plan 016. Una entrada por tarea ejecutada; las
entradas son **aditivas** (no se reescriben las anteriores).

- **Base:** `bb1767af343c108b20de2b2b4de7ebe1d7658c2f` + la 015 aplicada en el árbol (esquema v6),
  sin commits.
- **Fecha de apertura:** 2026-11-05.
- **Estado:** T1 hecha; T2–T9 pendientes.
- **Suites de partida:** 55/55 (`backend`) y 59/59 (`full-release`).

```text
Comandos de referencia
  backend:      cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend
  full-release: cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release
```

---

## T1 - Comprobación única de validez y causa/acción de las dos ramas al abrir

**Fecha:** 2026-11-05
**Requisito:** RF-1 · RNF-2, RNF-3, RNF-5. **Plan:** §2.1 y §7.

### Rojo (tests primero)

Se escribió `test/backend/test_comprobacion_validez.cpp` (13 casos) y se registró el binario antes de
tener el módulo. El rojo fue de compilación/enlace, porque la implementación no existía:

```text
ninja: error: '.../src/backend/src/database/comprobacion_validez.cpp', needed by
'.../backend_autogen/mocs_compilation.cpp', missing and no known rule to make it
```

### Verde

```text
$ ./test_comprobacion_validez              → 13 passed, 0 failed
$ ./test_backend_apertura_base_datos        → 16 passed, 0 failed
$ ./test_backend_version_esquema            → 44 passed, 0 failed
$ ctest --preset backend                    → 100% tests passed, 56/56 (14,19 s)
```

Casos: `baseValida_esValida`, `sinTipoGeneral_esEsquemaYNombraLaFila`, `sinIndice_esEsquemaYNombraElObjeto`,
`sinDisparadorDeBorrado_esEsquemaYNombraElObjeto`, `sinDisparadorDeRenombrado_esEsquemaYNombraElObjeto`,
`tablaRetirada_esEsquema`, `versionAnterior_esVersionConAcciones`, `noModificaElCatalogo`,
`noDejaTransaccionAbierta`, `funcionaDentroDeUnaTransaccion`, `aperturaYComprobacionCoinciden`,
`mensajeDeVersion_incluyeVersionesYCausaYAccion` y `mensajeDeEsquema_incluyeMotivoYAccion`.

### API

`src/backend/include/backend/database/comprobacion_validez.hpp`:

```cpp
namespace validez_base {
    enum class Causa { Valida, Version, Esquema };   // diagnóstico interno
    struct Comprobacion {
        Causa causa = Causa::Valida;
        QString mensaje;                             // "" si Valida
        bool valida() const;
    };
    Comprobacion comprobar(QSqlDatabase& db,
                           int versionEsperada = VersionEsquema::VERSION_ESQUEMA_ACTUAL);
    QString mensajeDeVersion(int versionArchivo, int versionEsperada);
    QString mensajeDeEsquema(int versionEsperada, const QString& motivo);
}
```

- **Reutiliza** `VersionEsquema::esquemaEsperado` (no hay segunda guardia); la versión se lee primero
  para decidir la causa y no calcular un motivo engañoso en una base de otra versión.
- **Sin escritura**: transacción de solo lectura con el patrón P5 (si ya hay una abierta, se usa y no
  se cierra). Un fallo de lectura de versión se informa como `Causa::Version`.
- **Coste medido** sobre una base válida en memoria (3 filas en `TiposAula`):
  **≈ 1,06 ms por llamada** (1000 iteraciones, media; incluye `PRAGMA user_version` + guardia de
  metadatos + `SELECT` del catálogo). No recorre datos de dominio.

### Mensajes de apertura (dos ramas alineadas)

- **Causa versión:** «… No se migra ni se modifican los datos; cree una base nueva (se respalda la
  actual): restaurar un respaldo de la versión anterior volverá a fallar.»
- **Causa esquema:** «… No se migra ni se modifican los datos; puede restaurar un respaldo de la
  versión vigente o crear una base nueva (con respaldo obligatorio).»

### Archivos tocados

- `src/backend/include/backend/database/comprobacion_validez.hpp` (nuevo).
- `src/backend/src/database/comprobacion_validez.cpp` (nuevo).
- `src/backend/CMakeLists.txt` (añade el `.cpp`).
- `src/backend/src/database/apertura_base_datos.cpp` (acciones de las dos ramas).
- `test/backend/test_comprobacion_validez.cpp` (nuevo, 13 casos).
- `test/backend/test_apertura_base_datos.cpp` (aserciones de las dos acciones).
- `test/CMakeLists.txt` (`add_gtest(test_comprobacion_validez …)`).

### Hallazgos / desviaciones

- El binario es de **gtest** y necesita una `QCoreApplication` viva para que `QSqlDatabase::open()`
  cargue el driver SQLite; se registró un `testing::Environment` que la crea antes de los tests.
- `mensajeDeVersion` usa «debe crear una base nueva» (en vez de «Cree») para contener literalmente
  la acción «crear una base nueva» que exige la tarea; la apertura conserva «cree una base nueva».
- No se toca T2–T9 ni la 013.

---

## T2 - Antes de generar: el servicio no envía y la vista comprueba antes de componer

**Fecha:** 2026-11-05
**Requisito:** RF-1 · RNF-3. **Plan:** §2.2 y §7.

### Rojo (tests primero)

Se escribieron los casos antes del código y el build falló como se esperaba:

```text
error: ‘BaseInvalida’ no es un miembro de ‘Estado’
error: no matching function for call to ‘ServicioGeneracion::ServicioGeneracion(
       PuertoFalso&, int, {}, {}, lambda)’
error: ‘class NucleoDatos’ has no member named ‘comprobarValidezBase’
```

### Verde

```text
$ ./test_servicio_generacion     → 18 passed, 0 failed
$ ./test_backend_nucleo_datos    → 22 passed, 0 failed
$ ./test_contrato_generacion     → 7 passed, 0 failed
$ ctest --preset backend         → 100% tests passed, 56/56 (45,05 s)
$ ctest --preset full-release    → 100% tests passed, 60/60 (38,01 s)
```

> **Total de `full-release`:** **60/60**, no 59/59. El binario nuevo de T1
> (`test_comprobacion_validez`) también se compila y ejecuta en `full-release`; con T4 llegará a
> 61/61, como fija el plan §9.

### Firma y comportamiento

```cpp
explicit ServicioGeneracion(PuertoSolver& puerto,
                            int plazoMs = 60000,
                            std::function<QString()> huellaActual = {},
                            std::function<QDateTime()> reloj = {},
                            std::function<QString()> revisarBase = {},   // 016 RF-1
                            QObject* padre = nullptr);
```

`generar()` **no cambia de firma**. Orden: `enCurso` → `revisarBase` → `validarEntradaSolver` →
envío. Si `revisarBase()` devuelve un mensaje:

- `m_ultimo = ResultadoGeneracion{}` con `Estado::BaseInvalida` y ese mensaje;
- **no** se valida la entrada ni se llama al puerto;
- se emite `finalizada()` exactamente una vez.

`NucleoDatos::comprobarValidezBase()` devuelve `validez_base::comprobar(m_db)` (solo lectura, mismo
predicado que la apertura).

### Vista

- En `onGenerarClicked`, tras `enCurso` y **antes** de `leerOpciones()`/`construirEntrada`, se llama
  a `m_nucleo->comprobarValidezBase()`; si no es válida, se muestra el mensaje («La base de datos no
  es válida») y se sale sin componer ni enviar.
- El único `new ServicioGeneracion` cablea `revisarBase` como quinto argumento:
  `[this]{ return m_nucleo ? m_nucleo->comprobarValidezBase().mensaje : QString(); }`.

### Archivos tocados

- `src/backend/include/backend/services/ServicioGeneracion.hpp` y `.cpp`.
- `src/backend/include/backend/services/NucleoDatos.hpp` y `.cpp`.
- `src/frontend/src/views/schedule_generation_widget.cpp`.
- `test/backend/test_servicio_generacion.cpp`, `test_backend_nucleo_datos.cpp` y
  `test_contrato_generacion.cpp`.

### Hallazgos / desviaciones

- Ninguna funcional. El resto de estados conserva su comportamiento porque `revisarBase` vacío
  mantiene el flujo anterior.
- No se toca T3–T9 ni la 013.

---

## T3 - Antes de guardar la configuración de la hora académica

**Fecha:** 2026-11-05
**Requisito:** RF-1 · RNF-3. **Plan:** §2.3 y §7.

### Rojo (tests primero)

Casos añadidos en `test_servicio_configuracion_horaria.cpp` antes del código:

- `guardar_baseSinElTipoGeneral_rechazaSinEscribir` (P10: `DROP TRIGGER` de los dos y `DELETE` de
  `TiposAula`).
- `guardar_baseSinDisparador_rechazaSinEscribir`.
- `guardar_baseInvalidaTienePrioridadSobreElRango` (base rota + 29).
- `consultar_conBaseInvalida_sigueLeyendo`.

```text
FAIL!  : guardar_baseSinElTipoGeneral_rechazaSinEscribir() ('!resultado.ok' y mensaje sin causa)
FAIL!  : guardar_baseSinDisparador_rechazaSinEscribir() ('!resultado.ok' y mensaje sin el disparador)
FAIL!  : guardar_baseInvalidaTienePrioridadSobreElRango() (devolvía el mensaje de rango)
Totals: 9 passed, 3 failed
```

`consultar_conBaseInvalida_sigueLeyendo` pasaba ya: `consultar` no se toca.

### Verde

```text
$ ./test_servicio_configuracion_horaria → 12 passed, 0 failed
$ ctest --preset backend                → 100% tests passed, 56/56 (14,33 s)
$ ctest --preset full-release           → 100% tests passed, 60/60 (38,25 s)
```

### Orden y texto del rechazo

`ServicioConfiguracionHoraria::guardar`:

1. `validez_base::comprobar(m_db)`; si no es válida:
   ```text
   <mensaje de la causa> No se guardó la configuración de la hora académica.
   ```
   y **no escribe**.
2. `duracion_horaria::errorDeRango` (rango 30–90).
3. *Upsert* de la fila `id = 1`.

`consultar()` no comprueba la base: la pantalla sigue leyendo el valor aunque la base esté rota; solo
se rechaza el guardado.

### Archivos tocados

- `src/backend/src/services/ServicioConfiguracionHoraria.cpp` (comprobación antes del rango).
- `src/backend/include/backend/services/ServicioConfiguracionHoraria.hpp` (comentario).
- `test/backend/test_servicio_configuracion_horaria.cpp` (cuatro casos y helpers P10).

### Hallazgos / desviaciones

- Ninguna. No se toca T4–T9 ni la 013.

---

## T4 - El frontend no sustituye el tipo general

**Fecha:** 2026-11-05
**Requisito:** RF-2 · RNF-1, RNF-4. **Plan:** §2.4 y §7.

### Rojo (tests primero)

Nuevo `test/frontend/test_tipos_aula_ui.cpp` (5 casos) registrado dentro de
`if(TARGET frontend AND TARGET backend)` con `target_include_directories(... src/frontend/src)`.
Con la ayuda todavía devolviendo `int`, el build de `full-release` falla:

```text
error: solicitud por el miembro ‘ok’ en ‘resultado’, el cual es del tipo ‘const int’
error: solicitud por el miembro ‘mensajeError’ en ‘resultado’, el cual es del tipo ‘const int’
```

### Verde

```text
$ ./test_tipos_aula_ui          → 5 casos (7 con init/cleanup), 0 failed
$ ctest --preset full-release   → 100% tests passed, 61/61 (44,64 s)
$ ctest --preset backend        → 100% tests passed, 56/56 (14,03 s)
```

Casos: `encuentraElGeneral`, `sinElGeneral_fallaYNoDevuelveElPrimero`, `catalogoVacio_falla`,
`aceptaMayusculasYEspacios` y `mensaje_nombraElTipoYLaAccion`.

### Firma y mensaje

```cpp
inline Resultado<int> idTipoGeneral(const QVector<TipoAulaDTO>& tipos)
```

- Compara con `tipo_general::esNombreCanonico` (recorte + plegado); acepta «  AULA DE ESTUDIO ».
- **Nunca** devuelve otro tipo: si no encuentra el general (o el catálogo está vacío), error con:

```text
El catálogo de tipos de aula no contiene el tipo general «Aula de estudio»: la base no está en
el estado esperado. Restaure un respaldo de la versión vigente o cree una base nueva (con respaldo
obligatorio). No se ha guardado ningún dato.
```

`nombreTipoGeneral` y `poblar` no cambian.

### Vistas (4 llamadas)

- `classroom_list_widget.cpp`: la lectura del catálogo se comprueba (`!tipos.ok` → aviso y salida);
  después `idTipoGeneral` antes de abrir el diálogo; si falla, aviso y salida.
- `subject_list_widget.cpp`: idéntico.
- `teacher_list_widget.cpp` (alta y edición): si hay materias que crear, resuelve el catálogo y el
  general **antes** de `crearDocente`/`actualizarDocente`; si falla, aviso y `return` sin guardar
  docente, materias ni limpiar las anteriores.

### Criterio ejecutable

```text
$ grep -rn "idTipoGeneral" src/ | wc -l
5
```

La declaración más las **cuatro** llamadas (aula, materia y docente en alta y edición), ahora con
`Resultado<int>`. No aparece una sexta.

### Archivos tocados

- `src/frontend/src/services/tipos_aula_ui.hpp` (`idTipoGeneral` estricta; incluye `resultado.hpp`).
- `src/frontend/src/views/classroom_list_widget.cpp`, `subject_list_widget.cpp` y
  `teacher_list_widget.cpp`.
- `test/frontend/test_tipos_aula_ui.cpp` (nuevo).
- `test/CMakeLists.txt` (registro dentro del bloque de frontend).

### Hallazgos / desviaciones

- El mensaje de lectura fallida se mantiene separado («No se pudo leer el catálogo de tipos de
  aula.» + `mensajeError`); el de catálogo leído sin general es el de `idTipoGeneral`.
- No se toca T5–T9 ni la 013.

---

## T5 - Mensaje de V18: convención y acción, sin validación nueva

**Fecha:** 2026-11-05
**Requisito:** RF-3 · RNF-1, RNF-4. **Plan:** §2.5 y §2.7.

### Rojo (tests primero)

`exigeRechazoV18` en `test/backend/test_validador_entrada.cpp` amplía sus aserciones con la
convención («Aula de estudio») y la acción doble («Asigne al curso un aula del tipo general» y
«base es válida»), manteniendo `startsWith("V18:")`.

```text
FAIL!  : v18_referenciaSobreLaboratorio_falla() ('base es válida' ausente)
FAIL!  : v18_referenciaSobreTallerConMateriasEspecializadas_falla() ('base es válida' ausente)
Totals: 61 passed, 2 failed
```

### Verde

```text
$ ./test_validador_entrada → 63 passed, 0 failed
$ ctest --preset backend   → 100% tests passed, 56/56 (14,64 s)
```

Los seis casos `v18_*` siguen verdes (`..._porClave_pasa`, `..._aulasMoviles_noAplica`,
`..._sinAulaGeneralEnModoFijo_llegaPorV17` y `..._sinAulaGeneralEnModoMovil_llegaPorV17` incluidos).

### Texto final de V18

```text
V18: el aula «%1», referencia del curso %2%3, es del tipo «%4». El tipo general del centro es el
de nombre «%5» y el aula de referencia de cada curso debe ser de ese tipo. Asigne al curso un aula
del tipo general y compruebe que la base es válida.
```

- `%5` sigue saliendo de `tipoGeneralAula()` (definición única de la 015).
- Se conserva el identificador **V18** y el resto de información (aula, curso, tipo recibido y
  esperado).
- **No** se añade ninguna validación ni cambia el orden de la cadena; **V19** sigue libre para la 013.

### Documentación

`docs/contrato-solver-2.0.md`:

- §2 (V18): el mensaje nombra la convención, el tipo recibido y la acción doble; si el catálogo no
  tiene el tipo general, la base no es válida y se arregla restaurando un respaldo de la versión
  vigente o creando una base nueva, **no** editando el preset.
- §4 (aula fija, paso 1): se añade la acción («asignar un aula del tipo general y comprobar que la
  base es válida»).
- **La versión del contrato sigue en 2.0** y la regla no cambia.

### Archivos tocados

- `src/backend/src/solver/config/solver_config.cpp` (solo el texto de V18 y su comentario).
- `test/backend/test_validador_entrada.cpp` (`exigeRechazoV18`).
- `docs/contrato-solver-2.0.md`.

### Hallazgos / desviaciones

- Ninguna. No se toca T6–T9 ni la 013.

---

## T6 - Tipo general fuera de la huella de datos (fijado con pruebas)

**Fecha:** 2026-11-05
**Requisito:** RF-4. **Plan:** §2.6.

### Sin cambio de código (comprobado)

`huellaDatos` ya resume solo la **identidad** del tipo asignado (`aulas[].idTipoAula` y
`materias[].idTipoAula`); no lee `tipoAula` (el nombre visible) ni el catálogo `TiposAula`. No hizo
falta tocar `ServicioGeneracion.cpp` ni su header.

```text
$ grep -n "TiposAula\|tipo_general" src/backend/src/services/ServicioGeneracion.cpp
→ 0 coincidencias (exit 1)
```

### Test nuevo

`huellaDatos_noIncluyeElTipoGeneralNiSuNombreVisible`: dos dominios que solo difieren en
`AulaDTO.tipoAula`/`MateriaDTO.tipoAula` dan la **misma** huella (probado con el cambio conjunto y
con el cambio aislado de aula y de materia).

El control `huellaDatos_cambiaConElTipoDeAula` sigue verde: cambiar `idTipoAula` (la identidad) **sí**
cambia la huella.

### Resultado

```text
$ ./test_servicio_generacion → 19 passed, 0 failed
$ ctest --preset backend     → 100% tests passed, 56/56 (14,12 s)
```

El test nuevo pasa **desde la primera ejecución**: no había fase roja porque el comportamiento
correcto ya existía; la tarea consistía en fijarlo con pruebas y con el `grep`.

### Archivos tocados

- `test/backend/test_servicio_generacion.cpp` (caso nuevo; sin cambios de producción).

### Hallazgos / desviaciones

- Ninguna. No se toca T7–T9 ni la 013.

---

## T7 - Documentación de la API y cierre de la deuda de la 009

**Fecha:** 2026-11-05
**Requisito:** RF-5. **Plan:** §2.7. Solo documentación; sin cambios de código ni de tests.

### `docs/interfaz-frontend.md`

- **§2.2:** añadida la pieza **`validez_base::comprobar`** (con `Causa` como diagnóstico interno y
  una línea por causa) y la frase: la validez se comprueba **al abrir**, **antes de generar** y
  **antes de guardar la configuración**; no migra, no repara y no escribe. No se repite lo que ya
  documentó la 015.
- **§2.4.3:** añadida la **ayuda estricta** `TiposAulaUI::idTipoGeneral`:
  - devuelve `Resultado<int>` y **nunca** sustituye el tipo;
  - altas rechazadas (aula, materia y materia rápida desde docente, alta y edición), sin abrir el
    formulario ni escribir;
  - mensaje con qué falta y qué hacer (respaldo de la versión vigente o base nueva);
  - la base nueva ya trae el general; el catálogo **no** gana campos ni firmas nuevas;
  - **deuda de la 009 sobre esta ayuda: atendida**; la pantalla del catálogo sigue pendiente.
- **§2.4.4:** `guardar` comprueba la base **antes** del rango y no escribe si no es válida
  (mensaje de causa + «No se guardó la configuración de la hora académica.»); `consultar` no comprueba.
- **§3.1:** firma con `revisarBase` (antes de `padre`), `Estado::BaseInvalida`, la regla «no se valida
  la entrada ni se envía» y la nota de que la **huella no incluye el catálogo** (solo `idTipoAula`).
- **§5:** actualización 2026-11-05: la ayuda del tipo general queda **atendida** por la 016; la
  **pantalla** del catálogo sigue como deuda viva.

### `docs/guias/migracion-frontend.md`

- Título e intro: specs 009–012, **015 y 016**; suite del backend **56/56**.
- §1: fila de la **016** (comprobación en tres momentos, ayuda estricta, sin cambios de API).
- §4: checklist de la ayuda estricta y de la comprobación en los tres momentos.
- §5: backend 56/56.
- §6: resumen de la 016.

### Verificación documental (las cuatro órdenes)

```text
$ git diff --stat -- docs/interfaz-frontend.md docs/guias/migracion-frontend.md docs/contrato-solver-2.0.md
 docs/contrato-solver-2.0.md      |   8 ++-
 docs/guias/migracion-frontend.md |  30 ++++++++---
 docs/interfaz-frontend.md        | 115 ++++++++++++++++++++++++++++++---------
 3 files changed, 117 insertions(+), 36 deletions(-)

$ grep -n "validez_base\|antes de generar" docs/interfaz-frontend.md
142:**Comprobación en sesión (`validez_base`).** …
146:namespace validez_base {
161:La validez se comprueba **al abrir**, **antes de generar** y **antes de guardar la configuración**;

$ grep -n "deuda de la 009" docs/interfaz-frontend.md
424:  catálogo **no gana campos ni firmas nuevas**. Con esto queda **atendida la deuda de la 009 sobre

$ grep -n "016" docs/guias/migracion-frontend.md
(1, 13, 28, 141, 145 y 185: título, intro, fila §1, checklist §4 y resumen §6)
```

### Contrato y constitución

- `docs/contrato-solver-2.0.md` sigue en **2.0** (V1–V18); T7 **no lo editó** (los 8 cambios son
  de T5).
- `docs/constitution.md`: **intacto**.

### Archivos tocados

- `docs/interfaz-frontend.md`
- `docs/guias/migracion-frontend.md`
- `specs/016-tipo-general-comprobacion-y-cierre/tasks.md` (T7 marcada)

### Hallazgos / desviaciones

- Ninguna. §2.4.3 no tenía descripción previa de la ayuda, así que solo se añade; no se retira nada
  (la frase «catálogo vacío» ya la retiró la 015).

---

## T8 - Re-base de la 013 sobre el esquema v6 (planner, verificado)

**Fecha:** 2026-11-05. T8 la ejecutó el **planner** (documental, sin código); aquí queda la
verificación del resultado y la evidencia de alcance que pedía el plan.

- `grep -rn "Esquema (v5)" specs/013-preferencias-docente-solver/` → **0**.
- `Base` del plan de la 013 con la **015 y la 016 aplicadas** en el árbol.
- Nota de **ancla supersedida** en `spec.md` (RNF-4 y criterio 11): escenario posterior a la 015/016.

El directorio **no está bajo seguimiento**, así que el alcance se acota con la alternativa que fija
el plan (`git add -N` + `git diff --stat`; se restaura el índice después):

```text
$ git add -N specs/013-preferencias-docente-solver/
$ git diff --stat -- specs/013-preferencias-docente-solver/
 specs/013-preferencias-docente-solver/estado.md  | 193 ++++++++
 specs/013-preferencias-docente-solver/journal.md | 241 +++++++++
 specs/013-preferencias-docente-solver/plan.md    | 474 ++++++++++++++++++
 specs/013-preferencias-docente-solver/spec.md    | 597 +++++++++++++++++++++++
 specs/013-preferencias-docente-solver/tasks.md   | 148 ++++++
 5 files changed, 1653 insertions(+)

$ git reset -q -- specs/013-preferencias-docente-solver/
$ git status --porcelain | grep specs/013
?? specs/013-preferencias-docente-solver/
```

Las cinco entradas están bajo `specs/013-preferencias-docente-solver/`: la re-base no tocó nada
fuera de ese directorio. No se editó la 013 desde esta tarea. No hay commits.

---

## T9 - Regresión global

**Fecha:** 2026-11-05
**Requisito:** RF-1..RF-6 · RNF-6. **Plan:** §9.

### Suites completas

```text
cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend
→ 100% tests passed, 56/56 (23,59 s)

cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release
→ 100% tests passed, 61/61 (37,77 s)
```

Ambos presets compilan y ninguna tarea dejó tests rojos. Los totales coinciden con los esperados
(56/56 y 61/61); no hay diferencias que explicar.

### Literal único

```text
$ grep -rn "Aula de estudio" src --include='*.hpp' --include='*.cpp' | grep -v "/build" \
    | grep -vE '^[^:]+:[0-9]+:\s*(//|/\*|\*)'
src/common/include/common/tipo_general.hpp:13:    inline QString nombre() { return QStringLiteral("Aula de estudio"); }
```

**1** coincidencia, la de la definición única.

### Coste de la comprobación

`validez_base::comprobar` sobre una base válida en memoria (3 filas en `TiposAula`), 1000 iteraciones:
**≈ 1,03 ms por llamada** (medición del cierre; la de T1 fue ≈ 1,06 ms).

### T8 (planner) verificado

`grep -rn "Esquema (v5)" specs/013-preferencias-docente-solver/` → 0; `Base` del plan de la 013 con
la 015 y la 016 aplicadas; nota de ancla supersedida en `spec.md`. No se edita la 013.

### Tabla final T1–T9

| Tarea | Estado | Evidencia |
|---|---|---|
| T1 | ✅ hecha | `validez_base` + 13 casos; backend 56/56 |
| T2 | ✅ hecha | `BaseInvalida`/`revisarBase`, fachada y vista; 56/56 y 60/60 |
| T3 | ✅ hecha | guardado rechazado sin escribir; 56/56 |
| T4 | ✅ hecha | `idTipoGeneral` → `Resultado<int>`, 4 llamadas; 61/61 |
| T5 | ✅ hecha | V18 con convención y acción; contrato 2.0; 56/56 |
| T6 | ✅ hecha | huella sin el catálogo; `grep` 0; 56/56 |
| T7 | ✅ hecha | documentación §2.2/§2.4.3/§2.4.4/§3.1/§5 y guía |
| T8 | ✅ hecha (planner) | re-base de la 013 sobre v6, verificada por `grep` |
| T9 | ✅ hecha | 56/56 y 61/61; literal = 1 |

**La spec no se marca como implementada:** eso corresponde tras el veredicto APROBADO del reviewer.
T8 sigue sin marcar en `tasks.md` porque su edición es del planner.

### Archivos tocados

- `specs/016-tipo-general-comprobacion-y-cierre/journal.md` (esta sección)
- `specs/016-tipo-general-comprobacion-y-cierre/tasks.md` (T9 marcada)

Sin cambios de código, tests, docs ni specs ajenas. No hay commits.

---

## Cierre (2026-11-05)

**VEREDICTO: APROBADO (2026-11-05).** La 016 queda **implementada y cerrada**. La nota anterior («T8 sigue
sin marcar en `tasks.md`» y «la spec no se marca como implementada: eso corresponde tras el veredicto») queda
**superseded por esta entrada**: T8 está marcada y la spec pasa a `implementada (2026-11-05)`.

| Tarea | Qué dejó |
|---|---|
| T1 | `validez_base::comprobar` (causa + mensaje, solo lectura, sobre `esquemaEsperado`) y las dos ramas de la apertura alineadas |
| T2 | `Estado::BaseInvalida` + `revisarBase`, fachada `comprobarValidezBase()` y vista antes de componer |
| T3 | Guardado de la configuración rechazado sin escribir con la base inválida |
| T4 | `idTipoGeneral` → `Resultado<int>`; 4 llamadas en 3 vistas, sin respaldo silencioso |
| T5 | V18 con convención y acción; contrato 2.0 al día |
| T6 | Huella sin el catálogo (prueba + `grep`), sin cambio de código |
| T7 | Documentación §2.2/§2.4.3/§2.4.4/§3.1/§5, deuda de la 009 atendida y guía de migración |
| T8 | **Re-base de la 013 sobre v6, hecha por el planner** y verificada por `grep` + instantánea |
| T9 | Regresión global |

**Notas menores de la validación, aplicadas:** comentarios de esquema **v5 → v6**, **aserción fuerte** en
`test_tipos_aula_ui` y **evidencia de T8** en este `journal` (ver «T8 (planner) verificado»).

**Verificación final**

```text
backend       → 100% tests passed, 56/56
full-release  → 100% tests passed, 61/61
```

**Sin commits:** todo el trabajo de la 015 y de la 016 quedó en el árbol de trabajo; `HEAD` sigue en
`bb1767af343c108b20de2b2b4de7ebe1d7658c2f`. Las deudas aceptadas están en `estado.md` → «Deudas y notas».
