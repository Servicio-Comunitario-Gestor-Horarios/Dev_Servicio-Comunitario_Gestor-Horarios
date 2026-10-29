#pragma once

/**
 * @file ConstructorEntradaSolver.hpp
 * @brief Construcción del JSON de entrada del solver (RF-1).
 *
 * Combina una instantánea de los dominios del cliente (entidades: docentes,
 * aulas, materias, planes, cursos y turnos con sus eventos) con los parámetros del
 * solver (preset: franja horaria, penalizaciones, planificación, generación y los
 * campos por docente como horas_requeridas/horas_aula, casados por nombre).
 *
 * Deriva `dimensiones` de los tamaños de los arrays y estampa
 * `meta.fecha_modificacion` con un instante inyectado. No toca la base de datos.
 * Es una función pura (sin E/S): su único efecto es devolver la entrada 2.0.
 *
 * Los tipos de aula viajan por su **nombre** (`AulaDTO::tipoAula` /
 * `MateriaDTO::tipoAula`). Si un aula o una materia de la instantánea no declara
 * tipo, la construcción **falla** (RF-1): no se rellena un tipo por defecto ni se
 * emite JSON.
 */

#include <QDateTime>
#include <QJsonObject>

#include "backend/resultado.hpp"
#include "backend/services/ServicioAula.hpp"
#include "backend/services/ServicioCursos.hpp"
#include "backend/services/ServicioMaterias.hpp"
#include "backend/services/ServicioPlanesEstudio.hpp"
#include "backend/data/plan_estudio.hpp"
#include "backend/services/ServicioProfesor.hpp"
#include "backend/services/ServicioTurnosEventos.hpp"
#include "backend/solver/config/solver_config.hpp"

/// Instantánea de los dominios del cliente que forman la entrada del solver.
struct DatosDominio
{
    QVector<ProfesorDTO> docentes;
    QVector<AulaDTO>     aulas;
    QVector<MateriaDTO>  materias;
    QVector<PlanEstudio> planes;   ///< planes compuestos (materias + horas)
    QVector<CursoDTO>    cursos;
    QVector<TurnoDTO>    turnos;   ///< incluye sus eventos
};

/// Construye la entrada del solver (13 secciones) de forma pura (RF-1).
///
/// `parametros` aporta lo que no es entidad: `franja_horaria`, `aula_fija` y, por
/// docente (casado por nombre), `horas_requeridas`, `horas_aula`, `turno`, `plan` y
/// `materias_suplente`. Las entidades (materias, aulas, cursos, docentes y turnos
/// con sus eventos) se toman de `datos`; la versión de la entrada es siempre 2.0.
///
/// `duracionMinutos` es la duración efectiva de la hora académica: **sobrescribe
/// siempre** la del preset (que es un campo derivado) y alinea con ella la rejilla
/// de disponibilidad (RF-3, RF-5). Se valida **después** de la sobrescritura: un
/// preset con 0, 5 o 45 no bloquea, pero una `duracionMinutos` fuera de 30–90 falla
/// sin emitir JSON.
///
/// Devuelve error (sin emitir JSON) si un aula o una materia de `datos` no declara
/// tipo de aula.
Resultado<QJsonObject> construirJsonEntrada(const DatosDominio& datos,
                                            const SolverConfig& parametros,
                                            int duracionMinutos,
                                            const QDateTime& ahora);

/// Fachada del constructor (equivalente a `construirJsonEntrada`).
class ConstructorEntradaSolver
{
public:
    static Resultado<QJsonObject> construir(const DatosDominio& datos,
                                            const SolverConfig& parametros,
                                            int duracionMinutos,
                                            const QDateTime& ahora);
};
