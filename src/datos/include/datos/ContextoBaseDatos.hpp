#pragma once

/**
 * @file ContextoBaseDatos.hpp
 * @brief Ciclo de vida de la base de datos del lado cliente (RF-1, RF-2).
 *
 * Encapsula, en un único componente sin Qt Widgets, todo lo que hoy hacía el
 * arranque del cliente: resolver la ruta de la base, abrirla (crear/migrar/
 * respaldar con `AperturaBaseDatos`), mantener la conexión persistente y
 * construir el `NucleoDatos` que consume la interfaz.
 *
 * Es propiedad exclusiva del lado cliente: el proceso de cálculo (`--backend`)
 * nunca lo usa (RF-2). Al ser UI-free, es testeable de forma independiente y
 * permite cambiar la base sin tocar la interfaz.
 *
 * RAII: al destruirse cierra y libera su conexión.
 */

#include <QSqlDatabase>
#include <QString>

#include <memory>

#include "backend/database/apertura_base_datos.hpp"

class NucleoDatos;

class ContextoBaseDatos
{
public:
    /// Dependencias inyectables para los tests.
    struct Opciones
    {
        /// Nombre de la conexión Qt. Si está vacío se genera uno único por
        /// instancia (evita colisiones entre contextos y en los tests).
        QString nombreConexion;

        /// Parámetros de apertura (migración, respaldo, reloj) inyectables.
        AperturaBaseDatos::Opciones apertura;
    };

    ContextoBaseDatos();
    ~ContextoBaseDatos();

    ContextoBaseDatos(const ContextoBaseDatos&) = delete;
    ContextoBaseDatos& operator=(const ContextoBaseDatos&) = delete;

    /// Abre la base en `ruta` (crea, migra o respalda según haga falta) y deja
    /// el núcleo listo para usarse. Si devuelve un resultado no `ok()`, el
    /// contexto queda sin abrir y el motivo está en `Resultado::detalle`.
    ///
    /// Volver a llamar a `abrir()` cierra el estado anterior antes de reintentar.
    AperturaBaseDatos::Resultado abrir(const QString& ruta);

    /// Igual que `abrir(ruta)` con dependencias inyectables (tests).
    AperturaBaseDatos::Resultado abrir(const QString& ruta, const Opciones& opciones);

    /// Indica si la base está operativa (conexión abierta y núcleo construido).
    bool abierto() const;

    /// Fachada de dominios que consume la interfaz.
    /// Precondición: `abierto()`.
    NucleoDatos& nucleo();
    const NucleoDatos& nucleo() const;

    /// Conexión persistente del cliente. Precondición: `abierto()`.
    QSqlDatabase& conexion();

    /// Ruta de la base abierta (vacía si no hay base abierta).
    QString ruta() const;

    /// Ruta por defecto de la base del usuario, en el directorio de datos de la
    /// aplicación. Crea el directorio si no existe.
    static QString rutaPorDefecto();

private:
    /// Cierra la conexión y libera el núcleo (idempotente).
    void cerrar();

    std::unique_ptr<QSqlDatabase> m_conexion;
    std::unique_ptr<NucleoDatos> m_nucleo;
    QString m_nombreConexion;
    QString m_ruta;
};
