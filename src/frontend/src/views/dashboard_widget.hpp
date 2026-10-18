/**
 * @file dashboard_widget.hpp
 * @brief Widget del panel principal del dashboard (Frontend)
 */

#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

// Declaración adelantada
class NucleoDatos;
class GaugeWidget;

class DashboardWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DashboardWidget(QWidget *parent = nullptr);
    ~DashboardWidget();

    void actualizarTema(bool modoOscuro);
    void setNucleoDatos(NucleoDatos* nucleo);
    void actualizarEstadisticas();

private slots:
    void mostrarDetalles();
    void resolverConflictos();


private:
    void setupUi();
    QWidget* crearTarjetaResumen(const QString &icono, const QString &label, const QString &valor, QLabel*& outLabel);
    QWidget* crearPanelGeneracion();
    QWidget* crearPanelNotificaciones();

    // UI Dinámica para Notificaciones
    void agregarNotificacionUI(int numero, const QString& texto, bool destacada);
    void actualizarNotificacionesReales();

    // Punteros de Interfaz
    QPushButton *m_btnVerDetalles;
    QPushButton *m_btnResolverConflictos;
    GaugeWidget *m_gaugeWidget = nullptr;
    QVBoxLayout *m_layoutListaNotificaciones = nullptr;

    // Punteros de Datos
    NucleoDatos* m_nucleo = nullptr;
    QLabel* m_lblValorDocentes = nullptr;
    QLabel* m_lblValorAulas = nullptr;
    QLabel* m_lblValorMaterias = nullptr;
    QLabel* m_lblEstadoGeneracion = nullptr;
};