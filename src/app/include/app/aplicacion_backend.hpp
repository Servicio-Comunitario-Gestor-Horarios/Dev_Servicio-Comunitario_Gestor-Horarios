#pragma once

/**
 * @file aplicacion_backend.hpp
 * @brief Punto de entrada para el modo backend de la aplicación.
 *
 * Inicializa el servidor IPC (InternalServer) y queda a la espera
 * de conexiones desde el frontend.
 */

/**
 * @brief Ejecuta la aplicación en modo backend (headless).
 *
 * Crea QCoreApplication, inicia InternalServer para recibir conexiones IPC,
 * y entra en el bucle de eventos. No muestra ninguna interfaz gráfica.
 *
 * @param argc Número de argumentos de línea de comandos.
 * @param argv Arreglo de argumentos de línea de comandos.
 * @return Código de salida (0 = éxito, 1 = error al iniciar servidor).
 */
int ejecutarAplicacionBackend(int argc, char *argv[]);
