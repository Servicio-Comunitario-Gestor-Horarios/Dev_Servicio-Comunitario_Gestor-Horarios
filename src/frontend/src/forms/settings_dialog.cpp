#include "settings_dialog.hpp"

#include <QApplication>
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTimer>
#include <QVBoxLayout>

#include <backend/services/NucleoDatos.hpp>
#include <backend/services/calculo_eventos.hpp>

namespace {
const char* kOrganizacion = "LiceoRobertSerra";
const char* kAplicacion = "GestorHorarios";
} // namespace

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Configuración del Sistema");
    setFixedSize(560, 440);

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

    // ── Preferencias generales ────────────────────────────────────────────
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
    QSettings settings(kOrganizacion, kAplicacion);
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

    // ── Avanzado: generación de horarios ──────────────────────────────────
    QWidget *avanzadoTab = new QWidget();
    QVBoxLayout *avanzadoLayout = new QVBoxLayout(avanzadoTab);
    avanzadoLayout->setContentsMargins(10, 20, 10, 10);
    avanzadoLayout->setSpacing(14);

    QFormLayout *form = new QFormLayout();
    form->setHorizontalSpacing(18);
    form->setVerticalSpacing(12);
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    m_spinDuracion = new QSpinBox(avanzadoTab);
    m_spinDuracion->setRange(30, 90);
    m_spinDuracion->setSuffix(" min");
    m_spinDuracion->setValue(35);

    m_spinDias = new QSpinBox(avanzadoTab);
    m_spinDias->setRange(1, 7);
    m_spinDias->setValue(5);

    m_checkAulaFija = new QCheckBox(
        "Usar el aula fija asignada a cada curso", avanzadoTab);
    m_checkAulaFija->setChecked(true);

    form->addRow("Duración de la hora académica (bloque)", m_spinDuracion);
    form->addRow("Días a planificar", m_spinDias);
    form->addRow(QString(), m_checkAulaFija);

    avanzadoLayout->addLayout(form);

    QLabel *ayuda = new QLabel(
        "Estos valores se usan al generar el horario y al guardar un preset. "
        "La duración es global y se comprueba contra la ventana de cada turno; "
        "las franjas por día se calculan sumando las de los turnos.",
        avanzadoTab);
    ayuda->setWordWrap(true);
    ayuda->setStyleSheet("color: #64748b; font-size: 12px;");
    avanzadoLayout->addWidget(ayuda);
    avanzadoLayout->addStretch();

    tabWidget->addTab(generalTab, "Preferencias Generales");
    tabWidget->addTab(avanzadoTab, "Avanzado");
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
        QSettings settings(kOrganizacion, kAplicacion);
        if (!guardarConfiguracionAvanzada())
            return;
        settings.setValue("darkMode", m_darkModeSwitch->isChecked());
        settings.setValue("generacion/dias", m_spinDias->value());
        settings.setValue("generacion/aulaFija", m_checkAulaFija->isChecked());
        accept();
    });

    buttonsLayout->addWidget(btnDescartar);
    buttonsLayout->addWidget(btnGuardar);
    mainLayout->addLayout(buttonsLayout);

    // Días y aula fija son preferencias de la interfaz (QSettings); la duración
    // se lee del núcleo cuando se inyecta con `setNucleoDatos`.
    QSettings settingsUi(kOrganizacion, kAplicacion);
    m_spinDias->setValue(qBound(1, settingsUi.value("generacion/dias", 5).toInt(), 7));
    m_checkAulaFija->setChecked(settingsUi.value("generacion/aulaFija", true).toBool());
}

bool SettingsDialog::isDarkMode() const {
    return m_darkModeSwitch->isChecked();
}

void SettingsDialog::setNucleoDatos(NucleoDatos* nucleo) {
    m_nucleo = nucleo;
    if (!m_nucleo)
        return;
    if (const auto config = m_nucleo->configuracionHoraria(); config.ok)
        m_spinDuracion->setValue(qBound(30, config.valor.duracionMinutos, 90));
}

bool SettingsDialog::guardarConfiguracionAvanzada() {
    if (!m_nucleo)
        return true;

    const int duracion = m_spinDuracion->value();

    // La duración debe encajar en TODOS los turnos: el generador la aplica
    // siempre y un valor incompatible dejaría la generación inválida.
    if (const auto turnos = m_nucleo->listarTurnos(); turnos.ok) {
        for (const TurnoDTO& turno : turnos.valor) {
            const QString error = errorDeEncaje(turno, duracion);
            if (!error.isEmpty()) {
                QMessageBox::warning(this, "Duración incompatible", error);
                return false;
            }
        }
    }

    const auto guardado = m_nucleo->guardarConfiguracionHoraria(duracion);
    if (!guardado.ok) {
        QMessageBox::warning(this, "No se pudo guardar la duración",
                             guardado.mensajeError);
        return false;
    }
    return true;
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
