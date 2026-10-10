#include "consola/ConsolaDatos.hpp"

#include "backend/database/apertura_base_datos.hpp"
#include "datos/ContextoBaseDatos.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include <cstdio>

#ifndef PROJECT_VERSION
#define PROJECT_VERSION "0.1.0"
#endif

namespace
{
    /// Texto legible del estado de apertura de la base (RF-1, RF-4).
    QString estadoAperturaATexto(AperturaBaseDatos::Estado estado)
    {
        using Estado = AperturaBaseDatos::Estado;
        switch (estado)
        {
            case Estado::OkCreada:       return QStringLiteral("OkCreada (base nueva)");
            case Estado::OkAbierta:      return QStringLiteral("OkAbierta (versión actual)");
            case Estado::OkMigrada:      return QStringLiteral("OkMigrada (migración aplicada)");
            case Estado::FalloAusente:   return QStringLiteral("FalloAusente (versión de esquema ausente)");
            case Estado::FalloPosterior: return QStringLiteral("FalloPosterior (versión posterior a la esperada)");
            case Estado::FalloApertura:  return QStringLiteral("FalloApertura (no se pudo abrir/crear)");
            case Estado::FalloRespaldo:  return QStringLiteral("FalloRespaldo (no se pudo respaldar)");
            case Estado::FalloMigracion: return QStringLiteral("FalloMigracion (migración fallida)");
        }
        return QStringLiteral("Desconocido");
    }

    void imprimirUso(QTextStream& salida)
    {
        salida << "Uso: gestor-horarios-consola [--db <ruta>] [--cmd \"<comando>\"]... "
                  "[--ayuda] [--version]\n"
                  "  --db <ruta>     Ruta de la base de datos (por defecto ./consola_datos.db).\n"
                  "  --cmd \"<cmd>\"   Ejecuta un comando y termina (repetible; sin REPL).\n"
                  "  --ayuda         Muestra esta ayuda y la lista de comandos.\n"
                  "  --version       Muestra la versión.\n";
    }
} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    QTextStream salida(stdout);
    QTextStream entrada(stdin);

    QString rutaDb = QDir::current().filePath(QStringLiteral("consola_datos.db"));
    QStringList comandos;
    bool ayuda = false;

    const QStringList argumentos = app.arguments();
    for (int i = 1; i < argumentos.size(); ++i)
    {
        const QString& arg = argumentos[i];
        if (arg == QStringLiteral("--db") && i + 1 < argumentos.size())
            rutaDb = argumentos[++i];
        else if (arg == QStringLiteral("--cmd") && i + 1 < argumentos.size())
            comandos << argumentos[++i];
        else if (arg == QStringLiteral("--ayuda") || arg == QStringLiteral("--help")
                 || arg == QStringLiteral("-h"))
            ayuda = true;
        else if (arg == QStringLiteral("--version"))
        {
            salida << "gestor-horarios-consola v" << PROJECT_VERSION << '\n';
            return 0;
        }
        else
        {
            salida << "argumento desconocido: " << arg << "\n\n";
            imprimirUso(salida);
            return 1;
        }
    }

    // 1. Abrir la base por el mismo camino que la aplicación (RF-1, RF-4).
    ContextoBaseDatos contexto;
    const AperturaBaseDatos::Resultado apertura = contexto.abrir(rutaDb);

    salida << "Base de datos : " << rutaDb << '\n';
    salida << "Apertura      : " << estadoAperturaATexto(apertura.estado) << '\n';
    if (!apertura.detalle.isEmpty())
        salida << "Detalle       : " << apertura.detalle << '\n';
    if (!apertura.rutaRespaldo.isEmpty())
        salida << "Respaldo      : " << apertura.rutaRespaldo << '\n';
    salida.flush();

    if (!apertura.ok())
    {
        salida << "No se pudo abrir la base de datos. Saliendo.\n";
        salida.flush();
        return 2;
    }

    // 2. Consola.
    const bool interactivo = comandos.isEmpty();
    ConsolaDatos consola(contexto, salida, entrada, interactivo);
    consola.imprimirBienvenida();
    if (ayuda)
        consola.imprimirAyuda();

    if (!comandos.isEmpty())
    {
        for (const QString& comando : comandos)
        {
            if (!consola.ejecutarLinea(comando))
                break;
        }
        salida.flush();
        return 0;
    }

    // REPL interactivo.
    while (true)
    {
        salida << "consola> ";
        salida.flush();
        const QString linea = entrada.readLine();
        if (linea.isNull())   // EOF (Ctrl+D)
            break;
        if (!consola.ejecutarLinea(linea))
            break;
    }

    salida << "Hasta luego.\n";
    salida.flush();
    return 0;
}
