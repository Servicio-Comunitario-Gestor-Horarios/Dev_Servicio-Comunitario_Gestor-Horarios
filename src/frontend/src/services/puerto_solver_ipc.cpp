#include "services/puerto_solver_ipc.hpp"

#if defined(GESTOR_TIENE_BACKEND)

#include <middleware/internalclient.h>
#include <middleware/messages.h>

PuertoSolverIpc::PuertoSolverIpc(InternalClient* cliente, QObject* parent)
    : QObject(parent)
    , m_cliente(cliente)
{
    if (m_cliente) {
        connect(m_cliente, &InternalClient::respuestaRecibida,
                this, &PuertoSolverIpc::onRespuestaRecibida);
    }
}

void PuertoSolverIpc::resolver(const QJsonObject& entrada,
                               std::function<void(const RespuestaSolver&)> alResponder)
{
    if (!m_cliente) {
        RespuestaSolver respuesta;
        respuesta.ok = false;
        respuesta.factible = false;
        respuesta.error = QStringLiteral("No hay conexión con el proceso de cálculo.");
        alResponder(respuesta);
        return;
    }

    m_alResponder = std::move(alResponder);
    m_cliente->enviarSolicitud(Middleware::OP_RESOLVER_HORARIO, entrada);
}

void PuertoSolverIpc::onRespuestaRecibida(const QJsonObject& respuesta)
{
    if (respuesta.value(QStringLiteral("op")).toString() != Middleware::OP_RESOLVER_HORARIO)
        return;
    if (!m_alResponder)
        return;

    const int code = respuesta.value(QStringLiteral("code")).toInt(Middleware::RESP_ERROR);
    const QJsonValue data = respuesta.value(QStringLiteral("data"));

    RespuestaSolver resultado;
    if (code == Middleware::RESP_EXITO) {
        resultado.ok = true;
        resultado.factible = true;
        resultado.salida = data.toObject();
    } else if (code == Middleware::RESP_SIN_SOLUCION) {
        resultado.ok = true;
        resultado.factible = false;
        resultado.error = data.toString();
    } else {
        resultado.ok = false;
        resultado.factible = false;
        resultado.error = data.isString()
                              ? data.toString()
                              : respuesta.value(QStringLiteral("message")).toString();
    }

    auto callback = std::move(m_alResponder);
    m_alResponder = nullptr;
    callback(resultado);
}

#endif // GESTOR_TIENE_BACKEND
