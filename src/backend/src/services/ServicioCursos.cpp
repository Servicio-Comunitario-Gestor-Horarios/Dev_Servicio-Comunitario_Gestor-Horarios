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
    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO Cursos (anio, seccion, turno, aula) "
        "VALUES (:anio, :seccion, :turno, :aula)"
    );
    query.bindValue(":anio", anio);
    query.bindValue(":seccion", normalizarSeccion(seccion));
    query.bindValue(":turno", turno.trimmed().isEmpty() ? QVariant() : turno.trimmed());
    query.bindValue(":aula", aula > 0 ? QVariant(aula) : QVariant());

    if (!query.exec()) {
        qCritical() << "Error al crear curso:" << query.lastError().text();
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
    query.bindValue(":anio", anio);
    query.bindValue(":seccion", normalizarSeccion(seccion));
    query.bindValue(":turno", turno.trimmed().isEmpty() ? QVariant() : turno.trimmed());
    query.bindValue(":aula", aula > 0 ? QVariant(aula) : QVariant());
    query.bindValue(":id", id);

    if (!query.exec()) {
        qCritical() << "Error al actualizar curso:" << query.lastError().text();
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
