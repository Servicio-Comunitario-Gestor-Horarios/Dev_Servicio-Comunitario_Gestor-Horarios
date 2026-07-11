#ifndef DASHBOARD_WIDGET_HPP
#define DASHBOARD_WIDGET_HPP

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

class DashboardWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DashboardWidget(QWidget *parent = nullptr);
    ~DashboardWidget();

private slots:
    void mostrarDetalles();
    void resolverConflictos();

private:
    void setupUi();
    QWidget* crearTarjetaResumen(const QString &icono, const QString &label, const QString &valor);
    QWidget* crearPanelGeneracion();
    QWidget* crearPanelNotificaciones();

    QPushButton *btnVerDetalles;
    QPushButton *btnResolverConflictos;
};

#endif // DASHBOARD_WIDGET_HPP
