#include "gestion_cursos_dialog.hpp"

#include <QAbstractItemView>
#include <QComboBox>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMap>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QSet>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include "backend/services/NucleoDatos.hpp"
#include "backend/services/ServicioCursos.hpp"

#include "dialogs/schedule_theme.hpp"
#include "frontend/preferencias_generacion.hpp"

#include <algorithm>

namespace
{
/// Rol del año en cualquier elemento del árbol.
constexpr int kRolAnio = Qt::UserRole;
/// Rol de la sección en los elementos de sección.
constexpr int kRolSeccion = Qt::UserRole + 1;
/// Rol del id del curso en los elementos de sección.
constexpr int kRolId = Qt::UserRole + 2;

/// «1er año», «2do año», «5to año»… para los títulos del árbol.
QString etiquetaAnio(int anio)
{
    switch (anio) {
    case 1:
        return QStringLiteral("1er año");
    case 2:
        return QStringLiteral("2do año");
    case 3:
        return QStringLiteral("3er año");
    default:
        return QStringLiteral("%1to año").arg(anio);
    }
}
} // namespace

GestionCursosDialog::GestionCursosDialog(NucleoDatos* nucleo, QWidget* parent, int maxAnios)
    : QDialog(parent), m_nucleo(nucleo),
      m_maxAnios(maxAnios > 0
                     ? qBound(PreferenciasGeneracion::kMaxAniosMinimo, maxAnios,
                              PreferenciasGeneracion::kMaxAniosMaximo)
                     : PreferenciasGeneracion::maxAnios())
{
    construirUi();
    aplicarEstilo();
    recargar();
}

void GestionCursosDialog::construirUi()
{
    setWindowTitle(QStringLiteral("Años y secciones"));
    setModal(true);
    resize(620, 640);
    setMinimumSize(520, 560);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto* raiz = new QVBoxLayout(this);
    raiz->setContentsMargins(26, 24, 26, 20);
    raiz->setSpacing(16);

    auto* eyebrow = new QLabel(QStringLiteral("ESTRUCTURA ACADÉMICA"), this);
    eyebrow->setObjectName(QStringLiteral("cursosEyebrow"));
    raiz->addWidget(eyebrow);

    auto* titulo = new QLabel(QStringLiteral("Años y secciones"), this);
    titulo->setObjectName(QStringLiteral("cursosTitulo"));
    raiz->addWidget(titulo);

    auto* subtitulo = new QLabel(
        QStringLiteral("Crea cursos del 1ro al %1to año y elige su sección con una "
                       "letra del abecedario. Un año existe mientras tenga al menos "
                       "una sección; elimina todas sus secciones para borrarlo.")
            .arg(m_maxAnios),
        this);
    subtitulo->setObjectName(QStringLiteral("cursosSub"));
    subtitulo->setWordWrap(true);
    raiz->addWidget(subtitulo);

    m_arbol = new QTreeWidget(this);
    m_arbol->setObjectName(QStringLiteral("cursosArbol"));
    m_arbol->setColumnCount(1);
    m_arbol->setHeaderHidden(true);
    m_arbol->setRootIsDecorated(true);
    m_arbol->setUniformRowHeights(true);
    m_arbol->setSelectionMode(QAbstractItemView::SingleSelection);
    m_arbol->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_arbol->setMinimumHeight(260);
    raiz->addWidget(m_arbol, 1);

    // ── Formulario para crear un curso ────────────────────────────────────
    auto* tarjeta = new QFrame(this);
    tarjeta->setObjectName(QStringLiteral("cursosTarjeta"));

    auto* tarjetaLayout = new QVBoxLayout(tarjeta);
    tarjetaLayout->setContentsMargins(16, 14, 16, 14);
    tarjetaLayout->setSpacing(10);

    auto* tituloForm = new QLabel(QStringLiteral("Crear año o sección"), tarjeta);
    tituloForm->setObjectName(QStringLiteral("cursosTarjetaTitulo"));
    tarjetaLayout->addWidget(tituloForm);

    auto* fila = new QHBoxLayout();
    fila->setSpacing(8);

    auto* lblAnio = new QLabel(QStringLiteral("Año:"), tarjeta);
    m_spinAnio = new QSpinBox(tarjeta);
    m_spinAnio->setObjectName(QStringLiteral("cursosAnio"));
    m_spinAnio->setRange(1, m_maxAnios);
    m_spinAnio->setFixedWidth(74);
    m_spinAnio->setToolTip(
        QStringLiteral("Del 1ro al %1to año. El máximo se configura en "
                       "Configuración → Avanzado.")
            .arg(m_maxAnios));

    auto* lblSeccion = new QLabel(QStringLiteral("Sección:"), tarjeta);
    m_comboSeccion = new QComboBox(tarjeta);
    m_comboSeccion->setObjectName(QStringLiteral("cursosSeccion"));
    m_comboSeccion->setFixedWidth(82);
    m_comboSeccion->setMaxVisibleItems(13);
    m_comboSeccion->setToolTip(
        QStringLiteral("Elige una letra; las secciones que ya existen aparecen "
                       "desactivadas."));
    for (char letra = 'A'; letra <= 'Z'; ++letra) {
        const QString texto = QString(QChar::fromLatin1(letra));
        m_comboSeccion->addItem(texto, texto);
    }

    m_btnAgregar = new QPushButton(QStringLiteral("Agregar"), tarjeta);
    m_btnAgregar->setObjectName(QStringLiteral("cursosPrimario"));
    m_btnAgregar->setCursor(Qt::PointingHandCursor);
    m_btnAgregar->setAutoDefault(false);
    m_btnAgregar->setMinimumWidth(110);
    connect(m_btnAgregar, &QPushButton::clicked, this, &GestionCursosDialog::agregarCurso);

    fila->addWidget(lblAnio);
    fila->addWidget(m_spinAnio);
    fila->addWidget(lblSeccion);
    fila->addWidget(m_comboSeccion);
    fila->addWidget(m_btnAgregar);
    fila->addStretch(1);
    tarjetaLayout->addLayout(fila);
    raiz->addWidget(tarjeta);

    // ── Estado y acciones ────────────────────────────────────────────────
    m_lblEstado = new QLabel(this);
    m_lblEstado->setObjectName(QStringLiteral("cursosEstado"));
    m_lblEstado->setWordWrap(true);
    m_lblEstado->setMinimumHeight(18);
    raiz->addWidget(m_lblEstado);

    auto* pie = new QHBoxLayout();
    pie->setSpacing(10);

    m_btnEliminar = new QPushButton(QStringLiteral("Eliminar seleccionado"), this);
    m_btnEliminar->setObjectName(QStringLiteral("cursosPeligro"));
    m_btnEliminar->setCursor(Qt::PointingHandCursor);
    m_btnEliminar->setAutoDefault(false);
    m_btnEliminar->setEnabled(false);
    connect(m_btnEliminar, &QPushButton::clicked,
            this, &GestionCursosDialog::eliminarSeleccionado);

    auto* btnCerrar = new QPushButton(QStringLiteral("Cerrar"), this);
    btnCerrar->setObjectName(QStringLiteral("cursosSecundario"));
    btnCerrar->setCursor(Qt::PointingHandCursor);
    btnCerrar->setAutoDefault(false);
    connect(btnCerrar, &QPushButton::clicked, this, &QDialog::accept);

    pie->addStretch(1);
    pie->addWidget(m_btnEliminar);
    pie->addWidget(btnCerrar);
    raiz->addLayout(pie);

    connect(m_arbol, &QTreeWidget::itemSelectionChanged,
            this, &GestionCursosDialog::actualizarSegunSeleccion);
    connect(m_spinAnio, &QSpinBox::valueChanged,
            this, [this]() { actualizarSeccionesDisponibles(); });
}

void GestionCursosDialog::aplicarEstilo()
{
    const ScheduleTheme::Palette p = ScheduleTheme::current();

    QString qss = QStringLiteral(R"QSS(
QDialog { background: %BG%; }
QLabel { color: %TEXT%; }
QLabel#cursosEyebrow {
  font-size: 11px; font-weight: 700; letter-spacing: 1px; color: %ACCENT%;
}
QLabel#cursosTitulo { font-size: 21px; font-weight: 800; color: %TEXT%; }
QLabel#cursosSub { font-size: 12px; color: %MUTED%; }
QLabel#cursosEstado { font-size: 12px; color: %MUTED%; }
QFrame#cursosTarjeta {
  background: %SURFACE%; border: 1px solid %BORDER%; border-radius: 12px;
}
QLabel#cursosTarjetaTitulo { font-size: 13px; font-weight: 700; color: %TEXT%; }
QTreeWidget {
  background: %SURFACE%; color: %TEXT%; border: 1px solid %BORDER%;
  border-radius: 12px; outline: none; font-size: 13px; padding: 4px;
}
QTreeWidget::item { padding: 7px 4px; border-radius: 7px; }
QTreeWidget::item:hover { background: %SURFACE2%; }
QTreeWidget::item:selected { background: %ACCENT%; color: #FFFFFF; }
QSpinBox, QComboBox {
  background: %SURFACE2%; color: %TEXT%; border: 1px solid %BORDER%;
  border-radius: 8px; padding: 5px 8px; min-height: 24px; font-size: 13px;
}
QSpinBox:focus, QComboBox:focus { border-color: %ACCENT%; }
QSpinBox:disabled, QComboBox:disabled { color: %SUBTLE%; }
QComboBox::drop-down { border: none; width: 22px; }
QComboBox QAbstractItemView {
  background: %SURFACE%; color: %TEXT%; border: 1px solid %BORDER%;
  border-radius: 8px; padding: 4px; outline: none;
  selection-background-color: %ACCENT%; selection-color: #FFFFFF;
}
QComboBox QAbstractItemView::item { padding: 5px 8px; border-radius: 5px; }
QComboBox QAbstractItemView::item:disabled { color: %SUBTLE%; }
QPushButton {
  background: %SURFACE2%; color: %TEXT%; border: 1px solid %BORDER%;
  border-radius: 8px; padding: 7px 16px; font-size: 13px;
}
QPushButton:hover { border-color: %ACCENT%; }
QPushButton:pressed { background: %SURFACE3%; }
QPushButton:disabled { color: %SUBTLE%; }
QPushButton#cursosPrimario {
  background: %ACCENT%; color: #FFFFFF; border: none; font-weight: 700;
}
QPushButton#cursosPrimario:hover { background: %ACCENT_STRONG%; }
QPushButton#cursosSecundario { min-width: 96px; }
QPushButton#cursosPeligro { border-color: %DANGER%; color: %DANGER%; }
QPushButton#cursosPeligro:hover { background: %DANGER%; color: #FFFFFF; }
QPushButton#cursosPeligro:disabled { border-color: %BORDER%; color: %SUBTLE%; }
)QSS");

    qss.replace(QStringLiteral("%BG%"), p.bg);
    qss.replace(QStringLiteral("%SURFACE3%"), p.surface3);
    qss.replace(QStringLiteral("%SURFACE2%"), p.surface2);
    qss.replace(QStringLiteral("%SURFACE%"), p.surface);
    qss.replace(QStringLiteral("%BORDER%"), p.border);
    qss.replace(QStringLiteral("%ACCENT_STRONG%"), p.accentStrong);
    qss.replace(QStringLiteral("%ACCENT%"), p.accent);
    qss.replace(QStringLiteral("%TEXT%"), p.text);
    qss.replace(QStringLiteral("%MUTED%"), p.textMuted);
    qss.replace(QStringLiteral("%SUBTLE%"), p.textSubtle);
    qss.replace(QStringLiteral("%DANGER%"), p.danger);

    setStyleSheet(qss);

    // El popup de un QComboBox es una ventana aparte: se fija su paleta para
    // que la lista y las letras desactivadas se lean con el tema de la app.
    if (m_comboSeccion) {
        m_comboSeccion->setStyleSheet(qss);
        if (QAbstractItemView* vista = m_comboSeccion->view()) {
            const QColor fondo(p.surface);
            const QColor texto(p.text);
            const QColor resalte(p.accent);

            QPalette paleta = vista->palette();
            paleta.setColor(QPalette::Base, fondo);
            paleta.setColor(QPalette::Window, fondo);
            paleta.setColor(QPalette::Text, texto);
            paleta.setColor(QPalette::WindowText, texto);
            paleta.setColor(QPalette::ButtonText, texto);
            paleta.setColor(QPalette::Highlight, resalte);
            paleta.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#FFFFFF")));
            paleta.setColor(QPalette::Disabled, QPalette::Text, QColor(p.textSubtle));
            paleta.setColor(QPalette::Disabled, QPalette::WindowText, QColor(p.textSubtle));
            vista->setPalette(paleta);
            vista->viewport()->setPalette(paleta);
            vista->setAutoFillBackground(true);
        }
    }
}

void GestionCursosDialog::recargar()
{
    m_arbol->clear();

    int cursosOcultos = 0;

    if (m_nucleo) {
        const auto resultado = m_nucleo->listarCursos();
        if (!resultado.ok) {
            establecerEstado(
                QStringLiteral("No se pudieron leer los cursos: %1")
                    .arg(resultado.mensajeError),
                true);
            return;
        }

        // QMap mantiene los años en orden ascendente.
        QMap<int, QVector<CursoDTO>> porAnio;
        for (const CursoDTO& curso : resultado.valor) {
            if (curso.anio < 1 || curso.anio > m_maxAnios) {
                ++cursosOcultos;
                continue;
            }
            porAnio[curso.anio].append(curso);
        }

        for (auto it = porAnio.cbegin(); it != porAnio.cend(); ++it) {
            QVector<CursoDTO> cursos = it.value();
            std::sort(cursos.begin(), cursos.end(),
                      [](const CursoDTO& a, const CursoDTO& b) {
                          return cursoAntes(a.anio, a.seccion, b.anio, b.seccion);
                      });

            auto* padre = new QTreeWidgetItem(m_arbol);
            const int total = cursos.size();
            padre->setText(0, QStringLiteral("%1 · %2 sección%3")
                                  .arg(etiquetaAnio(it.key()))
                                  .arg(total)
                                  .arg(total == 1 ? QString() : QStringLiteral("es")));
            QFont fuente = padre->font(0);
            fuente.setBold(true);
            padre->setFont(0, fuente);
            padre->setData(0, kRolAnio, it.key());

            for (const CursoDTO& curso : cursos) {
                auto* hijo = new QTreeWidgetItem(padre);
                hijo->setText(0, QStringLiteral("Sección %1").arg(curso.seccion));
                hijo->setData(0, kRolAnio, curso.anio);
                hijo->setData(0, kRolSeccion, curso.seccion);
                hijo->setData(0, kRolId, curso.id);
            }
        }
    }

    m_arbol->expandAll();
    actualizarSeccionesDisponibles();

    if (cursosOcultos > 0) {
        establecerEstado(
            QStringLiteral("Hay %1 curso%2 en años superiores al máximo configurado "
                           "(%3to año); no se muestran. Ajusta el máximo en "
                           "Configuración → Avanzado para gestionarlos.")
                .arg(cursosOcultos)
                .arg(cursosOcultos == 1 ? QString() : QStringLiteral("s"))
                .arg(m_maxAnios),
            true);
    } else if (m_arbol->topLevelItemCount() == 0) {
        establecerEstado(QStringLiteral(
            "Todavía no hay años ni secciones. Agrega el primero con el formulario."));
    }

    actualizarSegunSeleccion();
}

void GestionCursosDialog::actualizarSegunSeleccion()
{
    const bool haySeleccion = m_arbol->currentItem() != nullptr;
    m_btnEliminar->setEnabled(haySeleccion);
    // Con o sin selección se sugiere una identidad: al seleccionar un año se
    // propone su siguiente sección; sin selección, el año siguiente al último.
    sugerirIdentidad();
}

void GestionCursosDialog::sugerirIdentidad()
{
    const int anio = anioSeleccionado();
    if (anio > 0) {
        m_spinAnio->setValue(anio);
        seleccionarSiguienteLibre();
        return;
    }

    int ultimoAnio = 0;
    for (int i = 0; i < m_arbol->topLevelItemCount(); ++i)
        ultimoAnio = qMax(ultimoAnio, m_arbol->topLevelItem(i)->data(0, kRolAnio).toInt());

    m_spinAnio->setValue(qBound(1, ultimoAnio + 1, m_maxAnios));
    seleccionarSiguienteLibre();
}

void GestionCursosDialog::actualizarSeccionesDisponibles()
{
    const int anio = m_spinAnio->value();
    QSet<QString> usadas;
    for (int i = 0; i < m_arbol->topLevelItemCount(); ++i) {
        QTreeWidgetItem* padre = m_arbol->topLevelItem(i);
        if (padre->data(0, kRolAnio).toInt() != anio)
            continue;
        for (int j = 0; j < padre->childCount(); ++j)
            usadas.insert(padre->child(j)->data(0, kRolSeccion).toString());
        break;
    }

    const QString actual = m_comboSeccion->currentData().toString();
    const QSignalBlocker bloqueo(m_comboSeccion);

    m_comboSeccion->clear();
    auto* modelo = qobject_cast<QStandardItemModel*>(m_comboSeccion->model());
    for (char letra = 'A'; letra <= 'Z'; ++letra) {
        const QString texto = QString(QChar::fromLatin1(letra));
        m_comboSeccion->addItem(texto, texto);

        if (usadas.contains(texto)) {
            if (modelo) {
                if (auto* item = modelo->item(m_comboSeccion->count() - 1))
                    item->setEnabled(false);
            }
        }
    }

    // Conserva la letra elegida si sigue libre; si no, pasa a la primera libre.
    const int indiceActual = m_comboSeccion->findData(actual);
    if (indiceActual >= 0 && !usadas.contains(actual)) {
        m_comboSeccion->setCurrentIndex(indiceActual);
        return;
    }
    for (int i = 0; i < m_comboSeccion->count(); ++i) {
        const QString letra = m_comboSeccion->itemData(i).toString();
        if (!usadas.contains(letra)) {
            m_comboSeccion->setCurrentIndex(i);
            return;
        }
    }
}

void GestionCursosDialog::seleccionarSiguienteLibre()
{
    const QString siguiente = siguienteSeccion(m_spinAnio->value());
    if (siguiente.isEmpty())
        return;

    const int indice = m_comboSeccion->findData(siguiente);
    if (indice >= 0)
        m_comboSeccion->setCurrentIndex(indice);
}

QString GestionCursosDialog::siguienteSeccion(int anio) const
{
    QSet<QString> usadas;
    for (int i = 0; i < m_arbol->topLevelItemCount(); ++i) {
        QTreeWidgetItem* padre = m_arbol->topLevelItem(i);
        if (padre->data(0, kRolAnio).toInt() != anio)
            continue;
        for (int j = 0; j < padre->childCount(); ++j)
            usadas.insert(padre->child(j)->data(0, kRolSeccion).toString());
        break;
    }

    for (char letra = 'A'; letra <= 'Z'; ++letra) {
        const QString candidata = QString(QChar::fromLatin1(letra));
        if (!usadas.contains(candidata))
            return candidata;
    }
    return {};
}

int GestionCursosDialog::anioSeleccionado() const
{
    QTreeWidgetItem* item = m_arbol->currentItem();
    return item ? item->data(0, kRolAnio).toInt() : 0;
}

void GestionCursosDialog::agregarCurso()
{
    if (!m_nucleo)
        return;

    const int anio = m_spinAnio->value();
    const QString seccion = m_comboSeccion->currentData().toString();

    if (seccion.isEmpty()) {
        establecerEstado(QStringLiteral("Selecciona una sección del abecedario."), true);
        m_comboSeccion->setFocus();
        return;
    }

    if (siguienteSeccion(anio).isEmpty()) {
        establecerEstado(
            QStringLiteral("El %1 ya tiene las 26 secciones. Elige otro año.")
                .arg(etiquetaAnio(anio)),
            true);
        return;
    }

    const auto resultado = m_nucleo->crearCurso(anio, seccion);
    if (!resultado.ok) {
        establecerEstado(resultado.mensajeError, true);
        return;
    }

    const QString nombre = nombreCurso(resultado.valor.anio, resultado.valor.seccion);
    recargar();
    m_spinAnio->setValue(resultado.valor.anio);
    seleccionarSiguienteLibre();
    m_comboSeccion->setFocus();
    establecerEstado(QStringLiteral("Se creó el curso «%1».").arg(nombre));

    emit cursosCambiados();
}

void GestionCursosDialog::eliminarSeleccionado()
{
    if (!m_nucleo)
        return;

    QTreeWidgetItem* item = m_arbol->currentItem();
    if (!item)
        return;

    const int anio = item->data(0, kRolAnio).toInt();

    if (item->parent() == nullptr) {
        QVector<int> ids;
        for (int i = 0; i < item->childCount(); ++i)
            ids.append(item->child(i)->data(0, kRolId).toInt());
        if (ids.isEmpty())
            return;

        const QString detalle =
            QStringLiteral("Se eliminará el %1 completo, con sus %2 sección%3.")
                .arg(etiquetaAnio(anio))
                .arg(ids.size())
                .arg(ids.size() == 1 ? QString() : QStringLiteral("es"));

        if (!confirmarYEliminar(ids, QStringLiteral("Eliminar año"), detalle))
            return;
    } else {
        const QString seccion = item->data(0, kRolSeccion).toString();
        const QString detalle =
            QStringLiteral("Se eliminará el curso «%1».").arg(nombreCurso(anio, seccion));
        if (!confirmarYEliminar({item->data(0, kRolId).toInt()},
                                QStringLiteral("Eliminar sección"), detalle))
            return;
    }

    recargar();
    m_spinAnio->setValue(qBound(1, anio, m_maxAnios));
    seleccionarSiguienteLibre();
}

bool GestionCursosDialog::confirmarYEliminar(const QVector<int>& ids, const QString& titulo,
                                             const QString& detalle)
{
    QMessageBox confirmacion(this);
    confirmacion.setWindowTitle(titulo);
    confirmacion.setIcon(QMessageBox::Warning);
    confirmacion.setText(detalle);
    confirmacion.setInformativeText(QStringLiteral("Esta acción no se puede deshacer."));
    confirmacion.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    confirmacion.setDefaultButton(QMessageBox::No);
    if (auto* si = confirmacion.button(QMessageBox::Yes))
        si->setText(QStringLiteral("Eliminar"));
    if (auto* no = confirmacion.button(QMessageBox::No))
        no->setText(QStringLiteral("Cancelar"));
    ScheduleTheme::applyMessageBoxTheme(confirmacion);

    if (confirmacion.exec() != QMessageBox::Yes)
        return false;

    int fallos = 0;
    for (int id : ids) {
        if (id > 0 && !m_nucleo->eliminarCurso(id))
            ++fallos;
    }

    if (fallos > 0)
        establecerEstado(
            QStringLiteral("No se pudieron eliminar %1 de %2 cursos.").arg(fallos).arg(ids.size()),
            true);
    else
        establecerEstado(QStringLiteral("Eliminado correctamente."));

    if (fallos < ids.size())
        emit cursosCambiados();

    return true;
}

void GestionCursosDialog::establecerEstado(const QString& texto, bool error)
{
    const ScheduleTheme::Palette p = ScheduleTheme::current();
    m_lblEstado->setText(texto);
    m_lblEstado->setStyleSheet(
        QStringLiteral("font-size: 12px; color: %1;").arg(error ? p.danger : p.textMuted));
}
