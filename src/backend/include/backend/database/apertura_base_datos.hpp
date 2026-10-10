#pragma once

#include <QDateTime>
#include <QSqlDatabase>
#include <QString>

#include <functional>

#include "backend/database/version_esquema.hpp"

/// Orquesta la apertura de la base de datos local (RF-1, RF-4).
///
/// Decide qué hacer a partir de la versión de esquema almacenada y aplica la
/// migración necesaria. La migración es obligatoriamente precedida de un respaldo:
/// si el respaldo no es posible, no se migra y la apertura se trata como fallo.
/// La conexión se abre y se cierra dentro de esta unidad con un nombre único, para
/// no colisionar con la conexión por defecto ni con la que use el llamador.
namespace AperturaBaseDatos
{
    /// Desenlace de intentar abrir la base de datos.
    enum class Estado
    {
        OkCreada,        ///< No existía: se creó con el esquema inicial.
        OkAbierta,       ///< Existía con la versión esperada: se abrió sin tocar.
        OkMigrada,       ///< Existía con versión anterior: se respaldó y migró.
        FalloAusente,    ///< Versión de esquema ausente o no interpretable.
        FalloPosterior,  ///< Versión de esquema posterior a la esperada.
        FalloApertura,   ///< No se pudo abrir la base o crear el esquema inicial.
        FalloRespaldo,   ///< No se pudo respaldar: no se migró.
        FalloMigracion   ///< La migración falló; la versión previa queda intacta.
    };

    /// Resultado de la apertura, con detalle legible y ruta del respaldo si hubo.
    struct Resultado
    {
        Estado estado = Estado::FalloApertura;
        QString detalle;
        QString rutaRespaldo;

        /// Indica si la apertura terminó con la base operativa.
        bool ok() const
        {
            return estado == Estado::OkCreada || estado == Estado::OkAbierta
                || estado == Estado::OkMigrada;
        }
    };

    /// Función de respaldo inyectable. Recibe la conexión abierta, la ruta destino
    /// y un puntero opcional para el motivo del fallo. Por defecto es
    /// `Respaldo::crearRespaldo`.
    using FuncionRespaldo = std::function<bool(QSqlDatabase&, const QString&, QString*)>;

    /// Dependencias inyectables de la apertura. Los campos vacíos usan los valores
    /// de producción: versión esperada actual, `Migracion::aplicarPaso`,
    /// `Respaldo::crearRespaldo` y el reloj del sistema.
    struct Opciones
    {
        int versionEsperada = VersionEsquema::VERSION_ESQUEMA_ACTUAL;
        VersionEsquema::PasoMigracion paso;
        FuncionRespaldo respaldo;
        QDateTime ahora;

        /// Observador de progreso de migración (RF-1): se invoca una vez por cada
        /// paso, con la versión destino del paso, antes de aplicarlo. Permite a la
        /// interfaz indicar que la migración está en marcha. Vacío = sin aviso.
        std::function<void(int versionDestino)> progreso;
    };

    /// Abre la base de datos en `ruta` con los valores de producción.
    Resultado abrir(const QString& ruta);

    /// Abre la base de datos en `ruta` con dependencias inyectables (tests).
    Resultado abrir(const QString& ruta, const Opciones& opciones);
}
