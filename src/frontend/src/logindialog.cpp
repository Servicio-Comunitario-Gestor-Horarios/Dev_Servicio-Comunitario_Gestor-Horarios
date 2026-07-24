#include "logindialog.h"
#include <QMessageBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPixmap>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QPainterPath>
#include <QAction>
#include <QStyle>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Liceo Nacional Robert Serra - Acceso");
    resize(1280, 800);

    QFont modernFont("Segoe UI", 10);
    modernFont.setStyleHint(QFont::SansSerif);
    this->setFont(modernFont);

    // ==========================================
    // ESTRUCTURA PRINCIPAL DE LA VENTANA
    // ==========================================
    QVBoxLayout *windowLayout = new QVBoxLayout(this);
    windowLayout->setContentsMargins(0, 0, 0, 0);
    windowLayout->setSpacing(0);

    // ==========================================
    // 1. ENCABEZADO
    // ==========================================
    QFrame *headerFrame = new QFrame(this);
    headerFrame->setFixedHeight(95);
    headerFrame->setObjectName("Header");

    QHBoxLayout *headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setContentsMargins(40, 0, 40, 0);
    headerLayout->setSpacing(25);

    QLabel *logoLabel = new QLabel(headerFrame);
    QPixmap logoPixmap(":/logo.png");
    if(!logoPixmap.isNull()) {
        logoLabel->setPixmap(logoPixmap.scaled(75, 75, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        logoLabel->setText("LOGO");
    }
    headerLayout->addWidget(logoLabel);

    QFrame *vLine = new QFrame(headerFrame);
    vLine->setFrameShape(QFrame::VLine);
    vLine->setFrameShadow(QFrame::Plain);
    vLine->setStyleSheet("color: #e2e8f0;");
    headerLayout->addWidget(vLine);

    QLabel *schoolTitle = new QLabel("Liceo Nacional <span style='color:#1e3a8a; font-weight:bold;'>Robert Serra</span>", headerFrame);
    schoolTitle->setStyleSheet("font-size: 25px; color: #334155;");
    headerLayout->addWidget(schoolTitle);

    headerLayout->addStretch();

    QLabel *capIcon = new QLabel("🎓", headerFrame);
    capIcon->setAlignment(Qt::AlignCenter);
    capIcon->setFixedSize(60, 60);
    capIcon->setStyleSheet("background-color: #eff6ff; color: #1e3a8a; font-size: 28px; border-radius: 30px;");
    headerLayout->addWidget(capIcon);

    windowLayout->addWidget(headerFrame);

    // ==========================================
    // 2. ÁREA CENTRAL (TARJETA DE LOGIN)
    // ==========================================
    QFrame *bodyFrame = new QFrame(this);
    bodyFrame->setObjectName("Body");

    QVBoxLayout *bodyLayout = new QVBoxLayout(bodyFrame);
    bodyLayout->setContentsMargins(0, 0, 0, 0);

    QFrame *loginCard = new QFrame(bodyFrame);
    loginCard->setFixedSize(420, 520);
    loginCard->setObjectName("LoginCard");

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(30);
    shadow->setColor(QColor(0, 0, 0, 15));
    shadow->setOffset(0, 10);
    loginCard->setGraphicsEffect(shadow);

    QVBoxLayout *cardLayout = new QVBoxLayout(loginCard);
    cardLayout->setContentsMargins(45, 40, 45, 40);
    cardLayout->setSpacing(8);

    QLabel *iconLabel = new QLabel("🛡️", loginCard);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setFixedSize(64, 64);
    iconLabel->setStyleSheet("font-size: 26px; background-color: #eff6ff; border-radius: 32px; border: none;");

    QHBoxLayout *iconLayout = new QHBoxLayout();
    iconLayout->addStretch();
    iconLayout->addWidget(iconLabel);
    iconLayout->addStretch();
    cardLayout->addLayout(iconLayout);

    cardLayout->addSpacing(5);

    QLabel *titleLabel = new QLabel("Sistema de Gestión de\nHorarios", loginCard);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 28px; font-weight: 700; color: #0f172a; border: none; background: transparent; line-height: 1.1;");
    cardLayout->addWidget(titleLabel);

    QLabel *subtitleLabel = new QLabel("Ingresa tus credenciales para acceder", loginCard);
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setStyleSheet("font-size: 16px; color: #64748b; border: none; background: transparent;");
    cardLayout->addWidget(subtitleLabel);

    cardLayout->addSpacing(15);

    // ==========================================
    // DIBUJO VECTORIAL DE ÍCONOS
    // ==========================================
    auto crearIcono = [](const QString& tipo) -> QIcon {
        QPixmap pix(24, 24);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        p.translate(0.5, 0.5);
        p.setPen(QPen(QColor("#94a3b8"), 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);

        if (tipo == "user") {
            p.drawEllipse(QRectF(6, 2, 12, 12));
            QPainterPath path;
            path.moveTo(3, 22);
            path.quadTo(12, 12, 21, 22);
            p.drawPath(path);
        } else if (tipo == "lock") {
            p.drawRoundedRect(4, 11, 15, 11, 2, 2);
            QPainterPath path;
            path.moveTo(7, 11);
            path.lineTo(7, 7);
            path.arcTo(7, 2, 9, 10, 180, -180);
            path.lineTo(16, 11);
            p.drawPath(path);
            p.drawLine(11.5, 14, 11.5, 17);
        } else if (tipo == "eye") {
            QPainterPath path;
            path.moveTo(2, 12);
            path.quadTo(12, 3, 22, 12);
            path.quadTo(12, 21, 2, 12);
            p.drawPath(path);
            p.drawEllipse(QRectF(8, 8, 8, 8));
        } else if (tipo == "eye_closed") {
            QPainterPath path;
            path.moveTo(2, 12);
            path.quadTo(12, 3, 22, 12);
            path.quadTo(12, 21, 2, 12);
            p.drawPath(path);
            p.drawEllipse(QRectF(8, 8, 8, 8));
            p.drawLine(3, 3, 21, 21);
        }
        return QIcon(pix);
    };

    // ==========================================
    // FORMULARIOS CON ÍCONOS INTEGRADOS
    // ==========================================
    QLabel *userLabel = new QLabel("Usuario", loginCard);
    userLabel->setStyleSheet("font-size: 12px; font-weight: 600; color: #475569; border: none; background: transparent;");
    cardLayout->addWidget(userLabel);

    usernameEdit = new QLineEdit(loginCard);
    usernameEdit->setPlaceholderText("Ej: admin123");
    usernameEdit->setFixedHeight(44);
    usernameEdit->addAction(crearIcono("user"), QLineEdit::LeadingPosition);
    cardLayout->addWidget(usernameEdit);

    cardLayout->addSpacing(6);

    QLabel *passLabel = new QLabel("Contraseña", loginCard);
    passLabel->setStyleSheet("font-size: 12px; font-weight: 600; color: #475569; border: none; background: transparent;");
    cardLayout->addWidget(passLabel);

    passwordEdit = new QLineEdit(loginCard);
    passwordEdit->setPlaceholderText("••••••••");
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setFixedHeight(44);
    passwordEdit->addAction(crearIcono("lock"), QLineEdit::LeadingPosition);

    QAction *eyeAction = passwordEdit->addAction(crearIcono("eye_closed"), QLineEdit::TrailingPosition);
    cardLayout->addWidget(passwordEdit);

    connect(eyeAction, &QAction::triggered, this, [this, eyeAction, crearIcono]() {
        if (passwordEdit->echoMode() == QLineEdit::Password) {
            passwordEdit->setEchoMode(QLineEdit::Normal);
            eyeAction->setIcon(crearIcono("eye"));
        } else {
            passwordEdit->setEchoMode(QLineEdit::Password);
            eyeAction->setIcon(crearIcono("eye_closed"));
        }
    });

    statusLabel = new QLabel(" ", loginCard);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("font-size: 12px; border: none; background: transparent;");
    cardLayout->addWidget(statusLabel);

    loginButton = new QPushButton("Iniciar Sesión  →]", loginCard);
    loginButton->setFixedHeight(44);
    loginButton->setCursor(Qt::PointingHandCursor);
    cardLayout->addWidget(loginButton);

    bodyLayout->addStretch();
    bodyLayout->addWidget(loginCard, 0, Qt::AlignHCenter);
    bodyLayout->addStretch();

    windowLayout->addWidget(bodyFrame, 1);

    // ==========================================
    // 3. FOOTER
    // ==========================================
    QLabel *footerLabel = new QLabel("¿Necesitas ayuda? <span style='color:#1e3a8a; font-weight:600;'>Contacta a soporte técnico</span>", this);
    footerLabel->setAlignment(Qt::AlignCenter);
    footerLabel->setStyleSheet("font-size: 12px; color: #64748b; padding-bottom: 25px; background: transparent; border: none;");
    windowLayout->addWidget(footerLabel);

    // ==========================================
    // HOJA DE ESTILOS (QSS)
    // ==========================================
    this->setStyleSheet(R"(
        QDialog {
            background-color: #f8fafc;
        }
        QFrame#Header {
            background-color: white;
            border-bottom: 1px solid #f1f5f9;
        }
        QFrame#Body {
            background-color: transparent;
        }
        QFrame#LoginCard {
            background-color: white;
            border-radius: 12px;
            border: none;
            border-top: 8px solid #1e3a8a;
        }
        QLineEdit {
            background-color: #f8fafc;
            border: 1px solid #e2e8f0;
            border-radius: 6px;
            padding: 0 10px;
            color: #0f172a;
            font-size: 13px;
            outline: none;
        }
        QLineEdit:focus {
            border: 1px solid #1e3a8a;
            background-color: white;
        }
        QLineEdit[error="true"] {
            border: 1px solid #ef4444;
            background-color: #fef2f2;
        }
        QLineEdit[error="true"]:focus {
            border: 1px solid #dc2626;
            background-color: white;
        }
        QLineEdit QToolButton {
            background: transparent;
            border: none;
            margin: 0px;
            padding: 0px;
        }
        QPushButton {
            background-color: #1e3a8a;
            color: white;
            font-weight: 600;
            border-radius: 6px;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #162a69;
        }
        QPushButton:pressed {
            background-color: #0f1c47;
        }
    )");

    connect(loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(usernameEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
    connect(passwordEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
    this->setFocus();
}

LoginDialog::~LoginDialog() {}

void LoginDialog::onLoginClicked()
{
    QString user = usernameEdit->text().trimmed();
    QString pass = passwordEdit->text().trimmed();

    usernameEdit->setProperty("error", user.isEmpty());
    usernameEdit->style()->unpolish(usernameEdit);
    usernameEdit->style()->polish(usernameEdit);

    passwordEdit->setProperty("error", pass.isEmpty());
    passwordEdit->style()->unpolish(passwordEdit);
    passwordEdit->style()->polish(passwordEdit);

    if (!user.isEmpty() && !pass.isEmpty()) {
        statusLabel->setStyleSheet("color: #10b981; font-weight: 600; border: none; background: transparent;");
        statusLabel->setText("✓ Acceso concedido");
        QTimer::singleShot(300, this, &QDialog::accept);
    } else {
        statusLabel->setStyleSheet("color: #ef4444; font-weight: 600; border: none; background: transparent;");
        statusLabel->setText("✗ Por favor, completa todos los campos");
    }
}