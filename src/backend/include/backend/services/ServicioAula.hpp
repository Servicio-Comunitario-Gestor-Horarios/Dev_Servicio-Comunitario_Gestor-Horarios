#pragma once

/**
 * @file ServicioAula.hpp
 * @brief Servicio para gestión de aulas.
 */

#include <QSqlDatabase>
#include <QVector>
#include <backend/resultado.hpp>
#include <backend/data/aula.hpp>

/**
 * @brief DTO (Data Transfer Object) para un aula.
 */
struct AulaDTO {
    int id = -1;
    QString nombre;
    int capacidad = 0;
    QString edificio;
    QString piso;

    /** @brief Convierte el DTO a un struct Aula del solver. */
    Aula toAula() const;
};

/**
 * @brief Servicio para operaciones CRUD sobre la tabla Aulas.
 */
class ServicioAula {
public:
    explicit ServicioAula(QSqlDatabase& db);

    // ─── CRUD ─────────────────────────────────────────────────────────────

    /** @brief Crea un nuevo aula en la base de datos. */
    Resultado<AulaDTO> crearAula(const QString& nombre, int capacidad,
                                 const QString& edificio = QString(),
                                 const QString& piso = QString());

    /** @brief Obtiene un aula por su ID. */
    Resultado<AulaDTO> obtenerAula(int id) const;

    /** @brief Lista todas las aulas registradas. */
    QVector<AulaDTO> listarAulas() const;

    /** @brief Actualiza los datos de un aula existente. */
    Resultado<AulaDTO> actualizarAula(int id, const QString& nombre, int capacidad,
                                      const QString& edificio = QString(),
                                      const QString& piso = QString());

    /** @brief Elimina un aula por su ID. */
    bool eliminarAula(int id);

    // ─── Extracción para el solver ────────────────────────────────────────

    /** @brief Obtiene todos los structs Aula para el solver. */
    QVector<Aula> obtenerTodasParaSolver() const;

    /** @brief Obtiene un struct Aula del solver por ID. */
    Resultado<Aula> obtenerAulaParaSolver(int id) const;

private:
    QSqlDatabase& m_db;

    /** @brief Valida los datos de un aula. */
    bool validarAula(const QString& nombre, int capacidad, QString& error) const;

    /** @brief Mapea un QSqlRecord a AulaDTO. */
    AulaDTO mapearARecord(const QSqlRecord& record) const;
};