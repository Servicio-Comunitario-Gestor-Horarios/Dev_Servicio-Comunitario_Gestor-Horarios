#include "app/frontend_application.hpp"
#include "app/backend_process_manager.hpp"
#include <frontend/frontend_application.hpp>
#include <QApplication>
#include <QDebug>

int run_frontend_app(int argc, char *argv[]) {
    QApplication app(argc, argv);

    BackendProcessManager bpm;
    bpm.start();

    // Conectar señales del BPM para monitoreo
    QObject::connect(&bpm, &BackendProcessManager::backendReady, []() {
        qDebug() << "Frontend: backend listo";
    });
    QObject::connect(&bpm, &BackendProcessManager::backendCrashed, []() {
        qWarning() << "Frontend: backend no disponible";
    });

    int result = run_frontend_app_impl(argc, argv);

    bpm.stop();
    return result;
}
