#include "backend/services/NucleoDatos.hpp"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

/// Clave textual de un registro cuyo identificador es numérico.
QString claveDe(qint64 id) {
    return QString::number(id);
}

/**
 * @brief Ejecuta una escritura y, si falla, la registra como cambio pendiente.
 *
 * El frontend puede reintentarla después (RF-3). Si el reintento vuelve a
 * fallar, el pendiente se conserva (RNF-3).
 */
template <typename Accion>
auto conPendiente(GestorPendientes& gestor, const QString& dominio,
                  const QString& registroId, TipoOperacion tipo,
                  const QString& descripcion, Accion accion) -> decltype(accion()) {
    decltype(accion()) resultado = accion();
    if (!resultado.ok) {
        OperacionPendiente operacion;
        operacion.dominio = dominio;
        operacion.registroId = registroId;
        operacion.tipo = tipo;
        operacion.descripcion = descripcion;
        operacion.accion = [accion]() { return accion().ok; };
        gestor.registrar(operacion);
    }
    return resultado;
}

/// Igual que `conPendiente`, para escrituras que devuelven `bool`.
template <typename Accion>
bool conPendienteBool(GestorPendientes& gestor, const QString& dominio,
                      const QString& registroId, TipoOperacion tipo,
                      const QString& descripcion, Accion accion) {
    const bool ok = accion();
    if (!ok) {
        OperacionPendiente operacion;
        operacion.dominio = dominio;
        operacion.registroId = registroId;
        operacion.tipo = tipo;
        operacion.descripcion = descripcion;
        operacion.accion = [accion]() { return accion(); };
        gestor.registrar(operacion);
    }
    return ok;
}

} // namespace

NucleoDatos::NucleoDatos(QSqlDatabase& db)
    : m_db(db),
      m_profesor(db),
      m_aula(db),
      m_materias(db),
      m_tipos(db),
      m_planes(db),
      m_carga(db),
      m_composicion(db),
      m_cursos(db),
      m_turnos(db) {}

// ─── Docentes ──────────────────────────────────────────────────────────────

Resultado<QVector<ProfesorDTO>> NucleoDatos::listarDocentes() const {
    return m_profesor.listarProfesores();
}

Resultado<ProfesorDTO> NucleoDatos::crearDocente(const QString& id,
                                                 const QString& nombre,
                                                 const QString& email,
                                                 const QString& telefono) {
    return conPendiente(
        m_pendientes, QStringLiteral("docente"), id, TipoOperacion::Alta,
        QStringLiteral("Pendiente de guardar: docente %1").arg(id),
        [this, id, nombre, email, telefono]() {
            return m_profesor.crearProfesor(id, nombre, email, telefono);
        });
}

Resultado<ProfesorDTO> NucleoDatos::actualizarDocente(const QString& id,
                                                      const QString& nombre,
                                                      const QString& email,
                                                      const QString& telefono) {
    return conPendiente(
        m_pendientes, QStringLiteral("docente"), id, TipoOperacion::Modificacion,
        QStringLiteral("Pendiente de guardar: docente %1").arg(id),
        [this, id, nombre, email, telefono]() {
            return m_profesor.actualizarProfesor(id, nombre, email, telefono);
        });
}

bool NucleoDatos::eliminarDocente(const QString& id) {
    return conPendienteBool(
        m_pendientes, QStringLiteral("docente"), id, TipoOperacion::Baja,
        QStringLiteral("Pendiente de eliminar: docente %1").arg(id),
        [this, id]() { return m_profesor.eliminarProfesor(id); });
}

bool NucleoDatos::asignarMateriaPorNombre(const QString& idProfesor, const QString& nombreMateria) {
    return conPendienteBool(
        m_pendientes, QStringLiteral("docente_materia"), idProfesor, TipoOperacion::Alta,
        QStringLiteral("Pendiente de asignar materia %1 al docente %2").arg(nombreMateria, idProfesor),
        [this, idProfesor, nombreMateria]() {
            return m_profesor.asignarMateriaPorNombre(idProfesor, nombreMateria);
        });
}

bool NucleoDatos::limpiarMaterias(const QString& idProfesor) {
    return conPendienteBool(
        m_pendientes, QStringLiteral("docente_materia"), idProfesor, TipoOperacion::Baja,
        QStringLiteral("Pendiente de limpiar materias del docente %1").arg(idProfesor),
        [this, idProfesor]() {
            return m_profesor.limpiarMaterias(idProfesor);
        });
}

// ─── Aulas ─────────────────────────────────────────────────────────────────

Resultado<QVector<AulaDTO>> NucleoDatos::listarAulas() const {
    return m_aula.listarAulas();
}

Resultado<AulaDTO> NucleoDatos::crearAula(const QString& nombre, int capacidad,
                                          const QString& edificio,
                                          const QString& piso, int idTipoAula) {
    return conPendiente(
        m_pendientes, QStringLiteral("aula"), nombre, TipoOperacion::Alta,
        QStringLiteral("Pendiente de guardar: aula %1").arg(nombre),
        [this, nombre, capacidad, edificio, piso, idTipoAula]() {
            return m_aula.crearAula(nombre, capacidad, edificio, piso, idTipoAula);
        });
}

Resultado<AulaDTO> NucleoDatos::actualizarAula(int id, const QString& nombre,
                                               int capacidad, const QString& edificio,
                                               const QString& piso, int idTipoAula) {
    return conPendiente(
        m_pendientes, QStringLiteral("aula"), claveDe(id), TipoOperacion::Modificacion,
        QStringLiteral("Pendiente de guardar: aula %1").arg(nombre),
        [this, id, nombre, capacidad, edificio, piso, idTipoAula]() {
            return m_aula.actualizarAula(id, nombre, capacidad, edificio, piso, idTipoAula);
        });
}

bool NucleoDatos::eliminarAula(int id) {
    return conPendienteBool(
        m_pendientes, QStringLiteral("aula"), claveDe(id), TipoOperacion::Baja,
        QStringLiteral("Pendiente de eliminar: aula %1").arg(id),
        [this, id]() { return m_aula.eliminarAula(id); });
}

// ─── Materias ──────────────────────────────────────────────────────────────

Resultado<QVector<MateriaDTO>> NucleoDatos::listarMaterias() const {
    return m_materias.listarMaterias();
}

Resultado<MateriaDTO> NucleoDatos::crearMateria(const QString& nombre,
                                                const QString& requisitos,
                                                int idTipoAula) {
    return conPendiente(
        m_pendientes, QStringLiteral("materia"), nombre, TipoOperacion::Alta,
        QStringLiteral("Pendiente de guardar: materia %1").arg(nombre),
        [this, nombre, requisitos, idTipoAula]() {
            return m_materias.crearMateria(nombre, requisitos, idTipoAula);
        });
}

Resultado<MateriaDTO> NucleoDatos::actualizarMateria(int id, const QString& nombre,
                                                     const QString& requisitos,
                                                     int idTipoAula) {
    return conPendiente(
        m_pendientes, QStringLiteral("materia"), claveDe(id), TipoOperacion::Modificacion,
        QStringLiteral("Pendiente de guardar: materia %1").arg(nombre),
        [this, id, nombre, requisitos, idTipoAula]() {
            return m_materias.actualizarMateria(id, nombre, requisitos, idTipoAula);
        });
}

bool NucleoDatos::eliminarMateria(int id) {
    return conPendienteBool(
        m_pendientes, QStringLiteral("materia"), claveDe(id), TipoOperacion::Baja,
        QStringLiteral("Pendiente de eliminar: materia %1").arg(id),
        [this, id]() { return m_materias.eliminarMateria(id); });
}

// ─── Tipos de aula ─────────────────────────────────────────────────────────

Resultado<QVector<TipoAulaDTO>> NucleoDatos::listarTiposAula() const {
    return m_tipos.listarTiposAula();
}

Resultado<TipoAulaDTO> NucleoDatos::obtenerTipoAula(int id) const {
    return m_tipos.obtenerTipoAula(id);
}

Resultado<TipoAulaDTO> NucleoDatos::crearTipoAula(const QString& nombre) {
    return conPendiente(
        m_pendientes, QStringLiteral("tipo_aula"), nombre, TipoOperacion::Alta,
        QStringLiteral("Pendiente de guardar: tipo de aula %1").arg(nombre),
        [this, nombre]() { return m_tipos.crearTipoAula(nombre); });
}

Resultado<TipoAulaDTO> NucleoDatos::renombrarTipoAula(int id, const QString& nombre) {
    return conPendiente(
        m_pendientes, QStringLiteral("tipo_aula"), claveDe(id), TipoOperacion::Modificacion,
        QStringLiteral("Pendiente de guardar: tipo de aula %1").arg(nombre),
        [this, id, nombre]() { return m_tipos.renombrarTipoAula(id, nombre); });
}

Resultado<bool> NucleoDatos::eliminarTipoAula(int id) {
    // El borrado en uso siempre se rechaza (RF-5): no se registra como pendiente
    // confirmable, sino que se devuelve el error con los dependientes.
    return m_tipos.eliminarTipoAula(id);
}

// ─── Planes de estudio ─────────────────────────────────────────────────────

Resultado<QVector<PlanDTO>> NucleoDatos::listarPlanes() const {
    return m_planes.listarPlanes();
}

Resultado<PlanDTO> NucleoDatos::crearPlan(const QString& codigo, const QString& nombre,
                                          const QString& descripcion) {
    return conPendiente(
        m_pendientes, QStringLiteral("plan"), codigo, TipoOperacion::Alta,
        QStringLiteral("Pendiente de guardar: plan %1").arg(codigo),
        [this, codigo, nombre, descripcion]() {
            return m_planes.crearPlan(codigo, nombre, descripcion);
        });
}

Resultado<PlanDTO> NucleoDatos::actualizarPlan(const QString& codigo,
                                               const QString& nombre,
                                               const QString& descripcion) {
    return conPendiente(
        m_pendientes, QStringLiteral("plan"), codigo, TipoOperacion::Modificacion,
        QStringLiteral("Pendiente de guardar: plan %1").arg(codigo),
        [this, codigo, nombre, descripcion]() {
            return m_planes.actualizarPlan(codigo, nombre, descripcion);
        });
}

bool NucleoDatos::eliminarPlan(const QString& codigo) {
    return conPendienteBool(
        m_pendientes, QStringLiteral("plan"), codigo, TipoOperacion::Baja,
        QStringLiteral("Pendiente de eliminar: plan %1").arg(codigo),
        [this, codigo]() { return m_planes.eliminarPlan(codigo); });
}

Resultado<QVector<CargaHorariaDTO>> NucleoDatos::listarCargaDePlan(
    const QString& codigoPlan) const {
    const QString codigo = codigoPlan.trimmed();
    if (codigo.isEmpty()) {
        return Resultado<QVector<CargaHorariaDTO>>::error(
            QStringLiteral("El código del plan de estudio no puede estar vacío."));
    }
    return Resultado<QVector<CargaHorariaDTO>>::exito(
        m_carga.listarCargaPorPlan(codigo));
}

Resultado<PlanEstudio> NucleoDatos::componerPlanCompleto(const QString& codigoPlan) const {
    return m_composicion.componerPlanCompleto(codigoPlan);
}

// ─── Cursos ────────────────────────────────────────────────────────────────

Resultado<QVector<CursoDTO>> NucleoDatos::listarCursos() const {
    return m_cursos.listarCursos();
}

Resultado<CursoDTO> NucleoDatos::crearCurso(int anio, const QString& seccion,
                                            const QString& turno, int aula) {
    const QString identidad = QStringLiteral("%1-%2").arg(anio).arg(normalizarSeccion(seccion));
    return conPendiente(
        m_pendientes, QStringLiteral("curso"), identidad, TipoOperacion::Alta,
        QStringLiteral("Pendiente de guardar: curso %1")
            .arg(nombreCurso(anio, normalizarSeccion(seccion))),
        [this, anio, seccion, turno, aula]() {
            return m_cursos.crearCurso(anio, seccion, turno, aula);
        });
}

Resultado<CursoDTO> NucleoDatos::actualizarCurso(int id, int anio, const QString& seccion,
                                                 const QString& turno, int aula) {
    return conPendiente(
        m_pendientes, QStringLiteral("curso"), claveDe(id), TipoOperacion::Modificacion,
        QStringLiteral("Pendiente de guardar: curso %1")
            .arg(nombreCurso(anio, normalizarSeccion(seccion))),
        [this, id, anio, seccion, turno, aula]() {
            return m_cursos.actualizarCurso(id, anio, seccion, turno, aula);
        });
}

bool NucleoDatos::eliminarCurso(int id) {
    return conPendienteBool(
        m_pendientes, QStringLiteral("curso"), claveDe(id), TipoOperacion::Baja,
        QStringLiteral("Pendiente de eliminar: curso %1").arg(id),
        [this, id]() { return m_cursos.eliminarCurso(id); });
}

// ─── Turnos y eventos ──────────────────────────────────────────────────────

Resultado<QVector<TurnoDTO>> NucleoDatos::listarTurnos() const {
    return m_turnos.listarTurnos();
}

Resultado<TurnoDTO> NucleoDatos::obtenerTurno(const QString& nombre) const {
    return m_turnos.obtenerTurno(nombre);
}

Resultado<TurnoDTO> NucleoDatos::crearTurno(const QString& nombre, const QTime& inicio,
                                            const QTime& fin, int numSlots) {
    return conPendiente(
        m_pendientes, QStringLiteral("turno"), nombre, TipoOperacion::Alta,
        QStringLiteral("Pendiente de guardar: turno %1").arg(nombre),
        [this, nombre, inicio, fin, numSlots]() {
            return m_turnos.crearTurno(nombre, inicio, fin, numSlots);
        });
}

Resultado<TurnoDTO> NucleoDatos::actualizarTurno(const QString& nombre,
                                                 const QTime& inicio, const QTime& fin,
                                                 int numSlots,
                                                 bool eliminarEventosFueraDeRango) {
    return conPendiente(
        m_pendientes, QStringLiteral("turno"), nombre, TipoOperacion::Modificacion,
        QStringLiteral("Pendiente de guardar: turno %1").arg(nombre),
        [this, nombre, inicio, fin, numSlots, eliminarEventosFueraDeRango]() {
            return m_turnos.actualizarTurno(nombre, inicio, fin, numSlots,
                                            eliminarEventosFueraDeRango);
        });
}

bool NucleoDatos::eliminarTurno(const QString& nombre) {
    return conPendienteBool(
        m_pendientes, QStringLiteral("turno"), nombre, TipoOperacion::Baja,
        QStringLiteral("Pendiente de eliminar: turno %1").arg(nombre),
        [this, nombre]() { return m_turnos.eliminarTurno(nombre); });
}

Resultado<EventoDTO> NucleoDatos::agregarEvento(const QString& turno, const QString& nombre,
                                                     int despuesDeSlot, int duracion,
                                                     const QTime& inicio, const QTime& fin,
                                                     int orden) {
    const QString clave =
        QStringLiteral("%1-%2-%3").arg(turno).arg(despuesDeSlot).arg(orden);
    return conPendiente(
        m_pendientes, QStringLiteral("evento"), clave, TipoOperacion::Alta,
        QStringLiteral("Pendiente de guardar: evento «%1» del turno %2").arg(nombre, turno),
        [this, turno, nombre, despuesDeSlot, duracion, inicio, fin, orden]() {
            return m_turnos.agregarEvento(turno, nombre, despuesDeSlot, duracion,
                                          inicio, fin, orden);
        });
}

Resultado<EventoDTO> NucleoDatos::actualizarEvento(int id, const QString& nombre,
                                                  int despuesDeSlot, int duracion,
                                                  const QTime& inicio, const QTime& fin,
                                                  int orden) {
    return conPendiente(
        m_pendientes, QStringLiteral("evento"), claveDe(id), TipoOperacion::Modificacion,
        QStringLiteral("Pendiente de guardar: evento %1").arg(id),
        [this, id, nombre, despuesDeSlot, duracion, inicio, fin, orden]() {
            return m_turnos.actualizarEvento(id, nombre, despuesDeSlot, duracion,
                                             inicio, fin, orden);
        });
}

bool NucleoDatos::eliminarEvento(int id) {
    return conPendienteBool(
        m_pendientes, QStringLiteral("evento"), claveDe(id), TipoOperacion::Baja,
        QStringLiteral("Pendiente de eliminar: evento %1").arg(id),
        [this, id]() { return m_turnos.eliminarEvento(id); });
}

Resultado<QVector<EventoDTO>> NucleoDatos::eventosFueraDeRango(const QString& turno,
                                                               int numSlots) const {
    return m_turnos.eventosFueraDeRango(turno, numSlots);
}

// ─── Recesos (compatibilidad transitoria; se retiran en T9) ────────────────

Resultado<RecesoDTO> NucleoDatos::agregarReceso(const QString& turno, int despuesDeSlot,
                                                int duracion, const QTime& inicio,
                                                const QTime& fin) {
    const QString clave = QStringLiteral("%1-%2").arg(turno).arg(despuesDeSlot);
    return conPendiente(
        m_pendientes, QStringLiteral("receso"), clave, TipoOperacion::Alta,
        QStringLiteral("Pendiente de guardar: receso del turno %1").arg(turno),
        [this, turno, despuesDeSlot, duracion, inicio, fin]() {
            return m_turnos.agregarReceso(turno, despuesDeSlot, duracion, inicio, fin);
        });
}

bool NucleoDatos::eliminarReceso(const QString& turno, int despuesDeSlot) {
    const QString clave = QStringLiteral("%1-%2").arg(turno).arg(despuesDeSlot);
    return conPendienteBool(
        m_pendientes, QStringLiteral("receso"), clave, TipoOperacion::Baja,
        QStringLiteral("Pendiente de eliminar: receso del turno %1").arg(turno),
        [this, turno, despuesDeSlot]() { return m_turnos.eliminarReceso(turno, despuesDeSlot); });
}

// ─── Cambios pendientes y reintento (RF-3) ────────────────────────────────

GestorPendientes& NucleoDatos::gestorPendientes() {
    return m_pendientes;
}

const GestorPendientes& NucleoDatos::gestorPendientes() const {
    return m_pendientes;
}

bool NucleoDatos::hayPendientes() const {
    return m_pendientes.hayPendientes();
}

Resultado<bool> NucleoDatos::reintentarPendiente(qint64 id) {
    return m_pendientes.reintentar(id);
}


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
        if (rel.modo == ModoAccion::AnularReferencia) {
            // Deja la referencia en NULL; el dependiente no se borra (RF-1, RF-5).
            QSqlQuery anular(m_db);
            anular.prepare(QStringLiteral("UPDATE %1 SET %2 = NULL WHERE %2 = :padre")
                               .arg(rel.tabla, rel.columna));
            anular.bindValue(":padre", id);
            if (!anular.exec()) {
                qCritical() << "Error al anular la referencia de" << dominio << ":"
                            << anular.lastError().text();
                return false;
            }
            continue;
        }

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
    return conPendienteBool(
        m_pendientes, dominio, id, TipoOperacion::Baja,
        QStringLiteral("Pendiente de eliminar: %1 %2").arg(dominio, id),
        [this, dominio, id]() -> bool {
            if (!dominioValido(dominio) || id.isEmpty()) {
                qWarning() << "Dominio o id inválido para la cascada:" << dominio << id;
                return false;
            }

            if (!m_db.transaction()) {
                qCritical() << "No se pudo iniciar la transacción para la cascada de"
                            << dominio;
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
        });
}
