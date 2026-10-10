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
  `-6` versión incompatible.

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
constexpr int VERSION_ESQUEMA_ACTUAL = 1;
enum class EstadoApertura { Crear, Abrir, Migrar, FalloAusente, FalloPosterior };
EstadoApertura decidirApertura(bool existe, int versionArchivo, int versionEsperada); // pura
int  leerVersionEsquema(QSqlDatabase& db);          // -1 si no es legible
bool fijarVersionEsquema(QSqlDatabase& db, int version);
using PasoMigracion = std::function<bool(QSqlDatabase&, int versionDestino)>;
bool aplicarHasta(QSqlDatabase& db, int versionDestino, const PasoMigracion& paso);
bool aplicarHasta(QSqlDatabase& db, int versionDestino);
}
```

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

### 2.4 `NucleoDatos` — fachada de dominios (planificado)

Fachada única sobre la conexión y los servicios de dominio; el frontend la consume **en proceso**
(ya no por IPC) para docentes, aulas, materias, cursos, planes de estudio, turnos y recesos.

```cpp
class NucleoDatos {
public:
    explicit NucleoDatos(QSqlDatabase& db);
    Resultado<QVector<Profesor>> listarProfesores() const;   // y un listar* por dominio
    Resultado<Aula>              listarAulas() const;
    Resultado<Profesor> crearProfesor(const ProfesorDTO& dto);
    Resultado<Profesor> actualizarProfesor(qint64 id, const ProfesorDTO& dto);
    Resultado<bool>     eliminarProfesor(qint64 id);         // requiere confirmación de cascada
    Resultado<QVector<Dependencia>> dependientesDe(const QString& dominio, qint64 id) const;
    // …
};
```

**Estados/errores:** todo devuelve `Resultado<T>`; la persistencia es atómica (si falla la
operación o la cascada, no queda nada aplicado). Un fallo de E/S se registra como cambio pendiente.

**Cómo lo trata el frontend:** poblar listas con `listar*`; antes de una baja, `dependientesDe` →
confirmación explícita; tras éxito, refrescar; ante error, mostrar mensaje y mantener el dato.

### 2.5 Estado por registro y cambios pendientes (planificado)

```cpp
enum class EstadoPendiente { Guardado, PendienteGuardar, PendienteEliminar };
EstadoPendiente estadoPendienteDe(const OperacionPendiente&); // pura

class GestorPendientes {
public:
    bool hayPendientes() const;                    // guardia de cierre (RF-6)
    QVector<OperacionPendiente> pendientes() const;
    Resultado<bool> reintentar(qint64 id);
};
```

**Cómo lo trata el frontend:** `Guardado` → fila normal; `PendienteGuardar` → resaltada + acción
«Reintentar»; `PendienteEliminar` → atenuada. No retirar de pantalla un dato pendiente (RNF-3). Al
cerrar con `hayPendientes()`, aviso cancelable (RF-6).

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
| `dependientesDe` | Confirmación de cascada | RF-2 |
| `estadoPendienteDe` / `GestorPendientes` | Marca por fila + «Reintentar»; aviso al cerrar | RF-3, RF-6, RNF-3/4 |
| `ServicioGeneracion` | Vista de generación (estados, «Generar», «datos anteriores») | RF-1, RF-3 |
| `AnalisisSalida` | Rejilla + panel de avisos | RF-3 |
| `HorarioSalida` + `ServicioHorarioSalida` | Visualización y guardado como archivo JSON | RF-1, RF-4 |
| `CargadorConfiguracionSolver` | Cargar/guardar config y presets JSON | RF-1, RF-4 |
| Salud del proceso de cálculo | Aviso no bloqueante al arrancar; cierre continúa | RF-2 |
