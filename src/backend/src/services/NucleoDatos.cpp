#include "backend/services/NucleoDatos.hpp"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

NucleoDatos::NucleoDatos(QSqlDatabase& db)
    : m_db(db),
      m_profesor(db),
      m_aula(db),
      m_materias(db),
      m_planes(db),
      m_cursos(db),
      m_turnos(db) {}

// ─── Docentes ──────────────────────────────────────────────────────────────

QVector<ProfesorDTO> NucleoDatos::listarDocentes() const {
    return m_profesor.listarProfesores();
}

Resultado<ProfesorDTO> NucleoDatos::crearDocente(const QString& id,
                                                 const QString& nombre,
                                                 const QString& email,
                                                 const QString& telefono) {
    return m_profesor.crearProfesor(id, nombre, email, telefono);
}

Resultado<ProfesorDTO> NucleoDatos::actualizarDocente(const QString& id,
                                                      const QString& nombre,
                                                      const QString& email,
                                                      const QString& telefono) {
    return m_profesor.actualizarProfesor(id, nombre, email, telefono);
}

bool NucleoDatos::eliminarDocente(const QString& id) {
    return m_profesor.eliminarProfesor(id);
}

// ─── Aulas ─────────────────────────────────────────────────────────────────

QVector<AulaDTO> NucleoDatos::listarAulas() const {
    return m_aula.listarAulas();
}

Resultado<AulaDTO> NucleoDatos::crearAula(const QString& nombre, int capacidad,
                                          const QString& edificio,
                                          const QString& piso) {
    return m_aula.crearAula(nombre, capacidad, edificio, piso);
}

Resultado<AulaDTO> NucleoDatos::actualizarAula(int id, const QString& nombre,
                                               int capacidad, const QString& edificio,
                                               const QString& piso) {
    return m_aula.actualizarAula(id, nombre, capacidad, edificio, piso);
}

bool NucleoDatos::eliminarAula(int id) {
    return m_aula.eliminarAula(id);
}

// ─── Materias ──────────────────────────────────────────────────────────────

QVector<MateriaDTO> NucleoDatos::listarMaterias() const {
    return m_materias.listarMaterias();
}

Resultado<MateriaDTO> NucleoDatos::crearMateria(const QString& nombre,
                                                const QString& requisitos) {
    return m_materias.crearMateria(nombre, requisitos);
}

Resultado<MateriaDTO> NucleoDatos::actualizarMateria(int id, const QString& nombre,
                                                     const QString& requisitos) {
    return m_materias.actualizarMateria(id, nombre, requisitos);
}

bool NucleoDatos::eliminarMateria(int id) {
    return m_materias.eliminarMateria(id);
}

// ─── Planes de estudio ─────────────────────────────────────────────────────

QVector<PlanDTO> NucleoDatos::listarPlanes() const {
    return m_planes.listarPlanes();
}

Resultado<PlanDTO> NucleoDatos::crearPlan(const QString& codigo, const QString& nombre,
                                          const QString& descripcion) {
    return m_planes.crearPlan(codigo, nombre, descripcion);
}

Resultado<PlanDTO> NucleoDatos::actualizarPlan(const QString& codigo,
                                               const QString& nombre,
                                               const QString& descripcion) {
    return m_planes.actualizarPlan(codigo, nombre, descripcion);
}

bool NucleoDatos::eliminarPlan(const QString& codigo) {
    return m_planes.eliminarPlan(codigo);
}

// ─── Cursos ────────────────────────────────────────────────────────────────

QVector<CursoDTO> NucleoDatos::listarCursos() const {
    return m_cursos.listarCursos();
}

Resultado<CursoDTO> NucleoDatos::crearCurso(const QString& nombre,
                                            const QString& turno, int aulaFija,
                                            int numEstudiantes,
                                            const QString& codigoPlan) {
    return m_cursos.crearCurso(nombre, turno, aulaFija, numEstudiantes, codigoPlan);
}

Resultado<CursoDTO> NucleoDatos::actualizarCurso(int id, const QString& nombre,
                                                 const QString& turno, int aulaFija,
                                                 int numEstudiantes,
                                                 const QString& codigoPlan) {
    return m_cursos.actualizarCurso(id, nombre, turno, aulaFija, numEstudiantes,
                                    codigoPlan);
}

bool NucleoDatos::eliminarCurso(int id) {
    return m_cursos.eliminarCurso(id);
}

Resultado<CursoMateriaDTO> NucleoDatos::asignarMateriaACurso(int idCurso, int idMateria,
                                                             int horasSemanales) {
    return m_cursos.asignarMateria(idCurso, idMateria, horasSemanales);
}

bool NucleoDatos::quitarMateriaDeCurso(int idCurso, int idMateria) {
    return m_cursos.quitarMateria(idCurso, idMateria);
}

// ─── Turnos y recesos ──────────────────────────────────────────────────────

QVector<TurnoDTO> NucleoDatos::listarTurnos() const {
    return m_turnos.listarTurnos();
}

Resultado<TurnoDTO> NucleoDatos::crearTurno(const QString& nombre, const QTime& inicio,
                                            const QTime& fin, int numSlots) {
    return m_turnos.crearTurno(nombre, inicio, fin, numSlots);
}

Resultado<TurnoDTO> NucleoDatos::actualizarTurno(const QString& nombre,
                                                 const QTime& inicio, const QTime& fin,
                                                 int numSlots) {
    return m_turnos.actualizarTurno(nombre, inicio, fin, numSlots);
}

bool NucleoDatos::eliminarTurno(const QString& nombre) {
    return m_turnos.eliminarTurno(nombre);
}

Resultado<RecesoDTO> NucleoDatos::agregarReceso(const QString& turno, int despuesDeSlot,
                                                int duracion, const QTime& inicio,
                                                const QTime& fin) {
    return m_turnos.agregarReceso(turno, despuesDeSlot, duracion, inicio, fin);
}

bool NucleoDatos::eliminarReceso(const QString& turno, int despuesDeSlot) {
    return m_turnos.eliminarReceso(turno, despuesDeSlot);
}

// ─── Eliminación en cascada (RF-2) ────────────────────────────────────────

QVector<Dependencia> NucleoDatos::recolectarDependientes(const QString& dominio,
                                                         const QString& id) const {
    QVector<Dependencia> dependientes;

    const QVector<RelacionCascada> relaciones = dependenciasDe(dominio);
    for (const RelacionCascada& rel : relaciones) {
        const QString columnaId =
            rel.columnaId.isEmpty() ? QStringLiteral("rowid") : rel.columnaId;

        QString sql = QStringLiteral("SELECT %1 AS id").arg(columnaId);
        if (!rel.etiqueta.isEmpty()) {
            sql += QStringLiteral(", %1 AS etiqueta").arg(rel.etiqueta);
        }
        sql += QStringLiteral(" FROM %1 WHERE %2 = :padre").arg(rel.tabla, rel.columna);

        QSqlQuery query(m_db);
        query.prepare(sql);
        query.bindValue(":padre", id);
        if (!query.exec()) {
            qCritical() << "Error al consultar dependientes de" << dominio << ":"
                        << query.lastError().text();
            continue;
        }

        while (query.next()) {
            Dependencia dep;
            dep.dominio = rel.dominio;
            dep.id = query.value("id").toLongLong();

            const QString etiqueta =
                rel.etiqueta.isEmpty() ? QString() : query.value("etiqueta").toString();
            dep.descripcion = etiqueta.isEmpty()
                                  ? rel.descripcion
                                  : QStringLiteral("%1: %2").arg(rel.descripcion, etiqueta);
            dependientes.append(dep);

            // La cascada es recursiva: un curso dependiente arrastra sus materias.
            if (rel.esDominio) {
                dependientes += recolectarDependientes(rel.dominio, QString::number(dep.id));
            }
        }
    }

    return dependientes;
}

QVector<Dependencia> NucleoDatos::dependientesDe(const QString& dominio,
                                                 const QString& id) const {
    if (!dominioValido(dominio) || id.isEmpty()) {
        return {};
    }
    return recolectarDependientes(dominio, id);
}

bool NucleoDatos::borrarRecursivo(const QString& dominio, const QString& id) {
    const QVector<RelacionCascada> relaciones = dependenciasDe(dominio);
    for (const RelacionCascada& rel : relaciones) {
        if (rel.esDominio) {
            // Recoge los ids de los hijos y los borra (con sus dependencias) antes
            // de borrar el padre, respetando las claves foráneas.
            const QString columnaId =
                rel.columnaId.isEmpty() ? QStringLiteral("rowid") : rel.columnaId;

            QSqlQuery seleccion(m_db);
            seleccion.prepare(QStringLiteral("SELECT %1 FROM %2 WHERE %3 = :padre")
                                  .arg(columnaId, rel.tabla, rel.columna));
            seleccion.bindValue(":padre", id);
            if (!seleccion.exec()) {
                qCritical() << "Error al listar dependientes de" << dominio << ":"
                            << seleccion.lastError().text();
                return false;
            }

            QVector<QString> idsHijos;
            while (seleccion.next()) {
                idsHijos.append(seleccion.value(0).toString());
            }
            for (const QString& idHijo : idsHijos) {
                if (!borrarRecursivo(rel.dominio, idHijo)) {
                    return false;
                }
            }
            continue;
        }

        QSqlQuery borrado(m_db);
        borrado.prepare(QStringLiteral("DELETE FROM %1 WHERE %2 = :padre")
                            .arg(rel.tabla, rel.columna));
        borrado.bindValue(":padre", id);
        if (!borrado.exec()) {
            qCritical() << "Error al borrar dependientes de" << dominio << ":"
                        << borrado.lastError().text();
            return false;
        }
    }

    const ClaveDominio clave = claveDeDominio(dominio);
    if (clave.tabla.isEmpty()) {
        return false;
    }

    QSqlQuery borradoRaiz(m_db);
    borradoRaiz.prepare(QStringLiteral("DELETE FROM %1 WHERE %2 = :clave")
                            .arg(clave.tabla, clave.columna));
    borradoRaiz.bindValue(":clave", id);
    if (!borradoRaiz.exec()) {
        qCritical() << "Error al borrar" << dominio << ":" << borradoRaiz.lastError().text();
        return false;
    }

    if (borradoRaiz.numRowsAffected() == 0) {
        qWarning() << "No se encontró" << dominio << "con clave" << id;
        return false;
    }

    return true;
}

bool NucleoDatos::eliminarConCascada(const QString& dominio, const QString& id) {
    if (!dominioValido(dominio) || id.isEmpty()) {
        qWarning() << "Dominio o id inválido para la cascada:" << dominio << id;
        return false;
    }

    if (!m_db.transaction()) {
        qCritical() << "No se pudo iniciar la transacción para la cascada de" << dominio;
        return false;
    }

    if (!borrarRecursivo(dominio, id)) {
        m_db.rollback();
        return false;
    }

    if (!m_db.commit()) {
        qCritical() << "No se pudo confirmar la cascada de" << dominio << ":"
                    << m_db.lastError().text();
        m_db.rollback();
        return false;
    }

    return true;
}
