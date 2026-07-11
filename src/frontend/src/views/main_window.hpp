#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QFrame>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void mostrarInicio();
    void mostrarDocentes();
    void mostrarAulas();
    void mostrarAsignaturas();
    void mostrarGeneracion();
    void mostrarVisualizacion();

private:
    void configurarEstilosMenu();
    void setupSidebar();
    void setupCentralArea();

    QStackedWidget *contenedorVistas;
    QPushButton *btnInicio;
    QPushButton *btnDocentes;
    QPushButton *btnAulas;
    QPushButton *btnAsignaturas;
    QPushButton *btnGeneracion;
    QPushButton *btnVisualizacion;
    QFrame *sidebar;
    QWidget *rightContainer;
};

#endif // MAIN_WINDOW_HPP