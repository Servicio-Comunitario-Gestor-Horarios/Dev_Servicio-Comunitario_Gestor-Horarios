#include "app/backend_process_manager.hpp"
#include <middleware/internalclient.h>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

BackendProcessManager::BackendProcessManager(QObject *parent)
    : QObject(parent)
    , m_process(new QProcess(this))
    , m_healthTimer(new QTimer(this))
    , m_healthClient(new InternalClient(this))
{
    m_backendExecutable = QCoreApplication::applicationFilePath();

    connect(m_process, &QProcess::started,
            this, &BackendProcessManager::onProcessStarted);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &BackendProcessManager::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred,
            this, &BackendProcessManager::onProcessErrorOccurred);
    connect(m_healthTimer, &QTimer::timeout,
            this, &BackendProcessManager::onHealthCheckTimeout);
}

BackendProcessManager::~BackendProcessManager() {
    stop();
}

void BackendProcessManager::start() {
    if (m_process->state() != QProcess::NotRunning) {
        qDebug() << "BackendProcessManager: backend ya está en ejecución";
        return;
    }
    m_restartAttempts = 0;
    launchProcess();
}

void BackendProcessManager::stop() {
    m_healthTimer->stop();
    if (m_process->state() == QProcess::NotRunning)
        return;

    qDebug() << "BackendProcessManager: deteniendo backend...";
    m_process->terminate();
    if (!m_process->waitForFinished(5000)) {
        qWarning() << "BackendProcessManager: backend no respondió, forzando kill";
        m_process->kill();
        m_process->waitForFinished(2000);
    }
    emit backendStopped();
}

bool BackendProcessManager::isRunning() const {
    return m_process->state() == QProcess::Running;
}

void BackendProcessManager::launchProcess() {
    QStringList args;
    args << QStringLiteral("--backend");

    qDebug() << "BackendProcessManager: lanzando" << m_backendExecutable << args;
    m_process->start(m_backendExecutable, args);
}

void BackendProcessManager::onProcessStarted() {
    qDebug() << "BackendProcessManager: proceso backend iniciado, esperando health-check...";
    QTimer::singleShot(INITIAL_WAIT_MS, this, [this]() {
        connect(m_healthClient, &InternalClient::healthCheckResponseReceived,
                this, [this](bool success) {
            if (success) {
                qDebug() << "BackendProcessManager: backend listo (health-check OK)";
                m_healthTimer->start(HEALTH_CHECK_INTERVAL_MS);
                emit backendReady();
            } else {
                qWarning() << "BackendProcessManager: health-check falló, reintentando...";
                QTimer::singleShot(INITIAL_WAIT_MS, m_healthClient, &InternalClient::sendHealthCheck);
            }
        });
        m_healthClient->sendHealthCheck();
    });
}

void BackendProcessManager::onProcessFinished(int exitCode, QProcess::ExitStatus status) {
    m_healthTimer->stop();

    if (status == QProcess::CrashExit) {
        qWarning() << "BackendProcessManager: backend CRASHEÓ con código" << exitCode;
        emit backendCrashed();

        if (m_restartAttempts < MAX_RESTART_ATTEMPTS) {
            m_restartAttempts++;
            int backoff = BACKOFF_BASE_MS * m_restartAttempts;
            qDebug() << "BackendProcessManager: reintento" << m_restartAttempts
                     << "de" << MAX_RESTART_ATTEMPTS << "en" << backoff << "ms";
            emit backendRestarting(m_restartAttempts);
            QTimer::singleShot(backoff, this, &BackendProcessManager::launchProcess);
        } else {
            qCritical() << "BackendProcessManager: máximo de reintentos alcanzado";
        }
    } else {
        qDebug() << "BackendProcessManager: backend terminó normalmente, código" << exitCode;
    }
}

void BackendProcessManager::onProcessErrorOccurred(QProcess::ProcessError error) {
    qCritical() << "BackendProcessManager: error en QProcess:" << error;
    emit backendCrashed();
}

void BackendProcessManager::onHealthCheckTimeout() {
    m_healthClient->sendHealthCheck();
}
