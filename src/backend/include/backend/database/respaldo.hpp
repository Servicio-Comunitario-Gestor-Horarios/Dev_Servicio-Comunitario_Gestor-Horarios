#pragma once

#include <QDateTime>
#include <QSqlDatabase>
#include <QString>

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
}
