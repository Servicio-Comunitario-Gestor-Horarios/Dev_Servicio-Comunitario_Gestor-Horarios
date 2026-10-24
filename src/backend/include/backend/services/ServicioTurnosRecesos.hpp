#pragma once

/**
 * @file ServicioTurnosRecesos.hpp
 * @brief Servicio para gestión de turnos y de sus eventos (tablas Turnos y Eventos).
 *
 * Nombre temporal: la spec 009 introduce los **eventos** de turno (con identidad
 * y orden propios) y la tarea T9 renombra este servicio a `ServicioTurnosEventos`.
 * Mientras dure la transición se conservan `RecesoDTO`, `TurnoDTO::recesos` y
 * `agregarReceso`/`eliminarReceso` como **capa derivada de los eventos**, para que
 * el constructor de entrada, la huella, el seed, la consola y los tests sigan
 * compilando sin un big-bang.
 */

#include <QSqlDatabase>
#include <QSqlRecord>
#include <QString>
#include <QTime>
#include <QVector>

#include "backend/resultado.hpp"

/// Evento del turno: sucede después de una franja y dura un tiempo (RF-1).
///
/// `id` es la identidad estable (asignada por la base, `-1` al crear);
/// `despuesDeSlot` está en `1..numSlots` —el `0` del DTO es un valor de
/// construcción, **no** «sin asignar»— y `orden` ordena los eventos de la misma
/// posición (`>= 1`; `0` al crear pide el automático `máximo + 1`).
struct EventoDTO {
    int id = -1;
    QString nombre;            ///< No vacío
    int despuesDeSlot = 0;     ///< Posición dentro del turno (1..numSlots)
    int orden = 0;             ///< Orden dentro de la posición (>= 1)
    int duracion = 0;          ///< Duración en minutos (> 0)
    QTime inicio;              ///< Hora de inicio (inválida = ausente)
    QTime fin;                 ///< Hora de fin (inválida = ausente)
};

/// Receso dentro de un turno: ocurre después de un slot y dura un tiempo.
///
/// **Compatibilidad transitoria (se retira en T9):** se deriva de los eventos
/// del turno; no existe una tabla `Recesos`.
struct RecesoDTO {
    int despuesDeSlot = 0;
    int duracion = 0;       ///< Duración en minutos
    QTime inicio;           ///< Hora de inicio (puede ser inválida)
    QTime fin;              ///< Hora de fin (puede ser inválida)
};

/// Representación persistente de un turno, sus eventos y los recesos derivados.
struct TurnoDTO {
    QString nombre;
    QTime inicio;
    QTime fin;
    int numSlots = 0;

    /// Eventos del turno, ordenados por (posición, orden, id).
    QVector<EventoDTO> eventos;

    /// Vista derivada de `eventos`, una entrada por evento (compatibilidad 1.0).
    QVector<RecesoDTO> recesos;
};

/// Servicio CRUD sobre las tablas Turnos y Eventos (RF-1, RF-2).
class ServicioTurnosRecesos {
public:
    explicit ServicioTurnosRecesos(QSqlDatabase& db);

    // ─── Turnos ───────────────────────────────────────────────────────────

    Resultado<TurnoDTO> crearTurno(const QString& nombre, const QTime& inicio,
                                   const QTime& fin, int numSlots);

    Resultado<TurnoDTO> obtenerTurno(const QString& nombre) const;

    Resultado<QVector<TurnoDTO>> listarTurnos() const;

    /// Actualiza el turno. Si se reducen las franjas y hay eventos que quedarían
    /// fuera de rango, sin `eliminarEventosFueraDeRango` se devuelve el error con
    /// la lista de afectados; con `true` se borran en la misma transacción (RF-4).
    Resultado<TurnoDTO> actualizarTurno(const QString& nombre, const QTime& inicio,
                                        const QTime& fin, int numSlots,
                                        bool eliminarEventosFueraDeRango = false);

    bool eliminarTurno(const QString& nombre);

    // ─── Eventos ──────────────────────────────────────────────────────────

    /// Alta de un evento. `orden == 0` pide el automático (máximo + 1 de la
    /// posición). Las horas son un par opcional: ambas o ninguna.
    Resultado<EventoDTO> agregarEvento(const QString& turno, const QString& nombre,
                                       int despuesDeSlot, int duracion,
                                       const QTime& inicio = QTime(),
                                       const QTime& fin = QTime(), int orden = 0);

    /// Edición por identidad; el turno del evento es inmutable y `orden` es
    /// obligatorio (`>= 1`).
    Resultado<EventoDTO> actualizarEvento(int id, const QString& nombre, int despuesDeSlot,
                                          int duracion, const QTime& inicio, const QTime& fin,
                                          int orden);

    /// Borrado por identidad; no renumera los eventos restantes.
    bool eliminarEvento(int id);

    /// Eventos que quedarían después de la franja `numSlots` (RF-4), para que la
    /// interfaz o la consola pidan confirmación antes de reducir las franjas.
    Resultado<QVector<EventoDTO>> eventosFueraDeRango(const QString& turno,
                                                      int numSlots) const;

    // ─── Recesos (compatibilidad transitoria; se retiran en T9) ────────────

    /// Alta de un evento llamado «Receso»; devuelve su vista como receso.
    Resultado<RecesoDTO> agregarReceso(const QString& turno, int despuesDeSlot,
                                       int duracion, const QTime& inicio = QTime(),
                                       const QTime& fin = QTime());

    /// Borra los eventos de la posición indicada (antiguo receso único).
    bool eliminarReceso(const QString& turno, int despuesDeSlot);

private:
    QSqlDatabase& m_db;

    bool validarTurno(const QString& nombre, const QTime& inicio, const QTime& fin,
                      int numSlots, QString& error) const;

    /// Valida nombre, posición, duración y orden de un evento del turno `dto`.
    bool validarEvento(const TurnoDTO& dto, const QString& nombre, int despuesDeSlot,
                       int duracion, int orden, bool ordenObligatorio, QString& error) const;

    /// Valida el par de horas, la diferencia con la duración, el encaje en la
    /// rejilla, la ventana de su posición y los solapes. `turno` debe incluir el
    /// candidato en `turno.eventos`.
    bool validarCoherenciaTemporal(const TurnoDTO& turno, const EventoDTO& candidato,
                                   QString& error) const;

    /// Carga `dto.eventos` (ordenados por posición, orden e id) y deriva `recesos`.
    bool cargarEventos(TurnoDTO& dto) const;

    TurnoDTO mapearARecord(const QSqlRecord& record) const;
};
