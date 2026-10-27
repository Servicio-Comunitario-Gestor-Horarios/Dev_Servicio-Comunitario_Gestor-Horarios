#include "widgets/combo_multiple.hpp"

#include <QAbstractItemView>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStyleOptionComboBox>
#include <QStylePainter>

ComboMultiple::ComboMultiple(QWidget* parent)
    : QComboBox(parent)
    , m_textoVacio(QStringLiteral("Todos"))
{
    setModel(new QStandardItemModel(0, 1, this));
    setCurrentIndex(-1);

    // Los clics de la lista se atienden aquí (en la vista del popup) para poder
    // marcar/desmarcar sin que el desplegable se cierre al primer clic.
    if (QAbstractItemView* lista = view())
        lista->viewport()->installEventFilter(this);
}

void ComboMultiple::agregarOpciones(const QStringList& textos, const QVariantList& datos)
{
    auto* modelo = qobject_cast<QStandardItemModel*>(model());
    if (!modelo)
        return;

    for (int i = 0; i < textos.size(); ++i) {
        auto* item = new QStandardItem(textos[i]);
        item->setData(i < datos.size() ? datos[i] : QVariant(), Qt::UserRole);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Unchecked);
        modelo->invisibleRootItem()->appendRow(item);
    }
    setCurrentIndex(-1);
    refrescar();
}

QStringList ComboMultiple::seleccionados() const
{
    QStringList marcados;
    for (int i = 0; i < count(); ++i)
        if (itemData(i, Qt::CheckStateRole).toInt() == static_cast<int>(Qt::Checked))
            marcados << itemText(i);
    return marcados;
}

QVariantList ComboMultiple::datosSeleccionados() const
{
    QVariantList datos;
    for (int i = 0; i < count(); ++i)
        if (itemData(i, Qt::CheckStateRole).toInt() == static_cast<int>(Qt::Checked))
            datos << itemData(i, Qt::UserRole);
    return datos;
}

void ComboMultiple::setSeleccion(const QStringList& textos)
{
    for (int i = 0; i < count(); ++i) {
        const bool marcar = textos.contains(itemText(i));
        model()->setData(model()->index(i, 0), marcar ? Qt::Checked : Qt::Unchecked,
                         Qt::CheckStateRole);
    }
    refrescar();
}

void ComboMultiple::setSeleccionDatos(const QVariantList& datos)
{
    for (int i = 0; i < count(); ++i) {
        const bool marcar = datos.contains(itemData(i, Qt::UserRole));
        model()->setData(model()->index(i, 0), marcar ? Qt::Checked : Qt::Unchecked,
                         Qt::CheckStateRole);
    }
    refrescar();
}

void ComboMultiple::setTextoVacio(const QString& texto)
{
    m_textoVacio = texto;
    refrescar();
}

bool ComboMultiple::eventFilter(QObject* vigilado, QEvent* evento)
{
    if (QAbstractItemView* lista = view(); lista && vigilado == lista->viewport()) {
        switch (evento->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick: {
            if (evento->type() == QEvent::MouseButtonRelease) {
                const auto* raton = static_cast<QMouseEvent*>(evento);
                const QModelIndex indice = lista->indexAt(raton->position().toPoint());
                if (indice.isValid()) {
                    const bool marcado =
                        indice.data(Qt::CheckStateRole).toInt() == static_cast<int>(Qt::Checked);
                    model()->setData(indice, marcado ? Qt::Unchecked : Qt::Checked,
                                     Qt::CheckStateRole);
                    refrescar();
                    emit seleccionCambiada();
                }
            }
            // Consumir el clic evita que el popup se cierre al marcar una casilla.
            return true;
        }
        default:
            break;
        }
    }
    return QComboBox::eventFilter(vigilado, evento);
}

void ComboMultiple::paintEvent(QPaintEvent* /*evento*/)
{
    // Se pinta como un desplegable normal, pero mostrando el resumen en lugar
    // del ítem actual (los ítems de la lista son casillas, no una elección única).
    QStylePainter pintor(this);
    QStyleOptionComboBox opcion;
    initStyleOption(&opcion);
    opcion.currentText = resumen();
    pintor.drawComplexControl(QStyle::CC_ComboBox, opcion);
    pintor.drawControl(QStyle::CE_ComboBoxLabel, opcion);
}

QString ComboMultiple::resumen() const
{
    const QStringList marcados = seleccionados();
    if (marcados.isEmpty())
        return m_textoVacio;
    if (marcados.size() <= 3)
        return marcados.join(QStringLiteral(", "));
    return QStringLiteral("%1 seleccionados").arg(marcados.size());
}

void ComboMultiple::refrescar()
{
    update();
}
