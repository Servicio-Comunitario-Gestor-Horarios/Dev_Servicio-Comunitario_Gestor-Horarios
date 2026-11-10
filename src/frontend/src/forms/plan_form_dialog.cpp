/**
 * @file plan_form_dialog.cpp
 * @brief Implementación del formulario de planes de estudio (Issue #97).
 */

#include "plan_form_dialog.hpp"

#include <QApplication>
#include <QCheckBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMap>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSet>
#include <QSpinBox>
#include <QStringList>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>

#include <algorithm>

namespace gestor::frontend::forms {

namespace {
constexpr int HORAS_POR_DEFECTO = 2;
constexpr int HORAS_MAXIMAS_POR_MATERIA = MAX_HORAS_SEMANALES_POR_ANIO;
constexpr int CURSO_MINIMO = 1;
constexpr int CURSO_MAXIMO = 6;
constexpr int ANIO_MINIMO_GLOBAL = 1;
constexpr int ANIO_MAXIMO_GLOBAL = 5;

QString nombreMateriaDe(const QVector<MateriaDTO>& materias, int id) {
    for (const MateriaDTO& materia : materias) {
        if (materia.id == id) {
            return materia.nombre;
        }
    }
    return QString();
}
} // namespace

PlanFormDialog::PlanFormDialog(NucleoDatos* nucleo, QWidget* parent)
    : QDialog(parent), m_nucleo(nucleo) {
    setupUi();
    aplicarTema();
    cargarCatalogo();
}

void PlanFormDialog::setupUi() {
    setWindowTitle("Plan de estudios");
    setModal(true);
    resize(960, 720);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(28, 24, 28, 24);
    mainLayout->setSpacing(18);

    // ── Cabecera ──────────────────────────────────────────────────────────
    m_titulo = new QLabel("Nuevo plan de estudio", this);
    m_titulo->setObjectName("PlanFormTitle");
    m_subtitulo = new QLabel("Define el código, el nombre y las materias del plan", this);
    m_subtitulo->setObjectName("PlanFormSub");

    auto* headerLayout = new QVBoxLayout();
    headerLayout->setSpacing(2);
    headerLayout->addWidget(m_titulo);
    headerLayout->addWidget(m_subtitulo);
    mainLayout->addLayout(headerLayout);

    // ── Datos del plan ────────────────────────────────────────────────────
    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(12);
    formLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_codigo = new QLineEdit(this);
    m_codigo->setPlaceholderText("Ej. 1001");
    m_codigo->setMaxLength(20);
    m_codigo->setValidator(new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("\\d{0,20}")), this));
    m_codigo->setToolTip("El código del plan debe ser numérico y único.");

    m_nombre = new QLineEdit(this);
    m_nombre->setPlaceholderText("Nombre del plan de estudio");
    m_nombre->setMaxLength(120);

    m_descripcion = new QTextEdit(this);
    m_descripcion->setPlaceholderText("Descripción (opcional)");
    m_descripcion->setFixedHeight(70);
    m_descripcion->setTabChangesFocus(true);

    formLayout->addRow(QStringLiteral("Código:"), m_codigo);
    formLayout->addRow(QStringLiteral("Nombre:"), m_nombre);
    formLayout->addRow(QStringLiteral("Descripción:"), m_descripcion);
    mainLayout->addLayout(formLayout);

    // ── Selector dual de materias ─────────────────────────────────────────
    auto* selectorLayout = new QHBoxLayout();
    selectorLayout->setSpacing(14);

    auto* grupoDisponibles = new QGroupBox(QStringLiteral("Materias disponibles"), this);
    auto* layoutDisponibles = new QVBoxLayout(grupoDisponibles);
    layoutDisponibles->setContentsMargins(12, 18, 12, 12);
    layoutDisponibles->setSpacing(8);

    m_buscar = new QLineEdit(grupoDisponibles);
    m_buscar->setPlaceholderText("Buscar materia…");
    m_buscar->setClearButtonEnabled(true);
    layoutDisponibles->addWidget(m_buscar);

    m_disponibles = new QListWidget(grupoDisponibles);
    m_disponibles->setSelectionMode(QAbstractItemView::SingleSelection);
    m_disponibles->setAlternatingRowColors(true);
    layoutDisponibles->addWidget(m_disponibles, 1);

    auto* botonesMedio = new QVBoxLayout();
    botonesMedio->setSpacing(10);
    botonesMedio->addStretch();

    m_btnAgregar = new QPushButton(QStringLiteral("Agregar  →"), this);
    m_btnAgregar->setObjectName("PlanAddButton");
    m_btnAgregar->setCursor(Qt::PointingHandCursor);
    m_btnAgregar->setMinimumWidth(110);

    m_btnQuitar = new QPushButton(QStringLiteral("←  Quitar"), this);
    m_btnQuitar->setObjectName("PlanRemoveButton");
    m_btnQuitar->setCursor(Qt::PointingHandCursor);
    m_btnQuitar->setMinimumWidth(110);

    botonesMedio->addWidget(m_btnAgregar);
    botonesMedio->addWidget(m_btnQuitar);
    botonesMedio->addStretch();

    auto* grupoSeleccionadas = new QGroupBox(QStringLiteral("Materias del plan"), this);
    auto* layoutSeleccionadas = new QVBoxLayout(grupoSeleccionadas);
    layoutSeleccionadas->setContentsMargins(12, 18, 12, 12);
    layoutSeleccionadas->setSpacing(8);

    m_seleccionadas = new QTableWidget(0, 3, grupoSeleccionadas);
    m_seleccionadas->setHorizontalHeaderLabels(
        {QStringLiteral("Materia"), QStringLiteral("Horas/sem."), QStringLiteral("Año")});
    m_seleccionadas->verticalHeader()->setVisible(false);
    m_seleccionadas->setShowGrid(false);
    m_seleccionadas->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_seleccionadas->setSelectionMode(QAbstractItemView::SingleSelection);
    m_seleccionadas->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_seleccionadas->verticalHeader()->setDefaultSectionSize(42);

    QHeaderView* cabecera = m_seleccionadas->horizontalHeader();
    cabecera->setSectionResizeMode(0, QHeaderView::Stretch);
    cabecera->setSectionResizeMode(1, QHeaderView::Fixed);
    cabecera->setSectionResizeMode(2, QHeaderView::Fixed);
    m_seleccionadas->setColumnWidth(1, 100);
    m_seleccionadas->setColumnWidth(2, 90);
    cabecera->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    layoutSeleccionadas->addWidget(m_seleccionadas, 1);

    selectorLayout->addWidget(grupoDisponibles, 1);
    selectorLayout->addLayout(botonesMedio);
    selectorLayout->addWidget(grupoSeleccionadas, 2);
    mainLayout->addLayout(selectorLayout, 1);

    // ── Modo global y horas totales ───────────────────────────────────────
    auto* totalesLayout = new QHBoxLayout();
    totalesLayout->setSpacing(12);

    m_global = new QCheckBox(
        QStringLiteral("Plan global: las mismas materias en todos los años (1.º–5.º)"),
        this);
    m_global->setChecked(true);
    m_global->setToolTip(
        "Si está marcado, las materias se guardan una sola vez y se aplican a los "
        "años 1 a 5. Desmárquelo para distribuir materias por año.");

    m_horasTotales = new QLabel(this);
    m_horasTotales->setObjectName("PlanHorasTotales");

    totalesLayout->addWidget(m_global);
    totalesLayout->addStretch();
    totalesLayout->addWidget(m_horasTotales);
    mainLayout->addLayout(totalesLayout);

    // ── Botones ───────────────────────────────────────────────────────────
    auto* botonesLayout = new QHBoxLayout();
    botonesLayout->addStretch();

    m_btnCancelar = new QPushButton(QStringLiteral("Cancelar"), this);
    m_btnCancelar->setObjectName("PlanSecondaryButton");
    m_btnCancelar->setCursor(Qt::PointingHandCursor);

    m_btnGuardar = new QPushButton(QStringLiteral("Guardar plan"), this);
    m_btnGuardar->setObjectName("PlanPrimaryButton");
    m_btnGuardar->setCursor(Qt::PointingHandCursor);
    m_btnGuardar->setDefault(true);

    botonesLayout->addWidget(m_btnCancelar);
    botonesLayout->addWidget(m_btnGuardar);
    mainLayout->addLayout(botonesLayout);

    // ── Conexiones ────────────────────────────────────────────────────────
    connect(m_btnAgregar, &QPushButton::clicked,
            this, &PlanFormDialog::agregarMateriaSeleccionada);
    connect(m_btnQuitar, &QPushButton::clicked,
            this, &PlanFormDialog::quitarMateriaSeleccionada);
    connect(m_disponibles, &QListWidget::itemDoubleClicked,
            this, &PlanFormDialog::agregarPorDobleClic);
    connect(m_buscar, &QLineEdit::textChanged,
            this, &PlanFormDialog::filtrarDisponibles);
    connect(m_global, &QCheckBox::toggled,
            this, &PlanFormDialog::alternarModoGlobal);
    connect(m_btnGuardar, &QPushButton::clicked,
            this, &PlanFormDialog::guardarPlan);
    connect(m_btnCancelar, &QPushButton::clicked,
            this, &QDialog::reject);

    actualizarTotales();
}

void PlanFormDialog::aplicarTema() {
    const bool isDark = qApp->property("isDarkMode").toBool();

    const QString fondo = isDark ? QStringLiteral("#1e1e1e") : QStringLiteral("#ffffff");
    const QString texto = isDark ? QStringLiteral("#f8fafc") : QStringLiteral("#0f172a");
    const QString suave = isDark ? QStringLiteral("#94a3b8") : QStringLiteral("#64748b");
    const QString borde = isDark ? QStringLiteral("#3f3f46") : QStringLiteral("#d1d5db");
    const QString campoFondo = isDark ? QStringLiteral("#27272a") : QStringLiteral("#ffffff");
    const QString tablaCabecera = isDark ? QStringLiteral("#252525") : QStringLiteral("#f8fafc");
    const QString tablaFondo = isDark ? QStringLiteral("#1e1e1e") : QStringLiteral("#ffffff");
    const QString seleccion = isDark ? QStringLiteral("#1d4ed8") : QStringLiteral("#dbeafe");
    const QString seleccionTexto = isDark ? QStringLiteral("#ffffff") : QStringLiteral("#0f172a");

    QString hoja = QStringLiteral(
                       "QDialog { background-color: %1; color: %2; }"
                       "QLabel { color: %2; }"
                       "QLabel#PlanFormTitle { font-size: 22px; font-weight: 800; color: %2; }"
                       "QLabel#PlanFormSub { font-size: 13px; color: %3; }"
                       "QLabel#PlanHorasTotales { font-size: 13px; font-weight: 700; color: %3; }"
                       "QLineEdit, QTextEdit, QSpinBox {"
                       "  border: 1px solid %4; border-radius: 6px; padding: 7px;"
                       "  background-color: %5; color: %2; font-size: 13px;"
                       "}"
                       "QLineEdit:focus, QTextEdit:focus, QSpinBox:focus {"
                       "  border: 1px solid #3b82f6;"
                       "}"
                       "QLineEdit:read-only { background-color: %6; color: %3; }"
                       "QGroupBox {"
                       "  border: 1px solid %4; border-radius: 8px; margin-top: 10px;"
                       "  padding-top: 10px; font-weight: 700; color: %2;"
                       "}"
                       "QGroupBox::title {"
                       "  subcontrol-origin: margin; left: 10px; padding: 0 4px;"
                       "}"
                       "QListWidget, QTableWidget {"
                       "  background-color: %7; border: 1px solid %4; border-radius: 6px;"
                       "  color: %2; outline: none;"
                       "}"
                       "QListWidget::item { padding: 6px 8px; }"
                       "QListWidget::item:selected, QTableWidget::item:selected {"
                       "  background-color: %8; color: %9;"
                       "}"
                       "QHeaderView::section {"
                       "  background-color: %10; color: %3; font-weight: 700;"
                       "  border: none; border-bottom: 1px solid %4; padding: 8px;"
                       "}"
                       "QTableWidget::item { padding: 6px 8px; }"
                       "QCheckBox { color: %2; spacing: 8px; }"
                       "QPushButton#PlanPrimaryButton {"
                       "  background-color: %11; color: white; font-weight: 700;"
                       "  border: none; border-radius: 6px; padding: 10px 22px;"
                       "}"
                       "QPushButton#PlanPrimaryButton:hover { background-color: %12; }"
                       "QPushButton#PlanSecondaryButton {"
                       "  background-color: %13; color: %2; font-weight: 700;"
                       "  border: 1px solid %4; border-radius: 6px; padding: 10px 18px;"
                       "}"
                       "QPushButton#PlanSecondaryButton:hover { background-color: %14; }"
                       "QPushButton#PlanAddButton {"
                       "  background-color: %11; color: white; font-weight: 700;"
                       "  border: none; border-radius: 6px; padding: 8px 10px;"
                       "}"
                       "QPushButton#PlanAddButton:hover { background-color: %12; }"
                       "QPushButton#PlanRemoveButton {"
                       "  background-color: %13; color: %2; font-weight: 700;"
                       "  border: 1px solid %4; border-radius: 6px; padding: 8px 10px;"
                       "}"
                       "QPushButton#PlanRemoveButton:hover { background-color: %14; }")
                       .arg(fondo)
                       .arg(texto)
                       .arg(suave)
                       .arg(borde)
                       .arg(campoFondo)
                       .arg(isDark ? QStringLiteral("#18181b") : QStringLiteral("#f1f5f9"))
                       .arg(tablaFondo)
                       .arg(seleccion)
                       .arg(seleccionTexto)
                       .arg(tablaCabecera)
                       .arg(isDark ? QStringLiteral("#3b82f6") : QStringLiteral("#1e3a8a"))
                       .arg(isDark ? QStringLiteral("#2563eb") : QStringLiteral("#162a69"))
                       .arg(isDark ? QStringLiteral("#2a2a2a") : QStringLiteral("#ffffff"))
                       .arg(isDark ? QStringLiteral("#333333") : QStringLiteral("#f8fafc"));

    setStyleSheet(hoja);

    if (m_titulo) {
        m_titulo->setStyleSheet(QStringLiteral("font-size: 22px; font-weight: 800; color: %1;")
                                    .arg(texto));
    }
    if (m_subtitulo) {
        m_subtitulo->setStyleSheet(QStringLiteral("font-size: 13px; color: %1;").arg(suave));
    }
}

void PlanFormDialog::cargarCatalogo() {
    m_materias.clear();
    if (!m_nucleo) {
        return;
    }

    const auto resultado = m_nucleo->listarMaterias();
    if (!resultado.ok) {
        QMessageBox::warning(this, QStringLiteral("Catálogo no disponible"),
                             resultado.mensajeError);
        return;
    }

    m_materias = resultado.valor;
    std::sort(m_materias.begin(), m_materias.end(),
              [](const MateriaDTO& a, const MateriaDTO& b) {
                  return QString::compare(a.nombre, b.nombre, Qt::CaseInsensitive) < 0;
              });
}

void PlanFormDialog::prepararCreacion(const QString& codigoSugerido,
                                      const QString& nombreSugerido,
                                      const QString& descripcion,
                                      const QVector<CargaHorariaDTO>& materias,
                                      bool global) {
    m_edicion = false;
    m_codigoOriginal.clear();

    setWindowTitle(QStringLiteral("Nuevo plan de estudio"));
    if (m_titulo) m_titulo->setText(QStringLiteral("Nuevo plan de estudio"));
    if (m_subtitulo) {
        m_subtitulo->setText(
            QStringLiteral("Define el código, el nombre y las materias del plan"));
    }

    m_codigo->setReadOnly(false);
    m_codigo->setText(codigoSugerido.isEmpty() ? sugerirCodigo() : codigoSugerido);
    m_nombre->setText(nombreSugerido);
    m_descripcion->setPlainText(descripcion);

    m_seleccionadas->setRowCount(0);
    m_global->setChecked(global);
    alternarModoGlobal(global);

    if (m_materias.isEmpty()) {
        cargarCatalogo();
    }

    for (const CargaHorariaDTO& carga : materias) {
        Seleccion seleccion;
        seleccion.idMateria = carga.idMateria;
        seleccion.horas = carga.horas > 0 ? carga.horas : HORAS_POR_DEFECTO;
        seleccion.curso = global ? 1 : carga.curso;
        seleccion.nombre = nombreMateriaDe(m_materias, carga.idMateria);
        if (seleccion.nombre.isEmpty()) {
            seleccion.nombre = carga.nombreMateria;
        }
        agregarFilaSeleccionada(seleccion);
    }

    refrescarDisponibles();
    actualizarTotales();
    m_codigo->setFocus();
}

void PlanFormDialog::prepararEdicion(const PlanDTO& plan,
                                     const QVector<CargaHorariaDTO>& materias) {
    m_edicion = true;
    m_codigoOriginal = plan.codigo;

    setWindowTitle(QStringLiteral("Editar plan de estudio"));
    if (m_titulo) m_titulo->setText(QStringLiteral("Editar plan de estudio"));
    if (m_subtitulo) {
        m_subtitulo->setText(
            QStringLiteral("El código no se puede modificar; puede cambiar nombre, "
                           "materias y horas"));
    }

    m_codigo->setReadOnly(true);
    m_codigo->setText(plan.codigo);
    m_nombre->setText(plan.nombre);
    m_descripcion->setPlainText(plan.descripcion);

    if (m_materias.isEmpty()) {
        cargarCatalogo();
    }

    QSet<int> cursos;
    for (const CargaHorariaDTO& carga : materias) {
        cursos.insert(carga.curso);
    }

    const bool global = cursos.isEmpty()
                        || (cursos.size() == 1
                            && *cursos.constBegin() >= ANIO_MINIMO_GLOBAL
                            && *cursos.constBegin() <= ANIO_MAXIMO_GLOBAL);

    m_seleccionadas->setRowCount(0);
    m_global->setChecked(global);
    alternarModoGlobal(global);

    for (const CargaHorariaDTO& carga : materias) {
        Seleccion seleccion;
        seleccion.idMateria = carga.idMateria;
        seleccion.horas = carga.horas > 0 ? carga.horas : HORAS_POR_DEFECTO;
        seleccion.curso = global ? 1 : carga.curso;
        seleccion.nombre = nombreMateriaDe(m_materias, carga.idMateria);
        if (seleccion.nombre.isEmpty()) {
            seleccion.nombre = carga.nombreMateria;
        }
        agregarFilaSeleccionada(seleccion);
    }

    refrescarDisponibles();
    actualizarTotales();
}

QString PlanFormDialog::sugerirCodigo() const {
    int maximo = 1000;
    if (m_nucleo) {
        const auto resultado = m_nucleo->listarPlanes();
        if (resultado.ok) {
            for (const PlanDTO& plan : resultado.valor) {
                bool ok = false;
                const int numero = plan.codigo.toInt(&ok);
                if (ok && numero > maximo) {
                    maximo = numero;
                }
            }
        }
    }
    return QString::number(maximo + 1);
}

void PlanFormDialog::refrescarDisponibles() {
    if (!m_disponibles) {
        return;
    }

    const QVector<Seleccion> seleccion = seleccionActual();
    QSet<int> idsSeleccionados;
    for (const Seleccion& item : seleccion) {
        idsSeleccionados.insert(item.idMateria);
    }

    const QString filtro = m_buscar ? m_buscar->text().trimmed() : QString();

    m_disponibles->clear();
    for (const MateriaDTO& materia : m_materias) {
        if (idsSeleccionados.contains(materia.id)) {
            continue;
        }
        if (!filtro.isEmpty()
            && !materia.nombre.contains(filtro, Qt::CaseInsensitive)) {
            continue;
        }

        auto* item = new QListWidgetItem(materia.nombre, m_disponibles);
        item->setData(Qt::UserRole, materia.id);
    }
}

void PlanFormDialog::filtrarDisponibles(const QString& texto) {
    const QString filtro = texto.trimmed();
    for (int i = 0; i < m_disponibles->count(); ++i) {
        QListWidgetItem* item = m_disponibles->item(i);
        item->setHidden(!filtro.isEmpty()
                        && !item->text().contains(filtro, Qt::CaseInsensitive));
    }
}

void PlanFormDialog::agregarMateriaSeleccionada() {
    QListWidgetItem* item = m_disponibles->currentItem();
    if (!item) {
        return;
    }

    const int id = item->data(Qt::UserRole).toInt();
    Seleccion seleccion;
    seleccion.idMateria = id;
    seleccion.horas = HORAS_POR_DEFECTO;
    seleccion.curso = 1;
    seleccion.nombre = nombreMateriaDe(m_materias, id);
    if (seleccion.nombre.isEmpty()) {
        seleccion.nombre = item->text();
    }

    agregarFilaSeleccionada(seleccion);
    refrescarDisponibles();
    actualizarTotales();
}

void PlanFormDialog::agregarPorDobleClic() {
    agregarMateriaSeleccionada();
}

void PlanFormDialog::quitarMateriaSeleccionada() {
    const int fila = m_seleccionadas->currentRow();
    if (fila < 0) {
        return;
    }

    m_seleccionadas->removeRow(fila);
    refrescarDisponibles();
    actualizarTotales();
}

void PlanFormDialog::agregarFilaSeleccionada(const Seleccion& seleccion) {
    const int fila = m_seleccionadas->rowCount();
    m_seleccionadas->insertRow(fila);

    auto* item = new QTableWidgetItem(seleccion.nombre);
    item->setData(Qt::UserRole, seleccion.idMateria);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    m_seleccionadas->setItem(fila, 0, item);

    auto* horas = new QSpinBox(m_seleccionadas);
    horas->setRange(1, HORAS_MAXIMAS_POR_MATERIA);
    horas->setValue(seleccion.horas > 0 ? seleccion.horas : HORAS_POR_DEFECTO);
    horas->setAlignment(Qt::AlignCenter);
    connect(horas, qOverload<int>(&QSpinBox::valueChanged),
            this, [this](int) { actualizarTotales(); });
    m_seleccionadas->setCellWidget(fila, 1, horas);

    auto* anio = new QSpinBox(m_seleccionadas);
    anio->setRange(CURSO_MINIMO, CURSO_MAXIMO);
    anio->setValue(seleccion.curso >= CURSO_MINIMO && seleccion.curso <= CURSO_MAXIMO
                       ? seleccion.curso
                       : CURSO_MINIMO);
    anio->setAlignment(Qt::AlignCenter);
    anio->setEnabled(!esGlobal());
    connect(anio, qOverload<int>(&QSpinBox::valueChanged),
            this, [this](int) { actualizarTotales(); });
    m_seleccionadas->setCellWidget(fila, 2, anio);

    m_seleccionadas->setColumnHidden(2, esGlobal());
    actualizarTotales();
}

QVector<PlanFormDialog::Seleccion> PlanFormDialog::seleccionActual() const {
    QVector<Seleccion> seleccion;
    if (!m_seleccionadas) {
        return seleccion;
    }

    const bool global = esGlobal();
    for (int fila = 0; fila < m_seleccionadas->rowCount(); ++fila) {
        QTableWidgetItem* item = m_seleccionadas->item(fila, 0);
        if (!item) {
            continue;
        }

        Seleccion actual;
        actual.idMateria = item->data(Qt::UserRole).toInt();
        actual.nombre = item->text();

        if (auto* horas = qobject_cast<QSpinBox*>(
                m_seleccionadas->cellWidget(fila, 1))) {
            actual.horas = horas->value();
        }

        if (global) {
            actual.curso = 1;
        } else if (auto* anio = qobject_cast<QSpinBox*>(
                       m_seleccionadas->cellWidget(fila, 2))) {
            actual.curso = anio->value();
        }

        seleccion.append(actual);
    }

    return seleccion;
}

bool PlanFormDialog::esGlobal() const {
    return m_global && m_global->isChecked();
}

void PlanFormDialog::alternarModoGlobal(bool global) {
    if (!m_seleccionadas) {
        return;
    }

    m_seleccionadas->setColumnHidden(2, global);
    for (int fila = 0; fila < m_seleccionadas->rowCount(); ++fila) {
        if (auto* anio = qobject_cast<QSpinBox*>(
                m_seleccionadas->cellWidget(fila, 2))) {
            anio->setEnabled(!global);
            if (global) {
                anio->setValue(CURSO_MINIMO);
            }
        }
    }

    actualizarTotales();
}

void PlanFormDialog::actualizarTotales() {
    if (!m_horasTotales) {
        return;
    }

    const bool isDark = qApp->property("isDarkMode").toBool();
    const QVector<Seleccion> seleccion = seleccionActual();

    int total = 0;
    bool excede = false;
    QString texto;

    if (esGlobal()) {
        for (const Seleccion& item : seleccion) {
            total += item.horas;
        }
        excede = total > MAX_HORAS_SEMANALES_POR_ANIO;
        texto = QStringLiteral("Horas totales: %1 h/semana (se repiten en 1.º–5.º)")
                    .arg(total);
    } else {
        QMap<int, int> porAnio;
        for (const Seleccion& item : seleccion) {
            porAnio[item.curso] += item.horas;
            total += item.horas;
        }

        QStringList partes;
        for (auto it = porAnio.constBegin(); it != porAnio.constEnd(); ++it) {
            partes << QStringLiteral("%1.º: %2 h").arg(it.key()).arg(it.value());
            if (it.value() > MAX_HORAS_SEMANALES_POR_ANIO) {
                excede = true;
            }
        }

        texto = QStringLiteral("Horas totales: %1 h (suma por año) · %2")
                    .arg(total)
                    .arg(partes.join(QStringLiteral(" · ")));
    }

    m_horasTotales->setText(texto);
    m_horasTotales->setStyleSheet(
        QStringLiteral("font-size: 13px; font-weight: 700; color: %1;")
            .arg(excede ? QStringLiteral("#dc2626")
                        : (isDark ? QStringLiteral("#94a3b8") : QStringLiteral("#64748b"))));
}

bool PlanFormDialog::validarFormulario() {
    QStringList errores;

    if (!m_edicion) {
        const QString codigo = m_codigo->text().trimmed();
        if (codigo.isEmpty()) {
            errores << QStringLiteral("El código del plan es obligatorio.");
        } else if (!QRegularExpression(QStringLiteral("^\\d+$"))
                        .match(codigo).hasMatch()) {
            errores << QStringLiteral("El código del plan debe contener solo números.");
        }
    }

    if (m_nombre->text().trimmed().isEmpty()) {
        errores << QStringLiteral("El nombre del plan es obligatorio.");
    }

    const QVector<Seleccion> seleccion = seleccionActual();
    if (seleccion.isEmpty()) {
        errores << QStringLiteral("Seleccione al menos una materia para el plan.");
    }

    QSet<int> ids;
    QMap<int, int> porAnio;
    for (const Seleccion& item : seleccion) {
        if (ids.contains(item.idMateria)) {
            errores << QStringLiteral("No se puede repetir la misma materia en el plan.");
        }
        ids.insert(item.idMateria);

        if (item.horas <= 0) {
            errores << QStringLiteral("Las horas semanales deben ser mayores a 0.");
        }
        porAnio[item.curso] += item.horas;
    }

    for (auto it = porAnio.constBegin(); it != porAnio.constEnd(); ++it) {
        if (it.value() > MAX_HORAS_SEMANALES_POR_ANIO) {
            errores << QStringLiteral("El año %1 supera el máximo de %2 horas semanales.")
                           .arg(it.key())
                           .arg(MAX_HORAS_SEMANALES_POR_ANIO);
        }
    }

    if (!errores.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("No se puede guardar el plan"),
                             errores.join(QStringLiteral("\n")));
        return false;
    }

    return true;
}

void PlanFormDialog::guardarPlan() {
    if (!validarFormulario()) {
        return;
    }
    if (!m_nucleo) {
        QMessageBox::warning(this, QStringLiteral("Sin conexión"),
                             QStringLiteral("No hay conexión con la base de datos."));
        return;
    }

    const QVector<Seleccion> seleccion = seleccionActual();
    QVector<PlanMateriaDTO> materias;
    materias.reserve(seleccion.size());
    for (const Seleccion& item : seleccion) {
        PlanMateriaDTO dto;
        dto.idMateria = item.idMateria;
        dto.curso = esGlobal() ? 1 : item.curso;
        dto.horas = item.horas;
        materias.append(dto);
    }

    const QString nombre = m_nombre->text().trimmed();
    const QString descripcion = m_descripcion->toPlainText().trimmed();

    Resultado<PlanDTO> resultado;
    if (m_edicion) {
        resultado = m_nucleo->actualizarPlanCompleto(
            m_codigoOriginal, nombre, descripcion, materias);
    } else {
        resultado = m_nucleo->crearPlanCompleto(
            m_codigo->text().trimmed(), nombre, descripcion, materias);
    }

    if (!resultado.ok) {
        QMessageBox::warning(this, QStringLiteral("No se pudo guardar el plan"),
                             resultado.mensajeError);
        return;
    }

    emit planGuardado();
    accept();
}

} // namespace gestor::frontend::forms
