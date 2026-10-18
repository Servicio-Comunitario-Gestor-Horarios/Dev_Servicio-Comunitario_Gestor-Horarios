#include "app/aplicacion_frontend.hpp"
#include "app/gestor_proceso_backend.hpp"
#include "app/instancia_unica.hpp"
#include "logindialog.h"
#include "views/main_window.hpp"

#include <QApplication>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>

#if defined(GESTOR_TIENE_BACKEND)
#include "backend/database/respaldo.hpp"
#include "datos/ContextoBaseDatos.hpp"

namespace
{
    /// Diálogo de fallo de apertura (RF-4, spec 001): ofrece «Reintentar»,
    /// «Restaurar respaldo…», «Crear base de datos nueva…» (con respaldo
    /// obligatorio antes de descartar, RF-6) y «Salir», y reintenta hasta que
    /// la apertura tenga éxito o el usuario salga.
    AperturaBaseDatos::Resultado abrirBaseConRecuperacion(ContextoBaseDatos& contexto)
    {
        const QString ruta = ContextoBaseDatos::rutaPorDefecto();
        AperturaBaseDatos::Resultado apertura = contexto.abrir(ruta);

        while (!apertura.ok())
        {
            QMessageBox mensaje(QMessageBox::Critical,
                                QObject::tr("No se pudo abrir la base de datos"),
                                apertura.detalle,
                                QMessageBox::NoButton);
            QPushButton* reintentar =
                mensaje.addButton(QObject::tr("Reintentar"), QMessageBox::AcceptRole);
            QPushButton* restaurar =
                mensaje.addButton(QObject::tr("Restaurar respaldo…"), QMessageBox::ActionRole);
            QPushButton* nuevaBase =
                mensaje.addButton(QObject::tr("Crear base de datos nueva…"),
                                  QMessageBox::DestructiveRole);
            mensaje.addButton(QObject::tr("Salir"), QMessageBox::RejectRole);
            mensaje.setDefaultButton(reintentar);
            mensaje.exec();

            if (mensaje.clickedButton() == reintentar)
            {
                apertura = contexto.abrir(ruta);
            }
            else if (mensaje.clickedButton() == restaurar)
            {
                // Elegir un respaldo existente: se valida integridad y versión
                // antes de sustituir la base actual (restaurarRespaldo).
                const QString origen = QFileDialog::getOpenFileName(
                    nullptr,
                    QObject::tr("Elegir un respaldo de la base de datos"),
                    QFileInfo(ruta).absolutePath(),
                    QObject::tr("Bases de datos (*.db *.sqlite *.sqlite3);;Todos los archivos (*)"));

                if (!origen.isEmpty())
                {
                    QString error;
                    if (!Respaldo::restaurarRespaldo(origen, ruta, &error))
                        QMessageBox::critical(nullptr,
                                              QObject::tr("No se pudo restaurar el respaldo"),
                                              error);
                }

                apertura = contexto.abrir(ruta);
            }
            else if (mensaje.clickedButton() == nuevaBase)
            {
                // RF-6: respaldo obligatorio antes de descartar; el descarte no
                // continúa sin confirmación explícita.
                const auto confirmar = QMessageBox::question(
                    nullptr,
                    QObject::tr("Crear base de datos nueva"),
                    QObject::tr("Se creará un respaldo de la base actual y después se descartará "
                                "para crear una vacía. Los datos actuales quedarán solo en el "
                                "respaldo. ¿Continuar?"),
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No);

                if (confirmar == QMessageBox::Yes)
                {
                    const Respaldo::ResultadoDescarte descarte =
                        Respaldo::descartarBaseYCrearNueva(ruta);

                    if (!descarte.ok())
                        QMessageBox::critical(nullptr,
                                              QObject::tr("No se pudo crear la base de datos nueva"),
                                              descarte.detalle);
                }

                apertura = contexto.abrir(ruta);
            }
            else
            {
                // «Salir» o cierre de la ventana: sin escribir en la base (RF-4).
                break;
            }
        }

        return apertura;
    }
} // namespace
#endif

namespace
{
    /// Mensaje para la caída del proceso de cálculo. El código `0xC0000374`
    /// (corrupción de memoria) es el síntoma en Windows de mezclar un binario
    /// Debug (`/MDd`) con las DLL de OR-Tools en release (`/MD`); la salida es
    /// generar con un build en Release.
    QString detalleColapsoBackend(int codigo)
    {
        QString detalle = QStringLiteral("El proceso de cálculo dejó de responder.");
        if (codigo != 0)
            detalle += QStringLiteral(" Código de salida: 0x%1.")
                           .arg(static_cast<quint32>(codigo), 8, 16, QLatin1Char('0'));
#if defined(Q_OS_WIN) && defined(_DEBUG)
        if (static_cast<quint32>(codigo) == 0xC0000374u)
            detalle += QStringLiteral(
                "\n\nEste ejecutable es de depuración (Debug) y las DLL de OR-Tools son de "
                "release, así que el motor corrompe la memoria al resolver. Compila y ejecuta "
                "la aplicación en Release (CMAKE_BUILD_TYPE=Release) para generar horarios.");
#endif
        return detalle;
    }
} // namespace

int ejecutarAplicacionFrontend(int argc, char *argv[], InstanciaUnica *instanciaUnica)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);

#if defined(GESTOR_TIENE_BACKEND)
    // 1. Ciclo de vida de la base del cliente (RF-1, RF-2), encapsulado en
    //    `ContextoBaseDatos` (módulo `datos`): abre/respalda y construye el
    //    núcleo de datos. Ante un fallo de apertura (RF-4) se ofrece el
    //    diálogo de recuperación: reintentar, restaurar respaldo o crear una
    //    base nueva con respaldo obligatorio. La instancia única ya se comprobó
    //    en main() y su detección sigue activa mientras transcurre la apertura.
    ContextoBaseDatos contexto;
    const AperturaBaseDatos::Resultado apertura = abrirBaseConRecuperacion(contexto);

    if (!apertura.ok())
    {
        // Eligió «Salir» (o la recuperación falló de forma definitiva).
        return 2;
    }
#endif

    // 2. Backend como proceso hijo. Arranque asíncrono: los errores
    //    se manejan por señales, no por valor de retorno.
    GestorProcesoBackend gestor;

    QObject::connect(&gestor, &GestorProcesoBackend::backendListo, []() {
        qDebug() << "[frontend] Backend listo";
    });
    QObject::connect(&gestor, &GestorProcesoBackend::backendColapsado, [](int codigo) {
        QMessageBox::critical(nullptr, QStringLiteral("Backend"),
                              detalleColapsoBackend(codigo));
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

    // 5. Enfoque de la instancia existente (RF-5): si otra instancia pide activar,
    //    traer esta ventana al frente. La detección sigue activa durante toda la
    //    sesión (la mantiene viva `main.cpp`).
    if (instanciaUnica)
    {
        QObject::connect(instanciaUnica, &InstanciaUnica::activarSolicitada, &ventana,
                         [&ventana]() {
                             ventana.showNormal();
                             ventana.raise();
                             ventana.activateWindow();
                         });
    }

    return QApplication::exec();
}
