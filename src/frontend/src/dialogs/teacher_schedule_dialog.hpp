#pragma once

#include <QDialog>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QButtonGroup>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QVector>
#include <QWidget>
#include <QTimer>
#include <QEvent>            // para eventFilter

/**
 * @brief Diálogo de visualización/edición de horarios de profesores.
 *
 * Estética basada en el prototipo SchedulePrototype: superficies claras,
 * tarjetas con sombra, navegador vertical de docentes con píldora activa en
 * degradado índigo y bloques de materia coloreados.
 */
class TeacherScheduleDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TeacherScheduleDialog(const QJsonObject& schedulesData, QWidget* parent = nullptr);
    ~TeacherScheduleDialog() override = default;

signals:
    void scheduleModified(const QJsonObject& updatedSchedules);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void onPreviousTeacher();
    void onNextTeacher();
    void onCellClicked(int day, int slot,
                       const QString& teacherName,
                       const QString& currentSubject);

private:
    void setupUi(const QJsonObject& schedulesData);
    void buildStyles();
    void buildNavigator();
    void showTeacherSchedule(int index);
    void createScheduleGrid(const QJsonObject& teacherSchedule);
    void updateNavigationButtons();
    int  countBlocks(const QJsonObject& schedule) const;

    bool checkConflict(const QString& teacherName, int day, int slot,
                       const QString& newSubject);
    void showConflictDialog(const QString& teacherName, int day, int slot,
                            const QString& currentSubject,
                            const QString& newSubject);
    void applySubjectChange(const QString& teacherName, int day, int slot,
                            const QString& newSubject);

    struct TeacherSchedule {
        QString name;
        QJsonObject schedule; // day -> slot -> subject
    };

    QVector<TeacherSchedule> m_teacherSchedules;
    int m_currentTeacherIndex = 0;

    // Cabecera
    QLabel* m_eyebrowLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_subtitleLabel = nullptr;
    QPushButton* m_btnPrevious = nullptr;
    QPushButton* m_btnNext = nullptr;

    // Navegador de docentes
    QButtonGroup* m_navGroup = nullptr;
    QVector<QPushButton*> m_navButtons;

    // Cuadrícula
    QScrollArea* m_gridScroll = nullptr;
    QWidget* m_gridHost = nullptr;
    QGridLayout* m_scheduleGrid = nullptr;

    bool m_entranceDone = false;
};
