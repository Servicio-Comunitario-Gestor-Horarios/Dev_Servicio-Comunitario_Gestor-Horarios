#pragma once

/**
 * @file puerto_solver_ipc.hpp
 * @brief Implementación de `PuertoSolver` sobre el cliente IPC (RF-1).
 *
 * Traduce una generación (entrada JSON -> HorarioSalida JSON) al protocolo del
 * middleware (`solver_resolve`), correlacionando la respuesta por su `op` y
 * mapeando los códigos de respuesta a `RespuestaSolver`.
 *
 * Solo se compila cuando el backend está disponible (`GESTOR_TIENE_BACKEND`).
 */

#if defined(GESTOR_TIENE_BACKEND)

#include <QJsonObject>
#include <QObject>

#include <functional>
#include <optional>

#include "backend/services/ServicioGeneracion.hpp"
#include "backend/solver/desenlace_solver.hpp"

class InternalClient;

/// Mapeo puro de un código IPC a un desenlace del cálculo (020, D6).
///
/// `RESP_EXITO` → `Exito`, `RESP_SIN_SOLUCION` → `Infactible`,
/// `RESP_NO_CONCLUYENTE` → `NoConcluyente`, `RESP_INVALIDO` → `ErrorContrato`,
/// `RESP_ERROR`/`RESP_TIEMPO_AGOTADO` → `SinRespuesta` (socket caído o plazo agotado, **no** un
/// contrato incumplido). Una respuesta **sin `code`** no es válida → `ErrorContrato`.
///
/// Solo se aplica a la operación del solver; el resto de operaciones conserva el
/// comportamiento global de `InternalClient`.
DesenlaceSolver desenlaceParaCodigo(std::optional<int> codigo);

class PuertoSolverIpc : public QObject, public PuertoSolver
{
    Q_OBJECT

public:
    explicit PuertoSolverIpc(InternalClient* cliente, QObject* parent = nullptr);

    void resolver(const QJsonObject& entrada,
                  std::function<void(const RespuestaSolver&)> alResponder) override;

private slots:
    void onRespuestaRecibida(const QJsonObject& respuesta);

private:
    InternalClient* m_cliente = nullptr;
    std::function<void(const RespuestaSolver&)> m_alResponder;
};

#endif // GESTOR_TIENE_BACKEND
