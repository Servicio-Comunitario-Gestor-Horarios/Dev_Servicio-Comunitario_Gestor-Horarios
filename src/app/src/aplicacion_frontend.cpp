#include "app/aplicacion_frontend.hpp"
#include "app/gestor_proceso_backend.hpp"
#include <frontend/aplicacion_frontend.hpp>
#include <QDebug>

/**
 * @brief Implementación del ciclo de vida del frontend.
 *
 * 1. Crea GestorProcesoBackend que lanza el backend como QProcess.
 * 2. Conecta señales para monitorear estado del backend.
 * 3. Delega en la librería frontend para la UI (login, ventanas).
 * 4. Detiene el backend al salir.
 */
int ejecutarAplicacionFrontend(int argc, char *argv[])
{
    // QApplication ya fue creada en main.cpp

    GestorProcesoBackend gestor;
    gestor.iniciar();

    QObject::connect(&gestor, &GestorProcesoBackend::backendListo, []() {
        qDebug() << "Frontend: backend listo";
    });
    QObject::connect(&gestor, &GestorProcesoBackend::backendColapsado, []() {
        qWarning() << "Frontend: backend no disponible";
    });

    int resultado = ejecutarAplicacionFrontendImpl(argc, argv);

    gestor.detener();
    return resultado;
}
