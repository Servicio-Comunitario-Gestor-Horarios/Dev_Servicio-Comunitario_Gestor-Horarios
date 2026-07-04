#include "login_dialog.hpp"
#include <QMessageBox>
#include <QTimer>

namespace gestor::frontend::dialogs {

    LoginDialog::LoginDialog(QWidget *parent)
        : QDialog(parent)
    {
        setWindowTitle("Iniciar Sesión - Gestor Horarios");
        setFixedSize(350, 200);

        // Crear widgets
        usernameEdit = new QLineEdit(this);
        usernameEdit->setPlaceholderText("Usuario");

        passwordEdit = new QLineEdit(this);
        passwordEdit->setPlaceholderText("Contraseña");
        passwordEdit->setEchoMode(QLineEdit::Password);

        loginButton = new QPushButton("Iniciar Sesión", this);
        cancelButton = new QPushButton("Cancelar", this);

        statusLabel = new QLabel(this);
        statusLabel->setAlignment(Qt::AlignCenter);
        statusLabel->setStyleSheet("color: gray;");
        statusLabel->setText("Ingresa tus credenciales");

        // Layout principal
        QVBoxLayout *mainLayout = new QVBoxLayout(this);

        // Título
        QLabel *titleLabel = new QLabel("Bienvenido al Gestor de Horarios", this);
        titleLabel->setAlignment(Qt::AlignCenter);
        QFont titleFont = titleLabel->font();
        titleFont.setPointSize(12);
        titleFont.setBold(true);
        titleLabel->setFont(titleFont);

        mainLayout->addWidget(titleLabel);
        mainLayout->addWidget(usernameEdit);
        mainLayout->addWidget(passwordEdit);
        mainLayout->addWidget(statusLabel);

        // Layout de botones
        QHBoxLayout *buttonLayout = new QHBoxLayout();
        buttonLayout->addStretch();
        buttonLayout->addWidget(loginButton);
        buttonLayout->addWidget(cancelButton);
        buttonLayout->addStretch();

        mainLayout->addLayout(buttonLayout);

        // Conexiones de señales y slots
        connect(loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
        connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

        // Permitir login con Enter en los campos de texto
        connect(usernameEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
        connect(passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
    }

    LoginDialog::~LoginDialog()
    {
    }

    void LoginDialog::onLoginClicked()
    {
        QString user = usernameEdit->text().trimmed();
        QString pass = passwordEdit->text().trimmed();

        // Mock: aceptamos cualquier usuario y contraseña no vacíos
        if (!user.isEmpty() && !pass.isEmpty()) {
            statusLabel->setStyleSheet("color: green;");
            statusLabel->setText("✓ Inicio de sesión exitoso (mock)");
            // Aceptar el diálogo después de un pequeño delay para dar feedback
            QTimer::singleShot(300, this, &QDialog::accept);
        } else {
            statusLabel->setStyleSheet("color: red;");
            statusLabel->setText("✗ Usuario y contraseña son obligatorios");
        }
    }

} 