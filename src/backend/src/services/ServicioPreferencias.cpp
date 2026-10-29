#include "backend/services/ServicioPreferencias.hpp"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTime>

#include "backend/services/franja_horaria_validacion.hpp"

namespace
{
    const QString FORMATO_HORA = QStringLiteral("HH:mm");

    QString horaTexto(const QTime& hora)
    {
        return hora.toString(FORMATO_HORA);
    }

    /// Rango legible «HH:mm–HH:mm» para los mensajes.
    QString textoFranja(const FranjaHoraria& franja)
    {
        return QStringLiteral("%1–%2").arg(horaTexto(franja.inicio), horaTexto(franja.fin));
    }

    /// ¿Existe el docente? `SELECT` directo (no `ServicioProfesor`) para no crear ciclo.
    bool existeDocente(QSqlDatabase& db, const QString& idProfesor)
    {
        QSqlQuery query(db);
        query.prepare(QStringLiteral("SELECT 1 FROM Profesores WHERE id = :id"));
        query.bindValue(QStringLiteral(":id"), idProfesor);
        return query.exec() && query.next();
    }

    /// Preferencia desde una fila; `sentido` solo es válido si el texto es conocido.
    bool mapear(const QSqlRecord& record, PreferenciaDTO* dto)
    {
        dto->id = record.value(QStringLiteral("id")).toInt();
        dto->idProfesor = record.value(QStringLiteral("id_Profesor")).toString();
        dto->franja.dia = record.value(QStringLiteral("dia")).toInt();
        dto->franja.inicio =
            QTime::fromString(record.value(QStringLiteral("hora_inicio")).toString(),
                              FORMATO_HORA);
        dto->franja.fin =
            QTime::fromString(record.value(QStringLiteral("hora_fin")).toString(), FORMATO_HORA);
        return sentidoDesdeTexto(record.value(QStringLiteral("sentido")).toString(),
                                 &dto->sentido);
    }

    /// Filas del docente en un día, opcionalmente excluyendo la propia (`idExcluido`).
    QVector<PreferenciaDTO> preferenciasDelDia(QSqlDatabase& db, const QString& idProfesor,
                                               int dia, int idExcluido, bool* ok)
    {
        QVector<PreferenciaDTO> lista;

        QSqlQuery query(db);
        query.prepare(QStringLiteral(
            "SELECT id, id_Profesor, dia, hora_inicio, hora_fin, sentido "
            "FROM Preferencia_Profesor "
            "WHERE id_Profesor = :profesor AND dia = :dia AND id != :excluido"));
        query.bindValue(QStringLiteral(":profesor"), idProfesor);
        query.bindValue(QStringLiteral(":dia"), QString::number(dia));
        query.bindValue(QStringLiteral(":excluido"), idExcluido);

        if (!query.exec())
        {
            qCritical() << "Error al consultar las preferencias:" << query.lastError().text();
            *ok = false;
            return lista;
        }

        while (query.next())
        {
            PreferenciaDTO dto;
            if (!mapear(query.record(), &dto))
            {
                qCritical() << "Preferencia con un sentido no válido en la base.";
                *ok = false;
                return lista;
            }
            lista.append(dto);
        }

        *ok = true;
        return lista;
    }

    /// Error de duplicado exacto o de solape de sentidos opuestos, o cadena vacía.
    ///
    /// `existentes` son las demás preferencias del mismo docente y día; el duplicado
    /// exige mismo rango y sentido, y el solape solo se rechaza entre sentidos distintos.
    QString errorDeDuplicadoOSolape(const QVector<PreferenciaDTO>& existentes,
                                    const FranjaHoraria& franja, SentidoPreferencia sentido,
                                    const QString& idProfesor)
    {
        for (const PreferenciaDTO& otra : existentes)
        {
            if (otra.franja.inicio == franja.inicio && otra.franja.fin == franja.fin &&
                otra.sentido == sentido)
            {
                return QStringLiteral(
                           "Ya existe una preferencia idéntica para el docente «%1» el día %2 de "
                           "%3 a %4 (%5). Cambie el rango o el sentido, o edite la preferencia "
                           "existente.")
                    .arg(idProfesor)
                    .arg(franja.dia)
                    .arg(horaTexto(franja.inicio), horaTexto(franja.fin),
                         textoDeSentido(sentido));
            }

            if (otra.sentido != sentido && haySolape(franja, otra.franja))
            {
                return QStringLiteral(
                           "La preferencia %1 (%2) se solapa con %3 (%4) del docente «%5» el día "
                           "%6. Cambie el rango o el sentido de una de las dos.")
                    .arg(textoFranja(franja), textoDeSentido(sentido),
                         textoFranja(otra.franja), textoDeSentido(otra.sentido), idProfesor)
                    .arg(franja.dia);
            }
        }

        return QString();
    }
} // namespace

ServicioPreferencias::ServicioPreferencias(QSqlDatabase& db) : m_db(db) {}

Resultado<PreferenciaDTO> ServicioPreferencias::agregar(const QString& idProfesor,
                                                        const FranjaHoraria& franja,
                                                        SentidoPreferencia sentido)
{
    const QString id = idProfesor.trimmed();

    if (id.isEmpty())
    {
        return Resultado<PreferenciaDTO>::error(
            QStringLiteral("El docente es obligatorio para registrar una preferencia. Indique el "
                           "docente y vuelva a intentarlo."));
    }

    // Validación compartida de día y rango (misma que la disponibilidad).
    const QString errorFranja = franja_horaria::errorDeFranja(franja);
    if (!errorFranja.isEmpty())
        return Resultado<PreferenciaDTO>::error(errorFranja);

    if (!sentidoValido(sentido))
    {
        return Resultado<PreferenciaDTO>::error(
            QStringLiteral("El sentido de la preferencia debe ser «preferida» o «evitada»; se "
                           "recibió %1. Corrija el sentido.")
                .arg(static_cast<int>(sentido)));
    }

    if (!existeDocente(m_db, id))
    {
        return Resultado<PreferenciaDTO>::error(
            QStringLiteral("No existe el docente «%1». Regístrelo antes de guardar preferencias.")
                .arg(id));
    }

    bool consultaOk = false;
    const QVector<PreferenciaDTO> existentes =
        preferenciasDelDia(m_db, id, franja.dia, 0, &consultaOk);
    if (!consultaOk)
    {
        return Resultado<PreferenciaDTO>::error(
            QStringLiteral("No se pudieron comprobar las preferencias del docente «%1». Compruebe "
                           "la base e intente de nuevo.")
                .arg(id));
    }

    const QString errorRegla = errorDeDuplicadoOSolape(existentes, franja, sentido, id);
    if (!errorRegla.isEmpty())
        return Resultado<PreferenciaDTO>::error(errorRegla);

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO Preferencia_Profesor (id_Profesor, dia, hora_inicio, hora_fin, sentido) "
        "VALUES (:profesor, :dia, :inicio, :fin, :sentido)"));
    query.bindValue(QStringLiteral(":profesor"), id);
    query.bindValue(QStringLiteral(":dia"), QString::number(franja.dia));
    query.bindValue(QStringLiteral(":inicio"), horaTexto(franja.inicio));
    query.bindValue(QStringLiteral(":fin"), horaTexto(franja.fin));
    query.bindValue(QStringLiteral(":sentido"), textoDeSentido(sentido));

    if (!query.exec())
    {
        qCritical() << "Error al agregar la preferencia:" << query.lastError().text();
        return Resultado<PreferenciaDTO>::error(
            QStringLiteral("No se pudo guardar la preferencia; intente de nuevo."));
    }

    PreferenciaDTO creada;
    creada.id = query.lastInsertId().toInt();
    creada.idProfesor = id;
    creada.franja = franja;
    creada.sentido = sentido;
    return Resultado<PreferenciaDTO>::exito(creada);
}

Resultado<QVector<PreferenciaDTO>> ServicioPreferencias::listar(const QString& idProfesor) const
{
    QVector<PreferenciaDTO> lista;

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT id, id_Profesor, dia, hora_inicio, hora_fin, sentido "
        "FROM Preferencia_Profesor "
        "WHERE id_Profesor = :profesor "
        "ORDER BY CAST(dia AS INTEGER), hora_inicio, hora_fin, id"));
    query.bindValue(QStringLiteral(":profesor"), idProfesor.trimmed());

    if (!query.exec())
    {
        qCritical() << "Error al listar las preferencias:" << query.lastError().text();
        return Resultado<QVector<PreferenciaDTO>>::error(
            QStringLiteral("No se pudieron leer las preferencias del docente «%1». Compruebe que "
                           "la base esté disponible e intente de nuevo.")
                .arg(idProfesor));
    }

    while (query.next())
    {
        PreferenciaDTO dto;
        if (!mapear(query.record(), &dto))
        {
            return Resultado<QVector<PreferenciaDTO>>::error(
                QStringLiteral("No se pudieron leer las preferencias: hay una fila con un sentido "
                               "no válido. Recargue la lista e intente de nuevo."));
        }
        lista.append(dto);
    }

    return Resultado<QVector<PreferenciaDTO>>::exito(lista);
}

Resultado<PreferenciaDTO> ServicioPreferencias::actualizar(int id, const FranjaHoraria& franja,
                                                           SentidoPreferencia sentido)
{
    if (id <= 0)
    {
        return Resultado<PreferenciaDTO>::error(
            QStringLiteral("El identificador de la preferencia no es válido; se recibió %1. "
                           "Recargue la lista y vuelva a intentarlo.")
                .arg(id));
    }

    PreferenciaDTO actual;
    {
        QSqlQuery query(m_db);
        query.prepare(QStringLiteral(
            "SELECT id, id_Profesor, dia, hora_inicio, hora_fin, sentido "
            "FROM Preferencia_Profesor WHERE id = :id"));
        query.bindValue(QStringLiteral(":id"), id);

        if (!query.exec())
        {
            qCritical() << "Error al obtener la preferencia:" << query.lastError().text();
            return Resultado<PreferenciaDTO>::error(
                QStringLiteral("No se pudo consultar la preferencia %1. Compruebe la base e "
                               "intente de nuevo.")
                    .arg(id));
        }

        if (!query.next() || !mapear(query.record(), &actual))
        {
            return Resultado<PreferenciaDTO>::error(
                QStringLiteral("No se encontró la preferencia con id %1. Recargue la lista y "
                               "vuelva a intentarlo.")
                    .arg(id));
        }
    }

    if (!existeDocente(m_db, actual.idProfesor))
    {
        return Resultado<PreferenciaDTO>::error(
            QStringLiteral("No existe el docente «%1» de la preferencia. Recargue la lista y "
                           "vuelva a intentarlo.")
                .arg(actual.idProfesor));
    }

    const QString errorFranja = franja_horaria::errorDeFranja(franja);
    if (!errorFranja.isEmpty())
        return Resultado<PreferenciaDTO>::error(errorFranja);

    if (!sentidoValido(sentido))
    {
        return Resultado<PreferenciaDTO>::error(
            QStringLiteral("El sentido de la preferencia debe ser «preferida» o «evitada»; se "
                           "recibió %1. Corrija el sentido.")
                .arg(static_cast<int>(sentido)));
    }

    // Duplicado y solape contra el **día nuevo**, excluyendo la propia fila (RF-2).
    bool consultaOk = false;
    const QVector<PreferenciaDTO> demas =
        preferenciasDelDia(m_db, actual.idProfesor, franja.dia, id, &consultaOk);
    if (!consultaOk)
    {
        return Resultado<PreferenciaDTO>::error(
            QStringLiteral("No se pudieron comprobar las preferencias del docente «%1». Compruebe "
                           "la base e intente de nuevo.")
                .arg(actual.idProfesor));
    }

    const QString errorRegla =
        errorDeDuplicadoOSolape(demas, franja, sentido, actual.idProfesor);
    if (!errorRegla.isEmpty())
        return Resultado<PreferenciaDTO>::error(errorRegla);

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "UPDATE Preferencia_Profesor SET "
        "dia = :dia, hora_inicio = :inicio, hora_fin = :fin, sentido = :sentido, "
        "fecha_modificacion = CURRENT_TIMESTAMP "
        "WHERE id = :id"));
    query.bindValue(QStringLiteral(":dia"), QString::number(franja.dia));
    query.bindValue(QStringLiteral(":inicio"), horaTexto(franja.inicio));
    query.bindValue(QStringLiteral(":fin"), horaTexto(franja.fin));
    query.bindValue(QStringLiteral(":sentido"), textoDeSentido(sentido));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec())
    {
        qCritical() << "Error al actualizar la preferencia:" << query.lastError().text();
        return Resultado<PreferenciaDTO>::error(
            QStringLiteral("No se pudo actualizar la preferencia; intente de nuevo."));
    }

    actual.franja = franja;
    actual.sentido = sentido;
    return Resultado<PreferenciaDTO>::exito(actual);
}

Resultado<bool> ServicioPreferencias::eliminar(int id)
{
    if (id <= 0)
    {
        return Resultado<bool>::error(
            QStringLiteral("El identificador de la preferencia no es válido; se recibió %1. "
                           "Recargue la lista y vuelva a intentarlo.")
                .arg(id));
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM Preferencia_Profesor WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec())
    {
        qCritical() << "Error al eliminar la preferencia:" << query.lastError().text();
        return Resultado<bool>::error(
            QStringLiteral("No se pudo eliminar la preferencia; intente de nuevo."));
    }

    if (query.numRowsAffected() == 0)
    {
        return Resultado<bool>::error(
            QStringLiteral("No se encontró la preferencia con id %1. Recargue la lista y vuelva a "
                           "intentarlo.")
                .arg(id));
    }

    return Resultado<bool>::exito(true);
}

Resultado<int> ServicioPreferencias::limpiar(const QString& idProfesor)
{
    const QString id = idProfesor.trimmed();

    if (id.isEmpty())
    {
        return Resultado<int>::error(
            QStringLiteral("El docente es obligatorio para limpiar sus preferencias. Indique el "
                           "docente y vuelva a intentarlo."));
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM Preferencia_Profesor WHERE id_Profesor = :profesor"));
    query.bindValue(QStringLiteral(":profesor"), id);

    if (!query.exec())
    {
        qCritical() << "Error al limpiar las preferencias:" << query.lastError().text();
        return Resultado<int>::error(
            QStringLiteral("No se pudieron limpiar las preferencias del docente «%1». Compruebe "
                           "la base e intente de nuevo.")
                .arg(id));
    }

    return Resultado<int>::exito(query.numRowsAffected());
}
