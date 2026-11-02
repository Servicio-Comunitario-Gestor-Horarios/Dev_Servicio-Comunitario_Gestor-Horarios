#include "teacher_form_dialog.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QApplication>

namespace gestor::frontend::forms {

TeacherFormDialog::TeacherFormDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Registrar Docente");
    resize(450, 460);

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
                             "QLineEdit, QComboBox { border: 1px solid %1; border-radius: 6px; padding: 8px; "
                             "background-color: %2; color: %3; font-size: 13px; }"
                             "QLineEdit::placeholder { color: %4; }"
                             "QLineEdit.error { border: 2px solid #dc2626; }"
                             ).arg(inputBorder, inputBg, textColor, placeholderColor);

    campoCedula = new QLineEdit(this);
    campoCedula->setStyleSheet(inputStyle);
    campoCedula->setPlaceholderText("Ej. 12345678");

    campoNombre = new QLineEdit(this);
    campoNombre->setStyleSheet(inputStyle);
    campoNombre->setPlaceholderText("Nombre completo");

    campoEmail = new QLineEdit(this);
    campoEmail->setStyleSheet(inputStyle);
    campoEmail->setPlaceholderText("correo@dominio.com");

    campoTelefono = new QLineEdit(this);
    campoTelefono->setStyleSheet(inputStyle);
    campoTelefono->setPlaceholderText("Opcional: 0412-1234567");

    // NUEVO: Campo de Materias
    campoMaterias = new QLineEdit(this);
    campoMaterias->setStyleSheet(inputStyle);
    campoMaterias->setPlaceholderText("Ej. Física, Matemáticas");

    // Configuración opcional del generador (no se muestra en la tabla).
    comboTurno = new QComboBox(this);
    comboTurno->setStyleSheet(inputStyle);
    comboTurno->addItem(QStringLiteral("— Sin preferencia —"), QString());

    comboPlan = new QComboBox(this);
    comboPlan->setStyleSheet(inputStyle);
    comboPlan->addItem(QStringLiteral("— Sin preferencia —"), QString());

    comboAnio = new QComboBox(this);
    comboAnio->setStyleSheet(inputStyle);
    comboAnio->addItem(QStringLiteral("Todos"), 0);

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
    formLayout->addRow(labelStyle + "Cédula / ID:</span>", campoCedula);
    formLayout->addRow(labelStyle + "Nombre:</span>", campoNombre);
    formLayout->addRow(labelStyle + "Email:</span>", campoEmail);
    formLayout->addRow(labelStyle + "Teléfono:</span>", campoTelefono);
    formLayout->addRow(labelStyle + "Materias:</span>", campoMaterias);
    formLayout->addRow(labelStyle + "Turno (opcional):</span>", comboTurno);
    formLayout->addRow(labelStyle + "Plan de estudio (opcional):</span>", comboPlan);
    formLayout->addRow(labelStyle + "Año preferencial (opcional):</span>", comboAnio);

    QLabel *ayuda = new QLabel(
        "Turno, plan y año solo se usan como valores iniciales de la "
        "configuración del generador de horarios.", this);
    ayuda->setWordWrap(true);
    ayuda->setStyleSheet(QString("color: %1; font-size: 11px;")
                             .arg(isDark ? "#9ca3af" : "#6b7280"));

    QHBoxLayout *botonesLayout = new QHBoxLayout();
    botonesLayout->addStretch();
    botonesLayout->addWidget(botonCancelar);
    botonesLayout->addWidget(botonGuardar);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(25, 25, 25, 25);
    mainLayout->addLayout(formLayout);
    mainLayout->addWidget(ayuda);
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
    campoNombre->setStyleSheet(baseInputStyle);
    campoEmail->setStyleSheet(baseInputStyle);
    campoTelefono->setStyleSheet(baseInputStyle);
    campoMaterias->setStyleSheet(baseInputStyle);

    if (campoCedula->text().trimmed().isEmpty()) {
        errores += "• Cédula es obligatoria.\n";
        campoCedula->setStyleSheet(baseInputStyle + " QLineEdit { border: 2px solid #dc2626; }");
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
        campoNombre->text().trimmed(),
        campoEmail->text().trimmed(),
        campoTelefono->text().trimmed(),
        campoMaterias->text().trimmed()
        );
    accept();
}

void TeacherFormDialog::cargarDatos(const QString& cedula, const QString& nombre,
                                    const QString& email, const QString& telefono,
                                    const QString& materias) {
    campoCedula->setText(cedula);
    campoNombre->setText(nombre);
    campoEmail->setText(email);
    campoTelefono->setText(telefono);
    campoMaterias->setText(materias);
}

void TeacherFormDialog::cargarOpcionesGeneracion(const QStringList& turnos,
                                                 const QStringList& planes,
                                                 const QVector<int>& anios)
{
    const QString sinPreferencia = QStringLiteral("— Sin preferencia —");

    comboTurno->clear();
    comboTurno->addItem(sinPreferencia, QString());
    for (const QString& turno : turnos)
        comboTurno->addItem(turno, turno);

    comboPlan->clear();
    comboPlan->addItem(sinPreferencia, QString());
    for (const QString& plan : planes)
        comboPlan->addItem(plan, plan);

    comboAnio->clear();
    comboAnio->addItem(QStringLiteral("Todos"), 0);
    for (int anio : anios)
        comboAnio->addItem(QString::number(anio), anio);
}

void TeacherFormDialog::cargarPreferencias(const QString& turno, const QString& plan,
                                           int anio)
{
    if (!turno.isEmpty()) {
        const int indice = comboTurno->findData(turno);
        if (indice >= 0)
            comboTurno->setCurrentIndex(indice);
    }
    if (!plan.isEmpty()) {
        const int indice = comboPlan->findData(plan);
        if (indice >= 0)
            comboPlan->setCurrentIndex(indice);
    }
    if (anio > 0) {
        const int indice = comboAnio->findData(anio);
        if (indice >= 0)
            comboAnio->setCurrentIndex(indice);
    }
}

QString TeacherFormDialog::cedula() const {
    return campoCedula->text().trimmed();
}

QString TeacherFormDialog::turnoSeleccionado() const {
    return comboTurno->currentData().toString();
}

QString TeacherFormDialog::planSeleccionado() const {
    return comboPlan->currentData().toString();
}

int TeacherFormDialog::anioSeleccionado() const {
    return comboAnio->currentData().toInt();
}

} // namespace gestor::frontend::forms