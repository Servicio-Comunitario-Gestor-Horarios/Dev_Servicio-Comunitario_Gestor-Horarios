#pragma once

/**
 * @file internalserver.h
 * @brief Servidor IPC para comunicación con el frontend.
 *
 * InternalServer utiliza QLocalServer para aceptar conexiones del
 * frontend y procesar solicitudes JSON (health-check, CRUD, etc.).
 */

#include <QObject>
#include <QLocalServer>
#include <QLocalSocket>

/**
 * @brief Servidor del middleware de comunicación IPC.
 *
 * Escucha en un socket local (nombre: GestorHorarios_Middleware) y
 * procesa solicitudes en formato JSON. Cada conexión entrante recibe
 * un QLocalSocket dedicado.
 *
 * ## Operaciones soportadas
 * - health_check: verificación de conectividad
 * - ready: confirmación de disponibilidad
 * - shutdown: apagado graceful
 * - teacher_*: operaciones CRUD (placeholder)
 */
class InternalServer : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Constructor.
     * @param parent QObject padre (opcional).
     */
    explicit InternalServer(QObject *parent = nullptr);

    /**
     * @brief Inicia el servidor IPC.
     *
     * Elimina cualquier instancia previa del servidor, escucha en
     * el nombre configurado y espera conexiones entrantes.
     *
     * @return true si el servidor inició correctamente.
     */
    bool start();

private slots:
    /** @brief Slot llamado cuando un nuevo cliente se conecta. */
    void onNewConnection();
    /** @brief Slot llamado cuando hay datos disponibles en un cliente. */
    void onReadyRead();
    /** @brief Slot llamado cuando un cliente se desconecta. */
    void onClientDisconnected();

private:
    QLocalServer *m_server;  ///< Servidor de sockets locales

    /**
     * @brief Registra una operación en el log de depuración.
     * @param direccion Dirección del flujo (Frontend -> Middleware, etc.).
     * @param operacion Nombre o descripción de la operación.
     */
    void registrarConexion(const QString &direccion, const QString &operacion);

    /**
     * @brief Envía una respuesta IPC al cliente.
     * @param status Código de estado (RESP_EXITO, RESP_ERROR, etc.).
     * @param data Payload de la respuesta.
     * @param clienteSocket Socket del cliente destino.
     */
    void sendResponse(int status, const QJsonValue &data,
                      QLocalSocket *clienteSocket);

    // ── CRUD Profesores (stubs — conectar al backend cuando esté listo) ──

    /** @brief Retorna lista vacía de profesores. */
    void handleTeacherList(QLocalSocket *clienteSocket);
    /** @brief Retorna profesor por ID o error no encontrado. */
    void handleTeacherGet(const QJsonObject &data, QLocalSocket *clienteSocket);
    /** @brief Valida campos y crea profesor (stub). */
    void handleTeacherCreate(const QJsonObject &data, QLocalSocket *clienteSocket);
    /** @brief Actualiza profesor existente (stub). */
    void handleTeacherUpdate(const QJsonObject &data, QLocalSocket *clienteSocket);
    /** @brief Elimina profesor por ID (stub). */
    void handleTeacherDelete(const QJsonObject &data, QLocalSocket *clienteSocket);

    // ── CRUD Aulas (stubs — conectar al backend cuando esté listo) ──

    /** @brief Retorna lista vacía de aulas. */
    void handleClassroomList(QLocalSocket *clienteSocket);
    /** @brief Valida campos y crea aula (stub). */
    void handleClassroomCreate(const QJsonObject &data, QLocalSocket *clienteSocket);
    /** @brief Actualiza aula existente (stub). */
    void handleClassroomUpdate(const QJsonObject &data, QLocalSocket *clienteSocket);
    /** @brief Elimina aula por ID (stub). */
    void handleClassroomDelete(const QJsonObject &data, QLocalSocket *clienteSocket);
};
