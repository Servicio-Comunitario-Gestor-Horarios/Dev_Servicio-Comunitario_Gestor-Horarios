/**
 * @file student_schedule_dialog.cpp
 * @brief Diálogo modal para visualizar y editar horarios de estudiantes
 *        Organizado por años (scroll vertical) y secciones (flechas laterales)
 *        Permite editar asignaturas con detección de conflictos
 */

#include "student_schedule_dialog.hpp"
#include <QGridLayout>
#include <QFrame>
#include <QMessageBox>
#include <QInputDialog>
#include <QScrollBar>
#include <QDebug>
#include <QTimer>
#include <QEvent>

StudentScheduleDialog::StudentScheduleDialog(const QJsonObject& schedulesData, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Horarios de Estudiantes");
    setModal(true);
    setMinimumSize(1000, 700);
    resize(1100, 750);

    setStyleSheet(
        "QDialog { background-color: #fafafa; }"
        "QLabel { color: #1a1a2e; }"
    );

    setupUi(schedulesData);

    // Configurar animación de scroll
    m_scrollAnimation = new QPropertyAnimation(m_scrollArea->verticalScrollBar(), "value", this);
    m_scrollAnimation->setDuration(500);
    m_scrollAnimation->setEasingCurve(QEasingCurve::OutCubic);

    // Mostrar el primer año
    if (!m_years.isEmpty()) {
        showYearSchedule(0);
    }
}

void StudentScheduleDialog::setupUi(const QJsonObject& schedulesData)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Parsear datos de horarios
    QJsonArray yearsArray = schedulesData["years"].toArray();
    for (const auto& yearVal : yearsArray) {
        QJsonObject yearObj = yearVal.toObject();
        YearSchedule ys;
        ys.year = yearObj["year"].toInt();
        ys.name = yearObj["name"].toString();

        QJsonArray sectionsArray = yearObj["sections"].toArray();
        for (const auto& sectionVal : sectionsArray) {
            QJsonObject sectionObj = sectionVal.toObject();
            SectionSchedule ss;
            ss.name = sectionObj["name"].toString();
            ss.schedule = sectionObj["schedule"].toObject();
            ys.sections.append(ss);
        }

        m_years.append(ys);
        m_currentSectionIndices.append(0);
    }

    // Header con navegación de años
    QWidget* headerWidget = new QWidget(this);
    headerWidget->setFixedHeight(100);
    headerWidget->setStyleSheet(
        "background-color: #00695c;"
        "border-bottom: 2px solid #004d40;"
    );
    QVBoxLayout* headerLayout = new QVBoxLayout(headerWidget);
    headerLayout->setContentsMargins(20, 10, 20, 10);
    headerLayout->setSpacing(10);

    // Título
    QLabel* titleLabel = new QLabel("Horarios por Año y Sección", headerWidget);
    titleLabel->setStyleSheet(
        "font-size: 22px;"
        "font-weight: bold;"
        "color: white;"
        "background: transparent;"
    );
    titleLabel->setAlignment(Qt::AlignCenter);
    headerLayout->addWidget(titleLabel);

    // Navegación de años (botones horizontales)
    m_yearNavWidget = new QWidget(headerWidget);
    m_yearNavLayout = new QHBoxLayout(m_yearNavWidget);
    m_yearNavLayout->setAlignment(Qt::AlignCenter);
    m_yearNavLayout->setSpacing(8);
    createYearNavigation();
    headerLayout->addWidget(m_yearNavWidget);

    mainLayout->addWidget(headerWidget);

    // Navegación de secciones (debajo del header)
    m_sectionNavWidget = new QWidget(this);
    m_sectionNavWidget->setFixedHeight(60);
    m_sectionNavWidget->setStyleSheet("background-color: #e0f2f1; border-bottom: 1px solid #b2dfdb;");
    m_sectionNavLayout = new QHBoxLayout(m_sectionNavWidget);
    m_sectionNavLayout->setAlignment(Qt::AlignCenter);
    m_sectionNavLayout->setSpacing(8);
    m_sectionNavLayout->setContentsMargins(20, 5, 20, 5);
    mainLayout->addWidget(m_sectionNavWidget);

    // Área de scroll con el horario
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setStyleSheet(
        "QScrollArea {"
        "   background-color: #fafafa;"
        "   border: none;"
        "}"
        "QScrollBar:vertical {"
        "   background: #f0f0f0;"
        "   width: 8px;"
        "   border-radius: 4px;"
        "}"
        "QScrollBar::handle:vertical {"
        "   background: #bdbdbd;"
        "   border-radius: 4px;"
        "   min-height: 30px;"
        "}"
        "QScrollBar::handle:vertical:hover { background: #9e9e9e; }"
    );

    m_contentWidget = new QWidget(m_scrollArea);
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    m_contentLayout->setSpacing(30);
    m_contentLayout->setContentsMargins(40, 30, 40, 30);

    // Título del horario actual
    m_scheduleTitleLabel = new QLabel(m_contentWidget);
    m_scheduleTitleLabel->setAlignment(Qt::AlignCenter);
    m_scheduleTitleLabel->setStyleSheet(
        "font-size: 20px;"
        "font-weight: bold;"
        "color: #00695c;"
        "margin-bottom: 10px;"
    );
    m_contentLayout->addWidget(m_scheduleTitleLabel);

    // Contenedor del horario (centrado horizontalmente)
    m_scheduleGridContainer = new QWidget(m_contentWidget);
    m_scheduleGridContainer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_scheduleGrid = new QGridLayout(m_scheduleGridContainer);
    m_scheduleGrid->setSpacing(2);
    m_scheduleGrid->setContentsMargins(0, 0, 0, 0);

    m_contentLayout->addWidget(m_scheduleGridContainer, 0, Qt::AlignHCenter);

    // Espaciador inferior
    m_contentLayout->addStretch(1);

    m_scrollArea->setWidget(m_contentWidget);
    mainLayout->addWidget(m_scrollArea);

    // Actualizar navegación de secciones para el primer año
    updateSectionNavigation(0);
}

void StudentScheduleDialog::createYearNavigation()
{
    // Limpiar botones existentes
    for (auto btn : m_yearButtons) {
        btn->deleteLater();
    }
    m_yearButtons.clear();

    for (int i = 0; i < m_years.size(); ++i) {
        QPushButton* btn = new QPushButton(m_years[i].name, m_yearNavWidget);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedSize(100, 40);
        btn->setProperty("yearIndex", i);
        btn->setStyleSheet(
            "QPushButton {"
            "   background-color: rgba(255,255,255,0.15);"
            "   color: white;"
            "   border: 1px solid rgba(255,255,255,0.3);"
            "   border-radius: 8px;"
            "   font-size: 13px;"
            "   font-weight: bold;"
            "}"
            "QPushButton:hover { background-color: rgba(255,255,255,0.25); }"
            "QPushButton:checked {"
            "   background-color: #ff9800;"
            "   border: 1px solid #f57c00;"
            "   color: white;"
            "}"
        );
        btn->setCheckable(true);
        connect(btn, &QPushButton::clicked, this, [this, i]() { onYearChanged(i); });
        m_yearNavLayout->addWidget(btn);
        m_yearButtons.append(btn);
    }
}

void StudentScheduleDialog::updateSectionNavigation(int yearIndex)
{
    // Limpiar botones existentes
    while (QLayoutItem* item = m_sectionNavLayout->takeAt(0)) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    if (yearIndex >= m_years.size()) return;

    const auto& year = m_years[yearIndex];
    QLabel* label = new QLabel("Secciones:", m_sectionNavWidget);
    label->setStyleSheet("font-weight: bold; color: #00695c; background: transparent; font-size: 13px;");
    m_sectionNavLayout->addWidget(label);

    for (int i = 0; i < year.sections.size(); ++i) {
        QPushButton* btn = new QPushButton(year.sections[i].name, m_sectionNavWidget);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setFixedSize(80, 35);
        btn->setProperty("sectionIndex", i);
        btn->setCheckable(true);
        btn->setStyleSheet(
            "QPushButton {"
            "   background-color: white;"
            "   color: #00695c;"
            "   border: 1px solid #b2dfdb;"
            "   border-radius: 6px;"
            "   font-size: 12px;"
            "   font-weight: bold;"
            "}"
            "QPushButton:hover { background-color: #e0f2f1; border-color: #80cbc4; }"
            "QPushButton:checked {"
            "   background-color: #009688;"
            "   color: white;"
            "   border-color: #00796b;"
            "}"
        );
        connect(btn, &QPushButton::clicked, this, [this, yearIndex, i]() { onSectionChanged(yearIndex, i); });
        m_sectionNavLayout->addWidget(btn);
    }
    m_sectionNavLayout->addStretch();
}

void StudentScheduleDialog::showYearSchedule(int yearIndex)
{
    if (yearIndex < 0 || yearIndex >= m_years.size()) return;

    m_currentYearIndex = yearIndex;
    int sectionIndex = m_currentSectionIndices[yearIndex];

    // Actualizar botones de años
    for (int i = 0; i < m_yearButtons.size(); ++i) {
        m_yearButtons[i]->setChecked(i == yearIndex);
    }

    // Actualizar navegación de secciones
    updateSectionNavigation(yearIndex);

    // Marcar sección actual
    if (sectionIndex < m_sectionNavLayout->count()) {
        QLayoutItem* item = m_sectionNavLayout->itemAt(1 + sectionIndex); // +1 por el label "Secciones:"
        if (item && item->widget()) {
            QPushButton* btn = qobject_cast<QPushButton*>(item->widget());
            if (btn) btn->setChecked(true);
        }
    }

    // Mostrar horario
    createScheduleGrid(yearIndex, sectionIndex);

    // Animar scroll
    animateScrollToYear(yearIndex);
}

void StudentScheduleDialog::createScheduleGrid(int yearIndex, int sectionIndex)
{
    if (yearIndex >= m_years.size() || sectionIndex >= m_years[yearIndex].sections.size()) return;

    const auto& section = m_years[yearIndex].sections[sectionIndex];

    m_scheduleTitleLabel->setText(QString("%1 - Sección %2").arg(m_years[yearIndex].name).arg(section.name));

    // Limpiar grid anterior
    QLayoutItem* item;
    while ((item = m_scheduleGrid->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    const QStringList days = {"Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sábado"};

    // ⚠️ 'slots' es macro de Qt. Renombrado a 'timeSlots'.
    const QStringList timeSlots = {
        "07:00-08:00", "08:00-09:00", "09:00-10:00", "10:00-11:00",
        "11:00-12:00", "12:00-13:00", "13:00-14:00", "14:00-15:00",
        "15:00-16:00", "16:00-17:00", "17:00-18:00", "18:00-19:00"
    };

    // Header
    QLabel* cornerLabel = new QLabel("Hora / Día", m_scheduleGridContainer);
    cornerLabel->setFixedSize(120, 40);
    cornerLabel->setAlignment(Qt::AlignCenter);
    cornerLabel->setStyleSheet(
        "background-color: #00695c; color: white; font-weight: bold;"
        "font-size: 12px; border: 1px solid #004d40; border-radius: 4px;"
        );
    m_scheduleGrid->addWidget(cornerLabel, 0, 0);

    for (int d = 0; d < days.size(); ++d) {
        QLabel* dayLabel = new QLabel(days[d], m_scheduleGridContainer);
        dayLabel->setFixedSize(120, 40);
        dayLabel->setAlignment(Qt::AlignCenter);
        dayLabel->setStyleSheet(
            "background-color: #009688; color: white; font-weight: bold;"
            "font-size: 12px; border: 1px solid #00796b; border-radius: 4px;"
            );
        m_scheduleGrid->addWidget(dayLabel, 0, d + 1);
    }

    // Filas
    for (int s = 0; s < timeSlots.size(); ++s) {
        QLabel* timeLabel = new QLabel(timeSlots[s], m_scheduleGridContainer);
        timeLabel->setFixedSize(120, 50);
        timeLabel->setAlignment(Qt::AlignCenter);
        timeLabel->setStyleSheet(
            "background-color: #e0f2f1; color: #00695c; font-weight: bold;"
            "font-size: 11px; border: 1px solid #b2dfdb; border-radius: 4px;"
            );
        m_scheduleGrid->addWidget(timeLabel, s + 1, 0);

        for (int d = 0; d < days.size(); ++d) {
            const QString dayKey = QString::number(d + 1);
            const QString slotKey = QString::number(s);

            QString subjectName;
            if (section.schedule.contains(dayKey)) {
                QJsonObject dayObj = section.schedule[dayKey].toObject();
                if (dayObj.contains(slotKey)) {
                    subjectName = dayObj[slotKey].toString();
                }
            }

            QFrame* cell = new QFrame(m_scheduleGridContainer);
            cell->setFixedSize(120, 50);
            cell->setFrameShape(QFrame::Box);
            cell->setLineWidth(1);
            cell->setCursor(Qt::PointingHandCursor);

            QString cellStyle = "QFrame { border: 1px solid #e0e0e0; border-radius: 4px; background-color: white; }";
            if (!subjectName.isEmpty()) {
                cellStyle += "QFrame { background-color: #e0f2f1; }";
            }
            cell->setStyleSheet(cellStyle);

            QVBoxLayout* cellLayout = new QVBoxLayout(cell);
            cellLayout->setContentsMargins(4, 4, 4, 4);
            cellLayout->setSpacing(2);

            QLabel* subjectLabel = new QLabel(subjectName.isEmpty() ? "—" : subjectName, cell);
            subjectLabel->setAlignment(Qt::AlignCenter);
            subjectLabel->setWordWrap(true);
            subjectLabel->setStyleSheet(
                QString("font-size: 11px; font-weight: %1; color: %2; background: transparent;")
                    .arg(subjectName.isEmpty() ? "normal" : "bold")
                    .arg(subjectName.isEmpty() ? "#9e9e9e" : "#00695c")
                );
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
}

bool StudentScheduleDialog::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QFrame* cell = qobject_cast<QFrame*>(watched);
        if (cell) {
            onCellClicked(cell->property("yearIndex").toInt(), cell->property("sectionIndex").toInt(),
                          cell->property("day").toInt(), cell->property("slot").toInt(),
                          cell->property("subject").toString());
            return true;
        }
    }
    return QDialog::eventFilter(watched, event);
}

void StudentScheduleDialog::onCellClicked(int yearIndex, int sectionIndex, int day, int slot, const QString& currentSubject)
{
    QString sectionName = m_years[yearIndex].sections[sectionIndex].name;

    QStringList availableSubjects = {"Matemáticas", "Física", "Química", "Biología", "Historia",
                                     "Geografía", "Literatura", "Inglés", "Educación Física",
                                     "Arte", "Música", "Tecnología", "Filosofía", "Ciudadanía"};

    availableSubjects.removeAll(currentSubject);

    bool ok;
    QString newSubject = QInputDialog::getItem(this, "Cambiar Materia",
        QString("Año: %1 - Sección: %2\nDía: %3\nFranja: %4\nMateria actual: %5\n\nSeleccione nueva materia:")
            .arg(m_years[yearIndex].name)
            .arg(sectionName)
            .arg(QStringList{"Lunes","Martes","Miércoles","Jueves","Viernes","Sábado"}[day])
            .arg(QStringList{"07-08","08-09","09-10","10-11","11-12","12-13","13-14","14-15","15-16","16-17","17-18","18-19"}[slot])
            .arg(currentSubject.isEmpty() ? "—" : currentSubject),
        availableSubjects, 0, false, &ok);

    if (ok && !newSubject.isEmpty() && newSubject != currentSubject) {
        if (checkConflict(yearIndex, sectionIndex, day, slot, newSubject)) {
            showConflictDialog(yearIndex, sectionIndex, day, slot, currentSubject, newSubject);
        } else {
            applySubjectChange(yearIndex, sectionIndex, day, slot, newSubject);
        }
    }
}

bool StudentScheduleDialog::checkConflict(int yearIndex, int sectionIndex, int day, int slot, const QString& newSubject)
{
    const auto& section = m_years[yearIndex].sections[sectionIndex];
    QString dayKey = QString::number(day + 1);

    if (section.schedule.contains(dayKey)) {
        QJsonObject dayObj = section.schedule[dayKey].toObject();
        for (auto it = dayObj.begin(); it != dayObj.end(); ++it) {
            if (it.key() != QString::number(slot) && it.value().toString() == newSubject) {
                return true; // Conflicto: misma materia en otra franja del mismo día
            }
        }
    }
    return false;
}

void StudentScheduleDialog::showConflictDialog(int yearIndex, int sectionIndex, int day, int slot,
                                               const QString& currentSubject, const QString& newSubject)
{
    Q_UNUSED(currentSubject);

    QString sectionName = m_years[yearIndex].sections[sectionIndex].name;
    const QStringList dayNames = {"Lunes", "Martes", "Miércoles", "Jueves", "Viernes", "Sábado"};

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("⚠️ Conflicto de Horario");
    msgBox.setText(QString("La sección <b>%1</b> ya tiene la materia <b>%2</b> "
                           "en otra franja del mismo día (%3).")
                       .arg(sectionName)
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
        applySubjectChange(yearIndex, sectionIndex, day, slot, newSubject);
    }
}
void StudentScheduleDialog::applySubjectChange(int yearIndex, int sectionIndex, int day, int slot, const QString& newSubject)
{
    // Actualizar datos internos
    auto& section = m_years[yearIndex].sections[sectionIndex];
    QString dayKey = QString::number(day + 1);
    QJsonObject dayObj = section.schedule.value(dayKey).toObject();
    dayObj[QString::number(slot)] = newSubject;
    section.schedule[dayKey] = dayObj;

    // Actualizar UI
    createScheduleGrid(yearIndex, sectionIndex);

    // Emitir señal de modificación
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

void StudentScheduleDialog::animateScrollToYear(int yearIndex)
{
    // Para los horarios de estudiantes, el scroll vertical muestra diferentes años
    // Aquí solo aseguramos que el contenido esté visible
    QTimer::singleShot(50, [this]() {
        m_scrollArea->verticalScrollBar()->setValue(0);
    });
}

void StudentScheduleDialog::onYearChanged(int yearIndex)
{
    showYearSchedule(yearIndex);
}

void StudentScheduleDialog::onSectionChanged(int yearIndex, int sectionIndex)
{
    if (yearIndex == m_currentYearIndex) {
        m_currentSectionIndices[yearIndex] = sectionIndex;
        createScheduleGrid(yearIndex, sectionIndex);
    }
}