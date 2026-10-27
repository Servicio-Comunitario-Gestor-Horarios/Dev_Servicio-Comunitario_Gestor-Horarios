#include "widgets/selector_materias.hpp"

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QStyle>
#include <QStyleOptionButton>
#include <QStylePainter>
#include <QVBoxLayout>

#include "dialogs/schedule_theme.hpp"

namespace {

/// Columnas de la rejilla de materias del diálogo.
constexpr int kColumnasRejilla = 4;
/// Ancho/alto de referencia de cada celda de la rejilla.
constexpr int kAnchoMateria = 150;
constexpr int kAltoMateria = 34;

/**
 * @brief Botón de materia de la rejilla.
 *
 * Se adapta al ancho de su columna (sin forzar el ancho del diálogo) y elide el
 * nombre cuando no cabe; el nombre completo queda en el tooltip.
 */
class BotonMateria : public QPushButton
{
public:
    BotonMateria(const QString& nombre, QWidget* padre)
        : QPushButton(nombre, padre)
    {
        setCheckable(true);
        setCursor(Qt::PointingHandCursor);
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        setMinimumWidth(0);
        setToolTip(nombre);
    }

    QSize sizeHint() const override { return QSize(kAnchoMateria, kAltoMateria); }
    QSize minimumSizeHint() const override { return QSize(70, kAltoMateria); }

protected:
    void paintEvent(QPaintEvent* /*evento*/) override
    {
        QStylePainter pintor(this);
        QStyleOptionButton opcion;
        initStyleOption(&opcion);

        const QString nombre = opcion.text;
        opcion.text.clear();
        pintor.drawControl(QStyle::CE_PushButton, opcion);

        QStyleOptionButton caja;
        initStyleOption(&caja);
        const QRect interior = style()->subElementRect(QStyle::SE_PushButtonContents, &caja, this);
        const QString elidido =
            fontMetrics().elidedText(nombre, Qt::ElideRight, interior.width() - 2);
        pintor.drawItemText(interior, Qt::AlignCenter, opcion.palette, isEnabled(), elidido,
                            QPalette::ButtonText);
    }
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────
// Botón de celda
// ─────────────────────────────────────────────────────────────────────────

BotonMateriasSuplentes::BotonMateriasSuplentes(QWidget* parent)
    : QPushButton(parent)
{
    setCursor(Qt::PointingHandCursor);
    refrescarTexto();
    connect(this, &QPushButton::clicked, this, &BotonMateriasSuplentes::abrirSelector);
}

void BotonMateriasSuplentes::setDisponibles(const QStringList& materias)
{
    m_disponibles = materias;
}

void BotonMateriasSuplentes::setSeleccion(const QStringList& materias)
{
    m_seleccion = materias;
    refrescarTexto();
}

void BotonMateriasSuplentes::abrirSelector()
{
    SelectorMateriasDialog dialogo(m_disponibles, m_seleccion, this);
    if (dialogo.exec() != QDialog::Accepted)
        return;
    const QStringList elegidas = dialogo.seleccionadas();
    if (elegidas == m_seleccion)
        return;
    setSeleccion(elegidas);
    emit seleccionCambiada(m_seleccion);
}

void BotonMateriasSuplentes::refrescarTexto()
{
    if (m_seleccion.isEmpty())
        setText(QStringLiteral("Elegir materias…"));
    else if (m_seleccion.size() <= 2)
        setText(m_seleccion.join(QStringLiteral(", ")));
    else
        setText(QStringLiteral("%1 materias").arg(m_seleccion.size()));

    setToolTip(m_seleccion.isEmpty() ? QStringLiteral("Sin materias suplentes")
                                     : m_seleccion.join(QStringLiteral("\n")));
}

// ─────────────────────────────────────────────────────────────────────────
// Diálogo con la rejilla de materias
// ─────────────────────────────────────────────────────────────────────────

SelectorMateriasDialog::SelectorMateriasDialog(const QStringList& disponibles,
                                               const QStringList& seleccionadas,
                                               QWidget* parent)
    : QDialog(parent)
{
    const ScheduleTheme::Palette p = ScheduleTheme::current();

    setWindowTitle(QStringLiteral("Materias suplentes"));
    setModal(true);
    resize(700, 480);

    auto* raiz = new QVBoxLayout(this);
    raiz->setContentsMargins(0, 0, 0, 0);
    raiz->setSpacing(0);

    auto* tarjeta = new QFrame(this);
    tarjeta->setObjectName(QStringLiteral("selCard"));
    auto* cuerpo = new QVBoxLayout(tarjeta);
    cuerpo->setContentsMargins(24, 20, 24, 20);
    cuerpo->setSpacing(12);

    auto* titulo = new QLabel(QStringLiteral("Materias suplentes"), tarjeta);
    titulo->setObjectName(QStringLiteral("selTitulo"));
    cuerpo->addWidget(titulo);

    auto* ayuda = new QLabel(
        QStringLiteral("Marca las materias que este docente puede cubrir como suplente. "
                       "Las asignadas se muestran en la columna «Materias asignadas»."),
        tarjeta);
    ayuda->setObjectName(QStringLiteral("selAyuda"));
    ayuda->setWordWrap(true);
    cuerpo->addWidget(ayuda);

    auto* contador = new QLabel(tarjeta);
    contador->setObjectName(QStringLiteral("selContador"));

    auto* desplazable = new QScrollArea(tarjeta);
    desplazable->setObjectName(QStringLiteral("selScroll"));
    desplazable->setWidgetResizable(true);
    desplazable->setFrameShape(QFrame::NoFrame);

    auto* rejillaHost = new QWidget(desplazable);
    rejillaHost->setObjectName(QStringLiteral("selRejilla"));
    auto* rejilla = new QGridLayout(rejillaHost);
    rejilla->setContentsMargins(2, 2, 2, 2);
    rejilla->setHorizontalSpacing(8);
    rejilla->setVerticalSpacing(8);

    if (disponibles.isEmpty()) {
        auto* vacio = new QLabel(
            QStringLiteral("No hay materias registradas en la base de datos."), rejillaHost);
        vacio->setObjectName(QStringLiteral("selAyuda"));
        vacio->setWordWrap(true);
        rejilla->addWidget(vacio, 0, 0, 1, kColumnasRejilla);
    } else {
        for (int i = 0; i < disponibles.size(); ++i) {
            auto* boton = new BotonMateria(disponibles[i], rejillaHost);
            boton->setObjectName(QStringLiteral("selMateria"));
            boton->setChecked(seleccionadas.contains(disponibles[i]));
            rejilla->addWidget(boton, i / kColumnasRejilla, i % kColumnasRejilla);
            m_botones << boton;
        }
    }
    for (int col = 0; col < kColumnasRejilla; ++col)
        rejilla->setColumnStretch(col, 1);

    desplazable->setWidget(rejillaHost);
    cuerpo->addWidget(desplazable, 1);
    cuerpo->addWidget(contador);

    auto* pie = new QHBoxLayout();
    pie->setSpacing(10);
    auto* limpiar = new QPushButton(QStringLiteral("Limpiar"), tarjeta);
    limpiar->setObjectName(QStringLiteral("selLimpiar"));
    limpiar->setCursor(Qt::PointingHandCursor);
    limpiar->setMinimumHeight(36);
    auto* cancelar = new QPushButton(QStringLiteral("Cancelar"), tarjeta);
    cancelar->setObjectName(QStringLiteral("selCancelar"));
    cancelar->setCursor(Qt::PointingHandCursor);
    cancelar->setMinimumHeight(36);
    auto* aceptar = new QPushButton(QStringLiteral("Aceptar"), tarjeta);
    aceptar->setObjectName(QStringLiteral("selAceptar"));
    aceptar->setCursor(Qt::PointingHandCursor);
    aceptar->setMinimumHeight(36);
    aceptar->setDefault(true);
    pie->addWidget(limpiar);
    pie->addStretch(1);
    pie->addWidget(cancelar);
    pie->addWidget(aceptar);
    cuerpo->addLayout(pie);

    raiz->addWidget(tarjeta);

    auto actualizarContador = [this, contador, disponibles]() {
        contador->setText(QStringLiteral("%1 de %2 seleccionadas")
                              .arg(this->seleccionadas().size())
                              .arg(disponibles.size()));
    };
    for (QAbstractButton* boton : m_botones)
        connect(boton, &QAbstractButton::toggled, this, [actualizarContador]() { actualizarContador(); });
    actualizarContador();

    connect(limpiar, &QPushButton::clicked, this, [this]() {
        for (QAbstractButton* boton : m_botones)
            boton->setChecked(false);
    });
    connect(cancelar, &QPushButton::clicked, this, &QDialog::reject);
    connect(aceptar, &QPushButton::clicked, this, &QDialog::accept);

    setStyleSheet(QString(
        "#selCard { background:%1; border:1px solid %2; border-radius:14px; }"
        "#selTitulo { font-size:17px; font-weight:700; color:%3; }"
        "#selAyuda { font-size:12px; color:%4; }"
        "#selContador { font-size:12px; color:%4; }"
        "#selScroll, #selRejilla { background:transparent; border:none; }"
        "#selMateria {"
        "  background:%5; color:%3; border:1px solid %2; border-radius:9px;"
        "  padding:4px 10px; text-align:center;"
        "}"
        "#selMateria:hover { border-color:%6; }"
        "#selMateria:checked { background:%6; color:#FFFFFF; border-color:%6; font-weight:600; }"
        "#selLimpiar, #selCancelar {"
        "  min-width:96px; padding:6px 16px; border-radius:9px;"
        "  border:1px solid %2; background:%5; color:%3; font-weight:600;"
        "}"
        "#selLimpiar:hover, #selCancelar:hover { background:%7; }"
        "#selAceptar {"
        "  min-width:110px; padding:6px 18px; border-radius:9px; border:none;"
        "  background:%6; color:#FFFFFF; font-weight:700;"
        "}"
        "#selAceptar:hover { background:%8; }")
                      .arg(p.surface, p.border, p.text, p.textMuted, p.surface2, p.accent,
                           p.surface3, p.accentStrong));
}

QStringList SelectorMateriasDialog::seleccionadas() const
{
    QStringList marcadas;
    for (QAbstractButton* boton : m_botones)
        if (boton->isChecked())
            marcadas << boton->text();
    return marcadas;
}
