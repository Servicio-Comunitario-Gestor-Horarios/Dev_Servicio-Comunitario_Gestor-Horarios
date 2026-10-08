#pragma once

#include <QDialog>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>        // usado como miembro (m_scheduleGrid)
#include <QLabel>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>            // usado en structs y slots
#include <QVector>
#include <QMap>
#include <QWidget>
#include <QTimer>
#include <QStackedWidget>
#include <QEvent>             // para eventFilter

class StudentScheduleDialog : public QDialog
{
    Q_OBJECT
public:
    explicit StudentScheduleDialog(const QJsonObject& schedulesData, QWidget* parent = nullptr);
    ~StudentScheduleDialog() override = default;

signals:
    void scheduleModified(const QJsonObject& updatedSchedules);

protected:
    // Necesario: el .cpp lo define y es virtual de QObject
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onYearChanged(int yearIndex);
    void onSectionChanged(int yearIndex, int sectionIndex);
    void onCellClicked(int yearIndex, int sectionIndex, int day, int slot, const QString& subjectName);

private:
    void setupUi(const QJsonObject& schedulesData);
    void showYearSchedule(int yearIndex);
    void createYearNavigation();
    void updateSectionNavigation(int yearIndex);   // <-- AÑADIDO (lo usa el .cpp)
    void createScheduleGrid(int yearIndex, int sectionIndex);
    void animateScrollToYear(int yearIndex);
    bool checkConflict(int yearIndex, int sectionIndex, int day, int slot, const QString& newSubject);
    void showConflictDialog(int yearIndex, int sectionIndex, int day, int slot,
                            const QString& currentSubject, const QString& newSubject);
    void applySubjectChange(int yearIndex, int sectionIndex, int day, int slot, const QString& newSubject);

    struct SectionSchedule {
        QString name;
        QJsonObject schedule;
    };

    struct YearSchedule {
        int year;
        QString name;
        QVector<SectionSchedule> sections;
    };

    QVector<YearSchedule> m_years;
    int m_currentYearIndex = 0;
    QVector<int> m_currentSectionIndices;

    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_contentWidget = nullptr;
    QVBoxLayout* m_contentLayout = nullptr;

    QWidget* m_yearNavWidget = nullptr;
    QHBoxLayout* m_yearNavLayout = nullptr;
    QVector<QPushButton*> m_yearButtons;

    QWidget* m_sectionNavWidget = nullptr;
    QHBoxLayout* m_sectionNavLayout = nullptr;
    QVector<QVector<QPushButton*>> m_sectionButtons;

    QWidget* m_scheduleGridContainer = nullptr;
    QGridLayout* m_scheduleGrid = nullptr;
    QLabel* m_scheduleTitleLabel = nullptr;

    QPropertyAnimation* m_scrollAnimation = nullptr;
};