#include "backend/services/ServicioAula.hpp"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>

// ─── Constantes ─────────────────────────────────────────────────────────────
static constexpr int CAPACIDAD_MINIMA = 1;
static constexpr int CAPACIDAD_MAXIMA = 500;

// ─── AulaDTO ────────────────────────────────────────────────────────────────

Aula AulaDTO::toAula() const {
    Aula aula;
    aula.nombre = nombre;
    aula.capacidad = capacidad;
    aula.locacion = edificio + (piso.isEmpty() ? "" : " - Piso " + piso);
    return aula;
}

// ─── ServicioAula ──────────────────────────────────────────────────────────

ServicioAula::ServicioAula(QSqlDatabase& db) : m_db(db) {}

bool ServicioAula::validarAula(const QString& nombre, int capacidad, QString& error) const {
    if (nombre.trimmed().isEmpty()) {
        error = "El nombre del aula no puede estar vacío.";
        return false;
    }

    if (capacidad < CAPACIDAD_MINIMA) {
        error = QString("La capacidad debe ser al menos %1.").arg(CAPACIDAD_MINIMA);
        return false;
    }

    if (capacidad > CAPACIDAD_MAXIMA) {
        error = QString("La capacidad no puede exceder %1.").arg(CAPACIDAD_MAXIMA);
        return false;
    }

    return true;
}

AulaDTO ServicioAula::mapearARecord(const QSqlRecord& record) const {
    AulaDTO dto;
    dto.id = record.value("id").toInt();
    dto.nombre = record.value("nombre").toString();
    dto.capacidad = record.value("capacidad").toInt();
    dto.edificio = record.value("edificio").toString();
    dto.piso = record.value("piso").toString();
    return dto;
}

// ─── CRUD ──────────────────────────────────────────────────────────────────

Resultado<AulaDTO> ServicioAula::crearAula(const QString& nombre, int capacidad,
                                           const QString& edificio, const QString& piso) {
    QString error;
    if (!validarAula(nombre, capacidad, error)) {
        return Resultado<AulaDTO>::error(error);
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO Aulas (nombre, capacidad, edificio, piso) "
        "VALUES (:nombre, :capacidad, :edificio, :piso)"
    );
    query.bindValue(":nombre", nombre.trimmed());
    query.bindValue(":capacidad", capacidad);
    query.bindValue(":edificio", edificio.trimmed());
    query.bindValue(":piso", piso.trimmed());

    if (!query.exec()) {
        if (query.lastError().text().contains("UNIQUE")) {
            return Resultado<AulaDTO>::error("Ya existe un aula con ese nombre.", -2);
        }
        qCritical() << "Error al crear aula:" << query.lastError().text();
        return Resultado<AulaDTO>::error("Error al guardar el aula en la base de datos.");
    }

    int nuevoId = query.lastInsertId().toInt();
    return obtenerAula(nuevoId);
}

Resultado<AulaDTO> ServicioAula::obtenerAula(int id) const {
    if (id <= 0) {
        return Resultado<AulaDTO>::error("ID de aula inválido.");
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT id, nombre, capacidad, edificio, piso FROM Aulas WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qCritical() << "Error al obtener aula:" << query.lastError().text();
        return Resultado<AulaDTO>::error("Error al consultar la base de datos.");
    }

    if (!query.next()) {
        return Resultado<AulaDTO>::error("No se encontró un aula con ese ID.", -3);
    }

    return Resultado<AulaDTO>::exito(mapearARecord(query.record()));
}

QVector<AulaDTO> ServicioAula::listarAulas() const {
    QVector<AulaDTO> resultados;

    QSqlQuery query(m_db);
    query.prepare("SELECT id, nombre, capacidad, edificio, piso FROM Aulas ORDER BY nombre");

    if (!query.exec()) {
        qCritical() << "Error al listar aulas:" << query.lastError().text();
        return resultados;
    }

    while (query.next()) {
        resultados.append(mapearARecord(query.record()));
    }

    return resultados;
}

Resultado<AulaDTO> ServicioAula::actualizarAula(int id, const QString& nombre, int capacidad,
                                                const QString& edificio, const QString& piso) {
    QString error;
    if (!validarAula(nombre, capacidad, error)) {
        return Resultado<AulaDTO>::error(error);
    }

    QSqlQuery query(m_db);
    query.prepare(
        "UPDATE Aulas SET "
        "nombre = :nombre, "
        "capacidad = :capacidad, "
        "edificio = :edificio, "
        "piso = :piso, "
        "fecha_modificacion = CURRENT_TIMESTAMP "
        "WHERE id = :id"
    );
    query.bindValue(":nombre", nombre.trimmed());
    query.bindValue(":capacidad", capacidad);
    query.bindValue(":edificio", edificio.trimmed());
    query.bindValue(":piso", piso.trimmed());
    query.bindValue(":id", id);

    if (!query.exec()) {
        if (query.lastError().text().contains("UNIQUE")) {
            return Resultado<AulaDTO>::error("Ya existe un aula con ese nombre.", -2);
        }
        qCritical() << "Error al actualizar aula:" << query.lastError().text();
        return Resultado<AulaDTO>::error("Error al actualizar el aula en la base de datos.");
    }

    if (query.numRowsAffected() == 0) {
        return Resultado<AulaDTO>::error("No se encontró un aula con ese ID.", -3);
    }

    return obtenerAula(id);
}

bool ServicioAula::eliminarAula(int id) {
    if (id <= 0) return false;

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM Aulas WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qCritical() << "Error al eliminar aula:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

// ─── Extracción para solver ──────────────────────────────────────────────

QVector<Aula> ServicioAula::obtenerTodasParaSolver() const {
    QVector<Aula> aulas;
    auto dtoList = listarAulas();

    for (const auto& dto : dtoList) {
        aulas.append(dto.toAula());
    }

    return aulas;
}

Resultado<Aula> ServicioAula::obtenerAulaParaSolver(int id) const {
    auto resultado = obtenerAula(id);

    if (!resultado.ok) {
        return Resultado<Aula>::error(resultado.mensajeError, resultado.codigoError);
    }

    return Resultado<Aula>::exito(resultado.valor.toAula());
}