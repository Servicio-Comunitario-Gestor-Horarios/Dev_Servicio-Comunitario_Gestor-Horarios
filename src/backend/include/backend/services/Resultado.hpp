#pragma once

/*
 * @file Resultado.hpp
 * @brief Definición del template Resultado para operaciones con estado.
 *
 * Este template se utiliza como envoltorio de retorno para operaciones
 * que pueden fallar, proporcionando información del error en caso de fallo.
 */

#include <QString>

/*
 * @brief Envoltorio para resultados de operaciones con estado de éxito/fallo.
 *
 * @tparam T Tipo del dato contenido en el resultado exitoso.
 */
template <typename T>
struct Resultado
{
    bool ok;        ///< Indica si la operación fue exitosa.
    T valor;        ///< Dato de retorno (válido solo si ok == true).
    QString error;  ///< Mensaje de error (válido solo si ok == false).

    /*
     * @brief Constructor para resultado exitoso.
     *
     * @param v Valor de retorno.
     *
     * @return Resultado con ok = true.
     */
    static Resultado<T> correcto(const T& v)
    {
        return {true, v, QString()};
    }

    /*
     * @brief Constructor para resultado fallido.
     *
     * @param e Mensaje de error.
     *
     * @return Resultado con ok = false.
     */
    static Resultado<T> fallo(const QString& e)
    {
        return {false, T(), e};
    }
};