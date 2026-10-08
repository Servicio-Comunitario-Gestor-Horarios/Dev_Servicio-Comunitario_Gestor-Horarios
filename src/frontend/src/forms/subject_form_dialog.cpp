/**
 * @file subject_form_dialog.cpp
 * @brief Implementación del formulario CRUD de asignaturas con soporte para modo oscuro.
 */

#include "subject_form_dialog.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QApplication>

namespace gestor::frontend::forms {

SubjectFormDialog::SubjectFormDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Registrar Asignatura");
    resize(400, 200);

    // Detectar si el modo oscuro está activo
    bool isDark = qApp->property("isDarkMode").toBool();

    QString bgColor = isDark ? "#1e1e1e" : "white";
    QString textColor = isDark ? "#ffffff" : "#374151";
    QString inputBg = isDark ? "#2a2a2a" : "white";
    QString inputBorder = isDark ? "#444444" : "#d1d5db";
    QString placeholderColor = isDark ? "#888888" : "#9ca3af";
    QString labelColor = isDark ? "#ffffff" : "#111827";

    setStyleSheet(QString("QDialog { background-color: %1; color: %2; }").arg(bgColor, textColor));

    QString inputStyle = QString(
                             "QLineEdit { border: 1px solid %1; border-radius: 6px; padding: 8px; "
                             "background-color: %2; color: %3; font-size: 13px; }"
                             "QLineEdit::placeholder { color: %4; }"
                             "QLineEdit.error { border: 2px solid #dc2626; }"
                             ).arg(inputBorder, inputBg, textColor, placeholderColor);

    m_campoNombre = new QLineEdit(this);
    m_campoNombre->setStyleSheet(inputStyle);
    m_campoNombre->setPlaceholderText("Nombre de la asignatura");

    m_comboTipoAula = new QComboBox(this);
    m_comboTipoAula->addItems({"Aula de estudio", "Cancha", "Laboratorio"});
    m_comboTipoAula->setStyleSheet(QString(
                                       "QComboBox {"
                                       "    border: 1px solid %1;"
                                       "    border-radius: 6px;"
                                       "    padding: 8px;"
                                       "    background-color: %2;"
                                       "    color: %3;"
                                       "    font-size: 13px;"
                                       "}"
                                       "QComboBox::drop-down {"
                                       "    border: none;"
                                       "    width: 20px;"
                                       "}"
                                       "QComboBox::down-arrow {"
                                       "    image: url(:/flecha.png);"
                                       "    width: 12px;"
                                       "    height: 12px;"
                                       "}"
                                       "QComboBox:hover {"
                                       "    border-color: #9ca3af;"
                                       "}"
                                       "QComboBox:focus {"
                                       "    border-color: #3b82f6;"
                                       "    outline: none;"
                                       "}"
                                       ).arg(inputBorder, inputBg, textColor));

    m_botonCancelar = new QPushButton("Cancelar", this);
    m_botonCancelar->setStyleSheet(isDark ?
                                       "QPushButton { background-color: #2a2a2a; border: 1px solid #444444; border-radius: 6px; padding: 10px 20px; color: #ffffff; font-weight: bold; } QPushButton:hover { background-color: #333333; }" :
                                       "QPushButton { background-color: white; border: 1px solid #d1d5db; border-radius: 6px; padding: 10px 20px; color: #374151; font-weight: bold; } QPushButton:hover { background-color: #f3f4f6; }"
                                   );
    m_botonCancelar->setCursor(Qt::PointingHandCursor);

    m_botonGuardar = new QPushButton("Guardar", this);
    m_botonGuardar->setStyleSheet(isDark ?
                                      "QPushButton { background-color: #3b82f6; border: none; border-radius: 6px; padding: 10px 20px; color: white; font-weight: bold; } QPushButton:hover { background-color: #2563eb; }" :
                                      "QPushButton { background-color: #1a237e; border: none; border-radius: 6px; padding: 10px 20px; color: white; font-weight: bold; } QPushButton:hover { background-color: #283593; }"
                                  );
    m_botonGuardar->setCursor(Qt::PointingHandCursor);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(15);
    QString labelStyle = QString("<span style='font-weight:bold; color:%1; font-size: 13px;'>").arg(labelColor);

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

    bool isDark = qApp->property("isDarkMode").toBool();
    QString baseInputStyle = QString(
                                 "QLineEdit { border: 1px solid %1; border-radius: 6px; padding: 8px; "
                                 "background-color: %2; color: %3; font-size: 13px; }"
                                 ).arg(isDark ? "#444444" : "#d1d5db", isDark ? "#2a2a2a" : "white", isDark ? "#ffffff" : "#374151");

    m_campoNombre->setStyleSheet(baseInputStyle);
    if (m_campoNombre->text().trimmed().isEmpty()) {
        errores += "• El nombre de la asignatura es obligatorio.\n";
        m_campoNombre->setStyleSheet(baseInputStyle + " QLineEdit { border: 2px solid #dc2626; }");
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

    emit materiaGuardada(m_campoNombre->text().trimmed(), m_comboTipoAula->currentText());
    accept();
}

void SubjectFormDialog::cargarDatos(const QString& nombre, const QString& tipoAula) {
    m_campoNombre->setText(nombre);
    int index = m_comboTipoAula->findText(tipoAula);
    if (index >= 0) m_comboTipoAula->setCurrentIndex(index);
}

} // namespace gestor::frontend::forms