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
#include <middleware/internalclient.h>
#include <middleware/messages.h>

class ScheduleVisualizationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ScheduleVisualizationWidget(QWidget *parent = nullptr);
    void setClient(InternalClient* client);

private slots:
    void onTeacherSchedulesClicked();
    void onStudentSchedulesClicked();
    void onLoadTestDataClicked();
    void onRespuestaRecibida(const QJsonObject& respuesta);

private:
    void setupUi();
    void sendRequest(const QString& op, const QJsonObject& payload);
    void openTeacherScheduleDialog(const QJsonObject& data);
    void openStudentScheduleDialog(const QJsonObject& data);
    void handleTeacherSchedulesResponse(const QJsonObject& respuesta);
    void handleStudentSchedulesResponse(const QJsonObject& respuesta);
    void handleLoadTestDataResponse(const QJsonObject& respuesta);

    QPushButton* m_btnTeacherSchedules = nullptr;
    QPushButton* m_btnStudentSchedules = nullptr;
    QPushButton* m_btnLoadTestData = nullptr;
    InternalClient* m_client = nullptr;
    QJsonObject m_pendingRequest;
};