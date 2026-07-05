#pragma once

/**
 * @file aplicacion_frontend.hpp
 * @brief Punto de entrada para el modo frontend de la aplicación.
 *
 * Inicializa la interfaz gráfica y el gestor de proceso backend.
 */

/**
 * @brief Ejecuta la aplicación en modo frontend.
 *
 * Crea el GestorProcesoBackend para lanzar el backend como proceso hijo,
 * conecta las señales de monitoreo y delega en la librería frontend
 * para mostrar la interfaz gráfica (LoginDialog, MainWindow).
 *
 * @param argc Número de argumentos de línea de comandos.
 * @param argv Arreglo de argumentos de línea de comandos.
 * @return Código de salida de la aplicación (0 = éxito).
 */
int ejecutarAplicacionFrontend(int argc, char *argv[]);
