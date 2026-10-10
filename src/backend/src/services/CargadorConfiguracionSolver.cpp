#include "backend/services/CargadorConfiguracionSolver.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

Resultado<SolverConfig> CargadorConfiguracionSolver::cargar(const QString& ruta)
{
    if (ruta.isEmpty())
    {
        return Resultado<SolverConfig>::error(
            QStringLiteral("No se indicó la ruta del archivo de configuración."));
    }

    QFile archivo(ruta);
    if (!archivo.exists())
    {
        return Resultado<SolverConfig>::error(
            QStringLiteral("No se encontró el archivo «%1». Compruebe la ruta e intente de nuevo.")
                .arg(ruta));
    }

    if (!archivo.open(QIODevice::ReadOnly))
    {
        return Resultado<SolverConfig>::error(
            QStringLiteral("No se pudo abrir «%1» para leerlo. Verifique los permisos e intente de"
                           " nuevo.")
                .arg(ruta));
    }

    const QByteArray contenido = archivo.readAll();
    archivo.close();

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(contenido, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject())
    {
        return Resultado<SolverConfig>::error(
            QStringLiteral("El archivo «%1» no es un JSON válido. Corrija el archivo e intente de"
                           " nuevo.")
                .arg(ruta));
    }

    return SolverConfig::fromJson(doc.object());
}

Resultado<bool> CargadorConfiguracionSolver::guardar(const QString& ruta, const SolverConfig& config)
{
    if (ruta.isEmpty())
    {
        return Resultado<bool>::error(
            QStringLiteral("No se indicó la ruta de destino para guardar la configuración."));
    }

    const QFileInfo info(ruta);
    const QDir directorio(info.absolutePath());
    if (!directorio.exists())
    {
        return Resultado<bool>::error(
            QStringLiteral("La carpeta «%1» no existe. Cree la carpeta o elija otra ubicación e"
                           " intente de nuevo.")
                .arg(info.absolutePath()));
    }

    QFile archivo(ruta);
    if (!archivo.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return Resultado<bool>::error(
            QStringLiteral("No se pudo escribir «%1». Verifique los permisos y el espacio en disco"
                           " e intente de nuevo.")
                .arg(ruta));
    }

    const QJsonDocument doc(config.toJson());
    if (archivo.write(doc.toJson(QJsonDocument::Indented)) == -1)
    {
        archivo.close();
        return Resultado<bool>::error(
            QStringLiteral("No se pudo guardar la configuración en «%1». Verifique el espacio en"
                           " disco e intente de nuevo.")
                .arg(ruta));
    }

    archivo.close();
    return Resultado<bool>::exito(true);
}

Resultado<QStringList> CargadorConfiguracionSolver::listarPresets(const QString& directorio)
{
    const QDir dir(directorio);
    if (!dir.exists())
    {
        return Resultado<QStringList>::error(
            QStringLiteral("La carpeta de presets «%1» no existe. Elija otra carpeta e intente de"
                           " nuevo.")
                .arg(directorio));
    }

    const QStringList archivos =
        dir.entryList(QStringList{QStringLiteral("*.json")}, QDir::Files, QDir::Name);
    return Resultado<QStringList>::exito(archivos);
}
