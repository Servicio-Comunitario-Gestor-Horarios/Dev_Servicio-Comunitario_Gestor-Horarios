#include "backend/services/ServicioProfesor.hpp"
#include "backend/services/franja_horaria_validacion.hpp"
#include "backend/services/ordenacion.hpp"

#include <algorithm>
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTime>

// ─── Constantes ────────────────────────────────────────────────────────────

static const QString FORMATO_HORA = QStringLiteral("HH:mm");

// ─── ProfesorDTO ───────────────────────────────────────────────────────────

Profesor ProfesorDTO::toProfesor() const {
    Profesor p;
    p.nombre = nombre;

    p.disponibilidad.reserve(disponibilidad.size());
    for (const auto& d : disponibilidad) {
        p.disponibilidad.append(d.franja);
    }

    p.materias.reserve(materias.size());
    for (const auto& m : materias) {
        p.materias.append(m.nombre);
    }

    return p;
}

// ─── ServicioProfesor ──────────────────────────────────────────────────────

ServicioProfesor::ServicioProfesor(QSqlDatabase& db) : m_db(db)
{
    // Tabla auxiliar opcional para las materias suplentes (#95). No forma parte
    // de `Migracion` para no cambiar la huella del esquema versionado; se crea
    // bajo demanda y convive con bases ya existentes.
    asegurarTablaSuplentes();
}

bool ServicioProfesor::asegurarTablaSuplentes() const
{
    QSqlQuery q(m_db);
    if (q.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS Profesor_Materia_Suplente ("
            "id_Profesor TEXT NOT NULL REFERENCES Profesores(id),"
            "id_Materia INTEGER NOT NULL REFERENCES Materias(id),"
            "PRIMARY KEY (id_Profesor, id_Materia)"
            ")"))) {
        return true;
    }
    // Si el esquema mínimo aún no existe (por ejemplo, una prueba que construye
    // el servicio sobre una base vacía), no es un error fatal: las consultas
    // posteriores devolverán listas vacías.
    qWarning() << "No se pudo asegurar la tabla de materias suplentes:"
               << q.lastError().text();
    return false;
}

bool ServicioProfesor::validarProfesor(const QString& id,
                                       const QString& nombre,
                                       const QString& email,
                                       QString& error) const {
    if (id.trimmed().isEmpty()) {
        error = "El ID del profesor no puede estar vacío.";
        return false;
    }

    if (nombre.trimmed().isEmpty()) {
        error = "El nombre del profesor no puede estar vacío.";
        return false;
    }

    const QString emailLimpio = email.trimmed();
    if (emailLimpio.isEmpty()) {
        error = "El email no puede estar vacío.";
        return false;
    }
    if (!emailLimpio.contains('@') || !emailLimpio.contains('.')) {
        error = "El email no tiene un formato válido.";
        return false;
    }

    return true;
}

ProfesorDTO ServicioProfesor::mapearARecord(const QSqlRecord& record) const {
    ProfesorDTO dto;
    dto.id       = record.value("id").toString();
    dto.nombre   = record.value("nombre").toString();
    dto.email    = record.value("email").toString();
    dto.telefono = record.value("telefono").toString();
    return dto;
}

bool ServicioProfesor::cargarDisponibilidad(ProfesorDTO& dto) const {
    QSqlQuery q(m_db);
    q.prepare(
        "SELECT id, dia, hora_inicio, hora_fin "
        "FROM Disponibilidad_Profesor "
        "WHERE id_Profesor = :id "
        "ORDER BY dia, hora_inicio"
    );
    q.bindValue(":id", dto.id);

    if (!q.exec()) {
        qCritical() << "Error al cargar disponibilidad:" << q.lastError().text();
        return false;
    }

    while (q.next()) {
        DisponibilidadDTO d;
        d.id            = q.value("id").toInt();
        d.franja.dia    = q.value("dia").toInt();
        d.franja.inicio = QTime::fromString(q.value("hora_inicio").toString(), FORMATO_HORA);
        d.franja.fin    = QTime::fromString(q.value("hora_fin").toString(),    FORMATO_HORA);
        dto.disponibilidad.append(d);
    }
    return true;
}

bool ServicioProfesor::cargarPreferencias(ProfesorDTO& dto) const {
    // Una sola consulta y un solo orden: los pone `ServicioPreferencias::listar`.
    const auto resultado = ServicioPreferencias(m_db).listar(dto.id);
    if (!resultado.ok) {
        qCritical() << "Error al cargar preferencias:" << resultado.mensajeError;
        return false;
    }
    dto.preferencias = resultado.valor;
    return true;
}

bool ServicioProfesor::cargarMaterias(ProfesorDTO& dto) const {
    QSqlQuery q(m_db);
    q.prepare(
        "SELECT m.id AS id, m.nombre AS nombre "
        "FROM Profesor_Materia pm "
        "JOIN Materias m ON m.id = pm.id_Materia "
        "WHERE pm.id_Profesor = :id "
        "ORDER BY m.nombre"
    );
    q.bindValue(":id", dto.id);

    if (!q.exec()) {
        qCritical() << "Error al cargar materias:" << q.lastError().text();
        return false;
    }

    while (q.next()) {
        MateriaAsignadaDTO m;
        m.id     = q.value("id").toInt();
        m.nombre = q.value("nombre").toString();
        dto.materias.append(m);
    }

    // Materias suplentes: tabla auxiliar. Si aún no existe, la lista queda vacía.
    QSqlQuery suplentes(m_db);
    suplentes.prepare(
        "SELECT m.id AS id, m.nombre AS nombre "
        "FROM Profesor_Materia_Suplente pms "
        "JOIN Materias m ON m.id = pms.id_Materia "
        "WHERE pms.id_Profesor = :id "
        "ORDER BY m.nombre"
    );
    suplentes.bindValue(":id", dto.id);

    if (suplentes.exec()) {
        while (suplentes.next()) {
            MateriaAsignadaDTO m;
            m.id     = suplentes.value("id").toInt();
            m.nombre = suplentes.value("nombre").toString();
            dto.materiasSuplentes.append(m);
        }
    } else {
        qWarning() << "No se pudieron cargar las materias suplentes:"
                   << suplentes.lastError().text();
    }

    return true;
}

// ─── CRUD ──────────────────────────────────────────────────────────────────

Resultado<ProfesorDTO> ServicioProfesor::crearProfesor(const QString& id,
                                                       const QString& nombre,
                                                       const QString& email,
                                                       const QString& telefono) {
    QString error;
    if (!validarProfesor(id, nombre, email, error)) {
        return Resultado<ProfesorDTO>::error(error);
    }

    // Pre-chequeo de duplicados (nombre y email son UNIQUE en el schema)
    {
        QSqlQuery checkNombre(m_db);
        checkNombre.prepare("SELECT 1 FROM Profesores WHERE nombre = :nombre LIMIT 1");
        checkNombre.bindValue(":nombre", nombre.trimmed());
        if (checkNombre.exec() && checkNombre.next()) {
            return Resultado<ProfesorDTO>::error("Ya existe un profesor con ese nombre.", -2);
        }
    }
    {
        QSqlQuery checkEmail(m_db);
        checkEmail.prepare("SELECT 1 FROM Profesores WHERE email = :email LIMIT 1");
        checkEmail.bindValue(":email", email.trimmed());
        if (checkEmail.exec() && checkEmail.next()) {
            return Resultado<ProfesorDTO>::error("Ya existe un profesor con ese email.", -2);
        }
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO Profesores (id, nombre, email, telefono) "
        "VALUES (:id, :nombre, :email, :telefono)"
    );
    query.bindValue(":id",       id.trimmed());
    query.bindValue(":nombre",   nombre.trimmed());
    query.bindValue(":email",    email.trimmed());
    query.bindValue(":telefono", telefono.trimmed());

    if (!query.exec()) {
        const QString err = query.lastError().text();
        if (err.contains("UNIQUE")) {
            return Resultado<ProfesorDTO>::error(
                "Ya existe un profesor con ese ID, nombre o email.", -2);
        }
        qCritical() << "Error al crear profesor:" << err;
        return Resultado<ProfesorDTO>::error("Error al guardar el profesor en la base de datos.");
    }

    return obtenerProfesor(id.trimmed());
}

Resultado<ProfesorDTO> ServicioProfesor::obtenerProfesor(const QString& id) const {
    if (id.trimmed().isEmpty()) {
        return Resultado<ProfesorDTO>::error("ID de profesor inválido.");
    }

    QSqlQuery query(m_db);
    query.prepare("SELECT id, nombre, email, telefono FROM Profesores WHERE id = :id");
    query.bindValue(":id", id.trimmed());

    if (!query.exec()) {
        qCritical() << "Error al obtener profesor:" << query.lastError().text();
        return Resultado<ProfesorDTO>::error("Error al consultar la base de datos.");
    }

    if (!query.next()) {
        return Resultado<ProfesorDTO>::error("No se encontró un profesor con ese ID.", -3);
    }

    ProfesorDTO dto = mapearARecord(query.record());
    if (!cargarDisponibilidad(dto) || !cargarPreferencias(dto) || !cargarMaterias(dto)) {
        return Resultado<ProfesorDTO>::error(
            "Error al cargar los datos relacionados del profesor.");
    }

    return Resultado<ProfesorDTO>::exito(dto);
}

Resultado<QVector<ProfesorDTO>> ServicioProfesor::listarProfesores() const {
    QVector<ProfesorDTO> resultados;

    QSqlQuery query(m_db);
    // OJO: sin ORDER BY. SQLite usa BINARY y no ordena bien los acentos.
    query.prepare("SELECT id, nombre, email, telefono FROM Profesores");

    if (!query.exec()) {
        qCritical() << "Error al listar profesores:" << query.lastError().text();
        return Resultado<QVector<ProfesorDTO>>::error(
            QStringLiteral("No se pudieron leer los docentes registrados. Compruebe que la base de"
                           " datos esté disponible e intente de nuevo."));
    }

    while (query.next()) {
        ProfesorDTO dto = mapearARecord(query.record());
        cargarDisponibilidad(dto);
        cargarPreferencias(dto);
        cargarMaterias(dto);
        resultados.append(dto);
    }

    // Orden en español (acentos, ñ y mayúsculas) con QCollator (ver ordenacion.hpp).
    std::stable_sort(resultados.begin(), resultados.end(),
                     [](const ProfesorDTO& a, const ProfesorDTO& b) {
                         return nombreAntes(a.nombre, b.nombre);
                     });

    return Resultado<QVector<ProfesorDTO>>::exito(resultados);
}

Resultado<ProfesorDTO> ServicioProfesor::actualizarProfesor(const QString& id,
                                                            const QString& nombre,
                                                            const QString& email,
                                                            const QString& telefono) {
    QString error;
    if (!validarProfesor(id, nombre, email, error)) {
        return Resultado<ProfesorDTO>::error(error);
    }

    QSqlQuery query(m_db);
    query.prepare(
        "UPDATE Profesores SET "
        "nombre = :nombre, "
        "email = :email, "
        "telefono = :telefono, "
        "fecha_modificacion = CURRENT_TIMESTAMP "
        "WHERE id = :id"
    );
    query.bindValue(":nombre",   nombre.trimmed());
    query.bindValue(":email",    email.trimmed());
    query.bindValue(":telefono", telefono.trimmed());
    query.bindValue(":id",       id.trimmed());

    if (!query.exec()) {
        const QString err = query.lastError().text();
        if (err.contains("UNIQUE")) {
            return Resultado<ProfesorDTO>::error(
                "Ya existe un profesor con ese nombre o email.", -2);
        }
        qCritical() << "Error al actualizar profesor:" << err;
        return Resultado<ProfesorDTO>::error(
            "Error al actualizar el profesor en la base de datos.");
    }

    if (query.numRowsAffected() == 0) {
        return Resultado<ProfesorDTO>::error("No se encontró un profesor con ese ID.", -3);
    }

    return obtenerProfesor(id.trimmed());
}

bool ServicioProfesor::eliminarProfesor(const QString& id) {
    const QString idLimpio = id.trimmed();
    if (idLimpio.isEmpty()) return false;

    if (!m_db.transaction()) {
        qCritical() << "No se pudo iniciar la transacción para eliminar profesor.";
        return false;
    }

    // 1) Borrar hijas (no hay ON DELETE CASCADE en el schema)
    auto borrarHijas = [&](const QString& sql) -> bool {
        QSqlQuery q(m_db);
        q.prepare(sql);
        q.bindValue(":id", idLimpio);
        if (!q.exec()) {
            qCritical() << "Error al eliminar datos relacionados:" << q.lastError().text();
            return false;
        }
        return true;
    };

    if (!borrarHijas("DELETE FROM Preferencia_Profesor    WHERE id_Profesor = :id") ||
        !borrarHijas("DELETE FROM Disponibilidad_Profesor WHERE id_Profesor = :id") ||
        !borrarHijas("DELETE FROM Profesor_Materia       WHERE id_Profesor = :id") ||
        !borrarHijas("DELETE FROM Profesor_Materia_Suplente WHERE id_Profesor = :id")) {
        m_db.rollback();
        return false;
    }

    // 2) Borrar el profesor
    QSqlQuery del(m_db);
    del.prepare("DELETE FROM Profesores WHERE id = :id");
    del.bindValue(":id", idLimpio);

    if (!del.exec()) {
        qCritical() << "Error al eliminar profesor:" << del.lastError().text();
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

// ─── Disponibilidad ────────────────────────────────────────────────────────

Resultado<DisponibilidadDTO> ServicioProfesor::agregarDisponibilidad(
        const QString& idProfesor, const FranjaHoraria& franja) {
    if (idProfesor.trimmed().isEmpty()) {
        return Resultado<DisponibilidadDTO>::error("ID de profesor inválido.");
    }

    // Validación compartida con las preferencias: única definición de día y rango.
    const QString errorFranja = franja_horaria::errorDeFranja(franja);
    if (!errorFranja.isEmpty()) {
        return Resultado<DisponibilidadDTO>::error(errorFranja);
    }

    // Verificar que el profesor exista antes de insertar (mejor mensaje que FK)
    auto existente = obtenerProfesor(idProfesor.trimmed());
    if (!existente.ok) {
        return Resultado<DisponibilidadDTO>::error(
            existente.mensajeError, existente.codigoError);
    }

    QSqlQuery query(m_db);
    query.prepare(
        "INSERT INTO Disponibilidad_Profesor (id_Profesor, dia, hora_inicio, hora_fin) "
        "VALUES (:id_Profesor, :dia, :hora_inicio, :hora_fin)"
    );
    query.bindValue(":id_Profesor", idProfesor.trimmed());
    query.bindValue(":dia",         QString::number(franja.dia)); // columna TEXT
    query.bindValue(":hora_inicio", franja.inicio.toString(FORMATO_HORA));
    query.bindValue(":hora_fin",    franja.fin.toString(FORMATO_HORA));

    if (!query.exec()) {
        qCritical() << "Error al agregar disponibilidad:" << query.lastError().text();
        return Resultado<DisponibilidadDTO>::error("Error al guardar la disponibilidad.");
    }

    DisponibilidadDTO dto;
    dto.id     = query.lastInsertId().toInt();
    dto.franja = franja;
    return Resultado<DisponibilidadDTO>::exito(dto);
}

bool ServicioProfesor::eliminarDisponibilidad(int idDisponibilidad) {
    if (idDisponibilidad <= 0) return false;

    QSqlQuery q(m_db);
    q.prepare("DELETE FROM Disponibilidad_Profesor WHERE id = :id");
    q.bindValue(":id", idDisponibilidad);

    if (!q.exec()) {
        qCritical() << "Error al eliminar disponibilidad:" << q.lastError().text();
        return false;
    }
    return q.numRowsAffected() > 0;
}

bool ServicioProfesor::limpiarDisponibilidad(const QString& idProfesor) {
    if (idProfesor.trimmed().isEmpty()) return false;

    QSqlQuery q(m_db);
    q.prepare("DELETE FROM Disponibilidad_Profesor WHERE id_Profesor = :id");
    q.bindValue(":id", idProfesor.trimmed());

    if (!q.exec()) {
        qCritical() << "Error al limpiar disponibilidad:" << q.lastError().text();
        return false;
    }
    return true;
}

bool ServicioProfesor::reemplazarDisponibilidad(const QString& idProfesor,
                                                const QVector<FranjaHoraria>& franjas,
                                                QString& error) {
    const QString idLimpio = idProfesor.trimmed();
    if (idLimpio.isEmpty()) {
        error = QStringLiteral("ID de profesor inválido.");
        return false;
    }

    {
        QSqlQuery existe(m_db);
        existe.prepare("SELECT 1 FROM Profesores WHERE id = :id LIMIT 1");
        existe.bindValue(":id", idLimpio);
        if (!existe.exec() || !existe.next()) {
            error = QStringLiteral("El docente no existe en la base de datos.");
            return false;
        }
    }

    for (const FranjaHoraria& franja : franjas) {
        const QString errorFranja = franja_horaria::errorDeFranja(franja);
        if (!errorFranja.isEmpty()) {
            error = errorFranja;
            return false;
        }
    }

    QSqlQuery borrar(m_db);
    borrar.prepare("DELETE FROM Disponibilidad_Profesor WHERE id_Profesor = :id");
    borrar.bindValue(":id", idLimpio);
    if (!borrar.exec()) {
        error = QStringLiteral("No se pudo reemplazar la disponibilidad del docente.");
        qCritical() << "Error al limpiar disponibilidad:" << borrar.lastError().text();
        return false;
    }

    QSqlQuery insertar(m_db);
    insertar.prepare(
        "INSERT INTO Disponibilidad_Profesor (id_Profesor, dia, hora_inicio, hora_fin) "
        "VALUES (:id_Profesor, :dia, :hora_inicio, :hora_fin)");

    for (const FranjaHoraria& franja : franjas) {
        insertar.bindValue(":id_Profesor", idLimpio);
        insertar.bindValue(":dia", QString::number(franja.dia));
        insertar.bindValue(":hora_inicio", franja.inicio.toString(FORMATO_HORA));
        insertar.bindValue(":hora_fin", franja.fin.toString(FORMATO_HORA));
        if (!insertar.exec()) {
            error = QStringLiteral("No se pudo guardar la disponibilidad del docente.");
            qCritical() << "Error al insertar disponibilidad:" << insertar.lastError().text();
            return false;
        }
    }

    return true;
}

// ─── Materias (N:M) ────────────────────────────────────────────────────────

bool ServicioProfesor::asignarMateria(const QString& idProfesor, int idMateria) {
    if (idProfesor.trimmed().isEmpty() || idMateria <= 0) return false;

    QSqlQuery q(m_db);
    q.prepare(
        "INSERT OR IGNORE INTO Profesor_Materia (id_Profesor, id_Materia) "
        "VALUES (:id_Profesor, :id_Materia)"
    );
    q.bindValue(":id_Profesor", idProfesor.trimmed());
    q.bindValue(":id_Materia",  idMateria);

    if (!q.exec()) {
        qCritical() << "Error al asignar materia:" << q.lastError().text();
        return false;
    }
    // INSERT OR IGNORE: si ya existía, sigue siendo éxito idempotente.
    return true;
}

bool ServicioProfesor::asignarMateriaPorNombre(const QString& idProfesor,
                                               const QString& nombreMateria) {
    if (idProfesor.trimmed().isEmpty() || nombreMateria.trimmed().isEmpty()) {
        return false;
    }

    QSqlQuery lookup(m_db);
    lookup.prepare("SELECT id FROM Materias WHERE nombre = :nombre LIMIT 1");
    lookup.bindValue(":nombre", nombreMateria.trimmed());

    if (!lookup.exec() || !lookup.next()) {
        qWarning() << "No se encontró la materia:" << nombreMateria;
        return false;
    }

    return asignarMateria(idProfesor, lookup.value("id").toInt());
}

bool ServicioProfesor::quitarMateria(const QString& idProfesor, int idMateria) {
    if (idProfesor.trimmed().isEmpty() || idMateria <= 0) return false;

    QSqlQuery q(m_db);
    q.prepare(
        "DELETE FROM Profesor_Materia "
        "WHERE id_Profesor = :id_Profesor AND id_Materia = :id_Materia"
    );
    q.bindValue(":id_Profesor", idProfesor.trimmed());
    q.bindValue(":id_Materia",  idMateria);

    if (!q.exec()) {
        qCritical() << "Error al quitar materia:" << q.lastError().text();
        return false;
    }
    return q.numRowsAffected() > 0;
}

bool ServicioProfesor::limpiarMaterias(const QString& idProfesor) {
    if (idProfesor.trimmed().isEmpty()) return false;

    QSqlQuery q(m_db);
    q.prepare("DELETE FROM Profesor_Materia WHERE id_Profesor = :id");
    q.bindValue(":id", idProfesor.trimmed());

    if (!q.exec()) {
        qCritical() << "Error al limpiar materias:" << q.lastError().text();
        return false;
    }
    return true;
}

bool ServicioProfesor::reemplazarMaterias(const QString& idProfesor,
                                          const QVector<int>& idsMaterias,
                                          QString& error) {
    const QString idLimpio = idProfesor.trimmed();
    if (idLimpio.isEmpty()) {
        error = QStringLiteral("ID de profesor inválido.");
        return false;
    }

    if (!limpiarMaterias(idLimpio)) {
        error = QStringLiteral("No se pudieron reemplazar las materias del docente.");
        return false;
    }

    QSqlQuery insertar(m_db);
    insertar.prepare(
        "INSERT OR IGNORE INTO Profesor_Materia (id_Profesor, id_Materia) "
        "VALUES (:id_Profesor, :id_Materia)");

    for (int idMateria : idsMaterias) {
        if (idMateria <= 0) {
            error = QStringLiteral("Hay una materia inválida en la selección.");
            return false;
        }
        insertar.bindValue(":id_Profesor", idLimpio);
        insertar.bindValue(":id_Materia", idMateria);
        if (!insertar.exec()) {
            error = QStringLiteral("No se pudieron guardar las materias del docente.");
            qCritical() << "Error al asignar materia:" << insertar.lastError().text();
            return false;
        }
    }
    return true;
}

// ─── Materias suplentes (tabla auxiliar) ───────────────────────────────────

bool ServicioProfesor::asignarMateriaSuplente(const QString& idProfesor, int idMateria) {
    if (idProfesor.trimmed().isEmpty() || idMateria <= 0) return false;
    asegurarTablaSuplentes();

    QSqlQuery q(m_db);
    q.prepare(
        "INSERT OR IGNORE INTO Profesor_Materia_Suplente (id_Profesor, id_Materia) "
        "VALUES (:id_Profesor, :id_Materia)");
    q.bindValue(":id_Profesor", idProfesor.trimmed());
    q.bindValue(":id_Materia", idMateria);

    if (!q.exec()) {
        qCritical() << "Error al asignar materia suplente:" << q.lastError().text();
        return false;
    }
    return true;
}

bool ServicioProfesor::quitarMateriaSuplente(const QString& idProfesor, int idMateria) {
    if (idProfesor.trimmed().isEmpty() || idMateria <= 0) return false;

    QSqlQuery q(m_db);
    q.prepare(
        "DELETE FROM Profesor_Materia_Suplente "
        "WHERE id_Profesor = :id_Profesor AND id_Materia = :id_Materia");
    q.bindValue(":id_Profesor", idProfesor.trimmed());
    q.bindValue(":id_Materia", idMateria);

    if (!q.exec()) {
        qCritical() << "Error al quitar materia suplente:" << q.lastError().text();
        return false;
    }
    return q.numRowsAffected() > 0;
}

bool ServicioProfesor::limpiarMateriasSuplentes(const QString& idProfesor) {
    if (idProfesor.trimmed().isEmpty()) return false;
    asegurarTablaSuplentes();

    QSqlQuery q(m_db);
    q.prepare("DELETE FROM Profesor_Materia_Suplente WHERE id_Profesor = :id");
    q.bindValue(":id", idProfesor.trimmed());

    if (!q.exec()) {
        qCritical() << "Error al limpiar materias suplentes:" << q.lastError().text();
        return false;
    }
    return true;
}

bool ServicioProfesor::reemplazarMateriasSuplentes(const QString& idProfesor,
                                                   const QVector<int>& idsMaterias,
                                                   QString& error) {
    const QString idLimpio = idProfesor.trimmed();
    if (idLimpio.isEmpty()) {
        error = QStringLiteral("ID de profesor inválido.");
        return false;
    }

    if (!limpiarMateriasSuplentes(idLimpio)) {
        error = QStringLiteral("No se pudieron reemplazar las materias suplentes del docente.");
        return false;
    }

    QSqlQuery insertar(m_db);
    insertar.prepare(
        "INSERT OR IGNORE INTO Profesor_Materia_Suplente (id_Profesor, id_Materia) "
        "VALUES (:id_Profesor, :id_Materia)");

    for (int idMateria : idsMaterias) {
        if (idMateria <= 0) {
            error = QStringLiteral("Hay una materia suplente inválida en la selección.");
            return false;
        }
        insertar.bindValue(":id_Profesor", idLimpio);
        insertar.bindValue(":id_Materia", idMateria);
        if (!insertar.exec()) {
            error = QStringLiteral("No se pudieron guardar las materias suplentes del docente.");
            qCritical() << "Error al asignar materia suplente:" << insertar.lastError().text();
            return false;
        }
    }
    return true;
}

// ─── Extracción para solver ────────────────────────────────────────────────

QVector<Profesor> ServicioProfesor::obtenerTodosParaSolver() const {
    QVector<Profesor> profesores;
    const auto dtoResultado = listarProfesores();

    if (!dtoResultado.ok) {
        qCritical() << "No se pudieron leer los docentes para el solver:"
                    << dtoResultado.mensajeError;
        return profesores;
    }

    profesores.reserve(dtoResultado.valor.size());

    for (const auto& dto : dtoResultado.valor) {
        profesores.append(dto.toProfesor());
    }

    return profesores;
}

Resultado<Profesor> ServicioProfesor::obtenerProfesorParaSolver(const QString& id) const {
    auto resultado = obtenerProfesor(id);

    if (!resultado.ok) {
        return Resultado<Profesor>::error(resultado.mensajeError, resultado.codigoError);
    }

    return Resultado<Profesor>::exito(resultado.valor.toProfesor());
}
