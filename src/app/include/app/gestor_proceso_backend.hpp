#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <memory>

class InternalClient;

/**
 * @brief Gestiona el ciclo de vida del proceso backend.
 *
 * Lanza el backend como QProcess hijo (`gestor-horarios --backend`), monitorea
 * su salud mediante health-check periódico por QLocalSocket, y lo detiene de
 * forma ordenada al cerrar. No reinicia automáticamente tras una caída (fuera de
 * alcance de la spec 002): un crash se reporta con `backendColapsado`.
 */
class GestorProcesoBackend : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Constructor.
     * @param parent QObject padre (opcional).
     */
    explicit GestorProcesoBackend(QObject *parent = nullptr);

    /** @brief Destructor — detiene el backend si está en ejecución. */
    ~GestorProcesoBackend() override;

    /**
     * @brief Inicia el proceso backend.
     *
     * Si el backend ya está en ejecución, no hace nada.
     * Resetea el contador de reintentos.
     */
    void iniciar();

    /**
     * @brief Detiene el backend gracefulmente.
     *
     * Envía terminate(), espera 5s, y si no responde hace kill().
     */
    void detener();

    /** @brief Retorna true si el proceso backend está en ejecución. */
    bool estaEjecutando() const;

signals:
    /** @brief Se emite cuando el backend responde al health-check inicial. */
    void backendListo();

    /**
     * @brief Se emite cuando el backend crashea.
     *
     * Si se alcanzó el máximo de reintentos, este es el evento final.
     */
    void backendColapsado();

    /**
     * @brief Se emite antes de cada reintento de reinicio.
     * @param intento Número de intento actual (1, 2, 3).
     */
    void backendReiniciando(int intento);

    /** @brief Se emite cuando el backend se detiene normalmente. */
    void backendDetenido();

private slots:
    /** @brief Slot llamado cuando el proceso backend se inicia. */
    void alIniciarProceso();

    /**
     * @brief Slot llamado cuando el proceso backend termina.
     * @param codigo Código de salida del proceso.
     * @param estado NormalExit o CrashExit.
     */
    void alFinalizarProceso(int codigo, QProcess::ExitStatus estado);

    /**
     * @brief Slot llamado cuando ocurre un error en QProcess.
     * @param error Tipo de error ocurrido.
     */
    void alOcurrirErrorProceso(QProcess::ProcessError error);

    /** @brief Slot llamado en cada tick del timer de verificación de salud. */
    void alExpiracionVerificacion();

private:
    /** @brief Lanza el proceso backend con argumento --backend. */
    void lanzarProceso();

    // Constantes de configuración
    static constexpr int INTERVALO_VERIFICACION_MS = 5000; ///< Cada 5s se verifica salud
    static constexpr int ESPERA_INICIAL_MS = 1500;         ///< Espera inicial tras lanzar backend

    QProcess *m_proceso = nullptr;                    ///< Proceso backend hijo
    QTimer *m_timerVerificacion = nullptr;              ///< Timer periódico de salud
    InternalClient *m_clienteVerificacion = nullptr;    ///< Cliente IPC para health-check
    QString m_ejecutableBackend;                        ///< Ruta al propio binario
};
