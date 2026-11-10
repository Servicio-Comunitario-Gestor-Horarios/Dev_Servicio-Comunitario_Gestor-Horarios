#pragma once

#include <QSqlDatabase>
#include <QSqlRecord>
#include <QString>
#include <QVector>

#include "backend/resultado.hpp"        // ← ajusta si tu ruta difiere
#include "backend/data/profesor.hpp"
#include "backend/data/franja_horaria.hpp"
#include "backend/services/ServicioPreferencias.hpp"

// ─── DTOs ──────────────────────────────────────────────────────────────────

/// Disponibilidad persistida: id de fila + franja horaria.
struct DisponibilidadDTO {
    int id = 0;                  ///< id en la tabla Disponibilidad_Profesor
    FranjaHoraria franja;
};

/// Materia asignada al profesor (id + nombre).
struct MateriaAsignadaDTO {
    int id = 0;                  ///< id_Materia en la tabla Materias
    QString nombre;
};

/// Representación persistente de un Profesor.
struct ProfesorDTO {
    QString id;
    QString nombre;
    QString email;
    QString telefono;

    QVector<DisponibilidadDTO>  disponibilidad;
    /// Preferencias horarias (poder frente a querer): lista propia y ordenada por día,
    /// inicio, fin e id. No entran en el modelo del solver (`toProfesor` las ignora).
    QVector<PreferenciaDTO>     preferencias;
    QVector<MateriaAsignadaDTO> materias;
    /// Materias en las que el docente puede actuar como suplente. Se guardan en
    /// una tabla auxiliar creada bajo demanda para no alterar el esquema versionado.
    QVector<MateriaAsignadaDTO> materiasSuplentes;

    Profesor toProfesor() const;
};

// ─── Servicio ──────────────────────────────────────────────────────────────

class ServicioProfesor {
public:
    explicit ServicioProfesor(QSqlDatabase& db);

    // CRUD principal
    Resultado<ProfesorDTO> crearProfesor(const QString& id,
                                         const QString& nombre,
                                         const QString& email,
                                         const QString& telefono = QString());

    Resultado<ProfesorDTO> obtenerProfesor(const QString& id) const;

    Resultado<QVector<ProfesorDTO>> listarProfesores() const;

    Resultado<ProfesorDTO> actualizarProfesor(const QString& id,
                                              const QString& nombre,
                                              const QString& email,
                                              const QString& telefono);

    bool eliminarProfesor(const QString& id);

    // Disponibilidad (1:N)
    Resultado<DisponibilidadDTO> agregarDisponibilidad(const QString& idProfesor,
                                                       const FranjaHoraria& franja);
    bool eliminarDisponibilidad(int idDisponibilidad);
    bool limpiarDisponibilidad(const QString& idProfesor);

    /// Reemplaza todas las franjas del docente de una vez.
    ///
    /// Valida cada franja antes de escribir. No abre transacción: está pensada
    /// para ejecutarse dentro de la transacción de `NucleoDatos::guardarFichaDocente`.
    bool reemplazarDisponibilidad(const QString& idProfesor,
                                  const QVector<FranjaHoraria>& franjas,
                                  QString& error);

    // Materias (N:M)
    bool asignarMateria(const QString& idProfesor, int idMateria);
    bool asignarMateriaPorNombre(const QString& idProfesor, const QString& nombreMateria);
    bool quitarMateria(const QString& idProfesor, int idMateria);
    bool limpiarMaterias(const QString& idProfesor);

    /// Reemplaza las materias titulares del docente (transacción del llamador).
    bool reemplazarMaterias(const QString& idProfesor,
                            const QVector<int>& idsMaterias, QString& error);

    // Materias suplentes (tabla auxiliar)
    bool asignarMateriaSuplente(const QString& idProfesor, int idMateria);
    bool quitarMateriaSuplente(const QString& idProfesor, int idMateria);
    bool limpiarMateriasSuplentes(const QString& idProfesor);

    /// Reemplaza las materias suplentes del docente (transacción del llamador).
    bool reemplazarMateriasSuplentes(const QString& idProfesor,
                                     const QVector<int>& idsMaterias, QString& error);

    // Extracción para solver
    QVector<Profesor>   obtenerTodosParaSolver() const;
    Resultado<Profesor> obtenerProfesorParaSolver(const QString& id) const;

private:
    QSqlDatabase& m_db;

    bool validarProfesor(const QString& id,
                         const QString& nombre,
                         const QString& email,
                         QString& error) const;

    ProfesorDTO mapearARecord(const QSqlRecord& record) const;

    bool cargarDisponibilidad(ProfesorDTO& dto) const;
    bool cargarPreferencias(ProfesorDTO& dto) const;
    bool cargarMaterias(ProfesorDTO& dto) const;

    /// Crea la tabla auxiliar de materias suplentes si no existe. Se mantiene
    /// fuera de `Migracion` para no alterar la huella del esquema versionado:
    /// es una tabla opcional y compatible con bases de la versión actual.
    bool asegurarTablaSuplentes() const;
};
