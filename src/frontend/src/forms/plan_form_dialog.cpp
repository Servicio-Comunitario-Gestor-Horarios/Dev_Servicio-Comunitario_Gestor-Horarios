#include "plan_form_dialog.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QApplication>
#include <QLabel>
#include <QRegularExpressionValidator>

namespace gestor::frontend::forms {

PlanFormDialog::PlanFormDialog(const QStringList& materiasTotales, QWidget *parent) : QDialog(parent) {
    setWindowTitle("Gestión de Plan de Estudio");
    resize(600, 500);

    bool isDark = qApp->property("isDarkMode").toBool();

    QString bgColor = isDark ? "#1e1e1e" : "white";
    QString textColor = isDark ? "#ffffff" : "#374151";
    QString inputBg = isDark ? "#2a2a2a" : "white";
    QString inputBorder = isDark ? "#444444" : "#d1d5db";
    QString labelColor = isDark ? "#ffffff" : "#111827";

    setStyleSheet(QString("QDialog { background-color: %1; color: %2; }").arg(bgColor, textColor));

    QString inputStyle = QString(
                             "QWidget { border: 1px solid %1; border-radius: 6px; padding: 6px; "
                             "background-color: %2; color: %3; font-size: 13px; }"
                             "QWidget:focus { border: 1px solid #3b82f6; }"
                             // --- INICIO NUEVO ESTILO DE SELECCIÓN ---
                             "QListWidget::item:selected { "
                             "    background-color: #3b82f6; "
                             "    color: white; "
                             "    border-radius: 4px; "
                             "}"
                             "QListWidget::item:hover { "
                             "    background-color: #444444; "
                             "}"
                             // --- FIN NUEVO ESTILO ---
                             ).arg(inputBorder, inputBg, textColor);

    // Campos básicos
    m_campoCodigo = new QLineEdit(this);
    m_campoCodigo->setStyleSheet(inputStyle);
    m_campoCodigo->setPlaceholderText("Ej: 202601");
    // Validador estricto: Solo números
    QRegularExpression rx("^[0-9]+$");
    m_campoCodigo->setValidator(new QRegularExpressionValidator(rx, this));

    m_campoNombre = new QLineEdit(this);
    m_campoNombre->setStyleSheet(inputStyle);
    m_campoNombre->setPlaceholderText("Nombre del plan");

    m_campoDescripcion = new QTextEdit(this);
    m_campoDescripcion->setStyleSheet(inputStyle);
    m_campoDescripcion->setPlaceholderText("Descripción breve...");
    m_campoDescripcion->setFixedHeight(60);

    // Listas de materias
    m_listaDisponibles = new QListWidget(this);
    m_listaDisponibles->setStyleSheet(inputStyle);
    m_listaDisponibles->addItems(materiasTotales);

    m_listaSeleccionadas = new QListWidget(this);
    m_listaSeleccionadas->setStyleSheet(inputStyle);

    // Botones de traslado
    m_btnAgregar = new QPushButton(">>", this);
    m_btnQuitar = new QPushButton("<<", this);
    QString btnTransferStyle = QString("QPushButton { background-color: %1; border: 1px solid %2; border-radius: 4px; padding: 4px; color: %3; font-weight: bold; } QPushButton:hover { background-color: #3b82f6; color: white; border: none; }").arg(inputBg, inputBorder, textColor);
    m_btnAgregar->setStyleSheet(btnTransferStyle);
    m_btnQuitar->setStyleSheet(btnTransferStyle);
    m_btnAgregar->setCursor(Qt::PointingHandCursor);
    m_btnQuitar->setCursor(Qt::PointingHandCursor);

    QVBoxLayout *transferLayout = new QVBoxLayout();
    transferLayout->addStretch();
    transferLayout->addWidget(m_btnAgregar);
    transferLayout->addWidget(m_btnQuitar);
    transferLayout->addStretch();

    QHBoxLayout *listasLayout = new QHBoxLayout();

    QVBoxLayout *dispLayout = new QVBoxLayout();
    QLabel *lblDisp = new QLabel("Materias Disponibles:");
    lblDisp->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 12px;").arg(textColor));
    dispLayout->addWidget(lblDisp);
    dispLayout->addWidget(m_listaDisponibles);

    QVBoxLayout *selLayout = new QVBoxLayout();
    QLabel *lblSel = new QLabel("Materias del Plan:");
    lblSel->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 12px;").arg(textColor));
    selLayout->addWidget(lblSel);
    selLayout->addWidget(m_listaSeleccionadas);

    listasLayout->addLayout(dispLayout);
    listasLayout->addLayout(transferLayout);
    listasLayout->addLayout(selLayout);

    // Botones principales
    m_botonCancelar = new QPushButton("Cancelar", this);
    m_botonCancelar->setStyleSheet(isDark ?
                                       "QPushButton { background-color: #2a2a2a; border: 1px solid #444444; border-radius: 6px; padding: 10px 20px; color: #ffffff; font-weight: bold; } QPushButton:hover { background-color: #333333; }" :
                                       "QPushButton { background-color: white; border: 1px solid #d1d5db; border-radius: 6px; padding: 10px 20px; color: #374151; font-weight: bold; } QPushButton:hover { background-color: #f3f4f6; }");
    m_botonCancelar->setCursor(Qt::PointingHandCursor);

    m_botonGuardar = new QPushButton("Guardar", this);
    m_botonGuardar->setStyleSheet(isDark ?
                                      "QPushButton { background-color: #3b82f6; border: none; border-radius: 6px; padding: 10px 20px; color: white; font-weight: bold; } QPushButton:hover { background-color: #2563eb; }" :
                                      "QPushButton { background-color: #1a237e; border: none; border-radius: 6px; padding: 10px 20px; color: white; font-weight: bold; } QPushButton:hover { background-color: #283593; }");
    m_botonGuardar->setCursor(Qt::PointingHandCursor);

    QFormLayout *formLayout = new QFormLayout();
    QString labelStyle = QString("<span style='font-weight:bold; color:%1; font-size: 13px;'>").arg(labelColor);
    formLayout->addRow(labelStyle + "Código numérico:</span>", m_campoCodigo);
    formLayout->addRow(labelStyle + "Nombre del Plan:</span>", m_campoNombre);
    formLayout->addRow(labelStyle + "Descripción:</span>", m_campoDescripcion);

    QHBoxLayout *botonesLayout = new QHBoxLayout();
    botonesLayout->addStretch();
    botonesLayout->addWidget(m_botonCancelar);
    botonesLayout->addWidget(m_botonGuardar);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->addLayout(formLayout);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(listasLayout);
    mainLayout->addSpacing(15);
    mainLayout->addLayout(botonesLayout);

    connect(m_btnAgregar, &QPushButton::clicked, this, &PlanFormDialog::agregarMateria);
    connect(m_btnQuitar, &QPushButton::clicked, this, &PlanFormDialog::quitarMateria);
    connect(m_botonGuardar, &QPushButton::clicked, this, &PlanFormDialog::guardarPlan);
    connect(m_botonCancelar, &QPushButton::clicked, this, &QDialog::reject);
}

void PlanFormDialog::agregarMateria() {
    QList<QListWidgetItem*> seleccionados = m_listaDisponibles->selectedItems();
    for (QListWidgetItem* item : seleccionados) {
        m_listaSeleccionadas->addItem(item->text());
        delete m_listaDisponibles->takeItem(m_listaDisponibles->row(item));
    }
}

void PlanFormDialog::quitarMateria() {
    QList<QListWidgetItem*> seleccionados = m_listaSeleccionadas->selectedItems();
    for (QListWidgetItem* item : seleccionados) {
        m_listaDisponibles->addItem(item->text());
        delete m_listaSeleccionadas->takeItem(m_listaSeleccionadas->row(item));
    }
}

bool PlanFormDialog::validarCampos() {
    QString errores;
    bool ok = true;

    if (m_campoCodigo->text().trimmed().isEmpty()) {
        errores += "• El código del plan es obligatorio y debe ser solo numérico.\n";
        ok = false;
    }
    if (m_campoNombre->text().trimmed().isEmpty()) {
        errores += "• El nombre del plan es obligatorio.\n";
        ok = false;
    }
    if (m_listaSeleccionadas->count() == 0) {
        errores += "• Debe asignar al menos una materia al plan.\n";
        ok = false;
    }

    if (!ok) {
        QMessageBox::warning(this, "Error de Validación", "Por favor corrija los siguientes errores:\n\n" + errores);
    }
    return ok;
}

void PlanFormDialog::guardarPlan() {
    if (!validarCampos()) return;

    QStringList materiasAsignadas;
    for (int i = 0; i < m_listaSeleccionadas->count(); ++i) {
        materiasAsignadas << m_listaSeleccionadas->item(i)->text();
    }

    emit planGuardado(m_campoCodigo->text().trimmed(),
                      m_campoNombre->text().trimmed(),
                      m_campoDescripcion->toPlainText().trimmed(),
                      materiasAsignadas);
    accept();
}

void PlanFormDialog::cargarDatos(const QString& codigo, const QString& nombre, const QString& descripcion, const QStringList& materiasSeleccionadas) {
    m_campoCodigo->setText(codigo);
    m_campoNombre->setText(nombre);
    m_campoDescripcion->setText(descripcion);

    for (const QString& materia : materiasSeleccionadas) {
        QList<QListWidgetItem*> items = m_listaDisponibles->findItems(materia, Qt::MatchExactly);
        if (!items.isEmpty()) {
            delete m_listaDisponibles->takeItem(m_listaDisponibles->row(items.first()));
        }
        m_listaSeleccionadas->addItem(materia);
    }
}

} // namespace gestor::frontend::forms
