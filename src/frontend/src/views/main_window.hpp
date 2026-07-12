#pragma once

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

    QStackedWidget *m_contenedorVistas;
    QPushButton *m_btnInicio;
    QPushButton *m_btnDocentes;
    QPushButton *m_btnAulas;
    QPushButton *m_btnAsignaturas;
    QPushButton *m_btnGeneracion;
    QPushButton *m_btnVisualizacion;
    QFrame *m_sidebar;
    QWidget *m_rightContainer;
};
