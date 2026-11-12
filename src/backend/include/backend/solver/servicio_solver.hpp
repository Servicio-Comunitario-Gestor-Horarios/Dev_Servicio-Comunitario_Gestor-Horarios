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
#include <QStringList>

#include "backend/services/ServicioGeneracion.hpp"

/// Resuelve `entrada` y propaga el desenlace del cálculo (020, RF-2/RF-3):
/// entrada inválida → `ErrorContrato`; sin solución demostrada → `Infactible`; sin prueba →
/// `NoConcluyente`; con horario → `Exito` (en `salida`). `ok`/`factible` se rellenan como
/// campos derivados de compatibilidad.
RespuestaSolver resolverEntradaSolver(const QJsonObject& entrada);

/// Inyecta la optimalidad por fase en la metadata de la salida (020, D8): añade
/// `metadata.fases_sin_optimalidad` solo si hay fases. Pura: no muta `salida`.
///
/// Viaja en la salida del solver (no en el modelo de datos): sin fases la clave no se emite,
/// de modo que una resolución probada óptima no se marca como no óptima.
QJsonObject salidaConFasesSinOptimalidad(const QJsonObject& salida,
                                         const QStringList& fasesNoOptimas);
