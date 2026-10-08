#include "teacher_form_dialog.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QApplication>

namespace gestor::frontend::forms {

TeacherFormDialog::TeacherFormDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Registrar Docente");
    resize(450, 350);

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

    campoCedula = new QLineEdit(this);
    campoCedula->setStyleSheet(inputStyle);
    campoCedula->setPlaceholderText("Ej. 12345678");

    campoId = new QLineEdit(this);
    campoId->setStyleSheet(inputStyle);
    campoId->setPlaceholderText("ID interno (ej. T001)");

    campoNombre = new QLineEdit(this);
    campoNombre->setStyleSheet(inputStyle);
    campoNombre->setPlaceholderText("Nombre completo");

    campoEmail = new QLineEdit(this);
    campoEmail->setStyleSheet(inputStyle);
    campoEmail->setPlaceholderText("correo@dominio.com");

    campoTelefono = new QLineEdit(this);
    campoTelefono->setStyleSheet(inputStyle);
    campoTelefono->setPlaceholderText("Opcional: 0412-1234567");

    botonCancelar = new QPushButton("Cancelar", this);
    botonCancelar->setStyleSheet(isDark ?
                                     "QPushButton { background-color: #2a2a2a; border: 1px solid #444444; border-radius: 6px; padding: 10px 20px; color: #ffffff; font-weight: bold; } QPushButton:hover { background-color: #333333; }" :
                                     "QPushButton { background-color: white; border: 1px solid #d1d5db; border-radius: 6px; padding: 10px 20px; color: #374151; font-weight: bold; } QPushButton:hover { background-color: #f3f4f6; }"
                                 );
    botonCancelar->setCursor(Qt::PointingHandCursor);

    botonGuardar = new QPushButton("Guardar", this);
    botonGuardar->setStyleSheet(isDark ?
                                    "QPushButton { background-color: #3b82f6; border: none; border-radius: 6px; padding: 10px 20px; color: white; font-weight: bold; } QPushButton:hover { background-color: #2563eb; }" :
                                    "QPushButton { background-color: #1a237e; border: none; border-radius: 6px; padding: 10px 20px; color: white; font-weight: bold; } QPushButton:hover { background-color: #283593; }"
                                );
    botonGuardar->setCursor(Qt::PointingHandCursor);

    // Validadores
    QRegularExpression regexEmail("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,4}$");
    campoEmail->setValidator(new QRegularExpressionValidator(regexEmail, this));
    QRegularExpression regexTelefono("^[0-9+()\\- ]+$");
    campoTelefono->setValidator(new QRegularExpressionValidator(regexTelefono, this));
    campoCedula->setValidator(new QRegularExpressionValidator(regexTelefono, this));

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(15);

    QString labelStyle = QString("<span style='font-weight:bold; color:%1; font-size: 13px;'>").arg(labelColor);
    formLayout->addRow(labelStyle + "Cédula:</span>", campoCedula);
    formLayout->addRow(labelStyle + "ID:</span>", campoId);
    formLayout->addRow(labelStyle + "Nombre:</span>", campoNombre);
    formLayout->addRow(labelStyle + "Email:</span>", campoEmail);
    formLayout->addRow(labelStyle + "Teléfono:</span>", campoTelefono);

    QHBoxLayout *botonesLayout = new QHBoxLayout();
    botonesLayout->addStretch();
    botonesLayout->addWidget(botonCancelar);
    botonesLayout->addWidget(botonGuardar);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(25, 25, 25, 25);
    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();
    mainLayout->addLayout(botonesLayout);

    connect(botonGuardar, &QPushButton::clicked, this, &TeacherFormDialog::guardarProfesor);
    connect(botonCancelar, &QPushButton::clicked, this, &QDialog::reject);
}

bool TeacherFormDialog::validarCampos() {
    bool ok = true;
    QString errores;

    // Restaurar estilos (manteniendo el color base según el tema)
    bool isDark = qApp->property("isDarkMode").toBool();
    QString baseInputStyle = QString(
                                 "QLineEdit { border: 1px solid %1; border-radius: 6px; padding: 8px; "
                                 "background-color: %2; color: %3; font-size: 13px; }"
                                 ).arg(isDark ? "#444444" : "#d1d5db", isDark ? "#2a2a2a" : "white", isDark ? "#ffffff" : "#374151");

    campoCedula->setStyleSheet(baseInputStyle);
    campoId->setStyleSheet(baseInputStyle);
    campoNombre->setStyleSheet(baseInputStyle);
    campoEmail->setStyleSheet(baseInputStyle);
    campoTelefono->setStyleSheet(baseInputStyle);

    if (campoCedula->text().trimmed().isEmpty()) {
        errores += "• Cédula es obligatoria.\n";
        campoCedula->setStyleSheet(baseInputStyle + " QLineEdit { border: 2px solid #dc2626; }");
        ok = false;
    }
    if (campoId->text().trimmed().isEmpty()) {
        errores += "• ID es obligatorio.\n";
        campoId->setStyleSheet(baseInputStyle + " QLineEdit { border: 2px solid #dc2626; }");
        ok = false;
    }
    if (campoNombre->text().trimmed().isEmpty()) {
        errores += "• Nombre es obligatorio.\n";
        campoNombre->setStyleSheet(baseInputStyle + " QLineEdit { border: 2px solid #dc2626; }");
        ok = false;
    }
    if (campoEmail->text().trimmed().isEmpty()) {
        errores += "• Email es obligatorio.\n";
        campoEmail->setStyleSheet(baseInputStyle + " QLineEdit { border: 2px solid #dc2626; }");
        ok = false;
    } else if (!campoEmail->hasAcceptableInput()) {
        errores += "• Formato de email inválido.\n";
        campoEmail->setStyleSheet(baseInputStyle + " QLineEdit { border: 2px solid #dc2626; }");
        ok = false;
    }
    if (!campoTelefono->text().trimmed().isEmpty() && !campoTelefono->hasAcceptableInput()) {
        errores += "• Formato de teléfono inválido (use solo dígitos, espacios, +, -, (, )).\n";
        campoTelefono->setStyleSheet(baseInputStyle + " QLineEdit { border: 2px solid #dc2626; }");
        ok = false;
    }

    if (!ok) {
        QMessageBox::warning(this, "Error de Validación",
                             "Por favor corrija los siguientes errores:\n\n" + errores);
    }
    return ok;
}

void TeacherFormDialog::guardarProfesor() {
    if (!validarCampos())
        return;

    emit profesorGuardado(
        campoCedula->text().trimmed(),
        campoId->text().trimmed(),
        campoNombre->text().trimmed(),
        campoEmail->text().trimmed(),
        campoTelefono->text().trimmed()
        );
    accept();
}

void TeacherFormDialog::cargarDatos(const QString& cedula, const QString& id,
                                    const QString& nombre, const QString& email,
                                    const QString& telefono) {
    campoCedula->setText(cedula);
    campoId->setText(id);
    campoNombre->setText(nombre);
    campoEmail->setText(email);
    campoTelefono->setText(telefono);
}

} // namespace gestor::frontend::forms