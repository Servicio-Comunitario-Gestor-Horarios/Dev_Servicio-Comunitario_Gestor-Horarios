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

            // 020 (RF-2/RF-3): cada desenlace tiene su código; NoFactible y no concluyente no
            // se confunden, y un error de entrada/contrato no se presenta como NoFactible.
            switch (respuesta.desenlace) {
                case DesenlaceSolver::Exito:
                    servidor.responder(socket, Middleware::RESP_EXITO, respuesta.salida);
                    break;
                case DesenlaceSolver::Infactible:
                    servidor.responder(socket, Middleware::RESP_SIN_SOLUCION, respuesta.error);
                    break;
                case DesenlaceSolver::NoConcluyente:
                    servidor.responder(socket, Middleware::RESP_NO_CONCLUYENTE, respuesta.error);
                    break;
                case DesenlaceSolver::ErrorContrato:
                    servidor.responder(socket, Middleware::RESP_INVALIDO, respuesta.error);
                    break;
                case DesenlaceSolver::SinRespuesta:
                default:
                    // El proceso que responde siempre tiene un desenlace propio; una pérdida de
                    // comunicación la marca el puerto del cliente, no este lado.
                    servidor.responder(socket, Middleware::RESP_ERROR, respuesta.error);
                    break;
            }
        });
#else
    qWarning() << "Backend: compilado sin solver (BUILD_BACKEND=OFF); solver_resolve no disponible";
#endif

    qDebug() << "Backend: listo para recibir conexiones";
    return QCoreApplication::exec();
}
