#pragma once

#include <QSqlDatabase>
#include <QString>

#include "backend/database/version_esquema.hpp"

/// Comprobación única de validez de la base para los tres momentos de la 016 (RF-1).
namespace validez_base
{
    /// Diagnóstico **interno** del fallo. Los consumidores usan solo
    /// `Comprobacion::valida()` y `Comprobacion::mensaje`; no ramifican por causa.
    enum class Causa
    {
        Valida,  ///< La base es válida.
        Version, ///< La versión no coincide o no se pudo leer.
        Esquema, ///< La versión coincide pero el catálogo no es el esperado.
    };

    struct Comprobacion
    {
        Causa causa = Causa::Valida;
        QString mensaje; ///< Vacío si `causa == Valida`.

        bool valida() const { return causa == Causa::Valida; }
    };

    /// Única comprobación de validez para abrir, generar y guardar (016 RF-1).
    ///
    /// Reutiliza `VersionEsquema::esquemaEsperado` (no duplica la guardia de la 015).
    /// Solo lectura: no migra, no repara y no escribe. `versionEsperada` se inyecta para
    /// las pruebas; en producción es la constante del esquema.
    Comprobacion comprobar(QSqlDatabase& db,
                           int versionEsperada = VersionEsquema::VERSION_ESQUEMA_ACTUAL);

    /// Mensaje de la causa versión: crear una base nueva y aviso de que un respaldo de la
    /// versión anterior volverá a fallar. Puro y determinista.
    QString mensajeDeVersion(int versionArchivo, int versionEsperada);

    /// Mensaje de la causa esquema: causa concreta y acción (respaldo de la versión
    /// vigente o base nueva). Puro y determinista.
    QString mensajeDeEsquema(int versionEsperada, const QString& motivo);
} // namespace validez_base
