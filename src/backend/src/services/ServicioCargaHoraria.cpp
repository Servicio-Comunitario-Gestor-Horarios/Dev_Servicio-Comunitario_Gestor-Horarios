#include "backend/services/ServicioCargaHoraria.hpp"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>

static constexpr int CURSO_MINIMO = 1;
static constexpr int CURSO_MAXIMO = 6;

ServicioCargaHoraria::ServicioCargaHoraria(QSqlDatabase& db) : m_db(db) {}

bool ServicioCargaHoraria::validarCarga(const QString& codigoPlan, int idMateria,
                                        int curso, int horas, QString& error) const {
    if (codigoPlan.trimmed().isEmpty()) {
        error = "El código del plan no puede estar vacío.";
        return false;
    }

    if (idMateria <= 0) {
        error = "ID de materia inválido.";
        return false;
    }

    if (curso < CURSO_MINIMO || curso > CURSO_MAXIMO) {
        error = QString("El curso debe estar entre %1 y %2.").arg(CURSO_MINIMO).arg(CURSO_MAXIMO);
        return false;
    }

    if (horas <= 0) {
        error = "Las horas semanales deben ser mayores a 0.";
        return false;
    }

    // Verificar que el plan existe
    QSqlQuery checkPlan(m_db);
    checkPlan.prepare("SELECT 1 FROM PlanEstudio WHERE codigo = :codigo");
    checkPlan.bindValue(":codigo", codigoPlan.trimmed());
    if (!checkPlan.exec() || !checkPlan.next()) {
        error = "El plan de estudio especificado no existe.";
        return false;
    }

    // Verificar que la materia existe
    QSqlQuery checkMateria(m_db);
    checkMateria.prepare("SELECT 1 FROM Materias WHERE id = :id");
    checkMateria.bindValue(":id", idMateria);
    if (!checkMateria.exec() || !checkMateria.next()) {
        error = "La materia especificada no existe.";
        return false;
    }

    return true;
}

CargaHorariaDTO ServicioCargaHoraria::mapearARecord(const QSqlRecord& record) const {
    CargaHorariaDTO dto;
    dto.id = record.value("id_Materia").toInt();
    dto.codigoPlanEstudio = record.value("codigo_PlanEstudio").toString();
    dto.idMateria = record.value("id_Materia").toInt();
    dto.curso = record.value("curso").toInt();
    dto.horas = record.value("horas").toInt();

    if (record.contains("nombre")) {
        dto.nombreMateria = record.value("nombre").toString();
    }

    return dto;
}

// ─── CRUD ──────────────────────────────────────────────────────────────────

Resultado<CargaHorariaDTO> ServicioCargaHoraria::asignarCarga(const QString& codigoPlan,
                                                              int idMateria, int curso,
                                                              int horas) {
    QString error;
    if (!validarCarga(codigoPlan, idMateria, curso, horas, error)) {
        return Resultado<CargaHorariaDTO>::error(error);
    }

    // Verificar duplicado (misma materia, mismo curso, mismo plan)
    QSqlQuery checkDuplicado(m_db);
    checkDuplicado.prepare(
        "SELECT 1 FROM PlanEstudio_Materia "
        "WHERE codigo_PlanEstudio = :codigoPlan AND id_Materia = :idMateria AND curso = :curso"
    );
    checkDuplicado.bindValue(":codigoPlan", codigoPlan.trimmed());
    checkDuplicado.bindValue(":idMateria", idMateria);
    checkDuplicado.bindValue(":curso", curso);

    if (checkDuplicado.exec() && checkDuplicado.next()) {
        return Resultado<CargaHorariaDTO>::error(
            "La materia ya está asignada a este curso en el plan.", -2
        );
    }

    // Verificar límite de horas por año
    if (excedeLimiteHoras(codigoPlan, curso, horas)) {
        return Resultado<CargaHorariaDTO>::error(
            QString("Las horas semanales totales para el curso %1 excederían el límite de %2.")
            .arg(curso).arg(MAX_HORAS_SEMANALES_POR_ANIO)
        );
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO PlanEstudio_Materia (codigo_PlanEstudio, id_Materia, curso, horas) "
        "VALUES (:codigoPlan, :idMateria, :curso, :horas)"
    );
    query.bindValue(":codigoPlan", codigoPlan.trimmed());
    query.bindValue(":idMateria", idMateria);
    query.bindValue(":curso", curso);
    query.bindValue(":horas", horas);

    if (!query.exec()) {
        qCritical() << "Error al asignar carga:" << query.lastError().text();
        return Resultado<CargaHorariaDTO>::error("Error al guardar la carga en la base de datos.");
    }

    return obtenerCarga(idMateria, codigoPlan);
}

Resultado<CargaHorariaDTO> ServicioCargaHoraria::obtenerCarga(int idMateria, const QString& codigoPlan) const {
    if (idMateria <= 0) {
        return Resultado<CargaHorariaDTO>::error("ID de materia inválido.");
    }

    if (codigoPlan.trimmed().isEmpty()) {
        return Resultado<CargaHorariaDTO>::error("Código de plan inválido.");
    }

    QSqlQuery query(m_db);
    query.prepare(
        "SELECT pm.codigo_PlanEstudio, pm.id_Materia, pm.curso, pm.horas, m.nombre "
        "FROM PlanEstudio_Materia pm "
        "JOIN Materias m ON pm.id_Materia = m.id "
        "WHERE pm.id_Materia = :idMateria AND pm.codigo_PlanEstudio = :codigoPlan"
    );
    query.bindValue(":idMateria", idMateria);
    query.bindValue(":codigoPlan", codigoPlan.trimmed());

    if (!query.exec()) {
        qCritical() << "Error al obtener carga:" << query.lastError().text();
        return Resultado<CargaHorariaDTO>::error("Error al consultar la base de datos.");
    }

    if (!query.next()) {
        return Resultado<CargaHorariaDTO>::error(
            "No se encontró la materia en el plan especificado.", -3
        );
    }

    return Resultado<CargaHorariaDTO>::exito(mapearARecord(query.record()));
}

QVector<CargaHorariaDTO> ServicioCargaHoraria::listarCargaPorPlan(const QString& codigoPlan) const {
    QVector<CargaHorariaDTO> resultados;

    if (codigoPlan.trimmed().isEmpty()) {
        return resultados;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "SELECT pm.codigo_PlanEstudio, pm.id_Materia, pm.curso, pm.horas, m.nombre "
        "FROM PlanEstudio_Materia pm "
        "JOIN Materias m ON pm.id_Materia = m.id "
        "WHERE pm.codigo_PlanEstudio = :codigoPlan "
        "ORDER BY pm.curso, m.nombre"
    );
    query.bindValue(":codigoPlan", codigoPlan.trimmed());

    if (!query.exec()) {
        qCritical() << "Error al listar carga:" << query.lastError().text();
        return resultados;
    }

    while (query.next()) {
        resultados.append(mapearARecord(query.record()));
    }

    return resultados;
}

Resultado<CargaHorariaDTO> ServicioCargaHoraria::actualizarCarga(int idMateria,
                                                                 const QString& codigoPlan,
                                                                 int horas) {
    if (idMateria <= 0) {
        return Resultado<CargaHorariaDTO>::error("ID de materia inválido.");
    }

    if (codigoPlan.trimmed().isEmpty()) {
        return Resultado<CargaHorariaDTO>::error("Código de plan inválido.");
    }

    if (horas <= 0) {
        return Resultado<CargaHorariaDTO>::error("Las horas deben ser mayores a 0.");
    }

    // Obtener el registro actual para verificar límites
    auto cargaActual = obtenerCarga(idMateria, codigoPlan);
    if (!cargaActual.ok) {
        return Resultado<CargaHorariaDTO>::error(cargaActual.mensajeError, cargaActual.codigoError);
    }

    const auto& actual = cargaActual.valor;

    // Verificar límite de horas por año (restando las horas actuales)
    int diferenciaHoras = horas - actual.horas;
    if (excedeLimiteHoras(codigoPlan, actual.curso, diferenciaHoras)) {
        return Resultado<CargaHorariaDTO>::error(
            QString("Las horas semanales totales para el curso %1 excederían el límite de %2.")
            .arg(actual.curso).arg(MAX_HORAS_SEMANALES_POR_ANIO)
        );
    }

    QSqlQuery query(m_db);
    query.prepare(
        "UPDATE PlanEstudio_Materia SET "
        "horas = :horas, "
        "fecha_modificacion = CURRENT_TIMESTAMP "
        "WHERE codigo_PlanEstudio = :codigoPlan AND id_Materia = :idMateria"
    );
    query.bindValue(":horas", horas);
    query.bindValue(":codigoPlan", codigoPlan.trimmed());
    query.bindValue(":idMateria", idMateria);

    if (!query.exec()) {
        qCritical() << "Error al actualizar carga:" << query.lastError().text();
        return Resultado<CargaHorariaDTO>::error("Error al actualizar la carga en la base de datos.");
    }

    if (query.numRowsAffected() == 0) {
        return Resultado<CargaHorariaDTO>::error(
            "No se encontró la materia en el plan especificado.", -3
        );
    }

    return obtenerCarga(idMateria, codigoPlan);
}

bool ServicioCargaHoraria::eliminarCarga(int idMateria, const QString& codigoPlan) {
    if (idMateria <= 0 || codigoPlan.trimmed().isEmpty()) {
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "DELETE FROM PlanEstudio_Materia "
        "WHERE codigo_PlanEstudio = :codigoPlan AND id_Materia = :idMateria"
    );
    query.bindValue(":codigoPlan", codigoPlan.trimmed());
    query.bindValue(":idMateria", idMateria);

    if (!query.exec()) {
        qCritical() << "Error al eliminar carga:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}

// ─── Consultas para el solver ────────────────────────────────────────────

int ServicioCargaHoraria::totalHorasPorAnio(const QString& codigoPlan, int curso) const {
    if (codigoPlan.trimmed().isEmpty() || curso < CURSO_MINIMO || curso > CURSO_MAXIMO) {
        return 0;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "SELECT SUM(horas) as total "
        "FROM PlanEstudio_Materia "
        "WHERE codigo_PlanEstudio = :codigoPlan AND curso = :curso"
    );
    query.bindValue(":codigoPlan", codigoPlan.trimmed());
    query.bindValue(":curso", curso);

    if (!query.exec() || !query.next()) {
        return 0;
    }

    return query.value("total").toInt();
}

bool ServicioCargaHoraria::excedeLimiteHoras(const QString& codigoPlan, int curso,
                                             int horasAdicionales) const {
    int totalActual = totalHorasPorAnio(codigoPlan, curso);
    return (totalActual + horasAdicionales) > MAX_HORAS_SEMANALES_POR_ANIO;
}