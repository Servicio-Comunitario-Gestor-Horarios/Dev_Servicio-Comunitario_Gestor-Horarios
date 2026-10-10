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

    ~DatabaseManager();

    /// Abre o crea la base de datos SQLite.
    /// Ejecuta automáticamente las migraciones.
    /// Retorna true si todo fue exitoso.
    bool initialize(const QString& dbPath);

    /// Cierra la conexión y libera recursos.
    void close();

    /// Indica si la base fue inicializada correctamente.
    bool isInitialized() const;

    /// Devuelve la conexión SQLite.
    QSqlDatabase& database();

    /// Devuelve la conexión SQLite (const).
    const QSqlDatabase& database() const;

private:

    /// Aplica las migraciones versionadas hasta la versión de esquema actual.
    /// `existia` indica si el archivo ya existía antes de abrir la conexión.
    bool runMigrations(bool existia);

private:

    QSqlDatabase m_db;

    bool m_initialized = false;
};
