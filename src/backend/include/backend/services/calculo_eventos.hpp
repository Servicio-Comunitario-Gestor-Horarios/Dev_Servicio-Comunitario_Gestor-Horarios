#pragma once

/**
 * @file calculo_eventos.hpp
 * @brief Funciones puras de coherencia temporal de los eventos de turno (RF-3, RF-4).
 *
 * Sin E/S y deterministas: la ventana de cada posición, las franjas utilizables y
 * el encaje con la hora académica, la contención del intervalo, los solapes y el
 * orden de registro. El servicio de turnos las usa para validar el alta y la
 * edición; la reducción de franjas y los cambios de ventana del turno se apoyan
 * en las mismas reglas.
 *
 * La franja de clase dura **D**, la hora académica de la configuración global
 * (spec 011): la ventana de la posición k va del fin de la franja k al inicio de
 * la k+1 y la última llega al fin del turno (N5 de la 009). El sobrante de la
 * ventana **no** se reparte entre las franjas.
 */

#include <QPair>
#include <QString>
#include <QTime>
#include <QVector>

#include "backend/services/ServicioTurnosEventos.hpp"

/// Minutos de la ventana del turno; 0 si las horas no forman un intervalo válido.
int minutosDeVentana(const TurnoDTO& turno);

/// Suma de las duraciones de los eventos del turno (las no positivas no suman).
int tiempoDeEventos(const TurnoDTO& turno);

/// Franjas que caben en el turno con una hora académica de `duracionMinutos`.
///
/// Delega en `duracion_horaria::franjasUtilizables` (punto único, RF-4). Devuelve
/// 0 si no cabe ninguna franja completa o si la duración no es positiva.
int franjasUtilizables(const TurnoDTO& turno, int duracionMinutos);

/// Minutos que faltan para que el turno encaje con `duracionMinutos`; 0 si cabe.
///
/// Delega en `duracion_horaria::deficitDeEncaje` (RF-4).
int deficitDeEncaje(const TurnoDTO& turno, int duracionMinutos);

/// Mensaje único del encaje del turno con la hora académica, o vacío si cabe.
///
/// Nombra el turno, la duración y el déficit, y dice qué ajustar (RNF-1). Si la
/// duración está fuera de rango devuelve el mensaje de
/// `duracion_horaria::errorDeRango`; si el turno no declara ventana pide declarar
/// sus horas.
QString errorDeEncaje(const TurnoDTO& turno, int duracionMinutos);

/// Ventana de la posición `posicion` (1..`numSlots`) con ancho de franja D.
///
/// Devuelve un par de horas inválidas si la posición está fuera de rango, si D no
/// es positiva o si el turno no tiene una ventana válida.
QPair<QTime, QTime> ventanaDePosicion(const TurnoDTO& turno, int posicion,
                                      int duracionMinutos);

/// Indica si el intervalo del evento cabe en la ventana de su posición con ancho D.
///
/// Un evento sin horas no tiene intervalo que comprobar: devuelve `true`.
bool eventoDentroDeVentana(const EventoDTO& evento, const TurnoDTO& turno, int posicion,
                           int duracionMinutos);

/// Minutos de franja que quedan al restar los eventos y repartir entre las franjas.
///
/// **Legado temporal (T4–T8): se retira en T9.** La 011 sustituye este reparto por
/// la franja de ancho D; solo lo conservan las sobrecargas antiguas de
/// `ventanaDePosicion`/`eventoDentroDeVentana`, que delegan en las nuevas. No se
/// añaden llamadores nuevos.
int duracionFranjaEstimada(const TurnoDTO& turno);

/// **Legado temporal (T4–T8): se retira en T9.**
/// Delega en `ventanaDePosicion(turno, posicion, duracionFranjaEstimada(turno))`.
QPair<QTime, QTime> ventanaDePosicion(const TurnoDTO& turno, int posicion);

/// **Legado temporal (T4–T8): se retira en T9.**
/// Delega en `eventoDentroDeVentana(evento, turno, posicion, duracionFranjaEstimada(turno))`.
bool eventoDentroDeVentana(const EventoDTO& evento, const TurnoDTO& turno, int posicion);

/// Indica si dos eventos con horas se pisan o van en orden cronológico invertido.
///
/// Recorre los eventos por `(posición, orden, id)` y exige que cada intervalo
/// empiece cuando menos donde terminó el más tardío de los anteriores. Los
/// eventos sin horas se ignoran.
bool haySolapeCronologico(const QVector<EventoDTO>& eventos);

/// Orden que corresponde al siguiente evento de `posicion`: máximo + 1, o 1.
int ordenSiguiente(const QVector<EventoDTO>& eventos, int posicion);

/// Eventos del turno que quedan después de la franja `numSlots`.
QVector<EventoDTO> eventosFueraDeRango(const TurnoDTO& turno, int numSlots);
