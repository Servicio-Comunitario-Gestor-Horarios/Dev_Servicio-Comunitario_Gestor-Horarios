#include "backend/services/ServicioAula.hpp"
#include "backend/services/ServicioTiposAula.hpp"
#include "backend/services/ordenacion.hpp"

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>

#include <algorithm>

// ─── Constantes ─────────────────────────────────────────────────────────────
static constexpr int CAPACIDAD_MINIMA = 1;
static constexpr int CAPACIDAD_MAXIMA = 500;

/// Columnas del aula con el nombre del tipo resuelto por JOIN.
static const QString CONSULTA_AULA =
    QStringLiteral("SELECT a.id AS id, a.nombre AS nombre, a.capacidad AS capacidad, "
                   "a.edificio AS edificio, a.piso AS piso, "
                   "a.id_tipo_aula AS id_tipo_aula, t.nombre AS tipo_aula "
                   "FROM Aulas a LEFT JOIN TiposAula t ON t.id = a.id_tipo_aula");

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

bool ServicioAula::validarTipoAula(int idTipoAula, QString& error) const {
    if (idTipoAula <= 0) {
        if (tipos_aula::catalogoVacio(m_db)) {
            error = "No hay tipos de aula registrados. Cree un tipo de aula antes de crear aulas.";
            return false;
        }
        error = "El aula debe tener un tipo de aula.";
        return false;
    }

    if (!tipos_aula::existe(m_db, idTipoAula)) {
        error = "No existe el tipo de aula indicado.";
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
    const QVariant tipo = record.value("id_tipo_aula");
    dto.idTipoAula = tipo.isNull() ? -1 : tipo.toInt();
    dto.tipoAula = record.value("tipo_aula").toString();
    return dto;
}

// ─── CRUD ──────────────────────────────────────────────────────────────────

Resultado<AulaDTO> ServicioAula::crearAula(const QString& nombre, int capacidad,
                                           const QString& edificio, const QString& piso,
                                           int idTipoAula) {
    QString error;
    if (!validarAula(nombre, capacidad, error)) {
        return Resultado<AulaDTO>::error(error);
    }

    if (!validarTipoAula(idTipoAula, error)) {
        return Resultado<AulaDTO>::error(error);
    }

    // Verificar duplicado antes de INSERT (schema no tiene UNIQUE en nombre)
    QSqlQuery checkQuery(m_db);
    checkQuery.prepare("SELECT id FROM Aulas WHERE nombre = :nombre");
    checkQuery.bindValue(":nombre", nombre.trimmed());
    if (checkQuery.exec() && checkQuery.next()) {
        return Resultado<AulaDTO>::error("Ya existe un aula con ese nombre.", -2);
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO Aulas (nombre, capacidad, edificio, piso, id_tipo_aula) "
        "VALUES (:nombre, :capacidad, :edificio, :piso, :id_tipo_aula)"
    );
    query.bindValue(":nombre", nombre.trimmed());
    query.bindValue(":capacidad", capacidad);
    query.bindValue(":edificio", edificio.trimmed());
    query.bindValue(":piso", piso.trimmed());
    query.bindValue(":id_tipo_aula", idTipoAula);

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
    query.prepare(CONSULTA_AULA + QStringLiteral(" WHERE a.id = :id"));
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

Resultado<QVector<AulaDTO>> ServicioAula::listarAulas() const {
    QVector<AulaDTO> resultados;

    QSqlQuery query(m_db);
    query.prepare(CONSULTA_AULA + QStringLiteral(" ORDER BY a.nombre"));

    if (!query.exec()) {
        qCritical() << "Error al listar aulas:" << query.lastError().text();
        return Resultado<QVector<AulaDTO>>::error(
            QStringLiteral("No se pudieron leer las aulas registradas. Compruebe que la base de"
                           " datos esté disponible e intente de nuevo."));
    }

    while (query.next()) {
        resultados.append(mapearARecord(query.record()));
    }

    std::stable_sort(resultados.begin(), resultados.end(),
                     [](const AulaDTO& a, const AulaDTO& b) {
                         return nombreAntes(a.nombre, b.nombre);
                     });

    return Resultado<QVector<AulaDTO>>::exito(resultados);
}

Resultado<AulaDTO> ServicioAula::actualizarAula(int id, const QString& nombre, int capacidad,
                                                const QString& edificio, const QString& piso,
                                                int idTipoAula) {
    QString error;
    if (!validarAula(nombre, capacidad, error)) {
        return Resultado<AulaDTO>::error(error);
    }

    if (!validarTipoAula(idTipoAula, error)) {
        return Resultado<AulaDTO>::error(error);
    }

    QSqlQuery query(m_db);
    query.prepare(
        "UPDATE Aulas SET "
        "nombre = :nombre, "
        "capacidad = :capacidad, "
        "edificio = :edificio, "
        "piso = :piso, "
        "id_tipo_aula = :id_tipo_aula, "
        "fecha_modificacion = CURRENT_TIMESTAMP "
        "WHERE id = :id"
    );
    query.bindValue(":nombre", nombre.trimmed());
    query.bindValue(":capacidad", capacidad);
    query.bindValue(":edificio", edificio.trimmed());
    query.bindValue(":piso", piso.trimmed());
    query.bindValue(":id_tipo_aula", idTipoAula);
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
    const auto dtoResultado = listarAulas();

    if (!dtoResultado.ok) {
        qCritical() << "No se pudieron leer las aulas para el solver:" << dtoResultado.mensajeError;
        return aulas;
    }

    for (const auto& dto : dtoResultado.valor) {
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