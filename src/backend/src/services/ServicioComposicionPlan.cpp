#include "backend/services/ServicioComposicionPlan.hpp"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>

ServicioComposicionPlan::ServicioComposicionPlan(QSqlDatabase& db) : m_db(db) {}

bool ServicioComposicionPlan::planExiste(const QString& codigoPlan) const {
    if (codigoPlan.trimmed().isEmpty()) {
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT 1 FROM PlanEstudio WHERE codigo = :codigo");
    query.bindValue(":codigo", codigoPlan.trimmed());

    if (!query.exec()) {
        qCritical() << "Error al verificar existencia del plan:" << query.lastError().text();
        return false;
    }

    return query.next();
}

QVector<QString> ServicioComposicionPlan::obtenerRequisitos(int materiaId) const {
    QVector<QString> requisitos;

    QSqlQuery query(m_db);
    query.prepare("SELECT requisitos FROM Materias WHERE id = :id");
    query.bindValue(":id", materiaId);

    if (!query.exec() || !query.next()) {
        return requisitos;
    }

    QString reqText = query.value("requisitos").toString();
    if (reqText.isEmpty()) {
        return requisitos;
    }

    // Parsear requisitos (separados por coma)
    for (const QString& req : reqText.split(',')) {
        QString trimmed = req.trimmed();
        if (!trimmed.isEmpty()) {
            requisitos.append(trimmed);
        }
    }

    return requisitos;
}

QVector<MateriaEnPlan> ServicioComposicionPlan::obtenerMateriasConCarga(const QString& codigoPlan) const {
    QVector<MateriaEnPlan> resultado;

    if (codigoPlan.trimmed().isEmpty()) {
        qWarning() << "ServicioComposicionPlan: Código de plan vacío.";
        return resultado;
    }

    if (!planExiste(codigoPlan)) {
        qWarning() << "ServicioComposicionPlan: Plan no existe:" << codigoPlan;
        return resultado;
    }

    QSqlQuery query(m_db);
    query.prepare(
        "SELECT pm.id_Materia, pm.curso, pm.horas, m.nombre "
        "FROM PlanEstudio_Materia pm "
        "JOIN Materias m ON pm.id_Materia = m.id "
        "WHERE pm.codigo_PlanEstudio = :codigoPlan "
        "ORDER BY pm.curso, m.nombre"
    );
    query.bindValue(":codigoPlan", codigoPlan.trimmed());

    if (!query.exec()) {
        qCritical() << "Error al obtener materias con carga:" << query.lastError().text();
        return resultado;
    }

    while (query.next()) {
        MateriaEnPlan item;
        item.curso = query.value("curso").toInt();
        item.horas = query.value("horas").toInt();

        // Construir Materia struct del solver
        Materia materia;
        materia.nombre = query.value("nombre").toString();
        materia.horas_semanales = item.horas;
        materia.requerimientos = obtenerRequisitos(query.value("id_Materia").toInt());

        item.materia = materia;
        resultado.append(item);
    }

    return resultado;
}

QVector<QPair<QString, QString>> ServicioComposicionPlan::listarPlanesDisponibles() const {
    QVector<QPair<QString, QString>> planes;

    QSqlQuery query(m_db);
    query.prepare("SELECT codigo, nombre FROM PlanEstudio ORDER BY codigo");

    if (!query.exec()) {
        qCritical() << "Error al listar planes:" << query.lastError().text();
        return planes;
    }

    while (query.next()) {
        planes.append(qMakePair(
            query.value("codigo").toString(),
            query.value("nombre").toString()
        ));
    }

    return planes;
}

Resultado<PlanEstudio> ServicioComposicionPlan::componerPlanCompleto(const QString& codigoPlan) const {
    if (codigoPlan.trimmed().isEmpty()) {
        return Resultado<PlanEstudio>::error("Código de plan inválido.");
    }

    // Obtener datos del plan desde PlanEstudio
    QSqlQuery planQuery(m_db);
    planQuery.prepare("SELECT codigo, nombre, descripcion FROM PlanEstudio WHERE codigo = :codigo");
    planQuery.bindValue(":codigo", codigoPlan.trimmed());

    if (!planQuery.exec()) {
        qCritical() << "Error al consultar plan:" << planQuery.lastError().text();
        return Resultado<PlanEstudio>::error("Error al consultar la base de datos.");
    }

    if (!planQuery.next()) {
        return Resultado<PlanEstudio>::error(
            QString("No se encontró un plan con código '%1'.").arg(codigoPlan), -3
        );
    }

    // Construir PlanEstudio struct del solver
    PlanEstudio plan;
    plan.nombre = planQuery.value("nombre").toString();

    // Obtener materias con carga horaria desde PlanEstudio_Materia
    plan.materias = obtenerMateriasConCarga(codigoPlan);

    if (plan.materias.isEmpty()) {
        return Resultado<PlanEstudio>::error(
            QString("El plan '%1' no tiene materias asignadas.").arg(codigoPlan)
        );
    }

    return Resultado<PlanEstudio>::exito(plan);
}