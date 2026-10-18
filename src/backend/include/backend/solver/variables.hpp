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
#include "qmap.h"

using namespace operations_research::sat;

/**
 * @brief Mapa de variables de decisión del solver.
 *
 * Contiene:
 * - assignment         (x): profesor-materia-curso-dia-slot
 * - aulaAssignment     (y): aula de cursos moviles (solo si el modo no es aula fija)
 * - asignacionDocente  (z): docente activo por materia-curso (D1/D2)
 * - exceso             (e): bloques por encima del tope de horas de aula (S1)
 * - hay                (h): presencia de la materia en (m,c,d,seg,s) para S3
 * - isolated           (i): grupo de 1 (penaliza)

 */
struct VariablesSolver {
    QMap<std::tuple<int, int, int, int, int>, BoolVar> assignment;          ///< [p][m][c][d][s]
    QMap<std::tuple<int, int, int, int>, BoolVar> aulaAssignment;           ///< [c][d][s][a]
    QMap<std::tuple<int,int,int>, BoolVar> asignacionDocente;               ///< z: [p][c][m]
    QMap<int, IntVar> exceso;                                              ///< e[p]

    // --- S3: Agrupación de bloques ---
    // hay[m][c][d][seg][s] = 1 si la materia tiene clase en esa franja.
    // isolated[m][c][d][seg][s] = 1 si la franja es un grupo de 1 (penaliza 1).
    // longGroup[m][c][d][seg][s] = 1 si la franja es la 4.ª+ consecutiva (penaliza 1).
    QMap<std::tuple<int, int, int, int, int>, BoolVar> hay;                 ///< [m][c][d][seg][s]
    QMap<std::tuple<int, int, int, int, int>, BoolVar> isolated;            ///< [m][c][d][seg][s]
    QMap<std::tuple<int, int, int, int, int>, BoolVar> longGroup;           ///< [m][c][d][seg][s]

};

/**
 * @brief Crea las variables de decisión BoolVar para el solver.
 *
 * Recorre cursos -> materias (derivadas de su plan) -> profesores y crea `x` SOLO donde:
 * 1. p tiene la materia m (titular o suplente),
 * 2. p es elegible (plan/anio; D7/D13),
 * 3. p esta disponible en (d, s),
 * 4. (d, s) esta dentro del turno DEL CURSO (D11) y, si p declara turno, dentro del suyo (D6).
 * Crea `z[p][c][m]` para TODO candidato elegible por plan/anio (aunque no tenga ninguna `x`):
 * los recortes por turno/disponibilidad viven en las `x` y D3 (`Σx = bloques·z`) fuerza z=0, de
 * modo que la ausencia de candidato con disponibilidad hace INFEASIBLE a D1 (no se omite).
 * `y[c][d][s][a]` solo con el modo de aulas moviles (config.aula_fija == false).
 *
 * @param model Modelo CP-SAT donde se crean las BoolVar
 * @param config Configuración completa del solver
 * @return Estructura VariablesSolver con ambos mapas poblados
 */
VariablesSolver crearVariables(
    CpModelBuilder& model,
    const SolverConfig& config
);
