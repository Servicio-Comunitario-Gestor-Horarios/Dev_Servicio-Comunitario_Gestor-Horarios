#include "services/puerto_solver_ipc.hpp"

#if defined(GESTOR_TIENE_BACKEND)

#include <middleware/internalclient.h>
#include <middleware/messages.h>

namespace {

/// Texto del `data` o del campo `message` de la respuesta, si lo traen.
QString textoDeLaRespuesta(const QJsonObject& respuesta, const QJsonValue& data)
{
    if (data.isString())
        return data.toString();
    return respuesta.value(QStringLiteral("message")).toString();
}

} // namespace

DesenlaceSolver desenlaceParaCodigo(std::optional<int> codigo)
{
    if (!codigo.has_value())
        return DesenlaceSolver::ErrorContrato;   // respuesta sin `code`: no es válida

    switch (*codigo) {
        case Middleware::RESP_EXITO:
            return DesenlaceSolver::Exito;
        case Middleware::RESP_SIN_SOLUCION:
            return DesenlaceSolver::Infactible;
        case Middleware::RESP_NO_CONCLUYENTE:
            return DesenlaceSolver::NoConcluyente;
        case Middleware::RESP_INVALIDO:
            return DesenlaceSolver::ErrorContrato;
        case Middleware::RESP_ERROR:
        case Middleware::RESP_TIEMPO_AGOTADO:
            // Socket caído o plazo agotado: «sin respuesta», nunca contrato incumplido.
            return DesenlaceSolver::SinRespuesta;
        default:
            return DesenlaceSolver::ErrorContrato;
    }
}

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
        // Sin conexión con el proceso: «sin respuesta», no un contrato incumplido.
        RespuestaSolver respuesta;
        respuesta.desenlace = DesenlaceSolver::SinRespuesta;
        respuesta.error = QStringLiteral("No hay conexión con el proceso de cálculo; no se puede"
                                         " afirmar que no exista solución.");
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

    const QJsonValue codigoJson = respuesta.value(QStringLiteral("code"));
    const std::optional<int> codigo =
        codigoJson.isDouble() ? std::optional<int>(codigoJson.toInt()) : std::nullopt;
    const QJsonValue data = respuesta.value(QStringLiteral("data"));

    RespuestaSolver resultado;
    resultado.desenlace = desenlaceParaCodigo(codigo);
    const QString texto = textoDeLaRespuesta(respuesta, data);

    switch (resultado.desenlace) {
        case DesenlaceSolver::Exito:
            resultado.ok = true;
            resultado.factible = true;
            resultado.salida = data.toObject();
            break;

        case DesenlaceSolver::Infactible:
        case DesenlaceSolver::NoConcluyente:
            // Respuesta válida del motor sin horario: se conserva su texto.
            resultado.ok = true;
            resultado.factible = false;
            resultado.error = texto;
            break;

        case DesenlaceSolver::SinRespuesta: {
            resultado.ok = false;
            resultado.factible = false;
            if (!texto.isEmpty()) {
                resultado.error = texto;
            } else if (codigo == std::optional<int>(Middleware::RESP_TIEMPO_AGOTADO)) {
                resultado.error = QStringLiteral("El proceso de cálculo no respondió a tiempo; no"
                                                 " se puede afirmar que no exista solución.");
            } else {
                resultado.error = QStringLiteral("Se perdió la comunicación con el proceso de"
                                                 " cálculo; no se puede afirmar que no exista"
                                                 " solución.");
            }
            break;
        }

        case DesenlaceSolver::ErrorContrato:
            resultado.ok = false;
            resultado.factible = false;
            if (!texto.isEmpty()) {
                resultado.error = texto;
            } else if (!codigo.has_value()) {
                resultado.error = QStringLiteral("La respuesta del proceso de cálculo no corresponde"
                                                 " al contrato (sin código).");
            } else {
                resultado.error = QStringLiteral("La respuesta del proceso de cálculo no corresponde"
                                                 " al contrato.");
            }
            break;
    }

    auto callback = std::move(m_alResponder);
    m_alResponder = nullptr;
    callback(resultado);
}

#endif // GESTOR_TIENE_BACKEND
