#pragma once

/**
 * @file ServicioConfiguracionHoraria.hpp
 * @brief Servicio de la configuración global de la hora académica (RF-1, RF-2).
 *
 * Única pieza que escribe en `ConfiguracionHoraria`: una sola fila (`id = 1`),
 * 35 minutos por defecto y rango 30–90. La validación del rango es la de
 * `duracion_horaria::errorDeRango` (punto único, RNF-4); la creación del esquema
 * deja la fila por defecto, que no es una edición de usuario.
 */

#include <QString>

#include <QSqlDatabase>

#include "backend/resultado.hpp"

/**
 * @brief Valor efectivo de la duración de la hora académica y su origen.
 */
struct ConfiguracionHoraria
{
    int duracionMinutos = 0; ///< Duración efectiva, en minutos (30–90).
    bool porDefecto = false; ///< `true` si no hay fila y se usa el valor por defecto.
};

/**
 * @brief Consulta y edición de la configuración global de la hora académica.
 */
class ServicioConfiguracionHoraria
{
public:
    explicit ServicioConfiguracionHoraria(QSqlDatabase& db);

    /**
     * @brief Devuelve el valor efectivo y si proviene del defecto (RF-2).
     *
     * Sin fila devuelve `{35, porDefecto = true}` y **no** es error; solo falla
     * si la lectura de la base falla.
     */
    Resultado<ConfiguracionHoraria> consultar() const;

    /**
     * @brief Valida el rango y hace *upsert* de la fila `id = 1` (RF-1, RF-2).
     *
     * Una duración fuera de 30–90 devuelve el mensaje de
     * `duracion_horaria::errorDeRango` **sin escribir**; el valor anterior se
     * conserva. Es una sola sentencia: participa de la transacción del llamador.
     */
    Resultado<ConfiguracionHoraria> guardar(int duracionMinutos);

private:
    QSqlDatabase& m_db;
};
