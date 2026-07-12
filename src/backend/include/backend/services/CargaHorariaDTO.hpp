#pragma once

/*
 * @file CargaHorariaDTO.hpp
 * @brief DTO para transferencia de datos de carga horaria.
 */

#include <QString>

/*
 * @brief Objeto de transferencia de datos para carga horaria.
 */
struct CargaHorariaDTO
{
    int id;                     ///< Identificador único autoincrementable
    QString planCodigo;         ///< Código del plan asociado
    int materiaId;              ///< ID de la materia
    int año;                    ///< Año o semestre (1-6)
    int horasSemanales;         ///< Horas semanales de la materia
};