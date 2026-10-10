#include "datos/ContextoBaseDatos.hpp"

#include "backend/database/version_esquema.hpp"
#include "backend/services/NucleoDatos.hpp"

#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>
#include <QDebug>

ContextoBaseDatos::ContextoBaseDatos() = default;

ContextoBaseDatos::~ContextoBaseDatos()
{
    cerrar();
}

AperturaBaseDatos::Resultado ContextoBaseDatos::abrir(const QString& ruta)
{
    return abrir(ruta, Opciones{});
}

AperturaBaseDatos::Resultado ContextoBaseDatos::abrir(const QString& ruta, const Opciones& opciones)
{
    // Reintentar sobre un contexto ya abierto parte de cero.
    cerrar();

    // 1. Apertura con versión/migración/respaldo (RF-1, RF-4).
    const AperturaBaseDatos::Resultado resultado =
        AperturaBaseDatos::abrir(ruta, opciones.apertura);
    if (!resultado.ok())
        return resultado;

    // 2. Conexión persistente del cliente con nombre único.
    m_nombreConexion = opciones.nombreConexion.isEmpty()
        ? QStringLiteral("contexto_base_datos_%1")
              .arg(QUuid::createUuid().toString(QUuid::WithoutBraces))
        : opciones.nombreConexion;

    m_conexion = std::make_unique<QSqlDatabase>(
        QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_nombreConexion));
    m_conexion->setDatabaseName(ruta);

    if (!m_conexion->open())
    {
        qWarning() << "No se pudo abrir la base de datos para operar:" << m_conexion->lastError().text();
        AperturaBaseDatos::Resultado fallo;
        fallo.estado = AperturaBaseDatos::Estado::FalloApertura;
        fallo.detalle = QStringLiteral("No se pudo abrir la base de datos «%1» para operar con "
                                       "ella. Verifique que no esté en uso y que haya permisos de "
                                       "lectura y escritura, e intente de nuevo.")
                            .arg(ruta);
        cerrar();
        return fallo;
    }

    {
        QSqlQuery pragma(*m_conexion);
        pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    }

    // 3. Fachada de dominios sobre la conexión recién abierta.
    m_nucleo = std::make_unique<NucleoDatos>(*m_conexion);
    m_ruta = ruta;

    return resultado;
}

bool ContextoBaseDatos::abierto() const
{
    return m_conexion != nullptr && m_nucleo != nullptr;
}

NucleoDatos& ContextoBaseDatos::nucleo()
{
    Q_ASSERT(m_nucleo);
    return *m_nucleo;
}

const NucleoDatos& ContextoBaseDatos::nucleo() const
{
    Q_ASSERT(m_nucleo);
    return *m_nucleo;
}

QSqlDatabase& ContextoBaseDatos::conexion()
{
    Q_ASSERT(m_conexion);
    return *m_conexion;
}

QString ContextoBaseDatos::ruta() const
{
    return m_ruta;
}

QString ContextoBaseDatos::rutaPorDefecto()
{
    const QString directorio =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(directorio);
    return QDir(directorio).filePath(QStringLiteral("gestor_horarios.db"));
}

void ContextoBaseDatos::cerrar()
{
    // El núcleo guarda referencias a la conexión: se destruye primero.
    m_nucleo.reset();

    if (m_conexion)
    {
        if (m_conexion->isOpen())
            m_conexion->close();
        m_conexion.reset();

        // Sin handles vivos, quitar la conexión del registro de Qt no avisa.
        if (!m_nombreConexion.isEmpty())
            QSqlDatabase::removeDatabase(m_nombreConexion);
    }

    m_nombreConexion.clear();
    m_ruta.clear();
}
