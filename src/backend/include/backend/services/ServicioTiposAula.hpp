#pragma once

/**
 * @file ServicioTiposAula.hpp
 * @brief Servicio para el catálogo de tipos de aula.
 *
 * El catálogo guarda la **clave normalizada** del nombre (recorte de espacios
 * extremos + plegado Unicode) y la tabla tiene un índice único sobre ella, de
 * modo que «Física» y «FÍSICA» son el mismo tipo aunque se escriba directo en
 * la base. Las aulas y materias referencian el catálogo por `id`, así que un
 * renombrado se propaga sin reescribir sus filas.
 */

#include <QSqlDatabase>
#include <QSqlRecord>
#include <QString>
#include <QVector>

#include <backend/resultado.hpp>

/**
 * @brief DTO (Data Transfer Object) para un tipo de aula.
 */
struct TipoAulaDTO {
    int id = -1;
    QString nombre;
};

/**
 * @brief Consultas de apoyo al catálogo, compartidas por los servicios que
 *        exigen un tipo de aula existente.
 */
namespace tipos_aula {
/** @brief Indica si el catálogo no tiene ninguna fila. */
bool catalogoVacio(QSqlDatabase& db);
/** @brief Indica si el id existe en el catálogo. */
bool existe(QSqlDatabase& db, int idTipoAula);
} // namespace tipos_aula

/**
 * @brief Servicio para operaciones CRUD sobre la tabla TiposAula.
 */
class ServicioTiposAula {
public:
    explicit ServicioTiposAula(QSqlDatabase& db);

    /**
     * @brief Clave normalizada del nombre: `trimmed().toCaseFolded()`.
     *
     * Solo sirve para comparar; el nombre visible se conserva tal cual (salvo
     * el recorte de espacios extremos).
     */
    static QString claveTipo(const QString& nombre);

    /** @brief Crea un tipo de aula; rechaza nombre vacío y clave duplicada. */
    Resultado<TipoAulaDTO> crearTipoAula(const QString& nombre);

    /** @brief Obtiene un tipo de aula por su ID. */
    Resultado<TipoAulaDTO> obtenerTipoAula(int id) const;

    /** @brief Lista el catálogo ordenado por nombre. Error y vacío se distinguen. */
    Resultado<QVector<TipoAulaDTO>> listarTiposAula() const;

    /** @brief Renombra un tipo existente; las aulas/materias lo ven por referencia. */
    Resultado<TipoAulaDTO> renombrarTipoAula(int id, const QString& nombre);

    /** @brief Elimina un tipo libre; error con los dependientes si está en uso. */
    Resultado<bool> eliminarTipoAula(int id);

private:
    QSqlDatabase& m_db;

    /** @brief Valida nombre/clave y busca duplicados salvo en `excluirId`. */
    bool validarNombreYClave(const QString& nombre, int excluirId, QString& error) const;

    /** @brief Nombres de las aulas o materias que referencian el tipo. */
    QVector<QString> dependientesDe(const QString& tabla, int id) const;

    /** @brief Mapea un QSqlRecord a TipoAulaDTO. */
    TipoAulaDTO mapearARecord(const QSqlRecord& record) const;
};
