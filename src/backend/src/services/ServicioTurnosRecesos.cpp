#include "backend/services/ServicioTurnosRecesos.hpp"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>

#include "backend/services/calculo_eventos.hpp"

static const QString FORMATO_HORA = QStringLiteral("HH:mm");

namespace
{
    QString formatearHora(const QTime& hora)
    {
        return hora.isValid() ? hora.toString(FORMATO_HORA) : QStringLiteral("—");
    }

    QString formatearIntervalo(const QTime& inicio, const QTime& fin)
    {
        return QStringLiteral("%1–%2").arg(formatearHora(inicio), formatearHora(fin));
    }

    /// Vista de receso de un evento (compatibilidad transitoria del contrato 1.0).
    RecesoDTO recesoDeEvento(const EventoDTO& evento)
    {
        RecesoDTO receso;
        receso.despuesDeSlot = evento.despuesDeSlot;
        receso.duracion      = evento.duracion;
        receso.inicio        = evento.inicio;
        receso.fin           = evento.fin;
        return receso;
    }
} // namespace

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

bool ServicioTurnosRecesos::validarEvento(const TurnoDTO& dto, const QString& nombre,
                                          int despuesDeSlot, int duracion, int orden,
                                          bool ordenObligatorio, QString& error) const {
    if (nombre.trimmed().isEmpty()) {
        error = "El nombre del evento no puede estar vacío.";
        return false;
    }

    if (despuesDeSlot < 1 || despuesDeSlot > dto.numSlots) {
        error = QStringLiteral("La posición del evento debe estar entre 1 y %1.")
                    .arg(dto.numSlots);
        return false;
    }

    if (duracion <= 0) {
        error = "La duración del evento debe ser mayor que cero.";
        return false;
    }

    if (orden < 0 || (ordenObligatorio && orden < 1)) {
        error = "El orden del evento debe ser mayor o igual que uno.";
        return false;
    }

    return true;
}

bool ServicioTurnosRecesos::validarCoherenciaTemporal(const TurnoDTO& turno,
                                                      const EventoDTO& candidato,
                                                      QString& error) const {
    const bool hayHoras = candidato.inicio.isValid() || candidato.fin.isValid();
    if (candidato.inicio.isValid() != candidato.fin.isValid()) {
        error = "Las horas del evento deben indicarse juntas (inicio y fin) o ninguna.";
        return false;
    }

    if (hayHoras) {
        if (candidato.fin <= candidato.inicio) {
            error = QStringLiteral("La hora de fin del evento (%1) debe ser posterior a la de"
                                   " inicio (%2).")
                        .arg(formatearHora(candidato.fin), formatearHora(candidato.inicio));
            return false;
        }

        const int minutosEntreHoras = candidato.inicio.secsTo(candidato.fin) / 60;
        if (minutosEntreHoras != candidato.duracion) {
            error = QStringLiteral("La duración del evento «%1» (%2 min) no coincide con la"
                                   " diferencia entre sus horas (%3 min).")
                        .arg(candidato.nombre)
                        .arg(candidato.duracion)
                        .arg(minutosEntreHoras);
            return false;
        }
    }

    // El reparto de la rejilla debe quedar en positivo: si los eventos ocupan
    // toda la ventana o no dejan un minuto por franja, no encajan (RF-3, RF-4).
    if (duracionFranjaEstimada(turno) <= 0) {
        error = QStringLiteral("Los eventos del turno «%1» no caben en su ventana de %2 a %3.")
                    .arg(turno.nombre,
                         formatearHora(turno.inicio),
                         formatearHora(turno.fin));
        return false;
    }

    if (hayHoras && !eventoDentroDeVentana(candidato, turno, candidato.despuesDeSlot)) {
        const QPair<QTime, QTime> ventana =
            ventanaDePosicion(turno, candidato.despuesDeSlot);
        error = QStringLiteral("El intervalo %1 del evento «%2» no cabe en la ventana de la"
                               " posición %3 (%4).")
                    .arg(formatearIntervalo(candidato.inicio, candidato.fin),
                         candidato.nombre,
                         QString::number(candidato.despuesDeSlot),
                         formatearIntervalo(ventana.first, ventana.second));
        return false;
    }

    if (haySolapeCronologico(turno.eventos)) {
        error = QStringLiteral("Los eventos del turno «%1» se solapan o van en orden"
                               " cronológico invertido: revisa las horas de «%2».")
                    .arg(turno.nombre, candidato.nombre);
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

bool ServicioTurnosRecesos::cargarEventos(TurnoDTO& dto) const {
    dto.eventos.clear();
    dto.recesos.clear();

    QSqlQuery q(m_db);
    q.prepare(
        "SELECT id, nombre, despues_de_slot, orden, duracion, inicio, fin "
        "FROM Eventos "
        "WHERE turno = :turno "
        "ORDER BY despues_de_slot, orden, id"
    );
    q.bindValue(":turno", dto.nombre);

    if (!q.exec()) {
        qCritical() << "Error al cargar eventos:" << q.lastError().text();
        return false;
    }

    while (q.next()) {
        EventoDTO e;
        e.id            = q.value("id").toInt();
        e.nombre        = q.value("nombre").toString();
        e.despuesDeSlot = q.value("despues_de_slot").toInt();
        e.orden         = q.value("orden").toInt();
        e.duracion      = q.value("duracion").toInt();
        e.inicio        = QTime::fromString(q.value("inicio").toString(), FORMATO_HORA);
        e.fin           = QTime::fromString(q.value("fin").toString(), FORMATO_HORA);
        dto.eventos.append(e);
    }

    // Compatibilidad transitoria: la sección de recesos se deriva de los eventos.
    for (const EventoDTO& evento : dto.eventos)
        dto.recesos.append(recesoDeEvento(evento));

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
    if (!cargarEventos(dto)) {
        return Resultado<TurnoDTO>::error("Error al cargar los eventos del turno.");
    }

    return Resultado<TurnoDTO>::exito(dto);
}

Resultado<QVector<TurnoDTO>> ServicioTurnosRecesos::listarTurnos() const {
    QVector<TurnoDTO> resultados;

    QSqlQuery query(m_db);
    query.prepare("SELECT nombre, inicio, fin, slots FROM Turnos ORDER BY inicio, nombre");

    if (!query.exec()) {
        qCritical() << "Error al listar turnos:" << query.lastError().text();
        return Resultado<QVector<TurnoDTO>>::error(
            QStringLiteral("No se pudieron leer los turnos registrados. Compruebe que la base de"
                           " datos esté disponible e intente de nuevo."));
    }

    while (query.next()) {
        TurnoDTO dto = mapearARecord(query.record());
        cargarEventos(dto);
        resultados.append(dto);
    }

    return Resultado<QVector<TurnoDTO>>::exito(resultados);
}

Resultado<TurnoDTO> ServicioTurnosRecesos::actualizarTurno(const QString& nombre,
                                                           const QTime& inicio,
                                                           const QTime& fin, int numSlots,
                                                           bool eliminarEventosFueraDeRango) {
    QString error;
    if (!validarTurno(nombre, inicio, fin, numSlots, error)) {
        return Resultado<TurnoDTO>::error(error);
    }

    auto existente = obtenerTurno(nombre.trimmed());
    if (!existente.ok) {
        return Resultado<TurnoDTO>::error(existente.mensajeError, existente.codigoError);
    }
    const TurnoDTO& anterior = existente.valor;

    // Eventos que la reducción de franjas dejaría fuera de rango.
    QVector<EventoDTO> afectados;
    if (numSlots < anterior.numSlots) {
        afectados = ::eventosFueraDeRango(anterior, numSlots);
        if (!afectados.isEmpty() && !eliminarEventosFueraDeRango) {
            QStringList lista;
            for (const EventoDTO& evento : afectados) {
                lista << QStringLiteral("#%1 «%2» (pos. %3)")
                             .arg(evento.id)
                             .arg(evento.nombre)
                             .arg(evento.despuesDeSlot);
            }
            return Resultado<TurnoDTO>::error(
                QStringLiteral("El turno «%1» quedaría con %2 franjas y tiene %3 evento%4"
                               " después de esa posición: %5. Confirma la eliminación para"
                               " continuar.")
                    .arg(anterior.nombre)
                    .arg(numSlots)
                    .arg(afectados.size())
                    .arg(afectados.size() == 1 ? QString() : QStringLiteral("s"))
                    .arg(lista.join(QStringLiteral(", "))));
        }
    }

    // Estado del turno con la ventana nueva y sin los eventos eliminados.
    TurnoDTO nuevo = anterior;
    nuevo.inicio   = inicio;
    nuevo.fin      = fin;
    nuevo.numSlots = numSlots;
    nuevo.eventos.clear();
    for (const EventoDTO& evento : anterior.eventos) {
        if (evento.despuesDeSlot <= numSlots)
            nuevo.eventos.append(evento);
    }

    if (!nuevo.eventos.isEmpty() && duracionFranjaEstimada(nuevo) <= 0) {
        return Resultado<TurnoDTO>::error(
            QStringLiteral("Los eventos del turno «%1» no caben en la nueva ventana de %2 a"
                           " %3 con %4 franjas.")
                .arg(nuevo.nombre,
                     formatearHora(inicio),
                     formatearHora(fin),
                     QString::number(numSlots)));
    }

    // La ventana nueva no puede dejar fuera el intervalo de un evento (RF-4).
    for (const EventoDTO& evento : nuevo.eventos) {
        if (!evento.inicio.isValid() || !evento.fin.isValid())
            continue;

        if (eventoDentroDeVentana(evento, nuevo, evento.despuesDeSlot))
            continue;

        const QPair<QTime, QTime> ventana =
            ventanaDePosicion(nuevo, evento.despuesDeSlot);
        return Resultado<TurnoDTO>::error(
            QStringLiteral("Cambiar la ventana del turno «%1» dejaría fuera el evento «%2»"
                           " (#%3, pos. %4): su intervalo %5 no cabe en la ventana %6.")
                .arg(nuevo.nombre,
                     evento.nombre,
                     QString::number(evento.id),
                     QString::number(evento.despuesDeSlot),
                     formatearIntervalo(evento.inicio, evento.fin),
                     formatearIntervalo(ventana.first, ventana.second)));
    }

    if (!m_db.transaction()) {
        qCritical() << "No se pudo iniciar la transacción para actualizar el turno.";
        return Resultado<TurnoDTO>::error("Error al actualizar el turno en la base de datos.");
    }

    // Borrado de los afectados y actualización del turno: una sola transacción.
    for (const EventoDTO& evento : afectados) {
        QSqlQuery borrar(m_db);
        borrar.prepare("DELETE FROM Eventos WHERE id = :id");
        borrar.bindValue(":id", evento.id);
        if (!borrar.exec()) {
            qCritical() << "Error al eliminar los eventos fuera de rango:"
                        << borrar.lastError().text();
            m_db.rollback();
            return Resultado<TurnoDTO>::error("Error al actualizar el turno en la base de datos.");
        }
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
    query.bindValue(":nombre", anterior.nombre);

    if (!query.exec() || query.numRowsAffected() == 0) {
        qCritical() << "Error al actualizar turno:" << query.lastError().text();
        m_db.rollback();
        return Resultado<TurnoDTO>::error("Error al actualizar el turno en la base de datos.");
    }

    if (!m_db.commit()) {
        qCritical() << "Error al confirmar la actualización del turno.";
        m_db.rollback();
        return Resultado<TurnoDTO>::error("Error al actualizar el turno en la base de datos.");
    }

    return obtenerTurno(anterior.nombre);
}

bool ServicioTurnosRecesos::eliminarTurno(const QString& nombre) {
    const QString nombreLimpio = nombre.trimmed();
    if (nombreLimpio.isEmpty()) return false;

    if (!m_db.transaction()) {
        qCritical() << "No se pudo iniciar la transacción para eliminar el turno.";
        return false;
    }

    // Los eventos referencian el turno sin ON DELETE CASCADE: se borran primero.
    QSqlQuery borrarEventos(m_db);
    borrarEventos.prepare("DELETE FROM Eventos WHERE turno = :turno");
    borrarEventos.bindValue(":turno", nombreLimpio);

    if (!borrarEventos.exec()) {
        qCritical() << "Error al eliminar los eventos del turno:"
                    << borrarEventos.lastError().text();
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

// ─── Eventos ───────────────────────────────────────────────────────────────

Resultado<EventoDTO> ServicioTurnosRecesos::agregarEvento(const QString& turno,
                                                          const QString& nombre,
                                                          int despuesDeSlot,
                                                          int duracion,
                                                          const QTime& inicio,
                                                          const QTime& fin,
                                                          int orden) {
    if (turno.trimmed().isEmpty()) {
        return Resultado<EventoDTO>::error("Nombre de turno inválido.");
    }

    // Valida que el turno exista y trae sus eventos (misma conexión).
    auto turnoExistente = obtenerTurno(turno.trimmed());
    if (!turnoExistente.ok) {
        return Resultado<EventoDTO>::error(turnoExistente.mensajeError,
                                           turnoExistente.codigoError);
    }
    const TurnoDTO& dto = turnoExistente.valor;

    QString error;
    if (!validarEvento(dto, nombre, despuesDeSlot, duracion, orden, false, error)) {
        return Resultado<EventoDTO>::error(error);
    }

    // El orden es el máximo de la posición + 1 cuando no se indica.
    const int ordenFinal = (orden == 0) ? ordenSiguiente(dto.eventos, despuesDeSlot)
                                        : orden;
    for (const EventoDTO& existente : dto.eventos) {
        if (existente.despuesDeSlot == despuesDeSlot && existente.orden == ordenFinal) {
            return Resultado<EventoDTO>::error(
                QStringLiteral("Ya existe un evento en la posición %1 del turno «%2» con"
                               " orden %3.")
                    .arg(despuesDeSlot)
                    .arg(dto.nombre)
                    .arg(ordenFinal));
        }
    }

    EventoDTO candidato;
    candidato.nombre        = nombre.trimmed();
    candidato.despuesDeSlot = despuesDeSlot;
    candidato.orden         = ordenFinal;
    candidato.duracion      = duracion;
    candidato.inicio        = inicio;
    candidato.fin           = fin;

    // Se valida el turno con el candidato dentro: ventana, rejilla y solapes.
    TurnoDTO conCandidato = dto;
    conCandidato.eventos.append(candidato);
    if (!validarCoherenciaTemporal(conCandidato, candidato, error)) {
        return Resultado<EventoDTO>::error(error);
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO Eventos "
        "(nombre, turno, despues_de_slot, orden, duracion, inicio, fin) "
        "VALUES (:nombre, :turno, :despues_de_slot, :orden, :duracion, :inicio, :fin)"
    );
    query.bindValue(":nombre", nombre.trimmed());
    query.bindValue(":turno", dto.nombre);
    query.bindValue(":despues_de_slot", despuesDeSlot);
    query.bindValue(":orden", ordenFinal);
    query.bindValue(":duracion", duracion);
    query.bindValue(":inicio", inicio.isValid() ? QVariant(inicio.toString(FORMATO_HORA))
                                                : QVariant());
    query.bindValue(":fin", fin.isValid() ? QVariant(fin.toString(FORMATO_HORA))
                                          : QVariant());

    if (!query.exec()) {
        qCritical() << "Error al agregar evento:" << query.lastError().text();
        return Resultado<EventoDTO>::error("Error al guardar el evento.");
    }

    EventoDTO creado;
    creado.id            = query.lastInsertId().toInt();
    creado.nombre        = nombre.trimmed();
    creado.despuesDeSlot = despuesDeSlot;
    creado.orden         = ordenFinal;
    creado.duracion      = duracion;
    creado.inicio        = inicio;
    creado.fin           = fin;
    return Resultado<EventoDTO>::exito(creado);
}

Resultado<EventoDTO> ServicioTurnosRecesos::actualizarEvento(int id, const QString& nombre,
                                                             int despuesDeSlot,
                                                             int duracion,
                                                             const QTime& inicio,
                                                             const QTime& fin, int orden) {
    if (id <= 0) {
        return Resultado<EventoDTO>::error("Identificador de evento inválido.");
    }

    // El turno del evento es inmutable: se lee para validarlo, no se acepta otro.
    QString turnoActual;
    {
        QSqlQuery query(m_db);
        query.prepare("SELECT turno FROM Eventos WHERE id = :id");
        query.bindValue(":id", id);

        if (!query.exec()) {
            qCritical() << "Error al obtener el evento:" << query.lastError().text();
            return Resultado<EventoDTO>::error("Error al consultar la base de datos.");
        }
        if (!query.next()) {
            return Resultado<EventoDTO>::error(
                "No se encontró un evento con ese identificador.", -4);
        }
        turnoActual = query.value("turno").toString();
    }

    auto turnoExistente = obtenerTurno(turnoActual);
    if (!turnoExistente.ok) {
        return Resultado<EventoDTO>::error(turnoExistente.mensajeError,
                                           turnoExistente.codigoError);
    }
    const TurnoDTO& dto = turnoExistente.valor;

    QString error;
    if (!validarEvento(dto, nombre, despuesDeSlot, duracion, orden, true, error)) {
        return Resultado<EventoDTO>::error(error);
    }

    // El esquema exige el par de horas completo; la coherencia temporal es de T4.
    if (inicio.isValid() != fin.isValid()) {
        return Resultado<EventoDTO>::error(
            "Las horas del evento deben indicarse juntas (inicio y fin) o ninguna.");
    }

    for (const EventoDTO& existente : dto.eventos) {
        if (existente.id != id && existente.despuesDeSlot == despuesDeSlot
            && existente.orden == orden) {
            return Resultado<EventoDTO>::error(
                QStringLiteral("Ya existe un evento en la posición %1 del turno «%2» con"
                               " orden %3.")
                    .arg(despuesDeSlot)
                    .arg(dto.nombre)
                    .arg(orden));
        }
    }

    EventoDTO candidato;
    candidato.id            = id;
    candidato.nombre        = nombre.trimmed();
    candidato.despuesDeSlot = despuesDeSlot;
    candidato.orden         = orden;
    candidato.duracion      = duracion;
    candidato.inicio        = inicio;
    candidato.fin           = fin;

    // El turno con el candidato sustituyendo al evento editado.
    TurnoDTO conCandidato = dto;
    conCandidato.eventos.clear();
    for (const EventoDTO& existente : dto.eventos) {
        if (existente.id != id)
            conCandidato.eventos.append(existente);
    }
    conCandidato.eventos.append(candidato);
    if (!validarCoherenciaTemporal(conCandidato, candidato, error)) {
        return Resultado<EventoDTO>::error(error);
    }

    QSqlQuery query(m_db);
    query.prepare(
        "UPDATE Eventos SET "
        "nombre = :nombre, "
        "despues_de_slot = :despues_de_slot, "
        "orden = :orden, "
        "duracion = :duracion, "
        "inicio = :inicio, "
        "fin = :fin, "
        "fecha_modificacion = CURRENT_TIMESTAMP "
        "WHERE id = :id"
    );
    query.bindValue(":nombre", nombre.trimmed());
    query.bindValue(":despues_de_slot", despuesDeSlot);
    query.bindValue(":orden", orden);
    query.bindValue(":duracion", duracion);
    query.bindValue(":inicio", inicio.isValid() ? QVariant(inicio.toString(FORMATO_HORA))
                                                : QVariant());
    query.bindValue(":fin", fin.isValid() ? QVariant(fin.toString(FORMATO_HORA))
                                          : QVariant());
    query.bindValue(":id", id);

    if (!query.exec()) {
        qCritical() << "Error al actualizar evento:" << query.lastError().text();
        return Resultado<EventoDTO>::error("Error al actualizar el evento.");
    }

    EventoDTO actualizado;
    actualizado.id            = id;
    actualizado.nombre        = nombre.trimmed();
    actualizado.despuesDeSlot = despuesDeSlot;
    actualizado.orden         = orden;
    actualizado.duracion      = duracion;
    actualizado.inicio        = inicio;
    actualizado.fin           = fin;
    return Resultado<EventoDTO>::exito(actualizado);
}

bool ServicioTurnosRecesos::eliminarEvento(int id) {
    if (id <= 0) return false;

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM Eventos WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qCritical() << "Error al eliminar evento:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

Resultado<QVector<EventoDTO>> ServicioTurnosRecesos::eventosFueraDeRango(
    const QString& turno, int numSlots) const {
    auto turnoExistente = obtenerTurno(turno.trimmed());
    if (!turnoExistente.ok) {
        return Resultado<QVector<EventoDTO>>::error(turnoExistente.mensajeError,
                                                    turnoExistente.codigoError);
    }

    return Resultado<QVector<EventoDTO>>::exito(
        ::eventosFueraDeRango(turnoExistente.valor, numSlots));
}

// ─── Recesos (compatibilidad transitoria; se retiran en T9) ────────────────

Resultado<RecesoDTO> ServicioTurnosRecesos::agregarReceso(const QString& turno,
                                                          int despuesDeSlot,
                                                          int duracion,
                                                          const QTime& inicio,
                                                          const QTime& fin) {
    auto evento = agregarEvento(turno, QStringLiteral("Receso"), despuesDeSlot, duracion,
                                inicio, fin);
    if (!evento.ok) {
        return Resultado<RecesoDTO>::error(evento.mensajeError, evento.codigoError);
    }

    return Resultado<RecesoDTO>::exito(recesoDeEvento(evento.valor));
}

bool ServicioTurnosRecesos::eliminarReceso(const QString& turno, int despuesDeSlot) {
    const QString nombreLimpio = turno.trimmed();
    if (nombreLimpio.isEmpty() || despuesDeSlot < 1) return false;

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM Eventos "
                  "WHERE turno = :turno AND despues_de_slot = :slot");
    query.bindValue(":turno", nombreLimpio);
    query.bindValue(":slot", despuesDeSlot);

    if (!query.exec()) {
        qCritical() << "Error al eliminar el receso (eventos de la posición):"
                    << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}
