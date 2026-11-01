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

    const QJsonObject objeto = doc.object();
    const Resultado<SolverConfig> configuracion = SolverConfig::fromJson(objeto);
    if (configuracion.ok)
        return configuracion;

    // RF-2: una configuración 1.0, sin versión o con versión no reconocida se rechaza sin
    // reinterpretarla ni convertirla. Como aquí la configuración es un preset guardado en disco, el
    // rechazo añade el archivo y la acción disponible. La versión se comprueba antes que las
    // secciones (V1), así que este mensaje no reprocha secciones ausentes.
    const QString version = objeto.value(QLatin1String("version")).toString();
    if (version != QLatin1String("2.0"))
    {
        const QString declarada = version.isEmpty()
                                      ? QStringLiteral("no declara versión")
                                      : QStringLiteral("declara la versión «%1»").arg(version);
        return Resultado<SolverConfig>::error(
            QStringLiteral("La configuración de «%1» %2; solo se acepta la versión 2.0. Genere una"
                           " configuración 2.0 desde los datos con la versión actual.")
                .arg(ruta, declarada));
    }

    return configuracion;
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
