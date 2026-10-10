#pragma once

#include <QObject>
#include <QString>

class QLocalServer;
class QLocalSocket;

/**
 * @file instancia_unica.hpp
 * @brief Detección de instancia única de la aplicación (RF-5).
 *
 * Al iniciar el cliente, la primera instancia crea un servidor local con un
 * nombre fijo y queda a la escucha durante toda la sesión (incluida la
 * migración de la base de datos y el diálogo de fallo de RF-4). Las instancias
 * siguientes se conectan, piden «activar» y esperan un acuse: si lo reciben,
 * enfocan la instancia existente y salen; si no lo reciben en plazo, avisan y
 * no arrancan.
 *
 * El detector es independiente de la base de datos y de la interfaz (solo usa
 * QtNetwork). Por eso vive en la librería `app_core`, que enlazan tanto el
 * ejecutable `gestor-horarios` como el test `test_instancia_unica`.
 *
 * Protocolo:
 *  - Secundaria → Primaria: mensaje `activar`.
 *  - Primaria → Secundaria: acuse `acuse` y, además, emite `activarSolicitada()`.
 */
class InstanciaUnica : public QObject
{
    Q_OBJECT

public:
    /// Nombre del servidor local que identifica a la aplicación.
    static const QString NOMBRE_POR_DEFECTO;

    /// Desenlace de intentar arrancar una instancia.
    enum class Resultado
    {
        Primaria,   ///< No había otra instancia: somos la principal.
        Secundaria, ///< Había otra y la enfocamos (acuse recibido).
        SinAcuse    ///< Había otra pero no respondió en plazo: no arrancamos.
    };

    /// @param nombre        Nombre del servidor local (por defecto, el de la app).
    /// @param plazoAcuseMs  Plazo para recibir el acuse (por defecto, 10 s).
    /// @param padre         Objeto Qt padre.
    explicit InstanciaUnica(const QString& nombre = NOMBRE_POR_DEFECTO,
                            int plazoAcuseMs = 10000,
                            QObject* padre = nullptr);

    ~InstanciaUnica() override;

    /// Intenta arrancar. Si es la principal, deja el servidor escuchando.
    Resultado iniciar();

    /// Resultado del último `iniciar()`.
    Resultado resultado() const { return m_resultado; }

    /// Detalle legible en español para el usuario (vacío si no hay incidencia).
    QString detalle() const { return m_detalle; }

    /// Indica si somos la instancia principal.
    bool esPrimaria() const { return m_resultado == Resultado::Primaria; }

    /// Indica si el servidor de detección sigue a la escucha (instancia principal).
    bool estaActiva() const;

signals:
    /// Una instancia secundaria pidió enfocar esta. La interfaz la usa para
    /// levantar/enfocar la ventana (o el diálogo activo durante el arranque).
    void activarSolicitada();

private slots:
    void alNuevaConexion();

private:
    /// Envía «activar» por `socket` y espera el acuse dentro del plazo.
    Resultado enviarActivacion(QLocalSocket& socket);

    QString m_nombre;
    int m_plazoAcuseMs;

    QLocalServer* m_servidor = nullptr;

    Resultado m_resultado = Resultado::SinAcuse;
    QString m_detalle;
};
