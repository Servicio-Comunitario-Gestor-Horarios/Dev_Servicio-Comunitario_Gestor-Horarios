#pragma once

#include <QSqlDatabase>
#include <QString>

#include <functional>

#include "backend/database/version_esquema.hpp"

/// Orquesta la apertura de la base de datos local (RF-6).
///
/// Esquema único: si el archivo no existe se crea entero; si existe, su versión y
/// su catálogo deben coincidir con el esquema esperado. Cualquier otra cosa es un
/// **fallo de esquema**: no se migra ni se borran datos (constitución §5).
/// La conexión se abre y cierra dentro de esta unidad con un nombre único.
namespace AperturaBaseDatos
{
    /// Desenlace de intentar abrir la base de datos.
    enum class Estado
    {
        OkCreada,       ///< No existía: se creó con el esquema único.
        OkAbierta,      ///< Existía con versión y catálogo esperados: se abrió sin tocar.
        FalloEsquema,   ///< Versión o catálogo no corresponden al esquema esperado.
        FalloApertura   ///< No se pudo abrir la base o crear el esquema.
    };

    /// Resultado de la apertura, con detalle legible.
    struct Resultado
    {
        Estado  estado = Estado::FalloApertura;
        QString detalle;

        /// Indica si la apertura terminó con la base operativa.
        bool ok() const
        {
            return estado == Estado::OkCreada || estado == Estado::OkAbierta;
        }
    };

    /// Dependencias inyectables de la apertura (tests). Vacías = valores de producción
    /// (`VersionEsquema::crearEsquema` y `VersionEsquema::esquemaEsperado`).
    struct Opciones
    {
        int versionEsperada = VersionEsquema::VERSION_ESQUEMA_ACTUAL;
        std::function<bool(QSqlDatabase&)> crear;
        std::function<bool(QSqlDatabase&)> verificar;
    };

    /// Abre la base de datos en `ruta` con los valores de producción.
    Resultado abrir(const QString& ruta);

    /// Abre la base de datos en `ruta` con dependencias inyectables (tests).
    Resultado abrir(const QString& ruta, const Opciones& opciones);
}
