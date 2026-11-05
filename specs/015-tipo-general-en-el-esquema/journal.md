# Journal 015 - El tipo general de aula como parte del esquema

Registro de la implementación de las tareas del plan 015. Una entrada por tarea ejecutada; las
entradas son **aditivas** (no se reescriben las anteriores).

- **Base:** `bb1767af343c108b20de2b2b4de7ebe1d7658c2f` (HEAD de `feat/Motor`).
- **Fecha de apertura:** 2026-11-05.
- **Estado:** T1 hecha; T2–T8 pendientes.
- **Aprobaciones:** spec **2026-11-05** · plan/tareas **2026-11-05** (el usuario ordenó seguir sin
  parada de aprobación del plan; los excesos de T1, T5, T6, T7 y T8 quedan aceptados).

```text
Comandos de referencia
  backend:        cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend
  verificación T1: ctest --preset backend -R '^(test_backend_version_esquema|test_servicio_aula|...)$'
```

---

## T1 - Definición única del literal, alta del tipo general en la creación y marca v6

**Fecha:** 2026-11-05
**Requisito:** RF-1, RF-2 (literal y unicidad), RF-6 (marca) · RNF-3, RNF-4, RNF-5. **Plan:** §2.1,
§2.2, §2.5 (solo marca y huella histórica), §5.

### Rojo (tests primero)

Se escribieron/ajustaron los tests antes del código (10 binarios del alcance de T1 más dos que la
tarea rompía, ver «Hallazgos»). `cmake --preset backend && cmake --build --preset backend
--parallel` compiló y `ctest` del alcance dio **0/10**:

```text
0% tests passed, 10 tests failed out of 10
FAILED: test_backend_version_esquema, test_backend_nucleo_datos, test_backend_cascada,
        test_backend_gestor_pendientes, test_servicio_aula, test_servicio_materias,
        test_servicio_tipos_aula, test_servicio_carga_horaria, test_servicio_profesor,
        test_datos_contexto_base_datos
```

Casos nuevos en rojo destacados: `versionActual_esSeis` (esperaba 5),
`crearEsquema_dejaElTipoGeneral`, `crearEsquema_esIdempotente`,
`crearEsquema_fallido_noDejaEsquemaParcial`, `crearAula_sinTipoValido_falla`,
`crearMateria_sinTipoValido_falla`, `crearTipoAula_duplicadoDelGeneral_falla`,
`listarTiposAula_incluyeElGeneral` y la búsqueda de `idTipoGeneral` en los seis tests que antes
creaban el general.

### Verde

```text
cmake --preset backend && cmake --build --preset backend --parallel
ctest --preset backend -R '^(test_backend_version_esquema|test_servicio_aula|test_servicio_materias|test_servicio_tipos_aula|test_servicio_carga_horaria|test_servicio_profesor|test_backend_nucleo_datos|test_backend_cascada|test_backend_gestor_pendientes|test_datos_contexto_base_datos)$'
→ 100% tests passed, 10/10 (1,06 s)
```

Casos por binario (Qt Test, todos en verde): `test_backend_version_esquema` 34, `test_servicio_aula`
27, `test_servicio_materias` 25, `test_servicio_tipos_aula` 31, `test_servicio_carga_horaria` 24,
`test_servicio_profesor` 57, `test_backend_nucleo_datos` 20, `test_backend_cascada` 21,
`test_backend_gestor_pendientes` 13, `test_datos_contexto_base_datos` 11.

Criterio ejecutable de la definición única (T1, plan §2.1), con y sin filtro de comentarios:

```text
$ grep -rn "Aula de estudio" src --include='*.hpp' --include='*.cpp' | grep -v "/build" \
    | grep -vE '^[^:]+:[0-9]+:\s*(//|/\*|\*)'
src/common/include/common/tipo_general.hpp:13:    inline QString nombre() { return QStringLiteral("Aula de estudio"); }
```

Igual sin filtro: **1** coincidencia. La huella de la v6 es la misma que la de la v5
(`eebe660c…`) porque en T1 el resumen sigue siendo solo de tablas y el DDL de tablas no cambia; T5
la sustituye al ampliar la huella a índices y disparadores.

### Archivos tocados

**Código**
- `src/common/include/common/tipo_general.hpp` (nuevo): `nombre()`, `claveNormalizada()`,
  `clave()`, `esNombreCanonico()`; única aparición del literal.
- `src/backend/CMakeLists.txt`: `common` en `target_link_libraries(backend PUBLIC …)`.
- `src/backend/src/database/migracion.cpp`: `INSERT OR IGNORE INTO TiposAula (nombre, clave)` con
  la definición única, tras el índice y antes de `Aulas`/`Cursos`.
- `src/backend/include/backend/database/version_esquema.hpp`: `VERSION_ESQUEMA_ACTUAL = 6` y
  comentario de la v6.
- `src/backend/src/solver/config/contrato_solver.cpp` y
  `src/backend/src/services/ServicioTiposAula.cpp`: `claveTipo` delega en
  `tipo_general::claveNormalizada`; `tipoGeneralAula()` devuelve `tipo_general::nombre()`.
- `src/backend/include/backend/solver/config/contrato_solver.hpp` (un comentario).
- `src/frontend/src/services/tipos_aula_ui.hpp`: `nombreTipoGeneral()` delegado y comentario.
- `src/seed/src/perfil.cpp`: fuera `kTipoGenerico`; `p.tiposAula` sigue con 3 tipos (T7 lo reduce a
  2); campo `tipo` de `kAulas` pasa a `QString`; `tipoEspecializadoDeMateria` y
  `tipoFinalDeMateria` caen en `tipo_general::nombre()`; comentario de las aulas reescrito.
- `src/seed/include/seed/perfil.hpp`: tres comentarios reescritos (sin el literal).
- `src/seed/src/sembrador.cpp` (cierre del árbol, ver abajo): el recuento de dominio de
  `TiposAula` excluye la fila del general y el alta de tipos reutiliza el id existente del general
  en vez de intentar crearlo.

**Tests**
- `test/backend/test_version_esquema.cpp`: `versionActual_esSeis`, nuevos
  `crearEsquema_dejaElTipoGeneral`, `crearEsquema_esIdempotente` y
  `crearEsquema_fallido_noDejaEsquemaParcial`; entrada 6 en `HUELLAS_CONOCIDAS` (misma huella que
  la 5, comentado).
- `test/backend/test_servicio_aula.cpp` y `test_servicio_materias.cpp`: helper `idTipoGeneral(db)`;
  `..._catalogoVacio_falla` → `..._sinTipoValido_falla` (alta con el general válida y `-1`
  rechazado sin el mensaje de catálogo vacío).
- `test/backend/test_servicio_tipos_aula.cpp`: `listarTiposAula_vacio` →
  `listarTiposAula_incluyeElGeneral`; nuevo `crearTipoAula_duplicadoDelGeneral_falla` (incluida
  «AULA DE ESTUDIO»); renombrado/borrado en uso pasan a «Laboratorio»/«Taller»; recuentos +1.
- `test/backend/test_servicio_carga_horaria.cpp`, `test_servicio_profesor.cpp`,
  `test_backend_nucleo_datos.cpp`, `test_backend_cascada.cpp`,
  `test_backend_gestor_pendientes.cpp` y `test/datos/test_contexto_base_datos.cpp`: dejan de crear
  el general y resuelven su id por `clave = 'aula de estudio'`.
- `test/backend/test_seed_aplicacion.cpp` (cierre del árbol): el caso de rollback acepta la única
  fila que sobrevive, el tipo general del esquema (los demás recuentos de dominio siguen en 0).

### Hallazgos / desviaciones

- **Dos tests no listados en T1 quedaban rojos con el alta del general**
  (`test_servicio_planes_estudio.cpp` y `test_composicion_plan.cpp`, ambos `crearTipoAula("Aula de
  estudio")` en `init`). El plan §7 obliga a corregir «en la tarea que lo rompa»; se sustituyó la
  creación por la búsqueda del id del general. No es alcance de T2–T7.
- **`test_seed_aplicacion` se cerró dentro de T1 (a petición del usuario)**, no en T7: al crear la
  base el tipo general ya existe, así que la siembra inicial fallaba con «La base ya tiene datos» y
  el alta del general chocaba por duplicado. Se aplicó lo mínimo en `sembrador.cpp`: el recuento de
  dominio de `TiposAula` excluye la fila del general (`clave <> tipo_general::clave()`) y el alta
  reutiliza su id en vez de crearlo. El **reemplazo total sigue sin cambios** (T7). Detalle en
  «Cierre del árbol».
- **Frontend:** la compilación completa no es posible en este host porque `build/`, `build-fe/` y
  `build-rel/` tienen caché generada en `/workspace` (rutas de Docker) y `--preset backend` excluye
  la interfaz. Se verificó el encabezado con `g++ -fsyntax-only` incluyendo
  `src/frontend/src`, `src/backend/include` y `src/common/include`: compila. El cambio es solo la
  delegación del literal (misma cadena); **no se abrió la GUI**.
- Sin cambios de firma ni de DTO; no se toca `spec.md`, `plan.md`, `AGENTS.md` ni
  `docs/constitution.md`.

### Cierre del árbol (ampliación de T1)

Tras la primera pasada quedaba rojo `test_seed_aplicacion` (14 casos) porque la base nueva ya trae
el tipo general. Se corrigió **dentro de T1** con el mínimo en `src/seed/src/sembrador.cpp`:

1. `contarFilasDeDominio` cuenta `TiposAula` con `WHERE clave <> :clave` usando
   `tipo_general::clave()`: una base recién creada sigue siendo «vacía».
2. El alta de tipos reutiliza el id del general si existe (`SELECT id … WHERE clave = …`) y solo
   crea los que faltan; sin `INSERT OR IGNORE` no queda un id duplicado ni se toca la fila.
3. Los recuentos no cambian: el perfil sigue declarando 3 tipos y la base queda con 3 (general
   reutilizado + «Laboratorio» + «Taller»). El perfil a 2 tipos y el reemplazo que conserva la
   identidad del general siguen siendo **T7**.

Único ajuste de test: en `test_seed_aplicacion.cpp`, el caso de rollback acepta **1** fila en
`TiposAula` (la del general que deja el esquema) y **0** en las demás tablas de dominio.

Verificación completa (esta vez toda la suite `backend`):

```text
cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend
→ 100% tests passed, 54/54 (19,46 s)
```

`test_seed_aplicacion`: 20/20 casos. `test_seed_perfil`: sin cambios y en verde.

`grep` del literal (sin comentarios): **1** coincidencia, en
`src/common/include/common/tipo_general.hpp:13`. Igual sin filtro.

---

## T2 - Disparador de borrado del tipo general

**Fecha:** 2026-11-05
**Requisito:** RF-3 (protección en cualquier vía) · RNF-6. **Plan:** §2.3 (variante fija del mensaje)
y §5.

### Rojo (tests primero)

En `test/backend/test_version_esquema.cpp`:

- `proteccion_borradoDirecto_delGeneral_esRechazado`: `DELETE FROM TiposAula WHERE clave =
  'aula de estudio'` falla, el mensaje contiene «No se puede eliminar el tipo de aula general del
  centro» y la fila sigue.
- `proteccion_borradoDeUnEspecializado_sePermite`: control positivo; «Laboratorio» sin dependientes
  se borra y solo queda el general.

```text
$ ./test_backend_version_esquema
FAIL!  : TestVersionEsquema::proteccion_borradoDirecto_delGeneral_esRechazado()
Totals: 35 passed, 1 failed, 0 skipped, 0 blacklisted, 122ms
```

### Verde

```text
$ ./test_backend_version_esquema → 36 passed, 0 failed
$ ./test_seed_aplicacion        → 20 passed, 0 failed
$ ctest --preset backend        → 100% tests passed, 54/54 (13,04 s)
```

### Código

`src/backend/include/backend/database/tipo_general.hpp` (nuevo): `OperacionProtegida` y la
**variante fija** `mensajeProteccionAlmacenamiento(op)`, con los dos textos del plan §2.3:

```text
Borrado:    No se puede eliminar el tipo de aula general del centro.
Renombrado: No se puede renombrar el tipo de aula general del centro.
```

En T2 solo se usa la de borrado; la de renombrado queda declarada para T3. La variante del servicio
(con el valor recibido) sigue siendo de T6.

`src/backend/src/database/migracion.cpp`: tras el alta del tipo general, con clave y mensaje desde la
definición única y escapado `'` → `''` (`escaparLiteralSql`):

```sql
CREATE TRIGGER IF NOT EXISTS TiposAula_general_no_borrar
BEFORE DELETE ON TiposAula
WHEN OLD.clave = 'aula de estudio'
BEGIN SELECT RAISE(ABORT, 'No se puede eliminar el tipo de aula general del centro.'); END
```

Es idempotente (`IF NOT EXISTS`) y participa de la transacción de `crearEsquema`.

### Ajuste del seed (mínimo)

El disparador bloquea el `DELETE FROM TiposAula` del reemplazo total. En `sembrador.cpp`, solo el
borrado de `TiposAula` pasa a `DELETE FROM TiposAula WHERE clave <> :clave` con
`tipo_general::clave()`; el resto de tablas y el orden FK-seguro no cambian. La fila general se
conserva y el alta la reutiliza (ya lo hacía desde T1). **No** se toca el perfil (sigue con 3 tipos)
ni los recuentos ni el resto de T7.

### Archivos tocados

- `src/backend/include/backend/database/tipo_general.hpp` (nuevo).
- `src/backend/src/database/migracion.cpp` (disparador y escapado).
- `src/seed/src/sembrador.cpp` (borrado con exclusión del general).
- `test/backend/test_version_esquema.cpp` (dos casos nuevos).

### Hallazgos / desviaciones

- Ninguna funcional. El mensaje de renombrado se declara ya en el mismo sitio para que T3 solo cree
  su disparador; no se ha creado el `BEFORE UPDATE` ni se ha tocado el servicio.
- `grep` del literal: sigue **1** (`common/tipo_general.hpp`).

---

## T3 - Disparador de renombrado del tipo general

**Fecha:** 2026-11-05
**Requisito:** RF-4 (protección en cualquier vía; un `UPDATE` que no cambia nada no se bloquea) ·
RNF-6. **Plan:** §2.3 y §5.

### Rojo (tests primero)

En `test/backend/test_version_esquema.cpp`:

- `proteccion_renombradoDirecto_delGeneral_esRechazado`: cambiar solo `nombre` y cambiar solo
  `clave` fallan con el mensaje en español y la fila conserva nombre y clave.
- `proteccion_updateSoloDeFecha_delGeneral_sePermite`: actualizar solo `fecha_modificacion` pasa.
- `proteccion_updateSinCambios_sePermite`: asignar los mismos `nombre` y `clave` pasa.

```text
$ ./test_backend_version_esquema
FAIL!  : TestVersionEsquema::proteccion_renombradoDirecto_delGeneral_esRechazado()
Totals: 38 passed, 1 failed, 0 skipped, 0 blacklisted, 62ms
```

### Verde

```text
$ ./test_backend_version_esquema → 39 passed, 0 failed
$ ./test_servicio_tipos_aula    → 31 passed, 0 failed
$ ./test_seed_aplicacion        → 20 passed, 0 failed
$ ctest --preset backend        → 100% tests passed, 54/54 (13,11 s)
```

### Código

`src/backend/src/database/migracion.cpp`, tras el disparador de borrado, con la misma clave y el
mensaje fijo de `backend/database/tipo_general.hpp` (`OperacionProtegida::Renombrado`) y el mismo
`escaparLiteralSql`:

```sql
CREATE TRIGGER IF NOT EXISTS TiposAula_general_no_renombrar
BEFORE UPDATE OF nombre, clave ON TiposAula
WHEN OLD.clave = 'aula de estudio' AND
     (NEW.nombre <> OLD.nombre OR NEW.clave <> OLD.clave)
BEGIN SELECT RAISE(ABORT, 'No se puede renombrar el tipo de aula general del centro.'); END
```

`BEFORE UPDATE OF nombre, clave` no se dispara si el `UPDATE` solo toca otra columna (la fecha), y
la condición `NEW <> OLD` deja pasar la asignación de los mismos valores. Idempotente (`IF NOT
EXISTS`) y dentro de la transacción de `crearEsquema`.

### Archivos tocados

- `src/backend/src/database/migracion.cpp` (segundo disparador).
- `test/backend/test_version_esquema.cpp` (tres casos nuevos).

No se ha tocado `ServicioTiposAula` (mensajes y prioridad del servicio son T6) ni el seed.

### Hallazgos / desviaciones

- Ninguna funcional. Ningún test existente renombra el general por SQL directo: los casos de
  renombrado del servicio usan tipos especializados desde T1 y siguen en verde.
- `grep` del literal: sigue **1** (`common/tipo_general.hpp`).

---

## T4 - Predicado puro de validez de la fila y traducción del rechazo

**Fecha:** 2026-11-05
**Requisito:** RF-2 (identidad), RF-6 (predicado único), RF-7 (traducción) · RNF-1. **Plan:**
§2.4/§2.6 y §5.

### Rojo (tests primero)

Nuevo `test/backend/test_tipo_general.cpp` (`add_gtest` en `test/CMakeLists.txt`, bloque backend)
con **10 casos** y la API ya declarada en el encabezado, pero sin implementación: el binario compila
y **falla al enlazar** por `evaluar`, `traducirRechazo` y `mensajeProteccionServicio` sin definir.

```text
$ cmake --build --preset backend --parallel
test_tipo_general.cpp:(.text+…): referencia a `tipo_general::evaluar(…)` sin definir
collect2: error: ld devolvió el estado de salida 1
```

### Verde

```text
$ ./test_tipo_general        → 10 passed (5 del predicado + 5 de traducirRechazo)
$ ctest --preset backend     → 100% tests passed, 55/55 (15,18 s)
```

### API final

`src/backend/include/backend/database/tipo_general.hpp`:

```cpp
namespace tipo_general {
    enum class OperacionProtegida { Borrado, Renombrado };

    QString mensajeProteccionAlmacenamiento(OperacionProtegida); // fija, ya existente (T2/T3)
    QString mensajeProteccionServicio(OperacionProtegida, const QString& valorRecibido);

    struct Fila { int id = 0; QString nombre; QString clave; };

    enum class EstadoFila { SinTipoGeneral, VariasFilas, ClaveDistinta, Valida };
    EstadoFila evaluar(const QVector<Fila>& filas, QString* motivo = nullptr);

    struct EstadoFilaProtegida { bool existe = false; bool esGeneral = false; };
    QString traducirRechazo(OperacionProtegida operacion, bool fallo,
                            const EstadoFilaProtegida& fila, const QString& valorRecibido);
}
```

Reglas implementadas en `src/backend/src/database/tipo_general.cpp`:

- `evaluar`: cuenta las filas con `claveNormalizada(nombre) == clave()`; `0` →
  `SinTipoGeneral`, `>1` → `VariasFilas`, `1` con clave almacenada distinta → `ClaveDistinta`,
  `1` con la clave exigida → `Valida`. Independiente del orden; `motivo` en español con el conteo,
  el nombre canónico y la clave esperada (vacío en `Valida`).
- `traducirRechazo`: `""` sin fallo, con la fila inexistente o con otra fila; si es el general,
  el mensaje del servicio. Borrado:
  «No se puede eliminar el tipo de aula general del centro: no admite borrado.»; renombrado:
  «No se puede renombrar el tipo de aula general del centro: su nombre no admite cambios
  (recibido «X»).»

### Archivos tocados

- `src/backend/include/backend/database/tipo_general.hpp` (`Fila`, `EstadoFila`, `evaluar`,
  `EstadoFilaProtegida`, `traducirRechazo`, `mensajeProteccionServicio`).
- `src/backend/src/database/tipo_general.cpp` (nuevo; predicado y mensajes).
- `src/backend/CMakeLists.txt` (añade `src/database/tipo_general.cpp`).
- `test/backend/test_tipo_general.cpp` (nuevo, 10 casos).
- `test/CMakeLists.txt` (`add_gtest(test_tipo_general …)`).

### Hallazgos / desviaciones

- En `Valida`, `motivo` queda **vacío** (no «relleno»): es lo que necesita la guardia de T5 para
  distinguir el éxito y lo que el predicado documenta. Los tres estados inválidos sí llevan conteo y
  valor esperado.
- No se toca `version_esquema.cpp` (guardia, T5), ni el servicio (T6), ni el seed (T7).
- `grep` del literal: sigue **1** (`common/tipo_general.hpp`).

---

## T5 - Guardia v6: índice, dos disparadores, fila y huella

**Fecha:** 2026-11-05
**Requisito:** RF-6 (validez de la base y guardia). **Plan:** §2.5 y §5.

### Rojo (tests primero)

En `test/backend/test_version_esquema.cpp`:

- `esquemaEsperado_exigeElIndiceYLosDosDisparadores`: quitar cada objeto (índice,
  `TiposAula_general_no_borrar`, `TiposAula_general_no_renombrar`) → `false` y el motivo lo nombra.
- `esquemaEsperado_exigeExactamenteUnaFila`: 0 filas; 2 con nombre canónico (la segunda con clave
  `AULA DE ESTUDIO`); 1 fila canónica con clave distinta. Montaje **P10**: `DROP TRIGGER
  TiposAula_general_no_borrar` antes de mutar, base en memoria por caso, sin restaurar y sin vías de
  desactivación en producto.
- `esquemaEsperado_toleraObjetosDeMas`: un índice extra → `true`.
- `esquemaEsperado_noDejaTransaccionAbierta` y `esquemaEsperado_funcionaDentroDeUnaTransaccion` (**P5**).
- `huellaEsquema_noCambiaSinSubirLaMarcaDeVersion`: `huellaEsquema` pasa a incluir índice y
  disparadores.

```text
$ ./test_backend_version_esquema
FAIL!  : esquemaEsperado_exigeElIndiceYLosDosDisparadores()
FAIL!  : esquemaEsperado_exigeExactamenteUnaFila()
FAIL!  : huellaEsquema_noCambiaSinSubirLaMarcaDeVersion()
  Actual  : "15d99b8628a43a09542db1e0a443c15f52302ba893a6af107445370d9fe7ac3a"
  Expected: "eebe660c385f9802dc7b0b63abb6e7821db3ad3f2ec35982eeae55e49e9deaa0"
Totals: 41 passed, 3 failed, 0 skipped, 0 blacklisted, 85ms
```

### Verde

```text
$ ./test_backend_version_esquema → 44 passed, 0 failed
$ ctest --preset backend        → 100% tests passed, 55/55 (13,18 s)
```

`test_backend_apertura_base_datos` y `test_datos_contexto_base_datos` siguen en verde.

### Guardia implementada

`src/backend/src/database/version_esquema.cpp`:

- `INDICES_ESPERADOS = { "TiposAula_clave" }` y `DISPARADORES_ESPERADOS = {
  "TiposAula_general_no_borrar", "TiposAula_general_no_renombrar" }`, comprobados por
  `sqlite_master` (`type`/`name`). **No** se exigen `sqlite_autoindex_*` ni se comparan cuerpos.
- Orden: tablas/columnas → tablas retiradas → objetos → fila. La fila solo se evalúa si
  `TiposAula` existe y tiene `nombre` y `clave` (si falta una columna ya se informa de ella).
  `SELECT id, nombre, clave FROM TiposAula ORDER BY id` → `tipo_general::evaluar`.
- Motivos nuevos: `faltan los objetos «índice TiposAula_clave», «disparador …»` y el motivo del
  predicado (`hay 0/1/2 filas…`). Se suman a los existentes; en éxito el motivo queda vacío.
- **P5**: toda la comprobación va en una transacción (`db.transaction()`); si ya hay una abierta,
  `transaction()` devuelve `false` y se usa esa sin cerrarla. La función no escribe y solo hace
  `commit()` de la transacción que abrió ella.

`huellaEsquema` del test pasa a
`type IN ('table','index','trigger') AND name NOT LIKE 'sqlite_%' ORDER BY type, name`; las
entradas 2–5 se comentan como **históricas** (resumen de solo tablas) y la 6 se sustituye por la
huella real nueva:

```text
v6 = 15d99b8628a43a09542db1e0a443c15f52302ba893a6af107445370d9fe7ac3a
```

### Archivos tocados

- `src/backend/src/database/version_esquema.cpp` (catálogo de objetos, predicado de la fila,
  motivos y transacción de solo lectura).
- `src/backend/include/backend/database/version_esquema.hpp` (comentario de `esquemaEsperado`).
- `test/backend/test_version_esquema.cpp`: seis casos nuevos, `huellaEsquema` ampliada, entradas
  históricas comentadas y huella v6 sustituida por la real.

### Hallazgos / desviaciones

- Ninguna. No se ha tocado el servicio (T6), el seed (T7) ni la 016; `esquemaEsperado` sigue
  escribiendo **nada** (la transacción es de lectura).
- `grep` del literal: sigue **1** (`common/tipo_general.hpp`).

---

## T6 - Mensajes del catálogo: prioridad, renombrado total y traducción del rechazo

**Fecha:** 2026-11-05
**Requisito:** RF-3, RF-4, RF-7 · RNF-1. **Plan:** §2.3 (P12) y §2.6.

### Rojo (tests primero)

En `test/backend/test_servicio_tipos_aula.cpp`:

- `eliminarTipoAula_general_fallaConPrioridad`: sin dependientes y con aula y materia que lo usan,
  el mensaje habla del **tipo general**, no de «en uso» ni de la lista.
- `renombrarTipoAula_general_fallaAunqueElNombreSeaElMismo`.
- `renombrarTipoAula_general_fallaConOtraGrafia` («AULA DE ESTUDIO»).
- `eliminarTipoAula_especializadoEnUso_sigueListandoDependientes`: control renombrado del caso que
  existía con «Laboratorio» + aula + materia; sigue listando ambos.

```text
$ ./test_servicio_tipos_aula
FAIL!  : eliminarTipoAula_general_fallaConPrioridad()   (mensaje sin «general»)
FAIL!  : renombrarTipoAula_general_fallaAunqueElNombreSeaElMismo() ('!resultado.ok')
FAIL!  : renombrarTipoAula_general_fallaConOtraGrafia() (mensaje sin «general»)
Totals: 31 passed, 3 failed
```

No se añadió test de carrera (P4): la traducción se prueba en `test_tipo_general` y los disparadores
por SQL directo en T2/T3.

### Verde

```text
$ ./test_servicio_tipos_aula  → 34 passed, 0 failed
$ ./test_backend_nucleo_datos  → 20 passed, 0 failed
$ ./test_backend_gestor_pendientes → 13 passed, 0 failed
$ ctest --preset backend      → 100% tests passed, 55/55 (15,85 s)
```

### Código y mensajes

`ServicioTiposAula` gana en privado `FilaTipoActual { existe, nombre, clave }`, `leerFila(id)`,
`esFilaGeneral(fila)` y `esTipoGeneral(id)`. `esFilaGeneral` reconoce el general por el **nombre
normalizado** (`esNombreCanonico`) **o** por la **clave almacenada** normalizada (cubre la fila
escrita a mano con clave válida y otro nombre). Sin cambios de firma pública.

Textos finales (de `mensajeProteccionServicio`, P12):

```text
Borrado:    No se puede eliminar el tipo de aula general del centro: no admite borrado.
Renombrado: No se puede renombrar el tipo de aula general del centro: su nombre no admite
            cambios (recibido «<valor>»).
```

- **`eliminarTipoAula`**: la comprobación del general va **antes** de `dependientesDe` (prioridad
  RF-3). Si el `DELETE` falla, se relee la fila y `traducirRechazo` devuelve el mensaje del catálogo
  (nunca el texto crudo); si no era el general, se mantiene el error genérico.
- **`renombrarTipoAula`**: rechazo incondicional del general **antes** de validar el nombre nuevo,
  incluso con el mismo nombre o solo otra grafía. Si el `UPDATE` falla, relectura + `traducirRechazo`
  con el valor recibido; el caso `UNIQUE` conserva su mensaje de duplicado.
- La rama de traducción tras un `exec` fallido queda **cubierta unitariamente** por
  `traducirRechazo` (T4) y no se fuerza desde el servicio (P4).

### Archivos tocados

- `src/backend/include/backend/services/ServicioTiposAula.hpp` (privados nuevos).
- `src/backend/src/services/ServicioTiposAula.cpp` (relectura, general, prioridad, traducción).
- `test/backend/test_servicio_tipos_aula.cpp` (tres casos nuevos y control renombrado).

### Hallazgos / desviaciones

- Ninguna. `test_backend_nucleo_datos` y `test_backend_gestor_pendientes` siguen verdes sin tocar sus
  mensajes: no dependían del texto anterior del catálogo.
- No se toca el seed (T7) ni la 016.
- `grep` del literal: sigue **1** (`common/tipo_general.hpp`).

---

## T7 - Siembra: reutilizar, excluir del chequeo, conservar y recuento 3

**Fecha:** 2026-11-05
**Requisito:** RF-5 · RNF-2, RNF-3, RNF-6. **Plan:** §2.7 y §5.

### Rojo (tests primero)

En `test_seed_perfil.cpp`:

- `tiposDeAulaDelPerfil`: 2 tipos («Laboratorio», «Taller»), el general no se declara y las
  aulas genéricas lo usan.
- `validarPerfilAceptaElTipoGeneralDelEsquema`.
- `recuentosNuevos`: `perfil.tiposAula == 2` y `recuentosEsperados.tiposAula == 3`.

En `test_seed_aplicacion.cpp`: `aplicar_sobreBaseNueva_noInformaBaseConDatos`,
`aplicar_reutilizaElTipoGeneral`, `reemplazar_conservaElTipoGeneral`,
`aplicar_dosVeces_mismosDatosYRecuentos` (bases vacías y `--reemplazar`) y
`aplicar_recuentoTiposEsTres`.

```text
$ ./test_seed_perfil
FAIL!  : TestSeedPerfil::tiposDeAulaDelPerfil()            (esperaba 2, había 3)
FAIL!  : TestSeedPerfil::validarPerfilAceptaElTipoGeneralDelEsquema() (el perfil declaraba el general)
FAIL!  : TestSeedPerfil::recuentosNuevos()
Totals: 35 passed, 3 failed
```

Los casos de `test_seed_aplicacion` pasaban ya con el comportamiento de T1/T2. Al pasar el perfil a
2 tipos, la primera pasada del seed falló como era de esperar (`El aula debe tener un tipo de aula
(se recibió: sin tipo)`): el `idTipoAula` ya no contenía el general. Se resolvió en el código con la
búsqueda explícita por clave (ver abajo) y quedó en verde.

### Verde

```text
$ ./test_seed_perfil      → 38 passed, 0 failed
$ ./test_seed_aplicacion  → 25 passed, 0 failed
$ ctest --preset backend  → 100% tests passed, 55/55 (20,71 s)
```

### Cambios

- `src/seed/src/perfil.cpp`: `p.tiposAula = { "Laboratorio", "Taller" }`; `validarPerfil` acepta el
  general del esquema en el catálogo permitido de aulas (`catalogo.insert(tipo_general::nombre())`),
  manteniendo el rechazo de tipos no declarados y la exigencia de aula para el tipo final de cada
  materia.
- `src/seed/include/seed/perfil.hpp`: comentario del catálogo (2 declarados + general del esquema).
- `src/seed/src/docentes.cpp`: `recuentosEsperados.tiposAula = perfil.tiposAula.size() + 1`.
- `src/seed/src/sembrador.cpp`: el alta crea solo los tipos del perfil y **resuelve el id del
  general** por `SELECT id FROM TiposAula WHERE clave = :clave`; si faltara, aborta con error
  explícito en vez de sembrar sin el tipo que el cálculo necesita. El chequeo de «base con datos»,
  la reutilización y el reemplazo por exclusión ya estaban desde T1/T2 y no cambian.

### Archivos tocados

- `src/seed/src/perfil.cpp`, `src/seed/include/seed/perfil.hpp`, `src/seed/src/docentes.cpp`,
  `src/seed/src/sembrador.cpp`.
- `test/backend/test_seed_perfil.cpp` y `test/backend/test_seed_aplicacion.cpp` (helpers
  `idTipoGeneral`/`contarTipoGeneral`, catálogo por clave natural y cinco casos nuevos).

### Hallazgos / desviaciones

- `test_generacion_seed` (frontend) no menciona tipos de aula; se comprobará en la suite completa de
  T8 junto con `test_selector_generacion`. En `backend` no se compilan.
- No se toca la guardia (T5), el servicio (T6), la documentación (T8) ni la 016.
- `grep` del literal: sigue **1** (`common/tipo_general.hpp`).

---

## T8 - Documentación, supersesiones y regresión

**Fecha:** 2026-11-05
**Requisito:** RF-7 (coherencia documental) · RNF-4, RNF-7. **Plan:** §2.8 y §5.

### Estado de partida (antes de tocar nada)

```text
backend       → 100% tests passed, 55/55 (13,82 s)
full-release  → 100% tests passed, 59/59 (38,64 s)
```

### Documentación

**`docs/interfaz-frontend.md`**

- `VERSION_ESQUEMA_ACTUAL = 6` con el comentario «v6: tipo general de aula garantizado y protegido»;
  `esquemaEsperado` documentado como tablas, objetos, fila y tablas retiradas.
- Catálogo esperado: tablas/columnas **por nombre**, el índice `TiposAula_clave`, los dos
  disparadores, **exactamente una** fila del tipo general (nombre normalizado y clave «aula de
  estudio») y ausencia de tablas retiradas. El cuerpo del disparador queda como guardia de release,
  fuera de la validez.
- **P11, antes → después:**
  - «La versión **5** añade las preferencias… al catálogo de la **v4**» → «La versión **6** añade el
    tipo general de aula (fila, índice y dos disparadores) al catálogo de la **v5**»; se conserva como
    historia que la v5 añadió las preferencias a la v4.
  - «Una base con la versión anterior (**4**) **no se migra**» → **5**.
- Nota de la huella: resume **tablas, índices y disparadores**.
- §2.4.3: el catálogo nunca nace vacío; el tipo general no se elimina ni se renombra, su alta falla
  por duplicado y el mensaje prioriza «es el tipo general»; se retira la frase de «catálogo vacío».

**`docs/guias/migracion-frontend.md`**

- Cabecera y resumen: «specs 009–012 y **015**»; «**Esquema de base:** `v6`»; suite del backend 55/55.
- Tabla §1: fila de **015** (esquema v6, tipo general garantizado y protegido, **sin cambios de API**).
- §3.2: «una base **v5 o anterior** se rechaza al abrir».
- §5: backend 55/55. §6: resumen de la 015.

**`docs/constitution.md`:** no se toca (plan §2.8).

### Supersesiones (verificadas, no redactadas)

Las siete notas del planner están presentes:

```text
specs/001-base-datos-local/spec.md
specs/004-persistencia-campos-curso/spec.md
specs/008-datos-de-prueba/spec.md
specs/009-tipos-de-aula-y-eventos/spec.md
specs/010-solver-tipos-y-eventos/spec.md
specs/011-horas-academicas/spec.md
specs/012-preferencias-docente/spec.md
```

> **Matiz de verificación:** el tercer `grep` literal de `tasks.md`
> (`"015, RF-6\|015, RF-5\|(015)"`) devuelve **5** archivos, no 7, porque las notas de **011** y
> **012** citan «la 015 (RF-6)» y no el patrón `(015)`. Las notas **sí existen** en las siete (se
> comprueba con `grep -rln "Anotado por la 015" …`). No se editan las specs: la redacción es del
> planner y el implementer solo verifica.

### Verificación (las cinco órdenes)

1. `cmake --preset backend && … && ctest --preset backend` → **100%, 55/55**.
2. `cmake --preset full-release && … && ctest --preset full-release` → **100%, 59/59**.
3. `grep` de restos de versión en `docs/` → **sin restos**.
4. `grep` del literal en `src/` (sin comentarios) → **1**:
   `src/common/include/common/tipo_general.hpp:13`.
5. `grep -rln "015, RF-6\|015, RF-5\|(015)" specs/001* … 012*` → **5** (001, 004, 008, 009, 010);
   con el patrón amplio `Anotado por la 015` → **7**. Detalle en el matiz de arriba.

### Archivos tocados

- `docs/interfaz-frontend.md`
- `docs/guias/migracion-frontend.md`
- `specs/015-tipo-general-en-el-esquema/tasks.md` (T8 marcada)

Sin cambios de código ni de tests en esta tarea. `docs/constitution.md` intacto. No hay commits.

---

## Correcciones de la validación (V1–V3)

**Fecha:** 2026-11-05. Correcciones H1, H2 y H6. Sin tocar `spec.md`, `plan.md`, `estado.md` ni
`tasks.md` (los actualiza el planner).

### V1 (H1) — el fallo de creación explica que no queda esquema parcial

- `src/backend/src/database/version_esquema.cpp` (`qCritical` del rollback):
  «La creación del esquema falló y se revirtió: no quedó esquema parcial. El archivo puede
  descartarse y crearse de nuevo (con respaldo obligatorio).»
- `src/backend/src/database/apertura_base_datos.cpp` (`detalle` de `FalloApertura` al crear, y su
  `qCritical`): «No se pudo crear el esquema de la base «<ruta>»: no quedó esquema parcial. El archivo
  puede descartarse y crearse de nuevo (con respaldo obligatorio).»
- Test nuevo en `test/backend/test_apertura_base_datos.cpp`:
  `abrir_falloDeCreacion_explicaQueNoQuedaEsquemaParcial`, con `Opciones.crear` que devuelve `false`;
  comprueba «no quedó esquema parcial», «descartarse», «crearse de nuevo» y «respaldo».

### V2 (H2) — el test de «no queda esquema parcial» prueba el rollback real

Reescrito `crearEsquema_fallido_noDejaEsquemaParcial` en `test/backend/test_version_esquema.cpp`:

- Precrea `ConfiguracionHoraria (id INTEGER PRIMARY KEY CHECK(id = 1))` **sin**
  `duracion_minutos`, así el `INSERT OR IGNORE` de `migracion.cpp` falla **después** de crear
  `TiposAula`, su índice, la fila canónica, los dos disparadores y el resto de tablas.
- Tras el fallo comprueba: `user_version == 0`; la `ConfiguracionHoraria` preexistente se queda;
  **no existen** `TiposAula`, `Aulas` ni `Cursos` (la consulta a `TiposAula` falla porque la tabla se
  revirtió); y `sqlite_master` no registra ninguno de los dos disparadores.

### V3 (H6) — el `detalle` de apertura documenta objetos y fila

`docs/interfaz-frontend.md` (§2.1): el `detalle` nombra ahora también los **objetos exigidos** que
faltan (índice único y los dos disparadores), la **fila** del tipo general cuando no cumple el
predicado (sin tipo, varias filas o clave almacenada distinta) y las tablas retiradas; y explica que
un fallo de creación no deja esquema parcial y que el archivo puede descartarse y recrearse con
respaldo.

### Verificación

```text
backend       → 100% tests passed, 55/55 (26,40 s)
full-release  → 100% tests passed, 59/59 (38,32 s)
```

`test_backend_version_esquema`: 44/44. `test_backend_apertura_base_datos`: 16/16.

---

## Cierre (2026-11-05)

**VEREDICTO: APROBADO (2026-11-05).** La 015 queda **implementada y cerrada**.

| Tarea | Qué dejó |
|---|---|
| T1 | Definición única (`common/tipo_general.hpp`), alta del tipo general en la creación (atómica e idempotente) y marca de esquema **v6** |
| T2 | Disparador `TiposAula_general_no_borrar` |
| T3 | Disparador `TiposAula_general_no_renombrar` (no bloquea un `UPDATE` que no cambia nombre ni clave) |
| T4 | Predicado puro `evaluar` y `traducirRechazo` |
| T5 | Guardia v6: índice y dos disparadores por nombre, fila, retiradas, transacción de solo lectura y huella ampliada |
| T6 | Mensajes del catálogo (prioridad, renombrado total, traducción) |
| T7 | Siembra: reutiliza, excluye del chequeo, conserva en el reemplazo y recuento 3 desde la base |
| T8 | Documentación de versión y guardia + supersesiones (7 specs) + regresión |

**Correcciones de la validación (V1–V3), aplicadas.** V1: mensaje del fallo de creación («no quedó esquema
parcial; el archivo puede descartarse y crearse de nuevo con respaldo») en `version_esquema.cpp` y
`apertura_base_datos.cpp`, con su test. V2: `crearEsquema_fallido_noDejaEsquemaParcial` prueba el rollback
real. V3: el `detalle` de apertura documenta objetos exigidos, la fila y las tablas retiradas.

**Verificación final**

```text
backend       → 100% tests passed, 55/55
full-release  → 100% tests passed, 59/59
```

**Sin commits:** todo el trabajo quedó en el árbol de trabajo; no se creó ninguna rama ni se empujó nada.
La spec pasa a `implementada (2026-11-05)` y la fase a **cerrada**.
