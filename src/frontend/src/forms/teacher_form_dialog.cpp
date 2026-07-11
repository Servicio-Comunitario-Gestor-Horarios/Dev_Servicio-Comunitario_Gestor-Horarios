#include "teacher_form_dialog.hpp"
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QRegularExpressionValidator>

namespace gestor::frontend::forms {

TeacherFormDialog::TeacherFormDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Registrar Docente");
    resize(450, 300);
    setStyleSheet("QDialog { background-color: white; }");

    QString inputStyle = "QLineEdit { border: 1px solid #d1d5db; border-radius: 6px; padding: 8px; background-color: white; color: #374151; font-size: 13px; }"
                         "QLineEdit::placeholder { color: #9ca3af; }";

    // Inicializando los punteros exactos
    campoId = new QLineEdit(this);
    campoId->setStyleSheet(inputStyle);

    campoNombre = new QLineEdit(this);
    campoNombre->setStyleSheet(inputStyle);

    campoEmail = new QLineEdit(this);
    campoEmail->setStyleSheet(inputStyle);

    campoTelefono = new QLineEdit(this);
    campoTelefono->setStyleSheet(inputStyle);

    campoMaterias = new QLineEdit(this);
    campoMaterias->setStyleSheet(inputStyle);
    campoMaterias->setPlaceholderText("Ej. Matemáticas, Física");

    botonCancelar = new QPushButton("Cancelar", this);
    botonCancelar->setStyleSheet("QPushButton { background-color: white; border: 1px solid #d1d5db; border-radius: 6px; padding: 10px 20px; color: #374151; font-weight: bold; } QPushButton:hover { background-color: #f3f4f6; }");
    botonCancelar->setCursor(Qt::PointingHandCursor);

    botonGuardar = new QPushButton("Guardar", this);
    botonGuardar->setStyleSheet("QPushButton { background-color: #1a237e; border: none; border-radius: 6px; padding: 10px 20px; color: white; font-weight: bold; } QPushButton:hover { background-color: #283593; }");
    botonGuardar->setCursor(Qt::PointingHandCursor);

    configurarValidadores();

    // Creando la vista
    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(15);

    QString labelStyle = "<span style='font-weight:bold; color:#111827; font-size: 13px;'>";
    formLayout->addRow(labelStyle + "Cédula/ID:</span>", campoId);
    formLayout->addRow(labelStyle + "Nombre:</span>", campoNombre);
    formLayout->addRow(labelStyle + "Email:</span>", campoEmail);
    formLayout->addRow(labelStyle + "Teléfono:</span>", campoTelefono);
    formLayout->addRow(labelStyle + "Materias:</span>", campoMaterias);

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

void TeacherFormDialog::configurarValidadores() {
    QRegularExpression regexEmail("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,4}$");
    campoEmail->setValidator(new QRegularExpressionValidator(regexEmail, this));
}

void TeacherFormDialog::guardarProfesor() {
    // TF4: Validación de email
    if (!campoEmail->hasAcceptableInput() && !campoEmail->text().isEmpty()) {
        QMessageBox::warning(this, "Error de Validación", "El formato del email es incorrecto.");
        return;
    }

    // TF3 y TF5: Simulación de InternalClient y manejo de servidor caído
    bool servidorDisponible = false; // TODO: Conectar con backend real

    if (!servidorDisponible) {
        // TF5: QMessageBox de error si el servidor no responde
        QMessageBox::critical(this, "Error de Conexión", "El servidor (InternalClient) no está disponible en este momento.");

        // NOTA: Para que puedas seguir probando la interfaz visualmente,
        // no vamos a detener la ejecución aquí (return;), pero en el
        // futuro deberías descomentar el return para que no se guarde si falla.
        // return;
    } else {
        // TF3: Aquí iría la llamada real IPC (OP_CREAR_PROFESOR)
        // InternalClient::getInstance()->sendRequest("OP_CREAR_PROFESOR", datos);
    }

    emit profesorGuardado(campoId->text(), campoNombre->text(), campoEmail->text(), campoTelefono->text(), campoMaterias->text());
    accept();
}
void TeacherFormDialog::cargarDatos(const QString& id, const QString& nombre, const QString& email, const QString& telefono, const QString& materias) {
    campoId->setText(id);
    campoNombre->setText(nombre);
    campoEmail->setText(email != "N/A" ? email : "");
    campoTelefono->setText(telefono != "N/A" ? telefono : "");
    campoMaterias->setText(materias != "N/A" ? materias : "");
}

} // namespace gestor::frontend::forms