#include "settings_dialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QApplication>
#include <QSettings>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QTimer>

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

    // USAR QSettings PARA PERSISTIR EL TEMA
    QSettings settings("LiceoRobertSerra", "GestorHorarios");
    m_temaInicial = settings.value("darkMode", false).toBool();
    m_darkModeSwitch->setChecked(m_temaInicial);

    // CONEXIÓN: Al mover el switch, SOLO actualiza vista previa (no guarda)
    connect(m_darkModeSwitch, &QCheckBox::toggled, this, [this](bool checked) {
        if (m_updatingFromSettings) return; // Si el cambio viene del sistema, lo ignoramos

        // 1. BLOQUEO: Desactivamos el botón temporalmente para evitar que el usuario haga "spam" de clics
        m_darkModeSwitch->setEnabled(false);

        // 2. DIBUJO INMEDIATO: Forzamos al botón a actualizar su propia bolita a la nueva posición
        m_darkModeSwitch->repaint();

        // 3. PAUSA RESPONSIVA: Le damos a Windows 50 milisegundos (aprox 3 fotogramas)
        // para que empuje el dibujo del botón a la pantalla ANTES de congelar el programa cargando el modo oscuro.
        QTimer::singleShot(50, this, [this, checked]() {
            emit themeChanged(checked); // Aplica todo el código pesado de los colores

            // 4. LIBERACIÓN: El modo oscuro ya cargó, volvemos a habilitar el botón
            m_darkModeSwitch->setEnabled(true);
        });
    });

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
    connect(btnGuardar, &QPushButton::clicked, this, [this]() {
        // Guardar en QSettings al aceptar
        QSettings settings("LiceoRobertSerra", "GestorHorarios");
        settings.setValue("darkMode", m_darkModeSwitch->isChecked());
        accept();
    });

    buttonsLayout->addWidget(btnDescartar);
    buttonsLayout->addWidget(btnGuardar);
    mainLayout->addLayout(buttonsLayout);
}

bool SettingsDialog::isDarkMode() const {
    return m_darkModeSwitch->isChecked();
}

void SettingsDialog::cancelarCambios() {
    // Si descarta, revertimos el tema al estado original y actualizamos visualmente el switch
    m_updatingFromSettings = true;
    emit themeChanged(m_temaInicial);
    m_darkModeSwitch->blockSignals(true);
    m_darkModeSwitch->setChecked(m_temaInicial);
    m_darkModeSwitch->blockSignals(false);
    m_updatingFromSettings = false;
    reject();
}