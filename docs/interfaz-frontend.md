# Interfaz expuesta al equipo de frontend

> **Documentación viva.** Describe los contratos (firmas, estados y errores) que nuestro lado
> (`src/backend`, `src/app` y `src/middleware`) expone para que la interfaz Qt de `src/frontend`
> —implementada por **otro equipo**— consuma la base de datos y la lógica de negocio.
>
> No duplica los requisitos: para el QUÉ y el POR QUÉ, ver `specs/001-base-datos-local/spec.md` y
> `specs/002-generacion-horarios-ipc/spec.md`. Aquí solo se documenta el **contrato de consumo**
> (qué exponemos, con qué firma, qué estados/errores y cómo debe tratarlos el frontend).
>
> Se mantiene con las tareas «Documentar la interfaz expuesta al frontend» de cada spec.

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
nombra las tablas/columnas que faltan y las tablas retiradas que sobran (p. ej. «faltan las columnas
«anio», «seccion», «aula» de la tabla «Cursos»; existe la tabla retirada «Curso_Materia»»).

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
constexpr int VERSION_ESQUEMA_ACTUAL = 2;   // esquema único (sin migración por versión)
enum class EstadoApertura { Crear, Abrir, FalloEsquema };
EstadoApertura decidirApertura(bool existe, int versionArchivo, int versionEsperada); // pura
int  leerVersionEsquema(QSqlDatabase& db);          // -1 si no es legible
bool fijarVersionEsquema(QSqlDatabase& db, int version);
bool crearEsquema(QSqlDatabase& db);                // catálogo entero en una transacción
bool esquemaEsperado(QSqlDatabase& db, QString* motivo = nullptr); // catálogo y tablas retiradas
}
```

`decidirApertura` es pura (sin E/S): `versionArchivo <= 0` representa versión ausente. `crearEsquema`
aplica el catálogo completo (`Migracion::aplicarPaso`) y fija `user_version` en una sola transacción;
si algo falla, revierte.

> **Guardia anti-regresión.** `test/backend/test_version_esquema.cpp` congela la **huella SHA-256**
> del catálogo real (`HUELLAS_CONOCIDAS`, por versión). Si se cambia el DDL hay que **subir
> `VERSION_ESQUEMA_ACTUAL`** y registrar la huella nueva en el mismo cambio; de lo contrario el test
> falla. Así una base creada por una compilación intermedia no queda con la marca actual y un catálogo
> viejo (incidente del 2026-10-17: `Cursos` pasó de `nombre`/`aula_fija` a `anio`/`seccion`/`turno`/
> `aula` sin subir la versión, y las bases existentes dejaron de abrir).

**Cómo lo trata el frontend:** normalmente no lo usa directamente; `AperturaBaseDatos` ya envuelve
la decisión y la verificación del catálogo.

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
servicios de dominio; el frontend la consume **en proceso** (ya no por IPC) para docentes, aulas,
materias, cursos, planes de estudio, turnos y recesos. El `NucleoDatos` **no es dueño** de la
conexión ni se instancia en la interfaz: lo construye el `ContextoBaseDatos` del módulo `datos`
(ver 2.7), que también se lo inyecta a la ventana principal. El frontend recibe un `NucleoDatos*`
ya construido y operativo.

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

    // Aulas
    Resultado<QVector<AulaDTO>> listarAulas() const;
    Resultado<AulaDTO>  crearAula(const QString& nombre, int capacidad,
                                  const QString& edificio = QString(), const QString& piso = QString());
    Resultado<AulaDTO>  actualizarAula(int id, const QString& nombre, int capacidad,
                                       const QString& edificio = QString(), const QString& piso = QString());
    bool                eliminarAula(int id);

    // Materias
    Resultado<QVector<MateriaDTO>> listarMaterias() const;
    Resultado<MateriaDTO>  crearMateria(const QString& nombre, const QString& requisitos = QString());
    Resultado<MateriaDTO>  actualizarMateria(int id, const QString& nombre, const QString& requisitos = QString());
    bool                   eliminarMateria(int id);

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

    // Cursos (identidad ano+seccion; turno y aula opcionales)
    Resultado<QVector<CursoDTO>> listarCursos() const;
    Resultado<CursoDTO>      crearCurso(int anio, const QString& seccion,
                                        const QString& turno = QString(), int aula = -1);
    Resultado<CursoDTO>      actualizarCurso(int id, int anio, const QString& seccion,
                                             const QString& turno = QString(), int aula = -1);
    bool                     eliminarCurso(int id);

    // Turnos y recesos
    Resultado<QVector<TurnoDTO>> listarTurnos() const;
    Resultado<TurnoDTO> crearTurno(const QString& nombre, const QTime& inicio, const QTime& fin, int numSlots);
    Resultado<TurnoDTO> actualizarTurno(const QString& nombre, const QTime& inicio, const QTime& fin, int numSlots);
    bool                eliminarTurno(const QString& nombre);
    Resultado<RecesoDTO> agregarReceso(const QString& turno, int despuesDeSlot, int duracion,
                                       const QTime& inicio = QTime(), const QTime& fin = QTime());
    bool                eliminarReceso(const QString& turno, int despuesDeSlot);

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
| `ProfesorDTO` | `id`, `nombre`, `email`, `telefono`, `QVector<DisponibilidadDTO> disponibilidad`, `QVector<MateriaAsignadaDTO> materias` |
| `AulaDTO` | `id`, `nombre`, `capacidad`, `edificio`, `piso` |
| `MateriaDTO` | `id`, `nombre`, `requisitos` |
| `PlanDTO` | `codigo`, `nombre`, `descripcion` |
| `CargaHorariaDTO` | `id`, `codigoPlanEstudio`, `idMateria`, `curso` (año), `horas`, `nombreMateria` |
| `CursoDTO` | `id`, `anio`, `seccion`, `turno`, `aula` (`-1` si no tiene) |
| `TurnoDTO` | `nombre`, `inicio`, `fin`, `numSlots`, `QVector<RecesoDTO> recesos` |
| `RecesoDTO` | `despuesDeSlot`, `duracion` (minutos), `inicio`, `fin` (pueden ser inválidos) |

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
| `profesor` | `Profesores` (`id`) | profesor-materia y disponibilidad |
| `aula` | `Aulas` (`id`) | **anula** la referencia de los cursos con esa aula (no los borra) |
| `curso` | `Cursos` (`id`) | (ninguno) |
| `turno` | `Turnos` (`nombre`) | recesos y **anula** la referencia de los cursos del turno |

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
| `receso` | agregar / eliminar | `"<turno>-<despuesDeSlot>"` |

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
doble) y un plazo (60 s por defecto).

```cpp
struct ResultadoGeneracion {
    enum class Estado { Inactivo, Calculando, Listo, DatosAnteriores, NoFactible,
                        ContratoInvalido, CalculoFallido, TiempoAgotado, EntradaInvalida };
    Estado        estado = Estado::Inactivo;
    QString       mensaje;
    bool          datosAnteriores = false;
    HorarioSalida horario;      // válido si presentable
    AnalisisSalida analisis;    // avisos P1–P3 y conflictos P4
    bool          presentable = false;
};

class ServicioGeneracion : public QObject {
    Q_OBJECT
public:
    ServicioGeneracion(PuertoSolver& puerto, int plazoMs = 60000,
                       std::function<QString()> huellaActual = {},
                       std::function<QDateTime()> reloj = {}, QObject* padre = nullptr);
    void generar(const QJsonObject& entrada, const QString& huella);
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

La entrada la construye `ConstructorEntradaSolver` (`construirJsonEntrada`, `src/backend`) a partir
de `DatosDominio` (dominios) + preset; se valida con `validarEntradaSolver` (V1–V13). La huella
(`huellaDatos`) detecta «datos anteriores».

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

Un «preset» es un archivo JSON con el mismo formato (las 13 secciones) que la configuración.
**Frontend:** cargar/guardar presets como archivos; ante fallo (`!ok`), mostrar `mensajeError`
(causa + acciones) y permitir reintentar.

### 3.3 `HorarioSalida` y guardado como archivo (implementado/en consolidación)

- `HorarioSalida`: `metadata` + `horarios` por curso (`DiaOutput` → `AsignacionOutput`:
  `slot`, `materia`, `profesor`, `aula`); `toJson()`/`fromJson()`.
- `ServicioHorarioSalida`: `guardarHorario(...)`, `cargarHorario(...)`, `listarArchivos()`.

**Frontend:** pintar el horario (curso × día/slot) + avisos; guardar solo si el usuario lo pide
(diálogo de archivo del frontend); ante fallo de escritura, mantener el contenido y reintentar.

### 3.4 `AnalisisSalida` — validaciones posteriores (implementado)

`ValidadorSalidaSolver` / `analizarSalidaSolver` (pura): P1 horas no cubiertas, P2 exceso de horas
de docente y P3 exceso de capacidad de aula → `avisos`; P4 solapamiento (docente/aula/curso) →
`conflictos` y `presentable() == false`. **Frontend:** panel de avisos junto al horario; nunca
mostrar como válido un resultado con `hayConflictoSolapamiento()`.

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
  - `construirEntrada(nucleo, opciones)` → `Composicion{entrada, contexto, avisos}`. Reutiliza
    `construirJsonEntrada` (backend) y **después inyecta por curso** el `plan`, el `turno` y el
    `num_estudiantes` (las `materias` se derivan del plan embebido y el año del curso vía
    `NucleoDatos::listarCargaDePlan`), y aplica la opción global de aulas fijas. El `turno` del
    preset tiene prioridad sobre el de la base (que suele venir vacío en los datos sembrados); el
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
preset es la propia entrada de 13 secciones). Al pulsar «Generar horarios»: valida, compone la
entrada, la envía con `ServicioGeneracion` y, si `presentable`, guarda el JSON en la carpeta de
horarios y emite `horarioGenerado(datosVisualizacion, rutaArchivo)`. Sin backend muestra un aviso
informativo.

Al cargar la base por primera vez, la vista **deriva de los turnos** el nº de slots por día (suma de
los slots de todos los turnos) y la **duración del bloque** (franja del turno menos recesos, sobre
sus slots; 40 min en los datos de prueba del centro); ambos quedan editables. La columna **Turno** de
cada curso es un desplegable con los turnos de la base y una opción «Sin turno» para los cursos aún
sin asignar (los del centro se siembran sin turno; el solver exige uno, V7).

**Cableado (`MainWindow`):** el `InternalClient` se guarda como miembro y se inyecta en las vistas;
en `setNucleoDatos` se lo reenvía a la vista de generación con el `NucleoDatos*`. Al recibir
`horarioGenerado`, la ventana llama a `ScheduleVisualizationWidget::loadSchedules(...)` y salta a la
sección de visualización para que el horario quede cargado y editable. La carpeta de destino se
obtiene de `ScheduleVisualizationWidget::schedulesDirectory()`.

**Visualización dirigida por datos (`dialogs/`):** el documento que consumen los diálogos de horarios
lleva, además de `teachers`/`years` y `_meta`, un bloque **`_cuadricula`** con la rejilla con la que se
generó: `duracion_minutos`, `num_dias`, `num_slots_dia`, `turnos` (nombre, franjas `slot` y hora de
`inicio`) y `recesos` (`turno`, `despues_de_slot`, `duracion`, `inicio`, `fin`). Cada sección declara
además su `turno`. `ScheduleModel::rowsFor` construye las filas de cada horario con esos datos: las
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

La integración completa (sembrar una base de prueba, componer la entrada desde el frontend con un
reparto plan/turno por curso, validarla y **resolverla** con el motor) está cubierta por
`test_generacion_seed` (`test/frontend/test_generacion_seed.cpp`), añadido cuando existen `frontend`,
`backend_solver` y `seed`. Requiere ejecutarse con la configuración que enlaza OR-Tools
(`Release`/Docker); en `Debug` sobre Windows el motor no arranca (CRT de depuración contra las DLL
Release de OR-Tools).


---

## 4. Resumen: elemento expuesto → acción del frontend

| Elemento expuesto | Acción del frontend | RF |
|---|---|---|
| `AperturaBaseDatos::Resultado` | Diálogo de fallo (Reintentar/Restaurar/Crear nueva/Salir) | RF-4, RF-6 |
| `Respaldo::restaurarRespaldo` | Restaurar respaldo elegido y reintentar apertura | RF-4 |
| `NucleoDatos` | Listas desde la base y CRUD en proceso | RF-2 |
| `ContextoBaseDatos` (`src/datos`) | El arranque lo crea y entrega `NucleoDatos*` a la ventana (`setNucleoDatos`) | RF-1, RF-2 |
| `NucleoDatos::dependientesDe` / `eliminarConCascada` | Confirmación de cascada | RF-2 |
| `estadoPendienteDe` / `GestorPendientes` | Marca por fila + «Reintentar»; aviso al cerrar | RF-3, RF-6, RNF-3/4 |
| `InstanciaUnica` | Enfocar la instancia existente; salir si no hay acuse | RF-5 |
| `ServicioGeneracion` | Vista de generación (estados, «Generar», «datos anteriores») | RF-1, RF-3 |
| `GeneracionSolver::construirEntrada` / `aFormatoVisualizacion` (frontend) | Componer la entrada desde la config y traducir la salida al formato de visualización | RF-1 |
| `PuertoSolverIpc` (frontend) | Enviar `solver_resolve` por IPC y mapear la respuesta | RF-1 |
| `AnalisisSalida` | Rejilla + panel de avisos | RF-3 |
| `HorarioSalida` + `ServicioHorarioSalida` | Visualización y guardado como archivo JSON | RF-1, RF-4 |
| `CargadorConfiguracionSolver` | Cargar/guardar config y presets JSON | RF-1, RF-4 |
| Salud del proceso de cálculo | Aviso no bloqueante al arrancar; cierre continúa | RF-2 |

> Todas las filas están implementadas. La sección 2 corresponde a la spec 001; las filas de
> `ServicioGeneracion`, `AnalisisSalida`, `CargadorConfiguracionSolver` y el guardado de horarios
> corresponden a la spec 002.
