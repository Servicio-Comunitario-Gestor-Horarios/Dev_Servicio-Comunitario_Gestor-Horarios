#include "app/aplicacion_frontend.hpp"
#include "app/gestor_proceso_backend.hpp"
#include "logindialog.h"
#include "views/main_window.hpp"
#include <QApplication>
#include <QDebug>
#include <QMessageBox>

int ejecutarAplicacionFrontend(int argc, char *argv[])
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

    // 1. Backend como proceso hijo. Arranque asíncrono: los errores
    //    se manejan por señales, no por valor de retorno.
    GestorProcesoBackend gestor;

    QObject::connect(&gestor, &GestorProcesoBackend::backendListo, []() {
        qDebug() << "[frontend] Backend listo";
    });
    QObject::connect(&gestor, &GestorProcesoBackend::backendColapsado, []() {
        QMessageBox::critical(nullptr, "Backend",
                              "El backend dejó de responder.");
    });

    gestor.iniciar();

    // 2. Login modal
    LoginDialog login;
    if (login.exec() != QDialog::Accepted) {
        return 1;
    }

    // 3. Ventana principal
    MainWindow ventana;
    ventana.show();

    return QApplication::exec();
}
