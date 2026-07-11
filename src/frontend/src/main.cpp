#include <QApplication>
#include "logindialog.h"
#include "views/main_window.hpp" // Incluimos nuestra nueva ventana principal

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    // 1. Creamos y mostramos la ventana de inicio de sesión
    LoginDialog login;

    // 2. Si el usuario ingresa sus datos y se acepta el inicio de sesión...
    if (login.exec() == QDialog::Accepted) {
        // 3. Creamos y mostramos la ventana principal
        MainWindow w;
        w.show();

        // 4. Mantenemos la aplicación corriendo
        return a.exec();
    }

    // Si el usuario presiona "Cancelar" o cierra la ventana de login, la app se cierra.
    return 0;
}
