#include "backend/services/ServicioCursos.hpp"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

ServicioCursos::ServicioCursos(QSqlDatabase& db) : m_db(db) {}

bool ServicioCursos::validarCurso(const QString& nombre, int numEstudiantes,
                                  QString& error) const {
    if (nombre.trimmed().isEmpty()) {
        error = "El nombre del curso no puede estar vacío.";
        return false;
    }

    if (numEstudiantes < 0) {
        error = "El número de estudiantes no puede ser negativo.";
        return false;
    }

    return true;
}

CursoDTO ServicioCursos::mapearARecord(const QSqlRecord& record) const {
    CursoDTO dto;
    dto.id = record.value("id").toInt();
    dto.nombre = record.value("nombre").toString();
    dto.turno = record.value("turno").toString();

    const QVariant aula = record.value("aula_fija");
    dto.aulaFija = aula.isNull() ? -1 : aula.toInt();

    dto.numEstudiantes = record.value("num_estudiantes").toInt();
    dto.codigoPlan = record.value("codigo_plan").toString();
    return dto;
}

bool ServicioCursos::cargarMaterias(CursoDTO& dto) const {
    QSqlQuery q(m_db);
    q.prepare(
        "SELECT m.id AS id, m.nombre AS nombre, cm.horas_semanales AS horas "
        "FROM Curso_Materia cm "
        "JOIN Materias m ON m.id = cm.id_Materia "
        "WHERE cm.id_Curso = :id "
        "ORDER BY m.nombre"
    );
    q.bindValue(":id", dto.id);

    if (!q.exec()) {
        qCritical() << "Error al cargar materias del curso:" << q.lastError().text();
        return false;
    }

    while (q.next()) {
        CursoMateriaDTO m;
        m.idMateria = q.value("id").toInt();
        m.nombreMateria = q.value("nombre").toString();
        m.horasSemanales = q.value("horas").toInt();
        dto.materias.append(m);
    }

    return true;
}

// ─── CRUD ──────────────────────────────────────────────────────────────────

Resultado<CursoDTO> ServicioCursos::crearCurso(const QString& nombre,
                                               const QString& turno,
                                               int aulaFija,
                                               int numEstudiantes,
                                               const QString& codigoPlan) {
    QString error;
    if (!validarCurso(nombre, numEstudiantes, error)) {
        return Resultado<CursoDTO>::error(error);
    }

    // Pre-chequeo de duplicado (nombre es UNIQUE en el esquema).
    {
        QSqlQuery check(m_db);
        check.prepare("SELECT 1 FROM Cursos WHERE nombre = :nombre LIMIT 1");
        check.bindValue(":nombre", nombre.trimmed());
        if (check.exec() && check.next()) {
            return Resultado<CursoDTO>::error("Ya existe un curso con ese nombre.", -2);
        }
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO Cursos (nombre, turno, aula_fija, num_estudiantes, codigo_plan) "
        "VALUES (:nombre, :turno, :aula_fija, :num_estudiantes, :codigo_plan)"
    );
    query.bindValue(":nombre", nombre.trimmed());
    query.bindValue(":turno", turno.trimmed());
    query.bindValue(":aula_fija", aulaFija > 0 ? QVariant(aulaFija) : QVariant());
    query.bindValue(":num_estudiantes", numEstudiantes);
    query.bindValue(":codigo_plan",
                    codigoPlan.trimmed().isEmpty() ? QVariant() : codigoPlan.trimmed());

    if (!query.exec()) {
        const QString err = query.lastError().text();
        if (err.contains("UNIQUE")) {
            return Resultado<CursoDTO>::error("Ya existe un curso con ese nombre.", -2);
        }
        qCritical() << "Error al crear curso:" << err;
        return Resultado<CursoDTO>::error("Error al guardar el curso en la base de datos.");
    }

    return obtenerCurso(query.lastInsertId().toInt());
}

Resultado<CursoDTO> ServicioCursos::obtenerCurso(int id) const {
    if (id <= 0) {
        return Resultado<CursoDTO>::error("ID de curso inválido.");
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT id, nombre, turno, aula_fija, num_estudiantes, codigo_plan "
                  "FROM Cursos WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qCritical() << "Error al obtener curso:" << query.lastError().text();
        return Resultado<CursoDTO>::error("Error al consultar la base de datos.");
    }

    if (!query.next()) {
        return Resultado<CursoDTO>::error("No se encontró un curso con ese ID.", -3);
    }

    CursoDTO dto = mapearARecord(query.record());
    if (!cargarMaterias(dto)) {
        return Resultado<CursoDTO>::error(
            "Error al cargar las materias del curso.");
    }

    return Resultado<CursoDTO>::exito(dto);
}

QVector<CursoDTO> ServicioCursos::listarCursos() const {
    QVector<CursoDTO> resultados;

    QSqlQuery query(m_db);
    query.prepare("SELECT id, nombre, turno, aula_fija, num_estudiantes, codigo_plan "
                  "FROM Cursos ORDER BY nombre");

    if (!query.exec()) {
        qCritical() << "Error al listar cursos:" << query.lastError().text();
        return resultados;
    }

    while (query.next()) {
        CursoDTO dto = mapearARecord(query.record());
        cargarMaterias(dto);
        resultados.append(dto);
    }

    return resultados;
}

Resultado<CursoDTO> ServicioCursos::actualizarCurso(int id, const QString& nombre,
                                                    const QString& turno, int aulaFija,
                                                    int numEstudiantes,
                                                    const QString& codigoPlan) {
    QString error;
    if (!validarCurso(nombre, numEstudiantes, error)) {
        return Resultado<CursoDTO>::error(error);
    }

    QSqlQuery query(m_db);
    query.prepare(
        "UPDATE Cursos SET "
        "nombre = :nombre, "
        "turno = :turno, "
        "aula_fija = :aula_fija, "
        "num_estudiantes = :num_estudiantes, "
        "codigo_plan = :codigo_plan, "
        "fecha_modificacion = CURRENT_TIMESTAMP "
        "WHERE id = :id"
    );
    query.bindValue(":nombre", nombre.trimmed());
    query.bindValue(":turno", turno.trimmed());
    query.bindValue(":aula_fija", aulaFija > 0 ? QVariant(aulaFija) : QVariant());
    query.bindValue(":num_estudiantes", numEstudiantes);
    query.bindValue(":codigo_plan",
                    codigoPlan.trimmed().isEmpty() ? QVariant() : codigoPlan.trimmed());
    query.bindValue(":id", id);

    if (!query.exec()) {
        const QString err = query.lastError().text();
        if (err.contains("UNIQUE")) {
            return Resultado<CursoDTO>::error("Ya existe un curso con ese nombre.", -2);
        }
        qCritical() << "Error al actualizar curso:" << err;
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

    if (!m_db.transaction()) {
        qCritical() << "No se pudo iniciar la transacción para eliminar el curso.";
        return false;
    }

    // La relación con materias no tiene ON DELETE CASCADE: se borra primero.
    QSqlQuery borrarMaterias(m_db);
    borrarMaterias.prepare("DELETE FROM Curso_Materia WHERE id_Curso = :id");
    borrarMaterias.bindValue(":id", id);

    if (!borrarMaterias.exec()) {
        qCritical() << "Error al eliminar materias del curso:"
                    << borrarMaterias.lastError().text();
        m_db.rollback();
        return false;
    }

    QSqlQuery del(m_db);
    del.prepare("DELETE FROM Cursos WHERE id = :id");
    del.bindValue(":id", id);

    if (!del.exec()) {
        qCritical() << "Error al eliminar curso:" << del.lastError().text();
        m_db.rollback();
        return false;
    }

    const bool borrado = del.numRowsAffected() > 0;
    if (!borrado) {
        m_db.rollback();
        return false;
    }

    m_db.commit();
    return true;
}

// ─── Materias del curso (N:M) ─────────────────────────────────────────────

Resultado<CursoMateriaDTO> ServicioCursos::asignarMateria(int idCurso, int idMateria,
                                                          int horasSemanales) {
    if (idCurso <= 0 || idMateria <= 0) {
        return Resultado<CursoMateriaDTO>::error("Curso o materia inválidos.");
    }

    if (horasSemanales < 0) {
        return Resultado<CursoMateriaDTO>::error(
            "Las horas semanales no pueden ser negativas.");
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT OR REPLACE INTO Curso_Materia (id_Curso, id_Materia, horas_semanales) "
        "VALUES (:id_Curso, :id_Materia, :horas)"
    );
    query.bindValue(":id_Curso", idCurso);
    query.bindValue(":id_Materia", idMateria);
    query.bindValue(":horas", horasSemanales);

    if (!query.exec()) {
        qCritical() << "Error al asignar materia al curso:" << query.lastError().text();
        return Resultado<CursoMateriaDTO>::error(
            "Error al guardar la materia del curso.");
    }

    CursoMateriaDTO dto;
    dto.idMateria = idMateria;
    dto.horasSemanales = horasSemanales;

    QSqlQuery nombre(m_db);
    nombre.prepare("SELECT nombre FROM Materias WHERE id = :id");
    nombre.bindValue(":id", idMateria);
    if (nombre.exec() && nombre.next()) {
        dto.nombreMateria = nombre.value("nombre").toString();
    }

    return Resultado<CursoMateriaDTO>::exito(dto);
}

bool ServicioCursos::quitarMateria(int idCurso, int idMateria) {
    if (idCurso <= 0 || idMateria <= 0) return false;

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM Curso_Materia "
                  "WHERE id_Curso = :id_Curso AND id_Materia = :id_Materia");
    query.bindValue(":id_Curso", idCurso);
    query.bindValue(":id_Materia", idMateria);

    if (!query.exec()) {
        qCritical() << "Error al quitar materia del curso:" << query.lastError().text();
        return false;
    }

    return query.numRowsAffected() > 0;
}
