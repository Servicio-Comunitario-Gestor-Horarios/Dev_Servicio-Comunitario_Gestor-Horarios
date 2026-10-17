#include "backend/database/apertura_base_datos.hpp"

#include <QDebug>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace
{
    using AperturaBaseDatos::Estado;
    using AperturaBaseDatos::Resultado;

    /// Flujo de apertura sobre una conexión ya abierta.
    ///
    /// `existe` indica si el archivo existía antes de abrir la conexión (SQLite
    /// crea el archivo al abrirlo). No cierra la conexión: eso lo hace el llamador.
    Resultado procesar(QSqlDatabase& db,
                       const QString& ruta,
                       bool existe,
                       int versionEsperada,
                       const std::function<bool(QSqlDatabase&)>& crear,
                       const std::function<bool(QSqlDatabase&)>& verificar)
    {
        Resultado resultado;

        // Habilitar claves foráneas en la conexión de apertura.
        {
            QSqlQuery pragma(db);
            pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
        }

        // Base inexistente: crear el esquema único entero.
        if (!existe)
        {
            if (!crear(db))
            {
                resultado.estado = Estado::FalloApertura;
                resultado.detalle =
                    QStringLiteral("No se pudo crear el esquema de la base «%1».").arg(ruta);
                return resultado;
            }

            resultado.estado = Estado::OkCreada;
            resultado.detalle =
                QStringLiteral("Se creó la base de datos «%1» con el esquema único.").arg(ruta);
            return resultado;
        }

        // Base existente: versión y catálogo deben corresponder al esquema esperado.
        const int versionArchivo = VersionEsquema::leerVersionEsquema(db);

        const VersionEsquema::EstadoApertura decision =
            VersionEsquema::decidirApertura(true, versionArchivo, versionEsperada);

        if (decision == VersionEsquema::EstadoApertura::Abrir && verificar(db))
        {
            resultado.estado = Estado::OkAbierta;
            resultado.detalle =
                QStringLiteral("La base de datos «%1» ya está en la versión %2.")
                    .arg(ruta)
                    .arg(versionArchivo);
            return resultado;
        }

        resultado.estado = Estado::FalloEsquema;

        if (decision == VersionEsquema::EstadoApertura::Abrir)
        {
            QString motivo;
            VersionEsquema::esquemaEsperado(db, &motivo);

            resultado.detalle =
                QStringLiteral("El esquema de «%1» no corresponde al esperado: declara la versión"
                               " %2, que coincide, pero su contenido no es el esperado%3."
                               " No se migra ni se modifican los datos; puede crear una base nueva"
                               " (se respalda la actual) o restaurar un respaldo.")
                    .arg(ruta)
                    .arg(versionArchivo)
                    .arg(motivo.isEmpty() ? QString() : QStringLiteral(" (%1)").arg(motivo));
        }
        else
        {
            resultado.detalle =
                QStringLiteral("El esquema de «%1» no corresponde al esperado: declara la versión"
                               " %2 y esta aplicación espera la versión %3. No se migra ni se"
                               " modifican los datos; puede crear una base nueva (se respalda la"
                               " actual) o restaurar un respaldo.")
                    .arg(ruta)
                    .arg(versionArchivo)
                    .arg(versionEsperada);
        }

        return resultado;
    }
} // namespace

namespace AperturaBaseDatos
{
    Resultado abrir(const QString& ruta)
    {
        return abrir(ruta, Opciones{});
    }

    Resultado abrir(const QString& ruta, const Opciones& opciones)
    {
        Resultado resultado;

        if (ruta.isEmpty())
        {
            resultado.estado = Estado::FalloApertura;
            resultado.detalle =
                QStringLiteral("No se indicó la ruta de la base de datos.");
            return resultado;
        }

        // Resolver las dependencias no inyectadas con los valores de producción.
        const int versionEsperada = opciones.versionEsperada;

        const std::function<bool(QSqlDatabase&)> crear =
            opciones.crear ? opciones.crear
                           : std::function<bool(QSqlDatabase&)>(
                                 [](QSqlDatabase& db) -> bool {
                                     return VersionEsquema::crearEsquema(db);
                                 });

        const std::function<bool(QSqlDatabase&)> verificar =
            opciones.verificar ? opciones.verificar
                               : std::function<bool(QSqlDatabase&)>(
                                     [](QSqlDatabase& db) -> bool {
                                         return VersionEsquema::esquemaEsperado(db);
                                     });

        // Se comprueba antes de abrir porque SQLite crea el archivo al abrirlo.
        const bool existe = QFile::exists(ruta);

        // Conexión propia con nombre único para no colisionar con la por defecto.
        const QString nombreConexion =
            QStringLiteral("apertura_base_datos_%1")
                .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), nombreConexion);
            db.setDatabaseName(ruta);

            if (!db.open())
            {
                qWarning() << "No se pudo abrir la base de datos:" << db.lastError().text();
                resultado.estado = Estado::FalloApertura;
                resultado.detalle =
                    QStringLiteral("No se pudo abrir la base de datos «%1». Verifique que el archivo"
                                   " no esté en uso y que haya permisos de lectura y escritura, e"
                                   " intente de nuevo.")
                        .arg(ruta);
            }
            else
            {
                resultado = procesar(db, ruta, existe, versionEsperada, crear, verificar);
                db.close();
            }

            db = QSqlDatabase();
        }

        QSqlDatabase::removeDatabase(nombreConexion);

        return resultado;
    }
} // namespace AperturaBaseDatos
