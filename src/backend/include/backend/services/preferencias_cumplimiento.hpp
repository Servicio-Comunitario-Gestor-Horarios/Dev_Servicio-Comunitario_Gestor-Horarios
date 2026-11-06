#pragma once

/**
 * @file preferencias_cumplimiento.hpp
 * @brief Estado por preferencia y cumplimiento agregado (spec 013, RF-8).
 *
 * Puro: sin E/S, sin reloj y sin OR-Tools. Recibe la configuración —con las preferencias
 * intervinientes ya proyectadas—, el horario devuelto por el solver y las preferencias no
 * aplicadas de la construcción, y asigna a cada preferencia **un** estado con la precedencia
 * **no aplicada (con su motivo) → incumplida → respetada**.
 *
 * El contrato de salida del solver no cambia: este análisis se hace en el backend sobre el
 * horario y la proyección ya calculada.
 */

#include <QString>
#include <QVector>

#include "backend/data/horario_salida.hpp"
#include "backend/services/preferencias.hpp"
#include "backend/services/proyeccion_preferencias.hpp"
#include "backend/solver/config/solver_config.hpp"

/// Estado de una preferencia de la generación.
enum class EstadoPreferencia
{
    NoAplicada,   ///< No intervino; lleva su único motivo.
    Incumplida,   ///< Intervino y el horario no la respeta.
    Respetada,    ///< Intervino y el horario la respeta.
};

/// Una clase que incumple una preferencia evitada: curso (`anio|seccion`), materia y franja.
struct ClaseIncumplida
{
    QString curso;
    int     materia = -1;
    int     slot = -1;
};

/// Estado de una preferencia. **Un solo numerado de día en el resultado:** el de **dominio** de
/// la 012 (0 = domingo … 6 = sábado); el día de la generación no sale de este módulo.
struct PreferenciaEstado
{
    int               docente = -1;
    int               dia = -1;   ///< numeración de DOMINIO (0 = domingo … 6 = sábado)
    QVector<int>      slot;       ///< franjas alcanzadas (vacío en las no aplicadas)
    QString           sentido;    ///< "preferida" | "evitada"
    EstadoPreferencia estado = EstadoPreferencia::Respetada;
    MotivoPreferencia motivo = MotivoPreferencia::Ninguno;   ///< solo en las no aplicadas
    QVector<ClaseIncumplida> clasesIncumplidas;              ///< solo en evitadas incumplidas
};

/// Cumplimiento por preferencia y agregado: los tres totales.
struct CumplimientoPreferencias
{
    QVector<PreferenciaEstado> porPreferencia;
    int respetadas = 0;
    int incumplidas = 0;
    int noAplicadas = 0;
};

/// Analiza el estado de cada preferencia de la generación (RF-8).
///
/// `config.preferencias` son las **intervinientes** ya proyectadas (día de la generación) y
/// `noAplicadas` las que no intervinieron (con su franja de dominio y su único motivo). Una
/// evitada es incumplida si hay alguna clase **de su docente** en sus franjas (y se listan esas
/// clases); una preferida es incumplida si no hay ninguna. Motivo `Ninguno` salvo en las no
/// aplicadas. Resultado en orden canónico por `(docente, día, sentido, franjas)`.
CumplimientoPreferencias analizarPreferencias(const SolverConfig& config,
                                              const HorarioSalida& horario,
                                              const QVector<PreferenciaNoAplicada>& noAplicadas);
