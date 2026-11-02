/**
 * @file student_schedule_dialog.cpp
 * @brief Diálogo de horarios de estudiantes: pestañas de año y navegador de
 *        secciones sobre la base compartida ScheduleGridDialog.
 */

#include "student_schedule_dialog.hpp"
#include "schedule_theme.hpp"

#include <QButtonGroup>
#include <QFontMetrics>
#include <QFrame>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

using namespace ScheduleTheme;

StudentScheduleDialog::StudentScheduleDialog(const QJsonObject& schedulesData, QWidget* parent)
    : ScheduleGridDialog(schedulesData, /*showCourse=*/false, parent)
{
    setWindowTitle("Horarios de Estudiantes");
    setupUi();
    if (!m_years.isEmpty()) {
        showYear(0);
    }
}

void StudentScheduleDialog::setupUi()
{
    applyStyles();

    // ── Parsear años/secciones (los horarios se leen de m_data) ───────────
    const QJsonArray yearsArray = m_data.value("years").toArray();
    for (const QJsonValue& yearVal : yearsArray) {
        const QJsonObject yearObj = yearVal.toObject();
        YearInfo info;
        info.year = yearObj.value("year").toInt();
        info.name = yearObj.value("name").toString();
        const QJsonArray sectionsArray = yearObj.value("sections").toArray();
        for (const QJsonValue& secVal : sectionsArray) {
            info.sectionNames.append(secVal.toObject().value("name").toString());
        }
        m_years.append(info);
        m_currentSectionIndices.append(0);
    }

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 22, 24, 22);
    root->setSpacing(18);

    // ── Cabecera ─────────────────────────────────────────────────────────
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

    m_titleLabel = new QLabel("—", header);
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

    // ── Cuerpo: cuadrícula + navegador de secciones ──────────────────────
    QHBoxLayout* body = new QHBoxLayout();
    body->setSpacing(16);
    body->addWidget(buildGridCard(this), 1);
    body->addWidget(buildSectionNavigator(this), 0, Qt::AlignTop);
    root->addLayout(body, 1);

    // ── Leyenda ──────────────────────────────────────────────────────────
    root->addWidget(buildLegendCard(this));

    createYearNavigation();
}

QWidget* StudentScheduleDialog::buildSectionNavigator(QWidget* parent)
{
    QFrame* sectionCard = new QFrame(parent);
    sectionCard->setObjectName("NavCard");
    applyShadow(sectionCard, darkMode());
    sectionCard->setFixedWidth(124);

    QVBoxLayout* sectionLayout = new QVBoxLayout(sectionCard);
    sectionLayout->setContentsMargins(12, 14, 12, 14);
    sectionLayout->setSpacing(10);

    auto* sectionTitle = new QLabel("SECCIONES", sectionCard);
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
    return sectionCard;
}

void StudentScheduleDialog::createYearNavigation()
{
    m_yearGroup = new QButtonGroup(this);
    m_yearGroup->setExclusive(true);

    for (int i = 0; i < m_years.size(); ++i) {
        auto* btn = new QPushButton(m_years[i].name, m_yearNavWidget);
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

        connect(btn, &QPushButton::clicked, this, [this, i]() { showYear(i); });
    }
}

void StudentScheduleDialog::updateSectionNavigation(int yearIndex)
{
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

    const YearInfo& year = m_years[yearIndex];
    for (int i = 0; i < year.sectionNames.size(); ++i) {
        auto* btn = new QPushButton(year.sectionNames[i], m_sectionNavHost);
        btn->setObjectName("SecBtn");
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumHeight(40);
        btn->setToolTip(year.sectionNames[i]);

        m_sectionNavLayout->addWidget(btn);
        m_sectionButtons.append(btn);
        m_sectionGroup->addButton(btn, i);

        connect(btn, &QPushButton::clicked, this, [this, i]() { selectSection(i); });
    }
    m_sectionNavLayout->addStretch();
}

void StudentScheduleDialog::showYear(int yearIndex)
{
    if (yearIndex < 0 || yearIndex >= m_years.size()) return;

    m_currentYearIndex = yearIndex;

    for (int i = 0; i < m_yearButtons.size(); ++i) {
        m_yearButtons[i]->setChecked(i == yearIndex);
    }

    updateSectionNavigation(yearIndex);
    selectSection(m_currentSectionIndices[yearIndex]);
}

void StudentScheduleDialog::selectSection(int sectionIndex)
{
    if (m_currentYearIndex < 0 || m_currentYearIndex >= m_years.size()) return;
    if (sectionIndex < 0 || sectionIndex >= m_years[m_currentYearIndex].sectionNames.size()) return;

    m_currentSectionIndices[m_currentYearIndex] = sectionIndex;

    for (int i = 0; i < m_sectionButtons.size(); ++i) {
        m_sectionButtons[i]->setChecked(i == sectionIndex);
    }

    onScheduleChanged();
    rebuildGrid();
    resetGridScroll();
}

void StudentScheduleDialog::onScheduleChanged()
{
    if (m_currentYearIndex < 0 || m_currentYearIndex >= m_years.size()) return;
    const YearInfo& year = m_years[m_currentYearIndex];
    const int si = m_currentSectionIndices.value(m_currentYearIndex, 0);
    const QString sectionName = year.sectionNames.value(si);

    m_titleLabel->setText(year.name);
    QString subtitulo = QString("Sección %1").arg(sectionName);
    const QString turno = currentTurno();
    if (!turno.isEmpty())
        subtitulo += QString("  ·  Turno %1").arg(turno);
    subtitulo += QString("  ·  %1 clases  ·  %2 horas académicas")
                     .arg(visualBlockCount())
                     .arg(blockCount());
    m_subtitleLabel->setText(subtitulo);
}

ScheduleModel::Ref StudentScheduleDialog::currentRef() const
{
    ScheduleModel::Ref r;
    r.kind = ScheduleModel::OwnerKind::Section;
    r.yearIndex = m_currentYearIndex;
    r.sectionIndex = m_currentSectionIndices.value(m_currentYearIndex, 0);
    if (m_currentYearIndex >= 0 && m_currentYearIndex < m_years.size()) {
        r.name = m_years[m_currentYearIndex].sectionNames.value(r.sectionIndex);
    }
    return r;
}

QString StudentScheduleDialog::displayName() const
{
    if (m_currentYearIndex < 0 || m_currentYearIndex >= m_years.size()) return QString();
    const YearInfo& year = m_years[m_currentYearIndex];
    return QString("%1 · %2").arg(year.name,
                                  year.sectionNames.value(m_currentSectionIndices.value(m_currentYearIndex, 0)));
}
