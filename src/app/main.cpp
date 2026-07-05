//realizado por: Paola peña 
//fecha: 2026/07/04

// Parches de compatibilidad para GCC 12 + Qt6
#include <utility>
#include <iterator>
#include <QTextFormat>

// Inclusiones de la aplicación
#include <QApplication>
#include "dialogs/login_dialog.hpp"
#include "views/main_window.hpp"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Instanciamos tu maqueta
    gestor::frontend::dialogs::LoginDialog login;
    
    if (login.exec() == QDialog::Accepted) {
        gestor::frontend::views::MainWindow window;
        window.show();
        return app.exec();
    }

    return 0;
}