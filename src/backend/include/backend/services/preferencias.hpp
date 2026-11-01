#pragma once

/**
 * @file preferencias.hpp
 * @brief Modelo puro de las preferencias horarias del docente (RF-1, RF-2).
 *
 * Sin E/S: el sentido (preferida/evitada) con su forma persistida y la detección de
 * solape entre franjas. Los rangos contiguos **no** se solapan; `haySolape` compara solo
 * el rango horario, así que el llamador debe filtrar por día (el servicio consulta por
 * docente y día).
 */

#include <QString>

#include "backend/data/franja_horaria.hpp"

/// Sentido de una preferencia horaria: lo que el docente prefiere o evita.
enum class SentidoPreferencia
{
    Preferida,
    Evitada,
};

/// Forma persistida del sentido: «preferida» o «evitada» (vacía si el valor no es válido).
QString textoDeSentido(SentidoPreferencia sentido);

/// Lee la forma persistida del sentido; `false` si el texto no es uno conocido.
///
/// Solo escribe `sentido` cuando devuelve `true`.
bool sentidoDesdeTexto(const QString& texto, SentidoPreferencia* sentido);

/// Defensa ante un valor casteado fuera del enumerado (la base tiene además `CHECK`).
bool sentidoValido(SentidoPreferencia sentido);

/// ¿Se solapan los rangos horarios de dos franjas? Los contiguos no.
///
/// `a.inicio < b.fin && b.inicio < a.fin`: el fin de una igual al inicio de la otra no
/// cuenta como solape. No mira el día (lo filtra el llamador).
bool haySolape(const FranjaHoraria& a, const FranjaHoraria& b);
