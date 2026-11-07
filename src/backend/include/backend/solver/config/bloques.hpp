#pragma once

/**
 * @file bloques.hpp
 * @brief Funciones puras de la rejilla de bloques de dos horas (spec 014, RF-1–RF-6).
 *
 * Sin E/S, sin reloj y sin OR-Tools: trabajan sobre la configuración del solver y las
 * franjas ocupadas de un curso o de una materia. Los días son los de la GENERACIÓN
 * (0 = lunes); la conversión a día de dominio (0 = domingo) es del informe.
 *
 * Nota de nombres: el espacio se llama `rejilla` porque `bloques` ya es la función de
 * conversión de horas del contrato (`int bloques(int, int)`) y C++ no admite un namespace
 * y una función con el mismo nombre en el mismo ámbito.
 */

#include <QMap>
#include <QSet>
#include <QString>
#include <QVector>

#include "backend/solver/config/contrato_solver.hpp"

namespace rejilla {
/// Tramo ocupado: secuencia máxima de franjas ocupadas dentro de un día y segmento.
struct Tramo {
    int dia = 0;        ///< Día de la generación (0 = lunes).
    int segmento = 0;   ///< Índice del segmento dentro del turno.
    int inicio = 0;     ///< Primera franja (slot) del tramo.
    int longitud = 0;   ///< Número de franjas consecutivas.
};

/// Par de franjas adyacentes: cubre `inicio` e `inicio + 1`.
struct Par {
    int dia = 0;
    int segmento = 0;
    int inicio = 0;
};

/// Pesos blandos por instancia (plan §4.2). La separación es blanda con prioridad alta; los
/// huecos al final son duros y no tienen peso.
struct PesosBlandos {
    int separacion = 1;    ///< `maxHuecos + 1`: domina a todos los huecos no-viernes.
    int huecoViernes = 1;  ///< Peso unitario de los huecos que no caen en viernes.
};

/// Segmentos contiguos del turno del curso `curso`, cortados por los eventos.
QVector<QVector<int>> segmentosDelCurso(const SolverConfig& config, int curso);

/// Tramos ocupados del curso: franjas consecutivas de `ocupadas` dentro de un mismo
/// día y segmento. Recorre los segmentos en orden y los tramos desde el inicio.
QVector<Tramo> tramosDeCurso(const SolverConfig& config, int curso,
                             const QSet<FranjaSemana>& ocupadas);

/// Pares de franjas adyacentes de cada tramo, emparejando DESDE EL INICIO. Si el tramo
/// tiene longitud impar, la última franja queda sin par (bloque de 1).
QVector<Par> paresDeCurso(const SolverConfig& config, int curso,
                          const QSet<FranjaSemana>& ocupadas);

/// Franjas de la materia que no forman parte de un bloque de dos horas suyo (está en un
/// bloque compartido o en un bloque de 1). Empareja sus franjas desde el inicio de cada
/// tramo; suma la longitud impar de cada tramo.
int franjasDescubiertasDeMateria(const SolverConfig& config, int curso,
                                 const QSet<FranjaSemana>& ocupadasMateria);

/// Suma de `franjasDescubiertasDeMateria` sobre las materias del curso.
int franjasDescubiertasDeCurso(const SolverConfig& config, int curso,
                               const QMap<int, QSet<FranjaSemana>>& ocupadasPorMateria);

/// Desvío de una materia: `máx(0, franjas descubiertas − h mod 2)`. Un bloque de 1 no
/// suma aquí si la materia es impar (su resto); `bloquesDeUno` lo cuenta aparte.
int desvioDeMateria(const SolverConfig& config, int curso, int materia,
                    const QSet<FranjaSemana>& ocupadasMateria);

/// Desvío único del curso: `Σ_m máx(0, franjas descubiertas_m − h_m mod 2)`. Es la única
/// métrica de agrupación (RF-6); en una solución factible vale 0.
int desvioDeCurso(const SolverConfig& config, int curso,
                  const QMap<int, QSet<FranjaSemana>>& ocupadasPorMateria);

/// Días de generación (0 = lunes) en los que la materia tiene al menos una franja ocupada,
/// ordenados y sin repetir.
QVector<int> diasDeMateria(const SolverConfig& config, int curso, int materia,
                           const QSet<FranjaSemana>& ocupadasMateria);

/// Nº de pares de apariciones en días consecutivos (los días deben venir ordenados y sin
/// repetir, como los devuelve `diasDeMateria`).
int incumplimientosSeparacion(const QVector<int>& dias);

/// Bloques de 1 del curso: franjas ocupadas que quedan sin par al emparejar los tramos
/// desde el inicio (longitudes impares). Es una comprobación del P4, NO un término del
/// desvío; en una solución factible vale 0.
int bloquesDeUno(const SolverConfig& config, int curso, const QSet<FranjaSemana>& ocupadas);

/// Número de apariciones esperadas de una materia de `horas`: `⌈horas/2⌉`.
int aparicionesEsperadas(int horas);

/// Tope de franjas descubiertas del curso: `Σ_m (h_m mod 2)`.
int cotaDeCurso(const SolverConfig& config, int curso);

/// Comprobación del tope AGREGADO del curso: `franjas descubiertas ≤ cota`. Es necesaria
/// pero no suficiente (el desvío 0 exige además que ninguna materia supere su propio
/// resto); la condición por materia es `desvioDeMateria(...) == 0`.
bool cumpleTopeDeCurso(const SolverConfig& config, int curso,
                       const QMap<int, QSet<FranjaSemana>>& ocupadasPorMateria);

/// Causa (5) detectable sobre la entrada (RF-10): total semanal del curso impar o alguna
/// materia con `⌈h/2⌉ > nº_días`. Devuelve true y el texto en español en `motivo`; si no
/// aplica, devuelve false y deja `motivo` vacío (admite `nullptr`).
bool causaCincoDeCurso(const SolverConfig& config, int curso, QString* motivo);

/// Pesos blandos por instancia: separación (`maxHuecos + 1`) y viernes (1).
PesosBlandos pesosBlandos(const SolverConfig& config);
} // namespace rejilla
