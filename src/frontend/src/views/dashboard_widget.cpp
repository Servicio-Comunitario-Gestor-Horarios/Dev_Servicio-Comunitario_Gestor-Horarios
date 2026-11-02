/**
 * @file dashboard_widget.cpp
 * @brief Ventana de bienvenida: logo, frase y totales del centro.
 */

#include "dashboard_widget.hpp"
#include "backend/services/NucleoDatos.hpp"

#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QResource>
#include <QVBoxLayout>

// Fuerza a enlazar el objeto del .qrc (la biblioteca `frontend` es estática):
// sin esta referencia el logo `:/logo.png` puede quedar fuera del ejecutable.
// Debe declararse fuera de un namespace anónimo para que el símbolo de Qt
// conserve enlace externo.
static void iniciarRecursos()
{
    Q_INIT_RESOURCE(resources);
}

DashboardWidget::DashboardWidget(QWidget *parent) : QWidget(parent)
{
    iniciarRecursos();
    this->setStyleSheet("background: transparent;");
    setupUi();
}

// ==========================================
// DISTRIBUCIÓN VISUAL
// ==========================================
void DashboardWidget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 36, 40, 36);
    mainLayout->setSpacing(0);

    mainLayout->addStretch(3);

    // ── Logo del liceo ────────────────────────────────────────────────────
    QLabel *logo = new QLabel(this);
    logo->setObjectName("DashboardLogo");
    logo->setAlignment(Qt::AlignCenter);
    logo->setStyleSheet("background: transparent; border: none;");
    const QPixmap pixmap(QStringLiteral(":/logo.png"));
    if (!pixmap.isNull()) {
        logo->setPixmap(pixmap.scaled(150, 150, Qt::KeepAspectRatio,
                                      Qt::SmoothTransformation));
    }
    mainLayout->addWidget(logo, 0, Qt::AlignHCenter);

    mainLayout->addSpacing(16);

    // ── Bienvenida y frase ────────────────────────────────────────────────
    QLabel *bienvenida = new QLabel(QStringLiteral("Bienvenido"), this);
    bienvenida->setObjectName("DashboardTitle");
    bienvenida->setAlignment(Qt::AlignCenter);
    bienvenida->setStyleSheet(
        "font-size: 30px; font-weight: 800; color: #0f172a; border: none;");
    mainLayout->addWidget(bienvenida);

    mainLayout->addSpacing(6);

    QLabel *frase = new QLabel(
        QStringLiteral("Enseñar y aprender: dos caminos que crecen juntos."), this);
    frase->setObjectName("DashboardSubtitle");
    frase->setAlignment(Qt::AlignCenter);
    frase->setWordWrap(true);
    frase->setStyleSheet(
        "font-size: 14px; font-style: italic; color: #64748b; border: none;");
    mainLayout->addWidget(frase);

    mainLayout->addStretch(4);

    // ── Totales del centro, al pie ────────────────────────────────────────
    QHBoxLayout *statsLayout = new QHBoxLayout();
    statsLayout->setSpacing(16);
    statsLayout->addStretch(1);
    statsLayout->addWidget(crearTarjetaResumen(
        QStringLiteral("👥"), QStringLiteral("Docentes"), m_lblValorDocentes));
    statsLayout->addWidget(crearTarjetaResumen(
        QStringLiteral("🏫"), QStringLiteral("Aulas"), m_lblValorAulas));
    statsLayout->addWidget(crearTarjetaResumen(
        QStringLiteral("📚"), QStringLiteral("Materias"), m_lblValorMaterias));
    statsLayout->addStretch(1);
    mainLayout->addLayout(statsLayout);
}

QWidget* DashboardWidget::crearTarjetaResumen(const QString &icono, const QString &label,
                                              QLabel*& outLabel)
{
    QFrame *card = new QFrame(this);
    card->setObjectName("DashboardCard");
    card->setFixedSize(190, 86);
    card->setStyleSheet(
        "QFrame#DashboardCard { background-color: white; border-radius: 14px; "
        "border: 1px solid #e2e8f0; }");

    QHBoxLayout *layout = new QHBoxLayout(card);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(12);

    QLabel *iconLabel = new QLabel(icono, card);
    iconLabel->setObjectName("CardIcon");
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setFixedSize(38, 38);
    iconLabel->setStyleSheet(
        "background-color: #f8fafc; border: 1px solid #eef2f7; "
        "border-radius: 10px; font-size: 17px;");

    QVBoxLayout *infoLayout = new QVBoxLayout();
    infoLayout->setSpacing(0);

    QLabel *valueWidget = new QLabel(QStringLiteral("..."), card);
    valueWidget->setObjectName("CardValue");
    valueWidget->setStyleSheet(
        "font-size: 22px; font-weight: 800; color: #0f172a; border: none;");

    QLabel *labelWidget = new QLabel(label, card);
    labelWidget->setObjectName("CardLabel");
    labelWidget->setStyleSheet(
        "font-size: 11px; font-weight: 600; color: #64748b; border: none;");

    infoLayout->addWidget(valueWidget);
    infoLayout->addWidget(labelWidget);

    layout->addWidget(iconLabel);
    layout->addLayout(infoLayout, 1);

    outLabel = valueWidget;
    return card;
}

// ==========================================
// DATOS
// ==========================================
void DashboardWidget::setNucleoDatos(NucleoDatos* nucleo)
{
    m_nucleo = nucleo;
    actualizarEstadisticas();
}

void DashboardWidget::actualizarEstadisticas()
{
    if (!m_nucleo)
        return;

    const auto resDocentes = m_nucleo->listarDocentes();
    if (m_lblValorDocentes)
        m_lblValorDocentes->setText(QString::number(resDocentes.ok ? resDocentes.valor.size() : 0));

    const auto resAulas = m_nucleo->listarAulas();
    if (m_lblValorAulas)
        m_lblValorAulas->setText(QString::number(resAulas.ok ? resAulas.valor.size() : 0));

    const auto resMaterias = m_nucleo->listarMaterias();
    if (m_lblValorMaterias)
        m_lblValorMaterias->setText(QString::number(resMaterias.ok ? resMaterias.valor.size() : 0));
}

// ==========================================
// TEMA
// ==========================================
void DashboardWidget::actualizarTema(bool modoOscuro)
{
    const QString texto = modoOscuro ? QStringLiteral("#ffffff") : QStringLiteral("#0f172a");
    const QString tenue = modoOscuro ? QStringLiteral("#a9b3c0") : QStringLiteral("#64748b");
    const QString fondo = modoOscuro ? QStringLiteral("#1e1e1e") : QStringLiteral("#ffffff");
    const QString borde = modoOscuro ? QStringLiteral("#333333") : QStringLiteral("#e2e8f0");
    const QString fondoIcono = modoOscuro ? QStringLiteral("#252525") : QStringLiteral("#f8fafc");
    const QString bordeIcono = modoOscuro ? QStringLiteral("#3a3a3a") : QStringLiteral("#eef2f7");

    if (auto lbl = findChild<QLabel*>(QStringLiteral("DashboardTitle")))
        lbl->setStyleSheet(QString("font-size: 30px; font-weight: 800; color: %1; border: none;")
                               .arg(texto));
    if (auto lbl = findChild<QLabel*>(QStringLiteral("DashboardSubtitle")))
        lbl->setStyleSheet(QString("font-size: 14px; font-style: italic; color: %1; border: none;")
                               .arg(tenue));

    for (QFrame *card : findChildren<QFrame*>(QStringLiteral("DashboardCard")))
        card->setStyleSheet(QString(
            "QFrame#DashboardCard { background-color: %1; border-radius: 14px; "
            "border: 1px solid %2; }").arg(fondo, borde));

    for (QLabel *lbl : findChildren<QLabel*>(QStringLiteral("CardIcon")))
        lbl->setStyleSheet(QString(
            "background-color: %1; border: 1px solid %2; border-radius: 10px; "
            "font-size: 17px;").arg(fondoIcono, bordeIcono));

    for (QLabel *lbl : findChildren<QLabel*>(QStringLiteral("CardLabel")))
        lbl->setStyleSheet(QString("font-size: 11px; font-weight: 600; color: %1; border: none;")
                               .arg(tenue));

    for (QLabel *lbl : findChildren<QLabel*>(QStringLiteral("CardValue")))
        lbl->setStyleSheet(QString("font-size: 22px; font-weight: 800; color: %1; border: none;")
                               .arg(texto));
}
