/**
 * @file schedule_grid_dialog.cpp
 * @brief Implementación de la base compartida de los diálogos de horarios.
 */

#include "schedule_grid_dialog.hpp"
#include "schedule_theme.hpp"

#include <QApplication>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QScrollArea>
#include <QScrollBar>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>

using namespace ScheduleTheme;

namespace {

constexpr int kTimeColW = 116;
constexpr int kDayColW  = 150;
constexpr int kHeaderH  = 46;
constexpr int kRowH     = 74;
constexpr int kRecessH  = 36;
constexpr int kDays     = 5;

const char* kDragMime = "application/x-gestor-horario-bloque";

QString elideText(const QString& text, const QFont& font, int width)
{
    if (text.isEmpty()) return text;
    const QFontMetrics fm(font);
    return fm.elidedText(text, Qt::ElideRight, width);
}

} // namespace

ScheduleGridDialog::ScheduleGridDialog(const QJsonObject& rootData, bool showCourse, QWidget* parent)
    : QDialog(parent)
    , m_data(rootData)
    , m_showCourse(showCourse)
{
    setModal(true);
    setMinimumSize(1080, 740);
    resize(1240, 860);
}

// ─────────────────────────────────────────────────────────────────────────────
// Construcción de piezas
// ─────────────────────────────────────────────────────────────────────────────

QWidget* ScheduleGridDialog::buildGridCard(QWidget* parent)
{
    const Palette p = current();

    QFrame* card = new QFrame(parent);
    card->setObjectName("ScheduleCard");
    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(10, 10, 10, 10);
    cardLayout->setSpacing(0);

    // Sin QGraphicsDropShadowEffect: la cuadrícula tiene decenas de widgets que
    // se repintan al pasar el cursor; un efecto aquí forzaría un renderizado
    // fuera de pantalla continuo. La elevación se logra con borde y fondo.
    m_gridScroll = new QScrollArea(card);
    m_gridScroll->setObjectName("GridScroll");
    m_gridScroll->setFrameShape(QFrame::NoFrame);
    m_gridScroll->setWidgetResizable(false);
    m_gridScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_gridScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_gridHost = new QWidget(m_gridScroll);
    m_gridHost->setObjectName("GridHost");
    m_gridHost->setStyleSheet(QString("QWidget#GridHost { background: %1; }").arg(p.border));

    m_scheduleGrid = new QGridLayout(m_gridHost);
    m_scheduleGrid->setSpacing(1);
    m_scheduleGrid->setContentsMargins(0, 0, 0, 0);

    m_gridScroll->setWidget(m_gridHost);
    m_gridScroll->viewport()->setAutoFillBackground(false);
    cardLayout->addWidget(m_gridScroll);
    card->setMinimumWidth(480);
    return card;
}

QWidget* ScheduleGridDialog::buildLegendCard(QWidget* parent)
{
    static const QStringList subjects = {
        "Matemáticas", "Física", "Química", "Biología", "Historia",
        "Geografía", "Literatura", "Inglés", "Educación Física",
        "Arte", "Música", "Tecnología", "Filosofía", "Ciudadanía"
    };

    QFrame* legendCard = new QFrame(parent);
    legendCard->setObjectName("LegendCard");
    applyShadow(legendCard, darkMode(), 22, 6);
    QHBoxLayout* legendLayout = new QHBoxLayout(legendCard);
    legendLayout->setContentsMargins(16, 10, 16, 10);
    legendLayout->setSpacing(16);

    auto* legendTitle = new QLabel("LEYENDA", legendCard);
    legendTitle->setObjectName("LegendTitle");
    legendLayout->addWidget(legendTitle);

    for (const QString& subject : subjects) {
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

        auto* text = new QLabel(subject, itemWidget);
        text->setObjectName("LegendText");
        itemLayout->addWidget(text);

        legendLayout->addWidget(itemWidget);
    }
    legendLayout->addStretch();
    return legendCard;
}

QFrame* ScheduleGridDialog::createBlockCell(int day, int slot, const ScheduleModel::Block& block)
{
    const Palette p = current();

    QFrame* cell = new QFrame(m_gridHost);
    cell->setObjectName("BlockCell");
    cell->setFixedSize(kDayColW, kRowH);
    cell->setFrameShape(QFrame::NoFrame);
    cell->setProperty("day", day);
    cell->setProperty("slot", slot);
    cell->setProperty("hasBlock", !block.isEmpty());
    cell->setAcceptDrops(true);
    cell->installEventFilter(this);

    auto* lay = new QVBoxLayout(cell);
    lay->setContentsMargins(9, 7, 9, 7);
    lay->setSpacing(1);

    if (block.isEmpty()) {
        cell->setCursor(Qt::ArrowCursor);
        cell->setStyleSheet(QString(
            "#BlockCell { background-color:%1; border:1px dashed %2; border-radius:10px; }"
            "#BlockCell:hover { background-color:%3; border-color:%4; }")
            .arg(p.surface2, p.border, p.surface3, p.borderStrong));
        auto* dash = new QLabel("—", cell);
        dash->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        dash->setAlignment(Qt::AlignCenter);
        dash->setStyleSheet(QString("color:%1; font-size:13px; background:transparent;")
                                .arg(p.textSubtle));
        lay->addWidget(dash, 1);
        return cell;
    }

    const QColor base = subjectColor(block.materia);
    const QColor hover = base.lighter(112);
    const QColor textColor = subjectTextColor(base);

    cell->setCursor(Qt::OpenHandCursor);
    cell->setStyleSheet(QString(
        "#BlockCell { background-color:%1; border:none; border-radius:10px; }"
        "#BlockCell:hover { background-color:%2; }")
        .arg(base.name(), hover.name()));

    // Materia
    auto* subject = new QLabel(cell);
    subject->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    subject->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    QFont subjectFont = subject->font();
    subjectFont.setBold(true);
    subjectFont.setPointSize(9);
    subject->setFont(subjectFont);
    subject->setStyleSheet(QString("color:%1; background:transparent;").arg(textColor.name()));
    subject->setText(elideText(block.materia, subjectFont, kDayColW - 18));
    subject->setToolTip(block.materia);
    lay->addWidget(subject);

    // Profesor
    QString line2 = block.profesor;
    QString line3 = block.aula;
    if (m_showCourse && !block.curso.isEmpty()) {
        line3 = line3.isEmpty() ? block.curso : (line3 + "  ·  " + block.curso);
    }

    auto addMeta = [&](const QString& text, int alpha) {
        if (text.isEmpty()) return;
        auto* label = new QLabel(cell);
        label->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        QFont f = label->font();
        f.setPointSize(7);
        label->setFont(f);
        QColor c = textColor;
        c.setAlpha(alpha);
        label->setStyleSheet(QString("color:%1; background:transparent;")
                                 .arg(c.name(QColor::HexArgb)));
        label->setText(elideText(text, f, kDayColW - 18));
        label->setToolTip(text);
        lay->addWidget(label);
    };
    addMeta(line2, 225);   // profesor
    addMeta(line3, 185);   // aula (+ curso)

    lay->addStretch(0);
    return cell;
}

// ─────────────────────────────────────────────────────────────────────────────
// Rejilla
// ─────────────────────────────────────────────────────────────────────────────

void ScheduleGridDialog::rebuildGrid()
{
    const Palette p = current();
    const QStringList days = ScheduleModel::dayNames();
    const QVector<ScheduleModel::RowDef> rows = ScheduleModel::rowDefs();

    // Limpiar cuadrícula anterior
    QLayoutItem* item;
    while ((item = m_scheduleGrid->takeAt(0)) != nullptr) {
        if (item->widget()) { item->widget()->hide(); item->widget()->deleteLater(); }
        delete item;
    }

    const QString headStyle = QString(
        "background-color:%1; color:%2; font-weight:700; font-size:11px;"
        "letter-spacing:0.5px; border:none; border-radius:8px;")
        .arg(p.surface2, p.textSubtle);

    // Esquina + cabeceras
    auto* corner = new QLabel("HORA", m_gridHost);
    corner->setFixedSize(kTimeColW, kHeaderH);
    corner->setAlignment(Qt::AlignCenter);
    corner->setStyleSheet(headStyle);
    m_scheduleGrid->addWidget(corner, 0, 0);

    for (int d = 0; d < kDays && d < days.size(); ++d) {
        auto* day = new QLabel(days[d].toUpper(), m_gridHost);
        day->setFixedSize(kDayColW, kHeaderH);
        day->setAlignment(Qt::AlignCenter);
        day->setStyleSheet(headStyle);
        m_scheduleGrid->addWidget(day, 0, d + 1);
    }

    const QJsonObject schedule = ScheduleModel::scheduleOf(m_data, currentRef());

    int teachingRows = 0;
    int recessRows = 0;
    for (int r = 0; r < rows.size(); ++r) {
        const ScheduleModel::RowDef& rd = rows[r];
        const int gridRow = r + 1;

        if (rd.recess) {
            ++recessRows;
            auto* cornerRecess = new QLabel("R", m_gridHost);
            cornerRecess->setFixedSize(kTimeColW, kRecessH);
            cornerRecess->setAlignment(Qt::AlignCenter);
            cornerRecess->setStyleSheet(QString(
                "background-color:%1; color:%2; font-weight:700; font-size:10px;"
                "letter-spacing:1px; border:none; border-radius:8px;")
                .arg(p.surface2, p.warning));
            m_scheduleGrid->addWidget(cornerRecess, gridRow, 0);

            auto* bar = new QFrame(m_gridHost);
            bar->setObjectName("RecessRow");
            bar->setFixedHeight(kRecessH);
            bar->setStyleSheet(QString(
                "#RecessRow { background-color:%1; border:1px dashed %2; border-radius:8px; }")
                .arg(p.surface2, p.warning));
            auto* barLayout = new QHBoxLayout(bar);
            barLayout->setContentsMargins(0, 0, 0, 0);
            auto* barLabel = new QLabel(QString("RECESO   %1").arg(rd.turno), bar);
            barLabel->setAlignment(Qt::AlignCenter);
            barLabel->setStyleSheet(QString(
                "color:%1; font-size:10px; font-weight:700; letter-spacing:2px; background:transparent;")
                .arg(p.warning));
            barLayout->addWidget(barLabel);
            m_scheduleGrid->addWidget(bar, gridRow, 1, 1, kDays);
            continue;
        }

        ++teachingRows;

        auto* timeLabel = new QLabel(m_gridHost);
        timeLabel->setFixedSize(kTimeColW, kRowH);
        timeLabel->setAlignment(Qt::AlignCenter);
        timeLabel->setStyleSheet(QString(
            "background-color:%1; color:%2; font-weight:600; font-size:11px;"
            "border:none; border-radius:8px;")
            .arg(p.surface2, p.textMuted));
        timeLabel->setText(rd.label);
        timeLabel->setToolTip(rd.turno);
        m_scheduleGrid->addWidget(timeLabel, gridRow, 0);

        for (int d = 0; d < kDays; ++d) {
            const ScheduleModel::Block b =
                ScheduleModel::getBlock(schedule, d + 1, rd.slot);
            m_scheduleGrid->addWidget(createBlockCell(d + 1, rd.slot, b), gridRow, d + 1);
        }
    }

    // Tamaño determinista (ver nota histórica: sizeHint() puede dar 0x0 tras
    // reconstruir porque los widgets nuevos aún no están "visibles").
    const int gap = m_scheduleGrid->spacing();
    const int gridW = kTimeColW + kDays * kDayColW + kDays * gap;
    const int gridH = kHeaderH + teachingRows * kRowH + recessRows * kRecessH
                    + (teachingRows + recessRows) * gap;
    m_gridHost->setFixedSize(gridW, gridH);
    m_scheduleGrid->activate();
}

void ScheduleGridDialog::resetGridScroll()
{
    if (!m_gridScroll) return;
    QTimer::singleShot(0, this, [this]() {
        m_gridScroll->verticalScrollBar()->setValue(0);
        m_gridScroll->horizontalScrollBar()->setValue(0);
    });
}

int ScheduleGridDialog::blockCount() const
{
    const QJsonObject schedule = ScheduleModel::scheduleOf(m_data, currentRef());
    int n = 0;
    for (auto it = schedule.constBegin(); it != schedule.constEnd(); ++it) {
        const QJsonObject day = it.value().toObject();
        for (auto jt = day.constBegin(); jt != day.constEnd(); ++jt) {
            if (!ScheduleModel::parseBlock(jt.value()).isEmpty()) ++n;
        }
    }
    return n;
}

// ─────────────────────────────────────────────────────────────────────────────
// Arrastrar y soltar + conflictos
// ─────────────────────────────────────────────────────────────────────────────

void ScheduleGridDialog::startDrag(QFrame* cell, const QPoint& pos)
{
    if (!cell) return;

    QJsonObject payload;
    payload["day"] = cell->property("day").toInt();
    payload["slot"] = cell->property("slot").toInt();

    auto* mime = new QMimeData();
    mime->setData(kDragMime, QJsonDocument(payload).toJson(QJsonDocument::Compact));

    auto* drag = new QDrag(this);
    drag->setMimeData(mime);
    drag->setPixmap(cell->grab());
    drag->setHotSpot(pos);
    drag->exec(Qt::MoveAction);
}

bool ScheduleGridDialog::eventFilter(QObject* watched, QEvent* event)
{
    QFrame* cell = qobject_cast<QFrame*>(watched);

    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        if (cell && cell->property("hasBlock").toBool()) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                m_pressPos = me->pos();
                m_pressCell = cell;
                m_maybeDrag = true;
            }
        }
        break;
    }
    case QEvent::MouseMove: {
        if (m_maybeDrag && m_pressCell) {
            auto* me = static_cast<QMouseEvent*>(event);
            if ((me->buttons() & Qt::LeftButton)
                && (me->pos() - m_pressPos).manhattanLength() >= QApplication::startDragDistance()) {
                QFrame* source = m_pressCell;
                m_maybeDrag = false;
                m_pressCell = nullptr;
                startDrag(source, me->pos());
                return true;
            }
        }
        break;
    }
    case QEvent::MouseButtonRelease:
        m_maybeDrag = false;
        m_pressCell = nullptr;
        break;
    case QEvent::DragEnter: {
        auto* de = static_cast<QDragEnterEvent*>(event);
        if (de->mimeData()->hasFormat(kDragMime)) {
            de->acceptProposedAction();
            return true;
        }
        break;
    }
    case QEvent::DragMove: {
        auto* de = static_cast<QDragMoveEvent*>(event);
        if (de->mimeData()->hasFormat(kDragMime)) {
            de->acceptProposedAction();
            return true;
        }
        break;
    }
    case QEvent::Drop: {
        auto* de = static_cast<QDropEvent*>(event);
        if (cell && de->mimeData()->hasFormat(kDragMime)) {
            const QJsonObject payload =
                QJsonDocument::fromJson(de->mimeData()->data(kDragMime)).object();
            const int fromDay = payload.value("day").toInt();
            const int fromSlot = payload.value("slot").toInt();
            const int toDay = cell->property("day").toInt();
            const int toSlot = cell->property("slot").toInt();
            moveBlock(fromDay, fromSlot, toDay, toSlot);
            de->acceptProposedAction();
            return true;
        }
        break;
    }
    default:
        break;
    }

    return QDialog::eventFilter(watched, event);
}

void ScheduleGridDialog::moveBlock(int fromDay, int fromSlot, int toDay, int toSlot)
{
    const ScheduleModel::Ref self = currentRef();
    const ScheduleModel::MovePlan plan =
        ScheduleModel::planMove(m_data, self, fromDay, fromSlot, toDay, toSlot);

    if (!plan.valid) return;   // celda origen vacía o mismo bloque

    const QStringList days = ScheduleModel::dayNames();
    const QVector<ScheduleModel::RowDef> rows = ScheduleModel::rowDefs();
    auto slotLabel = [&](int slot) {
        for (const ScheduleModel::RowDef& rd : rows) {
            if (!rd.recess && rd.slot == slot) return rd.label;
        }
        return QString::number(slot);
    };

    if (plan.hasConflict) {
        QStringList details;
        for (const ScheduleModel::Conflict& c : plan.conflicts) {
            details << QString("%1  ·  %2, %3  ·  %4 (%5)")
                           .arg(c.other.name,
                                days.value(c.day - 1, QString::number(c.day)),
                                slotLabel(c.slot),
                                c.block.materia,
                                c.reason);
        }
        const QString intro = QString(
            "Mover \"%1\" a %2, %3. Esa franja ya está ocupada en otro horario; "
            "si continúa se intercambiarán las dos clases en todos los horarios.")
            .arg(plan.moved.materia,
                 days.value(toDay - 1, QString::number(toDay)),
                 slotLabel(toSlot));

        if (!askSwapConflict(this, "Conflicto de horario", intro, details)) {
            return;   // Cancelar
        }
        ScheduleModel::applyMove(m_data, self, plan, /*swapOthers=*/true);
    } else {
        ScheduleModel::applyMove(m_data, self, plan, /*swapOthers=*/false);
    }

    rebuildGrid();
    onScheduleChanged();
    emitModified();
}

// ─────────────────────────────────────────────────────────────────────────────
// Estilos
// ─────────────────────────────────────────────────────────────────────────────

void ScheduleGridDialog::applyStyles()
{
    const Palette p = current();

    QString qss = QString(
        "QDialog { background-color: @BG@; border: none; }"
        "#Header, #GridHost, #NavHost, #LegendItem, #YearNav { background: transparent; }"

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

        "#Pill { background-color: @SURF2@; color: @MUTED@; border: 1px solid @BORDER@;"
        "  border-radius: 10px; padding: 8px 16px; font-size: 13px; font-weight: 600; }"
        "#Pill:hover { background-color: @SURF3@; color: @TEXT@; }"
        "#Pill:checked { color: #FFFFFF; font-weight: 700; border: 1px solid @ACCENTSTRONG@;"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 @ACCENT@, stop:1 @ACCENTSTRONG@); }"

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

void ScheduleGridDialog::showEvent(QShowEvent* event)
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
