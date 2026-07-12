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

// Widget para el gauge semicircular
class GaugeWidget : public QWidget
{
public:
    GaugeWidget(int value, QWidget *parent = nullptr) : QWidget(parent), m_value(value) {
        setFixedSize(120, 60);
    }
    void setValue(int value) { m_value = value; update(); }
protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        int padding = 5; // píxeles de borde interno
        int w = width() - 2 * padding;
        int h = height() - 2 * padding;
        int side = qMin(w, h) * 2; // o qMin(w, h) si usas un semicírculo completo
        painter.translate(width()/2, height());
        painter.scale(1.0, -1.0);

        QRectF rect(-side/2, -side/2, side, side);

        // Fondo del arco (gris)
        painter.setPen(QPen(QColor(226, 232, 240), 12));
        painter.drawArc(rect, 0 * 16, 250 * 16);

        // Progreso (verde)
        painter.setPen(QPen(QColor(22, 163, 74), 12));
        int startAngle = -130 * 16;
        int spanAngle = (int)(m_value / 100.0 * 180 * 16);
        painter.drawArc(rect, startAngle, spanAngle);

        // --- Texto: resetear transformación y dibujar en la parte inferior ---
        painter.resetTransform();
        painter.setPen(QColor(15, 23, 42));
        QFont font = painter.font();
        font.setPointSize(14);
        font.setBold(true);
        painter.setFont(font);

        QString text = QString::number(m_value) + "%";
        // Rectángulo para el texto: centrado horizontalmente y en la parte baja
        QRect textRect(5, h - 30, w, 60);
        painter.drawText(textRect, Qt::AlignCenter, text);
    }
private:
    int m_value;
};

DashboardWidget::DashboardWidget(QWidget *parent) : QWidget(parent)
{
    setupUi();
}

DashboardWidget::~DashboardWidget() {}

void DashboardWidget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(16);

    // Título (fuentes reducidas)
    QLabel *title = new QLabel("Panel de control");
    title->setStyleSheet("font-size: 20px; font-weight: 700; color: #0f172a; margin-left: 20px; margin-top: 20px;");
    QLabel *subtitle = new QLabel("Resumen de la institucion y registro de horarios");
    subtitle->setStyleSheet("font-size: 13px; color: #64748b; margin-left: 20px;");

    QVBoxLayout *titleLayout = new QVBoxLayout();
    titleLayout->addWidget(title);
    titleLayout->addWidget(subtitle);
    mainLayout->addLayout(titleLayout);

    // Tarjetas de resumen (iconos y textos más pequeños)
    QHBoxLayout *cardsLayout = new QHBoxLayout();
    cardsLayout->setSpacing(16);
    cardsLayout->addWidget(crearTarjetaResumen("group", "Total Docentes", "124"));
    cardsLayout->addWidget(crearTarjetaResumen("desktop_windows", "Aulas Disponibles", "36"));
    cardsLayout->addWidget(crearTarjetaResumen("book", "Asignaturas Activas", "58"));
    mainLayout->addLayout(cardsLayout);

    // Grid de dos paneles (reducir espaciado)
    QHBoxLayout *gridLayout = new QHBoxLayout();
    gridLayout->setSpacing(16);
    gridLayout->addWidget(crearPanelGeneracion(), 1);
    gridLayout->addWidget(crearPanelNotificaciones(), 1);
    mainLayout->addLayout(gridLayout);

    mainLayout->addStretch();
}

QWidget* DashboardWidget::crearTarjetaResumen(const QString &icono, const QString &label, const QString &valor)
{
    QFrame *card = new QFrame();
    card->setStyleSheet("QFrame { background-color: white; border-radius: 10px; border: 1px solid #e2e8f0; padding: 5px; }");
    QHBoxLayout *layout = new QHBoxLayout(card);
    layout->setSpacing(12);

    // Icono (emojis)
    QString iconText;
    if (icono == "group") iconText = "👥";
    else if (icono == "desktop_windows") iconText = "🖥️";
    else if (icono == "book") iconText = "📚";
    else iconText = "📌";

    QLabel *iconLabel = new QLabel(iconText);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setFixedSize(44, 44);
    iconLabel->setStyleSheet("QLabel { background-color: #f1f5f9; border-radius: 10px; font-size: 22px; }");

    QLabel *labelWidget = new QLabel(label);
    labelWidget->setStyleSheet("font-size: 13px; font-weight: 600; color: #64748b;");
    QLabel *valueWidget = new QLabel(valor);
    valueWidget->setStyleSheet("font-size: 24px; font-weight: 700; color: #0f172a;");

    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->addWidget(labelWidget);
    infoLayout->addWidget(valueWidget);

    layout->addWidget(iconLabel);
    layout->addLayout(infoLayout);
    return card;
}

QWidget* DashboardWidget::crearPanelGeneracion()
{
    QFrame *panel = new QFrame();
    panel->setStyleSheet("QFrame { background-color: white; border-radius: 10px; border: 1px solid #e2e8f0; padding: 18px; }");
    QVBoxLayout *layout = new QVBoxLayout(panel);

    QLabel *title = new QLabel("Última Generación de Horario");
    title->setStyleSheet("font-size: 16px; font-weight: 700; color: #0f172a;");
    layout->addWidget(title);

    QHBoxLayout *statusLayout = new QHBoxLayout();
    statusLayout->setSpacing(20);

    // Gauge
    GaugeWidget *gauge = new GaugeWidget(85);
    statusLayout->addWidget(gauge);

    // Detalles
    QVBoxLayout *detailsLayout = new QVBoxLayout();
    QLabel *estadoLabel = new QLabel("Estado:");
    estadoLabel->setStyleSheet("font-size: 13px; font-weight: 600; color: #0f172a;");
    QLabel *estadoText = new QLabel("Generado, con ajustes manuales necesarios");
    estadoText->setStyleSheet("font-size: 13px; color: #64748b;");
    detailsLayout->addWidget(estadoLabel);
    detailsLayout->addWidget(estadoText);

    QHBoxLayout *actionsLayout = new QHBoxLayout();
    actionsLayout->setSpacing(10);
    m_btnVerDetalles = new QPushButton("Ver Detalles");
    m_btnVerDetalles->setStyleSheet("QPushButton { background-color: transparent; color: #0f172a; border: 1px solid #e2e8f0; padding: 6px 14px; border-radius: 6px; font-weight: 600; font-size: 12px; } QPushButton:hover { background-color: #f1f5f9; }");
    m_btnResolverConflictos = new QPushButton("Resolver Conflictos");
    m_btnResolverConflictos->setStyleSheet("QPushButton { background-color: #0f172a; color: white; border: none; padding: 6px 14px; border-radius: 6px; font-weight: 600; font-size: 12px; } QPushButton:hover { background-color: #1e293b; }");
    actionsLayout->addWidget(m_btnVerDetalles);
    actionsLayout->addWidget(m_btnResolverConflictos);
    detailsLayout->addLayout(actionsLayout);

    statusLayout->addLayout(detailsLayout);
    layout->addLayout(statusLayout);

    connect(m_btnVerDetalles, &QPushButton::clicked, this, &DashboardWidget::mostrarDetalles);
    connect(m_btnResolverConflictos, &QPushButton::clicked, this, &DashboardWidget::resolverConflictos);

    return panel;
}

QWidget* DashboardWidget::crearPanelNotificaciones()
{
    QFrame *panel = new QFrame();
    panel->setStyleSheet("QFrame { background-color: white; border-radius: 10px; border: 1px solid #e2e8f0; padding: 5px; }");
    QVBoxLayout *layout = new QVBoxLayout(panel);

    QLabel *title = new QLabel("Notificaciones Importantes");
    title->setStyleSheet("font-size: 16px; font-weight: 700; color: #0f172a;");
    layout->addWidget(title);

    struct Notificacion {
        int numero;
        QString texto;
        bool destacada;
    };
    QList<Notificacion> notificaciones = {
        {1, "Revisar disponibilidad de la Profa. Castro", true},
        {2, "Asignación de materia \"Matemáticas\" pendiente para aula 3B", false},
        {3, "Asignación de materia \"Matemáticas\" pendiente para aula 3", false}
    };

    for (const auto &n : notificaciones) {
        QFrame *item = new QFrame();
        if (n.destacada) {
            item->setStyleSheet("QFrame { background-color: #f1f5f9; border-radius: 6px; padding: 2px 3px; }");
        } else {
            item->setStyleSheet("QFrame { padding: 2px 3px; }");
        }
        QHBoxLayout *itemLayout = new QHBoxLayout(item);
        itemLayout->setSpacing(12); // Antes 16

        QLabel *numLabel = new QLabel(QString::number(n.numero));
        numLabel->setFixedSize(22, 22); // Antes 26x26
        numLabel->setAlignment(Qt::AlignCenter);
        numLabel->setStyleSheet("QLabel { background-color: #e0e7ff; color: #4f46e5; border-radius: 11px; font-weight: 700; font-size: 12px; }"); // fuente reducida

        QLabel *textLabel = new QLabel(n.texto);
        textLabel->setStyleSheet("font-size: 13px; color: #0f172a; font-weight: 500;"); // Antes 14px
        textLabel->setWordWrap(true);

        itemLayout->addWidget(numLabel);
        itemLayout->addWidget(textLabel);
        layout->addWidget(item);
    }

    return panel;
}

void DashboardWidget::mostrarDetalles()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Detalles de Generación");
    dialog.setModal(true);
    dialog.resize(450, 260); // Tamaño reducido

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QGridLayout *grid = new QGridLayout();
    grid->setSpacing(12); // Antes 16

    QStringList labels = {"Progreso Global", "Horas Asignadas", "Aulas Utilizadas", "Conflictos Detectados"};
    QStringList values = {"85%", "1,240 / 1,450", "34 / 36", "3"};
    QList<QString> styles = {"color: #16a34a;", "", "", "color: #d97706;"};

    for (int i = 0; i < 4; ++i) {
        QFrame *card = new QFrame();
        card->setStyleSheet("QFrame { background-color: #f8fafc; border: 1px solid #e2e8f0; padding: 12px; border-radius: 6px; }"); // padding reducido
        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        QLabel *label = new QLabel(labels[i]);
        label->setStyleSheet("font-size: 12px; color: #64748b; font-weight: 600;"); // Antes 14px
        QLabel *value = new QLabel(values[i]);
        value->setStyleSheet("font-size: 18px; font-weight: 700; color: #0f172a; " + styles[i]); // Antes 22px
        cardLayout->addWidget(label);
        cardLayout->addWidget(value);
        grid->addWidget(card, i / 2, i % 2);
    }
    layout->addLayout(grid);

    QLabel *desc = new QLabel("El algoritmo genético ha completado la fase 4. Se ha maximizado la compactación del horario de los docentes, sin embargo, existen solapamientos de horas en asignaturas clave que requieren intervención manual para respetar la normativa del plantel.");
    desc->setWordWrap(true);
    desc->setStyleSheet("font-size: 12px; color: #64748b; margin-top: 6px;"); // Antes 14px
    layout->addWidget(desc);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    layout->addWidget(buttonBox);

    dialog.exec();
}

void DashboardWidget::resolverConflictos()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Resolver Conflictos");
    dialog.setModal(true);
    dialog.resize(550, 350); // Tamaño reducido

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QLabel *header = new QLabel("Se encontraron los siguientes choques en la matriz de disponibilidad:");
    header->setStyleSheet("font-size: 13px; color: #0f172a; margin-bottom: 12px;"); // Antes 14px
    layout->addWidget(header);

    struct Conflicto {
        QString titulo;
        QString descripcion;
    };
    QList<Conflicto> conflictos = {
        {"Choque de Horario: Profa. Castro", "Asignada a Biología (Aula 2) y Química (Aula 4) el Lunes a las 08:00 AM."},
        {"Aula 3B sin capacidad", "El grupo de 4to Año tiene 45 alumnos, pero el Aula 3B tiene aforo máximo de 30."},
        {"Disponibilidad Excedida: Prof. Méndez", "Excede en 2 horas su carga horaria semanal máxima permitida (36 hrs)."}
    };

    for (const auto &c : conflictos) {
        QFrame *item = new QFrame();
        item->setStyleSheet("QFrame { border: 1px solid #fecaca; background-color: #fef2f2; padding: 12px; border-radius: 6px; }"); // padding reducido
        QHBoxLayout *itemLayout = new QHBoxLayout(item);
        QVBoxLayout *infoLayout = new QVBoxLayout();
        QLabel *titulo = new QLabel(c.titulo);
        titulo->setStyleSheet("font-size: 13px; font-weight: 700; color: #991b1b;"); // Antes 14px
        QLabel *desc = new QLabel(c.descripcion);
        desc->setStyleSheet("font-size: 12px; color: #7f1d1d;"); // Antes 13px
        infoLayout->addWidget(titulo);
        infoLayout->addWidget(desc);
        QPushButton *btn = new QPushButton("Reasignar");
        btn->setStyleSheet("QPushButton { background-color: #0f172a; color: white; border: none; padding: 6px 14px; border-radius: 6px; font-weight: 600; font-size: 12px; } QPushButton:hover { background-color: #1e293b; }"); // padding reducido
        itemLayout->addLayout(infoLayout);
        itemLayout->addWidget(btn);
        layout->addWidget(item);
    }

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    dialog.exec();
}
