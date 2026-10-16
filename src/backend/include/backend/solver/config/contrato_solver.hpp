#pragma once

/**
* @file contrato_solver.hpp
* @brief Funciones puras y constantes del contrato del solver
*
* Puras y deterministas: sin E/S, sin reloj y sin OR-Tools
*/

#include <QString>
#include <QVector>

#include "backend/solver/config/solver_config.hpp"

// Constantes del motor
namespace ContratoSolver {
    constexpr int PESO_HORAS = 1000;
    constexpr int COSTE_SUPLENTE = 100;
    constexpr int COSTE_MAX_SUPLENTE = 100;
}

int bloques(int horas, int duracion_minutos);

int bloquesAula(int horas_aula, int duracion_minutos);

QString claveCurso(int anio, const QString& seccion);

QVector<PlanMateriaConfig> materiasDelCurso(const SolverConfig& config, const CursoSolverConfig& curso);
