#pragma once

#include <QSqlDatabase>
#include <QSqlRecord>
#include <QString>
#include <QVector>

#include "backend/resultado.hpp"        // ← ajusta si tu ruta difiere
#include "backend/data/profesor.hpp"
#include "backend/data/franja_horaria.hpp"

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
    QVector<MateriaAsignadaDTO> materias;

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

    QVector<ProfesorDTO>   listarProfesores() const;

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

    // Materias (N:M)
    bool asignarMateria(const QString& idProfesor, int idMateria);
    bool asignarMateriaPorNombre(const QString& idProfesor, const QString& nombreMateria);
    bool quitarMateria(const QString& idProfesor, int idMateria);
    bool limpiarMaterias(const QString& idProfesor);

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
    bool cargarMaterias(ProfesorDTO& dto) const;
};
