#pragma once

#include <QDialog>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>        // usado como miembro m_scheduleGrid
#include <QLabel>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QVector>
#include <QWidget>
#include <QTimer>
#include <QEvent>             // para eventFilter

class TeacherScheduleDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TeacherScheduleDialog(const QJsonObject& schedulesData, QWidget* parent = nullptr);
    ~TeacherScheduleDialog() override = default;

signals:
    void scheduleModified(const QJsonObject& updatedSchedules);

protected:
    // Lo define el .cpp; es virtual de QObject y necesita override
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onPreviousTeacher();
    void onNextTeacher();
    void onCellClicked(int day, int slot,
                       const QString& teacherName,
                       const QString& currentSubject);

private:
    void setupUi(const QJsonObject& schedulesData);
    void showTeacherSchedule(int index);
    void createScheduleGrid(const QJsonObject& teacherSchedule);
    void animateScrollTo(int targetIndex);
    bool checkConflict(const QString& teacherName, int day, int slot,
                       const QString& newSubject);
    void showConflictDialog(const QString& teacherName, int day, int slot,
                            const QString& currentSubject,
                            const QString& newSubject);
    void applySubjectChange(const QString& teacherName, int day, int slot,
                            const QString& newSubject);
    void updateNavigationButtons();

    struct TeacherSchedule {
        QString name;
        QJsonObject schedule; // day -> slot -> subject
    };

    QVector<TeacherSchedule> m_teacherSchedules;
    int m_currentTeacherIndex = 0;

    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_contentWidget = nullptr;
    QVBoxLayout* m_contentLayout = nullptr;

    QLabel* m_teacherNameLabel = nullptr;
    QLabel* m_teacherCounter = nullptr;

    QWidget* m_scheduleGridContainer = nullptr;
    QGridLayout* m_scheduleGrid = nullptr;

    QPushButton* m_btnPrevious = nullptr;
    QPushButton* m_btnNext = nullptr;

    QPropertyAnimation* m_scrollAnimation = nullptr;
};