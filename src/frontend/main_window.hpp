#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QMainWindow>
#include <QStackedWidget>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void on_action_salir();
    void on_action_acerca_de();

private:
    QStackedWidget *m_central_stack;

    void setup_menu_bar();
    void setup_toolbar();
    void setup_status_bar();
};

#endif // MAIN_WINDOW_HPP
