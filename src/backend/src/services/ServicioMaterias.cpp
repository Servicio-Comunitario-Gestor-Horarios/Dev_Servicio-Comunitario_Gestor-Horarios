#include "backend/services/ServicioMaterias.hpp"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>

// ─── MateriaDTO ────────────────────────────────────────────────────────────

QVector<QString> MateriaDTO::requisitosComoVector() const {
    QVector<QString> resultado;

    if (requisitos.trimmed().isEmpty()) {
        return resultado;
    }

    for (const QString& req : requisitos.split(',')) {
        QString trimmed = req.trimmed();
        if (!trimmed.isEmpty()) {
            resultado.append(trimmed);
        }
    }

    return resultado;
}

Materia MateriaDTO::toMateria(int horasSemanales) const {
    Materia materia;
    materia.nombre = nombre;
    materia.horas_semanales = horasSemanales;
    materia.requerimientos = requisitosComoVector();
    return materia;
}

// ─── ServicioMaterias ─────────────────────────────────────────────────────

ServicioMaterias::ServicioMaterias(QSqlDatabase& db) : m_db(db) {}

bool ServicioMaterias::validarMateria(const QString& nombre, QString& error) const {
    if (nombre.trimmed().isEmpty()) {
        error = "El nombre de la materia no puede estar vacío.";
        return false;
    }
    return true;
}

MateriaDTO ServicioMaterias::mapearARecord(const QSqlRecord& record) const {
    MateriaDTO dto;
    dto.id = record.value("id").toInt();
    dto.nombre = record.value("nombre").toString();
    dto.requisitos = record.value("requisitos").toString();
    return dto;
}

// ─── CRUD ──────────────────────────────────────────────────────────────────

Resultado<MateriaDTO> ServicioMaterias::crearMateria(const QString& nombre, const QString& requisitos) {
    QString error;
    if (!validarMateria(nombre, error)) {
        return Resultado<MateriaDTO>::error(error);
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO Materias (nombre, requisitos) "
        "VALUES (:nombre, :requisitos)"
    );
    query.bindValue(":nombre", nombre.trimmed());
    query.bindValue(":requisitos", requisitos.trimmed());

    if (!query.exec()) {
        if (query.lastError().text().contains("UNIQUE")) {
            return Resultado<MateriaDTO>::error("Ya existe una materia con ese nombre.", -2);
        }
        qCritical() << "Error al crear materia:" << query.lastError().text();
        return Resultado<MateriaDTO>::error("Error al guardar la materia en la base de datos.");
    }

    int nuevoId = query.lastInsertId().toInt();
    return obtenerMateria(nuevoId);
}

Resultado<MateriaDTO> ServicioMaterias::obtenerMateria(int id) const {
    if (id <= 0) {
        return Resultado<MateriaDTO>::error("ID de materia inválido.");
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT id, nombre, requisitos FROM Materias WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qCritical() << "Error al obtener materia:" << query.lastError().text();
        return Resultado<MateriaDTO>::error("Error al consultar la base de datos.");
    }

    if (!query.next()) {
        return Resultado<MateriaDTO>::error("No se encontró una materia con ese ID.", -3);
    }

    return Resultado<MateriaDTO>::exito(mapearARecord(query.record()));
}

QVector<MateriaDTO> ServicioMaterias::listarMaterias() const {
    QVector<MateriaDTO> resultados;

    QSqlQuery query(m_db);
    query.prepare("SELECT id, nombre, requisitos FROM Materias ORDER BY nombre");

    if (!query.exec()) {
        qCritical() << "Error al listar materias:" << query.lastError().text();
        return resultados;
    }

    while (query.next()) {
        resultados.append(mapearARecord(query.record()));
    }

    return resultados;
}

Resultado<MateriaDTO> ServicioMaterias::actualizarMateria(int id, const QString& nombre, const QString& requisitos) {
    QString error;
    if (!validarMateria(nombre, error)) {
        return Resultado<MateriaDTO>::error(error);
    }

    QSqlQuery query(m_db);
    query.prepare(
        "UPDATE Materias SET "
        "nombre = :nombre, "
        "requisitos = :requisitos, "
        "fecha_modificacion = CURRENT_TIMESTAMP "
        "WHERE id = :id"
    );
    query.bindValue(":nombre", nombre.trimmed());
    query.bindValue(":requisitos", requisitos.trimmed());
    query.bindValue(":id", id);

    if (!query.exec()) {
        if (query.lastError().text().contains("UNIQUE")) {
            return Resultado<MateriaDTO>::error("Ya existe una materia con ese nombre.", -2);
        }
        qCritical() << "Error al actualizar materia:" << query.lastError().text();
        return Resultado<MateriaDTO>::error("Error al actualizar la materia en la base de datos.");
    }

    if (query.numRowsAffected() == 0) {
        return Resultado<MateriaDTO>::error("No se encontró una materia con ese ID.", -3);
    }

    return obtenerMateria(id);
}

bool ServicioMaterias::eliminarMateria(int id) {
    if (id <= 0) return false;

    // Verificar si está asociada a algún plan
    QSqlQuery checkQuery(m_db);
    checkQuery.prepare("SELECT 1 FROM PlanEstudio_Materia WHERE id_Materia = :id");
    checkQuery.bindValue(":id", id);

    if (checkQuery.exec() && checkQuery.next()) {
        qWarning() << "No se puede eliminar la materia porque está asociada a un plan de estudio.";
        return false;
    }

    // Verificar si está asociada a algún profesor
    QSqlQuery checkProfQuery(m_db);
    checkProfQuery.prepare("SELECT 1 FROM Profesor_Materia WHERE id_Materia = :id");
    checkProfQuery.bindValue(":id", id);

    if (checkProfQuery.exec() && checkProfQuery.next()) {
        qWarning() << "No se puede eliminar la materia porque está asociada a un profesor.";
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM Materias WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qCritical() << "Error al eliminar materia:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

// ─── Extracción para solver ──────────────────────────────────────────────

QVector<Materia> ServicioMaterias::obtenerTodasParaSolver() const {
    QVector<Materia> materias;
    auto dtoList = listarMaterias();

    for (const auto& dto : dtoList) {
        materias.append(dto.toMateria(0));
    }

    return materias;
}

Resultado<Materia> ServicioMaterias::obtenerMateriaParaSolver(int id) const {
    auto resultado = obtenerMateria(id);

    if (!resultado.ok) {
        return Resultado<Materia>::error(resultado.mensajeError, resultado.codigoError);
    }

    return Resultado<Materia>::exito(resultado.valor.toMateria(0));
}

Resultado<Materia> ServicioMaterias::obtenerMateriaParaSolver(int id, int horasSemanales) const {
    auto resultado = obtenerMateria(id);

    if (!resultado.ok) {
        return Resultado<Materia>::error(resultado.mensajeError, resultado.codigoError);
    }

    return Resultado<Materia>::exito(resultado.valor.toMateria(horasSemanales));
}