# Plan 015 — El tipo general de aula como parte del esquema

Base: **`bb1767af343c108b20de2b2b4de7ebe1d7658c2f`** (HEAD de `feat/Motor`, árbol limpio, esquema vigente **v5**). Spec aprobada el **2026-11-05**.

> **Alcance.** Este plan implementa la **mitad 1** (garantía estructural) de la antigua 015: fila del tipo
> general en la creación, definición única del literal, prohibición de borrado y de renombrado (con
> protección en el almacenamiento), convivencia con los datos de prueba, **validez de la base** y **guardia
> v6**, documentación de versión/guardia y supersesiones.
> **Fuera:** los momentos de comprobación, el frontend sin respaldo silencioso, el mensaje de V18, la huella
> de datos y la documentación de la API —todo eso es la **016**—.

## 0. Reglas de trabajo

- **Tests primero** en cada tarea: caso(s) en rojo → código → verde. `add_gtest`/`add_qtest` en
  `test/CMakeLists.txt`.
- Una tarea por commit; **ninguna tarea cierra con el árbol sin compilar ni con tests rojos** ajenos.
- Una tarea se marca **solo** cuando pasa su verificación (comando de la tarea) y, en T8, la suite completa.
- **El plan no toca la 016.** Si algo de este plan afectara al frontend, se limita al **literal** (T1); el
  cambio de comportamiento del atajo es de la 016.

## 1. Inventario de cambios

**Nuevos**
- `src/common/include/common/tipo_general.hpp` — **definición única** del literal y de la normalización.
- `src/backend/include/backend/database/tipo_general.hpp` — estructuras y predicado de la fila.
- `src/backend/src/database/tipo_general.cpp` — implementación del predicado y mensajes del catálogo.
- `test/backend/test_tipo_general.cpp` — pruebas del predicado puro (gtest).

**Modificados**
- `src/backend/CMakeLists.txt` — enlaza `common` (PUBLIC) y añade `src/database/tipo_general.cpp`.
- `src/backend/src/database/migracion.cpp` — alta del tipo general + dos disparadores.
- `src/backend/include/backend/database/version_esquema.hpp` — `VERSION_ESQUEMA_ACTUAL = 6` y comentario.
- `src/backend/src/database/version_esquema.cpp` — catálogo de objetos, predicado de la fila, motivos y
  mensaje del rollback (no queda esquema parcial).
- `src/backend/src/database/apertura_base_datos.cpp` — **mensaje del fallo de creación** (no queda
  esquema parcial; descartar el archivo y crear una base nueva con respaldo).
- `src/backend/include/backend/services/ServicioTiposAula.hpp` / `src/services/ServicioTiposAula.cpp` —
  `claveTipo` delegada, `esTipoGeneral`, mensajes y prioridad, traducción del rechazo.
- `src/backend/src/solver/config/contrato_solver.cpp` — `tipoGeneralAula()` y `claveTipo` delegados.
- `src/seed/include/seed/perfil.hpp`, `src/seed/src/perfil.cpp`, `src/seed/src/docentes.cpp`,
  `src/seed/src/sembrador.cpp` — perfil sin el general, reutilización, exclusión y conservación.
- `src/frontend/src/services/tipos_aula_ui.hpp` — `nombreTipoGeneral()` delegado (sin cambiar la caída; es
  de la 016).
- `test/CMakeLists.txt` + los tests que se listan en §6.
- `docs/interfaz-frontend.md`, `docs/guias/migracion-frontend.md` y notas de supersesión en
  `specs/001`, `004`, `008`, `009`, `010`, `011`, `012`.
- `specs/015-tipo-general-en-el-esquema/tasks.md`.

## 2. Diseño

### 2.1 Definición única del literal y de la normalización

**Dónde vive:** `src/common/include/common/tipo_general.hpp`. `common` es la biblioteca **INTERFACE** del
proyecto, sin dependencias de negocio y por debajo de todos los módulos; es el único sitio que no invierte
dependencias (datos/base → backend/solver → datos/seed → frontend).

```cpp
namespace tipo_general {
    // ÚNICA aparición del literal en el código de producción (015 RF-2).
    inline QString nombre()            { return QStringLiteral("Aula de estudio"); }
    // ÚNICO algoritmo de clave de tipo (recorte + plegado Unicode).
    inline QString claveNormalizada(const QString& n) { return n.trimmed().toCaseFolded(); }
    inline QString clave()             { return claveNormalizada(nombre()); }
    inline bool esNombreCanonico(const QString& n) { return claveNormalizada(n) == clave(); }
}
```

**Cómo lo consumen los demás** (todos por esta definición; ninguno repite el literal):

| Punto | Antes | Después |
|---|---|---|
| `contrato_solver::tipoGeneralAula()` | `QStringLiteral("Aula de estudio")` | `tipo_general::nombre()` |
| `contrato_solver::claveTipo` | `trimmed().toCaseFolded()` | `tipo_general::claveNormalizada` (envoltorio conservado) |
| `ServicioTiposAula::claveTipo` | `trimmed().toCaseFolded()` | `tipo_general::claveNormalizada` (envoltorio conservado) |
| Alta/DDL y disparadores | — | `tipo_general::nombre()` / `clave()` |
| Guardia (predicado de la fila) | — | `tipo_general::clave()` |
| Mensajes del catálogo | texto propio | `backend/database/tipo_general.hpp` |
| Seed (`perfil.cpp`) | `kTipoGenerico` (`const char*`) | `tipo_general::nombre()` en T1 (el catálogo de T7 no cambia aquí) |
| Frontend (`tipos_aula_ui.hpp`) | `QStringLiteral(...)` | `tipo_general::nombre()` |

- **CMake:** `target_link_libraries(backend PUBLIC Qt6::Core Qt6::Sql common)`. `seed` (→ `datos` → `backend`)
  y `frontend` (→ `backend`) lo heredan; `app` ya enlaza `common`.
- **Algoritmo de normalización:** se **unifica** en `common` y los dos `claveTipo` pasan a delegar
  (firmas intactas: cero cambios en los ~40 puntos de llamada). No se renombra nada.
- **Criterio ejecutable de la definición única:** en el **código de producción** (`src/`), la cadena
  `Aula de estudio` aparece **una sola vez**, en `common/tipo_general.hpp`. El criterio **excluye
  comentarios** y artefactos de `build*/`; fuera del alcance: documentación, tests y ejemplos.
  ```bash
  grep -rn "Aula de estudio" src --include='*.hpp' --include='*.cpp' | grep -v "/build" \
    | grep -vE '^[^:]+:[0-9]+:\s*(//|/\*|\*)'
  ```
  Se ejecuta en **T1** (debe dar **1**) y se **repite en T8** para comprobar que sigue en 1.
- **Barrido de literales y comentarios (P1).** Hoy hay **9** coincidencias en `src/`: **3 literales**
  (`contrato_solver.cpp:38`, `perfil.cpp:44`, `tipos_aula_ui.hpp:24`) y **6 comentarios**
  (`contrato_solver.hpp:82`, `perfil.cpp:309`, `perfil.hpp:67`, `perfil.hpp:111`, `perfil.hpp:115`,
  `tipos_aula_ui.hpp:9`). **T1**:
  1. sustituye los **tres** literales por la definición única (el de `perfil.cpp` es una línea; su
     comportamiento no cambia en T1 —el perfil sigue declarando los 3 tipos— y T7 es quien quita el
     general del catálogo declarado);
  2. **reescribe los 6 comentarios** para que no contengan la cadena (p. ej. «el tipo general»,
     «el general, «Laboratorio» y «Taller»»);
  3. convierte el campo `tipo` del struct local `kAulas` de `perfil.cpp` de `const char*` a `QString`
     (**P7**), porque ahora recibe `tipo_general::nombre()`.

### 2.2 Alta en la creación de la base

En `Migracion::aplicarPaso` (`src/backend/src/database/migracion.cpp`), **después** de crear `TiposAula` y
su índice único y **antes** de `Cursos`:

1. `INSERT OR IGNORE INTO TiposAula (nombre, clave) VALUES (?, ?)` con `tipo_general::nombre()` y
   `tipo_general::clave()` enlazados (`QSqlQuery::bindValue`). Idempotente: si ya existe la clave, no hace
   nada (el índice único frena el duplicado).
2. Creación de los dos disparadores (§2.3).

La sentencia está dentro de la **transacción única** de `VersionEsquema::crearEsquema` (ya existente), que
fija `PRAGMA user_version` al final. Camino único: `DatabaseManager::initialize` → `runMigrations` →
`crearEsquema`, y `AperturaBaseDatos` → `crearEsquema`; **no hay otra ruta de creación**.

**Fallo a medias.** Si cualquier paso falla, `crearEsquema` hace `rollback`: **no queda esquema parcial**
(el DDL de SQLite es transaccional) y `user_version` queda en **0**. El archivo puede quedar en disco con
la versión 0; `VersionEsquema::decidirApertura(true, 0, 6)` devuelve `FalloEsquema`, así que **no se
reintenta sobre ese archivo**: se descarta con respaldo (`Respaldo::descartarBaseYCrearNueva`) o se borra
desde el diálogo de recuperación. El `qCritical` de `crearEsquema` añade el texto «no quedó esquema
parcial; el archivo puede descartarse y crearse de nuevo» (el mensaje de usuario es de la 016).

### 2.3 Disparadores (nombres estables y texto)

| Objeto | Tipo | Definición |
|---|---|---|
| `TiposAula_general_no_borrar` | `BEFORE DELETE` | `WHEN OLD.clave = '<clave>'` → `RAISE(ABORT, …)` |
| `TiposAula_general_no_renombrar` | `BEFORE UPDATE OF nombre, clave` | `WHEN OLD.clave = '<clave>' AND (NEW.nombre <> OLD.nombre OR NEW.clave <> OLD.clave)` → `RAISE(ABORT, …)` |

```sql
CREATE TRIGGER IF NOT EXISTS TiposAula_general_no_borrar
BEFORE DELETE ON TiposAula
WHEN OLD.clave = '<clave>'
BEGIN
    SELECT RAISE(ABORT, 'No se puede eliminar el tipo de aula general del centro.');
END;
```

- **`CREATE TRIGGER` no admite parámetros**: el valor se **interpola** desde
  `tipo_general::clave()` con un escapado defensivo (`'` → `''`). No hay literales duplicados: el valor
  sale de la definición única.
- **Texto en español** (RNF-1) y **dos variantes declaradas (P12)**, porque el `RAISE` es **estático**:
  1. **fija, para el disparador** — `tipo_general::mensajeProteccionAlmacenamiento(Operacion)`, sin valor
     recibido («No se puede eliminar el tipo de aula general del centro.» / «No se puede renombrar el tipo
     de aula general del centro.»);
  2. **con el valor recibido, para el servicio** —
     `tipo_general::mensajeProteccionServicio(Operacion, valorRecibido)` («… es el tipo general del centro y
     no admite borrado.» / «… su nombre no admite cambios (recibido «X»).»).
  **Solo la del servicio** cumple el mensaje con valor de RNF-1; la del almacenamiento es deliberadamente
  corta y no se usa en las respuestas del servicio. Las cuatro se construyen **con
  `tipo_general::nombre()`** (interpolado), de modo que ninguna repite el literal: también los motivos de la
  guardia.
- **`BEFORE UPDATE OF nombre, clave`**: un `UPDATE` que no toque ninguna de las dos columnas **no dispara**
  el trigger (caso de la fecha de modificación). La condición `NEW <> OLD` es la segunda barrera si alguien
  escribe un `UPDATE` sobre esas columnas sin cambiar el valor.
- Los disparadores son obligatorios para la validez (§2.5), así que no se pueden crear «de más» ni omitir.

### 2.4 Predicado de validez de la fila (puro)

`src/backend/include/backend/database/tipo_general.hpp`:

```cpp
namespace tipo_general {
struct Fila { int id = 0; QString nombre; QString clave; };
enum class EstadoFila { Valida, SinTipoGeneral, VariasFilas, ClaveDistinta };
/// Puro y determinista: no toca la base, no depende del reloj ni del orden de entrada.
EstadoFila evaluar(const QVector<Fila>& filas, QString* motivo = nullptr);
}
```

Reglas (formulación única de la spec, L. 120/251/417/447):

1. cuenta las filas cuyo `claveNormalizada(nombre) == clave()`.
2. `0` → `SinTipoGeneral`; `>1` → `VariasFilas`; `1` con `fila.clave != clave()` → `ClaveDistinta`;
   `1` con la clave correcta → `Valida`.
3. `motivo` en español con el conteo y el valor esperado (p. ej. «hay 2 filas cuyo nombre normalizado es
   «Aula de estudio»; debe haber exactamente una»).

No hay funciones con el reloj como parámetro en toda la spec: **ninguna** decisión de este plan depende de
la hora. (El respaldo del seed ya recibe `ahora` y no se toca.)

### 2.5 Guardia v6

**`version_esquema.hpp`**: `VERSION_ESQUEMA_ACTUAL = 6` con el comentario de la v6 (tipo general garantizado
y protegido; guardia con índice, dos disparadores y la fila).

**`version_esquema.cpp`**:

- `TABLAS_ESPERADAS`, `TABLAS_RETIRADAS` y `COLUMNAS_*`: **sin cambios** (las retiradas siguen invalidando).
- Nuevos `INDICES_ESPERADOS = { "TiposAula_clave" }` y
  `DISPARADORES_ESPERADOS = { "TiposAula_general_no_borrar", "TiposAula_general_no_renombrar" }`;
  comprobación por nombre con
  `SELECT name FROM sqlite_master WHERE type = ? AND name = ?` (`index` / `trigger`).
  **No se comparan cuerpos** y **no se exigen** los `sqlite_autoindex_*`.
- Fila: `SELECT id, nombre, clave FROM TiposAula ORDER BY id` → `tipo_general::evaluar(...)`. Si la lectura
  falla, se devuelve `false` con motivo («no se pudo leer el catálogo de tipos de aula»). **Orden de las
  comprobaciones:** primero tablas/columnas, después objetos y al final la fila; si a `TiposAula` le faltan
  `nombre` o `clave`, **no se evalúa la fila** (ya se informa la columna faltante y evaluarla taparía el
  motivo real, que es lo que comprueban los tests de columnas).
- Motivos: se añaden a las piezas existentes —`faltan los objetos «índice …», «disparador …»` y el motivo
  del predicado—. Los objetos de más **no** se miran (tolerancia de RF-6).
- **Consistencia (P5):** toda la comprobación —tablas/columnas, `sqlite_master` y la fila— se ejecuta en
  **una transacción de solo lectura** (`db.transaction()` … `commit()`), de modo que no haya TOCTOU entre
  las consultas. Si ya hay una transacción abierta (`db.transaction()` devuelve `false`), se ejecuta dentro
  de ella sin abrir otra: la instantánea la da la externa. `esquemaEsperado` **no escribe** nunca.
  Pruebas: `esquemaEsperado_noDejaTransaccionAbierta` y `esquemaEsperado_funcionaDentroDeUnaTransaccion`.
- `decidirApertura` no cambia: una base con versión ≠ 6 sigue siendo `FalloEsquema`.

**Huella congelada (guardia de release, `test/backend/test_version_esquema.cpp`)**: la función
`huellaEsquema` pasa de `type='table'` a `type IN ('table','index','trigger')` (nombre + `sql` normalizado,
orden por tipo y nombre, excluyendo `sqlite_%`). Se registra la huella **v6**. Las entradas 2–5 se conservan
como **históricas** (se calcularon con el resumen de solo tablas) y se comentan como tales: el test solo
compara la de la versión vigente.
> La huella **no** forma parte de la validez (spec, RF-6): es anti-regresión del DDL. El cuerpo del
> disparador entra en la huella, no en la guardia.

### 2.6 Servicio del catálogo

`ServicioTiposAula` gana un ayudante privado:

```cpp
struct FilaTipoActual { bool existe; QString nombre; QString clave; };
FilaTipoActual leerFila(int id) const;
/// Es el general si el nombre normalizado lo es o si la clave almacenada lo es
/// (cubre la fila escrita a mano con una clave válida y otro nombre).
bool esTipoGeneral(int id) const;
```

- **Ayudante de traducción puro (P4)**, en `backend/database/tipo_general.hpp` y probado en
  `test_tipo_general` (sin base de datos):
  ```cpp
  enum class OperacionProtegida { Borrado, Renombrado };
  struct EstadoFilaProtegida { bool existe; bool esGeneral; };
  /// Puro: `""` si no hay nada que traducir; si el fallo es de la fila general, el mensaje del catálogo.
  QString traducirRechazo(OperacionProtegida op, bool fallo,
                          const EstadoFilaProtegida& fila, const QString& valorRecibido);
  ```
- **`eliminarTipoAula(id)`**: si `esTipoGeneral(id)` → error
  «No se puede eliminar el tipo de aula general del centro: no admite borrado.» **antes** de mirar
  dependientes (prioridad de RF-3). Si no, la lógica actual. Si el `DELETE` falla, **se relee la fila** y se
  usa `traducirRechazo(Borrado, true, estado, "")`.
- **`renombrarTipoAula(id, nombre)`**: si `esTipoGeneral(id)` → error **siempre**, incluso si el nombre
  recibido es idéntico, con el mensaje del servicio (incluye el valor recibido). Si el `UPDATE` falla, misma
  relectura y `traducirRechazo(Renombrado, true, estado, nombre)`.
- **Alcance de la protección (P4):** el disparador cubre **solo las escrituras directas** (probadas por SQL
  directo en T2/T3); el servicio cubre su propio camino con la comprobación previa. La rama de traducción
  del servicio **no se fuerza desde un test de servicio** (sería un test de una carrera artificial): se
  prueba **unitaria** sobre `traducirRechazo`.
- `claveTipo` delega en `tipo_general::claveNormalizada` (§2.1). `crearTipoAula` no cambia: el alta con el
  nombre del general ya falla por duplicado (clave repetida).
- **No hay cambios de firma ni de DTO** (RNF-4 de la spec / D-7).

### 2.7 Siembra

- **`perfil.cpp`**: `p.tiposAula = { "Laboratorio", "Taller" }`; se elimina `kTipoGenerico` y las aulas y
  `tipoFinalDeMateria` usan `tipo_general::nombre()`. `validarPerfil` valida contra el catálogo
  **declarado + el general** (el general lo pone el esquema).
- **`docentes.cpp`**: `recuentosEsperados.tiposAula = perfil.tiposAula.size() + 1` → **3**.
- **`sembrador.cpp`**:
  - `contarFilasDeDominio`: para `TiposAula`, `SELECT COUNT(*) FROM TiposAula WHERE clave <> ?` con la
    clave del general → la fila no convierte la base en «con datos».
  - Alta completa: crea solo los tipos del perfil y resuelve el id del general con
    `SELECT id FROM TiposAula WHERE clave = ?` (error si falta: la base no sería válida).
  - Reemplazo total: `DELETE FROM TiposAula WHERE clave <> ?` en vez de `DELETE FROM TiposAula`; **no** se
    desactiva ningún disparador (RNF-6 de la spec).
  - `leerRecuentos` no cambia: cuenta las filas del catálogo (3) tras el commit.
- **`perfil.hpp`**: comentarios del catálogo (2 tipos declarados + el general del esquema).

### 2.8 Documentación y supersesiones

- `docs/interfaz-frontend.md`: `VERSION_ESQUEMA_ACTUAL = 6` (y su comentario); la descripción del catálogo
  esperado pasa a «tablas, columnas, el índice nombrado, los dos disparadores y la fila del tipo general»;
  la nota de la guardia anti-regresión dice que el resumen incluye tablas, índices y disparadores.
- `docs/guias/migracion-frontend.md`: «**Esquema de base:** `v6`», la nota de «bases anteriores no abren»
  pasa a «v5 o anterior» y se añade la fila de la **015** (esquema v6, tipo general garantizado y
  protegido; **sin cambios de API**).
- `docs/interfaz-frontend.md`, además de la constante: la frase «La versión **5** añade las preferencias
  (`Preferencia_Profesor`) al catálogo de la **v4**» pasa a describir la **v6** (tipo general garantizado y
  protegido) sobre la **v5**, y «Una base con la versión anterior (**4**) **no se migra**» pasa a **5**
  (**P11**). El `grep` de T8 las cubre.
- **`docs/constitution.md`: no se toca.** §5 ya protege «tipos de aula»; §7.3/§7.4 no describen el esquema
  físico y la regla del aula de referencia no cambia. Se deja constancia aquí para que el implementer no lo
  edite.
- **Notas de supersesión: las redacta el planner en esta fase (P6); el implementer solo verifica.** Quedan
  aplicadas en **001, 004, 008, 009, 010, 011 y 012**, una por spec justo bajo la cabecera, con fecha
  **2026-11-05**. **Criterio de verificación (H5):** todas llevan el texto `Anotado por la 015` y
  `grep -rln "Anotado por la 015" specs/001* specs/004* specs/008* specs/009* specs/010* specs/011* specs/012*`
  devuelve **siete archivos**. Texto comprometido:
  - **001** — «El esquema esperado ya no se describe solo como tablas y columnas: incluye el índice único,
    los dos disparadores y la fila del tipo general (015, RF-6).»
  - **004 (RF-6 y criterio)** — «Se añaden a la comprobación el índice, los dos disparadores y la fila del
    tipo general (015, RF-6). Se mantiene la exigencia de ausencia de las tablas retiradas.»
  - **008 (definición «Base vacía», RF-10 y RF-12)** — «La fila del tipo general no cuenta como dato de
    dominio; el recuento de tipos es 3 y se lee de la base; el perfil no declara «Aula de estudio» (015,
    RF-5).»
  - **009 (RF-5, definición «Esquema esperado», RF-9 y RF-10)** — «El tipo general no se renombra ni se
    elimina, y el alta con su nombre falla por duplicado; el esquema esperado incluye índice, disparadores y
    la fila; la siembra reutiliza el tipo general (015).»
  - **010 (nota del 2026-10-26 y definición de «tipo general»)** — «Quedan supersedidas la parte de «sin
    cambio de esquema» y la de «el nombre que debe usar la semilla»: la existencia y el nombre los garantiza
    la base y la siembra los reutiliza. Se mantienen la convención, la regla del aula de referencia y el
    contrato 2.0 (015).»
  - **011 y 012** — «Su marca de esquema (v4 y v5) y la descripción del catálogo esperado pasan a la v6 y
    al predicado de la 015 (RF-6); se mantiene la política sin migración.»
- **Notas de supersesión** (una por spec, justo bajo la cabecera, con la fecha 2026-11-05):
  - **001** — el esquema esperado ya no es solo tablas/columnas.
  - **004 (RF-6 y criterio)** — se añaden índice, disparadores y la fila; **se mantiene** la ausencia de
    tablas retiradas.
  - **008 (definición «Base vacía», RF-10 y RF-12)** — la fila no cuenta como dato; recuento 3 leído de la
    base; el perfil no declara «Aula de estudio».
  - **009 (RF-5, definición «Esquema esperado», RF-9, RF-10)** — el general no se renombra ni se elimina;
    la siembra lo reutiliza.
  - **010 (nota del 2026-10-26 y definición de «tipo general»)** — «sin cambio de esquema» y «nombre que
    debe usar la semilla» quedan supersedidos; se mantienen convención, regla y contrato 2.0.
  - **011 y 012** — su marca de esquema (v4, v5) y su descripción del catálogo pasan a v6 y al predicado.

## 3. Funciones puras y deterministas

| Función | Entradas | Salida | Reloj |
|---|---|---|---|
| `tipo_general::claveNormalizada` | `QString` | `QString` | no |
| `tipo_general::esNombreCanonico` | `QString` | `bool` | no |
| `tipo_general::evaluar` | `QVector<Fila>` | `EstadoFila` + motivo | no |
| `ServicioTiposAula::claveTipo` (envoltorio) | `QString` | `QString` | no |

`evaluar` es independiente del orden de entrada (cuenta y, si hay varias, informa del número). No hay
ninguna función con el tiempo como parámetro en esta spec.

## 4. Decisiones y alternativas descartadas

1. **La definición vive en `common`**, no en `backend/database` ni en el solver. Descartado: ponerla en
   `backend/database` haría que el solver dependiera de la capa de base; ponerla en el solver obligaría a
   la base y a la interfaz a depender del solver. `common` es la única capa inferior a todos.
2. **Un solo literal y la clave derivada** (`clave = claveNormalizada(nombre)`), no dos constantes. Descartado:
   declarar a mano `"aula de estudio"` junto al nombre permite que se desincronicen.
3. **Normalización unificada con envoltorios**: los dos `claveTipo` delegan y conservan su firma. Descartado:
   borrar uno y renombrar llamadas (≈40 sitios) no aporta y arriesga el diff.
4. **Disparadores con nombre estable y condición por `OLD.clave`**. Descartado: condición por `id` (el id no
   es estable entre bases y no identifica el rol) y `AFTER` (el borrado ya habría ocurrido).
5. **Texto de los disparadores interpolado con escapado** porque `CREATE TRIGGER` no admite parámetros.
   Descartado: dejar los mensajes solo en el servicio (una escritura directa vería un error sin explicación).
6. **Traducción del rechazo releyendo la fila**, no analizando el texto del error. Descartado: parsear
   `lastError()` acopla el código al formato del driver.
6-bis. **La traducción es un ayudante puro** (`traducirRechazo`) probado unitariamente, y **el disparador
   cubre solo las escrituras directas** (**P4**). Descartado: forzar la carrera desde un test de servicio
   (la comprobación previa hace que la rama sea inalcanzable y el test solo podría simularla artificialmente).
6-ter. **La guardia corre en una transacción de solo lectura** (**P5**). Descartado: tres consultas sueltas
   (TOCTOU entre `sqlite_master` y la fila) y una consulta única (no se pueden unir objetos y filas sin
   perder el motivo por objeto).
7. **La huella del test incluye índice y disparadores (cuerpo incluido)** como guardia de release; la
   **validez** no los compara. Descartado: dejar la huella en tablas (la v6 no se distinguiría de la v5 y
   el incidente del 2026-10-17 volvería a ser posible).
8. **El seed resuelve el id del general por clave**, no por nombre visible. Descartado: por nombre haría que
   una fila manipulada (nombre canónico, clave mala) pasara el alta.
9. **Borrado por exclusión en el reemplazo** (`WHERE clave <> ?`), no desactivando disparadores. Descartado:
   desactivar la protección desde el producto está prohibido (RNF-6 de la spec).

## 5. Trazabilidad RF → tarea → verificación

| RF | Tarea(s) | Verificación |
|---|---|---|
| **RF-1** existencia en la creación, inseparable e idempotente | T1 | `test_backend_version_esquema`: `crearEsquema_dejaElTipoGeneral`, `crearEsquema_esIdempotente`, `crearEsquema_fallido_noDejaEsquemaParcial`; `test_servicio_aula`/`test_servicio_materias`: alta con el general en base nueva |
| **RF-2** identidad por nombre, unicidad de la clave, duplicado, **definición única** | T1 (literal), T4 (predicado), T5 (guardia) | `test_tipo_general`; `test_servicio_tipos_aula.crearTipoAula_duplicadoDelGeneral_falla`; `grep` de la cadena en `src/` = 1 |
| **RF-3** borrado prohibido por cualquier vía, prioridad del mensaje, traducción | T2, T4 (traducción pura), T6 | `test_servicio_tipos_aula.eliminarTipoAula_*`; `proteccion_borradoDirecto_delGeneral_esRechazado`; `test_tipo_general.traducirRechazo_*`; `test_backend_version_esquema.esquemaEsperado_*` |
| **RF-4** renombrado prohibido, `UPDATE` sin cambios no bloqueado, servicio rechaza todo | T3, T4, T6 | `test_servicio_tipos_aula.renombrarTipoAula_*`; `proteccion_renombradoDirecto_delGeneral_esRechazado`; `test_tipo_general.traducirRechazo_*`; caso de la fecha |
| **RF-5** siembra: reutilizar, excluir del chequeo, conservar, recuento 3, doble aplicación | T7 | `test_seed_aplicacion`; `test_seed_perfil` |
| **RF-6** validez (índice, dos disparadores, fila, sin tablas retiradas), v6, huella | T4, T5, T1 (marca) | `test_backend_version_esquema` (`esquemaEsperado_*`, huella v6, `esquemaEsperado_noDejaTransaccionAbierta`, `esquemaEsperado_funcionaDentroDeUnaTransaccion`) |
| **RF-7** servicios sin cambios de firma, mensajes en español, documentación y supersesiones | T6, T8 | `test_servicio_tipos_aula`; `test_backend_contrato_interfaz`; `docs/*` y notas de specs |
| **RNF-1** mensajes con entidad, valor, causa y acción | T2, T3, T6 | aserciones sobre `mensajeError` de los casos del general |
| **RNF-2** protección de datos (sin borrado sin confirmación) | T7, T8 | `test_seed_aplicacion` (respaldo del reemplazo) y revisión de que no se añade ninguna escritura nueva |
| **RNF-3** determinismo e idempotencia | T1, T7 | `crearEsquema_esIdempotente`, `test_seed_aplicacion` (dos aplicaciones) |
| **RNF-4** alcance mínimo (sin cambios de contrato/API) | T1, T6, T8 | `test_backend_contrato_interfaz`, `test_contrato_solver`, `test_cargador_configuracion` en verde |
| **RNF-5** una sola definición del literal | T1, T8 | `grep` = 1 en `src/` (sin comentarios), repetido en T8 |
| **RNF-6** protección no desactivable desde el producto | T2, T3, T7 | `grep -n "DROP TRIGGER\|PRAGMA writable_schema"` = 0; reemplazo por exclusión |
| **RNF-7** trazabilidad | todas | este apartado + `tasks.md` |

## 6. Tests existentes que cambian (y por qué)

| Test | Cambio | Motivo |
|---|---|---|
| `test_backend_version_esquema.cpp` | `VERSION_ESQUEMA_ACTUAL == 5` → **6**; `huellaEsquema` incluye índice y disparadores; `HUELLAS_CONOCIDAS` + v6; nuevos casos de fila/objetos/retiradas | La marca sube y la guardia cambia |
| `test_servicio_tipos_aula.cpp` | `listarTiposAula_vacio` → `listarTiposAula_incluyeElGeneral`; los 5 `crearTipoAula("Aula de estudio")` pasan a **reutilizar** el tipo (helper `idTipoGeneral`); `contarTipos()` +1; `renombrarTipoAula_propagaAAulasYMaterias_sinReescribir`, `eliminarTipoAula_enUsoPorAula_falla` y `eliminarTipoAula_enUsoPorAulaYMateria_listaAmbos` pasan a usar un tipo **especializado** («Laboratorio»/«Taller») y el general se reserva para los casos de prioridad de T6 (**P3**) | El catálogo ya no nace vacío, el alta del general es duplicada y el general no se puede renombrar ni eliminar |
| `test_servicio_carga_horaria.cpp`, `test_servicio_profesor.cpp` | `crearTipoAula("Aula de estudio")` → buscar el id del general (**P2**) | Evitar el duplicado |
| `test_servicio_aula.cpp` / `test_servicio_materias.cpp` | `crearAula_catalogoVacio_falla` / `crearMateria_catalogoVacio_falla` → `..._sinTipoValido_falla` + caso positivo con el general | «Catálogo vacío» es inalcanzable en una base nueva (RF-1) |
| `test_nucleo_datos.cpp`, `test_cascada.cpp`, `test_gestor_pendientes.cpp`, `test/datos/test_contexto_base_datos.cpp` | `crearTipoAula("Aula de estudio")` → buscar el id del general (o crear «Laboratorio»/«Taller») | Evitar el duplicado |
| `test_seed_perfil.cpp` | `perfil.tiposAula` pasa a **2**; la validación acepta el general del esquema; `recuentosEsperados.tiposAula == 3` | El perfil deja de declarar el general |
| `test_seed_aplicacion.cpp` | recuentos 3 (sin cambios); «base con datos» excluye la fila; reemplazo conserva el general (mismo id); doble siembra | RF-5 |
| `test_contrato_interfaz.cpp`, `test_apertura_base_datos.cpp`, `test_contexto_base_datos.cpp:101/116` | **sin cambios** (usan `VERSION_ESQUEMA_ACTUAL` simbólico) | Comprobado al planificar |
| `test_nucleo_datos.cpp` (**solo la línea 96**) | **sin cambios** esa línea (compara con `VERSION_ESQUEMA_ACTUAL` simbólico); el resto del archivo **sí** cambia por `crearTipoAula` (**P8**) | Separar la comprobación de versión del uso del catálogo |

## 7. Riesgos vivos

- **Superficie de tests amplia** por el catálogo que ya no nace vacío: cualquier test que cuente tipos o cree
  «Aula de estudio» cambia. Se listan todos arriba; si aparece otro, se corrige en la tarea que lo rompa.
- **Huella histórica**: las entradas 2–5 de `HUELLAS_CONOCIDAS` no se pueden recalcular con el resumen
  nuevo; se conservan como históricas y el test solo usa la vigente.
- **Disparadores con texto interpolado**: si en el futuro el nombre llevara una comilla, el escapado la
  cubre; no se aceptan parámetros en `CREATE TRIGGER`.
- **Traducción del rechazo**: se resuelve releyendo la fila y con `traducirRechazo`; si la fila
  desapareciera entre el fallo y la relectura, se devuelve «no se encontró» (correcto: ya no hay nada que
  proteger). La rama solo se prueba **unitaria** (P4).
- **El `RAISE` es estático** (P12): el texto del disparador no puede incluir el valor recibido; por eso hay
  dos variantes declaradas y solo la del servicio cumple «mensaje con valor» (RNF-1).
- **TOCTOU en la guardia**: mitigado con la transacción de solo lectura (P5); si una conexión ajena escribe
  justo después del `commit`, la siguiente comprobación (016) lo verá.
- **El seed no puede crear el general**: si la base no lo tuviera, el alta completa falla con error explícito
  en vez de continuar (mejor que una base sembrada sin tipo general).
- **La base real del usuario (v5) deja de abrir**: es la decisión D-3; se recrea con respaldo.
- **La 016 depende de esto**: no se adelanta nada de su alcance (ni el comportamiento del frontend, ni el
  mensaje de V18, ni la huella de datos).

## 8. Verificación global (cierre de T8)

```bash
cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend
cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release
```

Totales de partida (registro de la 012): **54/54** en `backend` y **58/58** en `full-release`. Esta spec
añade **un** test (`test_tipo_general`), así que lo esperado es **55/55** y **59/59**; `full-release` incluye
la interfaz, que en esta spec solo cambia el delegado del literal. Si el total difiere, se anota el motivo en
`journal.md`.

## 9. Estimación

| Tarea | Contenido | Estimación |
|---|---|---|
| T1 | Definición única + alta + marca v6 | **> 30 min** |
| T2 | Disparador de borrado | 20–30 |
| T3 | Disparador de renombrado | 20–30 |
| T4 | Predicado puro de la fila | 25–30 |
| T5 | Guardia: índice, disparadores, fila, huella | **> 30 min** |
| T6 | Mensajes del servicio y traducción | **> 30 min** |
| T7 | Siembra | **> 30 min** |
| T8 | Documentación, supersesiones y regresión | **> 30 min** |

Los excesos quedan **declarados y aceptados** con la indicación del usuario del 2026-11-05 de seguir sin
parada de aprobación del plan.
