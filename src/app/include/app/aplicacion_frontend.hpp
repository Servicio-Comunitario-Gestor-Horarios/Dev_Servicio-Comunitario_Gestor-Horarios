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
 * Compone el arranque del cliente: crea el contexto de base de datos
 * (`ContextoBaseDatos`, que abre/migra/respalda y construye el `NucleoDatos`),
 * inyecta el núcleo de datos en la ventana principal, lanza el backend como
 * proceso hijo y muestra la interfaz gráfica (LoginDialog, MainWindow).
 *
 * El ciclo de vida de la base ya no vive aquí: lo encapsula el módulo `datos`,
 * y la interfaz lo recibe por inyección (sin singleton global).
 *
 * La instancia única se comprueba antes, en `main.cpp`.
 *
 * @param argc Número de argumentos de línea de comandos.
 * @param argv Arreglo de argumentos de línea de comandos.
 * @return Código de salida de la aplicación (0 = éxito).
 */
int ejecutarAplicacionFrontend(int argc, char *argv[]);
