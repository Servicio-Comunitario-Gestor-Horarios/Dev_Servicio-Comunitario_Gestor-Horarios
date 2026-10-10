#include "app/aplicacion_frontend.hpp"
#include "app/gestor_proceso_backend.hpp"
#include "logindialog.h"
#include "views/main_window.hpp"

#include <QApplication>
#include <QDebug>
#include <QMessageBox>

#if defined(GESTOR_TIENE_BACKEND)
#include "backend/database/apertura_base_datos.hpp"
#include "backend/services/NucleoDatos.hpp"

#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

#include <memory>

namespace
{
    /// Conexión persistente del cliente y núcleo de datos que consume la UI.
    /// Viven durante toda la sesión y se exponen con `nucleoDatosActivo()`.
    std::unique_ptr<QSqlDatabase> g_conexion;
    std::unique_ptr<NucleoDatos> g_nucleo;

    /// Ruta por defecto de la base de datos del usuario.
    QString rutaBaseDatosPorDefecto()
    {
        const QString directorio =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(directorio);
        return QDir(directorio).filePath(QStringLiteral("gestor_horarios.db"));
    }
} // namespace
#endif

NucleoDatos* nucleoDatosActivo()
{
#if defined(GESTOR_TIENE_BACKEND)
    return g_nucleo.get();
#else
    return nullptr;
#endif
}

int ejecutarAplicacionFrontend(int argc, char *argv[])
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

#if defined(GESTOR_TIENE_BACKEND)
    // 1. Apertura de la base de datos (RF-1) con migración y respaldo (RF-4).
    //    La instancia única ya se comprobó en main() y su detección sigue activa
    //    mientras transcurre esta apertura (incluida la migración).
    const QString rutaBase = rutaBaseDatosPorDefecto();
    const AperturaBaseDatos::Resultado apertura = AperturaBaseDatos::abrir(rutaBase);

    if (!apertura.ok())
    {
        // El diálogo completo de recuperación (RF-4) corresponde al frontend;
        // aquí se informa la causa y no se arranca la interfaz.
        QMessageBox::critical(nullptr, QObject::tr("No se pudo abrir la base de datos"),
                              apertura.detalle);
        return 2;
    }

    // 2. Conexión persistente del cliente y núcleo de datos (RF-2).
    auto conexion = std::make_unique<QSqlDatabase>(
        QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                  QStringLiteral("cliente_gestor_horarios")));
    conexion->setDatabaseName(rutaBase);

    if (!conexion->open())
    {
        QMessageBox::critical(nullptr, QObject::tr("No se pudo abrir la base de datos"),
                              conexion->lastError().text());
        return 2;
    }

    {
        QSqlQuery pragma(*conexion);
        pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    }

    g_conexion = std::move(conexion);
    g_nucleo = std::make_unique<NucleoDatos>(*g_conexion);
#endif

    // 3. Backend como proceso hijo. Arranque asíncrono: los errores
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

    // 4. Login modal
    LoginDialog login;
    if (login.exec() != QDialog::Accepted) {
        return 1;
    }

    // 5. Ventana principal
    MainWindow ventana;
    ventana.show();

    return QApplication::exec();
}
