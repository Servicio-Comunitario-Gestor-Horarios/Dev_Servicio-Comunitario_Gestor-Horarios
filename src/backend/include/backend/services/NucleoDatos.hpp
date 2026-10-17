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
#include "backend/services/GestorPendientes.hpp"
#include "backend/services/ServicioAula.hpp"
#include "backend/services/ServicioCargaHoraria.hpp"
#include "backend/services/ServicioComposicionPlan.hpp"
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

    Resultado<QVector<ProfesorDTO>> listarDocentes() const;
    Resultado<ProfesorDTO> crearDocente(const QString& id, const QString& nombre,
                                        const QString& email,
                                        const QString& telefono = QString());
    Resultado<ProfesorDTO> actualizarDocente(const QString& id, const QString& nombre,
                                             const QString& email,
                                             const QString& telefono = QString());
    bool eliminarDocente(const QString& id);

    // ─── Aulas ────────────────────────────────────────────────────────────

    Resultado<QVector<AulaDTO>> listarAulas() const;
    Resultado<AulaDTO> crearAula(const QString& nombre, int capacidad,
                                 const QString& edificio = QString(),
                                 const QString& piso = QString());
    Resultado<AulaDTO> actualizarAula(int id, const QString& nombre, int capacidad,
                                      const QString& edificio = QString(),
                                      const QString& piso = QString());
    bool eliminarAula(int id);

    // ─── Materias ─────────────────────────────────────────────────────────

    Resultado<QVector<MateriaDTO>> listarMaterias() const;
    Resultado<MateriaDTO> crearMateria(const QString& nombre,
                                       const QString& requisitos = QString());
    Resultado<MateriaDTO> actualizarMateria(int id, const QString& nombre,
                                            const QString& requisitos = QString());
    bool eliminarMateria(int id);

    // ─── Planes de estudio ────────────────────────────────────────────────

    Resultado<QVector<PlanDTO>> listarPlanes() const;
    Resultado<PlanDTO> crearPlan(const QString& codigo, const QString& nombre,
                                 const QString& descripcion = QString());
    Resultado<PlanDTO> actualizarPlan(const QString& codigo, const QString& nombre,
                                      const QString& descripcion = QString());
    bool eliminarPlan(const QString& codigo);

    /// Carga horaria (materias con sus horas y año) de un plan de estudio.
    ///
    /// La interfaz de generación la usa para derivar las materias de cada curso
    /// de la entrada del solver (RF-1). Devuelve error si el código está vacío;
    /// un plan sin materias devuelve un vector vacío (no es error).
    Resultado<QVector<CargaHorariaDTO>> listarCargaDePlan(const QString& codigoPlan) const;

    /// Compone un plan de estudio completo —materias con su año y horas— tal y
    /// como lo espera el solver. La interfaz de generación la usa para embeber
    /// (en la entrada) los planes referenciados por los cursos (RF-1).
    /// Devuelve error si el plan no existe o no tiene materias.
    Resultado<PlanEstudio> componerPlanCompleto(const QString& codigoPlan) const;

    // ─── Cursos ───────────────────────────────────────────────────────────

    Resultado<QVector<CursoDTO>> listarCursos() const;
    Resultado<CursoDTO> crearCurso(int anio, const QString& seccion,
                                   const QString& turno = QString(), int aula = -1);
    Resultado<CursoDTO> actualizarCurso(int id, int anio, const QString& seccion,
                                        const QString& turno = QString(), int aula = -1);
    bool eliminarCurso(int id);

    // ─── Turnos y recesos ─────────────────────────────────────────────────

    Resultado<QVector<TurnoDTO>> listarTurnos() const;
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

    // ─── Cambios pendientes y reintento (RF-3, RNF-3, RNF-4) ──────────────

    /// Gestor de cambios pendientes: estado por registro y guardia de cierre.
    GestorPendientes& gestorPendientes();
    const GestorPendientes& gestorPendientes() const;

    /// Indica si hay cambios pendientes de reintento (guardia de cierre, RF-6).
    bool hayPendientes() const;

    /**
     * @brief Reintenta el cambio pendiente `id`.
     *
     * Si tiene éxito, deja de estar pendiente; si vuelve a fallar, se conserva.
     */
    Resultado<bool> reintentarPendiente(qint64 id);

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
    ServicioCargaHoraria m_carga;
    ServicioComposicionPlan m_composicion;
    ServicioCursos m_cursos;
    ServicioTurnosRecesos m_turnos;

    /// Estado en memoria de las escrituras fallidas pendientes de reintento.
    GestorPendientes m_pendientes;
};
