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
#include <QEvent>             // para eventFilter

/**
 * @brief Diálogo de visualización/edición de horarios de estudiantes.
 *
 * Estética basada en el prototipo SchedulePrototype: pestañas de año tipo
 * "pill", navegador vertical de secciones con píldora activa en degradado,
 * cuadrícula con bloques de materia coloreados y leyenda flotante.
 *
 * La selección de año y sección usa QButtonGroup exclusivos para garantizar
 * que solo una pestaña aparezca activa a la vez.
 */
class StudentScheduleDialog : public QDialog
{
    Q_OBJECT
public:
    explicit StudentScheduleDialog(const QJsonObject& schedulesData, QWidget* parent = nullptr);
    ~StudentScheduleDialog() override = default;

signals:
    void scheduleModified(const QJsonObject& updatedSchedules);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void onCellClicked(int yearIndex, int sectionIndex, int day, int slot, const QString& subjectName);

private:
    void setupUi(const QJsonObject& schedulesData);
    void buildStyles();
    void createYearNavigation();
    void updateSectionNavigation(int yearIndex);
    void showYearSchedule(int yearIndex);
    void selectSection(int sectionIndex);
    void createScheduleGrid(int yearIndex, int sectionIndex);
    int  countBlocks(const QJsonObject& schedule) const;

    bool checkConflict(int yearIndex, int sectionIndex, int day, int slot, const QString& newSubject);
    void showConflictDialog(int yearIndex, int sectionIndex, int day, int slot,
                            const QString& currentSubject, const QString& newSubject);
    void applySubjectChange(int yearIndex, int sectionIndex, int day, int slot, const QString& newSubject);

    struct SectionSchedule {
        QString name;
        QJsonObject schedule;
    };

    struct YearSchedule {
        int year = 0;
        QString name;
        QVector<SectionSchedule> sections;
    };

    QVector<YearSchedule> m_years;
    int m_currentYearIndex = 0;
    QVector<int> m_currentSectionIndices;

    // Cabecera
    QLabel* m_eyebrowLabel = nullptr;
    QLabel* m_titleLabel = nullptr;
    QLabel* m_subtitleLabel = nullptr;

    // Pestañas de año
    QWidget* m_yearNavWidget = nullptr;
    QHBoxLayout* m_yearNavLayout = nullptr;
    QButtonGroup* m_yearGroup = nullptr;
    QVector<QPushButton*> m_yearButtons;

    // Navegador de secciones
    QWidget* m_sectionNavHost = nullptr;
    QVBoxLayout* m_sectionNavLayout = nullptr;
    QButtonGroup* m_sectionGroup = nullptr;
    QVector<QPushButton*> m_sectionButtons;

    // Cuadrícula
    QScrollArea* m_gridScroll = nullptr;
    QWidget* m_gridHost = nullptr;
    QGridLayout* m_scheduleGrid = nullptr;

    bool m_entranceDone = false;
};
