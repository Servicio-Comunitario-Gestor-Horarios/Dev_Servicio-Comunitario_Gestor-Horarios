/**
 * @file teacher_schedule_dialog.cpp
 * @brief Diálogo de horarios de profesores con la estética del prototipo
 *        SchedulePrototype: superficies claras, tarjetas con sombra,
 *        navegador vertical de docentes y bloques de materia coloreados.
 */

#include "teacher_schedule_dialog.hpp"
#include "schedule_theme.hpp"

#include <QGridLayout>
#include <QFrame>
#include <QMessageBox>
#include <QScrollBar>
#include <QScrollArea>
#include <QDebug>
#include <QTimer>
#include <QInputDialog>
#include <QEvent>
#include <QShowEvent>
#include <QFontMetrics>
#include <QJsonArray>
#include <QStringList>

using namespace ScheduleTheme;

namespace {
// Dimensiones de la cuadrícula (en px)
constexpr int kTimeColW = 108;
constexpr int kDayColW  = 126;
constexpr int kHeaderH  = 46;
constexpr int kRowH     = 58;

const QStringList kDays = {"Lunes", "Martes", "Miércoles",
                           "Jueves", "Viernes", "Sábado"};

const QStringList kSlots = {
    "07:00-08:00", "08:00-09:00", "09:00-10:00", "10:00-11:00",
    "11:00-12:00", "12:00-13:00", "13:00-14:00", "14:00-15:00",
    "15:00-16:00", "16:00-17:00", "17:00-18:00", "18:00-19:00"
};

const QStringList kSlotShort = {"07-08","08-09","09-10","10-11","11-12","12-13",
                                "13-14","14-15","15-16","16-17","17-18","18-19"};

// Materias de la leyenda (coinciden con la paleta del prototipo)
const QStringList kLegendSubjects = {
    "Matemáticas", "Física", "Química", "Literatura",
    "Historia", "Inglés", "Educación Física", "Arte"
};
} // namespace

TeacherScheduleDialog::TeacherScheduleDialog(const QJsonObject& schedulesData, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Horarios de Profesores");
    setModal(true);
    setMinimumSize(1040, 720);
    resize(1200, 820);

    setupUi(schedulesData);

    if (!m_teacherSchedules.isEmpty()) {
        showTeacherSchedule(0);
    }
}

void TeacherScheduleDialog::setupUi(const QJsonObject& schedulesData)
{
    buildStyles();

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 22, 24, 22);
    root->setSpacing(18);

    // ---- Parsear datos ----
    const QJsonArray teachersArray = schedulesData.value("teachers").toArray();
    for (const auto& teacherVal : teachersArray) {
        const QJsonObject teacherObj = teacherVal.toObject();
        TeacherSchedule ts;
        ts.name = teacherObj.value("name").toString();
        ts.schedule = teacherObj.value("schedule").toObject();
        m_teacherSchedules.append(ts);
    }

    // =========================================================
    // CABECERA
    // =========================================================
    QWidget* header = new QWidget(this);
    header->setObjectName("Header");
    QHBoxLayout* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(2, 0, 2, 0);
    headerLayout->setSpacing(16);

    QVBoxLayout* titleBox = new QVBoxLayout();
    titleBox->setSpacing(2);

    m_eyebrowLabel = new QLabel("HORARIOS DE PROFESORES", header);
    m_eyebrowLabel->setObjectName("Eyebrow");
    titleBox->addWidget(m_eyebrowLabel);

    m_titleLabel = new QLabel("—", header);
    m_titleLabel->setObjectName("Title");
    titleBox->addWidget(m_titleLabel);

    m_subtitleLabel = new QLabel(header);
    m_subtitleLabel->setObjectName("Subtitle");
    titleBox->addWidget(m_subtitleLabel);

    headerLayout->addLayout(titleBox);
    headerLayout->addStretch();

    m_btnPrevious = new QPushButton("←  Anterior", header);
    m_btnPrevious->setObjectName("GhostBtn");
    m_btnPrevious->setCursor(Qt::PointingHandCursor);
    m_btnPrevious->setMinimumHeight(40);
    connect(m_btnPrevious, &QPushButton::clicked, this, &TeacherScheduleDialog::onPreviousTeacher);
    headerLayout->addWidget(m_btnPrevious, 0, Qt::AlignBottom);

    m_btnNext = new QPushButton("Siguiente  →", header);
    m_btnNext->setObjectName("GhostBtn");
    m_btnNext->setCursor(Qt::PointingHandCursor);
    m_btnNext->setMinimumHeight(40);
    connect(m_btnNext, &QPushButton::clicked, this, &TeacherScheduleDialog::onNextTeacher);
    headerLayout->addWidget(m_btnNext, 0, Qt::AlignBottom);

    root->addWidget(header);

    // =========================================================
    // CUERPO: cuadrícula + navegador de docentes
    // =========================================================
    QHBoxLayout* body = new QHBoxLayout();
    body->setSpacing(16);

    // --- Tarjeta de la cuadrícula ---
    QFrame* scheduleCard = new QFrame(this);
    scheduleCard->setObjectName("ScheduleCard");
    applyShadow(scheduleCard, darkMode());
    QVBoxLayout* cardLayout = new QVBoxLayout(scheduleCard);
    cardLayout->setContentsMargins(10, 10, 10, 10);
    cardLayout->setSpacing(0);

    m_gridScroll = new QScrollArea(scheduleCard);
    m_gridScroll->setObjectName("GridScroll");
    m_gridScroll->setFrameShape(QFrame::NoFrame);
    m_gridScroll->setWidgetResizable(false);
    m_gridScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_gridScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_gridHost = new QWidget(m_gridScroll);
    m_gridHost->setObjectName("GridHost");
    const Palette p = current();
    m_gridHost->setStyleSheet(QString("QWidget#GridHost { background: %1; }").arg(p.border));

    m_scheduleGrid = new QGridLayout(m_gridHost);
    m_scheduleGrid->setSpacing(1);
    m_scheduleGrid->setContentsMargins(0, 0, 0, 0);

    m_gridScroll->setWidget(m_gridHost);
    m_gridScroll->viewport()->setAutoFillBackground(false);
    cardLayout->addWidget(m_gridScroll);
    scheduleCard->setMinimumWidth(460);
    body->addWidget(scheduleCard, 1);

    // --- Tarjeta navegador de docentes ---
    QFrame* navCard = new QFrame(this);
    navCard->setObjectName("NavCard");
    applyShadow(navCard, darkMode());
    navCard->setFixedWidth(238);
    QVBoxLayout* navLayout = new QVBoxLayout(navCard);
    navLayout->setContentsMargins(12, 14, 12, 14);
    navLayout->setSpacing(10);

    QLabel* navTitle = new QLabel("DOCENTES", navCard);
    navTitle->setObjectName("NavTitle");
    navLayout->addWidget(navTitle);

    QScrollArea* navScroll = new QScrollArea(navCard);
    navScroll->setObjectName("NavScroll");
    navScroll->setWidgetResizable(true);
    navScroll->setFrameShape(QFrame::NoFrame);
    navScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget* navHost = new QWidget(navScroll);
    navHost->setObjectName("NavHost");
    QVBoxLayout* navHostLayout = new QVBoxLayout(navHost);
    navHostLayout->setContentsMargins(0, 0, 0, 0);
    navHostLayout->setSpacing(4);

    m_navGroup = new QButtonGroup(this);
    m_navGroup->setExclusive(true);

    for (int i = 0; i < m_teacherSchedules.size(); ++i) {
        QPushButton* btn = new QPushButton(navHost);
        btn->setObjectName("NavBtn");
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumHeight(40);
        btn->setToolTip(m_teacherSchedules[i].name);

        const QFontMetrics fm(btn->font());
        btn->setText(fm.elidedText(m_teacherSchedules[i].name, Qt::ElideRight, 176));

        navHostLayout->addWidget(btn);
        m_navButtons.append(btn);
        m_navGroup->addButton(btn, i);

        connect(btn, &QPushButton::clicked, this, [this, i]() {
            showTeacherSchedule(i);
        });
    }
    navHostLayout->addStretch();

    navScroll->setWidget(navHost);
    navScroll->viewport()->setAutoFillBackground(false);
    navLayout->addWidget(navScroll, 1);
    body->addWidget(navCard, 0, Qt::AlignTop);

    root->addLayout(body, 1);

    // =========================================================
    // LEYENDA
    // =========================================================
    QFrame* legendCard = new QFrame(this);
    legendCard->setObjectName("LegendCard");
    applyShadow(legendCard, darkMode(), 22, 6);
    QHBoxLayout* legendLayout = new QHBoxLayout(legendCard);
    legendLayout->setContentsMargins(16, 10, 16, 10);
    legendLayout->setSpacing(18);

    QLabel* legendTitle = new QLabel("LEYENDA", legendCard);
    legendTitle->setObjectName("LegendTitle");
    legendLayout->addWidget(legendTitle);

    for (const QString& subject : kLegendSubjects) {
        QWidget* itemWidget = new QWidget(legendCard);
        itemWidget->setObjectName("LegendItem");
        QHBoxLayout* itemLayout = new QHBoxLayout(itemWidget);
        itemLayout->setContentsMargins(0, 0, 0, 0);
        itemLayout->setSpacing(7);

        QFrame* swatch = new QFrame(itemWidget);
        swatch->setFixedSize(14, 14);
        swatch->setStyleSheet(QString("background: %1; border-radius: 4px;")
                                  .arg(subjectColor(subject).name()));
        itemLayout->addWidget(swatch);

        QLabel* text = new QLabel(subject, itemWidget);
        text->setObjectName("LegendText");
        itemLayout->addWidget(text);

        legendLayout->addWidget(itemWidget);
    }
    legendLayout->addStretch();

    root->addWidget(legendCard);

    updateNavigationButtons();
}

void TeacherScheduleDialog::buildStyles()
{
    const Palette p = current();

    QString qss = QString(
        "QDialog { background-color: @BG@; border: none; }"
        "#Header, #GridHost, #NavHost, #LegendItem { background: transparent; }"

        "#Eyebrow { color: @ACCENT@; font-size: 11px; font-weight: 700; letter-spacing: 2px; background: transparent; border: none; }"
        "#Title { color: @TEXT@; font-size: 24px; font-weight: 800; background: transparent; border: none; }"
        "#Subtitle { color: @MUTED@; font-size: 13px; background: transparent; border: none; }"

        "#GhostBtn { background-color: @SURFACE@; color: @MUTED@; border: 1px solid @BORDER@;"
        "  border-radius: 10px; padding: 8px 16px; font-size: 13px; font-weight: 600; }"
        "#GhostBtn:hover { color: @TEXT@; background-color: @SURF2@; border-color: @BORDERSTRONG@; }"
        "#GhostBtn:pressed { background-color: @SURF3@; }"
        "#GhostBtn:disabled { color: @SUBTLE@; background-color: @SURF2@; border-color: @BORDER@; }"

        "#ScheduleCard, #NavCard, #LegendCard { background-color: @SURFACE@; border: 1px solid @BORDER@; border-radius: 18px; }"
        "#NavTitle, #LegendTitle { color: @SUBTLE@; font-size: 10px; font-weight: 700; letter-spacing: 1.5px; background: transparent; border: none; }"
        "#LegendText { color: @MUTED@; font-size: 12px; background: transparent; border: none; }"

        "#NavBtn { background: transparent; color: @MUTED@; border: 1px solid transparent;"
        "  border-radius: 10px; padding: 9px 14px; font-size: 13px; font-weight: 600; text-align: left; }"
        "#NavBtn:hover { background-color: @SURF2@; color: @TEXT@; }"
        "#NavBtn:checked { color: #FFFFFF; font-weight: 700; border: 1px solid @ACCENTSTRONG@;"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 @ACCENT@, stop:1 @ACCENTSTRONG@); }"

        "#GridScroll, #NavScroll { background: transparent; border: none; }"

        "QScrollBar:vertical { background: transparent; width: 10px; margin: 4px; }"
        "QScrollBar::handle:vertical { background: @BORDERSTRONG@; border-radius: 5px; min-height: 40px; }"
        "QScrollBar::handle:vertical:hover { background: @SUBTLE@; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }"
        "QScrollBar:horizontal { background: transparent; height: 10px; margin: 4px; }"
        "QScrollBar::handle:horizontal { background: @BORDERSTRONG@; border-radius: 5px; min-width: 40px; }"
        "QScrollBar::handle:horizontal:hover { background: @SUBTLE@; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }"
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: transparent; }"
    );

    qss.replace("@BG@", p.bg);
    qss.replace("@SURFACE@", p.surface);
    qss.replace("@SURF2@", p.surface2);
    qss.replace("@SURF3@", p.surface3);
    qss.replace("@BORDERSTRONG@", p.borderStrong);
    qss.replace("@BORDER@", p.border);
    qss.replace("@TEXT@", p.text);
    qss.replace("@MUTED@", p.textMuted);
    qss.replace("@SUBTLE@", p.textSubtle);
    qss.replace("@ACCENTSTRONG@", p.accentStrong);
    qss.replace("@ACCENT@", p.accent);

    setStyleSheet(qss);
}

void TeacherScheduleDialog::showTeacherSchedule(int index)
{
    if (index < 0 || index >= m_teacherSchedules.size()) return;

    m_currentTeacherIndex = index;
    const auto& teacher = m_teacherSchedules[index];

    m_titleLabel->setText(teacher.name);
    m_subtitleLabel->setText(QString("Docente %1 de %2  ·  %3 bloques asignados")
                                 .arg(index + 1)
                                 .arg(m_teacherSchedules.size())
                                 .arg(countBlocks(teacher.schedule)));

    // Actualizar navegador (solo una píldora activa)
    for (int i = 0; i < m_navButtons.size(); ++i) {
        m_navButtons[i]->setChecked(i == index);
    }

    createScheduleGrid(teacher.schedule);
    updateNavigationButtons();

    // Volver al inicio de la cuadrícula
    QTimer::singleShot(0, this, [this]() {
        m_gridScroll->verticalScrollBar()->setValue(0);
        m_gridScroll->horizontalScrollBar()->setValue(0);
    });
}

void TeacherScheduleDialog::createScheduleGrid(const QJsonObject& schedule)
{
    const Palette p = current();

    // Limpiar cuadrícula anterior
    QLayoutItem* item;
    while ((item = m_scheduleGrid->takeAt(0)) != nullptr) {
        if (item->widget()) { item->widget()->hide(); item->widget()->deleteLater(); }
        delete item;
    }

    const QString headStyle = QString(
        "background-color: %1; color: %2; font-weight: 700; font-size: 11px;"
        "letter-spacing: 0.5px; border: none; border-radius: 8px;")
        .arg(p.surface2, p.textSubtle);

    // Esquina
    QLabel* corner = new QLabel("HORA", m_gridHost);
    corner->setFixedSize(kTimeColW, kHeaderH);
    corner->setAlignment(Qt::AlignCenter);
    corner->setStyleSheet(headStyle);
    m_scheduleGrid->addWidget(corner, 0, 0);

    // Cabeceras de días
    for (int d = 0; d < kDays.size(); ++d) {
        QLabel* day = new QLabel(kDays[d].toUpper(), m_gridHost);
        day->setFixedSize(kDayColW, kHeaderH);
        day->setAlignment(Qt::AlignCenter);
        day->setStyleSheet(headStyle);
        m_scheduleGrid->addWidget(day, 0, d + 1);
    }

    // Filas de franjas horarias
    for (int s = 0; s < kSlots.size(); ++s) {
        QLabel* timeLabel = new QLabel(kSlots[s], m_gridHost);
        timeLabel->setFixedSize(kTimeColW, kRowH);
        timeLabel->setAlignment(Qt::AlignCenter);
        timeLabel->setStyleSheet(QString(
            "background-color: %1; color: %2; font-weight: 600; font-size: 11px;"
            "border: none; border-radius: 8px;")
            .arg(p.surface2, p.textMuted));
        m_scheduleGrid->addWidget(timeLabel, s + 1, 0);

        for (int d = 0; d < kDays.size(); ++d) {
            const QString dayKey = QString::number(d + 1);
            const QString slotKey = QString::number(s);

            QString subjectName;
            if (schedule.contains(dayKey)) {
                const QJsonObject dayObj = schedule.value(dayKey).toObject();
                if (dayObj.contains(slotKey)) {
                    subjectName = dayObj.value(slotKey).toString();
                }
            }

            QFrame* cell = new QFrame(m_gridHost);
            cell->setFixedSize(kDayColW, kRowH);
            cell->setFrameShape(QFrame::NoFrame);
            cell->setCursor(Qt::PointingHandCursor);

            QString cellStyle;
            if (subjectName.isEmpty()) {
                cellStyle = QString(
                    "QFrame { background-color: %1; border: none; border-radius: 8px; }"
                    "QFrame:hover { background-color: %2; }")
                    .arg(p.surface2, p.surface3);
            } else {
                const QColor base = subjectColor(subjectName);
                const QColor hover = base.lighter(112);
                cellStyle = QString(
                    "QFrame { background-color: %1; border: none; border-radius: 8px; }"
                    "QFrame:hover { background-color: %2; }")
                    .arg(base.name(), hover.name());
            }
            cell->setStyleSheet(cellStyle);

            QVBoxLayout* cellLayout = new QVBoxLayout(cell);
            cellLayout->setContentsMargins(6, 4, 6, 4);
            cellLayout->setSpacing(0);

            QLabel* subjectLabel = new QLabel(cell);
            subjectLabel->setAlignment(Qt::AlignCenter);
            subjectLabel->setWordWrap(true);
            if (subjectName.isEmpty()) {
                subjectLabel->setText("—");
                subjectLabel->setStyleSheet(
                    QString("font-size: 12px; color: %1; background: transparent;")
                        .arg(p.textSubtle));
            } else {
                subjectLabel->setText(subjectName);
                subjectLabel->setStyleSheet(QString(
                    "font-size: 11px; font-weight: 700; color: %1; background: transparent;")
                    .arg(subjectTextColor(subjectColor(subjectName)).name()));
            }
            cellLayout->addWidget(subjectLabel);

            cell->setProperty("day", d);
            cell->setProperty("slot", s);
            cell->setProperty("subject", subjectName);
            cell->setProperty("teacherName", m_teacherSchedules[m_currentTeacherIndex].name);

            cell->installEventFilter(this);
            m_scheduleGrid->addWidget(cell, s + 1, d + 1);
        }
    }

    // Ajustar el huésped a la medida exacta de la cuadrícula
    m_scheduleGrid->invalidate();
    m_gridHost->setFixedSize(m_scheduleGrid->sizeHint());

    // Animación de aparición de la cuadrícula
    fadeIn(m_gridHost, 200, 0.0, 1.0);
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

    bool ok = false;
    const QString newSubject = QInputDialog::getItem(
        this,
        "Cambiar Materia",
        QString("Profesor: %1\nDía: %2\nFranja: %3\nMateria actual: %4\n\nSeleccione nueva materia:")
            .arg(teacherName)
            .arg(kDays.value(day, "?"))
            .arg(kSlotShort.value(slot, "?"))
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
    Q_UNUSED(slot);

    const Palette p = current();

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Conflicto de horario");
    msgBox.setText(QString("El profesor <b>%1</b> ya tiene la materia <b>%2</b> "
                           "en otra franja del mismo día (%3).")
                       .arg(teacherName)
                       .arg(newSubject)
                       .arg(kDays.value(day, "?")));
    msgBox.setInformativeText("¿Desea reemplazar la materia actual por la nueva?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStyleSheet(QString(
        "QMessageBox { background-color: %1; }"
        "QLabel { color: %2; font-size: 13px; background: transparent; }"
        "QPushButton { padding: 8px 20px; border-radius: 8px; font-weight: 600; min-width: 96px; }")
        .arg(p.surface, p.text));

    if (auto* btnReplace = qobject_cast<QPushButton*>(msgBox.button(QMessageBox::Yes))) {
        btnReplace->setText("Reemplazar");
        btnReplace->setStyleSheet(QString(
            "background-color: %1; color: white; border: none; border-radius: 8px;"
            "padding: 8px 20px; font-weight: 600; min-width: 96px;")
            .arg(p.danger));
    }
    if (auto* btnCancel = qobject_cast<QPushButton*>(msgBox.button(QMessageBox::No))) {
        btnCancel->setText("Cancelar");
        btnCancel->setStyleSheet(QString(
            "background-color: %1; color: %2; border: 1px solid %3; border-radius: 8px;"
            "padding: 8px 20px; font-weight: 600; min-width: 96px;")
            .arg(p.surface2, p.text, p.border));
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

int TeacherScheduleDialog::countBlocks(const QJsonObject& schedule) const
{
    int n = 0;
    for (auto it = schedule.constBegin(); it != schedule.constEnd(); ++it) {
        const QJsonObject day = it.value().toObject();
        for (auto jt = day.constBegin(); jt != day.constEnd(); ++jt) {
            if (!jt.value().toString().isEmpty()) ++n;
        }
    }
    return n;
}

void TeacherScheduleDialog::updateNavigationButtons()
{
    m_btnPrevious->setEnabled(m_currentTeacherIndex > 0);
    m_btnNext->setEnabled(m_currentTeacherIndex < m_teacherSchedules.size() - 1);
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

void TeacherScheduleDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    if (!m_entranceDone) {
        m_entranceDone = true;
        setWindowOpacity(0.0);
        auto* anim = new QPropertyAnimation(this, "windowOpacity", this);
        anim->setDuration(220);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }
}
