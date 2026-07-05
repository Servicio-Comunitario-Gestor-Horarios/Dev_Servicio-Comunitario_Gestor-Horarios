#include "app/aplicacion_backend.hpp"
#include <middleware/internalserver.h>
#include <QCoreApplication>
#include <QDebug>

/**
 * @brief Implementación del modo servidor backend.
 *
 * Flujo de arranque:
 * 1. Crea el servidor IPC (InternalServer) con QLocalSocket.
 * 2. Si el servidor no puede iniciar, retorna 1.
 * 3. Entra en QCoreApplication::exec() para procesar conexiones.
 *
 * @note DatabaseManager y SolverManager se integrarán en fases futuras.
 */
int ejecutarAplicacionBackend(int argc, char *argv[])
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    qDebug() << "Backend: iniciando en modo backend...";

    InternalServer servidor;
    if (!servidor.start()) {
        qCritical() << "Backend: no se pudo iniciar el servidor IPC";
        return 1;
    }

    // TODO: Inicializar DatabaseManager (Sprint 2 — Nicole)
    // TODO: Inicializar SolverManager OR-Tools (Sprint 4)

    qDebug() << "Backend: listo para recibir conexiones";
    return QCoreApplication::exec();
}
