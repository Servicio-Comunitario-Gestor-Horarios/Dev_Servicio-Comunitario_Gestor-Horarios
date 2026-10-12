#pragma once

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <QMap>
#include <QString>
#include <middleware/internalclient.h>
#include <middleware/messages.h>

class TeacherScheduleDialog;
class StudentScheduleDialog;

class ScheduleVisualizationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ScheduleVisualizationWidget(QWidget *parent = nullptr);
    void setClient(InternalClient* client);

private slots:
    void onTeacherSchedulesClicked();
    void onStudentSchedulesClicked();
    void onLoadScheduleClicked();
    void onRespuestaRecibida(const QJsonObject& respuesta);

private:
    void setupUi();
    void ensureSchedulesDirectory();
    void copyEmbeddedTestSchedules();
    void sendRequest(const QString& op, const QJsonObject& payload);
    void showTeacherSchedules(const QJsonObject& data);
    void showStudentSchedules(const QJsonObject& data);
    void handleTeacherSchedulesResponse(const QJsonObject& respuesta);
    void handleStudentSchedulesResponse(const QJsonObject& respuesta);
    void handleLoadTestDataResponse(const QJsonObject& respuesta);

    QPushButton* m_btnTeacherSchedules = nullptr;
    QPushButton* m_btnStudentSchedules = nullptr;
    QPushButton* m_btnLoadSchedule = nullptr;
    InternalClient* m_client = nullptr;
    QJsonObject m_pendingRequest;
    QString m_schedulesDir;

    // Datos cargados
    QJsonObject m_loadedTeacherSchedules;
    QJsonObject m_loadedStudentSchedules;
    bool m_hasLoadedData = false;
};