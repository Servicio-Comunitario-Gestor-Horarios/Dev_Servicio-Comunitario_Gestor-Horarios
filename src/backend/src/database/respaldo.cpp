#include "backend/database/respaldo.hpp"

#include "backend/database/apertura_base_datos.hpp"
#include "backend/database/version_esquema.hpp"

#include <QDebug>
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

    /// Abre la base en `ruta` con una conexión temporal y delega el respaldo en
    /// `respaldoFn`. Cierra y libera la conexión antes de volver.
    bool respaldarArchivo(const QString& ruta, const QString& rutaRespaldo,
                          const std::function<bool(QSqlDatabase&, const QString&, QString*)>& respaldoFn,
                          QString* error)
    {
        const QString nombreConexion =
            QStringLiteral("respaldo_descarte_%1")
                .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

        bool ok = false;
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), nombreConexion);
            db.setDatabaseName(ruta);

            if (!db.open())
            {
                reportar(error,
                         QStringLiteral("No se pudo abrir la base de datos para respaldarla."));
            }
            else
            {
                ok = respaldoFn(db, rutaRespaldo, error);
                db.close();
            }

            db = QSqlDatabase();
        }

        QSqlDatabase::removeDatabase(nombreConexion);
        return ok;
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
        qWarning() << "No se pudo crear el respaldo:" << query.lastError().text();
        reportar(error,
                 QStringLiteral("No se pudo crear el respaldo en «%1». Verifique que haya espacio"
                                " en disco y permisos de escritura en la carpeta de destino, e"
                                " intente de nuevo.")
                     .arg(rutaRespaldo));
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
        qWarning() << "No se pudo abrir el respaldo:" << detalle;
        reportar(error,
                 QStringLiteral("No se pudo abrir el respaldo «%1». Compruebe que el archivo existe,"
                                " no está en uso y es accesible, e intente de nuevo.")
                     .arg(rutaRespaldo));
        return false;
    }

    if (!integridadOk)
    {
        qWarning() << "El respaldo está dañado:" << detalle;
        reportar(error,
                 QStringLiteral("El respaldo «%1» está dañado o no es una base de datos válida."
                                " Elija otro respaldo o cree una base de datos nueva.")
                     .arg(rutaRespaldo));
        return false;
    }

    if (version <= 0 || version > VersionEsquema::VERSION_ESQUEMA_ACTUAL)
    {
        reportar(error,
                 QStringLiteral("La versión de esquema del respaldo «%1» (%2) no es compatible"
                                " con la de esta aplicación (%3). Use un respaldo de una versión"
                                " compatible o cree una base de datos nueva.")
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

Respaldo::ResultadoDescarte
Respaldo::descartarBaseYCrearNueva(const QString& ruta, const OpcionesDescarte& opciones)
{
    ResultadoDescarte resultado;

    if (ruta.isEmpty())
    {
        resultado.estado = EstadoDescarte::FalloBase;
        resultado.detalle = QStringLiteral("No se indicó la ruta de la base de datos.");
        return resultado;
    }

    const auto respaldoFn =
        opciones.respaldo
            ? opciones.respaldo
            : std::function<bool(QSqlDatabase&, const QString&, QString*)>(
                  [](QSqlDatabase& db, const QString& rutaRespaldo, QString* error) -> bool {
                      return Respaldo::crearRespaldo(db, rutaRespaldo, error);
                  });

    const auto crearFn =
        opciones.crearNueva
            ? opciones.crearNueva
            : std::function<bool(const QString&, QString*)>(
                  [](const QString& rutaNueva, QString* error) -> bool {
                      const AperturaBaseDatos::Resultado res =
                          AperturaBaseDatos::abrir(rutaNueva);
                      if (!res.ok() && error)
                          *error = res.detalle;
                      return res.ok();
                  });

    // Sin base previa: no hay nada que respaldar ni descartar; se crea una nueva.
    if (!QFile::exists(ruta))
    {
        QString error;
        if (!crearFn(ruta, &error))
        {
            resultado.estado = EstadoDescarte::FalloCreacion;
            resultado.detalle =
                QStringLiteral("No se pudo crear la base de datos «%1». %2")
                    .arg(ruta, error);
            return resultado;
        }

        resultado.estado = EstadoDescarte::Ok;
        resultado.detalle = QStringLiteral("No había una base anterior; se creó «%1».").arg(ruta);
        return resultado;
    }

    // 1. Respaldo obligatorio antes de descartar.
    const QDateTime ahora =
        opciones.ahora.isValid() ? opciones.ahora : QDateTime::currentDateTime();
    const QString rutaRespaldo = construirNombreRespaldo(ruta, ahora);

    QString errorRespaldo;
    if (!respaldarArchivo(ruta, rutaRespaldo, respaldoFn, &errorRespaldo))
    {
        resultado.estado = EstadoDescarte::FalloRespaldo;
        resultado.detalle =
            QStringLiteral("No se pudo respaldar la base «%1», así que no se descartó y no se"
                           " creó una base nueva. Verifique el espacio en disco y los permisos, e"
                           " intente de nuevo. %2")
                .arg(ruta, errorRespaldo);
        return resultado;
    }

    // Desde aquí el respaldo ya existe: se conserva su ruta aunque el descarte o
    // la creación fallen, para poder ofrecer restaurarlo.
    resultado.rutaRespaldo = rutaRespaldo;

    // 2. Descartar la base anterior.
    if (!QFile::remove(ruta))
    {
        resultado.estado = EstadoDescarte::FalloDescarte;
        resultado.detalle =
            QStringLiteral("Se respaldó la base anterior en «%1», pero no se pudo descartarla; no"
                           " se creó una base nueva. Verifique los permisos e intente de nuevo.")
                .arg(rutaRespaldo);
        return resultado;
    }

    // 3. Crear la base nueva.
    QString errorCreacion;
    if (!crearFn(ruta, &errorCreacion))
    {
        resultado.estado = EstadoDescarte::FalloCreacion;
        resultado.detalle =
            QStringLiteral("Se descartó la base anterior, pero no se pudo crear la nueva. Puede"
                           " restaurar el respaldo «%1». %2")
                .arg(rutaRespaldo, errorCreacion);
        return resultado;
    }

    resultado.estado = EstadoDescarte::Ok;
    resultado.detalle =
        QStringLiteral("Se respaldó la base anterior en «%1» y se creó una base nueva.").arg(rutaRespaldo);
    return resultado;
}
