#pragma once

/**
 * @file solver_config.hpp
 * @brief Minimal solver configuration structs for export and CP-SAT.
 */

#include <QString>
#include <QVector>

struct MateriaSolverConfig {
    QString nombre;
};

struct ProfesorSolverConfig {
    QString nombre;
};

struct AulaSolverConfig {
    QString nombre;
};

struct SolverConfig {
    QVector<MateriaSolverConfig> materias;
    QVector<ProfesorSolverConfig> profesores;
    QVector<AulaSolverConfig> aulas;
};
