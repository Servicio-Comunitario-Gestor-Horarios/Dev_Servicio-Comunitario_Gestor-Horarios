/**
 * @file dashboard_widget.hpp
 * @brief Widget del panel principal del dashboard
 */

#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

class GaugeWidget; // Declaración adelantada

class DashboardWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DashboardWidget(QWidget *parent = nullptr);
    ~DashboardWidget();

    void actualizarTema(bool modoOscuro); // <--- FUNCIÓN AÑADIDA PARA CAMBIAR EL TEMA

private slots:
    void mostrarDetalles();
    void resolverConflictos();

private:
    void setupUi();
    QWidget* crearTarjetaResumen(const QString &icono, const QString &label, const QString &valor);
    QWidget* crearPanelGeneracion();
    QWidget* crearPanelNotificaciones();

    QPushButton *m_btnVerDetalles;
    QPushButton *m_btnResolverConflictos;
    GaugeWidget *m_gaugeWidget = nullptr; // <--- Referencia añadida para el medidor
};