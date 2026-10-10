#pragma once

/**
 * @file ServicioTurnosRecesos.hpp
 * @brief Servicio para gestión de turnos y sus recesos (tablas Turnos y Recesos).
 */

#include <QSqlDatabase>
#include <QSqlRecord>
#include <QString>
#include <QTime>
#include <QVector>

#include "backend/resultado.hpp"

/// Receso dentro de un turno: ocurre después de un slot y dura un tiempo.
struct RecesoDTO {
    int despuesDeSlot = 0;
    int duracion = 0;       ///< Duración en minutos
    QTime inicio;           ///< Hora de inicio (puede ser inválida)
    QTime fin;              ///< Hora de fin (puede ser inválida)
};

/// Representación persistente de un turno y sus recesos.
struct TurnoDTO {
    QString nombre;
    QTime inicio;
    QTime fin;
    int numSlots = 0;

    QVector<RecesoDTO> recesos;
};

/// Servicio CRUD sobre las tablas Turnos y Recesos (RF-2).
class ServicioTurnosRecesos {
public:
    explicit ServicioTurnosRecesos(QSqlDatabase& db);

    // ─── Turnos ───────────────────────────────────────────────────────────

    Resultado<TurnoDTO> crearTurno(const QString& nombre, const QTime& inicio,
                                   const QTime& fin, int numSlots);

    Resultado<TurnoDTO> obtenerTurno(const QString& nombre) const;

    Resultado<QVector<TurnoDTO>> listarTurnos() const;

    Resultado<TurnoDTO> actualizarTurno(const QString& nombre, const QTime& inicio,
                                        const QTime& fin, int numSlots);

    bool eliminarTurno(const QString& nombre);

    // ─── Recesos ──────────────────────────────────────────────────────────

    Resultado<RecesoDTO> agregarReceso(const QString& turno, int despuesDeSlot,
                                       int duracion, const QTime& inicio = QTime(),
                                       const QTime& fin = QTime());

    bool eliminarReceso(const QString& turno, int despuesDeSlot);

private:
    QSqlDatabase& m_db;

    bool validarTurno(const QString& nombre, const QTime& inicio, const QTime& fin,
                      int numSlots, QString& error) const;
    TurnoDTO mapearARecord(const QSqlRecord& record) const;
    bool cargarRecesos(TurnoDTO& dto) const;
};
