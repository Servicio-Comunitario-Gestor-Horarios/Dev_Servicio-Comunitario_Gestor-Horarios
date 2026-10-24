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
        QStringLiteral("TiposAula"),    QStringLiteral("Eventos"),
        QStringLiteral("Cursos")
    };

    /// Tablas que NO deben existir (retiradas en el modelo nuevo).
    const QStringList TABLAS_RETIRADAS = {
        QStringLiteral("Curso_Materia"), QStringLiteral("Recesos")
    };

    /// Columnas que debe tener la tabla `Cursos`.
    const QStringList COLUMNAS_CURSOS = {
        QStringLiteral("anio"), QStringLiteral("seccion"),
        QStringLiteral("turno"), QStringLiteral("aula")
    };

    /// Columnas que debe tener la tabla `Aulas` (el tipo es obligatorio).
    const QStringList COLUMNAS_AULAS = {
        QStringLiteral("nombre"), QStringLiteral("capacidad"),
        QStringLiteral("edificio"), QStringLiteral("piso"),
        QStringLiteral("id_tipo_aula")
    };

    /// Columnas que debe tener la tabla `Materias` (el tipo es obligatorio).
    const QStringList COLUMNAS_MATERIAS = {
        QStringLiteral("nombre"), QStringLiteral("requisitos"),
        QStringLiteral("id_tipo_aula")
    };

    /// Columnas que debe tener la tabla `Eventos` (identidad y orden propios).
    const QStringList COLUMNAS_EVENTOS = {
        QStringLiteral("id"), QStringLiteral("nombre"), QStringLiteral("turno"),
        QStringLiteral("despues_de_slot"), QStringLiteral("orden"),
        QStringLiteral("duracion"), QStringLiteral("inicio"), QStringLiteral("fin")
    };

    /// Columnas que debe tener la tabla `TiposAula` (clave normalizada almacenada).
    const QStringList COLUMNAS_TIPOS = {
        QStringLiteral("id"), QStringLiteral("nombre"), QStringLiteral("clave")
    };

    /// Tablas del catálogo con sus columnas obligatorias.
    struct TablaConColumnas
    {
        const char* tabla;
        const QStringList* columnas;
    };

    const TablaConColumnas TABLAS_CON_COLUMNAS[] = {
        { "Cursos", &COLUMNAS_CURSOS },
        { "Aulas", &COLUMNAS_AULAS },
        { "Materias", &COLUMNAS_MATERIAS },
        { "Eventos", &COLUMNAS_EVENTOS },
        { "TiposAula", &COLUMNAS_TIPOS },
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

    // Columnas que faltan, agrupadas por tabla (el motivo nombra tabla y columna).
    QStringList piezasColumnas;
    for (const TablaConColumnas& entrada : TABLAS_CON_COLUMNAS)
    {
        const QString tabla = QString::fromLatin1(entrada.tabla);
        if (!tablaExiste(db, tabla))
            continue; // ya figura como tabla faltante

        QStringList columnasFaltantes;
        for (const QString& columna : *entrada.columnas)
            if (!columnaExiste(db, tabla, columna))
                columnasFaltantes << columna;

        if (!columnasFaltantes.isEmpty())
            piezasColumnas << QStringLiteral("faltan las columnas «%1» de la tabla «%2»")
                                  .arg(columnasFaltantes.join(QStringLiteral("», «")), tabla);
    }

    QStringList retiradasPresentes;
    for (const QString& retirada : TABLAS_RETIRADAS)
        if (tablaExiste(db, retirada))
            retiradasPresentes << retirada;

    if (faltantes.isEmpty() && piezasColumnas.isEmpty() && retiradasPresentes.isEmpty())
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

        piezas << piezasColumnas;

        if (!retiradasPresentes.isEmpty())
            piezas << QStringLiteral("existe la tabla retirada «%1»")
                          .arg(retiradasPresentes.join(QStringLiteral("», «")));

        *motivo = piezas.join(QStringLiteral("; "));
    }

    return false;
}
