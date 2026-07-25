/**
 * @file dashboard_widget.cpp
 * @brief Implementación del widget del panel principal del dashboard
 */

#include "dashboard_widget.hpp"
#include <QPainter>
#include <QProgressBar>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QGridLayout>
#include <QGraphicsDropShadowEffect>

namespace {
// ==========================================
// HERRAMIENTA AUXILIAR: DIÁLOGOS FLOTANTES
// ==========================================
QDialog* crearDialogoFlotante(QWidget* parent, const QString& icono, const QString& titulo, QVBoxLayout*& layoutInterno) {
    QDialog* dialog = new QDialog(parent);
    dialog->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    dialog->setAttribute(Qt::WA_TranslucentBackground);
    dialog->setModal(true);

    QVBoxLayout *mainContainer = new QVBoxLayout(dialog);
    mainContainer->setContentsMargins(10, 10, 10, 10);

    QFrame *bgFrame = new QFrame(dialog);
    bgFrame->setStyleSheet("QFrame { background-color: white; border-radius: 12px; border: 1px solid #cbd5e1; }");

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(dialog);
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 40));
    shadow->setOffset(0, 4);
    bgFrame->setGraphicsEffect(shadow);

    layoutInterno = new QVBoxLayout(bgFrame);
    layoutInterno->setContentsMargins(24, 20, 24, 24);
    layoutInterno->setSpacing(16);

    // Encabezado
    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *headerIcon = new QLabel(icono, bgFrame);
    headerIcon->setStyleSheet("font-size: 20px; background: transparent; border: none;");

    QLabel *headerTitle = new QLabel(titulo, bgFrame);
    headerTitle->setStyleSheet("font-size: 18px; font-weight: 800; color: #0f172a; background: transparent; border: none;");

    QPushButton *closeBtn = new QPushButton("✕", bgFrame);
    closeBtn->setFixedSize(30, 30);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet("QPushButton { font-size: 16px; font-weight: bold; color: #64748b; background: transparent; border: none; } QPushButton:hover { color: #ef4444; }");
    QObject::connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::reject);

    headerLayout->addWidget(headerIcon);
    headerLayout->addWidget(headerTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(closeBtn);
    layoutInterno->addLayout(headerLayout);

    QFrame *hLine = new QFrame(bgFrame);
    hLine->setFrameShape(QFrame::HLine);
    hLine->setStyleSheet("color: #f1f5f9;");
    layoutInterno->addWidget(hLine);

    mainContainer->addWidget(bgFrame);
    return dialog;
}
}

// ==========================================
// WIDGET PARA EL GAUGE SEMICIRCULAR CORREGIDO
// ==========================================
class GaugeWidget : public QWidget
{
public:
    GaugeWidget(int value, QWidget *parent = nullptr) : QWidget(parent), m_value(value) {
        setFixedSize(130, 85);
    }
    void setValue(int value) { m_value = value; update(); }
protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        int side = width() - 20;

        painter.translate(width() / 2.0, 65.0);

        QRectF rect(-side / 2.0, -side / 2.0, side, side);

        // Fondo del arco (gris)
        painter.setPen(QPen(QColor(226, 232, 240), 12, Qt::SolidLine, Qt::FlatCap));
        painter.drawArc(rect, 0 * 16, 180 * 16);

        // Progreso (verde)
        painter.setPen(QPen(QColor(22, 163, 74), 12, Qt::SolidLine, Qt::FlatCap));
        int spanAngle = (int)(m_value / 100.0 * 180.0 * 16.0);
        painter.drawArc(rect, 180 * 16, -spanAngle);

        // Texto centrado perfectamente
        painter.resetTransform();
        painter.setPen(QColor(15, 23, 42));
        QFont font = painter.font();
        font.setPointSize(16);
        font.setBold(true);
        painter.setFont(font);

        painter.drawText(QRect(0, 52, width(), 30), Qt::AlignCenter, QString::number(m_value) + "%");
    }
private:
    int m_value;
};

DashboardWidget::DashboardWidget(QWidget *parent) : QWidget(parent)
{
    this->setStyleSheet("background: transparent;");
    setupUi();
}

DashboardWidget::~DashboardWidget() {}

// ==========================================
// DISTRIBUCIÓN VISUAL PRINCIPAL
// ==========================================
void DashboardWidget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 40, 40, 40);
    mainLayout->setSpacing(24);

    // Título
    QVBoxLayout *titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(4);

    QLabel *title = new QLabel("Panel de control");
    title->setStyleSheet("font-size: 24px; font-weight: 800; color: #0f172a; border: none;");
    QLabel *subtitle = new QLabel("Resumen de la institucion y registro de horarios");
    subtitle->setStyleSheet("font-size: 14px; color: #64748b; border: none;");

    titleLayout->addWidget(title);
    titleLayout->addWidget(subtitle);
    mainLayout->addLayout(titleLayout);

    // Tarjetas de resumen (Fila superior ampliada)
    QHBoxLayout *cardsLayout = new QHBoxLayout();
    cardsLayout->setSpacing(20);
    cardsLayout->addWidget(crearTarjetaResumen("group", "Total Docentes", "124"));
    cardsLayout->addWidget(crearTarjetaResumen("desktop_windows", "Aulas Disponibles", "36"));
    cardsLayout->addWidget(crearTarjetaResumen("book", "Asignaturas Activas", "58"));
    mainLayout->addLayout(cardsLayout);

    // Grid de dos paneles (Fila inferior ampliada)
    QHBoxLayout *gridLayout = new QHBoxLayout();
    gridLayout->setSpacing(20);
    gridLayout->addWidget(crearPanelGeneracion(), 1);
    gridLayout->addWidget(crearPanelNotificaciones(), 1);
    mainLayout->addLayout(gridLayout);

    mainLayout->addStretch();
}

QWidget* DashboardWidget::crearTarjetaResumen(const QString &icono, const QString &label, const QString &valor)
{
    QFrame *card = new QFrame();
    card->setObjectName("DashboardCard");
    card->setStyleSheet("QFrame#DashboardCard { background-color: white; border-radius: 12px; border: 1px solid #e2e8f0; }");
    card->setFixedHeight(130); // Altura aumentada para mayor presencia visual

    QHBoxLayout *layout = new QHBoxLayout(card);
    layout->setContentsMargins(28, 24, 28, 24); // Márgenes internos más amplios
    layout->setSpacing(18);

    QString iconText = (icono == "group") ? "👥" : (icono == "desktop_windows") ? "🖥️" : (icono == "book") ? "📚" : "📌";

    QLabel *iconLabel = new QLabel(iconText);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setFixedSize(54, 54); // Icono sutilmente más grande
    iconLabel->setStyleSheet("background-color: #f8fafc; border: 1px solid #f1f5f9; border-radius: 10px; font-size: 22px;");

    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(3);

    QLabel *labelWidget = new QLabel(label);
    labelWidget->setStyleSheet("font-size: 13px; font-weight: 600; color: #64748b; border: none;");
    QLabel *valueWidget = new QLabel(valor);
    valueWidget->setStyleSheet("font-size: 28px; font-weight: 800; color: #0f172a; border: none;"); // Número protagonista más grande

    infoLayout->addWidget(labelWidget);
    infoLayout->addWidget(valueWidget);
    infoLayout->addStretch();

    layout->addWidget(iconLabel);
    layout->addLayout(infoLayout);
    layout->addStretch();

    return card;
}

QWidget* DashboardWidget::crearPanelGeneracion()
{
    QFrame *panel = new QFrame();
    panel->setObjectName("DashboardCard");
    panel->setStyleSheet("QFrame#DashboardCard { background-color: white; border-radius: 12px; border: 1px solid #e2e8f0; }");
    panel->setFixedHeight(280); // Panel inferior con mayor volumen

    QVBoxLayout *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(28, 26, 28, 26);

    QLabel *title = new QLabel("Última Generación de Horario");
    title->setStyleSheet("font-size: 16px; font-weight: 800; color: #0f172a; border: none;");
    layout->addWidget(title);
    layout->addStretch();

    QHBoxLayout *statusLayout = new QHBoxLayout();
    statusLayout->setSpacing(25);

    QVBoxLayout *gaugeContainer = new QVBoxLayout();
    gaugeContainer->addStretch();
    gaugeContainer->addWidget(new GaugeWidget(85));
    gaugeContainer->addStretch();
    statusLayout->addLayout(gaugeContainer);

    QVBoxLayout *detailsLayout = new QVBoxLayout();
    detailsLayout->setSpacing(8);

    QLabel *estadoLabel = new QLabel("Estado:");
    estadoLabel->setStyleSheet("font-size: 13px; font-weight: 700; color: #0f172a; border: none;");
    QLabel *estadoText = new QLabel("Generado, con ajustes manuales necesarios");
    estadoText->setStyleSheet("font-size: 13px; color: #64748b; border: none;");
    estadoText->setWordWrap(true);

    detailsLayout->addWidget(estadoLabel);
    detailsLayout->addWidget(estadoText);
    detailsLayout->addSpacing(12);

    QHBoxLayout *actionsLayout = new QHBoxLayout();
    actionsLayout->setSpacing(12);

    m_btnVerDetalles = new QPushButton("Ver Detalles");
    m_btnVerDetalles->setStyleSheet("QPushButton { background-color: white; color: #0f172a; border: 1px solid #cbd5e1; padding: 9px 18px; border-radius: 6px; font-weight: 600; font-size: 12px; } QPushButton:hover { background-color: #f8fafc; }");
    m_btnVerDetalles->setCursor(Qt::PointingHandCursor);

    m_btnResolverConflictos = new QPushButton("Resolver Conflictos");
    m_btnResolverConflictos->setStyleSheet("QPushButton { background-color: #0f172a; color: white; border: none; padding: 9px 18px; border-radius: 6px; font-weight: 600; font-size: 12px; } QPushButton:hover { background-color: #1e293b; }");
    m_btnResolverConflictos->setCursor(Qt::PointingHandCursor);

    actionsLayout->addWidget(m_btnVerDetalles);
    actionsLayout->addWidget(m_btnResolverConflictos);
    actionsLayout->addStretch();

    detailsLayout->addLayout(actionsLayout);
    statusLayout->addLayout(detailsLayout);

    layout->addLayout(statusLayout);
    layout->addStretch();

    connect(m_btnVerDetalles, &QPushButton::clicked, this, &DashboardWidget::mostrarDetalles);
    connect(m_btnResolverConflictos, &QPushButton::clicked, this, &DashboardWidget::resolverConflictos);

    return panel;
}

QWidget* DashboardWidget::crearPanelNotificaciones()
{
    QFrame *panel = new QFrame();
    panel->setObjectName("DashboardCard");
    panel->setStyleSheet("QFrame#DashboardCard { background-color: white; border-radius: 12px; border: 1px solid #e2e8f0; }");
    panel->setFixedHeight(280); // Altura unificada con el panel izquierdo

    QVBoxLayout *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(28, 26, 28, 26);
    layout->setSpacing(14);

    QLabel *title = new QLabel("Notificaciones Importantes");
    title->setStyleSheet("font-size: 16px; font-weight: 800; color: #0f172a; border: none;");
    layout->addWidget(title);

    struct Notificacion { int numero; QString texto; bool destacada; };
    QList<Notificacion> notificaciones = {
        {1, "Revisar disponibilidad de la Profa. Castro", true},
        {2, "Asignación de materia \"Matemáticas\" pendiente para aula 3B", false},
        {3, "Asignación de materia \"Matemáticas\" pendiente para aula 3", false}
    };

    for (const auto &n : notificaciones) {
        QFrame *item = new QFrame();
        item->setFixedHeight(50); // Celdas de notificaciones ligeramente más altas y espaciosas
        item->setStyleSheet(n.destacada ? "QFrame { background-color: #f8fafc; border-radius: 8px; border: none; }" : "QFrame { background-color: white; border-radius: 8px; border: 1px solid #e2e8f0; }");

        QHBoxLayout *itemLayout = new QHBoxLayout(item);
        itemLayout->setContentsMargins(16, 0, 16, 0);
        itemLayout->setSpacing(16);

        QLabel *numLabel = new QLabel(QString::number(n.numero));
        numLabel->setFixedSize(26, 26);
        numLabel->setAlignment(Qt::AlignCenter);
        numLabel->setStyleSheet("background-color: #e0e7ff; color: #4338ca; border-radius: 13px; font-weight: 800; font-size: 11px; border: none;");

        QLabel *textLabel = new QLabel(n.texto);
        textLabel->setStyleSheet("font-size: 13px; color: #0f172a; border: none; background: transparent;");
        textLabel->setWordWrap(true);

        itemLayout->addWidget(numLabel);
        itemLayout->addWidget(textLabel);
        layout->addWidget(item);
    }

    layout->addStretch();
    return panel;
}

// ==========================================
// DIÁLOGOS
// ==========================================
void DashboardWidget::mostrarDetalles()
{
    QVBoxLayout* layoutInterno = nullptr;
    QDialog* dialog = crearDialogoFlotante(this, "📊", "Detalles de Generación", layoutInterno);
    dialog->resize(550, 360);

    QGridLayout *grid = new QGridLayout();
    grid->setSpacing(16);

    QStringList labels = {"Progreso Global", "Horas Asignadas", "Aulas Utilizadas", "Conflictos Detectados"};
    QStringList values = {"85%", "1,240 / 1,450", "34 / 36", "3"};
    QStringList styles = {"color: #10b981;", "color: #0f172a;", "color: #0f172a;", "color: #f59e0b;"};

    for (int i = 0; i < 4; ++i) {
        QFrame *card = new QFrame(dialog);
        card->setStyleSheet("QFrame { background-color: #f8fafc; border: 1px solid #e2e8f0; border-radius: 8px; }");
        QVBoxLayout *cardLayout = new QVBoxLayout(card);

        QLabel *label = new QLabel(labels[i], card);
        label->setAlignment(Qt::AlignLeft);
        label->setStyleSheet("font-size: 13px; color: #64748b; font-weight: 600; border: none; background: transparent;");
        QLabel *value = new QLabel(values[i], card);
        value->setAlignment(Qt::AlignLeft);
        value->setStyleSheet("font-size: 20px; font-weight: 800; border: none; background: transparent; " + styles[i]);

        cardLayout->addWidget(label);
        cardLayout->addWidget(value);
        grid->addWidget(card, i / 2, i % 2);
    }
    layoutInterno->addLayout(grid);

    QLabel *desc = new QLabel("El algoritmo genético ha completado la fase 4. Se ha maximizado la compactación del horario de los docentes, sin embargo, existen solapamientos de horas en asignaturas clave que requieren intervención manual para respetar la normativa del plantel.", dialog);
    desc->setWordWrap(true);
    desc->setAlignment(Qt::AlignLeft);
    desc->setStyleSheet("font-size: 13px; color: #64748b; border: none; background: transparent;");
    layoutInterno->addWidget(desc);

    dialog->exec();
}

void DashboardWidget::resolverConflictos()
{
    QVBoxLayout* layoutInterno = nullptr;
    QDialog* dialog = crearDialogoFlotante(this, "⚠️", "Resolver Conflictos", layoutInterno);
    dialog->resize(600, 480);

    QLabel *headerDesc = new QLabel("Se encontraron los siguientes choques en la matriz de disponibilidad:", dialog);
    headerDesc->setStyleSheet("font-size: 14px; color: #334155; border: none; background: transparent;");
    layoutInterno->addWidget(headerDesc);

    struct Conflicto { QString titulo, descripcion, btnTexto; };
    QList<Conflicto> conflictos = {
        {"Choque de Horario: Profa. Castro", "Asignada a Biología (Aula 2) y Química (Aula 4) el Lunes a las 08:00 AM.", "Reasignar"},
        {"Aula 3B sin capacidad", "El grupo de 4to Año tiene 45 alumnos, pero el Aula 3B tiene aforo máximo de 30.", "Cambiar\nAula"},
        {"Disponibilidad Excedida: Prof. Méndez", "Excede en 2 horas su carga horaria semanal máxima permitida (36 hrs).", "Ajustar\nCarga"}
    };

    for (const auto &c : conflictos) {
        QFrame *item = new QFrame(dialog);
        item->setStyleSheet("QFrame { border: 1px solid #fecaca; background-color: #fff1f2; border-radius: 8px; }");

        QHBoxLayout *itemLayout = new QHBoxLayout(item);
        itemLayout->setContentsMargins(16, 16, 16, 16);

        QVBoxLayout *infoLayout = new QVBoxLayout();
        infoLayout->setSpacing(4);

        QLabel *titulo = new QLabel(c.titulo, item);
        titulo->setStyleSheet("font-size: 14px; font-weight: 800; color: #9f1239; border: none; background: transparent;");
        QLabel *desc = new QLabel(c.descripcion, item);
        desc->setStyleSheet("font-size: 13px; color: #9f1239; border: none; background: transparent;");
        desc->setWordWrap(true);

        infoLayout->addWidget(titulo);
        infoLayout->addWidget(desc);

        QPushButton *btn = new QPushButton(c.btnTexto, item);
        btn->setFixedSize(100, 45);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet("QPushButton { background-color: #0f172a; color: white; border: none; border-radius: 6px; font-weight: 800; font-size: 12px; } QPushButton:hover { background-color: #1e293b; }");

        itemLayout->addLayout(infoLayout);
        itemLayout->addWidget(btn, 0, Qt::AlignVCenter);

        layoutInterno->addWidget(item);
    }

    dialog->exec();
}
