#include "backend/services/ServicioCursos.hpp"
#include "backend/services/ordenacion.hpp"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include <algorithm>

ServicioCursos::ServicioCursos(QSqlDatabase& db) : m_db(db) {}

CursoDTO ServicioCursos::mapearARecord(const QSqlRecord& record) const {
    CursoDTO dto;
    dto.id      = record.value("id").toInt();
    dto.anio    = record.value("anio").toInt();
    dto.seccion = record.value("seccion").toString();
    dto.turno   = record.value("turno").toString();

    const QVariant aula = record.value("aula");
    dto.aula = aula.isNull() ? -1 : aula.toInt();

    return dto;
}

// ─── CRUD ──────────────────────────────────────────────────────────────────

Resultado<CursoDTO> ServicioCursos::crearCurso(int anio, const QString& seccion,
                                               const QString& turno, int aula) {
    const auto identidad = validarIdentidadCurso(anio, seccion);
    if (!identidad.ok) {
        return Resultado<CursoDTO>::error(identidad.mensajeError);
    }

    if (aula > 0 && !existeAula(aula)) {
        return Resultado<CursoDTO>::error(
            QStringLiteral("El aula indicada no existe; elija un aula registrada."));
    }

    if (existeCurso(identidad.valor.anio, identidad.valor.seccion)) {
        return Resultado<CursoDTO>::error(
            QStringLiteral("Ya existe un curso con el año %1 y la sección «%2». "
                           "Elija otro año o sección.")
                .arg(identidad.valor.anio)
                .arg(identidad.valor.seccion),
            -2);
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO Cursos (anio, seccion, turno, aula) "
        "VALUES (:anio, :seccion, :turno, :aula)"
    );
    query.bindValue(":anio", identidad.valor.anio);
    query.bindValue(":seccion", identidad.valor.seccion);
    query.bindValue(":turno", turno.trimmed().isEmpty() ? QVariant() : turno.trimmed());
    query.bindValue(":aula", aula > 0 ? QVariant(aula) : QVariant());

    if (!query.exec()) {
        const QString err = query.lastError().text();
        if (err.contains(QStringLiteral("UNIQUE"))) {
            // Defensa en profundidad: la unicidad la garantiza el esquema (RF-1).
            return Resultado<CursoDTO>::error(
                QStringLiteral("Ya existe un curso con el año %1 y la sección «%2». "
                               "Elija otro año o sección.")
                    .arg(identidad.valor.anio)
                    .arg(identidad.valor.seccion),
                -2);
        }
        if (err.contains(QStringLiteral("FOREIGN KEY"))) {
            return Resultado<CursoDTO>::error(
                QStringLiteral("El aula indicada no existe; elija un aula registrada."));
        }
        qCritical() << "Error al crear curso:" << err;
        return Resultado<CursoDTO>::error("Error al guardar el curso en la base de datos.");
    }

    return obtenerCurso(query.lastInsertId().toInt());
}

Resultado<CursoDTO> ServicioCursos::obtenerCurso(int id) const {
    if (id <= 0) {
        return Resultado<CursoDTO>::error("ID de curso inválido.");
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT id, anio, seccion, turno, aula FROM Cursos WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qCritical() << "Error al obtener curso:" << query.lastError().text();
        return Resultado<CursoDTO>::error("Error al consultar la base de datos.");
    }

    if (!query.next()) {
        return Resultado<CursoDTO>::error("No se encontró un curso con ese ID.", -3);
    }

    return Resultado<CursoDTO>::exito(mapearARecord(query.record()));
}

Resultado<QVector<CursoDTO>> ServicioCursos::listarCursos() const {
    QVector<CursoDTO> resultados;

    QSqlQuery query(m_db);
    query.prepare("SELECT id, anio, seccion, turno, aula FROM Cursos");

    if (!query.exec()) {
        qCritical() << "Error al listar cursos:" << query.lastError().text();
        return Resultado<QVector<CursoDTO>>::error(
            QStringLiteral("No se pudieron leer los cursos registrados. Compruebe que la base de"
                           " datos esté disponible e intente de nuevo."));
    }

    while (query.next()) {
        resultados.append(mapearARecord(query.record()));
    }

    std::stable_sort(resultados.begin(), resultados.end(),
                     [](const CursoDTO& a, const CursoDTO& b) {
                         return cursoAntes(a.anio, a.seccion, b.anio, b.seccion);
                     });

    return Resultado<QVector<CursoDTO>>::exito(resultados);
}

Resultado<CursoDTO> ServicioCursos::actualizarCurso(int id, int anio, const QString& seccion,
                                                    const QString& turno, int aula) {
    if (id <= 0) {
        return Resultado<CursoDTO>::error("ID de curso inválido.");
    }

    const auto identidad = validarIdentidadCurso(anio, seccion);
    if (!identidad.ok) {
        return Resultado<CursoDTO>::error(identidad.mensajeError);
    }

    if (aula > 0 && !existeAula(aula)) {
        return Resultado<CursoDTO>::error(
            QStringLiteral("El aula indicada no existe; elija un aula registrada."));
    }

    // La unicidad no debe rechazar el propio registro al no cambiar su identidad.
    if (existeCurso(identidad.valor.anio, identidad.valor.seccion, id)) {
        return Resultado<CursoDTO>::error(
            QStringLiteral("Ya existe un curso con el año %1 y la sección «%2». "
                           "Elija otro año o sección.")
                .arg(identidad.valor.anio)
                .arg(identidad.valor.seccion),
            -2);
    }

    QSqlQuery query(m_db);
    query.prepare(
        "UPDATE Cursos SET "
        "anio = :anio, "
        "seccion = :seccion, "
        "turno = :turno, "
        "aula = :aula, "
        "fecha_modificacion = CURRENT_TIMESTAMP "
        "WHERE id = :id"
    );
    query.bindValue(":anio", identidad.valor.anio);
    query.bindValue(":seccion", identidad.valor.seccion);
    query.bindValue(":turno", turno.trimmed().isEmpty() ? QVariant() : turno.trimmed());
    query.bindValue(":aula", aula > 0 ? QVariant(aula) : QVariant());
    query.bindValue(":id", id);

    if (!query.exec()) {
        const QString err = query.lastError().text();
        if (err.contains(QStringLiteral("UNIQUE"))) {
            return Resultado<CursoDTO>::error(
                QStringLiteral("Ya existe un curso con ese año y sección. Elija otro año o sección."),
                -2);
        }
        if (err.contains(QStringLiteral("FOREIGN KEY"))) {
            return Resultado<CursoDTO>::error(
                QStringLiteral("El aula indicada no existe; elija un aula registrada."));
        }
        qCritical() << "Error al actualizar curso:" << err;
        return Resultado<CursoDTO>::error(
            "Error al actualizar el curso en la base de datos.");
    }

    if (query.numRowsAffected() == 0) {
        return Resultado<CursoDTO>::error("No se encontró un curso con ese ID.", -3);
    }

    return obtenerCurso(id);
}

bool ServicioCursos::eliminarCurso(int id) {
    if (id <= 0) return false;

    QSqlQuery del(m_db);
    del.prepare("DELETE FROM Cursos WHERE id = :id");
    del.bindValue(":id", id);

    if (!del.exec()) {
        qCritical() << "Error al eliminar curso:" << del.lastError().text();
        return false;
    }

    return del.numRowsAffected() > 0;
}

bool ServicioCursos::existeCurso(int anio, const QString& seccion, int exceptoId) const {
    QSqlQuery query(m_db);
    query.prepare("SELECT 1 FROM Cursos WHERE anio = :anio AND seccion = :seccion "
                  "AND id <> :excepto LIMIT 1");
    query.bindValue(":anio", anio);
    query.bindValue(":seccion", seccion);
    query.bindValue(":excepto", exceptoId);
    return query.exec() && query.next();
}

bool ServicioCursos::existeAula(int idAula) const {
    QSqlQuery query(m_db);
    query.prepare("SELECT 1 FROM Aulas WHERE id = :id LIMIT 1");
    query.bindValue(":id", idAula);
    return query.exec() && query.next();
}
