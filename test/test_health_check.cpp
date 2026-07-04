#include <QCoreApplication>
#include <QTimer>
#include <QDebug>

#include <middleware/internalserver.h>
#include <middleware/internalclient.h>

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    // 1. Inicializar el Servidor IPC
    InternalServer server;
    if (!server.start()) {
       
        qCritical() << " ERROR CRÍTICO: El Servidor Middleware IPC no pudo inicializarse correctamente.";
        return -1;
    }

    // 2. Test Unitario Automatizado de extremo a extremo (End-to-End)
    InternalClient client;

    // Conectamos la respuesta del cliente para verificar el éxito del Health-Check
    QObject::connect(&client, &InternalClient::healthCheckResponseReceived, [](bool success) {
        qDebug() << "\n==================================================";
        if (success) {
            qDebug() << " [TEST UNITARIO]: ¡ÉXITO COMPLETO!";
            qDebug() << "    -> El Health-Check funciona de extremo a extremo.";
        } else {
            qCritical() << " [TEST UNITARIO]: ¡FALLÓ!";
        }
        qDebug() << "==================================================\n";
    });

    // Esperar 1 segundo tras encender el servidor para lanzar la prueba automáticamente
    QTimer::singleShot(1000, &client, &InternalClient::sendHealthCheck);

    return a.exec();
}
