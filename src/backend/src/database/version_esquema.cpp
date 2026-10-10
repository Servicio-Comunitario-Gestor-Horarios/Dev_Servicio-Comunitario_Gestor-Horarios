#include "backend/database/version_esquema.hpp"

#include "backend/database/migracion.hpp"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>

VersionEsquema::EstadoApertura VersionEsquema::decidirApertura(bool existe,
                                                               int versionArchivo,
                                                               int versionEsperada)
{
    // Sin archivo: hay que crear el esquema inicial.
    if (!existe)
        return EstadoApertura::Crear;

    // Versión ausente o no interpretable (SQLite devuelve 0 por defecto).
    if (versionArchivo <= 0)
        return EstadoApertura::FalloAusente;

    if (versionArchivo < versionEsperada)
        return EstadoApertura::Migrar;

    if (versionArchivo == versionEsperada)
        return EstadoApertura::Abrir;

    // Cualquier versión posterior se trata como fallo de apertura.
    return EstadoApertura::FalloPosterior;
}

int VersionEsquema::leerVersionEsquema(QSqlDatabase& db)
{
    QSqlQuery query(db);

    if (!query.exec("PRAGMA user_version"))
    {
        qCritical() << "No se pudo leer la version de esquema:" << query.lastError().text();
        return -1;
    }

    if (!query.next())
        return -1;

    return query.value(0).toInt();
}

bool VersionEsquema::fijarVersionEsquema(QSqlDatabase& db, int version)
{
    QSqlQuery query(db);

    // `PRAGMA user_version` no admite parámetros enlazados; `version` es un int,
    // por lo que la interpolación no supone riesgo de inyección.
    if (!query.exec(QString("PRAGMA user_version = %1").arg(version)))
    {
        qCritical() << "No se pudo fijar la version de esquema:" << query.lastError().text();
        return false;
    }

    return true;
}

bool VersionEsquema::aplicarHasta(QSqlDatabase& db,
                                  int versionDestino,
                                  const PasoMigracion& paso)
{
    if (!db.isOpen())
    {
        qCritical() << "No se puede migrar: la base de datos no esta abierta.";
        return false;
    }

    const int versionActual = leerVersionEsquema(db);

    if (versionActual < 0)
        return false;

    // Ya estamos en la versión destino (o por delante): nada que aplicar.
    if (versionActual >= versionDestino)
        return true;

    for (int destino = versionActual + 1; destino <= versionDestino; ++destino)
    {
        if (!db.transaction())
        {
            qCritical() << "No se pudo iniciar la transaccion de migracion:"
                        << db.lastError().text();
            return false;
        }

        const bool ok = paso(db, destino) && fijarVersionEsquema(db, destino);

        if (!ok)
        {
            db.rollback();
            qCritical() << "Migracion a la version" << destino
                        << "fallida; se revierte a la version" << versionActual << ".";
            return false;
        }

        if (!db.commit())
        {
            db.rollback();
            qCritical() << "No se pudo confirmar la migracion a la version"
                        << destino << ":" << db.lastError().text();
            return false;
        }
    }

    return true;
}

bool VersionEsquema::aplicarHasta(QSqlDatabase& db, int versionDestino)
{
    const PasoMigracion paso = [](QSqlDatabase& conexion, int destino) -> bool {
        return Migracion::aplicarPaso(conexion, destino);
    };

    return aplicarHasta(db, versionDestino, paso);
}
