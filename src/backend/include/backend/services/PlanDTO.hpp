#pragma once

/*
 * @file PlanDTO.hpp
 * @brief DTO para transferencia de datos de plan de estudio.
 */

#include <QString>

/*
 * @brief Objeto de transferencia de datos para plan de estudio.
 */
struct PlanDTO
{
    QString codigo;         ///< Código único del plan (PK)
    QString descripcion;    ///< Descripción del plan
    bool activo;            ///< Indica si el plan está activo
};