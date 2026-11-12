/**
 * @file dashboard_widget.hpp
 * @brief Ventana de bienvenida del sistema (panel principal).
 *
 * Muestra el logo del liceo, una bienvenida con una frase sobre la enseñanza y
 * el estudio, y los totales de docentes, aulas y materias al pie.
 */

#pragma once

#include <QWidget>
#include <QLabel>

class NucleoDatos;

class DashboardWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DashboardWidget(QWidget *parent = nullptr);
    ~DashboardWidget() override = default;

    void actualizarTema(bool modoOscuro);
    void setNucleoDatos(NucleoDatos* nucleo);
    void actualizarEstadisticas();

private:
    void setupUi();
    QWidget* crearTarjetaResumen(const QString& icono, const QString& label,
                                 QLabel*& outLabel);

    NucleoDatos* m_nucleo = nullptr;
    QLabel* m_lblValorDocentes = nullptr;
    QLabel* m_lblValorAulas = nullptr;
    QLabel* m_lblValorMaterias = nullptr;
};
