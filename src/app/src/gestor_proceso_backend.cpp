#include "app/gestor_proceso_backend.hpp"
#include <middleware/internalclient.h>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

/**
 * @brief Constructor — prepara el gestor sin iniciar el proceso.
 *
 * Configura las conexiones Qt para manejar el ciclo de vida del QProcess
 * y el timer de verificación periódica de salud.
 */
GestorProcesoBackend::GestorProcesoBackend(QObject *parent)
    : QObject(parent)
    , m_proceso(new QProcess(this))
    , m_timerVerificacion(new QTimer(this))
    , m_clienteVerificacion(new InternalClient(this))
{
    m_ejecutableBackend = QCoreApplication::applicationFilePath();

    connect(m_proceso, &QProcess::started,
            this, &GestorProcesoBackend::alIniciarProceso);
    connect(m_proceso, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &GestorProcesoBackend::alFinalizarProceso);
    connect(m_proceso, &QProcess::errorOccurred,
            this, &GestorProcesoBackend::alOcurrirErrorProceso);
    connect(m_timerVerificacion, &QTimer::timeout,
            this, &GestorProcesoBackend::alExpiracionVerificacion);
}

/**
 * @brief Destructor — asegura que el backend se detenga.
 */
GestorProcesoBackend::~GestorProcesoBackend()
{
    detener();
}

/**
 * @brief Inicia el proceso backend.
 *
 * Si ya hay un proceso en ejecución, no hace nada.
 * Resetea el contador de reintentos y lanza el proceso.
 */
void GestorProcesoBackend::iniciar()
{
    if (m_proceso->state() != QProcess::NotRunning) {
        qDebug() << "GestorProcesoBackend: backend ya está en ejecución";
        return;
    }
    m_intentosReinicio = 0;
    lanzarProceso();
}

/**
 * @brief Detiene el backend gracefulmente.
 *
 * 1. Detiene el timer de verificación de salud.
 * 2. Envía terminate() (SIGTERM en Unix).
 * 3. Espera hasta 5 segundos a que termine.
 * 4. Si no responde, envía kill() (SIGKILL).
 */
void GestorProcesoBackend::detener()
{
    m_timerVerificacion->stop();
    if (m_proceso->state() == QProcess::NotRunning)
        return;

    qDebug() << "GestorProcesoBackend: deteniendo backend...";
    m_proceso->terminate();
    if (!m_proceso->waitForFinished(5000)) {
        qWarning() << "GestorProcesoBackend: backend no respondió, forzando kill";
        m_proceso->kill();
        m_proceso->waitForFinished(2000);
    }
    emit backendDetenido();
}

/**
 * @brief Consulta si el backend está en ejecución.
 * @return true si el proceso QProcess está en estado Running.
 */
bool GestorProcesoBackend::estaEjecutando() const
{
    return m_proceso->state() == QProcess::Running;
}

/**
 * @brief Lanza el proceso backend con el argumento --backend.
 *
 * Usa la ruta al propio ejecutable (applicationFilePath) para lanzarse
 * a sí mismo en modo backend.
 */
void GestorProcesoBackend::lanzarProceso()
{
    QStringList args;
    args << QStringLiteral("--backend");

    qDebug() << "GestorProcesoBackend: lanzando" << m_ejecutableBackend << args;
    m_proceso->start(m_ejecutableBackend, args);
}

/**
 * @brief Callback cuando el proceso backend se inicia exitosamente.
 *
 * Espera ESPERA_INICIAL_MS (1.5s) para dar tiempo al backend a abrir
 * el socket IPC, luego envía un health-check.
 *
 * Si el health-check falla, reintenta una vez más.
 */
void GestorProcesoBackend::alIniciarProceso()
{
    qDebug() << "GestorProcesoBackend: proceso iniciado, verificando salud...";

    QTimer::singleShot(ESPERA_INICIAL_MS, this, [this]() {
        connect(m_clienteVerificacion, &InternalClient::healthCheckResponseReceived,
                this, [this](bool exito) {
            if (exito) {
                qDebug() << "GestorProcesoBackend: backend listo (verificación OK)";
                m_timerVerificacion->start(INTERVALO_VERIFICACION_MS);
                emit backendListo();
            } else {
                qWarning() << "GestorProcesoBackend: verificación falló, reintentando...";
                QTimer::singleShot(ESPERA_INICIAL_MS, m_clienteVerificacion,
                                   &InternalClient::sendHealthCheck);
            }
        });
        m_clienteVerificacion->sendHealthCheck();
    });
}

/**
 * @brief Callback cuando el proceso backend termina.
 *
 * Si la terminación fue por crash (CrashExit), intenta reiniciar
 * con backoff exponencial (2s, 4s, 6s) hasta MAX_INTENTOS_REINICIO.
 *
 * Si la terminación fue normal, solo registra el evento.
 */
void GestorProcesoBackend::alFinalizarProceso(int codigo, QProcess::ExitStatus estado)
{
    m_timerVerificacion->stop();

    if (estado == QProcess::CrashExit) {
        qWarning() << "GestorProcesoBackend: backend COLAPSÓ con código" << codigo;
        emit backendColapsado();

        if (m_intentosReinicio < MAX_INTENTOS_REINICIO) {
            m_intentosReinicio++;
            int espera = BASE_ESPERA_REINTENTO_MS * m_intentosReinicio;
            qDebug() << "GestorProcesoBackend: reintento" << m_intentosReinicio
                     << "de" << MAX_INTENTOS_REINICIO << "en" << espera << "ms";
            emit backendReiniciando(m_intentosReinicio);
            QTimer::singleShot(espera, this, &GestorProcesoBackend::lanzarProceso);
        } else {
            qCritical() << "GestorProcesoBackend: máximo de reintentos alcanzado";
        }
    } else {
        qDebug() << "GestorProcesoBackend: backend terminó normalmente, código" << codigo;
    }
}

/**
 * @brief Callback cuando ocurre un error en QProcess.
 *
 * Emite backendColapsado para que la UI pueda reaccionar.
 */
void GestorProcesoBackend::alOcurrirErrorProceso(QProcess::ProcessError error)
{
    qCritical() << "GestorProcesoBackend: error en QProcess:" << error;
    emit backendColapsado();
}

/**
 * @brief Callback del timer periódico de verificación de salud.
 *
 * Envía un health-check al backend cada INTERVALO_VERIFICACION_MS (5s).
 */
void GestorProcesoBackend::alExpiracionVerificacion()
{
    m_clienteVerificacion->sendHealthCheck();
}
