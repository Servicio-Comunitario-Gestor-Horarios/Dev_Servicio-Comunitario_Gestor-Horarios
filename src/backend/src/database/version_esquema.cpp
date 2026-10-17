#include "backend/database/version_esquema.hpp"

#include "backend/database/migracion.hpp"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QStringList>

namespace
{
    /// Tablas que debe tener el esquema único.
    const QStringList TABLAS_ESPERADAS = {
        QStringLiteral("Aulas"),        QStringLiteral("Profesores"),
        QStringLiteral("Materias"),     QStringLiteral("Profesor_Materia"),
        QStringLiteral("Disponibilidad_Profesor"), QStringLiteral("PlanEstudio"),
        QStringLiteral("PlanEstudio_Materia"),     QStringLiteral("Turnos"),
        QStringLiteral("Recesos"),      QStringLiteral("Cursos")
    };

    /// Tablas que NO deben existir (retiradas en el modelo nuevo).
    const QStringList TABLAS_RETIRADAS = { QStringLiteral("Curso_Materia") };

    /// Columnas que debe tener la tabla `Cursos`.
    const QStringList COLUMNAS_CURSOS = {
        QStringLiteral("anio"), QStringLiteral("seccion"),
        QStringLiteral("turno"), QStringLiteral("aula")
    };

    bool tablaExiste(QSqlDatabase& db, const QString& tabla)
    {
        QSqlQuery query(db);
        query.prepare(QStringLiteral("SELECT name FROM sqlite_master "
                                     "WHERE type='table' AND name=:nombre"));
        query.bindValue(QStringLiteral(":nombre"), tabla);
        return query.exec() && query.next();
    }

    bool columnaExiste(QSqlDatabase& db, const QString& tabla, const QString& columna)
    {
        QSqlQuery query(db);
        // `PRAGMA table_info` no admite parámetros enlazados; el nombre de tabla es
        // una constante interna, por lo que la interpolación no supone inyección.
        if (!query.exec(QStringLiteral("PRAGMA table_info(%1)").arg(tabla)))
            return false;

        while (query.next())
            if (query.value(1).toString().compare(columna, Qt::CaseInsensitive) == 0)
                return true;

        return false;
    }
} // namespace

VersionEsquema::EstadoApertura VersionEsquema::decidirApertura(bool existe,
                                                               int versionArchivo,
                                                               int versionEsperada)
{
    if (!existe)
        return EstadoApertura::Crear;

    if (versionArchivo == versionEsperada)
        return EstadoApertura::Abrir;

    // Ausente (<=0), anterior o posterior: todas se tratan como esquema no esperado.
    return EstadoApertura::FalloEsquema;
}

int VersionEsquema::leerVersionEsquema(QSqlDatabase& db)
{
    QSqlQuery query(db);

    if (!query.exec(QStringLiteral("PRAGMA user_version")))
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
    if (!query.exec(QStringLiteral("PRAGMA user_version = %1").arg(version)))
    {
        qCritical() << "No se pudo fijar la version de esquema:" << query.lastError().text();
        return false;
    }

    return true;
}

bool VersionEsquema::crearEsquema(QSqlDatabase& db)
{
    if (!db.isOpen())
    {
        qCritical() << "No se puede crear el esquema: la base de datos no esta abierta.";
        return false;
    }

    if (!db.transaction())
    {
        qCritical() << "No se pudo iniciar la transaccion de creacion del esquema:"
                    << db.lastError().text();
        return false;
    }

    const bool ok = Migracion::aplicarPaso(db)
                 && fijarVersionEsquema(db, VERSION_ESQUEMA_ACTUAL);

    if (!ok)
    {
        db.rollback();
        qCritical() << "La creacion del esquema fallo; se revierte.";
        return false;
    }

    if (!db.commit())
    {
        db.rollback();
        qCritical() << "No se pudo confirmar la creacion del esquema:" << db.lastError().text();
        return false;
    }

    return true;
}

bool VersionEsquema::esquemaEsperado(QSqlDatabase& db, QString* motivo)
{
    QStringList faltantes;

    for (const QString& tabla : TABLAS_ESPERADAS)
        if (!tablaExiste(db, tabla))
            faltantes << tabla;

    QStringList columnasFaltantes;
    if (tablaExiste(db, QStringLiteral("Cursos")))
    {
        for (const QString& columna : COLUMNAS_CURSOS)
            if (!columnaExiste(db, QStringLiteral("Cursos"), columna))
                columnasFaltantes << columna;
    }

    QStringList retiradasPresentes;
    for (const QString& retirada : TABLAS_RETIRADAS)
        if (tablaExiste(db, retirada))
            retiradasPresentes << retirada;

    if (faltantes.isEmpty() && columnasFaltantes.isEmpty() && retiradasPresentes.isEmpty())
    {
        if (motivo)
            motivo->clear();
        return true;
    }

    if (motivo)
    {
        QStringList piezas;

        if (!faltantes.isEmpty())
            piezas << QStringLiteral("faltan las tablas «%1»")
                          .arg(faltantes.join(QStringLiteral("», «")));

        if (!columnasFaltantes.isEmpty())
            piezas << QStringLiteral("faltan las columnas «%1» de la tabla «Cursos»")
                          .arg(columnasFaltantes.join(QStringLiteral("», «")));

        if (!retiradasPresentes.isEmpty())
            piezas << QStringLiteral("existe la tabla retirada «%1»")
                          .arg(retiradasPresentes.join(QStringLiteral("», «")));

        *motivo = piezas.join(QStringLiteral("; "));
    }

    return false;
}
