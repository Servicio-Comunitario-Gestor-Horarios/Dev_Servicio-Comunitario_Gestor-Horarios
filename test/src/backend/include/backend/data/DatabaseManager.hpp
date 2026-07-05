#pragma once

#include <QSqlDatabase>
#include <QString>

/// Maneja la conexión SQLite y las migraciones del esquema.
///
/// Uso:
/// DatabaseManager db;
/// db.initialize("horarios.db");
/// db.database(); // conexión lista para usar
class DatabaseManager
{
public:

    /// Abre o crea la base de datos SQLite.
    /// Ejecuta automáticamente las migraciones.
    /// Retorna true si todo fue exitoso.
    bool initialize(const QString& dbPath);

    /// Cierra la conexión.
    void close();

    /// Indica si la base fue inicializada correctamente.
    bool isInitialized() const;

    /// Devuelve la conexión SQLite.
    QSqlDatabase& database();

private:

    /// Ejecuta las migraciones CREATE TABLE IF NOT EXISTS.
    bool runMigrations();

private:

    QSqlDatabase m_db;

    bool m_initialized = false;
};