#include "backend/database/comprobacion_validez.hpp"

namespace validez_base
{
QString mensajeDeVersion(int versionArchivo, int versionEsperada)
{
    return QStringLiteral("La base de datos es de la versión %1 y esta aplicación espera la "
                          "versión %2. No se migra ni se repara: debe crear una base nueva (se "
                          "respalda la actual); restaurar un respaldo de la versión anterior "
                          "volverá a fallar.")
        .arg(versionArchivo)
        .arg(versionEsperada);
}

QString mensajeDeEsquema(int versionEsperada, const QString& motivo)
{
    QString mensaje = QStringLiteral(
        "La base de datos no cumple el esquema esperado de la versión %1").arg(versionEsperada);
    if (!motivo.trimmed().isEmpty())
        mensaje += QStringLiteral(": %1").arg(motivo.trimmed());
    mensaje += QStringLiteral(". Restaure un respaldo de la versión vigente o cree una base "
                              "nueva (con respaldo obligatorio).");
    return mensaje;
}

Comprobacion comprobar(QSqlDatabase& db, int versionEsperada)
{
    if (!db.isOpen())
    {
        return Comprobacion{Causa::Version,
                            QStringLiteral("No se pudo comprobar la base de datos: la conexión "
                                           "no está abierta. Cree una base nueva o restaure un "
                                           "respaldo de la versión vigente.")};
    }

    // P5 (015): toda la comprobación en una instantánea. Si ya hay una transacción abierta,
    // se usa esa y no se cierra; es de solo lectura y no escribe nunca.
    const bool transaccionPropia = db.transaction();
    const auto cerrar = [&db, transaccionPropia]() {
        if (transaccionPropia)
            db.commit();
    };

    const int versionArchivo = VersionEsquema::leerVersionEsquema(db);
    if (versionArchivo == -1)
    {
        cerrar();
        return Comprobacion{Causa::Version,
                            QStringLiteral("No se pudo leer la versión de esquema de la base. "
                                           "Cree una base nueva o restaure un respaldo de la "
                                           "versión vigente.")};
    }

    // La versión primero: una base de otra versión no paga la guardia completa y recibe la
    // acción correcta (un respaldo de la versión anterior volverá a fallar).
    if (versionArchivo != versionEsperada)
    {
        cerrar();
        return Comprobacion{Causa::Version,
                            mensajeDeVersion(versionArchivo, versionEsperada)};
    }

    QString motivo;
    const bool esquemaOk = VersionEsquema::esquemaEsperado(db, &motivo);
    cerrar();
    if (!esquemaOk)
    {
        return Comprobacion{Causa::Esquema,
                            mensajeDeEsquema(versionEsperada, motivo)};
    }

    return Comprobacion{};
}
} // namespace validez_base
