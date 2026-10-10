#pragma once

#include <QDateTime>
#include <QSqlDatabase>
#include <QString>

#include <functional>

/// Creación, validación y restauración de respaldos de la base de datos (RF-1, RF-6).
///
/// El respaldo es obligatorio antes de migrar o descartar la base: si no se puede
/// crear, la operación no debe continuar.
namespace Respaldo
{
    /// Construye la ruta de un respaldo a partir de la ruta de la base y un instante.
    ///
    /// Función pura y determinista: no lee el reloj ni toca el disco. El formato es
    /// `<rutaDb>.respaldo-YYYYMMDD-hhmmss`.
    QString construirNombreRespaldo(const QString& rutaDb, const QDateTime& ahora);

    /// Crea un respaldo de la base abierta en `db` usando `VACUUM INTO`.
    ///
    /// Retorna false si el respaldo no es posible (base ilegible, sin permisos,
    /// sin espacio, ruta de destino existente...). En ese caso, si `error` no es
    /// nulo, se le asigna el motivo.
    bool crearRespaldo(QSqlDatabase& db, const QString& rutaRespaldo, QString* error = nullptr);

    /// Valida un respaldo: `PRAGMA integrity_check` debe ser "ok" y su versión de
    /// esquema debe ser interpretable y no posterior a la esperada.
    ///
    /// Retorna false si el archivo no es un respaldo válido; si `error` no es nulo,
    /// se le asigna el motivo.
    bool validarRespaldo(const QString& rutaRespaldo, QString* error = nullptr);

    /// Restaura `rutaOrigen` sobre `rutaDestino`: valida el respaldo y reemplaza
    /// el archivo destino por una copia del respaldo.
    ///
    /// Retorna false si el respaldo no es válido o si la sustitución falla; si
    /// `error` no es nulo, se le asigna el motivo. El destino previo se conserva
    /// si el respaldo no es válido.
    bool restaurarRespaldo(const QString& rutaOrigen,
                           const QString& rutaDestino,
                           QString* error = nullptr);

    // ─── Crear base nueva descartando la anterior (RF-4, RF-6) ────────────

    /// Desenlace de descartar la base actual y crear una nueva.
    enum class EstadoDescarte
    {
        Ok,            ///< Se respaldó (si había base), se descartó y se creó la nueva.
        FalloBase,     ///< No se indicó una ruta válida.
        FalloRespaldo, ///< No se pudo respaldar: la base anterior NO se descartó.
        FalloDescarte, ///< Se respaldó, pero no se pudo descartar la base anterior.
        FalloCreacion  ///< Se descartó, pero no se pudo crear la nueva; el respaldo sigue disponible.
    };

    /// Resultado de «crear base de datos nueva», con la ruta del respaldo si lo hubo.
    struct ResultadoDescarte
    {
        EstadoDescarte estado = EstadoDescarte::FalloBase;
        QString detalle;
        QString rutaRespaldo;

        bool ok() const { return estado == EstadoDescarte::Ok; }
    };

    /// Dependencias inyectables de `descartarBaseYCrearNueva` (tests).
    struct OpcionesDescarte
    {
        /// Instante para el nombre del respaldo (determinista). Vacío = reloj del sistema.
        QDateTime ahora;

        /// Cómo respaldar la base abierta. Por defecto, `Respaldo::crearRespaldo`.
        std::function<bool(QSqlDatabase&, const QString&, QString*)> respaldo;

        /// Cómo crear la base nueva tras descartar. Por defecto, `AperturaBaseDatos::abrir`.
        std::function<bool(const QString&, QString*)> crearNueva;
    };

    /// Descarta la base en `ruta` y crea una nueva con el esquema inicial (RF-4, RF-6).
    ///
    /// El respaldo es obligatorio: si no se puede crear, la base anterior NO se
    /// descarta. Si la creación falla después de descartar, se conserva la ruta del
    /// respaldo para ofrecer restaurarlo. Si no existe base previa, solo se crea.
    ResultadoDescarte descartarBaseYCrearNueva(const QString& ruta,
                                               const OpcionesDescarte& opciones = {});
}
