#pragma once

/**
 * @file resultado.hpp
 * @brief Tipo de resultado para operaciones de servicios.
 */

#include <QString>
#include <QVariant>

/**
 * @brief Envuelve el resultado de una operación de servicio.
 * @tparam T Tipo del valor de retorno en caso de éxito.
 */
template<typename T>
struct Resultado {
    bool ok = false;           ///< Indica si la operación fue exitosa
    T valor{};                 ///< Valor de retorno (válido si ok == true)
    QString mensajeError;      ///< Mensaje de error (válido si ok == false)
    int codigoError = 0;       ///< Código de error adicional

    /** @brief Crea un resultado exitoso. */
    static Resultado<T> exito(const T& val) {
        Resultado<T> res;
        res.ok = true;
        res.valor = val;
        return res;
    }

    /** @brief Crea un resultado de error. */
    static Resultado<T> error(const QString& msg, int codigo = -1) {
        Resultado<T> res;
        res.ok = false;
        res.mensajeError = msg;
        res.codigoError = codigo;
        return res;
    }
};