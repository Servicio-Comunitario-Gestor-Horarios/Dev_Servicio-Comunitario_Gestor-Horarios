#pragma once

#include <QSqlDatabase>

#include <functional>

/// Versión del esquema y mecanismo de migraciones versionadas (RF-1).
///
/// La versión de esquema se guarda en `PRAGMA user_version` de SQLite.
/// `0` significa versión ausente o no interpretable y se trata como fallo de
/// apertura (no se migra). El esquema actual corresponde a la versión 2.
namespace VersionEsquema
{
    /// Versión de esquema que espera esta compilación de la aplicación.
    /// v1 = esquema base (aulas, profesores, materias, planes de estudio);
    /// v2 = dominios de curso, turnos y recesos.
    constexpr int VERSION_ESQUEMA_ACTUAL = 2;

    /// Resultado de decidir qué hacer al abrir una base de datos.
    enum class EstadoApertura
    {
        Crear,           ///< El archivo no existe: crear esquema inicial.
        Abrir,           ///< Versión igual a la esperada: abrir sin tocar.
        Migrar,          ///< Versión anterior: aplicar migraciones.
        FalloAusente,    ///< Versión ausente o no interpretable: no migrar.
        FalloPosterior   ///< Versión posterior a la esperada: no abrir.
    };

    /// Decide la apertura de forma determinista, sin E/S.
    /// `versionArchivo <= 0` representa una versión ausente/no interpretable.
    EstadoApertura decidirApertura(bool existe, int versionArchivo, int versionEsperada);

    /// Lee `PRAGMA user_version`. Devuelve -1 si no es legible.
    int leerVersionEsquema(QSqlDatabase& db);

    /// Fija `PRAGMA user_version` al valor indicado.
    bool fijarVersionEsquema(QSqlDatabase& db, int version);

    /// Aplica una migración concreta (a la versión `versionDestino`).
    /// Debe implementar exactamente el cambio de esa versión.
    /// Retorna false si falla; no gestiona transacciones.
    using PasoMigracion = std::function<bool(QSqlDatabase&, int versionDestino)>;

    /// Aplica las migraciones desde la versión almacenada hasta `versionDestino`.
    ///
    /// Cada paso va en su propia transacción junto con la actualización de
    /// `user_version`: si un paso falla, se revierte y la versión previa queda
    /// intacta (estado conocido, sin cambios parciales).
    bool aplicarHasta(QSqlDatabase& db, int versionDestino, const PasoMigracion& paso);

    /// Igual que la variante anterior, usando las migraciones reales
    /// registradas en `Migracion::aplicarPaso`.
    bool aplicarHasta(QSqlDatabase& db, int versionDestino);
}
