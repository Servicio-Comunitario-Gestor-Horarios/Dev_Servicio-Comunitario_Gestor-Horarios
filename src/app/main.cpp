#include <QCoreApplication>
#include <QApplication>
#include <QMessageBox>
#include <cstring>
#include <cstdio>

#include "app/instancia_unica.hpp"

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

    // Instancia única (RF-5): la detección se activa antes de abrir la base de
    // datos y permanece viva durante todo el arranque (migración y diálogos).
    InstanciaUnica instanciaUnica;
    const InstanciaUnica::Resultado arranque = instanciaUnica.iniciar();

    if (arranque == InstanciaUnica::Resultado::Secundaria)
    {
        // Ya se enfocó la instancia existente: no se abre una segunda sesión.
        return 0;
    }

    if (arranque == InstanciaUnica::Resultado::SinAcuse)
    {
        QMessageBox::warning(
            nullptr, QObject::tr("Instancia ya en ejecución"),
            instanciaUnica.detalle().isEmpty()
                ? QObject::tr("No se pudo enfocar la instancia existente; no se"
                              " iniciará una segunda sesión.")
                : instanciaUnica.detalle());
        return 2;
    }

    // Somos la instancia principal: continuar el arranque del cliente (base de
    // datos y núcleo de datos), delegado en `ejecutarAplicacionFrontend`.
    return ejecutarAplicacionFrontend(argc, argv);
}
