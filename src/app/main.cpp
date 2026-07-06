#include <QCoreApplication>
#include <QApplication>
#include <cstring>
#include <cstdio>

// Declaraciones de los módulos de aplicación
int ejecutarAplicacionFrontend(int argc, char *argv[]);
int ejecutarAplicacionBackend(int argc, char *argv[]);

/**
 * @brief Punto de entrada único de la aplicación Gestor-Horarios.
 *
 * Modos de operación:
 * - `--backend`: Ejecuta solo el servidor backend (headless, sin UI).
 * - `--version`: Muestra la versión del programa y termina.
 * - `--help`:   Muestra el mensaje de uso y termina.
 * - (default):  Ejecuta el modo frontend con interfaz gráfica,
 *               que a su vez lanza el backend como proceso hijo.
 *
 * ## Build condicional
 * - Con `BUILD_BACKEND=ON`: el modo backend está disponible.
 * - Con `BUILD_BACKEND=OFF`: solo modo frontend (sin OR-Tools).
 *
 * @param argc Número de argumentos de línea de comandos.
 * @param argv Arreglo de argumentos de línea de comandos.
 * @return Código de salida del proceso.
 */
int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--backend") == 0) {
            QCoreApplication app(argc, argv);
            return ejecutarAplicacionBackend(argc, argv);
        }
        if (std::strcmp(argv[i], "--version") == 0) {
            std::printf("Gestor-Horarios v%s\n", PROJECT_VERSION);
            return 0;
        }
        if (std::strcmp(argv[i], "--help") == 0) {
            std::printf("Uso: gestor-horarios [--backend] [--version] [--help]\n");
            return 0;
        }
    }

    // Modo frontend (default): UI + lanzamiento automático del backend
    QApplication app(argc, argv);
    return ejecutarAplicacionFrontend(argc, argv);
}
