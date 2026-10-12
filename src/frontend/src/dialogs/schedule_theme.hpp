/**
 * @file schedule_theme.hpp
 * @brief Tokens de diseño compartidos por los diálogos de horarios.
 *
 * Adapta la estética del prototipo SchedulePrototype (superficies claras,
 * acento índigo, bloques por materia, sombras suaves) a QSS/Qt, con soporte
 * de tema claro/oscuro según la propiedad global "isDarkMode" de qApp.
 *
 * Es un encabezado "header-only": todas las funciones son inline para poder
 * incluirse desde varios .cpp sin duplicar símbolos.
 */

#pragma once

#include <QApplication>
#include <QColor>
#include <QHash>
#include <QString>
#include <QWidget>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QAbstractAnimation>
#include <QDialog>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QDialogButtonBox>

#include <cmath>

namespace ScheduleTheme {

/** ¿Está activo el modo oscuro global? */
inline bool darkMode()
{
    return qApp && qApp->property("isDarkMode").toBool();
}

/** Conjunto de colores del tema actual. */
struct Palette {
    bool dark = false;
    QString bg, surface, surface2, surface3;
    QString border, borderStrong;
    QString text, textMuted, textSubtle;
    QString accent, accentStrong, accentSoft;
    QString success, warning, danger;
};

inline Palette palette(bool dark)
{
    Palette p;
    p.dark = dark;
    if (dark) {
        p.bg           = "#0C111D";
        p.surface      = "#141A29";
        p.surface2     = "#1B2232";
        p.surface3     = "#232B3E";
        p.border       = "#2D3443";
        p.borderStrong = "#3B4456";
        p.text         = "#EEF2F8";
        p.textMuted    = "#A9B3C0";
        p.textSubtle   = "#7E8798";
        p.accent       = "#7C8CF8";
        p.accentStrong = "#5B6CF5";
        p.accentSoft   = "#4A56C9";
        p.success      = "#34D399";
        p.warning      = "#FBBF24";
        p.danger       = "#F87171";
    } else {
        p.bg           = "#F4F6FB";
        p.surface      = "#FFFFFF";
        p.surface2     = "#F1F4F9";
        p.surface3     = "#E7ECF4";
        p.border       = "#E3E8F0";
        p.borderStrong = "#CBD3E0";
        p.text         = "#0F1729";
        p.textMuted    = "#5C667A";
        p.textSubtle   = "#8892A4";
        p.accent       = "#5B6CF5";
        p.accentStrong = "#3D4EF0";
        p.accentSoft   = "#A5AEF9";
        p.success      = "#12B76A";
        p.warning      = "#F79009";
        p.danger       = "#F04438";
    }
    return p;
}

inline Palette current()
{
    return palette(darkMode());
}

/** Color asociado a una materia (paleta armónica del prototipo). */
inline QColor subjectColor(const QString& subject)
{
    static const QHash<QString, QColor> map = {
        {"Matemáticas",      QColor("#5565F1")},
        {"Física",           QColor("#8C61EF")},
        {"Química",          QColor("#12C0E2")},
        {"Biología",         QColor("#2DBE70")},
        {"Historia",         QColor("#F29A36")},
        {"Geografía",        QColor("#E76F51")},
        {"Literatura",       QColor("#EA539E")},
        {"Lenguaje",         QColor("#EA539E")},
        {"Inglés",           QColor("#14B8A6")},
        {"Educación Física", QColor("#E9A400")},
        {"Arte",             QColor("#B164D8")},
        {"Música",           QColor("#D65DB1")},
        {"Tecnología",       QColor("#2F9BD6")},
        {"Filosofía",        QColor("#7C83FD")},
        {"Ciudadanía",       QColor("#F0564E")},
    };
    auto it = map.constFind(subject);
    if (it != map.constEnd()) {
        return it.value();
    }
    // Fallback estable: matiz derivado del nombre.
    const int hue = static_cast<int>(qHash(subject) % 360u);
    return QColor::fromHsl(hue, 170, 140);
}

/** Texto legible (blanco u oscuro) sobre el color de materia dado. */
inline QColor subjectTextColor(const QColor& c)
{
    auto lin = [](double v) {
        return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4);
    };
    const double L = 0.2126 * lin(c.redF())
                   + 0.7152 * lin(c.greenF())
                   + 0.0722 * lin(c.blueF());
    return L > 0.5 ? QColor("#1B2232") : QColor("#FFFFFF");
}

/** Sombra suave estilo "elevación" (QSS no soporta box-shadow).
 *
 * IMPORTANTE: usar solo en widgets con pocos hijos o que no se repintan con
 * frecuencia. Un QGraphicsEffect obliga a Qt a renderizar todo el subárbol en
 * un pixmap fuera de pantalla en cada repintado (p. ej. al hacer hover), y si
 * se anidan efectos (sombra + opacidad) la caché del efecto exterior puede
 * quedar obsoleta y mostrar el contenido en blanco. Por eso NO se aplica
 * ninguna sombra ni efecto a la tarjeta que contiene la cuadrícula.
 */
inline void applyShadow(QWidget* w, bool dark, int blur = 30, int dy = 10)
{
    if (!w) return;
    auto* s = new QGraphicsDropShadowEffect(w);
    s->setBlurRadius(blur);
    s->setOffset(0, dy);
    s->setColor(dark ? QColor(0, 0, 0, 150) : QColor(15, 23, 41, 32));
    w->setGraphicsEffect(s);
}

/**
 * Aplica el estilo del sistema a un QMessageBox (los popups de Qt por defecto
 * usan la paleta del sistema operativo y desentonan con el tema de la app).
 */
inline void applyMessageBoxTheme(QMessageBox& mb)
{
    const Palette p = current();
    mb.setStyleSheet(QString(
        "QMessageBox { background:%1; }"
        "QLabel { color:%2; font-size:13px; }"
        "QPushButton {"
        "  min-width:96px; min-height:30px; padding:4px 16px;"
        "  border-radius:8px; border:1px solid %3;"
        "  background:%4; color:%2; font-weight:600;"
        "}"
        "QPushButton:hover { background:%5; border-color:%6; }"
        "QPushButton:default { background:%6; color:#FFFFFF; border:none; }"
        "QPushButton:default:hover { background:%7; }")
        .arg(p.surface, p.text, p.border, p.surface2, p.surface3, p.accent, p.accentStrong));
}

/** Muestra un popup informativo/de advertencia con el estilo del sistema. */
inline void showStyledMessage(QWidget* parent, const QString& title,
                              const QString& text, bool warning = false)
{
    QMessageBox mb(parent);
    mb.setWindowTitle(title);
    mb.setText(text);
    mb.setIcon(warning ? QMessageBox::Warning : QMessageBox::Information);
    mb.setStandardButtons(QMessageBox::Ok);
    mb.button(QMessageBox::Ok)->setText("Entendido");
    applyMessageBoxTheme(mb);
    mb.exec();
}

/**
 * Diálogo de conflicto al mover un bloque. Presenta las ocupaciones detectadas
 * en otros horarios y ofrece Cancelar o Intercambiar (actualizando todos los
 * horarios afectados). Devuelve true si el usuario elige Intercambiar.
 */
inline bool askSwapConflict(QWidget* parent, const QString& title,
                            const QString& intro, const QStringList& details)
{
    const Palette p = current();

    QDialog dlg(parent);
    dlg.setWindowTitle(title);
    dlg.setModal(true);
    dlg.setMinimumWidth(470);

    auto* root = new QVBoxLayout(&dlg);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Tarjeta contenedora.
    auto* card = new QFrame(&dlg);
    card->setObjectName("conflictCard");
    auto* lay = new QVBoxLayout(card);
    lay->setContentsMargins(26, 22, 26, 20);
    lay->setSpacing(14);

    auto* heading = new QLabel(title, card);
    heading->setObjectName("conflictTitle");
    lay->addWidget(heading);

    auto* introLabel = new QLabel(intro, card);
    introLabel->setObjectName("conflictIntro");
    introLabel->setWordWrap(true);
    lay->addWidget(introLabel);

    if (!details.isEmpty()) {
        auto* box = new QFrame(card);
        box->setObjectName("conflictBox");
        auto* boxLay = new QVBoxLayout(box);
        boxLay->setContentsMargins(14, 12, 14, 12);
        boxLay->setSpacing(8);
        for (const QString& d : details) {
            auto* l = new QLabel(QString::fromUtf8("\u2022  ") + d, box);
            l->setObjectName("conflictDetail");
            l->setWordWrap(true);
            boxLay->addWidget(l);
        }
        lay->addWidget(box);
    }

    auto* buttons = new QHBoxLayout();
    buttons->setSpacing(10);
    buttons->addStretch(1);

    auto* cancel = new QPushButton("Cancelar", card);
    cancel->setObjectName("conflictCancel");
    cancel->setCursor(Qt::PointingHandCursor);
    cancel->setMinimumHeight(36);
    auto* swap = new QPushButton("Intercambiar", card);
    swap->setObjectName("conflictSwap");
    swap->setCursor(Qt::PointingHandCursor);
    swap->setMinimumHeight(36);
    swap->setDefault(true);

    buttons->addWidget(cancel);
    buttons->addWidget(swap);
    lay->addLayout(buttons);

    root->addWidget(card);

    QObject::connect(cancel, &QPushButton::clicked, &dlg, &QDialog::reject);
    QObject::connect(swap,   &QPushButton::clicked, &dlg, &QDialog::accept);

    dlg.setStyleSheet(QString(
        "#conflictCard { background:%1; border:1px solid %2; border-radius:14px; }"
        "#conflictTitle { font-size:17px; font-weight:700; color:%3; }"
        "#conflictIntro { font-size:13px; color:%4; }"
        "#conflictBox { background:%5; border:1px solid %2; border-radius:10px; }"
        "#conflictDetail { font-size:12px; color:%6; }"
        "#conflictCancel {"
        "  min-width:110px; padding:6px 18px; border-radius:9px;"
        "  border:1px solid %2; background:%5; color:%3; font-weight:600;"
        "}"
        "#conflictCancel:hover { background:%7; }"
        "#conflictSwap {"
        "  min-width:130px; padding:6px 18px; border-radius:9px; border:none;"
        "  color:#FFFFFF; font-weight:700;"
        "  background:qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 %8, stop:1 %9);"
        "}"
        "#conflictSwap:hover { background:qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 %9, stop:1 %10); }")
        .arg(p.surface, p.border, p.text, p.textMuted, p.surface2, p.textSubtle,
             p.surface3, p.accent, p.accentStrong, p.accentSoft));

    return dlg.exec() == QDialog::Accepted;
}

} // namespace ScheduleTheme
