#include "app/instancia_unica.hpp"

#include <QLocalServer>
#include <QLocalSocket>

namespace
{
    /// Mensaje que envía la instancia secundaria para pedir el foco.
    const QByteArray MENSAJE_ACTIVAR = QByteArrayLiteral("activar");

    /// Acuse que responde la instancia principal tras recibir «activar».
    const QByteArray MENSAJE_ACUSE = QByteArrayLiteral("acuse");

    /// Plazo máximo para establecer la conexión con la instancia existente.
    /// En Unix el fallo (socket ausente o rechazado) es inmediato; el plazo
    /// solo cubre sistemas donde la conexión puede tardar en resolverse.
    constexpr int PLAZO_CONEXION_MS = 3000;
} // namespace

const QString InstanciaUnica::NOMBRE_POR_DEFECTO =
    QStringLiteral("GestorHorarios_InstanciaUnica");

InstanciaUnica::InstanciaUnica(const QString& nombre, int plazoAcuseMs, QObject* padre)
    : QObject(padre)
    , m_nombre(nombre)
    , m_plazoAcuseMs(plazoAcuseMs)
{
}

InstanciaUnica::~InstanciaUnica()
{
    if (m_servidor)
    {
        m_servidor->close();
        delete m_servidor;
        m_servidor = nullptr;

        // Limpia el socket del sistema para no bloquear futuros arranques.
        QLocalServer::removeServer(m_nombre);
    }
}

bool InstanciaUnica::estaActiva() const
{
    return m_servidor != nullptr && m_servidor->isListening();
}

InstanciaUnica::Resultado InstanciaUnica::iniciar()
{
    // 1. ¿Hay otra instancia viva? Se comprueba intentando conectar primero.
    {
        QLocalSocket sonda;
        sonda.connectToServer(m_nombre);

        if (sonda.waitForConnected(PLAZO_CONEXION_MS))
        {
            // Hay una instancia principal: pedirle que se enfoque.
            return enviarActivacion(sonda);
        }

        // No hay servidor vivo. Si quedó un socket huérfano de un cierre
        // anómalo, se elimina para poder escuchar; solo se toca ante errores
        // que indican la ausencia de servidor (nunca ante permisos u otro).
        const QLocalSocket::LocalSocketError error = sonda.error();
        if (error == QLocalSocket::ConnectionRefusedError
            || error == QLocalSocket::ServerNotFoundError)
        {
            QLocalServer::removeServer(m_nombre);
        }
    }

    // 2. Somos la instancia principal: quedar a la escucha.
    m_servidor = new QLocalServer(this);
    connect(m_servidor, &QLocalServer::newConnection,
            this, &InstanciaUnica::alNuevaConexion);

    if (!m_servidor->listen(m_nombre))
    {
        // No se pudo ni conectar ni escuchar: fallo de arranque.
        delete m_servidor;
        m_servidor = nullptr;
        m_resultado = Resultado::SinAcuse;
        m_detalle = tr("No se pudo iniciar la aplicación: no había otra instancia "
                       "que enfocar, pero tampoco se pudo crear el servidor de "
                       "instancia única.");
        return m_resultado;
    }

    m_resultado = Resultado::Primaria;
    m_detalle.clear();
    return m_resultado;
}

InstanciaUnica::Resultado InstanciaUnica::enviarActivacion(QLocalSocket& socket)
{
    // No se espera a `waitForBytesWritten`: con escrituras pequeñas el buffer
    // ya queda vacío y ese método devolvería false aunque el envío haya ido bien.
    if (socket.write(MENSAJE_ACTIVAR) == -1)
    {
        m_resultado = Resultado::SinAcuse;
        m_detalle = tr("No se pudo contactar con la instancia en ejecución; "
                       "no se iniciará una segunda sesión.");
        return m_resultado;
    }
    socket.flush();

    // Espera el acuse dentro del plazo (por defecto, 10 s).
    QByteArray recibido;
    if (socket.waitForReadyRead(m_plazoAcuseMs))
        recibido = socket.readAll();
    else if (socket.bytesAvailable() > 0)
        recibido = socket.readAll();

    if (recibido.contains(MENSAJE_ACUSE))
    {
        m_resultado = Resultado::Secundaria;
        m_detalle.clear();
        return m_resultado;
    }

    m_resultado = Resultado::SinAcuse;
    m_detalle =
        tr("No se pudo enfocar la instancia existente en el plazo de %1 segundos; "
           "no se iniciará una segunda sesión.")
            .arg(m_plazoAcuseMs / 1000.0, 0, 'f', 1);
    return m_resultado;
}

void InstanciaUnica::alNuevaConexion()
{
    if (!m_servidor)
        return;

    while (m_servidor->hasPendingConnections())
    {
        QLocalSocket* socket = m_servidor->nextPendingConnection();

        connect(socket, &QLocalSocket::readyRead, socket, [this, socket]() {
            const QByteArray datos = socket->readAll();
            if (datos.contains(MENSAJE_ACTIVAR))
            {
                // Acuse primero y luego la señal, para que la secundaria salga.
                socket->write(MENSAJE_ACUSE);
                socket->flush();
                emit activarSolicitada();
            }
        });

        connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
    }
}
