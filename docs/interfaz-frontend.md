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

### 2.1 `AperturaBaseDatos` — abrir, migrar y respaldar (implementado)

`src/backend/include/backend/database/apertura_base_datos.hpp`:

```cpp
namespace AperturaBaseDatos {

enum class Estado {
    OkCreada,        // No existía: se creó con el esquema inicial.
    OkAbierta,       // Existía con la versión esperada: se abrió sin tocar.
    OkMigrada,       // Existía con versión anterior: se respaldó y migró.
    FalloAusente,    // Versión de esquema ausente o no interpretable.
    FalloPosterior,  // Versión de esquema posterior a la esperada.
    FalloApertura,   // No se pudo abrir la base o crear el esquema inicial.
    FalloRespaldo,   // No se pudo respaldar: no se migró.
    FalloMigracion   // La migración falló; la versión previa queda intacta.
};

struct Resultado {
    Estado  estado = Estado::FalloApertura;
    QString detalle;        // texto legible para el usuario (en español)
    QString rutaRespaldo;   // ruta del respaldo creado, si lo hubo
    bool    ok() const;     // true solo para OkCreada | OkAbierta | OkMigrada
};

struct Opciones { /* versionEsperada, paso, respaldo, ahora — inyectables en tests */ };

Resultado abrir(const QString& ruta);                 // valores de producción
Resultado abrir(const QString& ruta, const Opciones& opciones);
}
```

**Cómo lo trata el frontend:**
- `ok() == true` → continuar el arranque; si `estado == OkMigrada`, puede informar del respaldo.
- `ok() == false` → diálogo de fallo RF-4 (Reintentar / Restaurar respaldo / Crear base nueva /
  Salir). Deshabilitar «Crear base nueva» si el motivo es `FalloRespaldo`. `rutaRespaldo` no vacío
  permite ofrecer restaurarlo.

### 2.2 `VersionEsquema` — versión de esquema (implementado)

`src/backend/include/backend/database/version_esquema.hpp`:

```cpp
namespace VersionEsquema {
constexpr int VERSION_ESQUEMA_ACTUAL = 2;   // v1 = esquema base; v2 = cursos, turnos y recesos
enum class EstadoApertura { Crear, Abrir, Migrar, FalloAusente, FalloPosterior };
EstadoApertura decidirApertura(bool existe, int versionArchivo, int versionEsperada); // pura
int  leerVersionEsquema(QSqlDatabase& db);          // -1 si no es legible
bool fijarVersionEsquema(QSqlDatabase& db, int version);
using PasoMigracion = std::function<bool(QSqlDatabase&, int versionDestino)>;
bool aplicarHasta(QSqlDatabase& db, int versionDestino, const PasoMigracion& paso);
bool aplicarHasta(QSqlDatabase& db, int versionDestino);   // migraciones reales
}
```

`decidirApertura` es pura (sin E/S): `versionArchivo <= 0` representa versión ausente. `aplicarHasta`
ejecuta cada paso en su propia transacción junto con `user_version`; si un paso falla, revierte y la
versión previa queda intacta (estado conocido, sin cambios parciales).

**Cómo lo trata el frontend:** normalmente no lo usa directamente; `AperturaBaseDatos` ya envuelve
la decisión.

### 2.3 `Respaldo` — crear, validar y restaurar (implementado)

```cpp
namespace Respaldo {
QString construirNombreRespaldo(const QString& rutaDb, const QDateTime& ahora); // determinista
bool crearRespaldo(QSqlDatabase& db, const QString& rutaRespaldo, QString* error = nullptr);
bool validarRespaldo(const QString& rutaRespaldo, QString* error = nullptr);   // integridad + versión
bool restaurarRespaldo(const QString& rutaOrigen, const QString& rutaDestino, QString* error = nullptr);
}
```

**Cómo lo trata el frontend:** para «Restaurar respaldo» (RF-4), llamar a
`restaurarRespaldo(origen, rutaBaseActual, &error)`; si falla, mostrar `error` y reabrir el diálogo;
si tiene éxito, reintentar la apertura.

### 2.4 `NucleoDatos` — fachada de dominios (implementado)

`src/backend/include/backend/services/NucleoDatos.hpp`. Fachada única sobre la conexión y los
servicios de dominio; el frontend la consume **en proceso** (ya no por IPC) para docentes, aulas,
materias, cursos, planes de estudio, turnos y recesos. Se construye con `NucleoDatos(QSqlDatabase&)`;
el `NucleoDatos` **no es dueño** de la conexión (la abre/cierra el arranque con `AperturaBaseDatos`).

```cpp
class NucleoDatos {
public:
    explicit NucleoDatos(QSqlDatabase& db);

    // Docentes (la tabla/servicio se llama "Profesor"; la fachada usa "Docente")
    QVector<ProfesorDTO>   listarDocentes() const;
    Resultado<ProfesorDTO> crearDocente(const QString& id, const QString& nombre,
                                        const QString& email, const QString& telefono = QString());
    Resultado<ProfesorDTO> actualizarDocente(const QString& id, const QString& nombre,
                                             const QString& email, const QString& telefono = QString());
    bool                   eliminarDocente(const QString& id);

    // Aulas
    QVector<AulaDTO>    listarAulas() const;
    Resultado<AulaDTO>  crearAula(const QString& nombre, int capacidad,
                                  const QString& edificio = QString(), const QString& piso = QString());
    Resultado<AulaDTO>  actualizarAula(int id, const QString& nombre, int capacidad,
                                       const QString& edificio = QString(), const QString& piso = QString());
    bool                eliminarAula(int id);

    // Materias
    QVector<MateriaDTO>    listarMaterias() const;
    Resultado<MateriaDTO>  crearMateria(const QString& nombre, const QString& requisitos = QString());
    Resultado<MateriaDTO>  actualizarMateria(int id, const QString& nombre, const QString& requisitos = QString());
    bool                   eliminarMateria(int id);

    // Planes de estudio
    QVector<PlanDTO>   listarPlanes() const;
    Resultado<PlanDTO> crearPlan(const QString& codigo, const QString& nombre,
                                 const QString& descripcion = QString());
    Resultado<PlanDTO> actualizarPlan(const QString& codigo, const QString& nombre,
                                      const QString& descripcion = QString());
    bool               eliminarPlan(const QString& codigo);

    // Cursos y su relación con materias
    QVector<CursoDTO>        listarCursos() const;
    Resultado<CursoDTO>      crearCurso(const QString& nombre, const QString& turno = QString(),
                                        int aulaFija = -1, int numEstudiantes = 0,
                                        const QString& codigoPlan = QString());
    Resultado<CursoDTO>      actualizarCurso(int id, const QString& nombre, const QString& turno = QString(),
                                             int aulaFija = -1, int numEstudiantes = 0,
                                             const QString& codigoPlan = QString());
    bool                     eliminarCurso(int id);
    Resultado<CursoMateriaDTO> asignarMateriaACurso(int idCurso, int idMateria, int horasSemanales);
    bool                     quitarMateriaDeCurso(int idCurso, int idMateria);

    // Turnos y recesos
    QVector<TurnoDTO>   listarTurnos() const;
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
| `CursoDTO` | `id`, `nombre`, `turno`, `aulaFija` (`-1` si no tiene), `numEstudiantes`, `codigoPlan`, `QVector<CursoMateriaDTO> materias` |
| `CursoMateriaDTO` | `idMateria`, `nombreMateria`, `horasSemanales` |
| `TurnoDTO` | `nombre`, `inicio`, `fin`, `numSlots`, `QVector<RecesoDTO> recesos` |
| `RecesoDTO` | `despuesDeSlot`, `duracion` (minutos), `inicio`, `fin` (pueden ser inválidos) |

**Contrato de las operaciones:**
- Los `listar*` devuelven el `QVector` directamente (vacío = sin datos o error de lectura); los
  `crear*`/`actualizar*`/`asignar*`/`agregar*` devuelven `Resultado<T>`; los `eliminar*`, `quitar*`
  y `eliminarConCascada` devuelven `bool`.
- Cada escritura es atómica en su servicio. Si falla, **no queda aplicada** y se registra como
  **cambio pendiente** en el `GestorPendientes` del núcleo, con su `dominio`/`registroId` (ver 2.5).
- `eliminarConCascada` ejecuta el borrado del registro y de todos sus dependientes en **una sola
  transacción**: si falla cualquier parte, revierte y devuelve `false` sin cambios parciales.

**Cómo lo trata el frontend:** poblar las listas con los `listar*` al abrir cada vista; en las
altas/modificaciones usar el `Resultado<T>` (`ok`/`mensajeError`); antes de una baja consultar
`dependientesDe` y pedir confirmación explícita con la lista de `Dependencia::descripcion`; tras el
éxito, refrescar la vista; ante error, mostrar `mensajeError` y **no** dar la operación por hecha.

#### 2.4.1 Eliminación en cascada (`cascada.hpp`)

`src/backend/include/backend/services/cascada.hpp` describe, de forma pura (sin tocar la base), qué
depende de qué. `NucleoDatos::dependientesDe`/`eliminarConCascada` usan **estas** claves de dominio
(no las del CRUD):

| `dominio` | Tabla raíz (clave) | Dependientes que arrastra |
|---|---|---|
| `plan` | `PlanEstudio` (`codigo`) | materias del plan y cursos del plan (con sus materias) |
| `materia` | `Materias` (`id`) | vínculos plan-materia, profesor-materia y curso-materia |
| `profesor` | `Profesores` (`id`) | profesor-materia y disponibilidad |
| `aula` | `Aulas` (`id`) | cursos con esa aula fija (con sus materias) |
| `curso` | `Cursos` (`id`) | curso-materia |
| `turno` | `Turnos` (`nombre`) | recesos y cursos del turno |

`Dependencia` es `{ QString dominio; qint64 id; QString descripcion; }` (`descripcion` ya viene en
español, lista para el diálogo). `dependientesDe` devuelve vacío para un dominio desconocido o `id`
vacío. La cascada es recursiva (un curso dependiente arrastra sus materias).

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
| `curso` | alta | nombre (aún no hay id) |
| `curso` | modificación / baja | id (como texto) |
| `curso_materia` | asignar / quitar | `"<idCurso>-<idMateria>"` |
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

**Cómo lo trata el frontend:** conectar `activarSolicitada()` a levantar/enfocar la ventana (o el
diálogo de fallo/migración activo); con `Secundaria` salir con éxito; con `SinAcuse` mostrar
`detalle()` y salir con error.

---

## 3. Generación de horarios (spec 002)

### 3.1 `ServicioGeneracion` — orquestar una generación (planificado)

Instantánea de dominios → construir/validar JSON de entrada → enviar por IPC → esperar 60 s →
validar salida → marcar «datos anteriores». Estados/desenlaces: **en curso**, **listo**
(+`AnalisisSalida`), **no factible**, **contrato inválido**, **tiempo agotado/desconexión** y
**datos anteriores**. **Cómo lo trata el frontend:** botón «Generar» deshabilitado si hay una en
curso; mostrar estado/avisos; confirmación antes de guardar si son «datos anteriores»; no ofrecer
guardar si no es factible o el contrato es inválido.

### 3.2 `CargadorConfiguracionSolver` — config y presets en JSON (planificado)

Carga/guarda configuración y presets como archivos JSON (no base de datos), con `Resultado<T>`.
**Frontend:** cargar/guardar presets como archivos; ante fallo, avisar y permitir reintentar.

### 3.3 `HorarioSalida` y guardado como archivo (implementado/en consolidación)

- `HorarioSalida`: `metadata` + `horarios` por curso (`DiaOutput` → `AsignacionOutput`:
  `slot`, `materia`, `profesor`, `aula`); `toJson()`/`fromJson()`.
- `ServicioHorarioSalida`: `guardarHorario(...)`, `cargarHorario(...)`, `listarArchivos()`.

**Frontend:** pintar el horario (curso × día/slot) + avisos; guardar solo si el usuario lo pide
(diálogo de archivo del frontend); ante fallo de escritura, mantener el contenido y reintentar.

### 3.4 `AnalisisSalida` — validaciones posteriores (planificado)

`ValidadorSalidaSolver` / `analizarSalidaSolver`: P1–P3 (avisos de horas y capacidad) y P4
(solapamiento → no presentable). **Frontend:** panel de avisos; nunca mostrar como válido un
resultado con conflicto de solapamiento.

### 3.5 Salud del proceso de cálculo (implementado/en consolidación)

Comprobación al arrancar con plazo de 5 s y apagado ordenado al cerrar (5 s). **Frontend:** aviso
no bloqueante al arrancar; el cierre continúa aunque no responda (RF-2).

---

## 4. Resumen: elemento expuesto → acción del frontend

| Elemento expuesto | Acción del frontend | RF |
|---|---|---|
| `AperturaBaseDatos::Resultado` | Diálogo de fallo (Reintentar/Restaurar/Crear nueva/Salir) | RF-4, RF-6 |
| `Respaldo::restaurarRespaldo` | Restaurar respaldo elegido y reintentar apertura | RF-4 |
| `NucleoDatos` | Listas desde la base y CRUD en proceso | RF-2 |
| `NucleoDatos::dependientesDe` / `eliminarConCascada` | Confirmación de cascada | RF-2 |
| `estadoPendienteDe` / `GestorPendientes` | Marca por fila + «Reintentar»; aviso al cerrar | RF-3, RF-6, RNF-3/4 |
| `InstanciaUnica` | Enfocar la instancia existente; salir si no hay acuse | RF-5 |
| `ServicioGeneracion` | Vista de generación (estados, «Generar», «datos anteriores») | RF-1, RF-3 |
| `AnalisisSalida` | Rejilla + panel de avisos | RF-3 |
| `HorarioSalida` + `ServicioHorarioSalida` | Visualización y guardado como archivo JSON | RF-1, RF-4 |
| `CargadorConfiguracionSolver` | Cargar/guardar config y presets JSON | RF-1, RF-4 |
| Salud del proceso de cálculo | Aviso no bloqueante al arrancar; cierre continúa | RF-2 |

> Las filas desde `ServicioGeneracion` son **planificadas** (spec 002, generación); las anteriores
> son de la **spec 001** y están implementadas.
