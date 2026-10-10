#include "app/aplicacion_frontend.hpp"
#include "app/gestor_proceso_backend.hpp"
#include "logindialog.h"
#include "views/main_window.hpp"

#include <QApplication>
#include <QDebug>
#include <QMessageBox>

#if defined(GESTOR_TIENE_BACKEND)
#include "datos/ContextoBaseDatos.hpp"
#endif

int ejecutarAplicacionFrontend(int argc, char *argv[])
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

#if defined(GESTOR_TIENE_BACKEND)
    // 1. Ciclo de vida de la base del cliente (RF-1, RF-2), encapsulado en
    //    `ContextoBaseDatos` (módulo `datos`): abre/migra/respalda y construye
    //    el núcleo de datos. La instancia única ya se comprobó en main() y su
    //    detección sigue activa mientras transcurre esta apertura (incluida la
    //    migración).
    ContextoBaseDatos contexto;
    const AperturaBaseDatos::Resultado apertura =
        contexto.abrir(ContextoBaseDatos::rutaPorDefecto());

    if (!apertura.ok())
    {
        // El diálogo completo de recuperación (RF-4) corresponde al frontend;
        // aquí se informa la causa y no se arranca la interfaz.
        QMessageBox::critical(nullptr, QObject::tr("No se pudo abrir la base de datos"),
                              apertura.detalle);
        return 2;
    }
#endif

    // 2. Backend como proceso hijo. Arranque asíncrono: los errores
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

    // 3. Login modal
    LoginDialog login;
    if (login.exec() != QDialog::Accepted) {
        return 1;
    }

    // 4. Ventana principal, con el núcleo de datos inyectado por el arranque.
    MainWindow ventana;
#if defined(GESTOR_TIENE_BACKEND)
    ventana.setNucleoDatos(&contexto.nucleo());
#endif
    ventana.show();

    return QApplication::exec();
}
