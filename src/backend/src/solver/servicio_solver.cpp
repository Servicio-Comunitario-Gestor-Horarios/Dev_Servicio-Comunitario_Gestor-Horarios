#include "backend/solver/servicio_solver.hpp"

#include "backend/solver/solver.hpp"

#include <QJsonArray>

QJsonObject salidaConFasesSinOptimalidad(const QJsonObject& salida,
                                         const QStringList& fasesNoOptimas)
{
    if (fasesNoOptimas.isEmpty())
        return salida;

    QJsonObject copia = salida;
    QJsonObject metadata = copia.value(QStringLiteral("metadata")).toObject();
    QJsonArray fases;
    for (const QString& fase : fasesNoOptimas)
        fases.append(fase);
    metadata[QStringLiteral("fases_sin_optimalidad")] = fases;
    copia[QStringLiteral("metadata")] = metadata;
    return copia;
}

RespuestaSolver resolverEntradaSolver(const QJsonObject& entrada)
{
    RespuestaSolver respuesta;

    const Resultado<SolverConfig> config = SolverConfig::fromJson(entrada);
    if (!config.ok)
    {
        // Entrada fuera del contrato: es un error de entrada, no una infactibilidad.
        respuesta.desenlace = DesenlaceSolver::ErrorContrato;
        respuesta.error = config.mensajeError;
        return respuesta;
    }

    Solver solver;
    const Solver::ResultadoSolver resultado = solver.resolver(config.valor);

    // 020 (RF-2/RF-3): el desenlace del motor se propaga tal cual; `error` conserva su
    // diagnóstico (la causa del NoFactible o el texto del no concluyente).
    respuesta.desenlace = resultado.desenlace;
    respuesta.error = resultado.errores.join(QStringLiteral("\n"));

    switch (resultado.desenlace)
    {
        case DesenlaceSolver::Exito:
            respuesta.ok = true;
            respuesta.factible = true;
            // 020 (D8): la optimalidad por fase viaja en la metadata de la salida.
            respuesta.salida = salidaConFasesSinOptimalidad(resultado.resultado.toJson(),
                                                            resultado.fasesNoOptimas);
            break;

        case DesenlaceSolver::Infactible:
        case DesenlaceSolver::NoConcluyente:
            // Respuesta válida del motor sin horario: conserva su texto.
            respuesta.ok = true;
            respuesta.factible = false;
            break;

        case DesenlaceSolver::ErrorContrato:
        case DesenlaceSolver::SinRespuesta:
            respuesta.ok = false;
            respuesta.factible = false;
            break;
    }

    return respuesta;
}
