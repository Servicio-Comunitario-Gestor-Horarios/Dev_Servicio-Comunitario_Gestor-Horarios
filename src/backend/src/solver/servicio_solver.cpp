#include "backend/solver/servicio_solver.hpp"

#include "backend/solver/solver.hpp"

RespuestaSolver resolverEntradaSolver(const QJsonObject& entrada)
{
    RespuestaSolver respuesta;

    const Resultado<SolverConfig> config = SolverConfig::fromJson(entrada);
    if (!config.ok)
    {
        respuesta.ok = false;
        respuesta.error = config.mensajeError;
        return respuesta;
    }

    Solver solver;
    const Solver::ResultadoSolver resultado = solver.resolver(config.valor);

    respuesta.ok = true;
    if (!resultado.exito)
    {
        respuesta.factible = false;
        respuesta.error = resultado.errores.join(QStringLiteral("\n"));
        return respuesta;
    }

    respuesta.factible = true;
    respuesta.salida = resultado.resultado.toJson();
    return respuesta;
}
