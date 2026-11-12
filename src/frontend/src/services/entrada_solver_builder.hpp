#pragma once

/**
 * @file entrada_solver_builder.hpp
 * @brief Composición de la entrada del solver a partir del dominio y de la UI,
 *        y traducción del HorarioSalida al formato de visualización (RF-1).
 *
 * Vive en el frontend porque es la interfaz quien posee el núcleo de datos y la
 * configuración elegida por el usuario. Reutiliza la lógica de dominio del
 * backend (`construirJsonEntrada`, contrato del solver) en lugar de duplicarla.
 *
 * Solo se compila cuando el backend está disponible (`GESTOR_TIENE_BACKEND`).
 */

#if defined(GESTOR_TIENE_BACKEND)

#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

#include "backend/data/horario_salida.hpp"
#include "backend/services/proyeccion_preferencias.hpp"
#include "services/generacion_tipos.hpp"

class NucleoDatos;

namespace GeneracionSolver {

/// Resultado de componer la entrada del solver.
struct Composicion {
    QJsonObject entrada;    ///< JSON de entrada listo para validar/enviar.
    ContextoHorario contexto; ///< Nombres/índices para traducir la salida.
    QStringList avisos;     ///< Advertencias no bloqueantes (datos incompletos).
    QString huella;         ///< Huella con las preferencias intervinientes (013, RF-6).
    QVector<PreferenciaNoAplicada> preferenciasNoAplicadas; ///< Sin emitir, con su motivo.
};

/**
 * @brief Compone la entrada del solver desde el dominio y la configuración.
 *
 * Lee del núcleo (docentes, aulas, materias, cursos, turnos) y compone los
 * planes que referencian los cursos; con eso y la configuración del usuario
 * arma el `DatosDominio` + `SolverConfig` que consume `construirEntradaCompleta`
 * (que embebe los planes, casa los cursos por año/sección, aplica el modo
 * global de aulas fijas y proyecta las preferencias de los datos). No modifica
 * la base.
 */
Composicion construirEntrada(const NucleoDatos& nucleo, const OpcionesUI& opciones);

/**
 * @brief Huella de la instantánea actual del dominio con las preferencias que intervienen.
 *
 * Compone la entrada —el mismo camino que la generación— y devuelve su huella
 * (spec 011 RF-6; spec 013 RF-6: solo las preferencias intervinientes, ya
 * proyectadas). Así la huella que se envía y la que se compara al recibir no
 * pueden divergir. Si la composición falla, cae al respaldo sin preferencias
 * con 35 min de duración.
 */
QString huellaActual(const NucleoDatos& nucleo, const OpcionesUI& opciones);

/**
 * @brief Carga horaria semanal sugerida por docente.
 *
 * Suma las horas de las materias que cada docente dicta, considerando los planes
 * configurados por curso y el año de cada curso. Sirve para precargar la tabla
 * de docentes con valores válidos.
 */
QMap<QString, int> horasSugeridasPorDocente(
    const NucleoDatos& nucleo,
    const QMap<QString, ParametrosCurso>& cursos);

/**
 * @brief Traduce un HorarioSalida al formato que consume la visualización.
 *
 * Produce un objeto con las claves "teachers" y "years" que entienden
 * `ScheduleVisualizationWidget` y sus diálogos, usando el `contexto` para pasar
 * de índices a nombres. El `HorarioSalida` identifica cada curso por su clave
 * (`año|sección`), que el contexto traduce a su nombre visible.
 */
QJsonObject aFormatoVisualizacion(const HorarioSalida& horario,
                                  const ContextoHorario& contexto);

} // namespace GeneracionSolver

#endif // GESTOR_TIENE_BACKEND
