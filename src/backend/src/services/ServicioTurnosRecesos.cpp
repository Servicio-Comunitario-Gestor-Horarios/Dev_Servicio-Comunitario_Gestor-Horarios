#include "backend/services/ServicioTurnosRecesos.hpp"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

static const QString FORMATO_HORA = QStringLiteral("HH:mm");

ServicioTurnosRecesos::ServicioTurnosRecesos(QSqlDatabase& db) : m_db(db) {}

bool ServicioTurnosRecesos::validarTurno(const QString& nombre, const QTime& inicio,
                                         const QTime& fin, int numSlots,
                                         QString& error) const {
    if (nombre.trimmed().isEmpty()) {
        error = "El nombre del turno no puede estar vacío.";
        return false;
    }

    if (!inicio.isValid() || !fin.isValid()) {
        error = "Las horas del turno no son válidas.";
        return false;
    }

    if (inicio >= fin) {
        error = "La hora de inicio debe ser anterior a la hora de fin.";
        return false;
    }

    if (numSlots <= 0) {
        error = "El número de slots debe ser mayor que cero.";
        return false;
    }

    return true;
}

TurnoDTO ServicioTurnosRecesos::mapearARecord(const QSqlRecord& record) const {
    TurnoDTO dto;
    dto.nombre = record.value("nombre").toString();
    dto.inicio = QTime::fromString(record.value("inicio").toString(), FORMATO_HORA);
    dto.fin = QTime::fromString(record.value("fin").toString(), FORMATO_HORA);
    dto.numSlots = record.value("slots").toInt();
    return dto;
}

bool ServicioTurnosRecesos::cargarRecesos(TurnoDTO& dto) const {
    QSqlQuery q(m_db);
    q.prepare(
        "SELECT despues_de_slot, duracion, inicio, fin "
        "FROM Recesos "
        "WHERE turno = :turno "
        "ORDER BY despues_de_slot"
    );
    q.bindValue(":turno", dto.nombre);

    if (!q.exec()) {
        qCritical() << "Error al cargar recesos:" << q.lastError().text();
        return false;
    }

    while (q.next()) {
        RecesoDTO r;
        r.despuesDeSlot = q.value("despues_de_slot").toInt();
        r.duracion = q.value("duracion").toInt();
        r.inicio = QTime::fromString(q.value("inicio").toString(), FORMATO_HORA);
        r.fin = QTime::fromString(q.value("fin").toString(), FORMATO_HORA);
        dto.recesos.append(r);
    }

    return true;
}

// ─── Turnos ────────────────────────────────────────────────────────────────

Resultado<TurnoDTO> ServicioTurnosRecesos::crearTurno(const QString& nombre,
                                                      const QTime& inicio,
                                                           const QTime& fin, int numSlots) {
    QString error;
    if (!validarTurno(nombre, inicio, fin, numSlots, error)) {
        return Resultado<TurnoDTO>::error(error);
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO Turnos (nombre, inicio, fin, slots) "
        "VALUES (:nombre, :inicio, :fin, :slots)"
    );
    query.bindValue(":nombre", nombre.trimmed());
    query.bindValue(":inicio", inicio.toString(FORMATO_HORA));
    query.bindValue(":fin", fin.toString(FORMATO_HORA));
    query.bindValue(":slots", numSlots);

    if (!query.exec()) {
        if (query.lastError().text().contains("UNIQUE") ||
            query.lastError().text().contains("PRIMARY KEY")) {
            return Resultado<TurnoDTO>::error("Ya existe un turno con ese nombre.", -2);
        }
        qCritical() << "Error al crear turno:" << query.lastError().text();
        return Resultado<TurnoDTO>::error("Error al guardar el turno en la base de datos.");
    }

    return obtenerTurno(nombre.trimmed());
}

Resultado<TurnoDTO> ServicioTurnosRecesos::obtenerTurno(const QString& nombre) const {
    if (nombre.trimmed().isEmpty()) {
        return Resultado<TurnoDTO>::error("Nombre de turno inválido.");
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT nombre, inicio, fin, slots FROM Turnos WHERE nombre = :nombre");
    query.bindValue(":nombre", nombre.trimmed());

    if (!query.exec()) {
        qCritical() << "Error al obtener turno:" << query.lastError().text();
        return Resultado<TurnoDTO>::error("Error al consultar la base de datos.");
    }

    if (!query.next()) {
        return Resultado<TurnoDTO>::error("No se encontró un turno con ese nombre.", -3);
    }

    TurnoDTO dto = mapearARecord(query.record());
    if (!cargarRecesos(dto)) {
        return Resultado<TurnoDTO>::error("Error al cargar los recesos del turno.");
    }

    return Resultado<TurnoDTO>::exito(dto);
}

QVector<TurnoDTO> ServicioTurnosRecesos::listarTurnos() const {
    QVector<TurnoDTO> resultados;

    QSqlQuery query(m_db);
    query.prepare("SELECT nombre, inicio, fin, slots FROM Turnos ORDER BY inicio, nombre");

    if (!query.exec()) {
        qCritical() << "Error al listar turnos:" << query.lastError().text();
        return resultados;
    }

    while (query.next()) {
        TurnoDTO dto = mapearARecord(query.record());
        cargarRecesos(dto);
        resultados.append(dto);
    }

    return resultados;
}

Resultado<TurnoDTO> ServicioTurnosRecesos::actualizarTurno(const QString& nombre,
                                                           const QTime& inicio,
                                                      const QTime& fin, int numSlots) {
    QString error;
    if (!validarTurno(nombre, inicio, fin, numSlots, error)) {
        return Resultado<TurnoDTO>::error(error);
    }

    QSqlQuery query(m_db);
    query.prepare(
        "UPDATE Turnos SET "
        "inicio = :inicio, "
        "fin = :fin, "
        "slots = :slots, "
        "fecha_modificacion = CURRENT_TIMESTAMP "
        "WHERE nombre = :nombre"
    );
    query.bindValue(":inicio", inicio.toString(FORMATO_HORA));
    query.bindValue(":fin", fin.toString(FORMATO_HORA));
    query.bindValue(":slots", numSlots);
    query.bindValue(":nombre", nombre.trimmed());

    if (!query.exec()) {
        qCritical() << "Error al actualizar turno:" << query.lastError().text();
        return Resultado<TurnoDTO>::error(
            "Error al actualizar el turno en la base de datos.");
    }

    if (query.numRowsAffected() == 0) {
        return Resultado<TurnoDTO>::error("No se encontró un turno con ese nombre.", -3);
    }

    return obtenerTurno(nombre.trimmed());
}

bool ServicioTurnosRecesos::eliminarTurno(const QString& nombre) {
    const QString nombreLimpio = nombre.trimmed();
    if (nombreLimpio.isEmpty()) return false;

    if (!m_db.transaction()) {
        qCritical() << "No se pudo iniciar la transacción para eliminar el turno.";
        return false;
    }

    // Los recesos no tienen ON DELETE CASCADE: se borran primero.
    QSqlQuery borrarRecesos(m_db);
    borrarRecesos.prepare("DELETE FROM Recesos WHERE turno = :turno");
    borrarRecesos.bindValue(":turno", nombreLimpio);

    if (!borrarRecesos.exec()) {
        qCritical() << "Error al eliminar recesos del turno:"
                    << borrarRecesos.lastError().text();
        m_db.rollback();
        return false;
    }

    QSqlQuery del(m_db);
    del.prepare("DELETE FROM Turnos WHERE nombre = :nombre");
    del.bindValue(":nombre", nombreLimpio);

    if (!del.exec()) {
        qCritical() << "Error al eliminar turno:" << del.lastError().text();
        m_db.rollback();
        return false;
    }

    const bool borrado = del.numRowsAffected() > 0;
    if (!borrado) {
        m_db.rollback();
        return false;
    }

    m_db.commit();
    return true;
}

// ─── Recesos ───────────────────────────────────────────────────────────────

Resultado<RecesoDTO> ServicioTurnosRecesos::agregarReceso(const QString& turno,
                                                          int despuesDeSlot,
                                                          int duracion,
                                                          const QTime& inicio,
                                                          const QTime& fin) {
    if (turno.trimmed().isEmpty()) {
        return Resultado<RecesoDTO>::error("Nombre de turno inválido.");
    }

    if (despuesDeSlot < 0) {
        return Resultado<RecesoDTO>::error("El slot del receso no puede ser negativo.");
    }

    if (duracion <= 0) {
        return Resultado<RecesoDTO>::error("La duración del receso debe ser mayor que cero.");
    }

    // Verificar que el turno exista antes de insertar (mejor mensaje que FK).
    auto turnoExistente = obtenerTurno(turno.trimmed());
    if (!turnoExistente.ok) {
        return Resultado<RecesoDTO>::error(turnoExistente.mensajeError,
                                           turnoExistente.codigoError);
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT OR REPLACE INTO Recesos "
        "(turno, despues_de_slot, duracion, inicio, fin) "
        "VALUES (:turno, :despues_de_slot, :duracion, :inicio, :fin)"
    );
    query.bindValue(":turno", turno.trimmed());
    query.bindValue(":despues_de_slot", despuesDeSlot);
    query.bindValue(":duracion", duracion);
    query.bindValue(":inicio", inicio.isValid() ? inicio.toString(FORMATO_HORA) : QString());
    query.bindValue(":fin", fin.isValid() ? fin.toString(FORMATO_HORA) : QString());

    if (!query.exec()) {
        qCritical() << "Error al agregar receso:" << query.lastError().text();
        return Resultado<RecesoDTO>::error("Error al guardar el receso.");
    }

    RecesoDTO dto;
    dto.despuesDeSlot = despuesDeSlot;
    dto.duracion = duracion;
    dto.inicio = inicio;
    dto.fin = fin;
    return Resultado<RecesoDTO>::exito(dto);
}

bool ServicioTurnosRecesos::eliminarReceso(const QString& turno, int despuesDeSlot) {
    if (turno.trimmed().isEmpty() || despuesDeSlot < 0) return false;

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM Recesos "
                  "WHERE turno = :turno AND despues_de_slot = :slot");
    query.bindValue(":turno", turno.trimmed());
    query.bindValue(":slot", despuesDeSlot);

    if (!query.exec()) {
        qCritical() << "Error al eliminar receso:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}
