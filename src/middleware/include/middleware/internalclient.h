#pragma once

/**
 * @file internalclient.h
 * @brief Cliente IPC para comunicación con el backend.
 *
 * Encola peticiones (FIFO, una en vuelo) y las envía en formato NDJSON v1.
 * Las respuestas se correlacionan por el campo 'op'.
 */

#include <QObject>
#include <QLocalSocket>
#include <QByteArray>
#include <QJsonObject>
#include <QPair>
#include <QQueue>

class InternalClient : public QObject {
    Q_OBJECT
public:
    explicit InternalClient(QObject *parent = nullptr);

    Q_INVOKABLE void sendHealthCheck();

    /**
     * @brief Encola una solicitud IPC.
     * @note Soporta varias solicitudes; se envían de a una (FIFO) y se
     *       correlacionan por el campo 'op' de la respuesta.
     */
    Q_INVOKABLE void enviarSolicitud(const QString &op,
                                     const QJsonObject &payload = QJsonObject());

signals:
    void healthCheckResponseReceived(bool exitoso);
    void respuestaRecibida(const QJsonObject &respuesta);

private slots:
    void onConnected();
    void onReadyRead();
    void onDisconnected();
    void onErrorOccurred(QLocalSocket::LocalSocketError error);

private:
    QLocalSocket *m_socket;
    QQueue<QPair<QString, QJsonObject>> m_cola;  ///< peticiones pendientes (FIFO)
    QString    m_operacionEnVuelo;               ///< op esperando respuesta
    QByteArray m_buffer;                         ///< resto sin frame completo

    void enviarSiguiente();
};