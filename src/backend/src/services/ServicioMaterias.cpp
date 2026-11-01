#include "backend/services/ServicioMaterias.hpp"
#include "backend/services/ServicioTiposAula.hpp"
#include "backend/services/ordenacion.hpp"

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>

#include <algorithm>

/// Columnas de la materia con el nombre del tipo resuelto por JOIN.
static const QString CONSULTA_MATERIA =
    QStringLiteral("SELECT m.id AS id, m.nombre AS nombre, m.requisitos AS requisitos, "
                   "m.id_tipo_aula AS id_tipo_aula, t.nombre AS tipo_aula "
                   "FROM Materias m LEFT JOIN TiposAula t ON t.id = m.id_tipo_aula");

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

bool ServicioMaterias::validarTipoAula(int idTipoAula, QString& error) const {
    if (idTipoAula <= 0) {
        if (tipos_aula::catalogoVacio(m_db)) {
            error = "No hay tipos de aula registrados. Cree un tipo de aula antes de crear materias.";
            return false;
        }
        error = "La materia debe tener un tipo de aula (se recibió: sin tipo).";
        return false;
    }

    if (!tipos_aula::existe(m_db, idTipoAula)) {
        error = QStringLiteral("No existe el tipo de aula indicado (id %1).").arg(idTipoAula);
        return false;
    }

    return true;
}

MateriaDTO ServicioMaterias::mapearARecord(const QSqlRecord& record) const {
    MateriaDTO dto;
    dto.id = record.value("id").toInt();
    dto.nombre = record.value("nombre").toString();
    dto.requisitos = record.value("requisitos").toString();
    const QVariant tipo = record.value("id_tipo_aula");
    dto.idTipoAula = tipo.isNull() ? -1 : tipo.toInt();
    dto.tipoAula = record.value("tipo_aula").toString();
    return dto;
}

// ─── CRUD ──────────────────────────────────────────────────────────────────

Resultado<MateriaDTO> ServicioMaterias::crearMateria(const QString& nombre, const QString& requisitos,
                                                     int idTipoAula) {
    QString error;
    if (!validarMateria(nombre, error)) {
        return Resultado<MateriaDTO>::error(error);
    }

    if (!validarTipoAula(idTipoAula, error)) {
        return Resultado<MateriaDTO>::error(error);
    }

    // Verificar duplicado antes de INSERT (schema no tiene UNIQUE en nombre)
    QSqlQuery checkQuery(m_db);
    checkQuery.prepare("SELECT id FROM Materias WHERE nombre = :nombre");
    checkQuery.bindValue(":nombre", nombre.trimmed());
    if (checkQuery.exec() && checkQuery.next()) {
        return Resultado<MateriaDTO>::error("Ya existe una materia con ese nombre.", -2);
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO Materias (nombre, requisitos, id_tipo_aula) "
        "VALUES (:nombre, :requisitos, :id_tipo_aula)"
    );
    query.bindValue(":nombre", nombre.trimmed());
    query.bindValue(":requisitos", requisitos.trimmed());
    query.bindValue(":id_tipo_aula", idTipoAula);

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
    query.prepare(CONSULTA_MATERIA + QStringLiteral(" WHERE m.id = :id"));
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

Resultado<QVector<MateriaDTO>> ServicioMaterias::listarMaterias() const {
    QVector<MateriaDTO> resultados;

    QSqlQuery query(m_db);
    query.prepare(CONSULTA_MATERIA + QStringLiteral(" ORDER BY m.nombre"));

    if (!query.exec()) {
        qCritical() << "Error al listar materias:" << query.lastError().text();
        return Resultado<QVector<MateriaDTO>>::error(
            QStringLiteral("No se pudieron leer las materias registradas. Compruebe que la base de"
                           " datos esté disponible e intente de nuevo."));
    }

    while (query.next()) {
        resultados.append(mapearARecord(query.record()));
    }

    std::stable_sort(resultados.begin(), resultados.end(),
                     [](const MateriaDTO& a, const MateriaDTO& b) {
                         return nombreAntes(a.nombre, b.nombre);
                     });

    return Resultado<QVector<MateriaDTO>>::exito(resultados);
}

Resultado<MateriaDTO> ServicioMaterias::actualizarMateria(int id, const QString& nombre,
                                                         const QString& requisitos,
                                                         int idTipoAula) {
    QString error;
    if (!validarMateria(nombre, error)) {
        return Resultado<MateriaDTO>::error(error);
    }

    if (!validarTipoAula(idTipoAula, error)) {
        return Resultado<MateriaDTO>::error(error);
    }

    QSqlQuery query(m_db);
    query.prepare(
        "UPDATE Materias SET "
        "nombre = :nombre, "
        "requisitos = :requisitos, "
        "id_tipo_aula = :id_tipo_aula, "
        "fecha_modificacion = CURRENT_TIMESTAMP "
        "WHERE id = :id"
    );
    query.bindValue(":nombre", nombre.trimmed());
    query.bindValue(":requisitos", requisitos.trimmed());
    query.bindValue(":id_tipo_aula", idTipoAula);
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
    const auto dtoResultado = listarMaterias();

    if (!dtoResultado.ok) {
        qCritical() << "No se pudieron leer las materias para el solver:"
                    << dtoResultado.mensajeError;
        return materias;
    }

    for (const auto& dto : dtoResultado.valor) {
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