#include <frontend/aplicacion_frontend.hpp>
#include "logindialog.h"
#include <QApplication>
#include <QDebug>

/**
 * @brief Implementación de la interfaz gráfica del frontend.
 *
 * Flujo:
 * 1. Crea (o usa) la QApplication existente.
 * 2. Muestra el LoginDialog modal.
 * 3. Si el login es aceptado, retorna 0 (éxito).
 * 4. Si se cancela, retorna 1.
 *
 * @note En el build integrado, QApplication ya fue creada en main.cpp.
 *       En FRONTEND_STANDALONE, la crea aquí mismo.
 */
int ejecutarAplicacionFrontendImpl(int argc, char *argv[])
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
