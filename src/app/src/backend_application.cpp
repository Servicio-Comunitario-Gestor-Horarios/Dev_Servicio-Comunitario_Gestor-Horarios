#include "app/backend_application.hpp"
#include <middleware/internalserver.h>
#include <QCoreApplication>
#include <QDebug>

int run_backend_app(int argc, char *argv[]) {
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    qDebug() << "Backend: iniciando en modo backend...";

    InternalServer server;
    if (!server.start()) {
        qCritical() << "Backend: no se pudo iniciar el servidor IPC";
        return 1;
    }

    // TODO: Inicializar DatabaseManager
    // TODO: Inicializar SolverManager (OR-Tools)

    qDebug() << "Backend: listo para recibir conexiones";
    return QCoreApplication::exec();
}
