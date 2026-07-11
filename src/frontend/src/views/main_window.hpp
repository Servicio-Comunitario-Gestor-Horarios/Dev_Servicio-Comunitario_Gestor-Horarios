#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void mostrarInicio();
    void mostrarDocentes();

private:
    void configurarEstilosMenu();

    QStackedWidget *contenedorVistas;
    QPushButton *btnInicio;
    QPushButton *btnDocentes;
};

#endif // MAIN_WINDOW_HPP