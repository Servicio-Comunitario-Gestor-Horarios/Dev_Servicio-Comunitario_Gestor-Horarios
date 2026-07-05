#include "backend/database_manager.hpp"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

bool DatabaseManager::initialize(const QString& dbPath)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");

    m_db.setDatabaseName(dbPath);

    if (!m_db.open())
    {
        qCritical() << "Error abriendo la base de datos:"
                    << m_db.lastError().text();

        return false;
    }

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

bool DatabaseManager::runMigrations()
{
    QSqlQuery query(m_db);

    if (!query.exec(

        "CREATE TABLE IF NOT EXISTS teachers ("

        "id INTEGER PRIMARY KEY AUTOINCREMENT,"

        "name TEXT NOT NULL,"

        "email TEXT NOT NULL UNIQUE,"

        "phone TEXT,"

        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"

        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"

        ")"

    ))
    {
        qCritical() << "Error creando tabla teachers:"
                    << query.lastError().text();

        return false;
    }

    if (!query.exec(

        "CREATE TABLE IF NOT EXISTS classrooms ("

        "id INTEGER PRIMARY KEY AUTOINCREMENT,"

        "name TEXT NOT NULL UNIQUE,"

        "capacity INTEGER NOT NULL,"

        "building TEXT,"

        "floor INTEGER,"

        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"

        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"

        ")"

    ))
    {
        qCritical() << "Error creando tabla classrooms:"
                    << query.lastError().text();

        return false;
    }

    return true;
}