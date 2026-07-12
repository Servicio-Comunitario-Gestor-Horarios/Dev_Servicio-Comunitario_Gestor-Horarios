#pragma once

#include <QString>

/*
 * @brief Representa el resultado de una operación.
 *
 * Contiene el estado de éxito o error de una operación del backend,
 * el valor retornado y un mensaje descriptivo en caso de fallo.
 *
 * @tparam T Tipo de dato devuelto.
 */
template<typename T>
struct Resultado
{
    /*
     * @brief Indica si la operación fue exitosa.
     */
    bool exito = false;

    /*
     * @brief Valor retornado.
     *
     * Solo es válido cuando exito == true.
     */
    T valor{};

    /*
     * @brief Mensaje de error.
     */
    QString error;

    /*
     * @brief Construye un resultado exitoso.
     *
     * @param valor Valor retornado.
     * @return Resultado<T>
     */
    static Resultado<T> correcto(const T& valor)
    {
        Resultado<T> resultado;
        resultado.exito = true;
        resultado.valor = valor;
        return resultado;
    }

    /*
     * @brief Construye un resultado con error.
     *
     * @param mensaje Mensaje descriptivo.
     * @return Resultado <T>*/
     
     static Resultado<T> fallo(const QString& mensaje)
    {
        Resultado<T> resultado;
        resultado.exito = false;
        resultado.error = mensaje;
        return resultado;
    }
};