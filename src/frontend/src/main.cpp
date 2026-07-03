#include <QApplication>
#include "logindialog.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    LoginDialog login;
    if (login.exec() == QDialog::Accepted) {
        // Aquí abrirías la ventana principal en el futuro.
        // Por ahora, solo mostramos un mensaje en consola.
        qDebug() << "Login exitoso (mock)";
        // app.exec() no es necesario porque el diálogo ya es modal.
        // Si hubiera ventana principal, se mostraría aquí.
    }

    return 0;
}
