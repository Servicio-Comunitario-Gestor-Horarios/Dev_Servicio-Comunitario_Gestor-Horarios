#include "app/aplicacion_backend.hpp"

#include <middleware/internalserver.h>
#include <middleware/messages.h>

#include <QCoreApplication>
#include <QDebug>
#include <QJsonObject>
#include <QLocalSocket>

#if defined(GESTOR_TIENE_SOLVER)
#include "backend/solver/servicio_solver.hpp"
#endif

/**
 * @brief Implementación del modo proceso de cálculo (`--backend`).
 *
 * Solo resuelve: registra la ruta `solver_resolve` (parsea el JSON de entrada,
 * llama al solver y responde con el HorarioSalida), más salud y apagado del
 * servidor IPC. NO toca la base de datos (RF-2).
 */
int ejecutarAplicacionBackend(int argc, char *argv[])
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    qDebug() << "Backend: iniciando en modo cálculo...";

    InternalServer servidor;
    if (!servidor.start()) {
        qCritical() << "Backend: no se pudo iniciar el servidor IPC";
        return 1;
    }

#if defined(GESTOR_TIENE_SOLVER)
    servidor.registerRoute(Middleware::OP_RESOLVER_HORARIO,
        [&servidor](const QJsonObject& payload, QLocalSocket* socket) {
            const RespuestaSolver respuesta = resolverEntradaSolver(payload);
            if (!respuesta.ok) {
                servidor.responder(socket, Middleware::RESP_INVALIDO, respuesta.error);
            } else if (!respuesta.factible) {
                servidor.responder(socket, Middleware::RESP_SIN_SOLUCION, respuesta.error);
            } else {
                servidor.responder(socket, Middleware::RESP_EXITO, respuesta.salida);
            }
        });
#else
    qWarning() << "Backend: compilado sin solver (BUILD_BACKEND=OFF); solver_resolve no disponible";
#endif

    qDebug() << "Backend: listo para recibir conexiones";
    return QCoreApplication::exec();
}
