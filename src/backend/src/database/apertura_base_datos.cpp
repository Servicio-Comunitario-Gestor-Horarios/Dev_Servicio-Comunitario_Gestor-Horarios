#include "backend/database/apertura_base_datos.hpp"

#include "backend/database/migracion.hpp"
#include "backend/database/respaldo.hpp"

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
                       const VersionEsquema::PasoMigracion& paso,
                       const AperturaBaseDatos::FuncionRespaldo& respaldo,
                       const QDateTime& ahora)
    {
        Resultado resultado;

        // Habilitar claves foráneas en la conexión de apertura.
        {
            QSqlQuery pragma(db);
            pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
        }

        // Base inexistente: crear el esquema inicial y fijar la versión.
        if (!existe)
        {
            if (!VersionEsquema::aplicarHasta(db, versionEsperada, paso))
            {
                resultado.estado = Estado::FalloApertura;
                resultado.detalle =
                    QStringLiteral("No se pudo crear el esquema inicial de la base «%1».")
                        .arg(ruta);
                return resultado;
            }

            resultado.estado = Estado::OkCreada;
            resultado.detalle =
                QStringLiteral("Se creó la base de datos «%1» con el esquema inicial.").arg(ruta);
            return resultado;
        }

        const int versionArchivo = VersionEsquema::leerVersionEsquema(db);

        const VersionEsquema::EstadoApertura decision =
            VersionEsquema::decidirApertura(true, versionArchivo, versionEsperada);

        switch (decision)
        {
            case VersionEsquema::EstadoApertura::Abrir:
                resultado.estado = Estado::OkAbierta;
                resultado.detalle =
                    QStringLiteral("La base de datos «%1» ya está en la versión %2.")
                        .arg(ruta)
                        .arg(versionArchivo);
                return resultado;

            case VersionEsquema::EstadoApertura::FalloAusente:
                resultado.estado = Estado::FalloAusente;
                resultado.detalle =
                    QStringLiteral("La versión de esquema de «%1» está ausente o no es"
                                   " interpretable; no se migra.")
                        .arg(ruta);
                return resultado;

            case VersionEsquema::EstadoApertura::FalloPosterior:
                resultado.estado = Estado::FalloPosterior;
                resultado.detalle =
                    QStringLiteral("La versión de esquema de «%1» (%2) es posterior a la"
                                   " esperada (%3).")
                        .arg(ruta)
                        .arg(versionArchivo)
                        .arg(versionEsperada);
                return resultado;

            case VersionEsquema::EstadoApertura::Migrar:
            case VersionEsquema::EstadoApertura::Crear:
                break;
        }

        // Migración: el respaldo es obligatorio. Si no se puede respaldar, no se migra.
        const QString rutaRespaldo = Respaldo::construirNombreRespaldo(ruta, ahora);

        QString errorRespaldo;

        if (!respaldo(db, rutaRespaldo, &errorRespaldo))
        {
            resultado.estado = Estado::FalloRespaldo;
            resultado.detalle =
                QStringLiteral("No se pudo respaldar la base «%1» antes de migrar;"
                               " no se migró. %2")
                    .arg(ruta, errorRespaldo);
            return resultado;
        }

        resultado.rutaRespaldo = rutaRespaldo;

        if (!VersionEsquema::aplicarHasta(db, versionEsperada, paso))
        {
            // `aplicarHasta` revierte cada paso fallido: la versión previa queda intacta.
            resultado.estado = Estado::FalloMigracion;
            resultado.detalle =
                QStringLiteral("La migración de «%1» falló; la base queda en su versión"
                               " previa (%2). Respaldo: «%3».")
                    .arg(ruta)
                    .arg(versionArchivo)
                    .arg(rutaRespaldo);
            return resultado;
        }

        resultado.estado = Estado::OkMigrada;
        resultado.detalle =
            QStringLiteral("La base de datos «%1» se migró de la versión %2 a la %3.")
                .arg(ruta)
                .arg(versionArchivo)
                .arg(versionEsperada);
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

        const VersionEsquema::PasoMigracion paso =
            opciones.paso
                ? opciones.paso
                : VersionEsquema::PasoMigracion(
                      [](QSqlDatabase& conexion, int destino) -> bool {
                          return Migracion::aplicarPaso(conexion, destino);
                      });

        const FuncionRespaldo respaldo =
            opciones.respaldo ? opciones.respaldo
                              : FuncionRespaldo(
                                    [](QSqlDatabase& conexion, const QString& rutaRespaldo,
                                       QString* error) -> bool {
                                        return Respaldo::crearRespaldo(conexion, rutaRespaldo,
                                                                       error);
                                    });

        const QDateTime ahora =
            opciones.ahora.isValid() ? opciones.ahora : QDateTime::currentDateTime();

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
                resultado.estado = Estado::FalloApertura;
                resultado.detalle =
                    QStringLiteral("No se pudo abrir la base de datos «%1»: %2")
                        .arg(ruta, db.lastError().text());
            }
            else
            {
                resultado = procesar(db, ruta, existe, versionEsperada, paso, respaldo, ahora);
                db.close();
            }

            db = QSqlDatabase();
        }

        QSqlDatabase::removeDatabase(nombreConexion);

        return resultado;
    }
} // namespace AperturaBaseDatos
