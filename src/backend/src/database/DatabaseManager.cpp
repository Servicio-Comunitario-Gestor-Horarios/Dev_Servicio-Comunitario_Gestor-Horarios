#include "backend/database/DatabaseManager.hpp"
#include "backend/database/migracion.hpp"

#include <QDebug>
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

    if (!m_db.open())
    {
        qCritical() << "Error abriendo la base de datos:"
                    << m_db.lastError().text();

        return false;
    }

    QSqlQuery pragma(m_db);
    pragma.exec("PRAGMA foreign_keys = ON");

    if (!runMigrations())
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

bool DatabaseManager::runMigrations()
{
    return Migracion::runAll(m_db);
}
