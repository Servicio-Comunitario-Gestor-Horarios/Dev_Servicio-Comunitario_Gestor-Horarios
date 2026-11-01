#include "seed/sembrador.hpp"

#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <QTextStream>

#ifndef PROJECT_VERSION
#define PROJECT_VERSION "0.1.0"
#endif

namespace {

/// Código de salida del proceso para cada estado de la aplicación (plan 008 §7):
/// 0 ok · 1 uso · 2 base con datos · 3 confirmación requerida · 4 respaldo fallido ·
/// 5 apertura/esquema · 6 variante desconocida · 7 escritura · 8 perfil incoherente.
int codigoSalida(seed::EstadoAplicacion estado) {
    using Estado = seed::EstadoAplicacion;
    switch (estado) {
    case Estado::Ok:                   return 0;
    case Estado::UsoInvalido:          return 1;
    case Estado::BaseConDatos:         return 2;
    case Estado::ConfirmacionRequerida: return 3;
    case Estado::RespaldoFallido:      return 4;
    case Estado::EsquemaNoEsperado:    return 5;
    case Estado::FalloApertura:        return 5;
    case Estado::VarianteDesconocida:  return 6;
    case Estado::FalloEscritura:       return 7;
    case Estado::PerfilIncoherente:    return 8;
    }
    return 1;
}

/// Ayuda de la CLI (RF-13): lista todas las opciones y sus efectos.
void imprimirUso(QTextStream& salida) {
    salida << "Uso: gestor-horarios-seed --db <ruta> [--variante <v>]\n"
              "                     [--reemplazar | --solo-docentes] [--si] [--ayuda] [--version]\n"
              "\n"
              "Puebla una base con el centro de datos de prueba (Plan de Estudio 31059).\n"
              "\n"
              "Opciones:\n"
              "  --db <ruta>      Base de destino (obligatoria; se crea si no existe).\n"
              "  --variante <v>   Variante de docentes: holgada (por defecto), suplentes,\n"
              "                   sin-profesorado o sin-disponibilidad.\n"
              "  --reemplazar     Reemplazo total (borra y repuebla; exige --si y respaldo).\n"
              "  --solo-docentes  Recarga solo docentes, sus materias y su disponibilidad\n"
              "                   (exige --si y respaldo).\n"
              "  --si             Confirma la operación destructiva (sin terminal interactiva).\n"
              "  --ayuda          Muestra esta ayuda.\n"
              "  --version        Muestra la versión.\n";
}

/// Informe completo de la aplicación (RF-12, plan §8): la línea de base, el informe
/// determinista de `resumenTexto` (variante, recuentos, materias por año y totales por plan),
/// la ruta del respaldo si lo hubo y `ok`.
void imprimirResumen(QTextStream& salida, const seed::OpcionesAplicacion& opciones,
                     const seed::ResultadoAplicacion& resultado) {
    salida << "Base: " << opciones.rutaDb << '\n';
    salida << seed::resumenTexto(seed::perfilPorDefecto(), opciones.variante,
                                 resultado.recuentos, resultado.duracionMinutos,
                                 resultado.configuraciones)
           << '\n';
    if (resultado.rutaRespaldo.isEmpty()) {
        salida << "Respaldo: no aplica\n";
    } else {
        salida << "Respaldo: " << resultado.rutaRespaldo << '\n';
    }
    salida << "ok\n";
}

} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    QTextStream salida(stdout);
    QTextStream error(stderr);

    const QStringList argumentos = app.arguments();

    seed::OpcionesAplicacion opciones;
    QString rutaDb;
    bool ayuda = false;
    bool salir = false;   // ya se decidió el desenlace (--version, uso inválido, variante…)
    int codigo = 0;

    for (int i = 1; i < argumentos.size() && !salir; ++i) {
        const QString& arg = argumentos[i];
        if (arg == QStringLiteral("--db") && i + 1 < argumentos.size()) {
            rutaDb = argumentos[++i];
        } else if (arg == QStringLiteral("--variante") && i + 1 < argumentos.size()) {
            const QString texto = argumentos[++i];
            const auto variante = seed::varianteDesdeTexto(texto);
            if (!variante.has_value()) {
                error << "Variante de docentes desconocida: «" << texto << "»\n";
                codigo = codigoSalida(seed::EstadoAplicacion::VarianteDesconocida);
                salir = true;
            } else {
                opciones.variante = *variante;
            }
        } else if (arg == QStringLiteral("--reemplazar")) {
            opciones.reemplazarTodo = true;
        } else if (arg == QStringLiteral("--solo-docentes")) {
            opciones.soloDocentes = true;
        } else if (arg == QStringLiteral("--si")) {
            opciones.confirmado = true;
        } else if (arg == QStringLiteral("--ayuda") || arg == QStringLiteral("--help") ||
                   arg == QStringLiteral("-h")) {
            ayuda = true;
        } else if (arg == QStringLiteral("--version")) {
            salida << "gestor-horarios-seed v" << PROJECT_VERSION << '\n';
            salir = true;
        } else {
            error << "Argumento desconocido: " << arg << "\n\n";
            imprimirUso(error);
            codigo = 1;
            salir = true;
        }
    }

    if (!salir && ayuda) {
        imprimirUso(salida);
    } else if (!salir && rutaDb.trimmed().isEmpty()) {
        error << "Debe indicar la base de destino con «--db <ruta>».\n\n";
        imprimirUso(error);
        codigo = 1;
    } else if (!salir) {
        opciones.rutaDb = rutaDb;
        const seed::ResultadoAplicacion resultado = seed::aplicar(opciones);
        if (resultado.ok()) {
            imprimirResumen(salida, opciones, resultado);
            codigo = 0;
        } else {
            error << resultado.detalle << '\n';
            codigo = codigoSalida(resultado.estado);
        }
    }

    salida.flush();
    error.flush();
    return codigo;
}
