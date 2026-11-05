# Plan 016 — Comprobación y cierre del tipo general de aula

Base: **`bb1767af343c108b20de2b2b4de7ebe1d7658c2f`** (HEAD de `feat/Motor`) **+ la 015 aplicada en el árbol de trabajo, sin commits** (esquema **v6**, fila del tipo general, índice `TiposAula_clave`, los dos disparadores, guardia ampliada, seed adaptado; suites **55/55** `backend` y **59/59** `full-release` según el cierre de la 015).

> **Sobre la `Base`.** `HEAD` **sigue en `bb1767a`**: la 015 no se ha commiteado (su trabajo está en el
> árbol, junto con `specs/015-…/` y las notas de supersesión de 001/004/008/009/010/011/012). Si la 015
> **se commitea antes de empezar** la implementación de esta spec, el `Base` de la 016 **pasa a ser ese
> sha** (el de cierre de la 015) y deja de escribirse como «`bb1767a` + árbol»; este plan no cambia en nada
> más porque el árbol ya contiene lo que se consume. Todo lo que este plan describe se ha leído del **árbol
> de trabajo**, no de `bb1767a`. **A fecha de la validación**, el árbol contiene además la **016 ya
> implementada**, también sin commits (`HEAD` sigue en `bb1767a`) y las suites en **56/56** y **61/61**: eso
> no reescribe la `Base`, que sigue siendo `bb1767a` **+ el árbol**.

> **Alcance.** Implementa la **mitad 2** (comprobación y cierre) de la antigua 015: **cuándo** se
> comprueba la validez (al abrir, antes de generar y antes de guardar), el **frontend sin respaldo
> silencioso**, el **mensaje** de V18, el **tipo general fuera de la huella**, la **documentación de la
> API** y la **coherencia con la 013**. **No reimplementa** la garantía estructural (fila, protección,
> siembra, validez, guardia v6): la **reutiliza** tal y como la dejó la 015.
> **Fuera:** pantalla de administración del catálogo (deuda del frontend), migración de datos,
> validación nueva del contrato y cualquier cambio del solver, del contrato 2.0 o de la API del catálogo.

## 0. Reglas de trabajo

- **Tests primero** en cada tarea de código: caso(s) en rojo → código → verde. `add_qtest`/`add_gtest` en
  `test/CMakeLists.txt`.
- Una tarea por entrega; **ninguna cierra con el árbol sin compilar ni con tests rojos ajenos**.
- Una tarea se marca **solo** cuando pasa su verificación completa (comando de la tarea).
- **La 016 no toca código de la 013.** La re-base de la 013 (T8) es **documental y la hace el planner**.
- El implementer **no** edita `spec.md`, `plan.md` ni el `Estado` de ninguna spec.

## 1. Inventario de cambios

**Nuevos**

| Archivo | Responsabilidad |
|---|---|
| `src/backend/include/backend/database/comprobacion_validez.hpp` | `validez_base::Causa`, `Comprobacion`, `comprobar()` y los constructores puros de los mensajes (016 RF-1). **`Causa` es diagnóstico interno**: los tres consumidores usan solo `valida()`/`mensaje` (ver §2.1) |
| `src/backend/src/database/comprobacion_validez.cpp` | Implementación: guarda de versión + llamada al predicado único de la 015 |
| `test/backend/test_comprobacion_validez.cpp` | Pruebas de la comprobación (validez, causas, solo lectura, transacción, coincidencia con la apertura) |
| `test/frontend/test_tipos_aula_ui.cpp` | Pruebas de `TiposAulaUI::idTipoGeneral` (016 RF-2) |

**Modificados**

| Archivo | Cambio |
|---|---|
| `src/backend/CMakeLists.txt` | añade `src/database/comprobacion_validez.cpp` a `target_sources(backend …)` |
| `src/backend/src/database/apertura_base_datos.cpp` | **mensaje de la causa versión** (RF-1): crear base nueva y aviso de que un respaldo de la versión anterior volverá a fallar; **y calificador de la rama de esquema**: «restaurar un respaldo **de la versión vigente** o crear una base nueva» |
| `src/backend/include/backend/services/ServicioGeneracion.hpp` | `Estado::BaseInvalida` y parámetro `revisarBase` en el constructor |
| `src/backend/src/services/ServicioGeneracion.cpp` | comprobación antes de validar la entrada y de enviar (RF-1) |
| `src/backend/include/backend/services/NucleoDatos.hpp` / `src/backend/src/services/NucleoDatos.cpp` | método aditivo `comprobarValidezBase()` |
| `src/backend/src/services/ServicioConfiguracionHoraria.cpp` | comprobación antes de escribir la configuración (RF-1) |
| `src/backend/src/solver/config/solver_config.cpp` | **solo el texto** de V18 (RF-3); mismo identificador y misma regla |
| `src/frontend/src/services/tipos_aula_ui.hpp` | `idTipoGeneral` devuelve `Resultado<int>`; nunca otro tipo (RF-2) |
| `src/frontend/src/views/classroom_list_widget.cpp` | alta de aula: falla antes de abrir el formulario si no hay tipo general |
| `src/frontend/src/views/subject_list_widget.cpp` | alta de materia: ídem |
| `src/frontend/src/views/teacher_list_widget.cpp` | altas rápidas de materia desde docente (alta y edición): resuelve antes de escribir |
| `src/frontend/src/views/schedule_generation_widget.cpp` | comprobación antes de componer y cableado de `revisarBase` (RF-1) |
| `test/CMakeLists.txt` | **dentro de `if(TARGET frontend AND TARGET backend)`**: `add_qtest(test_tipos_aula_ui frontend/test_tipos_aula_ui.cpp frontend)` + `target_include_directories(test_tipos_aula_ui PRIVATE ${CMAKE_SOURCE_DIR}/src/frontend/src)` (el target `frontend` no exporta `src/frontend/src`); y en el bloque de `backend`, `add_gtest(test_comprobacion_validez backend/test_comprobacion_validez.cpp backend)` (`backend` ya enlaza `Qt6::Sql`) |
| `test/backend/test_apertura_base_datos.cpp` | la causa versión exige la acción nueva **y la rama de esquema exige «de la versión vigente»** |
| `test/backend/test_validador_entrada.cpp` | `exigeRechazoV18` añade convención y acción |
| `test/backend/test_servicio_generacion.cpp` | base inválida no envía; la huella no incluye el nombre visible del tipo |
| `test/backend/test_servicio_configuracion_horaria.cpp` | guardado rechazado con la base inválida, sin escribir |
| `test/backend/test_nucleo_datos.cpp` | `comprobarValidezBase()` en la fachada |
| `test/backend/test_contrato_generacion.cpp` | `estadosDeGeneracion_estanExponidos` enumera también `Estado::BaseInvalida` |
| `docs/interfaz-frontend.md` | §2.2 (comprobación en sesión), §2.4.3 (ayuda estricta y deuda de la 009), §2.4.4 (guardado), §3.1 (`ServicioGeneracion` y huella), §5 (deuda atendida) |
| `docs/guias/migracion-frontend.md` | fila de la 016, checklist de la ayuda y resumen |
| `docs/contrato-solver-2.0.md` | lectura nueva de V18 (convención + acción + base válida), **sin cambiar el contrato ni su versión** (**T5**; T7 solo verifica que está) |
| `specs/013-preferencias-docente-solver/{plan.md,tasks.md,spec.md,estado.md}` | re-base documental de la 013 (RF-6, **la hace el planner**) |
| `specs/016-tipo-general-comprobacion-y-cierre/tasks.md` y `estado.md` | seguimiento de tareas y estado |

No se toca `docs/constitution.md` (§5 ya protege el catálogo y §7 no describe el esquema físico; la regla
del aula de referencia no cambia). No se toca `MEMORY.md` (el planner entrega el parche al coordinator).

## 2. Diseño

### 2.1 Comprobación única de validez (RF-1, RNF-2, RNF-3)

**Qué se reutiliza, exactamente.** La 015 dejó **un solo predicado de validez**:
`VersionEsquema::esquemaEsperado(db, &motivo)` —tablas y columnas por nombre, ausencia de tablas
retiradas, presencia del índice `TiposAula_clave` y de los dos disparadores, y la fila del tipo general
vía `tipo_general::evaluar`—, en una **transacción de solo lectura** y sin escribir jamás. La 016 **no
escribe una segunda guardia**: la envuelve para (a) decidir la **causa** (versión vs. esquema) y (b)
devolver un **mensaje de usuario** con causa y acción. Eso es lo que exige «las tres comprobaciones serán
la misma»: mismo predicado, tres llamadas.

**Dónde se expone.** Módulo nuevo `backend/database/comprobacion_validez` (vive junto a
`version_esquema` y `tipo_general`, por debajo de los servicios y del frontend):

```cpp
// src/backend/include/backend/database/comprobacion_validez.hpp
namespace validez_base
{
    enum class Causa { Valida, Version, Esquema };

    struct Comprobacion
    {
        Causa   causa = Causa::Valida;
        QString mensaje;                        ///< "" si causa == Valida
        bool valida() const { return causa == Causa::Valida; }
    };

    /// Única comprobación de validez para los tres momentos (016 RF-1).
    /// Solo lectura: no migra, no repara y no escribe. `versionEsperada` se inyecta
    /// para las pruebas; en producción es la constante del esquema.
    Comprobacion comprobar(QSqlDatabase& db,
                           int versionEsperada = VersionEsquema::VERSION_ESQUEMA_ACTUAL);

    /// Constructores puros de los mensajes (sin E/S, sin reloj): se prueban solos.
    QString mensajeDeVersion(int versionArchivo, int versionEsperada);
    QString mensajeDeEsquema(int versionEsperada, const QString& motivo);
}
```

**`Causa` es diagnóstico interno, no contrato de consumo.** Los tres momentos (apertura, generación y
guardado) usan **solo** `valida()` y `mensaje`; ninguno ramifica por `Causa`, y no se añade una causa de
error/lectura: el fallo al leer la versión se informa como `Version` (con su mensaje propio de «no se pudo
leer la versión de esquema»). Así el enum puede crecer sin tocar consumidores y la decisión de ramificar
queda en un solo sitio, `comprobar`.

**Pseudocódigo (`comprobar`).**

```text
if (!db.isOpen())
    -> {Version, "No se pudo comprobar la base de datos: la conexión no está abierta. ..."}

propia = db.transaction()            # mismo patrón que la 015 (P5); si ya hay una, se usa esa
version = VersionEsquema::leerVersionEsquema(db)
if (version == -1):
    cerrarTransaccionSiPropia(); -> {Version, "No se pudo leer la versión de esquema ..."}
if (version != versionEsperada):
    cerrarTransaccionSiPropia(); -> {Version, mensajeDeVersion(version, versionEsperada)}

motivo = ""
ok = VersionEsquema::esquemaEsperado(db, &motivo)   # predicado único de la 015, dentro de la misma instantánea
cerrarTransaccionSiPropia()
if (!ok):
    -> {Esquema, mensajeDeEsquema(versionEsperada, motivo)}
-> {Valida, ""}
```

- **Orden:** versión primero (una lectura) y esquema después. Así una base v5 (el caso real del usuario) no
  paga la guardia completa y recibe el mensaje de versión, que es el que tiene una acción distinta; el
  `detalle` del esquema no se calcula cuando la versión no coincide (evita un motivo engañoso).
- **Coste (RNF-2):** una lectura de `PRAGMA user_version` + la guardia de la 015 (≈ 40 consultas de
  metadatos y un `SELECT` del catálogo de tipos, que tiene 3 filas). **No recorre datos de dominio.** La
  transacción de solo lectura es la misma que ya usa la apertura; se mide en T1 y se anota en
  `journal.md` (no se aserta tiempo: dependería de la máquina).
- **Sin escritura (RNF-3):** `esquemaEsperado` no escribe y `comprobar` tampoco; el test compara
  `sqlite_master` (tipo, nombre y SQL ordenados) y `user_version` antes y después.
- **Mensajes (RNF-1, español, deterministas).**
  - Versión: «La base de datos es de la versión **%1** y esta aplicación espera la versión **%2**. No se
    migra ni se repara. Cree una base nueva (se respalda la actual); **restaurar un respaldo de la
    versión anterior volverá a fallar**.»
  - Esquema: «La base de datos no cumple el esquema esperado de la versión **%1**: %2. Restaure un
    respaldo de la versión vigente o cree una base nueva (con respaldo obligatorio).»
- **El momento «al abrir»** ya existe (`AperturaBaseDatos::abrir` → `decidirApertura` + `esquemaEsperado`)
  y **no se reescribe**. Se alinean **las dos ramas** con RF-1: la de **versión** añade el aviso de que un
  respaldo de la versión anterior volverá a fallar, y la de **esquema** califica la acción como «restaurar
  un respaldo **de la versión vigente** o crear una base nueva (con respaldo obligatorio)» (hoy dice «o
  restaurar un respaldo» a secas). Se fija con la prueba cruzada de que, para el mismo defecto, la apertura
  y `comprobar` coinciden. `AperturaBaseDatos` conserva su mensaje propio (incluye la ruta).

### 2.2 Antes de generar (RF-1)

**Dónde.** En **dos capas con la misma función**:

1. **`ServicioGeneracion`** (la garantía: ningún camino llama al cálculo con una base inválida). No conoce
   la base de datos, así que recibe un **puerto de solo mensaje** por constructor, igual que ya recibe la
   huella y el reloj:

```cpp
enum class Estado { Inactivo, Calculando, Listo, DatosAnteriores, NoFactible,
                    ContratoInvalido, CalculoFallido, TiempoAgotado, EntradaInvalida,
                    BaseInvalida };                       // 016 RF-1

explicit ServicioGeneracion(PuertoSolver& puerto,
                            int plazoMs = 60000,
                            std::function<QString()> huellaActual = {},
                            std::function<QDateTime()> reloj = {},
                            std::function<QString()> revisarBase = {},   // "" = base válida
                            QObject* padre = nullptr);
```

   `generar(entrada, huella)` **no cambia de firma**. Orden nuevo:

```text
if (m_enCurso) return;                     # como hoy
if (m_revisarBase):
    mensaje = m_revisarBase()
    if (!mensaje.isEmpty()):
        m_ultimo = {}; m_ultimo.estado = BaseInvalida; m_ultimo.mensaje = mensaje
        emit finalizada(); return          # NO se valida la entrada ni se envía
... validarEntradaSolver(entrada) ...      # como hoy
```

   - **Precedencia declarada:** la validez de la base va **antes** de `validarEntradaSolver`. Con una base
     inválida, el diagnóstico útil es el de la base (V18 o un tipo inexistente atribuirían el fallo a los
     datos). Se fija con un caso: entrada **también** inválida + base inválida ⇒ `BaseInvalida`.
   - `revisarBase` se coloca **antes de `padre`** a propósito: el único constructor de producción
     (`schedule_generation_widget.cpp:128`) deja de compilar hasta que se cablea; no hay forma de olvidarlo
     en silencio.
2. **La vista** (`ScheduleGenerationWidget`), **antes de componer la entrada**: así el usuario no recibe
   «el aula no tiene tipo» (efecto de la base rota) sino la causa real. Se comprueba al pulsar «Generar»,
   después de `if (m_servicio->enCurso()) return;` y antes de `leerOpciones()`. Si falla: estado + diálogo
   con el mensaje de `validez_base`, y **no se compone ni se envía**.

**Cableado del frontend.** `NucleoDatos` gana un método **aditivo** (no cambia el contrato ni la API del
catálogo):

```cpp
/// Comprobación única de validez (016 RF-1). Solo lectura; mismo predicado que la apertura.
validez_base::Comprobacion comprobarValidezBase() const;
```

y la vista pasa `[this]{ return m_nucleo ? m_nucleo->comprobarValidezBase().mensaje : QString(); }` como
`revisarBase`. Criterio ejecutable de que no hay caminos sin cablear:
`grep -n "new ServicioGeneracion" src/` devuelve **1** y esa llamada pasa el quinto argumento.

### 2.3 Antes de guardar la configuración (RF-1)

`ServicioConfiguracionHoraria::guardar(int)` es la **única** pieza que escribe `ConfiguracionHoraria` y ya
tiene la conexión (`m_db`). La comprobación va **antes** del rango y del *upsert*:

```text
base = validez_base::comprobar(m_db)
if (!base.valida())
    return Resultado::error(base.mensaje + " No se guardó la configuración de la hora académica.")
... errorDeRango ... upsert ...
```

- **Precedencia:** base primero, igual que en la generación (fallo estructural antes que validación de
  entrada); se fija con un caso (base inválida + 29 ⇒ mensaje de base).
- `consultar()` **no** comprueba: RF-1 nombra tres momentos y la pantalla debe poder seguir mostrando el
  valor; el guardado es lo que se rechaza. Se fija con un caso explícito.
- **Sin escritura:** el caso de la base inválida comprueba que la fila conserva su valor y que no se crea
  ninguna fila (misma evidencia que los casos de rango de hoy).
- **`preset` no entra aquí.** RF-1 habla de «la configuración de generación» y el guardado de la base es
  el de la hora académica (spec 011). El *preset* (`CargadorConfiguracionSolver::guardar`) es un
  **archivo**, no la base, y no se toca: no hay nada que validar contra el esquema en una escritura que no
  es de la base. Se declara en el plan y en §2.4.4 de la guía para que no quede un camino «sin fijar».

### 2.4 Frontend sin respaldo silencioso (RF-2)

**Firma nueva** en `src/frontend/src/services/tipos_aula_ui.hpp` (ayuda de interfaz; **no** es la API del
catálogo, que conserva firmas y campos):

```cpp
/// Id del tipo general del catálogo. Si no está (o el catálogo está vacío), error con el
/// mensaje de qué falta y qué hacer. NUNCA devuelve otro tipo (016 RF-2).
inline Resultado<int> idTipoGeneral(const QVector<TipoAulaDTO>& tipos)
{
    for (const TipoAulaDTO& tipo : tipos)
        if (tipo_general::esNombreCanonico(tipo.nombre))     // trim + plegado, igual que la clave
            return Resultado<int>::exito(tipo.id);
    return Resultado<int>::error(QStringLiteral(
        "El catálogo de tipos de aula no contiene el tipo general «%1»: la base no está en el "
        "estado esperado. Restaure un respaldo de la versión vigente o cree una base nueva "
        "(con respaldo obligatorio). No se ha guardado ningún dato.").arg(tipo_general::nombre()));
}
```

- **`Resultado<int>` y no `std::optional<int>`:** el mensaje de RF-2 («qué falta y qué hacer», sin errores
  técnicos) es **uno solo** y vive con la ayuda; con `optional` habría que repetirlo en tres vistas
  (RNF-1 y criterio 2 se degradarían). Descartado también `-1` como centinela: es exactamente el respaldo
  silencioso que la spec prohíbe (`-1` acaba en el primer ítem del combo).
  El encabezado añade `#include "backend/resultado.hpp"` (hoy no lo necesita).
- **Normalización:** se compara con `tipo_general::esNombreCanonico` (recorte + plegado), no con
  `compare(Qt::CaseInsensitive)`: la interfaz acepta «  AULA DE ESTUDIO » igual que la base, y no aparece
  una tercera noción de igualdad.
- **Duplicados canónicos:** devuelve el primero; una base con dos filas canónicas es **inválida** por la
  guardia (015) y con RF-1 el producto no llega a guardar nada.
- **Mensajes de la vista, separados:** fallo de **lectura** del catálogo (`!listarTiposAula().ok`) → «No se
  pudo leer el catálogo de tipos de aula.» + `mensajeError`; catálogo leído sin el general → el mensaje de
  `idTipoGeneral`. En ambos casos **no se abre el formulario**.

**Call sites (tres archivos, cuatro llamadas; ninguna más)** — y el detalle que importa: aunque la ayuda
devuelva error, si se abre el diálogo el `QComboBox` se queda en el **primer** ítem (`cargarTiposAula` solo
selecciona si `id > 0`). Por eso la comprobación va **antes de abrir el diálogo**:

| Archivo:línea | Antes | Después |
|---|---|---|
| `classroom_list_widget.cpp:286` | aviso «Catálogo vacío» + `idTipoGeneral(tiposAula)` como `int` | `if (!tipos.ok)` → aviso y salir; `if (!id.ok)` → aviso y salir; `cargarTiposAula(tipos, id.valor)` |
| `subject_list_widget.cpp:376` | ídem | ídem |
| `teacher_list_widget.cpp:407` y `:468` | resuelve **después** de `crearDocente`/`actualizarDocente` y con el resultado ignorado | si hay materias que crear, resuelve **antes** de escribir; si falla, aviso y `return` (ni docente, ni materias, ni `limpiarMaterias`) |

`nombreTipoGeneral()` y `poblar()` **no cambian** (se usan para mostrar y para el CRUD). La **pantalla** de
gestión del catálogo sigue siendo deuda de la 009 y no entra aquí.

### 2.5 Mensaje de V18 (RF-3)

`src/backend/src/solver/config/solver_config.cpp:805-828`. **Misma regla, mismo momento, mismo
identificador**; solo el texto:

```text
V18: el aula «%1», referencia del curso %2%3, es del tipo «%4». El tipo general del centro es el de
nombre «%5» y el aula de referencia de cada curso debe ser de ese tipo. Asigne al curso un aula del
tipo general y compruebe que la base es válida.
```

- **No se añade validación.** El identificador sigue siendo V18 y el siguiente hueco (V19) sigue reservado
  a la 013; no se toca la cadena de validaciones ni el orden.
- `%5` sigue saliendo de `tipoGeneralAula()` (definición única de la 015): no se repite el literal.
- Lo que el mensaje **sí** añade al usuario: la convención (el de nombre «Aula de estudio»), que el aula
  recibida es de otro tipo (ya estaba) y la acción doble (asignar un aula del tipo general **y** comprobar
  que la base es válida).
- `test/backend/test_validador_entrada.cpp:272-286` (`exigeRechazoV18`) añade las tres aserciones de la
  convención y la acción; `startsWith("V18:")` se conserva, así que un cambio de código de error rompe el
  test.

### 2.6 Tipo general fuera de la huella de datos (RF-4)

**Comprobado hoy:** `huellaDatos(const DatosDominio&, int)` no recibe el catálogo —`DatosDominio` no tiene
campo de tipos— y de cada aula/materia usa la **identidad** (`idTipoAula`), no el nombre visible. El tipo
general, por tanto, **ya está fuera**; no hay cambio de código. Se **fija con pruebas**:

- `huellaDatos_noIncluyeElTipoGeneralNiSuNombreVisible`: dos dominios que solo difieren en
  `AulaDTO.tipoAula`/`MateriaDTO.tipoAula` (el nombre visible, inmutable para el general) dan **la misma**
  huella; cambiar `idTipoAula` **sí** la cambia (`huellaDatos_cambiaConElTipoDeAula`, ya existente).
- Criterio ejecutable de que no se ha colado una dependencia nueva:
  `grep -n "TiposAula\|tipo_general" src/backend/src/services/ServicioGeneracion.cpp` devuelve **0**.

### 2.7 Documentación (RF-5)

`docs/interfaz-frontend.md`:

- **§2.2 `VersionEsquema`:** añade `validez_base::comprobar` a la lista de piezas del esquema, con una
  línea por causa y la frase «la validez se comprueba al abrir, **antes de generar** y **antes de guardar
  la configuración**; no migra, no repara y no escribe». **No** repite lo que ya documentó la 015 (v6,
  índice, disparadores, predicado, huella).
- **§2.4.3 `Tipos de aula`:** **no hay descripción previa de la ayuda que retirar** (la sección solo
  enumera el catálogo y su CRUD en «Cómo lo trata el frontend»; `idTipoGeneral` no figura en la guía). Lo
  que se hace es **añadir** la conducta nueva: la ayuda que resuelve el tipo general **falla** en vez de
  devolver otro tipo; qué operaciones se rechazan (cualquier alta que la use: aula, materia y materia
  rápida desde docente) y con qué mensaje; **la base nueva ya trae el tipo general**; **no hay campos ni
  firmas nuevas** del catálogo; y la nota de que **la deuda de la 009 sobre esta ayuda queda atendida**. La
  deuda que **sigue** es la **pantalla** del catálogo, y se dice en el mismo párrafo (no se retira nada más).
- **§2.4.4 `ServicioConfiguracionHoraria`:** `guardar` rechaza si la base no es válida (causa y acción) y
  no escribe; `consultar` no comprueba.
- **§3.1 `ServicioGeneracion`:** firma con `revisarBase`, `Estado::BaseInvalida` y la regla «no se valida
  la entrada ni se envía»; se dice expresamente que **la huella no incluye el catálogo de tipos** (solo la
  identidad que referencia cada aula o materia).
- **§5 (deuda de la 009):** en la actualización del 2026-11-01, la ayuda del tipo general pasa a
  «atendida por la 016»; la pantalla del catálogo sigue pendiente.

`docs/guias/migracion-frontend.md`: fila de la **016** en §1 (comprobación en tres momentos, ayuda
estricta, sin cambios de API), el checklist de §4 (la ayuda ya no sustituye el tipo) y el resumen de §6.

`docs/contrato-solver-2.0.md` (§3, V18 y §5, presets anteriores): **lo edita T5**; la regla **no cambia**;
T5 añade la lectura del mensaje (convención + acción) y que **si el catálogo no tiene el tipo general la
base es inválida** (016) y eso se arregla restaurando respaldo o creando base, no editando el preset. La
versión del contrato sigue en **2.0**. **T7 no lo edita: solo verifica el resultado de T5.**

`docs/constitution.md`: **no se toca** (ni §5 ni §7 cambian).

### 2.8 Coherencia con la 013 (RF-6) — trabajo del planner, sin código

La 016 **deja la 013 re-basada**. Edita **solo** `specs/013-preferencias-docente-solver/`:

1. `plan.md`, línea de `Base`: de `bb1767a` al **estado posterior a la 015** —misma regla que este plan:
   el sha de cierre de la 015 si está commiteada; si no, «`bb1767a` + 015 y 016 aplicadas en el árbol (sin
   commits)»—.
2. `plan.md` §7 «Restricciones (no se tocan)»: «**Esquema (v5)** y migraciones» → «**Esquema (v6)** y
   migraciones».
3. `spec.md` **RNF-4** (última frase) y **criterio 11**: se conserva el texto y se añade la **nota de
   supersesión del ancla**: la comprobación deja de anclarse al escenario de datos de prueba y preset de
   `bb1767a` y se ancla al **escenario posterior a la 015** (base **v6**, tipo general garantizado por el
   esquema y **perfil de datos de prueba sin «Aula de estudio»**, 3 tipos contados desde la base). **RNF-4
   y el criterio 11 no cambian de requisito**; cambia la referencia contra la que se verifican.
4. `tasks.md`: la línea de `Base` y el criterio de T10 («idénticos a los de `bb1767a`» → «idénticos a los
   del escenario posterior a la 015, `Base` de este plan»).
5. `estado.md`: `Base`, entrada de historial («re-basada por la 016») y constancia de que **la 013 no tiene
   huella de esquema propia** (la única congelada es la del test de versión del esquema, que actualizó la
   015).

**Quién:** el **planner** (T8). El implementer **no** edita la 013: solo verifica el resultado con los
`grep` de T8 y lo re-comprueba en la regresión de T9. **Nada de `src/`.**

## 3. Funciones puras y deterministas

| Función | Entradas | Salida | Reloj / azar |
|---|---|---|---|
| `validez_base::mensajeDeVersion` | dos `int` | `QString` | no |
| `validez_base::mensajeDeEsquema` | `int`, `QString` | `QString` | no |
| `TiposAulaUI::idTipoGeneral` | `QVector<TipoAulaDTO>` | `Resultado<int>` | no |
| `tipo_general::esNombreCanonico` (015, reutilizada) | `QString` | `bool` | no |
| `validez_base::comprobar` | `QSqlDatabase&`, `int` | `Comprobacion` | no (solo lectura; determinista para la misma base) |

No hay ninguna función con el tiempo como parámetro en esta spec: **ninguna** decisión depende del reloj.
`idTipoGeneral` es pura (no toca la base) y **no depende del orden** más allá de devolver la primera fila
canónica, caso que la validez de la base ya excluye.

## 4. Decisiones y alternativas descartadas

1. **Se reutiliza `esquemaEsperado` (la guardia de la 015), no se escribe una comprobación nueva.**
   Descartado: una comprobación «ligera» limitada a versión + índice + disparadores + fila. Sería una
   **segunda noción de validez** (una base sin una tabla pasaría al generar) y contradice RF-1 («las tres
   comprobaciones serán la misma»). El coste de la guardia completa es de metadatos y catálogo (3 filas).
2. **La causa se decide con la versión primero.** Descartado: llamar a la guardia y deducir la causa del
   motivo. Con una base v5 el motivo sería «faltan tablas/objetos», que no es la causa real y ofrece la
   acción equivocada (restaurar un respaldo que volverá a fallar).
3. **Un solo mensaje por causa, compartido por los tres momentos** y un sufijo del momento solo donde
   aporta («No se guardó la configuración…»). Descartado: tres redacciones distintas para el mismo fallo.
4. **La comprobación antes de generar se inyecta como `std::function<QString()>`**, no como
   `QSqlDatabase&`. Descartado: acoplar `ServicioGeneracion` a SQL (rompe su carácter UI-free y obligaría a
   una base en 12 tests); y descartado comprobar solo en la vista (un llamador no-UI podría enviar).
   El parámetro va **antes de `padre`** para que olvidar el cableado **no compile**.
5. **Comprobar también en la vista antes de componer.** Descartado: comprobar solo en el servicio, porque
   la composición ocurre antes y un fallo de datos taparía la causa real.
6. **`idTipoGeneral` devuelve `Resultado<int>`.** Descartado: `std::optional<int>` (triplica el mensaje en
   tres vistas y pierde «qué hacer») y `-1` centinela (el combo cae al primer tipo: el respaldo silencioso
   que RF-2 prohíbe). La comprobación va **antes de abrir el diálogo**, porque el combo también tiene su
   propio defecto por omisión.
7. **La ayuda compara con `esNombreCanonico`**, no con `compare(CaseInsensitive)`. Descartado: dejar dos
   normalizaciones distintas entre la interfaz y la clave de la base.
8. **El guardado de la configuración comprueba la base; el preset (archivo) no.** Descartado: extender la
   comprobación al preset, que no es una escritura en la base y no tiene esquema que validar.
9. **V18 conserva identificador y solo cambia el texto.** Descartado: una validación nueva (la 013 tiene
   reservado V19) o cambiar la regla (fuera de alcance de la 016).
10. **La huella no cambia de código** (ya excluye el catálogo): se fija con pruebas y con un `grep`.
11. **La re-base de la 013 es documental y del planner.** Descartado: que la haga el implementer (editaría
    specs ajenas a su tarea) y descartado dejarla para después (la 013 se planificaría sobre una base que
    ya no existe).

## 5. Riesgos vivos

- **Comprobar antes de generar no debe duplicar la validación de entrada.** Se resuelve con precedencia
  declarada (base → entrada) y un caso que la fija; el coste es una comprobación de metadatos por clic.
- **El combo como respaldo silencioso residual.** Aunque `idTipoGeneral` sea estricta, `cargarTiposAula`
  deja el primer ítem si el id no llega; por eso las tres vistas **abortan antes de abrir** el diálogo y
  los formularios **no** cambian. Se verifica por revisión de los tres call sites (no hay arnés de GUI).
- **Otros usos de `idTipoGeneral`.** Solo hay tres archivos y cuatro llamadas (aula, materia y docente en
  alta y edición); `nombreTipoGeneral` no cambia. `grep -n "idTipoGeneral" src/` debe seguir devolviendo
  **5** líneas al cerrar T4 —la definición de la ayuda y las **cuatro** llamadas—: si aparece una sexta, hay
  un call site nuevo sin revisar.
- **V18 no puede cambiar su código de error.** `startsWith("V18:")` sigue asertado; el mensaje solo añade
  frases al final.
- **La 016 no toca la 013 en código.** T8 se limita a `specs/013*/`; como el directorio **no está bajo
  seguimiento**, el alcance no se prueba con `git diff --stat` (saldría vacío): se usa la instantánea de
  `git status --porcelain` de T8 o `git add -N` + `git diff --stat`.
- **Coste/transacción.** `comprobar` reutiliza el patrón P5 de la 015; se prueba que no deja transacción
  abierta y que funciona dentro de una ajena (el guardado de la configuración puede llegar dentro de una).
- **Base real del usuario.** Sigue en v5 y se rechaza al abrir (015); el mensaje de versión ahora dice que
  un respaldo de la versión anterior volverá a fallar. No es trabajo de esta spec recrearla.
- **Host de compilación.** T2, T4, T7 y T9 necesitan `full-release`; si el `build-rel` del host estuviera
  en una ruta ajena, se reconfigura el preset. Una tarea **no** se cierra con el árbol sin compilar: si el
  entorno lo impide, se para y se informa (no se sustituye por `-fsyntax-only` salvo indicación).
- **Totales de la suite.** Se esperan **56/56** (`backend`) y **61/61** (`full-release`) con los dos
  binarios nuevos; si el total difiere, se anota el motivo en `journal.md` (como en la 015).

## 6. Trazabilidad RF → tarea → verificación

| RF | Tarea(s) | Verificación |
|---|---|---|
| **RF-1** comprobación al abrir, antes de generar y antes de guardar; misma comprobación, solo lectura, sin escribir | T1, T2, T3 | `test_comprobacion_validez` (`baseValida_*`, `sinTipoGeneral_*`, `sinObjeto_*`, `tablaRetirada_*`, `versionAnterior_*`, `noModificaElCatalogo`, `noDejaTransaccionAbierta`, `funcionaDentroDeUnaTransaccion`, `aperturaYComprobacionCoinciden`); `test_backend_apertura_base_datos` (causa versión con la acción nueva); `test_servicio_generacion` (`baseInvalida_noEnvia`, `baseInvalidaTienePrioridadSobreLaEntrada`); `test_backend_nucleo_datos` (`comprobarValidezBase_*`); `test_servicio_configuracion_horaria` (`guardar_baseInvalida_*`, `consultar_baseInvalida_*`) |
| **RNF-2** barata y sin recorrer datos de dominio | T1 | la comprobación = 1 lectura de versión + guardia de metadatos; medida anotada en `journal.md` |
| **RNF-3** no escribe al comprobar | T1, T3 | `noModificaElCatalogo`; los casos de guardado comprueban valor y número de filas |
| **RF-2** frontend sin respaldo silencioso | T4 | `test_tipos_aula_ui` (`encuentraElGeneral`, `sinElGeneral_fallaYNoDevuelveElPrimero`, `catalogoVacio_falla`, `aceptaMayusculasYEspacios`, mensaje con qué falta y qué hacer); `grep` de las cuatro llamadas en las vistas; `full-release` compila |
| **RF-3** mensaje de V18 con la convención y la acción, sin validación nueva | T5 | `test_validador_entrada` (`v18_*`, `exigeRechazoV18` con convención y acción); `grep` de V1–V18 sin V19 nueva |
| **RF-4** tipo general fuera de la huella | T6 | `test_servicio_generacion.huellaDatos_noIncluyeElTipoGeneralNiSuNombreVisible`; `huellaDatos_cambiaConElTipoDeAula` sigue verde; `grep` del catálogo en `ServicioGeneracion.cpp` = 0 |
| **RF-5** documentación de la API y deuda de la 009 | T7, T9 | `grep` de la conducta nueva en §2.4.3/§2.2/§3.1/§2.4.4/§5 y de la fila de la 016 en la guía; T7 **verifica** lo que editó T5 en el contrato; revisión de que no se duplica la 015 |
| **RF-6** coherencia con la 013 | T8, T9 | `grep -n "Esquema (v5)" specs/013*` = 0; `grep -n "Base:" specs/013*/plan.md` con la base nueva; nota de supersesión del ancla en `spec.md` RNF-4/criterio 11; **alcance por instantánea** (el directorio no está bajo seguimiento): `git status --porcelain > /tmp/016-antes-T8.txt` antes de editar y `git status --porcelain \| diff /tmp/016-antes-T8.txt -` después, exigiendo que las únicas diferencias estén bajo `specs/013*/` (alternativa: `git add -N specs/013-preferencias-docente-solver/` y `git diff --stat -- specs/013-preferencias-docente-solver/`) |
| **RNF-1** mensajes con entidad, valor, causa y acción | T1, T3, T4, T5 | aserciones sobre los mensajes de los casos anteriores |
| **RNF-4** alcance mínimo (sin cambiar modelo, contrato, API del catálogo ni huella) | T2, T4, T6, T9 | `test_backend_contrato_interfaz`, `test_contrato_solver` y `test_cargador_configuracion` en verde sin tocarlos; `ServicioTiposAula` y el DTO sin cambios |
| **RNF-5** determinismo | T1, T6 | dos llamadas seguidas dan el mismo `Comprobacion.mensaje` y la misma huella |
| **RNF-6** trazabilidad | todas | esta tabla y `tasks.md` |

**Criterios de finalización → tarea:** 1 → T1, T2, T3 · 2 → T4 · 3 → T5 · 4 → T6 · 5 → T7 · 6 → T8 ·
7 → esta tabla y T9.

## 7. Tests existentes que cambian (y por qué)

| Test | Cambio | Motivo |
|---|---|---|
| `test/backend/test_apertura_base_datos.cpp` | el caso de versión añade las aserciones de la acción («crear una base nueva», «versión anterior», «volverá a fallar») **y el caso de esquema exige «un respaldo de la versión vigente»** | RF-1: hoy la rama de versión no advierte del respaldo fallido y la de esquema no califica el respaldo |
| `test/backend/test_validador_entrada.cpp` | `exigeRechazoV18` añade «Aula de estudio», «Asigne al curso un aula del tipo general» y «base es válida»; `startsWith("V18:")` se conserva | RF-3: solo cambia el texto |
| `test/backend/test_servicio_generacion.cpp` | casos nuevos `baseInvalida_noEnvia`, `baseInvalidaTienePrioridadSobreLaEntrada`, `huellaDatos_noIncluyeElTipoGeneralNiSuNombreVisible` | RF-1 y RF-4 |
| `test/backend/test_servicio_configuracion_horaria.cpp` | helper `invalidarBase()` (`DROP TRIGGER` y, si hace falta, borrado de la fila) y tres casos | RF-1 |
| `test/backend/test_nucleo_datos.cpp` | dos casos de `comprobarValidezBase()` (válida / sin disparador) | la fachada es la vía del frontend |
| `test/backend/test_contrato_generacion.cpp` | `estadosDeGeneracion_estanExponidos` añade `Estado::BaseInvalida` a la lista | el enum es parte del contrato observable de la generación |
| `test/CMakeLists.txt` | dos registros nuevos; el de `test_tipos_aula_ui` va **dentro de `if(TARGET frontend AND TARGET backend)`** y con su `target_include_directories` | los dos binarios y el preset `backend` intacto |
| `test_frontend_conversion.cpp`, `test_generacion_seed.cpp`, `test_selector_generacion.cpp` | **sin cambios** | se comprobó al planificar: no usan `idTipoGeneral` ni la validez de la base |

Ningún test existente cambia por la re-base de la 013 (no se ejecuta su código hasta implementarla).

## 8. Estimación y excesos

| Tarea | Contenido | Estimación |
|---|---|---|
| T1 | Comprobación única + causa/acción de las dos ramas al abrir | **> 30 min (declarado)** |
| T2 | Antes de generar: servicio, fachada y vista | **> 30 min (declarado)** |
| T3 | Antes de guardar la configuración | 20–30 |
| T4 | Frontend sin respaldo silencioso | **> 30 min (declarado)** |
| T5 | Mensaje de V18 + lectura en el contrato | 20–30 |
| T6 | Huella: pruebas y `grep` | 15–20 |
| T7 | Documentación de la API y deuda de la 009 | **> 30 min (declarado)** |
| T8 | Re-base de la 013 (planner, sin código) | 25–30 |
| T9 | Regresión global | 20–30 |

**Nueve tareas**, dentro del límite de 10 y con holgura para el QA. Excesos declarados con la indicación
del usuario del 2026-11-05 de **seguir sin parada de aprobación del plan**.

## 9. Verificación global (cierre de T9)

```bash
cmake --preset backend && cmake --build --preset backend --parallel && ctest --preset backend
cmake --preset full-release && cmake --build build-rel --parallel && ctest --preset full-release
```

Totales de partida (cierre de la 015): **55/55** en `backend` y **59/59** en `full-release`. Esta spec
añade **un** test de backend (`test_comprobacion_validez`) y **uno** de frontend (`test_tipos_aula_ui`):
lo esperado es **56/56** y **61/61**. Si el total difiere, se anota el motivo en `journal.md`.
