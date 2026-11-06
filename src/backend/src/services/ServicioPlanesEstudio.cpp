#include "backend/services/ServicioPlanesEstudio.hpp"
#include "backend/services/ordenacion.hpp"

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>

#include <algorithm>
#include <QMap>
#include <QSet>

ServicioPlanesEstudio::ServicioPlanesEstudio(QSqlDatabase& db) : m_db(db) {}

bool ServicioPlanesEstudio::esCodigoNumerico(const QString& codigo) {
    if (codigo.trimmed().isEmpty()) {
        return false;
    }
    for (const QChar c : codigo.trimmed()) {
        if (!c.isDigit()) {
            return false;
        }
    }
    return true;
}

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

bool ServicioPlanesEstudio::validarMaterias(const QVector<PlanMateriaDTO>& materias,
                                            QString& error) const {
    if (materias.isEmpty()) {
        error = "El plan debe tener al menos una materia.";
        return false;
    }

    QSet<int> idsVistos;
    QMap<int, int> horasPorCurso;

    for (const PlanMateriaDTO& materia : materias) {
        if (materia.idMateria <= 0) {
            error = "Hay una materia inválida en el plan.";
            return false;
        }

        if (idsVistos.contains(materia.idMateria)) {
            error = "No se puede repetir la misma materia en el plan.";
            return false;
        }
        idsVistos.insert(materia.idMateria);

        if (materia.curso < 1 || materia.curso > 6) {
            error = "El año de cada materia debe estar entre 1 y 6.";
            return false;
        }

        if (materia.horas <= 0) {
            error = "Las horas semanales de cada materia deben ser mayores a 0.";
            return false;
        }

        if (materia.horas > MAX_HORAS_SEMANALES_POR_ANIO) {
            error = QString("Una materia no puede tener más de %1 horas semanales.")
                        .arg(MAX_HORAS_SEMANALES_POR_ANIO);
            return false;
        }

        horasPorCurso[materia.curso] += materia.horas;
        if (horasPorCurso[materia.curso] > MAX_HORAS_SEMANALES_POR_ANIO) {
            error = QString("El año %1 supera el máximo de %2 horas semanales.")
                        .arg(materia.curso)
                        .arg(MAX_HORAS_SEMANALES_POR_ANIO);
            return false;
        }
    }

    return true;
}

bool ServicioPlanesEstudio::nombrePlanDisponible(const QString& nombre,
                                                 const QString& codigoExcluido,
                                                 QString& error) const {
    QSqlQuery query(m_db);
    query.prepare("SELECT codigo, nombre FROM PlanEstudio");

    if (!query.exec()) {
        qCritical() << "Error al comprobar el nombre del plan:" << query.lastError().text();
        error = "Error al consultar la base de datos.";
        return false;
    }

    while (query.next()) {
        const QString codigo = query.value("codigo").toString();
        if (!codigoExcluido.isEmpty() && codigo == codigoExcluido) {
            continue;
        }
        const QString existente = query.value("nombre").toString().trimmed();
        if (QString::compare(existente, nombre.trimmed(), Qt::CaseInsensitive) == 0) {
            error = "Ya existe un plan de estudio con ese nombre.";
            return false;
        }
    }

    return true;
}

bool ServicioPlanesEstudio::materiaExiste(int idMateria) const {
    QSqlQuery query(m_db);
    query.prepare("SELECT 1 FROM Materias WHERE id = :id");
    query.bindValue(":id", idMateria);
    if (!query.exec()) {
        qCritical() << "Error al comprobar la materia:" << query.lastError().text();
        return false;
    }
    return query.next();
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

    if (!nombrePlanDisponible(nombre, QString(), error)) {
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

Resultado<QVector<PlanDTO>> ServicioPlanesEstudio::listarPlanes() const {
    QVector<PlanDTO> resultados;

    QSqlQuery query(m_db);
    query.prepare("SELECT codigo, nombre, descripcion FROM PlanEstudio ORDER BY codigo");

    if (!query.exec()) {
        qCritical() << "Error al listar planes:" << query.lastError().text();
        return Resultado<QVector<PlanDTO>>::error(
            QStringLiteral("No se pudieron leer los planes de estudio registrados. Compruebe que"
                           " la base de datos esté disponible e intente de nuevo."));
    }

    while (query.next()) {
        resultados.append(mapearARecord(query.record()));
    }

    std::stable_sort(resultados.begin(), resultados.end(),
                     [](const PlanDTO& a, const PlanDTO& b) {
                         return nombreAntes(a.nombre, b.nombre);
                     });

    return Resultado<QVector<PlanDTO>>::exito(resultados);
}

Resultado<PlanDTO> ServicioPlanesEstudio::actualizarPlan(const QString& codigo, const QString& nombre,
                                                         const QString& descripcion) {
    QString error;
    if (!validarPlan(codigo, nombre, error)) {
        return Resultado<PlanDTO>::error(error);
    }

    if (!nombrePlanDisponible(nombre, codigo.trimmed(), error)) {
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

Resultado<PlanDTO> ServicioPlanesEstudio::crearPlanCompleto(
    const QString& codigo, const QString& nombre, const QString& descripcion,
    const QVector<PlanMateriaDTO>& materias) {
    QString error;
    if (!validarPlan(codigo, nombre, error)) {
        return Resultado<PlanDTO>::error(error);
    }

    const QString codigoLimpio = codigo.trimmed();
    if (!esCodigoNumerico(codigoLimpio)) {
        return Resultado<PlanDTO>::error(
            "El código del plan debe contener solo números.");
    }

    if (!nombrePlanDisponible(nombre, QString(), error)) {
        return Resultado<PlanDTO>::error(error);
    }

    if (!validarMaterias(materias, error)) {
        return Resultado<PlanDTO>::error(error);
    }

    for (const PlanMateriaDTO& materia : materias) {
        if (!materiaExiste(materia.idMateria)) {
            return Resultado<PlanDTO>::error(
                "Una de las materias seleccionadas ya no existe en el catálogo.");
        }
    }

    if (!m_db.transaction()) {
        qCritical() << "Error al iniciar transacción de plan:"
                    << m_db.lastError().text();
        return Resultado<PlanDTO>::error("Error al iniciar la transacción.");
    }

    QSqlQuery planQuery(m_db);
    planQuery.prepare(
        "INSERT INTO PlanEstudio (codigo, nombre, descripcion) "
        "VALUES (:codigo, :nombre, :descripcion)");
    planQuery.bindValue(":codigo", codigoLimpio);
    planQuery.bindValue(":nombre", nombre.trimmed());
    planQuery.bindValue(":descripcion", descripcion.trimmed());

    if (!planQuery.exec()) {
        const QString sqlError = planQuery.lastError().text();
        m_db.rollback();
        if (sqlError.contains("UNIQUE") || sqlError.contains("PRIMARY KEY")) {
            return Resultado<PlanDTO>::error("Ya existe un plan con ese código.", -2);
        }
        qCritical() << "Error al crear plan completo:" << sqlError;
        return Resultado<PlanDTO>::error("Error al guardar el plan en la base de datos.");
    }

    QSqlQuery materiaQuery(m_db);
    materiaQuery.prepare(
        "INSERT INTO PlanEstudio_Materia (codigo_PlanEstudio, id_Materia, curso, horas) "
        "VALUES (:codigo, :idMateria, :curso, :horas)");

    for (const PlanMateriaDTO& materia : materias) {
        materiaQuery.bindValue(":codigo", codigoLimpio);
        materiaQuery.bindValue(":idMateria", materia.idMateria);
        materiaQuery.bindValue(":curso", materia.curso);
        materiaQuery.bindValue(":horas", materia.horas);

        if (!materiaQuery.exec()) {
            qCritical() << "Error al asociar materia al plan:"
                        << materiaQuery.lastError().text();
            m_db.rollback();
            return Resultado<PlanDTO>::error(
                "Error al guardar las materias del plan en la base de datos.");
        }
    }

    if (!m_db.commit()) {
        qCritical() << "Error al confirmar el plan:" << m_db.lastError().text();
        m_db.rollback();
        return Resultado<PlanDTO>::error("Error al confirmar la transacción.");
    }

    return obtenerPlan(codigoLimpio);
}

Resultado<PlanDTO> ServicioPlanesEstudio::actualizarPlanCompleto(
    const QString& codigo, const QString& nombre, const QString& descripcion,
    const QVector<PlanMateriaDTO>& materias) {
    QString error;
    if (!validarPlan(codigo, nombre, error)) {
        return Resultado<PlanDTO>::error(error);
    }

    const QString codigoLimpio = codigo.trimmed();
    const auto existente = obtenerPlan(codigoLimpio);
    if (!existente.ok) {
        return existente;
    }

    if (!nombrePlanDisponible(nombre, codigoLimpio, error)) {
        return Resultado<PlanDTO>::error(error);
    }

    if (!validarMaterias(materias, error)) {
        return Resultado<PlanDTO>::error(error);
    }

    for (const PlanMateriaDTO& materia : materias) {
        if (!materiaExiste(materia.idMateria)) {
            return Resultado<PlanDTO>::error(
                "Una de las materias seleccionadas ya no existe en el catálogo.");
        }
    }

    if (!m_db.transaction()) {
        qCritical() << "Error al iniciar transacción de plan:"
                    << m_db.lastError().text();
        return Resultado<PlanDTO>::error("Error al iniciar la transacción.");
    }

    QSqlQuery planQuery(m_db);
    planQuery.prepare(
        "UPDATE PlanEstudio SET "
        "nombre = :nombre, descripcion = :descripcion, "
        "fecha_modificacion = CURRENT_TIMESTAMP "
        "WHERE codigo = :codigo");
    planQuery.bindValue(":nombre", nombre.trimmed());
    planQuery.bindValue(":descripcion", descripcion.trimmed());
    planQuery.bindValue(":codigo", codigoLimpio);

    if (!planQuery.exec()) {
        qCritical() << "Error al actualizar plan completo:"
                    << planQuery.lastError().text();
        m_db.rollback();
        return Resultado<PlanDTO>::error("Error al actualizar el plan en la base de datos.");
    }

    QSqlQuery borradoQuery(m_db);
    borradoQuery.prepare("DELETE FROM PlanEstudio_Materia WHERE codigo_PlanEstudio = :codigo");
    borradoQuery.bindValue(":codigo", codigoLimpio);

    if (!borradoQuery.exec()) {
        qCritical() << "Error al reemplazar materias del plan:"
                    << borradoQuery.lastError().text();
        m_db.rollback();
        return Resultado<PlanDTO>::error("Error al actualizar las materias del plan.");
    }

    QSqlQuery materiaQuery(m_db);
    materiaQuery.prepare(
        "INSERT INTO PlanEstudio_Materia (codigo_PlanEstudio, id_Materia, curso, horas) "
        "VALUES (:codigo, :idMateria, :curso, :horas)");

    for (const PlanMateriaDTO& materia : materias) {
        materiaQuery.bindValue(":codigo", codigoLimpio);
        materiaQuery.bindValue(":idMateria", materia.idMateria);
        materiaQuery.bindValue(":curso", materia.curso);
        materiaQuery.bindValue(":horas", materia.horas);

        if (!materiaQuery.exec()) {
            qCritical() << "Error al asociar materia al plan:"
                        << materiaQuery.lastError().text();
            m_db.rollback();
            return Resultado<PlanDTO>::error(
                "Error al guardar las materias del plan en la base de datos.");
        }
    }

    if (!m_db.commit()) {
        qCritical() << "Error al confirmar el plan:" << m_db.lastError().text();
        m_db.rollback();
        return Resultado<PlanDTO>::error("Error al confirmar la transacción.");
    }

    return obtenerPlan(codigoLimpio);
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