/**
 * @file teacher_schedule_dialog.cpp
 * @brief Diálogo modal para visualizar y editar horarios de profesores
 */

#include "teacher_schedule_dialog.hpp"
#include <QGridLayout>
#include <QFrame>
#include <QMessageBox>
#include <QScrollBar>
#include <QDebug>
#include <QTimer>
#include <QInputDialog>
#include <QEvent>
#include <QJsonArray>
#include <QStringList>

TeacherScheduleDialog::TeacherScheduleDialog(const QJsonObject& schedulesData, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Horarios de Profesores");
    setModal(true);
    setMinimumSize(900, 700);
    resize(1000, 750);

    setStyleSheet(
        "QDialog { background-color: #fafafa; }"
        "QLabel { color: #1a1a2e; }"
        );

    setupUi(schedulesData);

    m_scrollAnimation = new QPropertyAnimation(m_scrollArea->verticalScrollBar(), "value", this);
    m_scrollAnimation->setDuration(500);
    m_scrollAnimation->setEasingCurve(QEasingCurve::OutCubic);

    if (!m_teacherSchedules.isEmpty()) {
        showTeacherSchedule(0);
    }
}

void TeacherScheduleDialog::setupUi(const QJsonObject& schedulesData)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // ---- Parsear datos ----
    const QJsonArray teachersArray = schedulesData.value("teachers").toArray();
    for (const auto& teacherVal : teachersArray) {
        const QJsonObject teacherObj = teacherVal.toObject();
        TeacherSchedule ts;
        ts.name = teacherObj.value("name").toString();
        ts.schedule = teacherObj.value("schedule").toObject();
        m_teacherSchedules.append(ts);
    }

    // ---- Header con navegación ----
    QWidget* headerWidget = new QWidget(this);
    headerWidget->setFixedHeight(80);
    headerWidget->setStyleSheet(
        "background-color: #3f51b5;"
        "border-bottom: 2px solid #303f9f;"
        );
    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(20, 10, 20, 10);

    m_btnPrevious = new QPushButton("← Anterior", headerWidget);
    m_btnPrevious->setCursor(Qt::PointingHandCursor);
    m_btnPrevious->setFixedSize(120, 45);
    m_btnPrevious->setStyleSheet(
        "QPushButton {"
        "   background-color: rgba(255,255,255,0.2);"
        "   color: white;"
        "   border: 1px solid rgba(255,255,255,0.3);"
        "   border-radius: 8px;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: rgba(255,255,255,0.3); }"
        "QPushButton:pressed { background-color: rgba(255,255,255,0.1); }"
        );
    connect(m_btnPrevious, &QPushButton::clicked, this, &TeacherScheduleDialog::onPreviousTeacher);
    headerLayout->addWidget(m_btnPrevious);

    headerLayout->addStretch();

    m_teacherNameLabel = new QLabel(headerWidget);
    m_teacherNameLabel->setStyleSheet(
        "font-size: 24px; font-weight: bold; color: white; background: transparent;"
        );
    m_teacherNameLabel->setAlignment(Qt::AlignCenter);
    headerLayout->addWidget(m_teacherNameLabel);

    headerLayout->addStretch();

    m_teacherCounter = new QLabel(headerWidget);
    m_teacherCounter->setStyleSheet(
        "font-size: 14px; color: rgba(255,255,255,0.8); background: transparent;"
        );
    headerLayout->addWidget(m_teacherCounter);

    headerLayout->addStretch();

    m_btnNext = new QPushButton("Siguiente →", headerWidget);
    m_btnNext->setCursor(Qt::PointingHandCursor);
    m_btnNext->setFixedSize(120, 45);
    m_btnNext->setStyleSheet(
        "QPushButton {"
        "   background-color: rgba(255,255,255,0.2);"
        "   color: white;"
        "   border: 1px solid rgba(255,255,255,0.3);"
        "   border-radius: 8px;"
        "   font-size: 14px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover { background-color: rgba(255,255,255,0.3); }"
        "QPushButton:pressed { background-color: rgba(255,255,255,0.1); }"
        );
    connect(m_btnNext, &QPushButton::clicked, this, &TeacherScheduleDialog::onNextTeacher);
    headerLayout->addWidget(m_btnNext);

    mainLayout->addWidget(headerWidget);

    // ---- Área de scroll ----
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setStyleSheet(
        "QScrollArea { background-color: #fafafa; border: none; }"
        "QScrollBar:vertical { background: #f0f0f0; width: 8px; border-radius: 4px; }"
        "QScrollBar::handle:vertical { background: #bdbdbd; border-radius: 4px; min-height: 30px; }"
        "QScrollBar::handle:vertical:hover { background: #9e9e9e; }"
        );

    m_contentWidget = new QWidget(m_scrollArea);
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    m_contentLayout->setSpacing(30);
    m_contentLayout->setContentsMargins(40, 30, 40, 30);

    m_contentLayout->addStretch(1);

    m_scheduleGridContainer = new QWidget(m_contentWidget);
    m_scheduleGridContainer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_scheduleGrid = new QGridLayout(m_scheduleGridContainer);
    m_scheduleGrid->setSpacing(2);
    m_scheduleGrid->setContentsMargins(0, 0, 0, 0);

    m_contentLayout->addWidget(m_scheduleGridContainer, 0, Qt::AlignHCenter);
    m_contentLayout->addStretch(1);

    m_scrollArea->setWidget(m_contentWidget);
    mainLayout->addWidget(m_scrollArea);

    updateNavigationButtons();
}

void TeacherScheduleDialog::showTeacherSchedule(int index)
{
    if (index < 0 || index >= m_teacherSchedules.size()) return;

    m_currentTeacherIndex = index;
    const auto& teacher = m_teacherSchedules[index];

    m_teacherNameLabel->setText(teacher.name);
    m_teacherCounter->setText(QString("%1 / %2").arg(index + 1).arg(m_teacherSchedules.size()));

    QLayoutItem* item;
    while ((item = m_scheduleGrid->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    createScheduleGrid(teacher.schedule);
    animateScrollTo(index);
    updateNavigationButtons();
}

void TeacherScheduleDialog::createScheduleGrid(const QJsonObject& schedule)
{
    const QStringList days = {"Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sábado"};

    // ⚠️ 'slots' es macro de Qt -> renombrado a 'timeSlots'
    const QStringList timeSlots = {
        "07:00-08:00", "08:00-09:00", "09:00-10:00", "10:00-11:00",
        "11:00-12:00", "12:00-13:00", "13:00-14:00", "14:00-15:00",
        "15:00-16:00", "16:00-17:00", "17:00-18:00", "18:00-19:00"
    };

    QLabel* cornerLabel = new QLabel("Hora / Día", m_scheduleGridContainer);
    cornerLabel->setFixedSize(120, 40);
    cornerLabel->setAlignment(Qt::AlignCenter);
    cornerLabel->setStyleSheet(
        "background-color: #1a237e; color: white; font-weight: bold;"
        "font-size: 12px; border: 1px solid #303f9f; border-radius: 4px;"
        );
    m_scheduleGrid->addWidget(cornerLabel, 0, 0);

    for (int d = 0; d < days.size(); ++d) {
        QLabel* dayLabel = new QLabel(days[d], m_scheduleGridContainer);
        dayLabel->setFixedSize(120, 40);
        dayLabel->setAlignment(Qt::AlignCenter);
        dayLabel->setStyleSheet(
            "background-color: #3f51b5; color: white; font-weight: bold;"
            "font-size: 12px; border: 1px solid #303f9f; border-radius: 4px;"
            );
        m_scheduleGrid->addWidget(dayLabel, 0, d + 1);
    }

    for (int s = 0; s < timeSlots.size(); ++s) {
        QLabel* timeLabel = new QLabel(timeSlots[s], m_scheduleGridContainer);
        timeLabel->setFixedSize(120, 50);
        timeLabel->setAlignment(Qt::AlignCenter);
        timeLabel->setStyleSheet(
            "background-color: #e8eaf6; color: #1a237e; font-weight: bold;"
            "font-size: 11px; border: 1px solid #c5cae9; border-radius: 4px;"
            );
        m_scheduleGrid->addWidget(timeLabel, s + 1, 0);

        for (int d = 0; d < days.size(); ++d) {
            const QString dayKey = QString::number(d + 1);
            const QString slotKey = QString::number(s);

            QString subjectName;
            if (schedule.contains(dayKey)) {
                const QJsonObject dayObj = schedule.value(dayKey).toObject();
                if (dayObj.contains(slotKey)) {
                    subjectName = dayObj.value(slotKey).toString();
                }
            }

            QFrame* cell = new QFrame(m_scheduleGridContainer);
            cell->setFixedSize(120, 50);
            cell->setFrameShape(QFrame::Box);
            cell->setLineWidth(1);
            cell->setCursor(Qt::PointingHandCursor);
            cell->setStyleSheet(
                subjectName.isEmpty()
                    ? "QFrame { border: 1px solid #e0e0e0; border-radius: 4px; background-color: white; }"
                    : "QFrame { border: 1px solid #c5cae9; border-radius: 4px; background-color: #e3f2fd; }"
                );

            QVBoxLayout* cellLayout = new QVBoxLayout(cell);
            cellLayout->setContentsMargins(4, 4, 4, 4);
            cellLayout->setSpacing(2);

            QLabel* subjectLabel = new QLabel(subjectName.isEmpty() ? "—" : subjectName, cell);
            subjectLabel->setAlignment(Qt::AlignCenter);
            subjectLabel->setWordWrap(true);
            subjectLabel->setStyleSheet(
                QString("font-size: 11px; font-weight: %1; color: %2; background: transparent;")
                    .arg(subjectName.isEmpty() ? "normal" : "bold")
                    .arg(subjectName.isEmpty() ? "#9e9e9e" : "#1565c0")
                );
            cellLayout->addWidget(subjectLabel);

            cell->setProperty("day", d);
            cell->setProperty("slot", s);
            cell->setProperty("subject", subjectName);
            cell->setProperty("teacherName", m_teacherSchedules[m_currentTeacherIndex].name);

            cell->installEventFilter(this);
            m_scheduleGrid->addWidget(cell, s + 1, d + 1);
        }
    }
}

bool TeacherScheduleDialog::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QFrame* cell = qobject_cast<QFrame*>(watched);
        if (cell) {
            onCellClicked(cell->property("day").toInt(),
                          cell->property("slot").toInt(),
                          cell->property("teacherName").toString(),
                          cell->property("subject").toString());
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void TeacherScheduleDialog::onCellClicked(int day, int slot,
                                          const QString& teacherName,
                                          const QString& currentSubject)
{
    QStringList availableSubjects = {
        "Matemáticas", "Física", "Química", "Biología", "Historia",
        "Geografía", "Literatura", "Inglés", "Educación Física",
        "Arte", "Música", "Tecnología", "Filosofía", "Ciudadanía"
    };
    availableSubjects.removeAll(currentSubject);

    const QStringList dayNames = {"Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sábado"};
    const QStringList slotNames = {"07-08","08-09","09-10","10-11","11-12","12-13",
                                   "13-14","14-15","15-16","16-17","17-18","18-19"};

    bool ok = false;
    const QString newSubject = QInputDialog::getItem(
        this,
        "Cambiar Materia",
        QString("Profesor: %1\nDía: %2\nFranja: %3\nMateria actual: %4\n\nSeleccione nueva materia:")
            .arg(teacherName)
            .arg(dayNames.value(day, "?"))
            .arg(slotNames.value(slot, "?"))
            .arg(currentSubject.isEmpty() ? "—" : currentSubject),
        availableSubjects, 0, false, &ok);

    if (!ok || newSubject.isEmpty() || newSubject == currentSubject) return;

    if (checkConflict(teacherName, day, slot, newSubject)) {
        showConflictDialog(teacherName, day, slot, currentSubject, newSubject);
    } else {
        applySubjectChange(teacherName, day, slot, newSubject);
    }
}

bool TeacherScheduleDialog::checkConflict(const QString& teacherName, int day, int slot,
                                          const QString& newSubject)
{
    for (const auto& teacher : m_teacherSchedules) {
        if (teacher.name == teacherName) {
            const QString dayKey = QString::number(day + 1);
            if (teacher.schedule.contains(dayKey)) {
                const QJsonObject dayObj = teacher.schedule.value(dayKey).toObject();
                for (auto it = dayObj.constBegin(); it != dayObj.constEnd(); ++it) {
                    if (it.key() != QString::number(slot) && it.value().toString() == newSubject) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void TeacherScheduleDialog::showConflictDialog(const QString& teacherName, int day, int slot,
                                               const QString& currentSubject,
                                               const QString& newSubject)
{
    Q_UNUSED(currentSubject);

    const QStringList dayNames = {"Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sábado"};

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("⚠️ Conflicto de Horario");
    msgBox.setText(QString("El profesor <b>%1</b> ya tiene la materia <b>%2</b> "
                           "en otra franja del mismo día (%3).")
                       .arg(teacherName)
                       .arg(newSubject)
                       .arg(dayNames.value(day, "?")));
    msgBox.setInformativeText("¿Desea reemplazar la materia actual por la nueva?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setIcon(QMessageBox::Warning);

    // ✅ qobject_cast porque button() devuelve QAbstractButton*
    if (auto* btnReplace = qobject_cast<QPushButton*>(msgBox.button(QMessageBox::Yes))) {
        btnReplace->setText("Reemplazar");
        btnReplace->setStyleSheet(
            "background-color: #f44336; color: white; padding: 8px 16px; border-radius: 4px;");
    }
    if (auto* btnCancel = qobject_cast<QPushButton*>(msgBox.button(QMessageBox::No))) {
        btnCancel->setText("Cancelar");
        btnCancel->setStyleSheet(
            "background-color: #757575; color: white; padding: 8px 16px; border-radius: 4px;");
    }

    if (msgBox.exec() == QMessageBox::Yes) {
        applySubjectChange(teacherName, day, slot, newSubject);
    }
}

void TeacherScheduleDialog::applySubjectChange(const QString& teacherName, int day, int slot,
                                               const QString& newSubject)
{
    for (auto& teacher : m_teacherSchedules) {
        if (teacher.name == teacherName) {
            const QString dayKey = QString::number(day + 1);
            QJsonObject dayObj = teacher.schedule.value(dayKey).toObject();
            dayObj[QString::number(slot)] = newSubject;
            teacher.schedule[dayKey] = dayObj;
            break;
        }
    }

    showTeacherSchedule(m_currentTeacherIndex);

    QJsonObject updatedData;
    QJsonArray teachersArray;
    for (const auto& teacher : m_teacherSchedules) {
        QJsonObject tObj;
        tObj["name"] = teacher.name;
        tObj["schedule"] = teacher.schedule;
        teachersArray.append(tObj);
    }
    updatedData["teachers"] = teachersArray;
    emit scheduleModified(updatedData);
}

void TeacherScheduleDialog::animateScrollTo(int /*targetIndex*/)
{
    QTimer::singleShot(50, this, [this]() {
        const int targetY = m_scheduleGridContainer->mapTo(m_contentWidget, QPoint(0, 0)).y();
        const int viewportCenter = m_scrollArea->viewport()->height() / 2;
        const int widgetCenter = m_scheduleGridContainer->height() / 2;
        const int scrollTarget = targetY - viewportCenter + widgetCenter;

        m_scrollAnimation->stop();
        m_scrollAnimation->setStartValue(m_scrollArea->verticalScrollBar()->value());
        m_scrollAnimation->setEndValue(qMax(0, scrollTarget));
        m_scrollAnimation->start();
    });
}

void TeacherScheduleDialog::onPreviousTeacher()
{
    if (m_currentTeacherIndex > 0) {
        showTeacherSchedule(m_currentTeacherIndex - 1);
    }
}

void TeacherScheduleDialog::onNextTeacher()
{
    if (m_currentTeacherIndex < m_teacherSchedules.size() - 1) {
        showTeacherSchedule(m_currentTeacherIndex + 1);
    }
}

void TeacherScheduleDialog::updateNavigationButtons()
{
    m_btnPrevious->setEnabled(m_currentTeacherIndex > 0);
    m_btnNext->setEnabled(m_currentTeacherIndex < m_teacherSchedules.size() - 1);
}