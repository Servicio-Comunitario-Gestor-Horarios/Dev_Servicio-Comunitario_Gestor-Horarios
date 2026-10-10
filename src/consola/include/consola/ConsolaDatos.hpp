#pragma once

/**
 * @file ConsolaDatos.hpp
 * @brief Consola de terminal para probar la base de datos (herramienta de desarrollo).
 *
 * Ejercita la BD usando **el mismo contrato que consumirá el frontend**:
 * `ContextoBaseDatos` (apertura/migración/respaldo) y `NucleoDatos` (CRUD de
 * dominios, cascada y cambios pendientes). No depende de Qt Widgets.
 *
 * Diseño modular para ampliarlo:
 *  - Cada dominio es un método `cmdX(...)`.
 *  - Los comandos de primer nivel se registran en `registrarComandos()`; añadir
 *    uno nuevo es escribir su método y registrarlo en el mapa `m_comandos`.
 */

#include <QMap>
#include <QString>
#include <QStringList>

#include <functional>

class ContextoBaseDatos;
class NucleoDatos;
class QTextStream;

class ConsolaDatos
{
public:
    using Args = QStringList;

    /// @param contexto    Base ya abierta por el arranque (no se posee).
    /// @param salida      Flujo de salida (normalmente stdout).
    /// @param entrada     Flujo de entrada (normalmente stdin), para confirmaciones.
    /// @param interactivo Si es `false` (modo `--cmd`), las confirmaciones exigen `--si`.
    ConsolaDatos(ContextoBaseDatos& contexto,
                 QTextStream& salida,
                 QTextStream& entrada,
                 bool interactivo);

    /// Ejecuta una línea de comando. Devuelve `false` si el usuario pidió salir.
    bool ejecutarLinea(const QString& linea);

    void imprimirBienvenida();
    void imprimirAyuda();

private:
    using Manejador = std::function<void(const Args&)>;

    ContextoBaseDatos& m_contexto;
    QTextStream& m_salida;
    QTextStream& m_entrada;
    bool m_interactivo;

    /// Registro de comandos de primer nivel (extensible).
    QMap<QString, Manejador> m_comandos;

    NucleoDatos& nucleo();
    void registrarComandos();
    void imprimir(const QString& texto);
    bool confirmar(const QString& pregunta, const Args& args);

    // ─── Comandos por dominio ─────────────────────────────────────────────
    void cmdDocentes(const Args& args);
    void cmdAulas(const Args& args);
    void cmdMaterias(const Args& args);
    void cmdPlanes(const Args& args);
    void cmdCursos(const Args& args);
    void cmdTurnos(const Args& args);
    void cmdDependientes(const Args& args);
    void cmdCascada(const Args& args);
    void cmdPendientes(const Args& args);
    void cmdReintentar(const Args& args);
    void cmdEstado(const Args& args);
    void cmdAyuda(const Args& args);
};
