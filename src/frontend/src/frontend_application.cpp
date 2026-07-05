#include <frontend/frontend_application.hpp>
#include "logindialog.h"
#include <QApplication>
#include <QDebug>

int run_frontend_app_impl(int argc, char *argv[])
{
    QApplication app(argc, argv);

    LoginDialog login;
    if (login.exec() == QDialog::Accepted)
    {
        qDebug() << "Login exitoso";
        return 0;
    }

    return 1;
}
