#pragma once

#include <QDialog>
#include <QTabWidget>
#include "../include/frontend/toggle_switch.hpp" // Asegúrate de que esta ruta sea la tuya

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    bool isDarkMode() const;

signals:
    void themeChanged(bool isDark); // Avisa en tiempo real cuando se mueve el switch

private slots:
    void cancelarCambios(); // Para revertir si se le da a "Descartar"

private:
    ToggleSwitch *m_darkModeSwitch;
    bool m_temaInicial; // Guarda el estado original
};