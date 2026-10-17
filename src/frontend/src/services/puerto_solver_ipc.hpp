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

#include "backend/services/ServicioGeneracion.hpp"

class InternalClient;

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
