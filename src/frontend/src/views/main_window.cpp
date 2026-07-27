/* Proyecto:       Sistema de Gestión de Horarios
 * Archivo:        [main_window.cpp]
 * Autor:          Paola Peña
 * Fecha:          10 de Julio de 2026
 * Descripción:    Implementación de la interfaz gráfica (Frontend) utilizando
 *                 C++ y Qt6 para el módulo de gestión de docentes. Incluye el
 *                 diseño de la vista principal, la tabla de listado de docentes,
 *                 y el formulario de registro con validación de datos.
 */


#include "main_window.hpp"
#include "teacher_list_widget.hpp"
#include "subject_list_widget.hpp"
#include "dashboard_widget.hpp"
#include "view_placeholder.hpp"
#include "classroom_list_widget.hpp"
#include <middleware/internalclient.h>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Liceo Nacional Robert Serra - Gestión");
    resize(1280, 720);

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setObjectName("FondoPrincipal");
    centralWidget->setStyleSheet("#FondoPrincipal { background-color: #f4f5f7; }");

    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    setupSidebar();
    setupCentralArea();

    mainLayout->addWidget(m_sidebar);
    mainLayout->addWidget(m_rightContainer);

    setCentralWidget(centralWidget);

    // Crear cliente y pasar a los widgets
    InternalClient* client = new InternalClient(this);
    if (m_teacherListWidget) m_teacherListWidget->setClient(client);
    if (m_classroomListWidget) m_classroomListWidget->setClient(client);
    if (m_subjectListWidget) m_subjectListWidget->setClient(client);

    // Conectar botones
    connect(m_btnInicio, &QPushButton::clicked, this, &MainWindow::mostrarInicio);
    connect(m_btnDocentes, &QPushButton::clicked, this, &MainWindow::mostrarDocentes);
    connect(m_btnAulas, &QPushButton::clicked, this, &MainWindow::mostrarAulas);
    connect(m_btnAsignaturas, &QPushButton::clicked, this, &MainWindow::mostrarAsignaturas);
    connect(m_btnGeneracion, &QPushButton::clicked, this, &MainWindow::mostrarGeneracion);
    connect(m_btnVisualizacion, &QPushButton::clicked, this, &MainWindow::mostrarVisualizacion);

    mostrarInicio();
}

void MainWindow::setupSidebar() {
    m_sidebar = new QFrame(this);
    m_sidebar->setObjectName("Sidebar");
    m_sidebar->setFixedWidth(250);
    m_sidebar->setStyleSheet("#Sidebar { background-color: white; border-right: 1px solid #e0e0e0; }");

    QVBoxLayout *sidebarLayout = new QVBoxLayout(m_sidebar);
    sidebarLayout->setAlignment(Qt::AlignTop);
    sidebarLayout->setContentsMargins(0, 20, 0, 20);

    QLabel *logoLabel = new QLabel("Liceo Nacional\nRobert Serra", m_sidebar);
    logoLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: #1a237e; border: none; background: transparent; margin-left: 20px;");

    m_btnInicio = new QPushButton("🏠 Inicio", m_sidebar);
    m_btnDocentes = new QPushButton("🎓 Docentes", m_sidebar);
    m_btnAulas = new QPushButton("🏫 Aulas", m_sidebar);
    m_btnAsignaturas = new QPushButton("📚 Asignaturas", m_sidebar);
    m_btnGeneracion = new QPushButton("📅 Generación de horario", m_sidebar);
    m_btnVisualizacion = new QPushButton("👁️ Visualización de horario", m_sidebar);

    QList<QPushButton*> botones = {m_btnInicio, m_btnDocentes, m_btnAulas, m_btnAsignaturas, m_btnGeneracion, m_btnVisualizacion};
    for (auto btn : botones) {
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet("text-align: left; padding: 10px 20px; border: none; font-size: 14px; color: #4b5563; background-color: transparent; border-left: 4px solid transparent;");
    }

    QLabel *tituloAcademico = new QLabel("ACADÉMICO", m_sidebar);
    tituloAcademico->setStyleSheet("color: #6b7280; font-size: 11px; font-weight: bold; border: none; background: transparent; margin-left: 20px; margin-top: 15px;");
    QLabel *tituloHorarios = new QLabel("HORARIOS", m_sidebar);
    tituloHorarios->setStyleSheet("color: #6b7280; font-size: 11px; font-weight: bold; border: none; background: transparent; margin-left: 20px; margin-top: 15px;");

    sidebarLayout->addWidget(logoLabel);
    sidebarLayout->addSpacing(20);
    sidebarLayout->addWidget(m_btnInicio);
    sidebarLayout->addWidget(tituloAcademico);
    sidebarLayout->addWidget(m_btnDocentes);
    sidebarLayout->addWidget(m_btnAulas);
    sidebarLayout->addWidget(m_btnAsignaturas);
    sidebarLayout->addWidget(tituloHorarios);
    sidebarLayout->addWidget(m_btnGeneracion);
    sidebarLayout->addWidget(m_btnVisualizacion);
}

void MainWindow::setupCentralArea() {
    m_rightContainer = new QWidget(this);
    m_rightContainer->setObjectName("RightContainer");
    m_rightContainer->setStyleSheet("#RightContainer { background: transparent; }");

    QVBoxLayout *rightLayout = new QVBoxLayout(m_rightContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    // Header superior
    QFrame *topHeader = new QFrame(this);
    topHeader->setObjectName("TopHeader");
    topHeader->setFixedHeight(60);
    topHeader->setStyleSheet("#TopHeader { background-color: white; border-bottom: 1px solid #e0e0e0; }");

    QHBoxLayout *headerLayout = new QHBoxLayout(topHeader);
    QLabel *menuIcon = new QLabel("☰", topHeader);
    menuIcon->setStyleSheet("font-size: 20px; border: none; background: transparent; padding-left: 15px;");
    QLabel *userProfile = new QLabel("A  Header Subhead", topHeader);
    userProfile->setStyleSheet("border: none; background: transparent; color: #555; padding-right: 15px;");
    headerLayout->addWidget(menuIcon);
    headerLayout->addStretch();
    headerLayout->addWidget(userProfile);

    m_contenedorVistas = new QStackedWidget(this);

    // Índice 0: Dashboard
    m_contenedorVistas->addWidget(new DashboardWidget(this));

    // Índice 1: Docentes
    m_teacherListWidget = new TeacherListWidget(this);
    m_contenedorVistas->addWidget(m_teacherListWidget);

    // Índice 2: Aulas
    m_classroomListWidget = new ClassroomListWidget(this);
    m_contenedorVistas->addWidget(m_classroomListWidget);

    // Índice 3: Asignaturas
    m_subjectListWidget = new SubjectListWidget(this);
    m_contenedorVistas->addWidget(m_subjectListWidget);

    // Índice 4: Generación de horarios
    m_contenedorVistas->addWidget(new ViewPlaceholder("Generación de Horarios", this));

    // Índice 5: Visualización de horarios
    m_contenedorVistas->addWidget(new ViewPlaceholder("Visualización de Horarios", this));

    rightLayout->addWidget(topHeader);
    rightLayout->addWidget(m_contenedorVistas);
}

void MainWindow::mostrarInicio() {
    m_contenedorVistas->setCurrentIndex(0);
    configurarEstilosMenu();
    m_btnInicio->setStyleSheet(
        "text-align: left; padding: 10px 20px; font-size: 14px; font-weight: bold;"
        "background-color: #e8eaf6; color: #1a237e;"
        "border: none; border-left: 4px solid #1a237e;"
        );
}

void MainWindow::mostrarDocentes() {
    m_contenedorVistas->setCurrentIndex(1);
    configurarEstilosMenu();
    m_btnDocentes->setStyleSheet(
        "text-align: left; padding: 10px 20px; font-size: 14px; font-weight: bold;"
        "background-color: #e8eaf6; color: #1a237e;"
        "border: none; border-left: 4px solid #1a237e;"
        );
}

void MainWindow::mostrarAulas() {
    m_contenedorVistas->setCurrentIndex(2);
    configurarEstilosMenu();
    m_btnAulas->setStyleSheet(
        "text-align: left; padding: 10px 20px; font-size: 14px; font-weight: bold;"
        "background-color: #e8eaf6; color: #1a237e;"
        "border: none; border-left: 4px solid #1a237e;"
        );
}

void MainWindow::mostrarAsignaturas() {
    m_contenedorVistas->setCurrentIndex(3);
    configurarEstilosMenu();
    m_btnAsignaturas->setStyleSheet(
        "text-align: left; padding: 10px 20px; font-size: 14px; font-weight: bold;"
        "background-color: #e8eaf6; color: #1a237e;"
        "border: none; border-left: 4px solid #1a237e;"
        );
}

void MainWindow::mostrarGeneracion() {
    m_contenedorVistas->setCurrentIndex(4);
    configurarEstilosMenu();
    m_btnGeneracion->setStyleSheet(
        "text-align: left; padding: 10px 20px; font-size: 14px; font-weight: bold;"
        "background-color: #e8eaf6; color: #1a237e;"
        "border: none; border-left: 4px solid #1a237e;"
        );
}

void MainWindow::mostrarVisualizacion() {
    m_contenedorVistas->setCurrentIndex(5);
    configurarEstilosMenu();
    m_btnVisualizacion->setStyleSheet(
        "text-align: left; padding: 10px 20px; font-size: 14px; font-weight: bold;"
        "background-color: #e8eaf6; color: #1a237e;"
        "border: none; border-left: 4px solid #1a237e;"
        );
}

void MainWindow::configurarEstilosMenu() {
    QString estiloInactivo = "text-align: left; padding: 10px 20px; border: none; font-size: 14px; color: #4b5563; background-color: transparent; border-left: 4px solid transparent;";
    m_btnInicio->setStyleSheet(estiloInactivo);
    m_btnDocentes->setStyleSheet(estiloInactivo);
    m_btnAulas->setStyleSheet(estiloInactivo);
    m_btnAsignaturas->setStyleSheet(estiloInactivo);
    m_btnGeneracion->setStyleSheet(estiloInactivo);
    m_btnVisualizacion->setStyleSheet(estiloInactivo);
}