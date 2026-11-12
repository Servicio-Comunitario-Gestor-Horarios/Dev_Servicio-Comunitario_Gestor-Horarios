#pragma once

/**
 * @file centro_sin_ef.hpp
 * @brief Fixture de aceptación de la 014 sin Educación Física (24 cursos, 34/36 h).
 *
 * Datos del plan §8: 31059 (1º–5º) y 31060 (1º–4º), 8 cursos de 34 h y 16 de 36 h, 12
 * aulas generales, 30 docentes y rejilla de 5 días × 16 franjas con turnos de 8 y un evento
 * tras la franja 2. Con la separación blanda de B5b el fixture es factible con los **30 docentes
 * reales** (el reparto round-robin del plan §8). Los cursos del plan 31060 llevan un sufijo de
 * sección («A60»…) para que la clave `año|sección` del contrato sea única.
 */

#include "backend/solver/config/solver_config.hpp"

SolverConfig fixtureCentroSinEf();
