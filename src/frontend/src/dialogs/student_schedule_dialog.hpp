#pragma once

#include "schedule_grid_dialog.hpp"

#include <QString>
#include <QStringList>
#include <QVector>

class QPushButton;
class QButtonGroup;
class QWidget;
class QHBoxLayout;
class QVBoxLayout;

/**
 * @brief Diálogo de visualización/edición de horarios de estudiantes.
 *
 * Pestañas de año tipo "pill", navegador vertical de secciones y la rejilla
 * compartida con receso y arrastrar/soltar.
 */
class StudentScheduleDialog : public ScheduleGridDialog
{
    Q_OBJECT
public:
    explicit StudentScheduleDialog(const QJsonObject& schedulesData, QWidget* parent = nullptr);
    ~StudentScheduleDialog() override = default;

protected:
    ScheduleModel::Ref currentRef() const override;
    QString            displayName() const override;
    void               onScheduleChanged() override;

private:
    struct YearInfo {
        int year = 0;
        QString name;
        QStringList sectionNames;
    };

    void setupUi();
    QWidget* buildSectionNavigator(QWidget* parent);
    void createYearNavigation();
    void updateSectionNavigation(int yearIndex);
    void showYear(int yearIndex);
    void selectSection(int sectionIndex);

    QVector<YearInfo> m_years;
    int m_currentYearIndex = 0;
    QVector<int> m_currentSectionIndices;

    QWidget* m_yearNavWidget = nullptr;
    QHBoxLayout* m_yearNavLayout = nullptr;
    QButtonGroup* m_yearGroup = nullptr;
    QVector<QPushButton*> m_yearButtons;

    QWidget* m_sectionNavHost = nullptr;
    QVBoxLayout* m_sectionNavLayout = nullptr;
    QButtonGroup* m_sectionGroup = nullptr;
    QVector<QPushButton*> m_sectionButtons;
};
