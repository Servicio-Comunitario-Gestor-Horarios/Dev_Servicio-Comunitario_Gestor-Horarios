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
#include <QApplication>

namespace {
// ==========================================
// HERRAMIENTA AUXILIAR: DIÁLOGOS FLOTANTES
// ==========================================
QDialog* crearDialogoFlotante(QWidget* parent, const QString& icono, const QString& titulo, QVBoxLayout*& layoutInterno) {
    QDialog* dialog = new QDialog(parent);
    dialog->setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    dialog->setAttribute(Qt::WA_TranslucentBackground);
    dialog->setModal(true);

    bool isDark = qApp->property("isDarkMode").toBool();

    QVBoxLayout *mainContainer = new QVBoxLayout(dialog);
    mainContainer->setContentsMargins(10, 10, 10, 10);

    QFrame *bgFrame = new QFrame(dialog);
    QString bgStyle = isDark ?
                          "QFrame { background-color: #1e1e1e; border-radius: 12px; border: 1px solid #333333; color: white; }" :
                          "QFrame { background-color: white; border-radius: 12px; border: 1px solid #cbd5e1; }";
    bgFrame->setStyleSheet(bgStyle);

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
    QString titleColor = isDark ? "#ffffff" : "#0f172a";
    headerTitle->setStyleSheet(QString("font-size: 18px; font-weight: 800; color: %1; background: transparent; border: none;").arg(titleColor));

    QPushButton *closeBtn = new QPushButton("✕", bgFrame);
    closeBtn->setFixedSize(30, 30);
    closeBtn->setCursor(Qt::PointingHandCursor);
    QString closeBtnStyle = isDark ?
                                "QPushButton { font-size: 16px; font-weight: bold; color: #aaaaaa; background: transparent; border: none; } QPushButton:hover { color: #ef4444; }" :
                                "QPushButton { font-size: 16px; font-weight: bold; color: #64748b; background: transparent; border: none; } QPushButton:hover { color: #ef4444; }";
    closeBtn->setStyleSheet(closeBtnStyle);
    QObject::connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::reject);

    headerLayout->addWidget(headerIcon);
    headerLayout->addWidget(headerTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(closeBtn);
    layoutInterno->addLayout(headerLayout);

    QFrame *hLine = new QFrame(bgFrame);
    hLine->setFrameShape(QFrame::HLine);
    hLine->setStyleSheet(isDark ? "color: #333333; background-color: #333333; max-height: 1px;" : "color: #f1f5f9;");
    layoutInterno->addWidget(hLine);

    mainContainer->addWidget(bgFrame);
    return dialog;
}
}

// ==========================================
// WIDGET PARA EL GAUGE SEMICIRCULAR DINÁMICO
// ==========================================
class GaugeWidget : public QWidget
{
public:
    GaugeWidget(int value, QWidget *parent = nullptr) : QWidget(parent), m_value(value), m_isDark(false) {
        setFixedSize(130, 85);
    }
    void setValue(int value) { m_value = value; update(); }
    void setDarkMode(bool dark) { m_isDark = dark; update(); }

protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        int side = width() - 20;
        painter.translate(width() / 2.0, 65.0);
        QRectF rect(-side / 2.0, -side / 2.0, side, side);

        // Fondo del arco adaptable
        QColor arcBg = m_isDark ? QColor(51, 65, 85) : QColor(226, 232, 240);
        painter.setPen(QPen(arcBg, 12, Qt::SolidLine, Qt::FlatCap));
        painter.drawArc(rect, 0 * 16, 180 * 16);

        // Progreso (verde)
        painter.setPen(QPen(QColor(22, 163, 74), 12, Qt::SolidLine, Qt::FlatCap));
        int spanAngle = (int)(m_value / 100.0 * 180.0 * 16.0);
        painter.drawArc(rect, 180 * 16, -spanAngle);

        // Texto centrado perfectamente
        painter.resetTransform();
        painter.setPen(m_isDark ? Qt::white : QColor(15, 23, 42));
        QFont font = painter.font();
        font.setPointSize(16);
        font.setBold(true);
        painter.setFont(font);

        painter.drawText(QRect(0, 52, width(), 30), Qt::AlignCenter, QString::number(m_value) + "%");
    }
private:
    int m_value;
    bool m_isDark;
};

DashboardWidget::DashboardWidget(QWidget *parent) : QWidget(parent), m_gaugeWidget(nullptr)
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
    title->setObjectName("DashboardTitle");
    title->setStyleSheet("font-size: 24px; font-weight: 800; color: #0f172a; border: none;");

    QLabel *subtitle = new QLabel("Resumen de la institucion y registro de horarios");
    subtitle->setObjectName("DashboardSubtitle");
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
    card->setFixedHeight(130);

    QHBoxLayout *layout = new QHBoxLayout(card);
    layout->setContentsMargins(28, 24, 28, 24);
    layout->setSpacing(18);

    QString iconText = (icono == "group") ? "👥" : (icono == "desktop_windows") ? "🖥️" : (icono == "book") ? "📚" : "📌";

    QLabel *iconLabel = new QLabel(iconText);
    iconLabel->setObjectName("CardIcon");
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setFixedSize(54, 54);
    iconLabel->setStyleSheet("background-color: #f8fafc; border: 1px solid #f1f5f9; border-radius: 10px; font-size: 22px;");

    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(3);

    QLabel *labelWidget = new QLabel(label);
    labelWidget->setObjectName("CardLabel");
    labelWidget->setStyleSheet("font-size: 13px; font-weight: 600; color: #64748b; border: none;");

    QLabel *valueWidget = new QLabel(valor);
    valueWidget->setObjectName("CardValue");
    valueWidget->setStyleSheet("font-size: 28px; font-weight: 800; color: #0f172a; border: none;");

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
    panel->setFixedHeight(280);

    QVBoxLayout *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(28, 26, 28, 26);

    QLabel *title = new QLabel("Última Generación de Horario");
    title->setObjectName("PanelTitle");
    title->setStyleSheet("font-size: 16px; font-weight: 800; color: #0f172a; border: none;");
    layout->addWidget(title);
    layout->addStretch();

    QHBoxLayout *statusLayout = new QHBoxLayout();
    statusLayout->setSpacing(25);

    QVBoxLayout *gaugeContainer = new QVBoxLayout();
    gaugeContainer->addStretch();
    m_gaugeWidget = new GaugeWidget(85);
    gaugeContainer->addWidget(m_gaugeWidget);
    gaugeContainer->addStretch();
    statusLayout->addLayout(gaugeContainer);

    QVBoxLayout *detailsLayout = new QVBoxLayout();
    detailsLayout->setSpacing(8);

    QLabel *estadoLabel = new QLabel("Estado:");
    estadoLabel->setObjectName("StateTitle");
    estadoLabel->setStyleSheet("font-size: 13px; font-weight: 700; color: #0f172a; border: none;");

    QLabel *estadoText = new QLabel("Generado, con ajustes manuales necesarios");
    estadoText->setObjectName("StateDesc");
    estadoText->setStyleSheet("font-size: 13px; color: #64748b; border: none;");
    estadoText->setWordWrap(true);

    detailsLayout->addWidget(estadoLabel);
    detailsLayout->addWidget(estadoText);
    detailsLayout->addSpacing(12);

    QHBoxLayout *actionsLayout = new QHBoxLayout();
    actionsLayout->setSpacing(12);

    m_btnVerDetalles = new QPushButton("Ver Detalles");
    m_btnVerDetalles->setObjectName("BtnVerDetalles");
    m_btnVerDetalles->setStyleSheet("QPushButton { background-color: white; color: #0f172a; border: 1px solid #cbd5e1; padding: 9px 18px; border-radius: 6px; font-weight: 600; font-size: 12px; } QPushButton:hover { background-color: #f8fafc; }");
    m_btnVerDetalles->setCursor(Qt::PointingHandCursor);

    m_btnResolverConflictos = new QPushButton("Resolver Conflictos");
    m_btnResolverConflictos->setObjectName("BtnResolverConflictos");
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
    panel->setFixedHeight(280);

    QVBoxLayout *layout = new QVBoxLayout(panel);
    layout->setContentsMargins(28, 26, 28, 26);
    layout->setSpacing(14);

    QLabel *title = new QLabel("Notificaciones Importantes");
    title->setObjectName("PanelTitle");
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
        item->setObjectName("NotifItem");
        item->setFixedHeight(50);
        item->setStyleSheet(n.destacada ? "QFrame { background-color: #f8fafc; border-radius: 8px; border: none; }" : "QFrame { background-color: white; border-radius: 8px; border: 1px solid #e2e8f0; }");

        QHBoxLayout *itemLayout = new QHBoxLayout(item);
        itemLayout->setContentsMargins(16, 0, 16, 0);
        itemLayout->setSpacing(16);

        QLabel *numLabel = new QLabel(QString::number(n.numero));
        numLabel->setObjectName("NotifNum");
        numLabel->setFixedSize(26, 26);
        numLabel->setAlignment(Qt::AlignCenter);
        numLabel->setStyleSheet("background-color: #e0e7ff; color: #4338ca; border-radius: 13px; font-weight: 800; font-size: 11px; border: none;");

        QLabel *textLabel = new QLabel(n.texto);
        textLabel->setObjectName("NotifText");
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
// MÉTODO PARA ACTUALIZAR EL TEMA EN TIEMPO REAL
// ==========================================
void DashboardWidget::actualizarTema(bool modoOscuro) {
    if (m_gaugeWidget) {
        m_gaugeWidget->setDarkMode(modoOscuro);
    }

    if (modoOscuro) {
        if (auto lbl = findChild<QLabel*>("DashboardTitle")) lbl->setStyleSheet("font-size: 24px; font-weight: 800; color: #ffffff; border: none;");
        if (auto lbl = findChild<QLabel*>("DashboardSubtitle")) lbl->setStyleSheet("font-size: 14px; color: #aaaaaa; border: none;");

        for (QFrame *card : findChildren<QFrame*>("DashboardCard")) {
            card->setStyleSheet("QFrame#DashboardCard { background-color: #1e1e1e; border-radius: 12px; border: 1px solid #333333; color: white; }");
        }

        for (QLabel *lbl : findChildren<QLabel*>("PanelTitle")) {
            lbl->setStyleSheet("font-size: 16px; font-weight: 800; color: #ffffff; border: none;");
        }

        for (QLabel *lbl : findChildren<QLabel*>("CardLabel")) {
            lbl->setStyleSheet("font-size: 13px; font-weight: 600; color: #aaaaaa; border: none;");
        }
        for (QLabel *lbl : findChildren<QLabel*>("CardValue")) {
            lbl->setStyleSheet("font-size: 28px; font-weight: 800; color: #ffffff; border: none;");
        }
        for (QLabel *lbl : findChildren<QLabel*>("StateTitle")) {
            lbl->setStyleSheet("font-size: 13px; font-weight: 700; color: #ffffff; border: none;");
        }
        for (QLabel *lbl : findChildren<QLabel*>("StateDesc")) {
            lbl->setStyleSheet("font-size: 13px; color: #aaaaaa; border: none;");
        }
        for (QLabel *lbl : findChildren<QLabel*>("NotifText")) {
            lbl->setStyleSheet("font-size: 13px; color: #ffffff; border: none; background: transparent;");
        }

        if (m_btnVerDetalles)
            m_btnVerDetalles->setStyleSheet("QPushButton { background-color: #2a2a2a; color: white; border: 1px solid #444444; padding: 9px 18px; border-radius: 6px; font-weight: 600; font-size: 12px; } QPushButton:hover { background-color: #333333; }");
        if (m_btnResolverConflictos)
            m_btnResolverConflictos->setStyleSheet("QPushButton { background-color: #3b82f6; color: white; border: none; padding: 9px 18px; border-radius: 6px; font-weight: 600; font-size: 12px; } QPushButton:hover { background-color: #2563eb; }");

        for (QFrame *item : findChildren<QFrame*>("NotifItem")) {
            item->setStyleSheet("QFrame { background-color: #252525; border-radius: 8px; border: 1px solid #333333; color: white; }");
        }
    } else {
        if (auto lbl = findChild<QLabel*>("DashboardTitle")) lbl->setStyleSheet("font-size: 24px; font-weight: 800; color: #0f172a; border: none;");
        if (auto lbl = findChild<QLabel*>("DashboardSubtitle")) lbl->setStyleSheet("font-size: 14px; color: #64748b; border: none;");

        for (QFrame *card : findChildren<QFrame*>("DashboardCard")) {
            card->setStyleSheet("QFrame#DashboardCard { background-color: white; border-radius: 12px; border: 1px solid #e2e8f0; }");
        }

        for (QLabel *lbl : findChildren<QLabel*>("PanelTitle")) {
            lbl->setStyleSheet("font-size: 16px; font-weight: 800; color: #0f172a; border: none;");
        }
        for (QLabel *lbl : findChildren<QLabel*>("CardLabel")) {
            lbl->setStyleSheet("font-size: 13px; font-weight: 600; color: #64748b; border: none;");
        }
        for (QLabel *lbl : findChildren<QLabel*>("CardValue")) {
            lbl->setStyleSheet("font-size: 28px; font-weight: 800; color: #0f172a; border: none;");
        }
        for (QLabel *lbl : findChildren<QLabel*>("StateTitle")) {
            lbl->setStyleSheet("font-size: 13px; font-weight: 700; color: #0f172a; border: none;");
        }
        for (QLabel *lbl : findChildren<QLabel*>("StateDesc")) {
            lbl->setStyleSheet("font-size: 13px; color: #64748b; border: none;");
        }
        for (QLabel *lbl : findChildren<QLabel*>("NotifText")) {
            lbl->setStyleSheet("font-size: 13px; color: #0f172a; border: none; background: transparent;");
        }

        if (m_btnVerDetalles)
            m_btnVerDetalles->setStyleSheet("QPushButton { background-color: white; color: #0f172a; border: 1px solid #cbd5e1; padding: 9px 18px; border-radius: 6px; font-weight: 600; font-size: 12px; } QPushButton:hover { background-color: #f8fafc; }");
        if (m_btnResolverConflictos)
            m_btnResolverConflictos->setStyleSheet("QPushButton { background-color: #0f172a; color: white; border: none; padding: 9px 18px; border-radius: 6px; font-weight: 600; font-size: 12px; } QPushButton:hover { background-color: #1e293b; }");

        for (QFrame *item : findChildren<QFrame*>("NotifItem")) {
            item->setStyleSheet("QFrame { background-color: white; border-radius: 8px; border: 1px solid #e2e8f0; }");
        }
    }
}

// ==========================================
// DIÁLOGOS
// ==========================================
void DashboardWidget::mostrarDetalles()
{
    QVBoxLayout* layoutInterno = nullptr;
    QDialog* dialog = crearDialogoFlotante(this, "📊", "Detalles de Generación", layoutInterno);
    dialog->resize(550, 360);

    bool isDark = qApp->property("isDarkMode").toBool();

    QGridLayout *grid = new QGridLayout();
    grid->setSpacing(16);

    QStringList labels = {"Progreso Global", "Horas Asignadas", "Aulas Utilizadas", "Conflictos Detectados"};
    QStringList values = {"85%", "1,240 / 1,450", "34 / 36", "3"};
    // Estilos adaptados para modo claro y oscuro
    QStringList styles = isDark ?
                             QStringList{"color: #34d399;", "color: #ffffff;", "color: #ffffff;", "color: #fbbf24;"} :
                             QStringList{"color: #10b981;", "color: #0f172a;", "color: #0f172a;", "color: #f59e0b;"};

    for (int i = 0; i < 4; ++i) {
        QFrame *card = new QFrame(dialog);
        QString cardBg = isDark ? "background-color: #252525; border: 1px solid #333333;" : "background-color: #f8fafc; border: 1px solid #e2e8f0;";
        card->setStyleSheet(QString("QFrame { %1 border-radius: 8px; }").arg(cardBg));
        QVBoxLayout *cardLayout = new QVBoxLayout(card);

        QLabel *label = new QLabel(labels[i], card);
        label->setAlignment(Qt::AlignLeft);
        QString lblColor = isDark ? "#aaaaaa;" : "#64748b;";
        label->setStyleSheet(QString("font-size: 13px; color: %1 font-weight: 600; border: none; background: transparent;").arg(lblColor));

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
    QString descColor = isDark ? "#cccccc;" : "#64748b;";
    desc->setStyleSheet(QString("font-size: 13px; color: %1 border: none; background: transparent;").arg(descColor));
    layoutInterno->addWidget(desc);

    dialog->exec();
}

void DashboardWidget::resolverConflictos()
{
    QVBoxLayout* layoutInterno = nullptr;
    QDialog* dialog = crearDialogoFlotante(this, "⚠️", "Resolver Conflictos", layoutInterno);
    dialog->resize(600, 480);

    bool isDark = qApp->property("isDarkMode").toBool();

    QLabel *headerDesc = new QLabel("Se encontraron los siguientes choques en la matriz de disponibilidad:", dialog);
    QString headerColor = isDark ? "#cccccc;" : "#334155;";
    headerDesc->setStyleSheet(QString("font-size: 14px; color: %1 border: none; background: transparent;").arg(headerColor));
    layoutInterno->addWidget(headerDesc);

    struct Conflicto { QString titulo, descripcion, btnTexto; };
    QList<Conflicto> conflictos = {
        {"Choque de Horario: Profa. Castro", "Asignada a Biología (Aula 2) y Química (Aula 4) el Lunes a las 08:00 AM.", "Reasignar"},
        {"Aula 3B sin capacidad", "El grupo de 4to Año tiene 45 alumnos, pero el Aula 3B tiene aforo máximo de 30.", "Cambiar\nAula"},
        {"Disponibilidad Excedida: Prof. Méndez", "Excede en 2 horas su carga horaria semanal máxima permitida (36 hrs).", "Ajustar\nCarga"}
    };

    for (const auto &c : conflictos) {
        QFrame *item = new QFrame(dialog);
        // Si es oscuro, usamos un tono rojizo oscuro elegante (#3a1c1c con borde #5c2828); si es claro, #fff1f2 con borde #fecaca
        QString itemStyle = isDark ?
                                "QFrame { border: 1px solid #5c2828; background-color: #2c1616; border-radius: 8px; }" :
                                "QFrame { border: 1px solid #fecaca; background-color: #fff1f2; border-radius: 8px; }";
        item->setStyleSheet(itemStyle);

        QHBoxLayout *itemLayout = new QHBoxLayout(item);
        itemLayout->setContentsMargins(16, 16, 16, 16);

        QVBoxLayout *infoLayout = new QVBoxLayout();
        infoLayout->setSpacing(4);

        QLabel *titulo = new QLabel(c.titulo, item);
        QString alertTextColor = isDark ? "#fca5a5;" : "#9f1239;";
        titulo->setStyleSheet(QString("font-size: 14px; font-weight: 800; color: %1 border: none; background: transparent;").arg(alertTextColor));

        QLabel *desc = new QLabel(c.descripcion, item);
        desc->setStyleSheet(QString("font-size: 13px; color: %1 border: none; background: transparent;").arg(alertTextColor));
        desc->setWordWrap(true);

        infoLayout->addWidget(titulo);
        infoLayout->addWidget(desc);

        QPushButton *btn = new QPushButton(c.btnTexto, item);
        btn->setFixedSize(100, 45);
        btn->setCursor(Qt::PointingHandCursor);
        QString btnStyle = isDark ?
                               "QPushButton { background-color: #3b82f6; color: white; border: none; border-radius: 6px; font-weight: 800; font-size: 12px; } QPushButton:hover { background-color: #2563eb; }" :
                               "QPushButton { background-color: #0f172a; color: white; border: none; border-radius: 6px; font-weight: 800; font-size: 12px; } QPushButton:hover { background-color: #1e293b; }";
        btn->setStyleSheet(btnStyle);

        itemLayout->addLayout(infoLayout);
        itemLayout->addWidget(btn, 0, Qt::AlignVCenter);

        layoutInterno->addWidget(item);
    }

    dialog->exec();
}