#pragma once

/**
 * @file calculo_eventos.hpp
 * @brief Funciones puras de coherencia temporal de los eventos de turno (RF-3, RF-4).
 *
 * Sin E/S y deterministas: la ventana de cada posición, la franja estimada, la
 * contención del intervalo, los solapes y el orden de registro. El servicio de
 * turnos las usa para validar el alta y la edición; la reducción de franjas y
 * los cambios de ventana del turno se apoyan en las mismas reglas.
 *
 * La ventana de la posición k va del fin de la franja k al inicio de la k+1; la
 * última llega al fin del turno (N5 de la 009). La franja estimada reparte
 * `minutos de ventana − Σ duraciones de los eventos` entre las franjas, de modo
 * que los eventos de una posición ocupan exactamente su hueco (plan §3).
 */

#include <QPair>
#include <QTime>
#include <QVector>

#include "backend/services/ServicioTurnosRecesos.hpp"

/// Minutos de franja que quedan al restar los eventos y repartir entre las franjas.
///
/// Guarda: devuelve 0 si no hay franjas (`numSlots <= 0`), si la ventana del
/// turno no es positiva o si el reparto entero no llega a un minuto; el servicio
/// rechaza entonces el encaje del evento.
int duracionFranjaEstimada(const TurnoDTO& turno);

/// Ventana de la posición `posicion` (1..`numSlots`): `(inicio, fin)`.
///
/// Devuelve un par de horas inválidas si la posición está fuera de rango o si
/// el turno no tiene una rejilla válida (`duracionFranjaEstimada` es 0).
QPair<QTime, QTime> ventanaDePosicion(const TurnoDTO& turno, int posicion);

/// Indica si el intervalo del evento cabe en la ventana de su posición.
///
/// Un evento sin horas no tiene intervalo que comprobar: devuelve `true`.
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
