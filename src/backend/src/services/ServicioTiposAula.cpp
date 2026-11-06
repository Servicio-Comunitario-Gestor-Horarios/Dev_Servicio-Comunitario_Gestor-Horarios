#include "backend/services/ServicioTiposAula.hpp"
#include "backend/services/ordenacion.hpp"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStringList>

#include <algorithm>

namespace {

/// Une nombres entre comillas angulares: «A», «B» y «C».
QString enumerar(const QVector<QString>& nombres)
{
    QStringList lista;
    lista.reserve(nombres.size());
    for (const QString& nombre : nombres) {
        lista.append(QStringLiteral("«%1»").arg(nombre));
    }

    if (lista.size() <= 1) {
        return lista.value(0);
    }

    return lista.mid(0, lista.size() - 1).join(QStringLiteral(", "))
           + QStringLiteral(" y ") + lista.last();
}

} // namespace

namespace tipos_aula {

bool catalogoVacio(QSqlDatabase& db)
{
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM TiposAula")) || !query.next()) {
        qCritical() << "Error al contar los tipos de aula:" << query.lastError().text();
        // Ante un fallo de lectura se informa «no vacío» para no inventar un
        // mensaje de catálogo vacío; la inserción fallará y se reportará.
        return false;
    }
    return query.value(0).toInt() == 0;
}

bool existe(QSqlDatabase& db, int idTipoAula)
{
    if (idTipoAula <= 0) {
        return false;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT 1 FROM TiposAula WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), idTipoAula);
    if (!query.exec()) {
        qCritical() << "Error al consultar el tipo de aula:" << query.lastError().text();
        return false;
    }
    return query.next();
}

} // namespace tipos_aula

// ─── ServicioTiposAula ─────────────────────────────────────────────────────

ServicioTiposAula::ServicioTiposAula(QSqlDatabase& db) : m_db(db) {}

QString ServicioTiposAula::claveTipo(const QString& nombre)
{
    return nombre.trimmed().toCaseFolded();
}

bool ServicioTiposAula::validarNombreYClave(const QString& nombre, int excluirId,
                                            QString& error) const
{
    const QString limpio = nombre.trimmed();
    const QString clave = claveTipo(nombre);

    if (clave.isEmpty()) {
        error = "El nombre del tipo de aula no puede estar vacío.";
        return false;
    }

    // Comparación en memoria con claveTipo: frena duplicados aunque una fila se
    // haya escrito directo en la tabla con la clave sin normalizar.
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("SELECT id, clave FROM TiposAula"))) {
        qCritical() << "Error al consultar el catálogo de tipos de aula:"
                    << query.lastError().text();
        error = "Error al consultar el catálogo de tipos de aula.";
        return false;
    }

    while (query.next()) {
        if (query.value(QStringLiteral("id")).toInt() == excluirId) {
            continue;
        }
        if (claveTipo(query.value(QStringLiteral("clave")).toString()) == clave) {
            error = QStringLiteral("Ya existe un tipo de aula con ese nombre («%1»).").arg(limpio);
            return false;
        }
    }

    return true;
}

TipoAulaDTO ServicioTiposAula::mapearARecord(const QSqlRecord& record) const
{
    TipoAulaDTO dto;
    dto.id = record.value(QStringLiteral("id")).toInt();
    dto.nombre = record.value(QStringLiteral("nombre")).toString();
    return dto;
}

Resultado<TipoAulaDTO> ServicioTiposAula::crearTipoAula(const QString& nombre)
{
    QString error;
    if (!validarNombreYClave(nombre, -1, error)) {
        return Resultado<TipoAulaDTO>::error(error);
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO TiposAula (nombre, clave) VALUES (:nombre, :clave)"));
    query.bindValue(QStringLiteral(":nombre"), nombre.trimmed());
    query.bindValue(QStringLiteral(":clave"), claveTipo(nombre));

    if (!query.exec()) {
        if (query.lastError().text().contains(QStringLiteral("UNIQUE"))) {
            return Resultado<TipoAulaDTO>::error(
                QStringLiteral("Ya existe un tipo de aula con ese nombre."), -2);
        }
        qCritical() << "Error al crear tipo de aula:" << query.lastError().text();
        return Resultado<TipoAulaDTO>::error(
            "Error al guardar el tipo de aula en la base de datos.");
    }

    return obtenerTipoAula(query.lastInsertId().toInt());
}

Resultado<TipoAulaDTO> ServicioTiposAula::obtenerTipoAula(int id) const
{
    if (id <= 0) {
        return Resultado<TipoAulaDTO>::error("ID de tipo de aula inválido.");
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT id, nombre FROM TiposAula WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        qCritical() << "Error al obtener tipo de aula:" << query.lastError().text();
        return Resultado<TipoAulaDTO>::error("Error al consultar la base de datos.");
    }

    if (!query.next()) {
        return Resultado<TipoAulaDTO>::error(
            "No se encontró un tipo de aula con ese ID.", -3);
    }

    return Resultado<TipoAulaDTO>::exito(mapearARecord(query.record()));
}

Resultado<QVector<TipoAulaDTO>> ServicioTiposAula::listarTiposAula() const
{
    QVector<TipoAulaDTO> resultados;

    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("SELECT id, nombre FROM TiposAula ORDER BY nombre"))) {
        qCritical() << "Error al listar tipos de aula:" << query.lastError().text();
        return Resultado<QVector<TipoAulaDTO>>::error(
            QStringLiteral("No se pudieron leer los tipos de aula registrados. Compruebe que la"
                           " base de datos esté disponible e intente de nuevo."));
    }

    while (query.next()) {
        resultados.append(mapearARecord(query.record()));
    }

    std::stable_sort(resultados.begin(), resultados.end(),
                     [](const TipoAulaDTO& a, const TipoAulaDTO& b) {
                         return nombreAntes(a.nombre, b.nombre);
                     });

    return Resultado<QVector<TipoAulaDTO>>::exito(resultados);
}

Resultado<TipoAulaDTO> ServicioTiposAula::renombrarTipoAula(int id, const QString& nombre)
{
    const auto existente = obtenerTipoAula(id);
    if (!existente.ok) {
        return Resultado<TipoAulaDTO>::error(existente.mensajeError, existente.codigoError);
    }

    QString error;
    if (!validarNombreYClave(nombre, id, error)) {
        return Resultado<TipoAulaDTO>::error(error);
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "UPDATE TiposAula SET nombre = :nombre, clave = :clave, "
        "fecha_modificacion = CURRENT_TIMESTAMP WHERE id = :id"));
    query.bindValue(QStringLiteral(":nombre"), nombre.trimmed());
    query.bindValue(QStringLiteral(":clave"), claveTipo(nombre));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        if (query.lastError().text().contains(QStringLiteral("UNIQUE"))) {
            return Resultado<TipoAulaDTO>::error(
                QStringLiteral("Ya existe un tipo de aula con ese nombre."), -2);
        }
        qCritical() << "Error al renombrar el tipo de aula:" << query.lastError().text();
        return Resultado<TipoAulaDTO>::error(
            "Error al actualizar el tipo de aula en la base de datos.");
    }

    return obtenerTipoAula(id);
}

QVector<QString> ServicioTiposAula::dependientesDe(const QString& tabla, int id) const
{
    QVector<QString> nombres;

    // `tabla` solo llega desde constantes de este archivo (Aulas, Materias).
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT nombre FROM %1 WHERE id_tipo_aula = :id "
                                 "ORDER BY nombre")
                      .arg(tabla));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        qCritical() << "Error al leer los dependientes del tipo de aula:"
                    << query.lastError().text();
        return nombres;
    }

    while (query.next()) {
        nombres.append(query.value(0).toString());
    }

    return nombres;
}

Resultado<bool> ServicioTiposAula::eliminarTipoAula(int id)
{
    const auto existente = obtenerTipoAula(id);
    if (!existente.ok) {
        return Resultado<bool>::error(existente.mensajeError, existente.codigoError);
    }

    const QVector<QString> aulas = dependientesDe(QStringLiteral("Aulas"), id);
    const QVector<QString> materias = dependientesDe(QStringLiteral("Materias"), id);

    if (!aulas.isEmpty() || !materias.isEmpty()) {
        QStringList partes;
        if (!aulas.isEmpty()) {
            partes.append(QStringLiteral("las aulas %1").arg(enumerar(aulas)));
        }
        if (!materias.isEmpty()) {
            partes.append(QStringLiteral("las materias %1").arg(enumerar(materias)));
        }
        return Resultado<bool>::error(
            QStringLiteral("No se puede eliminar el tipo de aula «%1»: está en uso por %2.")
                .arg(existente.valor.nombre, partes.join(QStringLiteral(" y "))));
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM TiposAula WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        qCritical() << "Error al eliminar el tipo de aula:" << query.lastError().text();
        return Resultado<bool>::error(
            "Error al eliminar el tipo de aula de la base de datos.");
    }

    if (query.numRowsAffected() == 0) {
        return Resultado<bool>::error("No se encontró un tipo de aula con ese ID.", -3);
    }

    return Resultado<bool>::exito(true);
}
