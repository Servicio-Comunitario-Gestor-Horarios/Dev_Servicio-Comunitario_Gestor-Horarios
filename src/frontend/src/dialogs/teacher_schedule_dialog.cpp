/**
 * @file teacher_schedule_dialog.cpp
 * @brief Diálogo de horarios de profesores: navegador vertical de docentes
 *        sobre la base compartida ScheduleGridDialog.
 */

#include "teacher_schedule_dialog.hpp"
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

TeacherScheduleDialog::TeacherScheduleDialog(const QJsonObject& schedulesData, QWidget* parent)
    : ScheduleGridDialog(schedulesData, /*showCourse=*/true, parent)
{
    setWindowTitle("Horarios de Profesores");
    setupUi();
    if (!m_teacherNames.isEmpty()) {
        showTeacher(0);
    }
}

void TeacherScheduleDialog::setupUi()
{
    applyStyles();

    // Nombres de docentes (los horarios se leen de m_data vía currentRef()).
    const QJsonArray teachers = m_data.value("teachers").toArray();
    for (const QJsonValue& v : teachers) {
        m_teacherNames.append(v.toObject().value("name").toString());
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
    connect(m_btnPrevious, &QPushButton::clicked, this, [this]() {
        if (m_currentIndex > 0) showTeacher(m_currentIndex - 1);
    });
    headerLayout->addWidget(m_btnPrevious, 0, Qt::AlignBottom);

    m_btnNext = new QPushButton("Siguiente  →", header);
    m_btnNext->setObjectName("GhostBtn");
    m_btnNext->setCursor(Qt::PointingHandCursor);
    m_btnNext->setMinimumHeight(40);
    connect(m_btnNext, &QPushButton::clicked, this, [this]() {
        if (m_currentIndex < m_teacherNames.size() - 1) showTeacher(m_currentIndex + 1);
    });
    headerLayout->addWidget(m_btnNext, 0, Qt::AlignBottom);

    root->addWidget(header);

    // ── Cuerpo: cuadrícula + navegador ───────────────────────────────────
    QHBoxLayout* body = new QHBoxLayout();
    body->setSpacing(16);
    body->addWidget(buildGridCard(this), 1);
    body->addWidget(buildNavigator(this), 0, Qt::AlignTop);
    root->addLayout(body, 1);

    // ── Leyenda ──────────────────────────────────────────────────────────
    root->addWidget(buildLegendCard(this));

    updateNavButtons();
}

QWidget* TeacherScheduleDialog::buildNavigator(QWidget* parent)
{
    QFrame* navCard = new QFrame(parent);
    navCard->setObjectName("NavCard");
    applyShadow(navCard, darkMode());
    navCard->setFixedWidth(238);

    QVBoxLayout* navLayout = new QVBoxLayout(navCard);
    navLayout->setContentsMargins(12, 14, 12, 14);
    navLayout->setSpacing(10);

    auto* navTitle = new QLabel("DOCENTES", navCard);
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

    for (int i = 0; i < m_teacherNames.size(); ++i) {
        auto* btn = new QPushButton(navHost);
        btn->setObjectName("NavBtn");
        btn->setCheckable(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setMinimumHeight(40);
        btn->setToolTip(m_teacherNames[i]);
        const QFontMetrics fm(btn->font());
        btn->setText(fm.elidedText(m_teacherNames[i], Qt::ElideRight, 176));

        navHostLayout->addWidget(btn);
        m_navButtons.append(btn);
        m_navGroup->addButton(btn, i);

        connect(btn, &QPushButton::clicked, this, [this, i]() { showTeacher(i); });
    }
    navHostLayout->addStretch();

    navScroll->setWidget(navHost);
    navScroll->viewport()->setAutoFillBackground(false);
    navLayout->addWidget(navScroll, 1);
    return navCard;
}

void TeacherScheduleDialog::showTeacher(int index)
{
    if (index < 0 || index >= m_teacherNames.size()) return;

    m_currentIndex = index;

    for (int i = 0; i < m_navButtons.size(); ++i) {
        m_navButtons[i]->setChecked(i == index);
    }

    onScheduleChanged();
    rebuildGrid();
    updateNavButtons();
    resetGridScroll();
}

void TeacherScheduleDialog::onScheduleChanged()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_teacherNames.size()) return;
    m_titleLabel->setText(m_teacherNames[m_currentIndex]);
    m_subtitleLabel->setText(QString("Docente %1 de %2  ·  %3 bloques asignados")
                                 .arg(m_currentIndex + 1)
                                 .arg(m_teacherNames.size())
                                 .arg(blockCount()));
}

ScheduleModel::Ref TeacherScheduleDialog::currentRef() const
{
    ScheduleModel::Ref r;
    r.kind = ScheduleModel::OwnerKind::Teacher;
    r.teacherIndex = m_currentIndex;
    r.name = m_teacherNames.value(m_currentIndex);
    return r;
}

QString TeacherScheduleDialog::displayName() const
{
    return m_teacherNames.value(m_currentIndex);
}

void TeacherScheduleDialog::updateNavButtons()
{
    if (m_btnPrevious) m_btnPrevious->setEnabled(m_currentIndex > 0);
    if (m_btnNext)     m_btnNext->setEnabled(m_currentIndex < m_teacherNames.size() - 1);
}
