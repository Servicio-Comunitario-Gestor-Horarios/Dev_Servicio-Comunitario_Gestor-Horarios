#pragma once

/**
 * @file ConstructorEntradaSolver.hpp
 * @brief Construcción del JSON de entrada del solver (RF-1).
 *
 * Combina una instantánea de los dominios del cliente (entidades: docentes,
 * aulas, materias, planes, cursos y turnos/recesos) con los parámetros del solver
 * (preset: franja horaria, penalizaciones, planificación, generación y los campos
 * por docente como horas_requeridas/horas_aula, casados por nombre).
 *
 * Deriva `dimensiones` de los tamaños de los arrays y estampa
 * `meta.fecha_modificacion` con un instante inyectado. No toca la base de datos.
 * Es una función pura (sin E/S): su único efecto es devolver el JSON.
 */

#include <QDateTime>
#include <QJsonObject>

#include "backend/services/ServicioAula.hpp"
#include "backend/services/ServicioCursos.hpp"
#include "backend/services/ServicioMaterias.hpp"
#include "backend/services/ServicioPlanesEstudio.hpp"
#include "backend/services/ServicioProfesor.hpp"
#include "backend/services/ServicioTurnosRecesos.hpp"
#include "backend/solver/config/solver_config.hpp"

/// Instantánea de los dominios del cliente que forman la entrada del solver.
struct DatosDominio
{
    QVector<ProfesorDTO> docentes;
    QVector<AulaDTO>     aulas;
    QVector<MateriaDTO>  materias;
    QVector<PlanDTO>     planes;
    QVector<CursoDTO>    cursos;
    QVector<TurnoDTO>    turnos;   ///< incluye sus recesos
};

/// Construye el JSON de entrada (13 secciones) de forma pura.
///
/// `parametros` aporta lo que no es entidad: `version`, `franja_horaria`,
/// `planificacion`, `generacion`, `penalizaciones` y, por docente (casado por
/// nombre), `horas_requeridas`, `horas_aula`, `turno`, `plan` y
/// `materias_suplente`. Las entidades (materias, aulas, cursos, docentes y
/// turnos/recesos) se toman de `datos`.
QJsonObject construirJsonEntrada(const DatosDominio& datos,
                                 const SolverConfig& parametros,
                                 const QDateTime& ahora);

/// Fachada del constructor (equivalente a `construirJsonEntrada`).
class ConstructorEntradaSolver
{
public:
    static QJsonObject construir(const DatosDominio& datos,
                                 const SolverConfig& parametros,
                                 const QDateTime& ahora);
};
