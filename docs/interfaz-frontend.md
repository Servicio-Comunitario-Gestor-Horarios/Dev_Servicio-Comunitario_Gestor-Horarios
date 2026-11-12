# Interfaz expuesta al equipo de frontend

> **Documentación viva.** Describe los contratos (firmas, estados y errores) que nuestro lado
> (`src/backend`, `src/app` y `src/middleware`) expone para que la interfaz Qt de `src/frontend`
> —implementada por **otro equipo**— consuma la base de datos y la lógica de negocio.
>
> No duplica los requisitos: para el QUÉ y el POR QUÉ, ver `specs/001-base-datos-local/spec.md`,
> `specs/002-generacion-horarios-ipc/spec.md` y `specs/009-tipos-de-aula-y-eventos/spec.md`. Aquí solo
> se documenta el **contrato de consumo** (qué exponemos, con qué firma, qué estados/errores y cómo
> debe tratarlos el frontend).
>
> Se mantiene con las tareas «Documentar la interfaz expuesta al frontend» de cada spec.
>
> **Aviso de alcance (spec 009).** Esta entrega cambia el contrato que consume la interfaz: los
> descansos del turno pasan a ser **eventos con identidad**, las aulas y materias **exigen un tipo de
> aula** del catálogo y desaparecen el DTO y los métodos propios de los descansos. `src/frontend`
> **queda fuera del alcance de la 009** y **no compila** hasta que su equipo migre las llamadas y los
> formularios; la verificación de la entrega se hace con el preset `backend`
> (`BUILD_FRONTEND=OFF`). La deuda concreta está en la §5.

---

## 1. Convenciones generales

### 1.1 `Resultado<T>` — resultado de operaciones (implementado)

`src/backend/include/backend/resultado.hpp`:

```cpp
template<typename T>
struct Resultado {
    bool     ok = false;        // true si la operación fue exitosa
    T        valor{};           // válido solo si ok == true
    QString  mensajeError;      // válido solo si ok == false
    int      codigoError = 0;   // código de error adicional

    static Resultado<T> exito(const T& val);
    static Resultado<T> error(const QString& msg, int codigo = -1);
};
```

**Cómo lo trata el frontend:** comprobar siempre `ok`; si es `false`, mostrar `mensajeError`
(causa + acciones disponibles) y no dar la operación por completada. `codigoError` permite
distinguir casos sin depender del texto.

### 1.2 Mensajes IPC (middleware)

`src/middleware/include/middleware/messages.h` (implementado). NDJSON v1:
`{"v":1,"op":"...","payload":{...}}` → `{"v":1,"op":"...","status":"ok"|"error","code":N,"data":...}`.

- Operaciones de sistema: `health_check`, `ready`, `shutdown`, `solver_resolve`.
- Códigos: `0` éxito, `-1` error, `-2` no encontrado, `-3` tiempo agotado, `-4` inválido,
  `-5` no implementado (reservado), `-6` versión incompatible.

---

## 2. Base de datos local (spec 001)

### 2.1 `AperturaBaseDatos` — abrir o crear, sin migrar (implementado)

`src/backend/include/backend/database/apertura_base_datos.hpp`:

```cpp
namespace AperturaBaseDatos {

enum class Estado {
    OkCreada,        // No existía: se creó con el esquema único.
    OkAbierta,       // Existía con versión y catálogo esperados: se abrió sin tocar.
    FalloEsquema,    // Versión o catálogo no corresponden al esquema esperado.
    FalloApertura    // No se pudo abrir la base o crear el esquema.
};

struct Resultado {
    Estado  estado = Estado::FalloApertura;
    QString detalle;        // texto legible para el usuario (en español)
    bool    ok() const;     // true solo para OkCreada | OkAbierta
};

struct Opciones { /* versionEsperada, crear, verificar — inyectables en tests */ };

Resultado abrir(const QString& ruta);                 // valores de producción
Resultado abrir(const QString& ruta, const Opciones& opciones);
}
```

No hay migración por versión: si la base existe, su marca (`PRAGMA user_version`) **y** su catálogo
deben coincidir con el esquema esperado. `detalle` identifica la causa con precisión: si la versión no
coincide nombra la versión del archivo y la esperada; si la versión coincide pero el catálogo no,
nombra las tablas/columnas que faltan, los **objetos exigidos** que faltan (el índice único y los dos
disparadores del tipo general), la **fila** del tipo general cuando no cumple el predicado (sin tipo,
varias filas o clave almacenada distinta) y las tablas retiradas que sobran (p. ej. «faltan las
columnas «anio», «seccion», «aula» de la tabla «Cursos»; existe la tabla retirada «Curso_Materia»»).
Si la creación de la base falla, el detalle dice que **no quedó esquema parcial** y que el archivo
puede descartarse y crearse de nuevo (con respaldo obligatorio).

**Cómo lo trata el frontend:**
- `ok() == true` → continuar el arranque.
- `ok() == false` → diálogo de fallo RF-4 (Reintentar / Restaurar respaldo / Crear base nueva /
  Salir), mostrando `detalle` como causa. El arranque del cliente ya implementa ese diálogo
  (`src/app/src/aplicacion_frontend.cpp`).
- Para «Crear base nueva», usar `Respaldo::descartarBaseYCrearNueva` (ver 2.3): el respaldo es
  obligatorio, así que la opción funciona aunque la base no se pueda leer.

### 2.2 `VersionEsquema` — esquema único y su marca (implementado)

`src/backend/include/backend/database/version_esquema.hpp`:

```cpp
namespace VersionEsquema {
constexpr int VERSION_ESQUEMA_ACTUAL = 6;   // v6: tipo general de aula garantizado y protegido
enum class EstadoApertura { Crear, Abrir, FalloEsquema };
EstadoApertura decidirApertura(bool existe, int versionArchivo, int versionEsperada); // pura
int  leerVersionEsquema(QSqlDatabase& db);          // -1 si no es legible
bool fijarVersionEsquema(QSqlDatabase& db, int version);
bool crearEsquema(QSqlDatabase& db);                // catálogo entero en una transacción
bool esquemaEsperado(QSqlDatabase& db, QString* motivo = nullptr); // tablas, objetos, fila y retiradas
}
```

`decidirApertura` es pura (sin E/S): `versionArchivo <= 0` representa versión ausente. `crearEsquema`
aplica el catálogo completo (`Migracion::aplicarPaso`) y fija `user_version` en una sola transacción;
si algo falla, revierte. La versión **6** añade el **tipo general de aula** —la fila, el índice único
`TiposAula_clave` y los dos disparadores que impiden su borrado y su renombrado— al catálogo de la
**v5**. El catálogo esperado incluye, **por nombre**, las tablas y sus columnas, el índice y los dos
disparadores, **exactamente una** fila del tipo general (nombre visible normalizado «aula de estudio»
y su clave almacenada «aula de estudio») y la ausencia de las tablas retiradas. La **v5** había
añadido las preferencias (`Preferencia_Profesor`) al catálogo de la **v4**. Una base con la versión
anterior (**5**) **no se migra**: se rechaza al abrir.

> **Guardia anti-regresión.** `test/backend/test_version_esquema.cpp` congela la **huella SHA-256**
> del catálogo real (`HUELLAS_CONOCIDAS`, por versión) resumiendo **tablas, índices y disparadores**.
> Si se cambia el DDL hay que **subir `VERSION_ESQUEMA_ACTUAL`** y registrar la huella nueva en el
> mismo cambio; de lo contrario el test falla. Así una base creada por una compilación intermedia no
> queda con la marca actual y un catálogo viejo (incidente del 2026-10-17: `Cursos` pasó de
> `nombre`/`aula_fija` a `anio`/`seccion`/`turno`/`aula` sin subir la versión, y las bases existentes
> dejaron de abrir). La huella incluye el **cuerpo** del disparador como guardia de release; la
> validez de la base comprueba su **presencia por nombre**, no el cuerpo.

**Cómo lo trata el frontend:** normalmente no lo usa directamente; `AperturaBaseDatos` ya envuelve
la decisión y la verificación del catálogo.

**Comprobación en sesión (`validez_base`).** `src/backend/include/backend/database/comprobacion_validez.hpp`
envuelve el **predicado único** de la 015 (`VersionEsquema::esquemaEsperado`) sin duplicarlo:

```cpp
namespace validez_base {
    enum class Causa { Valida, Version, Esquema };   // diagnóstico interno
    struct Comprobacion { Causa causa; QString mensaje; bool valida() const; };
    Comprobacion comprobar(QSqlDatabase& db,
                           int versionEsperada = VersionEsquema::VERSION_ESQUEMA_ACTUAL);
}
```

- **Valida:** la base sirve; `mensaje` vacío.
- **Version:** la marca no coincide (o no se puede leer); el mensaje propone crear una base nueva y
  advierte de que un respaldo de la versión anterior volverá a fallar.
- **Esquema:** la versión coincide pero falta la fila del general, el índice, un disparador o sobra
  una tabla retirada; el mensaje lleva la causa concreta y la acción (respaldo de la versión vigente
  o base nueva con respaldo obligatorio).

La validez se comprueba **al abrir**, **antes de generar** y **antes de guardar la configuración**;
es la misma comprobación, de solo lectura: **no migra, no repara y no escribe**.

### 2.3 `Respaldo` — crear, validar y restaurar (implementado)

```cpp
namespace Respaldo {
QString construirNombreRespaldo(const QString& rutaDb, const QDateTime& ahora); // determinista
bool crearRespaldo(QSqlDatabase& db, const QString& rutaRespaldo, QString* error = nullptr);
bool validarRespaldo(const QString& rutaRespaldo, QString* error = nullptr);   // integridad + versión
bool restaurarRespaldo(const QString& rutaOrigen, const QString& rutaDestino, QString* error = nullptr);

// Crear base nueva descartando la anterior, con respaldo obligatorio:
enum class EstadoDescarte { Ok, FalloBase, FalloRespaldo, FalloDescarte, FalloCreacion };
struct ResultadoDescarte {
    EstadoDescarte estado = EstadoDescarte::FalloBase;
    QString detalle;        // texto en español (causa + acciones)
    QString rutaRespaldo;   // respaldo creado, si lo hubo (útil para restaurar)
    bool ok() const;        // true solo para Ok
};
struct OpcionesDescarte { /* ahora, respaldo, crearNueva — inyectables en tests */ };
ResultadoDescarte descartarBaseYCrearNueva(const QString& ruta,
                                           const OpcionesDescarte& opciones = {});
}
```

**Cómo lo trata el frontend:** para «Restaurar respaldo» (RF-4), llamar a
`restaurarRespaldo(origen, rutaBaseActual, &error)`; si falla, mostrar `error` y reabrir el diálogo;
si tiene éxito, reintentar la apertura.

Para «Crear base de datos nueva» (RF-4, RF-6), llamar a `descartarBaseYCrearNueva(rutaBaseActual)`:
- `Ok` → base nueva creada; continuar el arranque.
- `FalloRespaldo` → **no se descartó** nada; informar y dejar la opción deshabilitada (no se puede
  descartar sin respaldo).
- `FalloCreacion` → se descartó la anterior pero no se pudo crear la nueva; ofrecer restaurar
  `rutaRespaldo`.
- `FalloDescarte` → se respaldó pero no se pudo descartar; informar `detalle`.

### 2.4 `NucleoDatos` — fachada de dominios (implementado)

`src/backend/include/backend/services/NucleoDatos.hpp`. Fachada única sobre la conexión y los
servicios de dominio; el frontend la consume **en proceso** (ya no por IPC) para docentes,
preferencias horarias, aulas, materias, tipos de aula, cursos, planes de estudio y turnos con sus
eventos. El `NucleoDatos` **no es
dueño** de la conexión ni se instancia en la interfaz: lo construye el `ContextoBaseDatos` del módulo
`datos` (ver 2.7), que también se lo inyecta a la ventana principal. El frontend recibe un
`NucleoDatos*` ya construido y operativo.

```cpp
class NucleoDatos {
public:
    explicit NucleoDatos(QSqlDatabase& db);

    // Docentes (la tabla/servicio se llama "Profesor"; la fachada usa "Docente")
    Resultado<QVector<ProfesorDTO>> listarDocentes() const;
    Resultado<ProfesorDTO> crearDocente(const QString& id, const QString& nombre,
                                        const QString& email, const QString& telefono = QString());
    Resultado<ProfesorDTO> actualizarDocente(const QString& id, const QString& nombre,
                                             const QString& email, const QString& telefono = QString());
    bool                   eliminarDocente(const QString& id);

    // Preferencias horarias (dato distinto de la disponibilidad)
    Resultado<PreferenciaDTO> agregarPreferencia(const QString& idProfesor,
                                                 const FranjaHoraria& franja,
                                                 SentidoPreferencia sentido);
    Resultado<QVector<PreferenciaDTO>> listarPreferencias(const QString& idProfesor) const;
    Resultado<PreferenciaDTO> actualizarPreferencia(int id, const FranjaHoraria& franja,
                                                    SentidoPreferencia sentido);
    Resultado<bool> eliminarPreferencia(int id);
    Resultado<int>  limpiarPreferencias(const QString& idProfesor);

    // Aulas (el tipo es obligatorio y debe existir en el catálogo)
    Resultado<QVector<AulaDTO>> listarAulas() const;
    Resultado<AulaDTO>  crearAula(const QString& nombre, int capacidad,
                                  const QString& edificio, const QString& piso,
                                  int idTipoAula);
    Resultado<AulaDTO>  actualizarAula(int id, const QString& nombre, int capacidad,
                                       const QString& edificio, const QString& piso,
                                       int idTipoAula);
    bool                eliminarAula(int id);

    // Materias (mismo tipo obligatorio)
    Resultado<QVector<MateriaDTO>> listarMaterias() const;
    Resultado<MateriaDTO>  crearMateria(const QString& nombre, const QString& requisitos,
                                        int idTipoAula);
    Resultado<MateriaDTO>  actualizarMateria(int id, const QString& nombre,
                                             const QString& requisitos, int idTipoAula);
    bool                   eliminarMateria(int id);

    // Tipos de aula (catálogo)
    Resultado<QVector<TipoAulaDTO>> listarTiposAula() const;
    Resultado<TipoAulaDTO>          obtenerTipoAula(int id) const;
    Resultado<TipoAulaDTO>          crearTipoAula(const QString& nombre);
    Resultado<TipoAulaDTO>          renombrarTipoAula(int id, const QString& nombre);
    Resultado<bool>                 eliminarTipoAula(int id);  // error si está en uso

    // Planes de estudio
    Resultado<QVector<PlanDTO>> listarPlanes() const;
    Resultado<PlanDTO> crearPlan(const QString& codigo, const QString& nombre,
                                 const QString& descripcion = QString());
    Resultado<PlanDTO> actualizarPlan(const QString& codigo, const QString& nombre,
                                      const QString& descripcion = QString());
    bool               eliminarPlan(const QString& codigo);

    // Carga horaria de un plan (materias con sus horas y año) — RF-1.
    // La vista de generación la usa para derivar las materias de cada curso.
    // Código vacío → error; plan sin materias → vector vacío (no es error).
    Resultado<QVector<CargaHorariaDTO>> listarCargaDePlan(const QString& codigoPlan) const;

    // Capacidad horaria (horas disponibles frente a necesarias; ver §2.4.6).
    Resultado<QVector<HorasPlanAnioMateriaDTO>> horasPorPlanAnioMateria() const;
    Resultado<ResumenCapacidadHorariaDTO> resumenCapacidadHoraria(
        const QMap<QString, int>& horasAulaPorDocente = {},
        const QMap<QString, QString>& planPorCurso = {}) const;

    // Cursos (identidad ano+seccion; turno y aula opcionales)
    Resultado<QVector<CursoDTO>> listarCursos() const;
    Resultado<CursoDTO>      crearCurso(int anio, const QString& seccion,
                                        const QString& turno = QString(), int aula = -1);
    Resultado<CursoDTO>      actualizarCurso(int id, int anio, const QString& seccion,
                                             const QString& turno = QString(), int aula = -1);
    bool                     eliminarCurso(int id);

    // Turnos y eventos (los eventos son la única representación de lo que ocurre
    // dentro del turno: identidad, nombre, posición, orden y duración)
    Resultado<QVector<TurnoDTO>> listarTurnos() const;
    Resultado<TurnoDTO> obtenerTurno(const QString& nombre) const;
    Resultado<TurnoDTO> crearTurno(const QString& nombre, const QTime& inicio, const QTime& fin, int numSlots);
    Resultado<TurnoDTO> actualizarTurno(const QString& nombre, const QTime& inicio, const QTime& fin,
                                        int numSlots, bool eliminarEventosFueraDeRango = false);
    bool                eliminarTurno(const QString& nombre);
    Resultado<EventoDTO> agregarEvento(const QString& turno, const QString& nombre,
                                       int despuesDeSlot, int duracion,
                                       const QTime& inicio = QTime(), const QTime& fin = QTime(),
                                       int orden = 0);
    Resultado<EventoDTO> actualizarEvento(int id, const QString& nombre, int despuesDeSlot,
                                          int duracion, const QTime& inicio, const QTime& fin,
                                          int orden);
    bool                 eliminarEvento(int id);
    Resultado<QVector<EventoDTO>> eventosFueraDeRango(const QString& turno, int numSlots) const;

    // Eliminación en cascada (RF-2)
    QVector<Dependencia> dependientesDe(const QString& dominio, const QString& id) const;
    bool                 eliminarConCascada(const QString& dominio, const QString& id);

    // Cambios pendientes y reintento (RF-3, RNF-3, RNF-4)
    GestorPendientes&       gestorPendientes();
    const GestorPendientes& gestorPendientes() const;
    bool                    hayPendientes() const;
    Resultado<bool>         reintentarPendiente(qint64 id);
};
```

**DTOs que devuelve/recibe el frontend:**

| DTO | Campos |
|---|---|
| `ProfesorDTO` | `id`, `nombre`, `email`, `telefono`, `QVector<DisponibilidadDTO> disponibilidad`, `QVector<PreferenciaDTO> preferencias`, `QVector<MateriaAsignadaDTO> materias` |
| `AulaDTO` | `id`, `nombre`, `capacidad`, `edificio`, `piso`, `idTipoAula`, `tipoAula` (nombre del tipo, solo lectura) |
| `MateriaDTO` | `id`, `nombre`, `requisitos`, `idTipoAula`, `tipoAula` (solo lectura) |
| `TipoAulaDTO` | `id`, `nombre` |
| `PlanDTO` | `codigo`, `nombre`, `descripcion` |
| `CargaHorariaDTO` | `id`, `codigoPlanEstudio`, `idMateria`, `curso` (año), `horas`, `nombreMateria` |
| `CursoDTO` | `id`, `anio`, `seccion`, `turno`, `aula` (`-1` si no tiene) |
| `TurnoDTO` | `nombre`, `inicio`, `fin`, `numSlots`, `QVector<EventoDTO> eventos` (ordenados por posición, orden e id) |
| `EventoDTO` | `id` (`-1` al crear), `nombre`, `despuesDeSlot`, `orden`, `duracion` (minutos), `inicio`, `fin` (inválidas = ausentes) |
| `PreferenciaDTO` | `id`, `idProfesor`, `franja` (día 0–6, `inicio`, `fin`), `sentido` (`preferida`/`evitada`) |

**Contrato de las operaciones:**
- Los `listar*` devuelven `Resultado<QVector<Dto>>`: distinguen **error de lectura** (`!ok`, con
  `mensajeError`) de **sin datos** (`ok` con vector `valor` vacío). Los
  `crear*`/`actualizar*`/`asignar*`/`agregar*` devuelven `Resultado<T>`; los `eliminar*`, `quitar*`
  y `eliminarConCascada` devuelven `bool`.
- Cada escritura es atómica en su servicio. Si falla, **no queda aplicada** y se registra como
  **cambio pendiente** en el `GestorPendientes` del núcleo, con su `dominio`/`registroId` (ver 2.5).
- `eliminarConCascada` ejecuta el borrado del registro y de todos sus dependientes en **una sola
  transacción**: si falla cualquier parte, revierte y devuelve `false` sin cambios parciales.

**Cómo lo trata el frontend:** poblar las listas con los `listar*` al abrir cada vista; como ahora
devuelven `Resultado<QVector<Dto>>`, comprobar `ok`: si es `false`, mostrar `mensajeError` y **no**
presentar la lista como vacía; en las altas/modificaciones usar el `Resultado<T>`
(`ok`/`mensajeError`); antes de una baja consultar `dependientesDe` y pedir confirmación explícita
con la lista de `Dependencia::descripcion`; tras el éxito, refrescar la vista; ante error, mostrar
`mensajeError` y **no** dar la operación por hecha.

#### 2.4.1 Eliminación en cascada (`cascada.hpp`)

`src/backend/include/backend/services/cascada.hpp` describe, de forma pura (sin tocar la base), qué
depende de qué. `NucleoDatos::dependientesDe`/`eliminarConCascada` usan **estas** claves de dominio
(no las del CRUD):

| `dominio` | Tabla raíz (clave) | Dependientes que arrastra |
|---|---|---|
| `plan` | `PlanEstudio` (`codigo`) | materias del plan |
| `materia` | `Materias` (`id`) | vínculos plan-materia y profesor-materia |
| `profesor` | `Profesores` (`id`) | profesor-materia, disponibilidad y **preferencias** |
| `aula` | `Aulas` (`id`) | **anula** la referencia de los cursos con esa aula (no los borra) |
| `curso` | `Cursos` (`id`) | (ninguno) |
| `turno` | `Turnos` (`nombre`) | **borra** los eventos del turno y **anula** la referencia de los cursos del turno |

`Dependencia` es `{ QString dominio; qint64 id; QString descripcion; }` (`descripcion` ya viene en
español, lista para el diálogo). `dependientesDe` devuelve vacío para un dominio desconocido o `id`
vacío. Unas relaciones **borran** el dependiente y otras (aula→cursos, turno→cursos) solo
**anulan la referencia** (`SET NULL`): el curso permanece sin esa aula o turno.

> **Ojo con la clave `profesor`:** las operaciones CRUD de docentes son `crearDocente`/…, pero la
> cascada de un docente se pide con `dependientesDe("profesor", id)` /
> `eliminarConCascada("profesor", id)`.

**Cómo lo trata el frontend:** para una baja, `dependientesDe(dominio, id)`; si no está vacío,
listar las `descripcion` y exigir confirmación; si el usuario cancela, no llamar a
`eliminarConCascada`. Si `eliminarConCascada` devuelve `false` (sin cambios aplicados), el fallo ya
queda registrado como cambio pendiente: informar y conservar el dato en pantalla (ver 2.5).

#### 2.4.2 Eventos del turno (`ServicioTurnosEventos`)

`src/backend/include/backend/services/ServicioTurnosEventos.hpp`. Un **evento** es lo único que
ocurre dentro de un turno (un descanso, un acto, una pausa…): tiene `id` propio, nombre, posición
`despuesDeSlot` (`1..numSlots`), `orden` dentro de la posición (`>= 1`) y `duracion` en minutos.

- **Identidad y orden.** El `id` lo asigna la base y no cambia al editar; `(turno, posición, orden)`
  es único. Al crear, `orden == 0` pide el **automático** (`máximo + 1` de la posición); al editar,
  `orden` es **obligatorio**. Borrar **no renumera** los restantes.
- **Lista.** `TurnoDTO.eventos`, `listarTurnos` y `obtenerTurno` devuelven los eventos ordenados por
  `(despuesDeSlot, orden, id)`.
- **Turno inmutable.** `actualizarEvento` edita por identidad y no acepta cambiar el turno del
  evento; para moverlo de turno se borra y se crea. `despuesDeSlot = 0` se rechaza siempre: el `0`
  del DTO es un valor de construcción, **no** «sin asignar».
- **Horas.** `inicio`/`fin` son un par opcional: ambas o ninguna, `fin > inicio` y la diferencia en
  minutos debe coincidir con `duracion`. Si el evento tiene horas, su intervalo debe caber en la
  **ventana de su posición**; además no puede solaparse con otro evento del turno.
- **Duración de la franja y encaje.** La franja de clase dura **D**, la hora académica de la
  configuración global (35 por defecto; rango 30–90, §2.4.4); la rejilla **no reparte** la ventana del
  turno. La ventana de la posición *k* va del fin de la franja *k* al inicio de la *k+1*, con ancho D,
  y la última llega al fin del turno. Un turno encaja si
  `numSlots × D + Σ duracion de los eventos ≤ ventana`; si no, el servicio rechaza el alta o la
  actualización con el turno, D y el déficit, y el mensaje es el de `errorDeEncaje(turno, D)`
  (`calculo_eventos.hpp`, funciones puras; el sobrante de la ventana queda sin usar al final).
- **Reducción de franjas.** Si `actualizarTurno` reduce `numSlots` y hay eventos que quedarían fuera
  de rango, sin `eliminarEventosFueraDeRango` devuelve el error con los afectados
  (`eventosFueraDeRango(turno, numSlots)` permite listarlos antes); con `true` los borra en la misma
  transacción. Cambiar la ventana `[inicio, fin]` que deja fuera un evento con horas también se
  rechaza.
- **Cómo lo trata el frontend:** pintar los eventos por `(posición, orden)`; editar y borrar por
  `id` (nunca por posición); al reducir franjas, pedir confirmación con la lista de
  `eventosFueraDeRango`; ante `!ok`, mostrar `mensajeError` y no dar el cambio por hecho.

#### 2.4.3 Tipos de aula (`ServicioTiposAula`)

`src/backend/include/backend/services/ServicioTiposAula.hpp`. El catálogo se administra con
`crearTipoAula`/`renombrarTipoAula`/`listarTiposAula`/`eliminarTipoAula`; las aulas y materias lo
referencian por `idTipoAula`, así que renombrar un tipo se propaga sin reescribir sus filas.

- **Nombre y clave.** El servicio guarda el nombre **recortado** y una **clave normalizada**
  (`trimmed().toCaseFolded()`) con índice único en la base: «Laboratorio» y «laboratorio», o
  «Física» y «FÍSICA», son el mismo tipo aunque se escriba directo en la base. El nombre visible
  conserva mayúsculas y acentos.
- **Tipo general.** Toda base nace con el tipo general («Aula de estudio»): el catálogo nunca está
  vacío. Su alta por el servicio falla por duplicado; **no** se puede eliminar (el mensaje prioriza
  «es el tipo general» sobre la lista de dependientes) ni renombrar, ni siquiera con el mismo nombre
  o solo cambiando la grafía. El rechazo llega en español, también si una escritura directa lo
  provoca en el almacenamiento.
- **Ayuda estricta del tipo general (016).** `TiposAulaUI::idTipoGeneral`
  (`src/frontend/src/services/tipos_aula_ui.hpp`) devuelve `Resultado<int>`: **solo** el tipo general
  (comparado con `tipo_general::esNombreCanonico`, recorte + plegado) o un fallo; **nunca** otro tipo
  del catálogo. Si el catálogo está vacío o no tiene el general, el mensaje dice qué falta y qué
  hacer (restaurar un respaldo de la versión vigente o crear una base nueva), sin errores técnicos.
  Se rechazan las altas que usan la ayuda —aula, materia y materia rápida desde docente, en alta y
  edición— sin abrir el formulario o sin escribir nada. Una base nueva ya trae el tipo general, y el
  catálogo **no gana campos ni firmas nuevas**. Con esto queda **atendida la deuda de la 009 sobre
  esta ayuda**; la **pantalla** de gestión del catálogo sigue pendiente (§5).
- **Tipo obligatorio.** `crearAula`/`actualizarAula`/`crearMateria`/`actualizarMateria` exigen
  `idTipoAula > 0` y existente. Actualizar **sin** tipo no conserva el anterior: se rechaza.
- **Borrado en uso.** `eliminarTipoAula` de un tipo especializado devuelve error con la lista de
  aulas y materias que lo usan y **no borra nada**; no hay cascada ni `SET NULL`.
- **Cómo lo trata el frontend:** cargar el catálogo con `listarTiposAula()` en los formularios de
  aula y materia (desplegable obligatorio), ofrecer «gestionar tipos» con el CRUD y, antes de
  eliminar, mostrar `mensajeError` con los dependientes (no se confirma).

#### 2.4.4 Duración de la hora académica (`ServicioConfiguracionHoraria`)

`src/backend/include/backend/services/ServicioConfiguracionHoraria.hpp`. Configuración **global y
única** por base: una sola fila (`id = 1`); no hay valores por turno, curso, docente ni generación.

- **DTO.** `ConfiguracionHoraria{int duracionMinutos; bool porDefecto;}`: el valor efectivo y si
  proviene del defecto (35) porque no hay fila.
- **Consulta.** `consultar()` devuelve el valor efectivo; sin fila devuelve `{35, porDefecto = true}`
  y **no** es error. Solo falla si la lectura de la base falla. **No** comprueba la validez de la
  base (016): la pantalla sigue leyendo el valor aunque la base esté rota; solo se rechaza el
  guardado.
- **Edición.** `guardar(int)` comprueba **primero la validez de la base** (016): si no es válida,
  devuelve el mensaje de la causa más «No se guardó la configuración de la hora académica.» y **no
  escribe**. Después valida el rango 30–90 con `duracion_horaria::errorDeRango` (punto único, el
  mismo que V2 del contrato) y hace *upsert* de la fila. Una duración inválida devuelve el mensaje
  (campo, valor, rango y acción) y **no cambia** el valor anterior.
- **Fachada.** `NucleoDatos::configuracionHoraria()` y `guardarConfiguracionHoraria(int)` son la vía
  del frontend.
- **Punto único horas → minutos.** `duracion_horaria::minutosDeHorasAcademicas(horas, D)` es la única
  conversión de horas académicas a minutos (40 h × 35 = 1400 min). El tope de **40 horas académicas**
  semanales por año no cambia.
- **Cómo lo trata el frontend:** pedir el valor con `configuracionHoraria()` y mostrar
  `porDefecto`/origen; editar con `guardarConfiguracionHoraria()` y, ante `!ok`, mostrar
  `mensajeError` sin dar el cambio por hecho. La aritmética en minutos de las vistas es deuda de su
  equipo (ver §6).

#### 2.4.5 Preferencias horarias (`ServicioPreferencias`)

`src/backend/include/backend/services/ServicioPreferencias.hpp`. Registra cuándo **prefiere** o
**evita** dar clase un docente. Es un dato **distinto** de la disponibilidad (que es factibilidad) y
**no** interviene todavía en la generación.

- **Modelo.** `PreferenciaDTO{int id; QString idProfesor; FranjaHoraria franja; SentidoPreferencia
  sentido;}`: docente, día, hora de inicio, hora de fin y sentido (`preferida`/`evitada`), con
  **identidad propia** (`id`). El docente no cambia al actualizar: para mover una preferencia de
  docente se borra y se crea.
- **Día.** Entero **0–6** (0 = domingo … 6 = sábado), el mismo rango que la disponibilidad. Sábado y
  domingo se guardan y se exponen, pero **no intervienen en la generación actual** (lunes–viernes).
- **Validación.** Docente existente; día 0–6; horas válidas con inicio anterior al fin; sentido válido;
  **sin solapes de sentidos opuestos** en el mismo docente y día; **sin duplicado exacto** (docente,
  día, rango y sentido). Los solapes y rangos contiguos del **mismo sentido** se aceptan y no se
  fusionan. Cambiar el sentido o el día se revalida contra los opuestos; si contradice, se rechaza y se
  conserva el valor anterior.
- **Orden.** `listar()` devuelve por día, hora de inicio, hora de fin e `id`.
- **Operaciones.** `agregar`, `listar`, `actualizar`, `eliminar` y `limpiar`; todas devuelven
  `Resultado<T>` (`eliminar`, `Resultado<bool>`; `limpiar`, `Resultado<int>`), con mensajes en español
  que identifican campo, valor, causa y **acción**. `limpiar` no toca la disponibilidad.
- **Baja y cascada.** `eliminarDocente` y `eliminarConCascada` borran preferencias y disponibilidad en
  una transacción; `dependientesDe("profesor", id)` las muestra antes de confirmar.
- **Efecto en la generación (013).** Desde la 013 las preferencias **sí** intervienen: las que
  **intervienen** entran en la huella (`huellaDatos(datos, D, intervinientes)`) y un cambio suyo marca
  el horario como «datos anteriores»; las que **no intervienen** no la cambian.
  `ProfesorDTO::toProfesor()` sigue ignorándolas (el modelo del solver no cambia), y el resultado de la
  generación informa el estado de cada preferencia y el **cumplimiento** agregado.
- **Cómo lo trata el frontend:** cargar `ProfesorDTO.preferencias` desde `listarDocentes`, usar las
  cinco operaciones de `NucleoDatos`, separar preferencias de disponibilidad y mostrar el día con su
  nombre. Los cambios de firma son **aditivos**: el DTO gana el campo y `NucleoDatos` gana
  métodos; no rompen la compilación.

#### 2.4.6 Capacidad horaria (`ServicioCapacidadHoraria`)

`src/backend/include/backend/services/ServicioCapacidadHoraria.hpp`. Servicio **de solo lectura** que
compara las horas de aula disponibles entre los docentes con las horas necesarias para cubrir los
cursos, y detalla las horas de cada materia en cada año de cada plan. No cambia datos ni interviene
en la generación.

- **Qué calcula.**
  - `horasDisponibles`: Σ de las horas de aula de los docentes.
  - `horasNecesarias`: Σ, para cada curso con plan, de las horas del plan en el **año** de ese curso.
  - `detalle`: todas las filas de `PlanEstudio_Materia` de todos los planes, ordenadas por plan,
    año y materia (`HorasPlanAnioMateriaDTO{codigoPlan, nombrePlan, anio, idMateria, nombreMateria,
    horas}`).
- **Datos que aporta el frontend.** La base **no persiste** las horas de aula por docente (el 36 es
  política del perfil) ni el plan de cada curso (`Cursos` guarda año/sección/turno, no plan). Por eso
  `resumen()` recibe dos mapas:
  - `horasAulaPorDocente`: clave **nombre del docente**, valor horas semanales. Si llega **vacío**, se
    usan **36 horas por docente** y se añade un aviso. Un nombre que no exista en la base se ignora
    con aviso.
  - `planPorCurso`: clave **nombre del curso** tal como `nombreCurso(anio, seccion)` («1ro A»),
    valor código de plan. Si llega **vacío**, `horasNecesarias = 0` y se avisa de que faltan planes;
    el `detalle` por plan/año/materia se calcula igual. Un curso sin plan o con un plan inexistente
    se omite con aviso.
- **Errores y avisos.** `Resultado` solo es error si falla una lectura de la base. Los datos
  incompletos (sin mapa de horas, sin mapa de planes, curso/plan/docente desconocido) van en
  `avisos` (`QStringList`), en español.
- **Fachada.** `NucleoDatos::horasPorPlanAnioMateria()` y
  `NucleoDatos::resumenCapacidadHoraria(horasAulaPorDocente, planPorCurso)` delegan en el servicio.

```cpp
// El frontend arma los mapas con lo que muestra su tabla de generación/preset.
QMap<QString, int>    horasAula;   // «Ana Pérez» → 30
QMap<QString, QString> planes;     // «1ro A»     → «31059»
horasAula.insert(QStringLiteral("Ana Pérez"), 30);
planes.insert(nombreCurso(1, QStringLiteral("A")), QStringLiteral("31059"));

const auto resumen = nucleo->resumenCapacidadHoraria(horasAula, planes);
if (!resumen.ok) {
    // error de lectura: mostrar resumen.mensajeError
} else {
    // resumen.valor.docentes / horasDisponibles / cursos / horasNecesarias
    // resumen.valor.detalle  (95 filas con el centro del seed)
    // resumen.valor.avisos   (datos incompletos, nunca un error)
}
```

### 2.5 Estado por registro y cambios pendientes (implementado)

`src/backend/include/backend/services/GestorPendientes.hpp`. Cuando una escritura falla, no se
pierde: queda registrada en memoria como **cambio pendiente** reintentable (RF-3, RNF-3, RNF-4).

```cpp
enum class TipoOperacion { Alta, Modificacion, Baja };
enum class EstadoPendiente { Guardado, PendienteGuardar, PendienteEliminar };

struct OperacionPendiente {
    qint64  id = 0;                 // identidad de la operación (la asigna el gestor)
    QString dominio;                // dominio lógico (ver tabla siguiente)
    QString registroId;             // clave del registro afectado (para el estado por fila)
    TipoOperacion tipo = TipoOperacion::Alta;
    bool    pendiente = false;      // true si la escritura falló y sigue pendiente
    QString descripcion;            // texto en español para la interfaz
    std::function<bool()> accion;   // repite la escritura; true = éxito
};

EstadoPendiente estadoPendienteDe(const OperacionPendiente&);   // pura

class GestorPendientes {
public:
    qint64                        registrar(OperacionPendiente operacion);
    bool                          hayPendientes() const;                        // guardia de cierre (RF-6)
    QVector<OperacionPendiente>   pendientes() const;
    Resultado<bool>               reintentar(qint64 id);
    EstadoPendiente               estadoDe(const QString& dominio, const QString& registroId) const;
    const OperacionPendiente*     pendienteDe(const QString& dominio, const QString& registroId) const;
    int                           contar() const;
    void                          limpiar();
};
```

`estadoPendienteDe` es pura: una operación por defecto (`pendiente == false`) es `Guardado`; una
alta o modificación pendiente es `PendienteGuardar`; una baja pendiente es `PendienteEliminar`.
`NucleoDatos` expone su gestor con `gestorPendientes()`, más `hayPendientes()` y
`reintentarPendiente(id)` como atajos.

**Claves con las que `NucleoDatos` registra los pendientes** (para llamar a
`gestorPendientes().estadoDe(dominio, registroId)` y pintar la fila correcta):

| `dominio` | Operación | `registroId` |
|---|---|---|
| `docente` | alta / modificación / baja | id del docente |
| `aula` | alta | nombre del aula (aún no hay id) |
| `aula` | modificación / baja | id del aula (como texto) |
| `materia` | alta | nombre |
| `materia` | modificación / baja | id (como texto) |
| `plan` | alta / modificación / baja | código del plan |
| `curso` | alta | identidad `"<anio>-<seccion>"` normalizada |
| `curso` | modificación / baja | id (como texto) |
| `turno` | alta / modificación / baja | nombre del turno |
| `evento` | alta | `"<turno>-<despuesDeSlot>-<orden>"` (con el `orden` solicitado; `0` si se pidió el automático) |
| `evento` | modificación / baja | id del evento (como texto) |

> `eliminarConCascada(dominio, id)` registra el pendiente con el `dominio` de la cascada (p. ej.
> `"profesor"`) y `registroId = id`.

**Cómo lo trata el frontend:** `Guardado` → fila normal; `PendienteGuardar` → fila resaltada +
acción «Reintentar» (`reintentarPendiente(id)`), sin retirarla de pantalla; `PendienteEliminar` →
fila atenuada. Al cerrar, si `hayPendientes()`, mostrar un aviso cancelable que enumere
`pendientes()` (RF-6). El reintento que vuelve a fallar **conserva** el pendiente.

### 2.6 `InstanciaUnica` — una sola instancia y enfoque (implementado)

`src/app/include/app/instancia_unica.hpp` (librería `app_core`). El arranque la crea **antes** de
abrir la base y la mantiene activa durante toda la sesión, incluida la migración y el diálogo de
fallo (RF-5).

```cpp
class InstanciaUnica : public QObject {
    Q_OBJECT
public:
    static const QString NOMBRE_POR_DEFECTO;   // "GestorHorarios_InstanciaUnica"
    enum class Resultado { Primaria, Secundaria, SinAcuse };

    explicit InstanciaUnica(const QString& nombre = NOMBRE_POR_DEFECTO,
                            int plazoAcuseMs = 10000, QObject* padre = nullptr);

    Resultado iniciar();          // deja el servidor escuchando si somos la principal
    Resultado resultado() const;  // desenlace del último iniciar()
    QString   detalle() const;    // texto en español (vacío si no hay incidencia)
    bool      esPrimaria() const; // resultado() == Primaria
    bool      estaActiva() const; // el servidor de detección sigue escuchando

signals:
    void activarSolicitada();     // otra instancia pidió enfocar esta
};
```

**Estados:** `Primaria` (no había otra: continuar el arranque), `Secundaria` (había otra y se
enfocó, acuse recibido: salir sin arrancar), `SinAcuse` (había otra pero no respondió en plazo:
avisar con `detalle()` y no arrancar).

**Cómo lo trata el frontend:** el arranque del cliente (`src/app`) ya conecta
`activarSolicitada()` a `MainWindow` (`showNormal()` + `raise()` + `activateWindow()`), de modo que
un segundo arranque trae la ventana al frente. El equipo de frontend puede extender ese enfoque al
diálogo de fallo/migración que tenga abierto. Con `Secundaria` salir con éxito; con `SinAcuse`
mostrar `detalle()` y salir con error.

### 2.7 `ContextoBaseDatos` — ciclo de vida de la base del cliente (implementado)

`src/datos/include/datos/ContextoBaseDatos.hpp` (módulo `datos`, librería `datos`). Encapsula lo
que antes hacía el arranque del cliente: resolver la ruta por defecto, abrir/crear/migrar/respaldar
la base (`AperturaBaseDatos`), mantener la conexión persistente y construir el `NucleoDatos`. No
depende de Qt Widgets, por lo que es testeable sin UI.

```cpp
class ContextoBaseDatos {
public:
    struct Opciones { QString nombreConexion; AperturaBaseDatos::Opciones apertura; };
    ContextoBaseDatos();
    ~ContextoBaseDatos();                    // RAII: cierra y libera la conexión
    ContextoBaseDatos(const ContextoBaseDatos&) = delete;
    ContextoBaseDatos& operator=(const ContextoBaseDatos&) = delete;

    AperturaBaseDatos::Resultado abrir(const QString& ruta);
    AperturaBaseDatos::Resultado abrir(const QString& ruta, const Opciones& opciones);

    bool               abierto() const;
    NucleoDatos&       nucleo();             // precondición: abierto()
    const NucleoDatos& nucleo() const;
    QSqlDatabase&      conexion();
    QString            ruta() const;
    static QString     rutaPorDefecto();
};
```

**Quién lo usa.** El arranque del cliente (`src/app`, `ejecutarAplicacionFrontend`) crea un
`ContextoBaseDatos`, llama a `abrir(ContextoBaseDatos::rutaPorDefecto())` y, si la apertura es
`ok()`, inyecta `&contexto.nucleo()` en la ventana principal con `MainWindow::setNucleoDatos(...)`.
El contexto vive durante toda la sesión (RAII) y se destruye al cerrar. El proceso de cálculo
(`--backend`) **no** usa este componente (RF-2).

**Cómo lo trata el frontend:**
- No instancia la base ni resuelve su ruta: la recibe ya abierta. La ventana guarda el
  `NucleoDatos*` inyectado (puede ser `nullptr` si el binario se compiló sin persistencia).
- Si `abrir()` no es `ok()`, el arranque (no la ventana) muestra el **diálogo de recuperación de
  RF-4** con `Resultado::detalle` como causa: «Reintentar», «Restaurar respaldo…», «Crear base de
  datos nueva…» (con respaldo obligatorio y confirmación previa) y «Salir». Reintenta hasta que la
  apertura tiene éxito o el usuario sale; implementado en
  `src/app/src/aplicacion_frontend.cpp`. La ventana principal no se levanta hasta que la base abre.

---

## 3. Generación de horarios (spec 002)

### 3.1 `ServicioGeneracion` — orquestar una generación (implementado)

`src/backend/include/backend/services/ServicioGeneracion.hpp`. UI-free (QObject con señal
`finalizada()`). Recibe un `PuertoSolver` (el arranque inyecta el cliente IPC real; los tests, un
doble) y un plazo (300 s por defecto). El horario resultante es **válido** con las reglas
vigentes; con 8 workers y semilla fija **no se promete el mismo horario entre ejecuciones**.

```cpp
struct ResultadoGeneracion {
    enum class Estado { Inactivo, Calculando, Listo, DatosAnteriores, NoFactible,
                        ContratoInvalido, CalculoFallido, TiempoAgotado, EntradaInvalida,
                        BaseInvalida };
    Estado        estado = Estado::Inactivo;
    QString       mensaje;
    bool          datosAnteriores = false;
    HorarioSalida horario;      // válido si presentable
    AnalisisSalida analisis;    // aviso P2 y conflictos P4
    bool          presentable = false;
    CumplimientoPreferencias cumplimiento;  // 013: estado por preferencia y agregado
};

class ServicioGeneracion : public QObject {
    Q_OBJECT
public:
    ServicioGeneracion(PuertoSolver& puerto, int plazoMs = 300000,
                       std::function<QString()> huellaActual = {},
                       std::function<QDateTime()> reloj = {},
                       std::function<QString()> revisarBase = {}, QObject* padre = nullptr);
    void generar(const QJsonObject& entrada, const QString& huella,
                 const QVector<PreferenciaNoAplicada>& noAplicadas = {});  // 013
    bool enCurso() const;
    const ResultadoGeneracion& ultimoResultado() const;
signals:
    void finalizada();
};
```

**Cómo lo trata el frontend:**
- «Generar» deshabilitado mientras `enCurso()`; al terminar se emite `finalizada()` y se lee
  `ultimoResultado()`.
- `Listo` → mostrar `horario` + `analisis.avisos`.
- `DatosAnteriores` → aviso destacado + confirmación obligatoria antes de guardar.
- `NoFactible` / `ContratoInvalido` / `CalculoFallido` → informar; **no** ofrecer guardar.
- `TiempoAgotado` → informar; la app sigue operando (la respuesta tardía se ignora).
- `EntradaInvalida` → mostrar `mensaje` (motivo de la validación previa); no se envió nada.
- `BaseInvalida` → `revisarBase()` devolvió un mensaje con causa y acción: **no** se valida la entrada
  ni se envía nada al puerto; mostrar ese `mensaje`. Tiene prioridad sobre `EntradaInvalida`.

La entrada la construye `construirEntradaCompleta` (013, `src/backend`) a partir de `DatosDominio`
(dominios) + preset; devuelve `EntradaConstruida{json, intervinientes, noAplicadas}` y se valida con
`validarEntradaSolver` (**V1–V19**). `construirJsonEntrada` se mantiene como **envoltorio** que
devuelve solo el JSON (la firma de la 011), y la fachada `ConstructorEntradaSolver::construir` no
cambia. La huella (`huellaDatos(datos, D, intervinientes)`) detecta «datos anteriores» y **no incluye
el catálogo de tipos de aula**: solo la identidad que referencia cada aula o materia (`idTipoAula`), no
el nombre visible del tipo.

> **Aforo del motor (spec 018).** El **aforo de la cancha** (2) frente al resto de tipos (1) es una
> **regla del motor**: **no hay campo en el DTO** (`AulaDTO` sigue con nombre, capacidad, edificio,
> piso y tipo), **no** viaja en el contrato ni en el preset y **no** entra en la huella de datos. La
> **causa (2)** de un `NoFactible` cita ahora **demanda, aulas, franjas y aforo**: «Causa (2) conflicto
> de aula: el tipo «‹tipo›» necesita ‹demanda› bloques y sus ‹aulas› aulas ofrecen ‹capacidad› plazas
> (‹franjas› franjas × aforo ‹aforo›).». El frontend **no** calcula ni muestra el aforo: solo presenta
> el mensaje.
>
> **Fuera de alcance de la 018:** la **vista de horario** y su **estimación de ocupación** (pintar dos
> cursos en la misma aula y franja, o avisar del aforo en la rejilla) **no** forman parte de esta
> entrega y **no son deuda** de la misma: `src/frontend/src` no se toca.

**Preferencias en la generación (013).** El frontend pasa a `generar` el **diagnóstico** de
preferencias no aplicadas (`Composicion::preferenciasNoAplicadas`, con su motivo) y lee
`ResultadoGeneracion::cumplimiento` (`respetadas`, `incumplidas`, `noAplicadas`, más el estado de cada
preferencia con su día de dominio y, en las evitadas incumplidas, las clases que la incumplen). Es un
campo **aditivo**: el **contrato de salida del solver no cambia** ni el estado viaja en su JSON. La
vista comprueba la validez de la base **antes de componer** la entrada y cablea `revisarBase` en el
servicio.

### 3.2 `CargadorConfiguracionSolver` — config y presets en JSON (implementado)

`src/backend/include/backend/services/CargadorConfiguracionSolver.hpp`. Sin base de datos ni
OR-Tools (lógica del lado cliente).

```cpp
class CargadorConfiguracionSolver {
public:
    static Resultado<SolverConfig> cargar(const QString& ruta);           // + validación fromJson
    static Resultado<bool>        guardar(const QString& ruta, const SolverConfig& config);
    static Resultado<QStringList> listarPresets(const QString& directorio); // *.json ordenados
};
```

Un «preset» es un archivo JSON con el mismo formato que la configuración, salvo que **omite** la
sección opcional `preferencias` (013). El contrato tiene **diez secciones obligatorias y dos
opcionales** (`aula_fija` y `preferencias`); `meta` no es una sección.
**Frontend:** cargar/guardar presets como archivos; ante fallo (`!ok`), mostrar `mensajeError`
(causa + acciones) y permitir reintentar.

### 3.3 `HorarioSalida` y guardado como archivo (implementado/en consolidación)

- `HorarioSalida`: `metadata` + `horarios` por curso (`DiaOutput` → `AsignacionOutput`:
  `slot`, `materia`, `profesor`, `aula`); `toJson()`/`fromJson()`.
- `ServicioHorarioSalida`: `guardarHorario(...)`, `cargarHorario(...)`, `listarArchivos()`.

**Frontend:** pintar el horario (curso × día/slot) + avisos; guardar solo si el usuario lo pide
(diálogo de archivo del frontend); ante fallo de escritura, mantener el contenido y reintentar.

### 3.4 `AnalisisSalida` — validaciones posteriores (implementado)

`ValidadorSalidaSolver` / `analizarSalidaSolver` (pura): **P2 exceso de horas de docente** →
`avisos`; **P4 solapamiento** (docente/aula/curso) → `conflictos` y `presentable() == false`.
Los avisos de **P1 horas no cubiertas** y **P3 capacidad de aula** están **retirados por la 005
(decisión I2) hasta nuevo aviso**: no se emiten. **Frontend:** panel de avisos junto al horario;
nunca mostrar como válido un resultado con `hayConflictoSolapamiento()`.

### 3.5 Proceso de cálculo y salud (implementado)

El modo `--backend` resuelve por la ruta `solver_resolve` (JSON de entrada → `HorarioSalida` JSON, o
`RESP_SIN_SOLUCION`) y **nunca abre la base de datos** (RF-2). Al arrancar se comprueba su salud y
al cerrar se solicita apagado ordenado; el cierre continúa aunque no responda. **Frontend:** aviso
no bloqueante al arrancar.

> **Windows/MSVC — generar en Release.** Las DLL de OR-Tools precompiladas usan la CRT de release
> (`/MD`). Un binario **Debug** (`/MDd`) mezcla el *heap* y el proceso de cálculo **colapsa al
> resolver** con `0xC0000374` (corrupción de memoria); además, `test_solver_horarios` falla igual en
> Debug. Por eso la generación se prueba y se usa en un build **Release**: el preset `full-release`
> (`build-rel/`, `CMAKE_BUILD_TYPE=Release`) con el `bin` de OR-Tools en el `PATH`. `CMakeLists`
> avisa al configurar en Debug y, en esa combinación, define `GESTOR_SOLVER_REQUIERE_RELEASE`:
> «Generar horarios» **no envía** la petición y explica la salida (mejor que llegar a la caída),
> además de traducir el código `0xC0000374` en el diálogo del backend. El preset `full` (`build/`)
> está pensado para **Docker**, donde no aplica este desajuste (ver `docs/guias/desarrollo/CMAKE_GUIDE.md`).

### 3.6 Composición y envío desde el frontend (spec 005, implementado)

El backend expone las piezas, pero **quien posee el dominio y la configuración es la interfaz**; por
eso la composición vive en el frontend, en `src/frontend/src/services/` (se compilan solo si
`GESTOR_TIENE_BACKEND`, es decir, cuando el target `backend` existe):

- **`generacion_tipos.hpp`** — tipos puros compartidos (`OpcionesUI` con franja, dimensiones,
  planificación, penalizaciones y parámetros por curso/docente —`ParametrosCurso` lleva `plan`,
  `turno` y `num_estudiantes`, y `ParametrosDocente` horas, turno, plan, años y suplencias—;
  `ContextoHorario` con los nombres índice→texto de la última entrada).
- **`entrada_solver_builder.{hpp,cpp}`** — `GeneracionSolver::`:
  - `construirEntrada(nucleo, opciones)` → `Composicion{entrada, contexto, avisos, huella,
    preferenciasNoAplicadas}`. Reutiliza `construirEntradaCompleta` (backend, 013) y **después inyecta
    por curso** el `plan`, el `turno` y el
    `num_estudiantes` (las `materias` se derivan del plan embebido y el año del curso vía
    `NucleoDatos::listarCargaDePlan`), y aplica la opción global de aulas fijas. El `turno` **ya
    viene en la base** con el centro real (12 Mañana / 12 Tarde) y el preset lo replica; el
    `aula` la toma el constructor desde la base. `avisos` recoge los datos incompletos (curso sin
    plan, plan sin materias, lecturas fallidas).
  - `horasSugeridasPorDocente(nucleo, cursos)` → precarga válida de horas por docente.
  - `aFormatoVisualizacion(horario, contexto)` → JSON con `teachers`/`years` (más un bloque `_meta`
    informativo y un bloque `_cuadricula` con la rejilla usada: duración del bloque, turnos y recesos)
    que entienden la sección de visualización y sus diálogos. Traduce los índices del
    `HorarioSalida` a nombres y el día del solver (`0 = lunes`) a las claves `"1".."5"`; las franjas
    válidas son las de la configuración (`num_slots_dia`), no una plantilla fija, y cada sección
    declara su `turno`.
- **`puerto_solver_ipc.{hpp,cpp}`** — `PuertoSolverIpc`: implementa `PuertoSolver` sobre
  `InternalClient`, envía la op `Middleware::OP_RESOLVER_HORARIO` y correlaciona la respuesta
  (`RESP_EXITO` → `ok && factible`; `RESP_SIN_SOLUCION` → `ok && !factible`; otro → `!ok`).

**Vista `ScheduleGenerationWidget`** (`src/frontend/src/views/schedule_generation_widget.{hpp,cpp}`)
es la vista «Generación de horario» (índice 4 del `MainWindow`). Con `GESTOR_TIENE_BACKEND` muestra:
parámetros generales (franja, dimensiones, planificación, penalizaciones, aulas fijas), la tabla de
**cursos** (turno + plan + nº de estudiantes), la tabla de **docentes** (horas requeridas/en aula,
turno, plan, años y materias suplentes) y botones de **preset** (`CargadorConfiguracionSolver`; el
preset es la propia entrada —diez secciones obligatorias y dos opcionales—, que **omite**
`preferencias`). Al pulsar «Generar horarios»: valida, compone la
entrada, la envía con `ServicioGeneracion` y, si `presentable`, guarda el JSON en la carpeta de
horarios y emite `horarioGenerado(datosVisualizacion, rutaArchivo)`. Sin backend muestra un aviso
informativo.

Al cargar la base por primera vez, la vista **deriva de los turnos** el nº de slots por día (suma de
los slots de todos los turnos) y la **duración del bloque** (franja del turno menos la **suma de las
duraciones de sus eventos**, sobre sus slots; **40 min** en el centro real: `(340 − 20) / 8` y
`(330 − 10) / 8`), ambos editables. La fórmula es la misma de `calculo_eventos` (§2.4.2). El campo
`duracion_minutos` del
preset es **derivado**: la construcción (`construirEntradaCompleta`; el envoltorio
`construirJsonEntrada`) lo sobrescribe siempre con el de la configuración
global y su valor nunca se usa; un cambio de configuración desde que se guardó el preset no es error
ni aviso. La columna **Turno** de cada curso
es un desplegable con los turnos de la base y una opción «Sin turno» para los cursos sin asignar; en
el centro real los **24 cursos ya traen su turno** (12 Mañana / 12 Tarde), la configuración no lo
sobrescribe (D-8, capacidad futura de la spec 006) y el solver exige uno (V7).

**Controles de las tablas.** Las celdas editables llevan widgets, no texto libre: turno y plan son
desplegables, las horas son `QSpinBox`, los **años** son un desplegable de **selección múltiple**
(`widgets/combo_multiple.{hpp,cpp}`: casillas que no cierran el popup al marcar, con el resumen de lo
elegido en el propio control) y las **materias suplentes** un botón que resume la elección y abre
`SelectorMateriasDialog` (`widgets/selector_materias.{hpp,cpp}`, rejilla de materias marcables con
contador). Las dos tablas fijan `verticalHeader()->setDefaultSectionSize(40)` para dar cabida al
control de celda (32 px) más el relleno del ítem y que quede centrado, y `aplicarEstilo()` fija la
**paleta de la vista del popup** de cada desplegable (`Base`/`Window` = superficie del tema, `Text`,
`Highlight` = acento): sin eso el popup heredaba un brush nulo y el delegado pintaba los ítems en
negro en el tema claro.

**Cableado (`MainWindow`):** el `InternalClient` se guarda como miembro y se inyecta en las vistas;
en `setNucleoDatos` se lo reenvía a la vista de generación con el `NucleoDatos*`. Al recibir
`horarioGenerado`, la ventana llama a `ScheduleVisualizationWidget::loadSchedules(...)` y salta a la
sección de visualización para que el horario quede cargado y editable. La carpeta de destino se
obtiene de `ScheduleVisualizationWidget::schedulesDirectory()`.

**Visualización dirigida por datos (`dialogs/`):** el documento que consumen los diálogos de horarios
lleva, además de `teachers`/`years` y `_meta`, un bloque **`_cuadricula`** con la rejilla con la que se
generó: `duracion_minutos`, `num_dias`, `num_slots_dia`, `turnos` (nombre, franjas `slot` y hora de
`inicio`) y `recesos` (`turno`, `despues_de_slot`, `duracion`, `inicio`, `fin`); en el motor estos
`recesos` son los **eventos** del turno: no ocupan franja y **definen los segmentos** del bloque. Cada
sección declara además su `turno`. `ScheduleModel::rowsFor` construye las filas de cada horario con esos datos: las
franjas del turno del curso (o de todos los turnos si es un docente), la hora de cada bloque
(`inicio + k·duración + recesos previos`) y una fila de receso con su duración real. Si el documento
no trae `_cuadricula` (archivo de ejemplo integrado y horarios guardados por versiones anteriores) se
usa la plantilla histórica de 12 franjas de 45 min con receso de 30 min. `largoDeTramo` une las
franjas seguidas de la misma clase (misma materia, profesor y aula) en **un solo bloque**, que se
pinta con `rowSpan` y se arrastra entero (`planMoveRun`); un receso en medio corta la unión.

**Mover un bloque.** `planMoveRun` traduce el arrastre a una lista de **traslados elementales**
(`SlotMove`) y `applyMove` los aplica leyendo todas las franjas de origen del estado original antes de
escribir (una biyección sobre las franjas afectadas). Si el destino y el origen no se solapan —o son de
días distintos— es el intercambio de siempre: lo que ocupaba el destino pasa a las franjas de origen. Si
el destino **solapa el propio tramo** (se suelta la clase una o dos franjas más arriba o más abajo), el
tramo se **desplaza** y las clases que ocupaban las franjas ganadas pasan a las que el tramo deja libres:
la clase conserva su tamaño y ninguna otra desaparece (`plan.rotation`). Con `swapOthers` (el usuario
aceptó el aviso de conflicto) la misma lista de traslados se aplica a cada horario en conflicto.

**Una clase no se parte.** Un movimiento solo es admisible si las clases que ya ocupan el destino caben
**enteras** dentro de él: `parteUnaClase` (con `tramoQueContiene`) localiza la clase que quedaría
cortada. Soltar una materia de una hora sobre la primera franja de una de dos horas —o una clase grande
junto a otra grande— partiría la clase ajena (de dos horas pasaría a una), así que el diálogo lo avisa
(«El destino partiría una clase») y **no mueve nada**: hay que mover entera la clase del destino o
soltar la arrastrada sobre franjas libres. En cambio, intercambiar una clase de dos franjas con dos de
una **sí** es admisible (las dos caben enteras): la grande ocupa esas dos franjas y las pequeñas pasan
a las que aquella deja libres.

**El bloque cae donde se ve.** Al arrastrar una clase de varias franjas el `QDrag` lleva el bloque
entero y anota la franja agarrada (`grab`); al soltarlo se calcula
`toSlot = (franja bajo el puntero en la celda destino) − grab`, de modo que el bloque cae donde se ve.
Antes se usaba la franja del puntero como primera franja del bloque, así que agarrarlo por su segunda
franja lo dejaba una franja más abajo y el intercambio parecía no producirse.

El conversor `aFormatoVisualizacion` está cubierto por el test `test_frontend_conversion`
(`test/frontend/test_conversion_entrada.cpp`, target añadido en `test/CMakeLists.txt` cuando existen
`frontend` y `backend`). La rejilla (filas por turno, receso, horas y tramos) por
`test_cuadricula_horario` (`test/frontend/test_cuadricula_horario.cpp`).

La integración del centro real (sembrar la base, componer la entrada desde el frontend con el
plan, el turno y la matrícula de cada curso, y validarla) está cubierta por `test_entrada_seed`
(`test/frontend/test_entrada_seed.cpp`) y, con **generación real**, por `test_solver_centro_real`
(etiqueta `slow`, spec 018). El conjunto **se declara generable** con el aforo 2 de la cancha
(48 clases de Educación Física por turno ≤ 80 plazas y 24 pares ≤ 40 plazas de par): el test lento
resuelve **876 clases** en menos de 300 s y lo comprueba. El antiguo `test_generacion_seed` se retiró
en la 017.


---

## 4. Resumen: elemento expuesto → acción del frontend

| Elemento expuesto | Acción del frontend | RF |
|---|---|---|
| `AperturaBaseDatos::Resultado` | Diálogo de fallo (Reintentar/Restaurar/Crear nueva/Salir) | RF-4, RF-6 |
| `Respaldo::restaurarRespaldo` | Restaurar respaldo elegido y reintentar apertura | RF-4 |
| `NucleoDatos` | Listas desde la base y CRUD en proceso | RF-2 |
| `ContextoBaseDatos` (`src/datos`) | El arranque lo crea y entrega `NucleoDatos*` a la ventana (`setNucleoDatos`) | RF-1, RF-2 |
| `NucleoDatos::dependientesDe` / `eliminarConCascada` | Confirmación de cascada | RF-2 |
| `EventoDTO` / CRUD de eventos | Alta, edición y baja por identidad; eventos por posición y orden | RF-1, RF-2 |
| `PreferenciaDTO` / CRUD de preferencias | Alta, consulta, edición y baja por identidad; día 0–6 y sentido; separadas de la disponibilidad; la cascada del docente las lista | RF-1, RF-2 (012) |
| `TipoAulaDTO` / CRUD del catálogo | Desplegable obligatorio en aulas y materias; gestión del catálogo | RF-5 |
| `ConfiguracionHoraria` / `ServicioConfiguracionHoraria` | Valor efectivo y origen; editar 30–90 (inválida no cambia el valor) | RF-1, RF-2 |
| `duracion_horaria::errorDeRango` / `errorDeEncaje` | Mensajes del rango y del encaje del turno (turno, D, déficit) | RF-1, RF-4 |
| `construirEntradaCompleta(datos, preset, D, ahora)` (013; `construirJsonEntrada` es el envoltorio) | Pasar la D efectiva; el preset nunca manda la duración | RF-5, RF-2 (013) |
| `estadoPendienteDe` / `GestorPendientes` | Marca por fila + «Reintentar»; aviso al cerrar | RF-3, RF-6, RNF-3/4 |
| `InstanciaUnica` | Enfocar la instancia existente; salir si no hay acuse | RF-5 |
| `ServicioGeneracion` | Vista de generación (estados, «Generar», «datos anteriores») | RF-1, RF-3 |
| `GeneracionSolver::construirEntrada` / `aFormatoVisualizacion` (frontend) | Componer la entrada desde la config y traducir la salida al formato de visualización | RF-1 |
| `Composicion::huella` / `GeneracionSolver::huellaActual(nucleo, opciones)` | Enviar a `generar` la huella con las preferencias intervinientes | RF-6 (013) |
| `PuertoSolverIpc` (frontend) | Enviar `solver_resolve` por IPC y mapear la respuesta | RF-1 |
| `AnalisisSalida` | Rejilla + panel de avisos | RF-3 |
| `HorarioSalida` + `ServicioHorarioSalida` | Visualización y guardado como archivo JSON | RF-1, RF-4 |
| `CargadorConfiguracionSolver` | Cargar/guardar config y presets JSON | RF-1, RF-4 |
| Salud del proceso de cálculo | Aviso no bloqueante al arrancar; cierre continúa | RF-2 |

> Todas las filas están implementadas. La sección 2 corresponde a la spec 001; las filas de
> `ServicioGeneracion`, `AnalisisSalida`, `CargadorConfiguracionSolver` y el guardado de horarios
> corresponden a la spec 002.

---

## 5. Deuda del frontend y alcance (spec 009)

La spec 009 cambia **solo** `src/backend` (persistencia y servicios); `src/frontend` **no se toca** y
**no compila** contra el contrato nuevo. El equipo de interfaz debe migrar estas piezas antes de
volver a compilar con `BUILD_FRONTEND=ON`:

1. **Formularios de aula y materia.** `crearAula`/`actualizarAula` y `crearMateria`/
   `actualizarMateria` exigen ya `idTipoAula` (sin valor por defecto). Hay que añadir a los
   formularios un desplegable con `listarTiposAula()` y pasarlo en el último argumento, y ofrecer la
   gestión del catálogo (`crearTipoAula`/`renombrarTipoAula`/`eliminarTipoAula`).
2. **`classroom_list_widget.cpp`: tipo por nombre (heurística).** El resumen de aulas clasifica
   «laboratorio/cancha/deporte/teórica» **por el texto del nombre** del aula. Debe usar
   `AulaDTO.tipoAula`/`idTipoAula`, que vienen del catálogo; la heurística por nombre ya no es la
   fuente de verdad (un aula puede llamarse «A-101» y ser un laboratorio).
3. **`subject_list_widget.cpp`: compatibilidad por texto libre (heurística).** La estimación de
   ocupación empareja el texto libre de `MateriaDTO.requisitos` con el nombre del aula. Debe usar
   `MateriaDTO.idTipoAula`/`tipoAula` frente al `tipoAula` de las aulas, y dejar `requisitos` como
   texto informativo.
4. **`schedule_generation_widget.cpp`: aritmética en minutos reales y DTO/método antiguos.** La vista
   calculaba la duración del bloque en minutos reales (`inicio.secsTo(fin)/60`) restando la suma de
   los descansos y recorría los campos del DTO retirado; debe incluir
   `backend/services/ServicioTurnosEventos.hpp`, recorrer `turno.eventos` y usar la suma de `duracion`.
   La 011 retiró el reparto de la ventana: la D efectiva sale de `NucleoDatos::configuracionHoraria()`
   y el encaje se comprueba con `errorDeEncaje(turno, D)`; el frontend debe consumir esas funciones,
   no replicar la fórmula.
5. **Tests de frontend.** `test_frontend_conversion` y `test_generacion_seed` no se configuran con
   `BUILD_FRONTEND=OFF`; siguen siendo deuda del equipo de frontend y el segundo fija además el
   valor 40 de la spec 008, que la 011 revisará.

**Fuera de alcance de la 009:** la interfaz no se adapta, no se añaden pantallas de eventos ni de
tipos, y la verificación de la entrega es la suite del preset `backend`
(`ctest --preset backend`), sin los tests de frontend. El contrato 1.0 del solver sigue derivando su
sección de descansos de los eventos; la spec 010 es la dueña de sustituirlo por el contrato 2.0.

> **Actualización 2026-11-01.** La interfaz se migró en `feat/frontend-generacion` al integrar
> `feat/Base-datos` (merge): los formularios de aula y materia piden el tipo del catálogo
> (`listarTiposAula()`), el resumen de aulas clasifica por `AulaDTO.tipoAula`, la ocupación de
> asignaturas cruza `MateriaDTO.tipoAula` con el de las aulas y la vista de generación recorre
> `turno.eventos`.
>
> **Actualización 2026-11-05 (016).** La **ayuda del tipo general** (`TiposAulaUI::idTipoGeneral`)
> queda **atendida**: es estricta, devuelve el general o un fallo y nunca sustituye el tipo; las
> altas que la usan no abren el formulario ni escriben si falta. La **pantalla** de gestión del
> catálogo (`crearTipoAula`/`renombrarTipoAula`/`eliminarTipoAula`) sigue siendo **deuda viva** del
> equipo de frontend.

---

## 6. Deuda del frontend y alcance (spec 011)

> **Guía de migración paso a paso:** `docs/guias/migracion-frontend.md` (qué rompe la compilación,
> qué cambió en las specs 009–013, 015 y 016 y cómo verificarlo).

La 011 cambia `src/backend` y `src/seed`; `src/frontend` **no se toca** y **no compila** contra las
firmas nuevas. El equipo de interfaz debe migrar:

1. **La construcción exige D.** El camino vigente es
   `construirEntradaCompleta(datos, preset, duracionMinutos, ahora)` (013), que devuelve
   `EntradaConstruida{json, intervinientes, noAplicadas}`; `construirJsonEntrada(...)` se mantiene como
   **envoltorio** con la firma de la 011 (solo el JSON) y la fachada
   `ConstructorEntradaSolver::construir` no cambia. El builder debe leer la D efectiva con
   `NucleoDatos::configuracionHoraria()` y pasarla; el campo `duracion_minutos` del preset es derivado
   y la construcción lo sobrescribe siempre.
2. **La huella incluye D y las preferencias intervinientes (013).** La firma es
   `huellaDatos(datos, duracionMinutos, intervinientes)`; el frontend no la llama a mano: compone la
   entrada y usa `GeneracionSolver::huellaActual(nucleo, opciones)` (`Composicion::huella`, el mismo
   camino que la generación). El callback `huellaActual` de `ServicioGeneracion` ya está cableado en la
   vista, así que «datos anteriores» se marca si cambia la instantánea, la D o una preferencia
   interviniente.
3. **Selector de duración.** El selector de duración del editor de presets y de la vista de generación
   **deja de ofrecerse**: la duración es global y se edita con `guardarConfiguracionHoraria()`.
4. **Aritmética en minutos.** `schedule_generation_widget.cpp` sigue derivando la duración del turno y
   `horasAula` sigue en minutos: deuda del equipo de frontend; la conversión de presentación es
   `duracion_horaria::minutosDeHorasAcademicas`.
5. **Tests de frontend.** `test/frontend/test_generacion_seed.cpp` fija `duracion_minutos == 40` y
   queda fuera del preset `backend`; se reescribe al migrar el builder con D.

> **Actualización 2026-11-01.** Resuelto en `feat/frontend-generacion`: el builder pasa la D global,
> `huellaActual` construye la huella con la D efectiva, el selector de duración ya no se ofrece y
> `test_generacion_seed` se reescribió a 35 min con la rejilla y los eventos del seed. Verificado con
> `cmake --preset full && cmake --build build` y `ctest` (**53/53** en el host).
>
> **Actualización 2026-11-06 (spec 013).** El builder usa `construirEntradaCompleta` y compone
> `Composicion{entrada, contexto, avisos, huella, preferenciasNoAplicadas}`; la huella es
> `huellaDatos(datos, D, intervinientes)` y se obtiene con `huellaActual(nucleo, opciones)`;
> `ServicioGeneracion::generar` recibe el diagnóstico de no aplicadas y `ResultadoGeneracion` expone
> `cumplimiento` (aditivo; el contrato de salida del solver no cambia). Verificado con
> `ctest --preset backend` (**58/58**) y `ctest --preset full-release` (**63/63**).
>
> **Actualización 2026-11-08 (spec 017).** El conjunto de datos de prueba es ahora el **centro real
> del Liceo Nacional Robert Serra (2026/2027)**: 2 planes (**31059** Media General y **31060**
> Ciencias y Tecnología), **85 materias** (79 con carga + 6 sin carga; 16 compartidas), **95 cargas**,
> **24 cursos con turno** (12 Mañana / 12 Tarde), **13 aulas** (12 generales de 44 compartidas entre
> turnos + 1 cancha de 88), **32 docentes** (30 posiciones + 2 añadidos; **272** asignaciones y
> **160** disponibilidades, tope 36), **40 min** de hora académica y **3 eventos**; matrícula **889**
> (579 + 310). **No hay profesorados alternativos** ni conjuntos inválidos de ejemplo. Los cursos
> **ya traen su turno** en la base y la configuración no lo sobrescribe (D-8; sobrescribirlo es
> capacidad futura de la spec 006). En la 017 el conjunto quedó **sin declarar generable** por el
> aforo de la cancha (**48 > 40** por turno); `test_generacion_seed` se retiró y su sustituto
> `test_entrada_seed` compone y valida **sin resolver** (ver §3.6). Verificado con
> `ctest --preset backend` (**61/61**) y `ctest --preset full-release` (**66/66**).
>
> **Actualización 2026-11-10 (spec 018).** El **aforo de la cancha** es una **regla del motor** (cancha
> → **2**, resto → **1**) y **no viaja en el contrato ni en el DTO**: no hay campo nuevo en `AulaDTO`,
> en el preset ni en la huella. La **causa (2)** de un `NoFactible` cita demanda, aulas, franjas y
> aforo (ver §3.1). La plantilla pasa a **33 docentes** (P033 de apoyo en Idiomas; P028 con Inglés) y
> **165 disponibilidades**, con **16 mañana / 17 tarde**. El conjunto **se declara generable**
> (48 ≤ 80 y 96 ≤ 160 por turno; 24 ≤ 40 pares) y `test_solver_centro_real` (lento) resuelve **876
> clases** con la EF en la cancha, 0 pares cruzados, desvío 0 y huecos al final en **232 s**. La
> **vista de horario y su estimación de ocupación quedan fuera del alcance de la 018** (no son deuda).
> Verificado con `ctest --preset backend` (**62/62**), `ctest --preset full-release` (**67/67**) y
> `ctest --preset slow-release` (**2/2**).
