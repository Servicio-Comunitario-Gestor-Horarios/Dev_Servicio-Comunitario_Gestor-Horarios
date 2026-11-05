# Tareas 015 — El tipo general de aula como parte del esquema

Base: **`bb1767af343c108b20de2b2b4de7ebe1d7658c2f`**. Spec y plan aprobados el **2026-11-05**.
Regla: **una tarea por llamada**, tests en rojo → código → verde → marcar y parar. Comando base de todas:

```bash
cmake --preset backend && cmake --build --preset backend --parallel
```

---

## [x] T1. Definición única del literal, alta del tipo general en la creación y marca v6

**RF:** RF-1, RF-2 (literal y unicidad), RF-6 (marca) · RNF-3, RNF-4, RNF-5.

**Tests primero** (en rojo):
- `test/backend/test_version_esquema.cpp`: `versionActual_esCinco` → **`versionActual_esSeis`** (hoy espera 5);
  `crearEsquema_dejaElTipoGeneral` (exactamente una fila, nombre `Aula de estudio`, clave `aula de
  estudio`); `crearEsquema_esIdempotente` (segunda llamada no duplica ni cambia);
  `crearEsquema_fallido_noDejaEsquemaParcial` (tabla `TiposAula` preexistente e incompatible → falla,
  `user_version` queda en 0 y **no hay tablas nuevas**: la preexistente se queda, como es lógico).
- `test/backend/test_servicio_aula.cpp` y `test_servicio_materias.cpp`: en una base nueva, el alta con el id
  del tipo general **funciona**; se reescriben `crearAula_catalogoVacio_falla` y
  `crearMateria_catalogoVacio_falla` como `..._sinTipoValido_falla` (el estado «catálogo vacío» ya no existe).
- `test/backend/test_servicio_tipos_aula.cpp`: `crearTipoAula_duplicadoDelGeneral_falla` (mensaje de
  duplicado existente) y `listarTiposAula_incluyeElGeneral`; **`renombrarTipoAula_propagaAAulasYMaterias_sinReescribir`,
  `eliminarTipoAula_enUsoPorAula_falla` y `eliminarTipoAula_enUsoPorAulaYMateria_listaAmbos` pasan a usar
  «Laboratorio»/«Taller»** y el general se reserva para los casos de prioridad de T6 (**P3**).
- `test_servicio_carga_horaria.cpp` y `test_servicio_profesor.cpp`: sustituir `crearTipoAula("Aula de
  estudio")` por la búsqueda del id del general (**P2**).

**Código**
1. `src/common/include/common/tipo_general.hpp` con `nombre()`, `claveNormalizada()`, `clave()`,
   `esNombreCanonico()` (plan §2.1).
2. `src/backend/CMakeLists.txt`: `common` en `target_link_libraries(backend PUBLIC …)` (nada más en esta
   tarea).
3. **Barrido de los tres literales y los seis comentarios (P1):**
   - `contrato_solver.cpp:38` → `tipo_general::nombre()`; `claveTipo` → delegado;
     `ServicioTiposAula::claveTipo` → delegado; `tipos_aula_ui.hpp:24` (`nombreTipoGeneral`) → delegado
     (sin cambiar la caída al primer tipo; eso es de la 016).
   - `perfil.cpp:44`: se sustituye `kTipoGenerico` por `tipo_general::nombre()`; el campo `tipo` del struct
     local `kAulas` pasa de `const char*` a `QString` (**P7**) y `tipoEspecializadoDeMateria`/
     `tipoFinalDeMateria` devuelven `tipo_general::nombre()`. **En T1 el perfil sigue declarando los 3
     tipos**: quien los reduce a 2 es T7.
   - Comentarios a reescribir sin la cadena: `contrato_solver.hpp:82`, `perfil.cpp:309`, `perfil.hpp:67`,
     `perfil.hpp:111`, `perfil.hpp:115`, `tipos_aula_ui.hpp:9` (usar «el tipo general»).
4. `migracion.cpp`: `INSERT OR IGNORE INTO TiposAula (nombre, clave) VALUES (?, ?)` con valores de la
   definición única, tras el índice y antes de `Cursos`.
5. `version_esquema.hpp`: `VERSION_ESQUEMA_ACTUAL = 6` y comentario de la v6.
6. `test_version_esquema.cpp`: `HUELLAS_CONOCIDAS` con la entrada **6**; en T1 el valor es el mismo que el
   de la 5 (`eebe660c385f9802dc7b0b63abb6e7821db3ad3f2ec35982eeae55e49e9deaa0`), porque la función de huella
   todavía resume **solo tablas** y el DDL de tablas no cambia; se comenta, y T5 la sustituye al ampliar la
   huella a índices y disparadores.
7. `test_servicio_aula.cpp`, `test_servicio_materias.cpp`, `test_servicio_tipos_aula.cpp`,
   `test_servicio_carga_horaria.cpp`, `test_servicio_profesor.cpp`, `test_nucleo_datos.cpp`,
   `test_cascada.cpp`, `test_gestor_pendientes.cpp`, `test/datos/test_contexto_base_datos.cpp`: dejar de
   crear el tipo general; buscar su id (helper local `idTipoGeneral(db)` con
   `SELECT id FROM TiposAula WHERE clave = 'aula de estudio'`) y ajustar los recuentos de tipos (+1).

**Verificación**

```bash
cmake --preset backend && cmake --build --preset backend --parallel \
  && ctest --preset backend -R '^(test_backend_version_esquema|test_servicio_aula|test_servicio_materias|test_servicio_tipos_aula|test_servicio_carga_horaria|test_servicio_profesor|test_backend_nucleo_datos|test_backend_cascada|test_backend_gestor_pendientes|test_datos_contexto_base_datos)$'
grep -rn "Aula de estudio" src --include='*.hpp' --include='*.cpp' | grep -v "/build" \
  | grep -vE '^[^:]+:[0-9]+:\s*(//|/\*|\*)'
```

**Hecho cuando:** las dos órdenes pasan, el `grep` (que **excluye comentarios**) devuelve **una sola**
coincidencia (`common/tipo_general.hpp`) y el `grep` sin filtro también da **1** porque los seis comentarios
se reescribieron; una base nueva tiene el tipo general y se abre de nuevo; los tests tocados están en verde.
**Estimación: > 30 min.**

---

## [x] T2. Disparador de borrado del tipo general

**RF:** RF-3 (protección en cualquier vía) · RNF-6.

**Tests primero:** en `test_version_esquema.cpp`,
`proteccion_borradoDirecto_delGeneral_esRechazado` (`DELETE FROM TiposAula WHERE clave='aula de estudio'`
falla y la fila sigue) y `proteccion_borradoDeUnEspecializado_sePermite` (control positivo).

**Código:** en `migracion.cpp`, tras el alta:
`CREATE TRIGGER IF NOT EXISTS TiposAula_general_no_borrar BEFORE DELETE ON TiposAula WHEN OLD.clave =
'<clave>' BEGIN SELECT RAISE(ABORT, '<mensaje>'); END;`, con clave y mensaje de la definición única
(escapado `'` → `''`). Los textos se declaran en `backend/database/tipo_general.hpp`.

**Verificación**

```bash
cmake --build --preset backend --parallel && ctest --preset backend -R '^(test_backend_version_esquema|test_servicio_tipos_aula)$'
```

**Hecho cuando:** el borrado directo del general falla con el mensaje en español y la fila permanece; el
borrado de un especializado sin dependientes sigue funcionando; el resto de `test_version_esquema` sigue en
verde (la huella no cambia: T5 la amplía). **Estimación: 20–30 min.**

---

## [x] T3. Disparador de renombrado del tipo general

**RF:** RF-4 (protección en cualquier vía, `UPDATE` que no cambia nada no se bloquea) · RNF-6.

**Tests primero:** en `test_version_esquema.cpp`:
`proteccion_renombradoDirecto_delGeneral_esRechazado` (cambio de `nombre` y cambio de `clave`);
`proteccion_updateSoloDeFecha_delGeneral_sePermite`; `proteccion_updateSinCambios_sePermite`.

**Código:** `CREATE TRIGGER IF NOT EXISTS TiposAula_general_no_renombrar BEFORE UPDATE OF nombre, clave ON
TiposAula WHEN OLD.clave = '<clave>' AND (NEW.nombre <> OLD.nombre OR NEW.clave <> OLD.clave) BEGIN SELECT
RAISE(ABORT, '<mensaje>'); END;` con el escapado del §2.3 del plan.

**Verificación**

```bash
cmake --build --preset backend --parallel \
  && ctest --preset backend -R '^(test_backend_version_esquema|test_servicio_tipos_aula)$'
```

**Hecho cuando:** los tres casos pasan, un `UPDATE` de la fila general que no toca `nombre` ni `clave` no se
bloquea y `test_servicio_tipos_aula` sigue en verde (T1 ya movió al especializado los casos de renombrado y
borrado en uso, **P3**). **Estimación: 20–30 min.**

---

## [x] T4. Predicado puro de validez de la fila

**RF:** RF-2 (identidad), RF-6 (predicado único).

**Tests primero:** nuevo `test/backend/test_tipo_general.cpp` (`add_gtest(test_tipo_general
backend/test_tipo_general.cpp backend)` en `test/CMakeLists.txt`, dentro del bloque de backend):
`evaluar_sinFilas_esSinTipoGeneral`; `evaluar_unaFilaCanonica_esValida`; `evaluar_dosFilasConNombreCanonico_esVariasFilas`
(incluido el caso nombre canónico + clave `AULA DE ESTUDIO` junto a la correcta);
`evaluar_unaFilaConClaveDistinta_esClaveDistinta`; `evaluar_ignoraElOrdenDeEntrada`; y que cada estado
rellena un `motivo` en español con el conteo y el valor esperado.

**Código:** `src/backend/include/backend/database/tipo_general.hpp` (`Fila`, `EstadoFila`, `evaluar`,
`OperacionProtegida`, `EstadoFilaProtegida`, `traducirRechazo`) y `src/backend/src/database/tipo_general.cpp`;
añadir el `.cpp` a `target_sources(backend …)`. **La traducción del rechazo es un ayudante puro (P4)** y aquí
se prueba sola:
`traducirRechazo_sinFallo_devuelveVacio`; `traducirRechazo_falloDeFilaInexistente_devuelveVacio`;
`traducirRechazo_falloDeOtraFila_devuelveVacio`; `traducirRechazo_falloDelGeneral_borrado_devuelveElMensajeDelCatalogo`;
`traducirRechazo_falloDelGeneral_renombrado_incluyeElValorRecibido`.

**Verificación**

```bash
cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend -R '^test_tipo_general$'
```

**Hecho cuando:** todos los casos del predicado y los cinco de `traducirRechazo` pasan, con motivo/mensaje
en español, sin tocar la base. **Estimación: 25–30 min.**

---

## [x] T5. Guardia v6: índice, dos disparadores, fila y huella

**RF:** RF-6.

**Tests primero:** en `test_version_esquema.cpp`:
`esquemaEsperado_exigeElIndiceYLosDosDisparadores` (quitar cada objeto → `false` y el motivo lo nombra);
`esquemaEsperado_exigeExactamenteUnaFila` (0 filas, 2 filas con nombre canónico, fila única con clave
distinta → `false`, con el motivo del predicado); `esquemaEsperado_toleraObjetosDeMas` (un índice extra →
`true`); `esquemaEsperado_rechazaTablaRetirada` (ya existe: se mantiene); ampliar
`huellaEsquema_noCambiaSinSubirLaMarcaDeVersion` a la huella que incluye índice y disparadores;
`esquemaEsperado_noDejaTransaccionAbierta` y `esquemaEsperado_funcionaDentroDeUnaTransaccion` (**P5**).

> **Montaje de los casos de la fila (P10):** el disparador de borrado impide quitar la fila, así que cada
> caso **hace `DROP TRIGGER TiposAula_general_no_borrar`, muta la fila y comprueba**; la base es en memoria
> y se crea por test, de modo que no hace falta restaurar el disparador. Alternativa: construir el esquema a
> mano con `Migracion::aplicarPaso` y luego alterar. **No** se desactiva la protección desde código de
> producto.

**Código**
1. `version_esquema.cpp`: `INDICES_ESPERADOS` y `DISPARADORES_ESPERADOS` (consulta a `sqlite_master` por
   `type`/`name`), llamada a `tipo_general::evaluar` con las filas leídas, y motivos nuevos junto a los
   existentes. **Toda la comprobación va en una transacción de solo lectura** (`db.transaction()` …
   `commit()`); si ya hay una abierta, se usa esa (**P5**). La función no escribe.
2. `test_version_esquema.cpp`: `huellaEsquema` incluye `type IN ('table','index','trigger')`; sustituir la
   entrada 6 por la huella real nueva; comentar que las entradas 2–5 son históricas (resumen de solo tablas).

**Verificación**

```bash
cmake --build --preset backend --parallel && ctest --preset backend -R '^(test_backend_version_esquema|test_backend_apertura_base_datos|test_tipo_general)$'
```

**Hecho cuando:** una base v6 sin el índice, sin un disparador o sin la fila no corresponde al esquema
esperado y el motivo lo nombra; una base con objetos de más sigue siendo válida; la huella v6 está registrada;
las bases anteriores siguen rechazándose; `esquemaEsperado` no deja transacciones abiertas y funciona dentro
de una. **Estimación: > 30 min.**

---

## [x] T6. Mensajes del catálogo: prioridad, renombrado total y traducción del rechazo

**RF:** RF-3, RF-4, RF-7 · RNF-1.

**Tests primero:** en `test_servicio_tipos_aula.cpp`:
`eliminarTipoAula_general_fallaConPrioridad` (sin dependientes y **con** dependientes: el mensaje habla del
tipo general, no de la lista);
`renombrarTipoAula_general_fallaAunqueElNombreSeaElMismo`; `renombrarTipoAula_general_fallaConOtraGrafia`
(«AULA DE ESTUDIO»); `eliminarTipoAula_especializadoEnUso_sigueListandoDependientes` (control).

> **Sin test de carrera en el servicio (P4).** La comprobación previa hace que la rama de traducción sea
> inalcanzable desde el servicio, así que **no** se fuerza: la traducción se prueba en **T4** sobre el
> ayudante puro `traducirRechazo`, y el alcance del disparador (solo escrituras directas) se prueba por SQL
> directo en **T2/T3**. El servicio se limita a **usar** el ayudante tras releer la fila.

**Código:** `ServicioTiposAula` (`leerFila`, `esTipoGeneral`, prioridad en `eliminarTipoAula`, rechazo
incondicional en `renombrarTipoAula`, relectura y `traducirRechazo` tras un `exec` fallido). **Dos variantes
del mensaje (P12):** la del disparador es fija y va en el `RAISE`; la del servicio lleva el valor recibido y
es la única que cumple RNF-1. Sin cambios de firma pública.

**Verificación**

```bash
cmake --build --preset backend --parallel && ctest --preset backend -R '^(test_servicio_tipos_aula|test_backend_nucleo_datos|test_backend_gestor_pendientes)$'
```

**Hecho cuando:** los cuatro rechazos del general dan el mensaje en español con la prioridad pedida, el
borrado en uso de un especializado sigue listando dependientes, y ningún camino devuelve el texto crudo del
almacenamiento. **Estimación: > 30 min.**

---

## [x] T7. Siembra: reutilizar, excluir del chequeo, conservar y recuento 3

**RF:** RF-5 · RNF-2, RNF-3, RNF-6.

**Tests primero:** en `test_seed_perfil.cpp`: `perfil.tiposAula` = 2 («Laboratorio», «Taller»), la validación
acepta el general del esquema y `recuentosEsperados.tiposAula == 3`. En `test_seed_aplicacion.cpp`:
`aplicar_sobreBaseNueva_noInformaBaseConDatos`; `aplicar_reutilizaElTipoGeneral` (sin duplicar y con el
mismo id antes y después); `reemplazar_conservaElTipoGeneral`; `aplicar_dosVeces_mismosDatosYRecuentos`
(sobre bases vacías y con `--reemplazar`); `aplicar_recuentoTiposEsTres`.

**Código**
- `perfil.cpp` / `perfil.hpp`: fuera `kTipoGenerico`; `tiposAula = {"Laboratorio","Taller"}`; aulas,
  `tipoFinalDeMateria` y `validarPerfil` usan `tipo_general::nombre()`.
- `docentes.cpp`: `recuentosEsperados.tiposAula = perfil.tiposAula.size() + 1`.
- `sembrador.cpp`: `contarFilasDeDominio` excluye la fila del general; alta completa resuelve el id del
  general por `clave`; reemplazo con `DELETE FROM TiposAula WHERE clave <> ?`.
- `test_seed_aplicacion.cpp`: ajustar los helpers que comparan el catálogo (`SELECT nombre, clave FROM
  TiposAula`) y los recuentos si alguno contaba tipos del perfil.

**Verificación**

```bash
cmake --build --preset backend --parallel && ctest --preset backend -R '^(test_seed_aplicacion|test_seed_perfil)$'
```

**Hecho cuando:** una base recién creada se siembra sin avisar de «base con datos», el reemplazo total deja
el general con su identidad, dos aplicaciones dejan los mismos datos y el recuento de tipos es 3 leído de la
base. **Estimación: > 30 min.**

---

## [x] T8. Documentación, supersesiones y regresión

**RF:** RF-7 (coherencia documental) · RNF-4, RNF-7.

**Tests primero:** no aplica un test nuevo; se ejecuta la suite completa **antes** de editar los documentos
para fijar el estado verde y **después** para confirmar que nada se rompió.

**Documentación (la edita el implementer)**
1. `docs/interfaz-frontend.md`: `VERSION_ESQUEMA_ACTUAL = 6` + comentario; descripción del catálogo
   esperado con índice, dos disparadores y la fila; nota de la huella (tablas, índices y disparadores); y las
   dos frases de la **P11**: «La versión **5** añade las preferencias… al catálogo de la **v4**» pasa a
   describir la **v6** sobre la **v5**, y «Una base con la versión anterior (**4**) **no se migra**» pasa a
   **5**.
2. `docs/guias/migracion-frontend.md`: «Esquema de base: `v6`»; «bases anteriores no abren» pasa a «v5 o
   anterior»; fila de la 015 (esquema v6, sin cambios de API).
3. `docs/constitution.md`: **no se toca** (justificación en el plan §2.8).

**Verificación de lo que redacta el planner (P6)**
- **Las notas de supersesión se redactan en la fase de plan/cierre** (el planner las aplica en `specs/001`,
  `004`, `008`, `009`, `010`, `011` y `012`, con el texto comprometido en el plan §2.8). **T8 no las
  redacta: solo verifica que están**; si falta alguna, se **para** y se devuelve al planner.

**Verificación**

```bash
cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend
cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release
grep -rn "VERSION_ESQUEMA_ACTUAL = 5\|Esquema de base:\*\* \`v5\`\|La versión \*\*5\*\* añade\|versión anterior (\*\*4\*\*)" docs/ || echo "sin restos"
grep -rn "Aula de estudio" src --include='*.hpp' --include='*.cpp' | grep -v "/build" \
  | grep -vE '^[^:]+:[0-9]+:\s*(//|/\*|\*)'
grep -rln "Anotado por la 015" specs/001* specs/004* specs/008* specs/009* specs/010* specs/011* specs/012*
```

**Hecho cuando:** las dos suites están en verde (**55/55** en `backend` por `test_tipo_general` y
**59/59** en `full-release`, tomando 54 y 58 como partida); el primer `grep` **no devuelve nada**; el
segundo devuelve **una sola** coincidencia (`common/tipo_general.hpp`), igual que en T1; el tercero
devuelve **siete líneas**, una por spec, con la nota «Anotado por la 015». **Estimación: > 30 min.**

---

## Orden, dependencias y exceso

T1 → T2 → T3 → T4 → T5 → T6 → T7 → T8. **T4 antes de T5** (la guardia usa el predicado). Sin dependencias
circulares; ninguna tarea deja el árbol sin compilar.

| Tarea | Estimación | Exceso sobre 30 min |
|---|---|---|
| T1 | > 30 min | sí |
| T2 | 20–30 | no |
| T3 | 20–30 | no |
| T4 | 25–30 | no |
| T5 | > 30 min | sí |
| T6 | > 30 min | sí |
| T7 | > 30 min | sí |
| T8 | > 30 min | sí |

Los cinco excesos quedan **declarados y aceptados** con la indicación del usuario del 2026-11-05 de seguir
sin parada de aprobación. **No se divide la spec**: ocho tareas, por debajo del límite de 10.
