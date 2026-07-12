#include "backend/services/ServicioPlanesEstudio.hpp"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>

ServicioPlanesEstudio::ServicioPlanesEstudio(QSqlDatabase& db) : m_db(db) {}

bool ServicioPlanesEstudio::validarPlan(const QString& codigo, const QString& nombre,
                                        QString& error) const {
    if (codigo.trimmed().isEmpty()) {
        error = "El código del plan no puede estar vacío.";
        return false;
    }

    if (codigo.trimmed().length() > 20) {
        error = "El código del plan no puede exceder 20 caracteres.";
        return false;
    }

    if (nombre.trimmed().isEmpty()) {
        error = "El nombre del plan no puede estar vacío.";
        return false;
    }

    return true;
}

PlanDTO ServicioPlanesEstudio::mapearARecord(const QSqlRecord& record) const {
    PlanDTO dto;
    dto.codigo = record.value("codigo").toString();
    dto.nombre = record.value("nombre").toString();
    dto.descripcion = record.value("descripcion").toString();
    return dto;
}

// ─── CRUD ──────────────────────────────────────────────────────────────────

Resultado<PlanDTO> ServicioPlanesEstudio::crearPlan(const QString& codigo, const QString& nombre,
                                                    const QString& descripcion) {
    QString error;
    if (!validarPlan(codigo, nombre, error)) {
        return Resultado<PlanDTO>::error(error);
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO PlanEstudio (codigo, nombre, descripcion) "
        "VALUES (:codigo, :nombre, :descripcion)"
    );
    query.bindValue(":codigo", codigo.trimmed());
    query.bindValue(":nombre", nombre.trimmed());
    query.bindValue(":descripcion", descripcion.trimmed());

    if (!query.exec()) {
        if (query.lastError().text().contains("UNIQUE") ||
            query.lastError().text().contains("PRIMARY KEY")) {
            return Resultado<PlanDTO>::error("Ya existe un plan con ese código.", -2);
        }
        qCritical() << "Error al crear plan:" << query.lastError().text();
        return Resultado<PlanDTO>::error("Error al guardar el plan en la base de datos.");
    }

    return obtenerPlan(codigo);
}

Resultado<PlanDTO> ServicioPlanesEstudio::obtenerPlan(const QString& codigo) const {
    if (codigo.trimmed().isEmpty()) {
        return Resultado<PlanDTO>::error("Código de plan inválido.");
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT codigo, nombre, descripcion FROM PlanEstudio WHERE codigo = :codigo");
    query.bindValue(":codigo", codigo.trimmed());

    if (!query.exec()) {
        qCritical() << "Error al obtener plan:" << query.lastError().text();
        return Resultado<PlanDTO>::error("Error al consultar la base de datos.");
    }

    if (!query.next()) {
        return Resultado<PlanDTO>::error("No se encontró un plan con ese código.", -3);
    }

    return Resultado<PlanDTO>::exito(mapearARecord(query.record()));
}

QVector<PlanDTO> ServicioPlanesEstudio::listarPlanes() const {
    QVector<PlanDTO> resultados;

    QSqlQuery query(m_db);
    query.prepare("SELECT codigo, nombre, descripcion FROM PlanEstudio ORDER BY codigo");

    if (!query.exec()) {
        qCritical() << "Error al listar planes:" << query.lastError().text();
        return resultados;
    }

    while (query.next()) {
        resultados.append(mapearARecord(query.record()));
    }

    return resultados;
}

Resultado<PlanDTO> ServicioPlanesEstudio::actualizarPlan(const QString& codigo, const QString& nombre,
                                                         const QString& descripcion) {
    QString error;
    if (!validarPlan(codigo, nombre, error)) {
        return Resultado<PlanDTO>::error(error);
    }

    QSqlQuery query(m_db);
    query.prepare(
        "UPDATE PlanEstudio SET "
        "nombre = :nombre, "
        "descripcion = :descripcion, "
        "fecha_modificacion = CURRENT_TIMESTAMP "
        "WHERE codigo = :codigo"
    );
    query.bindValue(":nombre", nombre.trimmed());
    query.bindValue(":descripcion", descripcion.trimmed());
    query.bindValue(":codigo", codigo.trimmed());

    if (!query.exec()) {
        qCritical() << "Error al actualizar plan:" << query.lastError().text();
        return Resultado<PlanDTO>::error("Error al actualizar el plan en la base de datos.");
    }

    if (query.numRowsAffected() == 0) {
        return Resultado<PlanDTO>::error("No se encontró un plan con ese código.", -3);
    }

    return obtenerPlan(codigo);
}

bool ServicioPlanesEstudio::eliminarPlan(const QString& codigo) {
    if (codigo.trimmed().isEmpty()) {
        return false;
    }

    // Verificar si tiene materias asociadas
    QSqlQuery checkQuery(m_db);
    checkQuery.prepare("SELECT 1 FROM PlanEstudio_Materia WHERE codigo_PlanEstudio = :codigo");
    checkQuery.bindValue(":codigo", codigo.trimmed());

    if (checkQuery.exec() && checkQuery.next()) {
        qWarning() << "No se puede eliminar el plan porque tiene materias asociadas.";
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM PlanEstudio WHERE codigo = :codigo");
    query.bindValue(":codigo", codigo.trimmed());

    if (!query.exec()) {
        qCritical() << "Error al eliminar plan:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}