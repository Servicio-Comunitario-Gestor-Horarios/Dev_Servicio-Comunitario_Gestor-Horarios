#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <memory>

class InternalClient;

class BackendProcessManager : public QObject {
    Q_OBJECT
public:
    explicit BackendProcessManager(QObject *parent = nullptr);
    ~BackendProcessManager() override;

    void start();
    void stop();
    bool isRunning() const;

signals:
    void backendReady();
    void backendCrashed();
    void backendRestarting(int attempt);
    void backendStopped();

private slots:
    void onProcessStarted();
    void onProcessFinished(int exitCode, QProcess::ExitStatus status);
    void onProcessErrorOccurred(QProcess::ProcessError error);
    void onHealthCheckTimeout();

private:
    void launchProcess();
    static constexpr int MAX_RESTART_ATTEMPTS = 3;
    static constexpr int HEALTH_CHECK_INTERVAL_MS = 5000;
    static constexpr int INITIAL_WAIT_MS = 1500;
    static constexpr int BACKOFF_BASE_MS = 2000;

    QProcess *m_process = nullptr;
    QTimer *m_healthTimer = nullptr;
    InternalClient *m_healthClient = nullptr;
    int m_restartAttempts = 0;
    QString m_backendExecutable;
};
