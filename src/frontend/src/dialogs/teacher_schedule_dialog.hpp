#pragma once

#include "schedule_grid_dialog.hpp"

#include <QString>
#include <QStringList>
#include <QVector>

class QPushButton;
class QButtonGroup;
class QWidget;

/**
 * @brief Diálogo de visualización/edición de horarios de profesores.
 *
 * La rejilla, el arrastrar/soltar y los conflictos viven en ScheduleGridDialog.
 * Aquí sólo se aporta el navegador vertical de docentes y el título.
 */
class TeacherScheduleDialog : public ScheduleGridDialog
{
    Q_OBJECT
public:
    explicit TeacherScheduleDialog(const QJsonObject& schedulesData, QWidget* parent = nullptr);
    ~TeacherScheduleDialog() override = default;

protected:
    ScheduleModel::Ref currentRef() const override;
    QString            displayName() const override;
    void               onScheduleChanged() override;

private:
    void setupUi();
    QWidget* buildNavigator(QWidget* parent);
    void showTeacher(int index);
    void updateNavButtons();

    QStringList m_teacherNames;
    int m_currentIndex = 0;

    QButtonGroup* m_navGroup = nullptr;
    QVector<QPushButton*> m_navButtons;
    QPushButton* m_btnPrevious = nullptr;
    QPushButton* m_btnNext = nullptr;
};
