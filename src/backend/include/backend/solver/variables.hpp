#pragma once

/**
 * @file variables.hpp
 * @brief Definición de variables de decisión para el solver CP-SAT.
 *
 * Contiene la estructura VariablesSolver que almacena los mapas de BoolVar
 * utilizados por el solver para representar asignaciones de profesores
 * a materias, cursos, días y slots, así como asignación de aulas para
 * cursos móviles.
 */

#include <QMap>
#include <tuple>
#include "backend/solver/config/solver_config.hpp"
#include "ortools/sat/cp_model.h"

using namespace operations_research::sat;

/**
 * @brief Mapa de variables de decisión del solver.
 *
 * Contiene dos mapas de BoolVar:
 * - assignment: variables de asignación profesor-materia-curso-día-slot
 * - aulaAssignment: variables de asignación de aula para cursos móviles
 */
struct VariablesSolver {
    QMap<std::tuple<int, int, int, int, int>, BoolVar> assignment;  ///< [p][m][c][d][s]
    QMap<std::tuple<int, int, int, int>, BoolVar> aulaAssignment;   ///< [c][d][s][a]
};

/**
 * @brief Crea las variables de decisión BoolVar para el solver.
 *
 * Recorre cursos → materias → profesores y crea BoolVar SOLO donde:
 * 1. El profesor p tiene la materia m en materias_asignadas O materias_suplente
 * 2. El curso c tiene la materia m en cursos[c].materias
 * 3. El profesor p está disponible en (d, s)
 * 4. (d, s) está dentro del turno del curso
 *
 * Para aulas móviles (aula_fija == -1), crea aulaAssignment[c][d][s][a].
 *
 * @param model Modelo CP-SAT donde se crean las BoolVar
 * @param config Configuración completa del solver
 * @return Estructura VariablesSolver con ambos mapas poblados
 */
VariablesSolver crearVariables(
    CpModelBuilder& model,
    const SolverConfig& config
);