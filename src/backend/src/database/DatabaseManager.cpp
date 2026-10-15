#include "backend/database/DatabaseManager.hpp"
#include "backend/database/version_esquema.hpp"

#include <QDebug>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>

DatabaseManager::~DatabaseManager()
{
    close();
}

bool DatabaseManager::initialize(const QString& dbPath)
{
    if (m_initialized)
    {
        qWarning() << "DatabaseManager ya inicializado, cerrando conexión previa...";
        close();
    }

    m_db = QSqlDatabase::addDatabase("QSQLITE");

    m_db.setDatabaseName(dbPath);

    // Se comprueba antes de abrir porque SQLite crea el archivo al abrirlo.
    const bool existia = QFile::exists(dbPath);

    if (!m_db.open())
    {
        qCritical() << "Error abriendo la base de datos:"
                    << m_db.lastError().text();

        return false;
    }

    QSqlQuery pragma(m_db);
    pragma.exec("PRAGMA foreign_keys = ON");

    if (!runMigrations(existia))
    {
        qCritical() << "Error ejecutando migraciones.";

        close();

        return false;
    }

    m_initialized = true;

    return true;
}

void DatabaseManager::close()
{
    if (m_db.isOpen())
    {
        m_db.close();
    }

    // Libera la conexión registrada para permitir re-inicialización
    if (QSqlDatabase::contains(QSqlDatabase::defaultConnection))
    {
        QSqlDatabase::removeDatabase(QSqlDatabase::defaultConnection);
    }

    m_initialized = false;
}

bool DatabaseManager::isInitialized() const
{
    return m_initialized;
}

QSqlDatabase& DatabaseManager::database()
{
    return m_db;
}

const QSqlDatabase& DatabaseManager::database() const
{
    return m_db;
}

bool DatabaseManager::runMigrations(bool existia)
{
    const int versionArchivo = VersionEsquema::leerVersionEsquema(m_db);

    const VersionEsquema::EstadoApertura decision =
        VersionEsquema::decidirApertura(existia,
                                        versionArchivo,
                                        VersionEsquema::VERSION_ESQUEMA_ACTUAL);

    switch (decision)
    {
        case VersionEsquema::EstadoApertura::Crear:
            return VersionEsquema::crearEsquema(m_db);

        case VersionEsquema::EstadoApertura::Abrir:
            // La version coincide; el catalogo debe corresponder al esquema unico.
            return VersionEsquema::esquemaEsperado(m_db);

        case VersionEsquema::EstadoApertura::FalloEsquema:
            qCritical() << "El esquema de la base no corresponde al esperado (version"
                        << versionArchivo << "); no se migra.";
            return false;
    }

    return false;
}
