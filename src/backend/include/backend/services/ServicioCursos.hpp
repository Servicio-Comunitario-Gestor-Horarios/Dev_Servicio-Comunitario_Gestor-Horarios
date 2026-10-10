#pragma once

/**
 * @file ServicioCursos.hpp
 * @brief Servicio para gestión de cursos (tabla Cursos y su relación con materias).
 */

#include <QSqlDatabase>
#include <QSqlRecord>
#include <QString>
#include <QVector>

#include "backend/resultado.hpp"

/// Materia asignada a un curso, con sus horas semanales.
struct CursoMateriaDTO {
    int idMateria = 0;
    QString nombreMateria;
    int horasSemanales = 0;
};

/// Representación persistente de un curso.
struct CursoDTO {
    int id = -1;
    QString nombre;
    QString turno;
    int aulaFija = -1;          ///< id del aula fija (o -1 si no tiene)
    int numEstudiantes = 0;
    QString codigoPlan;         ///< codigo del PlanEstudio asociado (o vacío)

    QVector<CursoMateriaDTO> materias;
};

/// Servicio CRUD sobre la tabla Cursos y la relación Curso_Materia (RF-2).
class ServicioCursos {
public:
    explicit ServicioCursos(QSqlDatabase& db);

    // ─── CRUD ─────────────────────────────────────────────────────────────

    Resultado<CursoDTO> crearCurso(const QString& nombre,
                                   const QString& turno = QString(),
                                   int aulaFija = -1,
                                   int numEstudiantes = 0,
                                   const QString& codigoPlan = QString());

    Resultado<CursoDTO> obtenerCurso(int id) const;

    Resultado<QVector<CursoDTO>> listarCursos() const;

    Resultado<CursoDTO> actualizarCurso(int id,
                                        const QString& nombre,
                                        const QString& turno = QString(),
                                        int aulaFija = -1,
                                        int numEstudiantes = 0,
                                        const QString& codigoPlan = QString());

    bool eliminarCurso(int id);

    // ─── Materias del curso (N:M) ─────────────────────────────────────────

    Resultado<CursoMateriaDTO> asignarMateria(int idCurso, int idMateria,
                                              int horasSemanales);

    bool quitarMateria(int idCurso, int idMateria);

private:
    QSqlDatabase& m_db;

    bool validarCurso(const QString& nombre, int numEstudiantes, QString& error) const;
    CursoDTO mapearARecord(const QSqlRecord& record) const;
    bool cargarMaterias(CursoDTO& dto) const;
};
