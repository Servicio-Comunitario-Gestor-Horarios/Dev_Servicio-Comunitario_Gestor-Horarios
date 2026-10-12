/**
 * @file student_schedule_dialog.cpp
 * @brief Diálogo de horarios de estudiantes con la estética del prototipo
 *        SchedulePrototype: pestañas de año tipo "pill", navegador vertical
 *        de secciones, cuadrícula con bloques de materia coloreados y leyenda.
 *
 * Fix de pestañas: se usan QButtonGroup exclusivos (año y sección) para que
 * solo exista una pestaña activa a la vez.
 */

#include "student_schedule_dialog.hpp"
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

const QStringList kLegendSubjects = {
    "Matemáticas", "Física", "Química", "Literatura",
    "Historia", "Inglés", "Educación Física", "Arte"
};
} // namespace

StudentScheduleDialog::StudentScheduleDialog(const QJsonObject& schedulesData, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Horarios de Estudiantes");
    setModal(true);
    setMinimumSize(1060, 720);
    resize(1220, 820);

    setupUi(schedulesData);

    if (!m_years.isEmpty()) {
        showYearSchedule(0);
    }
}

void StudentScheduleDialog::setupUi(const QJsonObject& schedulesData)
{
    buildStyles();

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 22, 24, 22);
    root->setSpacing(18);

    // ---- Parsear datos ----
    const QJsonArray yearsArray = schedulesData.value("years").toArray();
    for (const auto& yearVal : yearsArray) {
        const QJsonObject yearObj = yearVal.toObject();
        YearSchedule ys;
        ys.year = yearObj.value("year").toInt();
        ys.name = yearObj.value("name").toString();

        const QJsonArray sectionsArray = yearObj.value("sections").toArray();
        for (const auto& sectionVal : sectionsArray) {
            const QJsonObject sectionObj = sectionVal.toObject();
            SectionSchedule ss;
            ss.name = sectionObj.value("name").toString();
            ss.schedule = sectionObj.value("schedule").toObject();
            ys.sections.append(ss);
        }

        m_years.append(ys);
        m_currentSectionIndices.append(0);
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

    m_eyebrowLabel = new QLabel("HORARIOS DE ESTUDIANTES", header);
    m_eyebrowLabel->setObjectName("Eyebrow");
    titleBox->addWidget(m_eyebrowLabel);

    m_titleLabel = new QLabel("Vista de cuadrícula por año y sección", header);
    m_titleLabel->setObjectName("Title");
    titleBox->addWidget(m_titleLabel);

    m_subtitleLabel = new QLabel(header);
    m_subtitleLabel->setObjectName("Subtitle");
    titleBox->addWidget(m_subtitleLabel);

    headerLayout->addLayout(titleBox);
    headerLayout->addStretch();

    // Pestañas de año
    m_yearNavWidget = new QWidget(header);
    m_yearNavWidget->setObjectName("YearNav");
    m_yearNavLayout = new QHBoxLayout(m_yearNavWidget);
    m_yearNavLayout->setContentsMargins(0, 0, 0, 0);
    m_yearNavLayout->setSpacing(8);
    headerLayout->addWidget(m_yearNavWidget, 0, Qt::AlignBottom);

    root->addWidget(header);

    // =========================================================
    // CUERPO: cuadrícula + navegador de secciones
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

    // --- Tarjeta navegador de secciones ---
    QFrame* sectionCard = new QFrame(this);
    sectionCard->setObjectName("NavCard");
    applyShadow(sectionCard, darkMode());
    sectionCard->setFixedWidth(120);
    QVBoxLayout* sectionLayout = new QVBoxLayout(sectionCard);
    sectionLayout->setContentsMargins(12, 14, 12, 14);
    sectionLayout->setSpacing(10);

    QLabel* sectionTitle = new QLabel("SECCIONES", sectionCard);
    sectionTitle->setObjectName("NavTitle");
    sectionLayout->addWidget(sectionTitle);

    QScrollArea* sectionScroll = new QScrollArea(sectionCard);
    sectionScroll->setObjectName("NavScroll");
    sectionScroll->setWidgetResizable(true);
    sectionScroll->setFrameShape(QFrame::NoFrame);
    sectionScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_sectionNavHost = new QWidget(sectionScroll);
    m_sectionNavHost->setObjectName("NavHost");
    m_sectionNavLayout = new QVBoxLayout(m_sectionNavHost);
    m_sectionNavLayout->setContentsMargins(0, 0, 0, 0);
    m_sectionNavLayout->setSpacing(6);

    m_sectionGroup = new QButtonGroup(this);
    m_sectionGroup->setExclusive(true);

    sectionScroll->setWidget(m_sectionNavHost);
    sectionScroll->viewport()->setAutoFillBackground(false);
    sectionLayout->addWidget(sectionScroll, 1);
    body->addWidget(sectionCard, 0, Qt::AlignTop);

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

    // Pestañas de año (una vez)
    createYearNavigation();
}

void StudentScheduleDialog::buildStyles()
{
    const Palette p = current();

    QString qss = QString(
        "QDialog { background-color: @BG@; border: none; }"
        "#Header, #GridHost, #NavHost, #LegendItem, #YearNav { background: transparent; }"

        "#Eyebrow { color: @ACCENT@; font-size: 11px; font-weight: 700; letter-spacing: 2px; background: transparent; border: none; }"
        "#Title { color: @TEXT@; font-size: 24px; font-weight: 800; background: transparent; border: none; }"
        "#Subtitle { color: @MUTED@; font-size: 13px; background: transparent; border: none; }"

        "#ScheduleCard, #NavCard, #LegendCard { background-color: @SURFACE@; border: 1px solid @BORDER@; border-radius: 18px; }"
        "#NavTitle, #LegendTitle { color: @SUBTLE@; font-size: 10px; font-weight: 700; letter-spacing: 1.5px; background: transparent; border: none; }"
        "#LegendText { color: @MUTED@; font-size: 12px; background: transparent; border: none; }"

        // Pestañas de año (horizontal, centradas)
        "#Pill { background-color: @SURF2@; color: @MUTED@; border: 1px solid @BORDER@;"
        "  border-radius: 10px; padding: 8px 16px; font-size: 13px; font-weight: 600; }"
        "#Pill:hover { background-color: @SURF3@; color: @TEXT@; }"
        "#Pill:checked { color: #FFFFFF; font-weight: 700; border: 1px solid @ACCENTSTRONG@;"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 @ACCENT@, stop:1 @ACCENTSTRONG@); }"

        // Secciones (vertical, centradas)
        "#SecBtn { background: transparent; color: @MUTED@; border: 1px solid transparent;"
        "  border-radius: 10px; padding: 9px 12px; font-size: 13px; font-weight: 600; }"
        "#SecBtn:hover { background-color: @SURF2@; color: @TEXT@; }"
        "#SecBtn:checked { color: #FFFFFF; font-weight: 700; border: 1px solid @ACCENTSTRONG@;"
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

void StudentScheduleDialog::createYearNavigation()
{
    m_yearGroup = new QButtonGroup(this);
    m_yearGroup->setExclusive(true);

    for (int i = 0; i < m_years.size(); ++i) {
        QPushButton* btn = new QPushButton(m_years[i].name, m_yearNavWidget);
        btn->setObjectName("Pill");
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumHeight(40);
        btn->setToolTip(m_years[i].name);

        const QFontMetrics fm(btn->font());
        btn->setText(fm.elidedText(m_years[i].name, Qt::ElideRight, 120));

        m_yearNavLayout->addWidget(btn);
        m_yearButtons.append(btn);
        m_yearGroup->addButton(btn, i);

        connect(btn, &QPushButton::clicked, this, [this, i]() {
            showYearSchedule(i);
        });
    }
}

void StudentScheduleDialog::updateSectionNavigation(int yearIndex)
{
    // Limpiar botones anteriores
    for (QPushButton* btn : m_sectionButtons) {
        m_sectionGroup->removeButton(btn);
        btn->hide();
        btn->deleteLater();
    }
    m_sectionButtons.clear();

    while (QLayoutItem* item = m_sectionNavLayout->takeAt(0)) {
        if (item->widget()) { item->widget()->hide(); item->widget()->deleteLater(); }
        delete item;
    }

    if (yearIndex < 0 || yearIndex >= m_years.size()) return;

    const YearSchedule& year = m_years[yearIndex];
    for (int i = 0; i < year.sections.size(); ++i) {
        QPushButton* btn = new QPushButton(year.sections[i].name, m_sectionNavHost);
        btn->setObjectName("SecBtn");
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumHeight(40);
        btn->setToolTip(year.sections[i].name);

        m_sectionNavLayout->addWidget(btn);
        m_sectionButtons.append(btn);
        m_sectionGroup->addButton(btn, i);

        connect(btn, &QPushButton::clicked, this, [this, i]() {
            selectSection(i);
        });
    }
    m_sectionNavLayout->addStretch();
}

void StudentScheduleDialog::showYearSchedule(int yearIndex)
{
    if (yearIndex < 0 || yearIndex >= m_years.size()) return;

    m_currentYearIndex = yearIndex;

    // Solo una pestaña de año activa
    for (int i = 0; i < m_yearButtons.size(); ++i) {
        m_yearButtons[i]->setChecked(i == yearIndex);
    }

    // Reconstruir secciones del año y seleccionar la recordada
    updateSectionNavigation(yearIndex);
    selectSection(m_currentSectionIndices[yearIndex]);
}

void StudentScheduleDialog::selectSection(int sectionIndex)
{
    if (m_currentYearIndex < 0 || m_currentYearIndex >= m_years.size()) return;
    const YearSchedule& year = m_years[m_currentYearIndex];
    if (sectionIndex < 0 || sectionIndex >= year.sections.size()) return;

    m_currentSectionIndices[m_currentYearIndex] = sectionIndex;

    // Solo una sección activa
    for (int i = 0; i < m_sectionButtons.size(); ++i) {
        m_sectionButtons[i]->setChecked(i == sectionIndex);
    }

    const QJsonObject& schedule = year.sections[sectionIndex].schedule;
    m_subtitleLabel->setText(QString("%1  ·  Sección %2  ·  %3 bloques asignados")
                                 .arg(year.name)
                                 .arg(year.sections[sectionIndex].name)
                                 .arg(countBlocks(schedule)));

    createScheduleGrid(m_currentYearIndex, sectionIndex);

    QTimer::singleShot(0, this, [this]() {
        m_gridScroll->verticalScrollBar()->setValue(0);
        m_gridScroll->horizontalScrollBar()->setValue(0);
    });
}

void StudentScheduleDialog::createScheduleGrid(int yearIndex, int sectionIndex)
{
    if (yearIndex < 0 || yearIndex >= m_years.size()) return;
    if (sectionIndex < 0 || sectionIndex >= m_years[yearIndex].sections.size()) return;

    const Palette p = current();
    const SectionSchedule& section = m_years[yearIndex].sections[sectionIndex];

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
            if (section.schedule.contains(dayKey)) {
                const QJsonObject dayObj = section.schedule.value(dayKey).toObject();
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

            cell->setProperty("yearIndex", yearIndex);
            cell->setProperty("sectionIndex", sectionIndex);
            cell->setProperty("day", d);
            cell->setProperty("slot", s);
            cell->setProperty("subject", subjectName);
            cell->setProperty("sectionName", section.name);

            cell->installEventFilter(this);
            m_scheduleGrid->addWidget(cell, s + 1, d + 1);
        }
    }

    m_scheduleGrid->invalidate();
    m_gridHost->setFixedSize(m_scheduleGrid->sizeHint());

    fadeIn(m_gridHost, 200, 0.0, 1.0);
}

bool StudentScheduleDialog::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QFrame* cell = qobject_cast<QFrame*>(watched);
        if (cell) {
            onCellClicked(cell->property("yearIndex").toInt(),
                          cell->property("sectionIndex").toInt(),
                          cell->property("day").toInt(),
                          cell->property("slot").toInt(),
                          cell->property("subject").toString());
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void StudentScheduleDialog::onCellClicked(int yearIndex, int sectionIndex, int day, int slot,
                                          const QString& currentSubject)
{
    if (yearIndex < 0 || yearIndex >= m_years.size()) return;
    if (sectionIndex < 0 || sectionIndex >= m_years[yearIndex].sections.size()) return;

    const QString sectionName = m_years[yearIndex].sections[sectionIndex].name;

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
        QString("Año: %1  ·  Sección: %2\nDía: %3\nFranja: %4\nMateria actual: %5\n\nSeleccione nueva materia:")
            .arg(m_years[yearIndex].name)
            .arg(sectionName)
            .arg(kDays.value(day, "?"))
            .arg(kSlotShort.value(slot, "?"))
            .arg(currentSubject.isEmpty() ? "—" : currentSubject),
        availableSubjects, 0, false, &ok);

    if (!ok || newSubject.isEmpty() || newSubject == currentSubject) return;

    if (checkConflict(yearIndex, sectionIndex, day, slot, newSubject)) {
        showConflictDialog(yearIndex, sectionIndex, day, slot, currentSubject, newSubject);
    } else {
        applySubjectChange(yearIndex, sectionIndex, day, slot, newSubject);
    }
}

bool StudentScheduleDialog::checkConflict(int yearIndex, int sectionIndex, int day, int slot,
                                          const QString& newSubject)
{
    if (yearIndex < 0 || yearIndex >= m_years.size()) return false;
    if (sectionIndex < 0 || sectionIndex >= m_years[yearIndex].sections.size()) return false;

    const SectionSchedule& section = m_years[yearIndex].sections[sectionIndex];
    const QString dayKey = QString::number(day + 1);

    if (section.schedule.contains(dayKey)) {
        const QJsonObject dayObj = section.schedule.value(dayKey).toObject();
        for (auto it = dayObj.constBegin(); it != dayObj.constEnd(); ++it) {
            if (it.key() != QString::number(slot) && it.value().toString() == newSubject) {
                return true;
            }
        }
    }
    return false;
}

void StudentScheduleDialog::showConflictDialog(int yearIndex, int sectionIndex, int day, int slot,
                                               const QString& currentSubject,
                                               const QString& newSubject)
{
    Q_UNUSED(currentSubject);
    Q_UNUSED(slot);

    const QString sectionName = m_years[yearIndex].sections[sectionIndex].name;
    const Palette p = current();

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Conflicto de horario");
    msgBox.setText(QString("La sección <b>%1</b> ya tiene la materia <b>%2</b> "
                           "en otra franja del mismo día (%3).")
                       .arg(sectionName)
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
        applySubjectChange(yearIndex, sectionIndex, day, slot, newSubject);
    }
}

void StudentScheduleDialog::applySubjectChange(int yearIndex, int sectionIndex, int day, int slot,
                                               const QString& newSubject)
{
    if (yearIndex < 0 || yearIndex >= m_years.size()) return;
    if (sectionIndex < 0 || sectionIndex >= m_years[yearIndex].sections.size()) return;

    SectionSchedule& section = m_years[yearIndex].sections[sectionIndex];
    const QString dayKey = QString::number(day + 1);
    QJsonObject dayObj = section.schedule.value(dayKey).toObject();
    dayObj[QString::number(slot)] = newSubject;
    section.schedule[dayKey] = dayObj;

    // Refrescar cuadrícula y subtítulo
    createScheduleGrid(yearIndex, sectionIndex);
    m_subtitleLabel->setText(QString("%1  ·  Sección %2  ·  %3 bloques asignados")
                                 .arg(m_years[yearIndex].name)
                                 .arg(section.name)
                                 .arg(countBlocks(section.schedule)));

    QJsonObject updatedData;
    QJsonArray yearsArray;
    for (const auto& year : m_years) {
        QJsonObject yObj;
        yObj["year"] = year.year;
        yObj["name"] = year.name;
        QJsonArray sectionsArray;
        for (const auto& sec : year.sections) {
            QJsonObject sObj;
            sObj["name"] = sec.name;
            sObj["schedule"] = sec.schedule;
            sectionsArray.append(sObj);
        }
        yObj["sections"] = sectionsArray;
        yearsArray.append(yObj);
    }
    updatedData["years"] = yearsArray;
    emit scheduleModified(updatedData);
}

int StudentScheduleDialog::countBlocks(const QJsonObject& schedule) const
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

void StudentScheduleDialog::showEvent(QShowEvent* event)
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
