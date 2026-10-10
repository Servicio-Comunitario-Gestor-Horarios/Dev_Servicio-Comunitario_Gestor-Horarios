#pragma once

/**
 * @file NucleoDatos.hpp
 * @brief Fachada única de acceso a los dominios de datos del cliente (RF-2).
 *
 * Envuelve los servicios de dominio sobre una misma conexión `QSqlDatabase` y
 * expone el CRUD que consume la interfaz (docentes, aulas, materias, planes de
 * estudio, cursos y turnos/recesos). No reimplementa la lógica de los servicios:
 * los delega.
 */

#include <QSqlDatabase>
#include <QString>
#include <QTime>
#include <QVector>

#include "backend/resultado.hpp"
#include "backend/services/ServicioAula.hpp"
#include "backend/services/ServicioCursos.hpp"
#include "backend/services/ServicioMaterias.hpp"
#include "backend/services/ServicioPlanesEstudio.hpp"
#include "backend/services/ServicioProfesor.hpp"
#include "backend/services/ServicioTurnosRecesos.hpp"
#include "backend/services/cascada.hpp"

class NucleoDatos {
public:
    explicit NucleoDatos(QSqlDatabase& db);

    // ─── Docentes ─────────────────────────────────────────────────────────

    QVector<ProfesorDTO> listarDocentes() const;
    Resultado<ProfesorDTO> crearDocente(const QString& id, const QString& nombre,
                                        const QString& email,
                                        const QString& telefono = QString());
    Resultado<ProfesorDTO> actualizarDocente(const QString& id, const QString& nombre,
                                             const QString& email,
                                             const QString& telefono = QString());
    bool eliminarDocente(const QString& id);

    // ─── Aulas ────────────────────────────────────────────────────────────

    QVector<AulaDTO> listarAulas() const;
    Resultado<AulaDTO> crearAula(const QString& nombre, int capacidad,
                                 const QString& edificio = QString(),
                                 const QString& piso = QString());
    Resultado<AulaDTO> actualizarAula(int id, const QString& nombre, int capacidad,
                                      const QString& edificio = QString(),
                                      const QString& piso = QString());
    bool eliminarAula(int id);

    // ─── Materias ─────────────────────────────────────────────────────────

    QVector<MateriaDTO> listarMaterias() const;
    Resultado<MateriaDTO> crearMateria(const QString& nombre,
                                       const QString& requisitos = QString());
    Resultado<MateriaDTO> actualizarMateria(int id, const QString& nombre,
                                            const QString& requisitos = QString());
    bool eliminarMateria(int id);

    // ─── Planes de estudio ────────────────────────────────────────────────

    QVector<PlanDTO> listarPlanes() const;
    Resultado<PlanDTO> crearPlan(const QString& codigo, const QString& nombre,
                                 const QString& descripcion = QString());
    Resultado<PlanDTO> actualizarPlan(const QString& codigo, const QString& nombre,
                                      const QString& descripcion = QString());
    bool eliminarPlan(const QString& codigo);

    // ─── Cursos ───────────────────────────────────────────────────────────

    QVector<CursoDTO> listarCursos() const;
    Resultado<CursoDTO> crearCurso(const QString& nombre,
                                   const QString& turno = QString(),
                                   int aulaFija = -1, int numEstudiantes = 0,
                                   const QString& codigoPlan = QString());
    Resultado<CursoDTO> actualizarCurso(int id, const QString& nombre,
                                        const QString& turno = QString(),
                                        int aulaFija = -1, int numEstudiantes = 0,
                                        const QString& codigoPlan = QString());
    bool eliminarCurso(int id);
    Resultado<CursoMateriaDTO> asignarMateriaACurso(int idCurso, int idMateria,
                                                    int horasSemanales);
    bool quitarMateriaDeCurso(int idCurso, int idMateria);

    // ─── Turnos y recesos ─────────────────────────────────────────────────

    QVector<TurnoDTO> listarTurnos() const;
    Resultado<TurnoDTO> crearTurno(const QString& nombre, const QTime& inicio,
                                   const QTime& fin, int numSlots);
    Resultado<TurnoDTO> actualizarTurno(const QString& nombre, const QTime& inicio,
                                        const QTime& fin, int numSlots);
    bool eliminarTurno(const QString& nombre);
    Resultado<RecesoDTO> agregarReceso(const QString& turno, int despuesDeSlot,
                                       int duracion, const QTime& inicio = QTime(),
                                       const QTime& fin = QTime());
    bool eliminarReceso(const QString& turno, int despuesDeSlot);

    // ─── Eliminación en cascada (RF-2) ────────────────────────────────────

    /**
     * @brief Lista los registros que se eliminarían en cascada.
     *
     * El frontend la usa para mostrar los dependientes y pedir confirmación
     * explícita antes de continuar. No modifica ningún dato.
     */
    QVector<Dependencia> dependientesDe(const QString& dominio, const QString& id) const;

    /**
     * @brief Elimina `dominio`/`id` y sus dependientes de forma atómica.
     *
     * Todo ocurre en una sola transacción: si cualquier borrado falla, se
     * revierte y no queda ninguna parte aplicada (RF-2).
     */
    bool eliminarConCascada(const QString& dominio, const QString& id);

private:
    QSqlDatabase& m_db;

    /// Consulta recursiva de los dependientes de `dominio`/`id`.
    QVector<Dependencia> recolectarDependientes(const QString& dominio,
                                                const QString& id) const;

    /// Borra `dominio`/`id` y sus dependientes dentro de la transacción activa.
    bool borrarRecursivo(const QString& dominio, const QString& id);

    ServicioProfesor m_profesor;
    ServicioAula m_aula;
    ServicioMaterias m_materias;
    ServicioPlanesEstudio m_planes;
    ServicioCursos m_cursos;
    ServicioTurnosRecesos m_turnos;
};
