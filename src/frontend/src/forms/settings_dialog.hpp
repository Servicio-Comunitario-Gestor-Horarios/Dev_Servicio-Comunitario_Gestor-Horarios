#pragma once

#include <QDialog>
#include <QTabWidget>
#include "../include/frontend/toggle_switch.hpp"

class NucleoDatos;
class QSpinBox;
class QCheckBox;
class QComboBox;

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    bool isDarkMode() const;

    /** Núcleo para leer/guardar la duración global de la hora académica. */
    void setNucleoDatos(NucleoDatos* nucleo);

signals:
    void themeChanged(bool isDark); // Avisa en tiempo real cuando se mueve el switch

private slots:
    void cancelarCambios(); // Para revertir si se le da a "Descartar"

private:
    bool guardarConfiguracionAvanzada();

    ToggleSwitch *m_darkModeSwitch;
    QSpinBox *m_spinDuracion = nullptr;
    QSpinBox *m_spinDias = nullptr;
    QComboBox *m_comboMaxAnios = nullptr;
    QCheckBox *m_checkAulaFija = nullptr;

    NucleoDatos* m_nucleo = nullptr;
    bool m_temaInicial; // Guarda el estado original
    bool m_updatingFromSettings = false; // Evita bucles al actualizar desde settings
};
