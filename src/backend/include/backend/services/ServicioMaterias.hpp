#pragma once

/**
 * @file ServicioMaterias.hpp
 * @brief Servicio para gestión de materias (tabla Materias existente).
 */

#include <QSqlDatabase>
#include <QVector>
#include <backend/resultado.hpp>
#include <backend/data/materia.hpp>

/**
 * @brief DTO para una materia (tabla Materias existente).
 */
struct MateriaDTO {
    int id = -1;
    QString nombre;
    QString requisitos;  // Texto plano, separado por comas

    /** @brief Convierte el DTO a un struct Materia del solver. */
    Materia toMateria(int horasSemanales = 0) const;

    /** @brief Obtiene los requisitos como vector. */
    QVector<QString> requisitosComoVector() const;
};

/**
 * @brief Servicio para operaciones CRUD sobre la tabla Materias existente.
 */
class ServicioMaterias {
public:
    explicit ServicioMaterias(QSqlDatabase& db);

    // ─── CRUD ─────────────────────────────────────────────────────────────

    /** @brief Crea una nueva materia. */
    Resultado<MateriaDTO> crearMateria(const QString& nombre, const QString& requisitos = QString());

    /** @brief Obtiene una materia por ID. */
    Resultado<MateriaDTO> obtenerMateria(int id) const;

    /** @brief Lista todas las materias. */
    QVector<MateriaDTO> listarMaterias() const;

    /** @brief Actualiza los datos de una materia. */
    Resultado<MateriaDTO> actualizarMateria(int id, const QString& nombre, const QString& requisitos = QString());

    /** @brief Elimina una materia por ID. */
    bool eliminarMateria(int id);

    // ─── Extracción para el solver ────────────────────────────────────────

    /** @brief Obtiene todas las materias como structs del solver. */
    QVector<Materia> obtenerTodasParaSolver() const;

    /** @brief Obtiene una materia del solver por ID. */
    Resultado<Materia> obtenerMateriaParaSolver(int id) const;

    /** @brief Obtiene materia del solver con carga horaria específica. */
    Resultado<Materia> obtenerMateriaParaSolver(int id, int horasSemanales) const;

private:
    QSqlDatabase& m_db;

    bool validarMateria(const QString& nombre, QString& error) const;
    MateriaDTO mapearARecord(const QSqlRecord& record) const;
};