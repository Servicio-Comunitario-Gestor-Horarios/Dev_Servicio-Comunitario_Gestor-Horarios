#pragma once

/*
 * @file MateriaDTO.hpp
 * @brief DTO para transferencia de datos de materia.
 */

#include <QString>

/*
 * @brief Objeto de transferencia de datos para materia.
 */
struct MateriaDTO
{
    int id;                 ///< Identificador único autoincrementable
    QString nombre;         ///< Nombre de la materia (único)
    QString descripcion;    ///< Descripción opcional de la materia
};