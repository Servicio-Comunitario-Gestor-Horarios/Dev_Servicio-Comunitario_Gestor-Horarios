#include "backend/services/NucleoDatos.hpp"

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
