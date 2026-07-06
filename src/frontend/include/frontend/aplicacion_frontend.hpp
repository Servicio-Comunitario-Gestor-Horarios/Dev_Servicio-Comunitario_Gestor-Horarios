#pragma once

/**
 * @file aplicacion_frontend.hpp
 * @brief Interfaz de la librería frontend.
 *
 * Esta librería contiene la lógica de la interfaz gráfica (LoginDialog,
 * MainWindow) y se compila como librería estática en el build integrado
 * o como ejecutable independiente con FRONTEND_STANDALONE=ON.
 */

/**
 * @brief Implementación concreta de la interfaz gráfica.
 *
 * Crea QApplication (si no existe ya), muestra el LoginDialog,
 * y retorna el código de salida.
 *
 * @param argc Número de argumentos de línea de comandos.
 * @param argv Arreglo de argumentos de línea de comandos.
 * @return 0 si el login fue exitoso, 1 si se canceló.
 */
int ejecutarAplicacionFrontendImpl(int argc, char *argv[]);
