#pragma once

#include <QMainWindow>
#include <QPushButton>
#include <QStackedWidget>
#include <QFrame>

class TeacherListWidget;
class ClassroomListWidget;
class SubjectListWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void mostrarInicio();
    void mostrarDocentes();
    void mostrarAulas();
    void mostrarAsignaturas();
    void mostrarGeneracion();
    void mostrarVisualizacion();

private:
    void setupSidebar();
    void setupCentralArea();
    void configurarEstilosMenu();

    QFrame *m_sidebar = nullptr;
    QStackedWidget *m_contenedorVistas = nullptr;
    QWidget *m_rightContainer = nullptr;

    QPushButton *m_btnInicio = nullptr;
    QPushButton *m_btnDocentes = nullptr;
    QPushButton *m_btnAulas = nullptr;
    QPushButton *m_btnAsignaturas = nullptr;
    QPushButton *m_btnGeneracion = nullptr;
    QPushButton *m_btnVisualizacion = nullptr;

    TeacherListWidget* m_teacherListWidget = nullptr;
    ClassroomListWidget* m_classroomListWidget = nullptr;
    SubjectListWidget* m_subjectListWidget = nullptr;
};