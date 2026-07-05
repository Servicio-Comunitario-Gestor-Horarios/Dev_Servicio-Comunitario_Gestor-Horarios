#include "main_window.hpp"

#include <QAction>
#include <QApplication>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_central_stack(new QStackedWidget(this))
{
    setCentralWidget(m_central_stack);
    setup_menu_bar();
    setup_toolbar();
    setup_status_bar();
}

void MainWindow::setup_menu_bar()
{
    // ── Archivo ──
    QMenu *menu_archivo = menuBar()->addMenu(tr("&Archivo"));

    QAction *act_salir = menu_archivo->addAction(tr("&Salir"));
    act_salir->setShortcut(QKeySequence::Quit);
    connect(act_salir, &QAction::triggered, this, &MainWindow::on_action_salir);

    // ── Ver ──
    QMenu *menu_ver = menuBar()->addMenu(tr("&Ver"));
    menu_ver->addAction(tr("&Pantalla completa"))->setShortcut(QKeySequence(Qt::Key_F11));

    // ── Ayuda ──
    QMenu *menu_ayuda = menuBar()->addMenu(tr("&Ayuda"));

    QAction *act_acerca_de = menu_ayuda->addAction(tr("&Acerca de..."));
    connect(act_acerca_de, &QAction::triggered, this, &MainWindow::on_action_acerca_de);
}

void MainWindow::setup_toolbar()
{
    QToolBar *toolbar = addToolBar(tr("Principal"));
    toolbar->setMovable(false);

    toolbar->addAction(tr("Nuevo"));
    toolbar->addAction(tr("Abrir"));
    toolbar->addAction(tr("Guardar"));
}

void MainWindow::setup_status_bar()
{
    statusBar()->showMessage(tr("Listo"));
}

// ── Slots ──

void MainWindow::on_action_salir()
{
    QApplication::quit();
}

void MainWindow::on_action_acerca_de()
{
    QMessageBox::about(this,
        tr("Acerca de Gestor-Horarios"),
        tr("Gestor-Horarios v0.1.0\n\n"
           "Sistema de gestión de horarios con optimización\n"
           "basado en OR-Tools y Qt6."));
}
