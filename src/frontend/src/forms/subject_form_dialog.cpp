#include "subject_form_dialog.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QMessageBox>

namespace gestor::frontend::forms {

SubjectFormDialog::SubjectFormDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Registrar Asignatura");
    resize(400, 200);
    setStyleSheet("QDialog { background-color: white; }");

    QString inputStyle =
        "QLineEdit { border: 1px solid #d1d5db; border-radius: 6px; padding: 8px; "
        "background-color: white; color: #374151; font-size: 13px; }"
        "QLineEdit::placeholder { color: #9ca3af; }"
        "QLineEdit.error { border: 2px solid #dc2626; }";

    m_campoNombre = new QLineEdit(this);
    m_campoNombre->setStyleSheet(inputStyle);
    m_campoNombre->setPlaceholderText("Nombre de la asignatura");

    m_comboTipoAula = new QComboBox(this);
    m_comboTipoAula->addItems({"Aula de estudio", "Cancha", "Laboratorio"});
    m_comboTipoAula->setStyleSheet(
        "QComboBox {"
        "   border: 1px solid #d1d5db;"
        "   border-radius: 6px;"
        "   padding: 8px;"
        "   background-color: white;"
        "   color: #374151;"
        "   font-size: 13px;"
        "}"
        "QComboBox::drop-down {"
        "   border: none;"
        "   width: 20px;"
        "}"
        "QComboBox::down-arrow {"
        "   image: url(:/flecha.png);"
        "   width: 12px;"
        "   height: 12px;"
        "}"
        "QComboBox:hover {"
        "   border-color: #9ca3af;"
        "}"
        "QComboBox:focus {"
        "   border-color: #1a237e;"
        "   outline: none;"
        "}"
        );

    m_botonCancelar = new QPushButton("Cancelar", this);
    m_botonCancelar->setStyleSheet(
        "QPushButton { background-color: white; border: 1px solid #d1d5db; "
        "border-radius: 6px; padding: 10px 20px; color: #374151; font-weight: bold; } "
        "QPushButton:hover { background-color: #f3f4f6; }");
    m_botonCancelar->setCursor(Qt::PointingHandCursor);

    m_botonGuardar = new QPushButton("Guardar", this);
    m_botonGuardar->setStyleSheet(
        "QPushButton { background-color: #1a237e; border: none; border-radius: 6px; "
        "padding: 10px 20px; color: white; font-weight: bold; } "
        "QPushButton:hover { background-color: #283593; }");
    m_botonGuardar->setCursor(Qt::PointingHandCursor);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(15);
    QString labelStyle = "<span style='font-weight:bold; color:#111827; font-size: 13px;'>";
    formLayout->addRow(labelStyle + "Nombre:</span>", m_campoNombre);
    formLayout->addRow(labelStyle + "Tipo de aula:</span>", m_comboTipoAula);

    QHBoxLayout *botonesLayout = new QHBoxLayout();
    botonesLayout->addStretch();
    botonesLayout->addWidget(m_botonCancelar);
    botonesLayout->addWidget(m_botonGuardar);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(25, 25, 25, 25);
    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();
    mainLayout->addLayout(botonesLayout);

    connect(m_botonGuardar, &QPushButton::clicked, this, &SubjectFormDialog::guardarMateria);
    connect(m_botonCancelar, &QPushButton::clicked, this, &QDialog::reject);
}

bool SubjectFormDialog::validarCampos() {
    bool ok = true;
    QString errores;

    m_campoNombre->setStyleSheet(m_campoNombre->styleSheet().replace("error", ""));
    if (m_campoNombre->text().trimmed().isEmpty()) {
        errores += "• El nombre de la asignatura es obligatorio.\n";
        m_campoNombre->setStyleSheet(m_campoNombre->styleSheet() + "border: 2px solid #dc2626;");
        ok = false;
    }

    if (!ok) {
        QMessageBox::warning(this, "Error de Validación",
                             "Por favor corrija los siguientes errores:\n\n" + errores);
    }
    return ok;
}

void SubjectFormDialog::guardarMateria() {
    if (!validarCampos()) return;

    // Stub: aquí se integraría la llamada al middleware (OP_CREAR_MATERIA, etc.)
    emit materiaGuardada(m_campoNombre->text().trimmed(), m_comboTipoAula->currentText());
    accept();
}

void SubjectFormDialog::cargarDatos(const QString& nombre, const QString& tipoAula) {
    m_campoNombre->setText(nombre);
    int index = m_comboTipoAula->findText(tipoAula);
    if (index >= 0) m_comboTipoAula->setCurrentIndex(index);
}

} // namespace gestor::frontend::forms