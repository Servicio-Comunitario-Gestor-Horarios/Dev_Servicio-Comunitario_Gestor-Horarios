#pragma once

/**
 * @file aplicacion_frontend.hpp
 * @brief Punto de entrada para el modo frontend de la aplicación.
 *
 * Inicializa la interfaz gráfica y el gestor de proceso backend.
 */

/// Núcleo de datos que consume la interfaz (declaración adelantada).
class NucleoDatos;

/**
 * @brief Ejecuta la aplicación en modo frontend.
 *
 * Abre la base de datos local (con migración y respaldo), deja el núcleo de
 * datos disponible para la interfaz (`nucleoDatosActivo()`), lanza el backend
 * como proceso hijo y muestra la interfaz gráfica (LoginDialog, MainWindow).
 *
 * La instancia única se comprueba antes, en `main.cpp`.
 *
 * @param argc Número de argumentos de línea de comandos.
 * @param argv Arreglo de argumentos de línea de comandos.
 * @return Código de salida de la aplicación (0 = éxito).
 */
int ejecutarAplicacionFrontend(int argc, char *argv[]);

/**
 * @brief Punto de entrada para la interfaz: núcleo de datos abierto al arrancar.
 *
 * Devuelve la fachada `NucleoDatos` construida por el arranque del cliente, que
 * el equipo de `src/frontend` consume en proceso para leer y escribir los
 * dominios. Devuelve `nullptr` si la base no está operativa (o si esta
 * configuración se compiló sin backend de persistencia).
 */
NucleoDatos* nucleoDatosActivo();

