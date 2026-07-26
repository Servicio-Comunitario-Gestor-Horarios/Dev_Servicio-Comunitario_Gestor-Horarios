#pragma once

/**
 * @file internalclient.h
 * @brief Cliente IPC para comunicación con el backend.
 *
 * InternalClient se conecta al servidor QLocalServer del backend
 * y envía solicitudes en formato JSON. Soporta health-check y
 * operaciones genéricas CRUD.
 */

#include <QObject>
#include <QLocalSocket>
#include <QJsonObject>

/**
 * @brief Cliente del middleware de comunicación IPC.
 *
 * Establece una conexión QLocalSocket con el servidor IPC del backend,
 * envía solicitudes con formato `{"op":"...", "payload":{...}}` y
 * recibe respuestas `{"status": N, "data": ..., "code": N}`.
 *
 * ## Uso básico
 * ~~~{.cpp}
 * InternalClient cliente;
 * cliente.sendHealthCheck();
 * // o
 * cliente.enviarSolicitud("shutdown");
 * ~~~
 */
class InternalClient : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Constructor.
     * @param parent QObject padre (opcional).
     */
    explicit InternalClient(QObject *parent = nullptr);

    /**
     * @brief Envía una solicitud de verificación de salud al backend.
     *
     * Método invocable desde QML o QTimer::singleShot.
     * Retorna vía la señal healthCheckResponseReceived(bool).
     */
    Q_INVOKABLE void sendHealthCheck();

    /**
     * @brief Envía una solicitud IPC genérica.
     *
     * Construye un JSON con la operación y payload opcional,
     * conecta al servidor y envía los datos.
     *
     * @param op Código de operación (ej: "shutdown", "teacher_list").
     * @param payload Datos adicionales de la operación (opcional).
     */
    Q_INVOKABLE void enviarSolicitud(const QString &op,
                                     const QJsonObject &payload = QJsonObject());

signals:
    /**
     * @brief Se emite al recibir respuesta del health-check.
     * @param exitoso true si el backend respondió con status "ok".
     */
    void healthCheckResponseReceived(bool exitoso);

    /**
     * @brief Se emite con la respuesta completa de cualquier operación.
     * @param respuesta Objeto JSON con la respuesta del servidor.
     */
    void respuestaRecibida(const QJsonObject &respuesta);

private slots:
    /** @brief Slot llamado al establecer conexión con el servidor. */
    void onConnected();
    /** @brief Slot llamado cuando hay datos disponibles para leer. */
    void onReadyRead();
    /** @brief Slot llamado cuando ocurre un error de conexión. */
    void onErrorOccurred(QLocalSocket::LocalSocketError error);

private:
    QLocalSocket *m_socket;                   ///< Socket de comunicación IPC
    QString m_operacionPendiente;              ///< Operación aguardando conexión
    QJsonObject m_payloadPendiente;            ///< Payload de la operación pendiente
};
