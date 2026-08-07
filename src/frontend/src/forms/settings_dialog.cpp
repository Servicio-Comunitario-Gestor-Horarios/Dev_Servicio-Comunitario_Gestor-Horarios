#include "settings_dialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QApplication>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Configuración del Sistema");
    setFixedSize(500, 350);

    setWindowModality(Qt::ApplicationModal);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    QHBoxLayout *headerLayout = new QHBoxLayout();
    QLabel *titleLabel = new QLabel("⚙️ Configuración del Sistema", this);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    QTabWidget *tabWidget = new QTabWidget(this);
    tabWidget->setStyleSheet("QTabWidget::pane { border-top: 1px solid #e2e8f0; }");

    QWidget *generalTab = new QWidget();
    QVBoxLayout *generalLayout = new QVBoxLayout(generalTab);
    generalLayout->setContentsMargins(10, 20, 10, 10);

    QHBoxLayout *themeRow = new QHBoxLayout();
    QVBoxLayout *themeTextLayout = new QVBoxLayout();

    QLabel *themeTitle = new QLabel("Modo Oscuro", this);
    themeTitle->setStyleSheet("font-weight: bold; font-size: 14px;");
    QLabel *themeDesc = new QLabel("Apariencia de la interfaz", this);
    themeDesc->setStyleSheet("color: #64748b; font-size: 12px;");

    themeTextLayout->addWidget(themeTitle);
    themeTextLayout->addWidget(themeDesc);
    themeTextLayout->setSpacing(2);

    m_darkModeSwitch = new ToggleSwitch(this);

    // Guardar el estado inicial y configurar el switch
    m_temaInicial = qApp->styleSheet().contains("background-color: #0f172a");
    m_darkModeSwitch->setChecked(m_temaInicial);

    // CONEXIÓN CLAVE: Al mover el switch, emite la señal al instante
    connect(m_darkModeSwitch, &QCheckBox::toggled, this, &SettingsDialog::themeChanged);

    themeRow->addLayout(themeTextLayout);
    themeRow->addStretch();
    themeRow->addWidget(m_darkModeSwitch);

    generalLayout->addLayout(themeRow);
    generalLayout->addStretch();

    tabWidget->addTab(generalTab, "Preferencias Generales");
    tabWidget->addTab(new QWidget(), "Avanzado");
    mainLayout->addWidget(tabWidget);

    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    buttonsLayout->addStretch();

    QPushButton *btnDescartar = new QPushButton("Descartar", this);
    btnDescartar->setStyleSheet("padding: 8px 16px; border: 1px solid #cbd5e1; border-radius: 4px; background: white; color: #475569;");
    btnDescartar->setCursor(Qt::PointingHandCursor);
    connect(btnDescartar, &QPushButton::clicked, this, &SettingsDialog::cancelarCambios);

    QPushButton *btnGuardar = new QPushButton("Guardar Configuración", this);
    btnGuardar->setStyleSheet("padding: 8px 16px; border: none; border-radius: 4px; background: #243c8a; color: white; font-weight: bold;");
    btnGuardar->setCursor(Qt::PointingHandCursor);
    connect(btnGuardar, &QPushButton::clicked, this, &QDialog::accept);

    buttonsLayout->addWidget(btnDescartar);
    buttonsLayout->addWidget(btnGuardar);
    mainLayout->addLayout(buttonsLayout);
}

bool SettingsDialog::isDarkMode() const {
    return m_darkModeSwitch->isChecked();
}

void SettingsDialog::cancelarCambios() {
    // Si descarta, revertimos el tema al estado original que tenía al abrir la ventana
    emit themeChanged(m_temaInicial);
    reject();
}