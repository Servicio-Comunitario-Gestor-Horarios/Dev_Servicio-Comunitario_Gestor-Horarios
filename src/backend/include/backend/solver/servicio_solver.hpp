#pragma once

/**
 * @file servicio_solver.hpp
 * @brief Resolución de una entrada del solver (RF-1, RF-2).
 *
 * Adapta el JSON de entrada del contrato al motor CP-SAT y devuelve la respuesta
 * tipada (`RespuestaSolver`) que el proceso de cálculo envía por IPC. No accede a
 * la base de datos.
 */

#include <QJsonObject>

#include "backend/services/ServicioGeneracion.hpp"

/// Resuelve `entrada`. Si el JSON no es válido → `ok=false`; si no hay solución
/// factible → `ok=true, factible=false`; si hay solución → `factible=true` con el
/// HorarioSalida JSON en `salida`.
RespuestaSolver resolverEntradaSolver(const QJsonObject& entrada);
