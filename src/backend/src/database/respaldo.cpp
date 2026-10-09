#include "backend/database/respaldo.hpp"

#include "backend/database/version_esquema.hpp"

#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QVariant>

namespace
{
    /// Asigna el motivo del fallo si el llamador pidió el detalle.
    void reportar(QString* error, const QString& motivo)
    {
        if (error)
            *error = motivo;
    }

    /// Escapa comillas simples para interpolar una ruta en una sentencia SQL.
    /// `VACUUM INTO` no admite parámetros enlazados para el nombre de archivo.
    QString escaparComillas(const QString& texto)
    {
        QString resultado = texto;
        resultado.replace(QLatin1Char('\''), QLatin1String("''"));
        return resultado;
    }
} // namespace

QString Respaldo::construirNombreRespaldo(const QString& rutaDb, const QDateTime& ahora)
{
    return QStringLiteral("%1.respaldo-%2")
        .arg(rutaDb, ahora.toString(QStringLiteral("yyyyMMdd-hhmmss")));
}

bool Respaldo::crearRespaldo(QSqlDatabase& db, const QString& rutaRespaldo, QString* error)
{
    if (!db.isOpen())
    {
        reportar(error,
                 QStringLiteral("No se puede respaldar: la base de datos no está abierta."));
        return false;
    }

    if (rutaRespaldo.isEmpty())
    {
        reportar(error,
                 QStringLiteral("No se puede respaldar: no se indicó la ruta del respaldo."));
        return false;
    }

    // `VACUUM INTO` genera un snapshot consistente; el destino no debe existir.
    QSqlQuery query(db);
    const QString sql =
        QStringLiteral("VACUUM INTO '%1'").arg(escaparComillas(rutaRespaldo));

    if (!query.exec(sql))
    {
        reportar(error,
                 QStringLiteral("No se pudo crear el respaldo en «%1»: %2")
                     .arg(rutaRespaldo, query.lastError().text()));
        return false;
    }

    return true;
}

bool Respaldo::validarRespaldo(const QString& rutaRespaldo, QString* error)
{
    if (rutaRespaldo.isEmpty() || !QFile::exists(rutaRespaldo))
    {
        reportar(error,
                 QStringLiteral("El respaldo «%1» no existe.").arg(rutaRespaldo));
        return false;
    }

    const QString nombreConexion =
        QStringLiteral("respaldo_validar_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    bool abierta = false;
    bool integridadOk = false;
    QString detalle;
    int version = -1;

    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), nombreConexion);
        db.setDatabaseName(rutaRespaldo);
        abierta = db.open();

        if (abierta)
        {
            {
                QSqlQuery integridad(db);

                if (integridad.exec(QStringLiteral("PRAGMA integrity_check")) && integridad.next())
                {
                    detalle = integridad.value(0).toString();
                    integridadOk = (detalle == QStringLiteral("ok"));
                }
                else
                {
                    detalle = integridad.lastError().text();
                }
            }

            if (integridadOk)
                version = VersionEsquema::leerVersionEsquema(db);

            db.close();
        }
        else
        {
            detalle = db.lastError().text();
        }

        db = QSqlDatabase();
    }

    QSqlDatabase::removeDatabase(nombreConexion);

    if (!abierta)
    {
        reportar(error,
                 QStringLiteral("No se pudo abrir el respaldo «%1»: %2")
                     .arg(rutaRespaldo, detalle));
        return false;
    }

    if (!integridadOk)
    {
        reportar(error,
                 QStringLiteral("El respaldo «%1» está dañado o no es una base de datos válida: %2")
                     .arg(rutaRespaldo, detalle));
        return false;
    }

    if (version <= 0 || version > VersionEsquema::VERSION_ESQUEMA_ACTUAL)
    {
        reportar(error,
                 QStringLiteral("La versión de esquema del respaldo «%1» (%2) no es compatible"
                                " con la esperada (%3).")
                     .arg(rutaRespaldo)
                     .arg(version)
                     .arg(VersionEsquema::VERSION_ESQUEMA_ACTUAL));
        return false;
    }

    return true;
}

bool Respaldo::restaurarRespaldo(const QString& rutaOrigen,
                                 const QString& rutaDestino,
                                 QString* error)
{
    if (!validarRespaldo(rutaOrigen, error))
        return false;

    if (rutaDestino.isEmpty())
    {
        reportar(error,
                 QStringLiteral("No se puede restaurar: no se indicó la ruta de destino."));
        return false;
    }

    // Se copia primero a un archivo temporal en el mismo directorio y después se
    // sustituye el destino: así el destino no queda a medias si la copia falla.
    const QString rutaTemporal = rutaDestino + QStringLiteral(".restaurando");

    if (QFile::exists(rutaTemporal) && !QFile::remove(rutaTemporal))
    {
        reportar(error,
                 QStringLiteral("No se pudo preparar la restauración: «%1» ya existe.")
                     .arg(rutaTemporal));
        return false;
    }

    if (!QFile::copy(rutaOrigen, rutaTemporal))
    {
        reportar(error,
                 QStringLiteral("No se pudo copiar el respaldo «%1» a «%2».")
                     .arg(rutaOrigen, rutaTemporal));
        QFile::remove(rutaTemporal);
        return false;
    }

    if (QFile::exists(rutaDestino) && !QFile::remove(rutaDestino))
    {
        reportar(error,
                 QStringLiteral("No se pudo reemplazar la base «%1».").arg(rutaDestino));
        QFile::remove(rutaTemporal);
        return false;
    }

    if (!QFile::rename(rutaTemporal, rutaDestino))
    {
        reportar(error,
                 QStringLiteral("No se pudo colocar el respaldo en «%1».").arg(rutaDestino));
        QFile::remove(rutaTemporal);
        return false;
    }

    return true;
}
