#pragma once

#include <QSqlDatabase>
#include <QString>

/// Versión del esquema y creación del esquema único (RF-6).
///
/// La versión de esquema se guarda en `PRAGMA user_version` de SQLite. **No hay
/// migración por versión**: la base se crea entera y cualquier versión distinta a
/// la esperada se trata como fallo de apertura (no se migra ni se borran datos).
namespace VersionEsquema
{
    /// Versión de esquema que espera esta compilación de la aplicación (marca única).
    ///
    /// v3: catálogo de tipos de aula con clave normalizada, referencia de tipo
    /// obligatoria en aulas y materias, y eventos del turno con identidad y orden
    /// (se retira el registro de recesos).
    constexpr int VERSION_ESQUEMA_ACTUAL = 3;

    /// Resultado de decidir qué hacer al abrir una base de datos.
    enum class EstadoApertura
    {
        Crear,         ///< El archivo no existe: crear el esquema único.
        Abrir,         ///< Versión igual a la esperada: abrir sin tocar.
        FalloEsquema   ///< Cualquier otra versión (ausente, anterior o posterior).
    };

    /// Decide la apertura de forma determinista, sin E/S.
    /// `versionArchivo <= 0` representa una versión ausente/no interpretable.
    EstadoApertura decidirApertura(bool existe, int versionArchivo, int versionEsperada);

    /// Lee `PRAGMA user_version`. Devuelve -1 si no es legible.
    int leerVersionEsquema(QSqlDatabase& db);

    /// Fija `PRAGMA user_version` al valor indicado.
    bool fijarVersionEsquema(QSqlDatabase& db, int version);

    /// Crea el esquema único entero en **una sola transacción**: aplica el catálogo
    /// completo y fija la marca de versión. Idempotente. Retorna false si falla.
    bool crearEsquema(QSqlDatabase& db);

    /// Comprueba que la base tenga el catálogo del esquema único (tablas y columnas
    /// esperadas) y **no** tenga tablas retiradas (p. ej. `Curso_Materia` o el
    /// registro de recesos `Recesos`, sustituido por `Eventos`).
    /// Si se pasa `motivo`, recibe una explicación legible de lo que no cuadra
    /// (vacía si el catálogo corresponde al esquema esperado).
    bool esquemaEsperado(QSqlDatabase& db, QString* motivo = nullptr);
}
